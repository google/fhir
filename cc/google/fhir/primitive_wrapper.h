/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GOOGLE_FHIR_PRIMITIVE_WRAPPER_H_
#define GOOGLE_FHIR_PRIMITIVE_WRAPPER_H_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/time/time.h"
#include "google/fhir/codes.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/extensions.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/json_format_results.h"
#include "google/fhir/json/json_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/type_macros.h"
#include "google/fhir/util.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "re2/re2.h"

namespace google {
namespace fhir {

absl::Status BuildHasNoValueExtension(::google::protobuf::Message* extension);

namespace primitives_internal {

using ::absl::FailedPreconditionError;
using ::absl::InvalidArgumentError;
using ::google::fhir::ClearExtensionsWithUrl;
using ::google::protobuf::Descriptor;
using ::google::protobuf::EnumDescriptor;
using ::google::protobuf::EnumValueDescriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

// Tests whether or not a message has the "PrimitiveHasNoValue" extension, which
// indicates that a primitive has no value and only extensions.
// This is necessary because proto3 does not differentiate between a primitive
// being absent, and a primitive having the "default" value (e.g., zero for an
// int, or the empty string).
// Returns an error status if the message has more than one PrimitiveHasNoValue
// extension.
absl::StatusOr<bool> HasPrimitiveHasNoValue(const Message& message);

static const char* kPrimitiveHasNoValueUrl =
    "https://g.co/fhir/StructureDefinition/primitiveHasNoValue";
static const char* kBinarySeparatorStrideUrl =
    "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride";

const std::vector<const char*>* const kConversionOnlyExtensionUrls =
    new std::vector<const char*>{
        kPrimitiveHasNoValueUrl,
        kBinarySeparatorStrideUrl,
    };

class PrimitiveWrapper {
 public:
  virtual ~PrimitiveWrapper() {}
  // Merges this wrapper into a target message.
  // Returns a ParseResult indicating if the parse succeeded, or a Status
  // if it encountered an unexpected error or the error reporter returned a
  // status (e.g., from fast-fail error reporters).
  virtual absl::StatusOr<ParseResult> MergeInto(
      ::google::protobuf::Message* target,
      const ScopedErrorReporter& error_reporter) const = 0;
  // Parses a FhirJson object into this wrapper.
  // Returns a ParseResult indicating if the parse succeeded, or a Status
  // if it encountered an unexpected error or the error reporter returned a
  // status (e.g., from fast-fail error reporters).
  virtual absl::StatusOr<ParseResult> Parse(
      const internal::FhirJson& json, const absl::TimeZone& default_time_zone,
      const ScopedErrorReporter& error_reporter) = 0;
  virtual absl::Status Wrap(const ::google::protobuf::Message&) = 0;
  virtual bool HasElement() const = 0;
  virtual absl::StatusOr<std::unique_ptr<::google::protobuf::Message>> GetElement()
      const = 0;

  // Validates the wrapped proto.
  // Any issues encountered are reported to the error_reporter, but do not
  // cause a status failure.
  // Status failures are a result of unexpected errors or if the error reporter
  // returns a status (e.g., from fast-fail error reporters).
  virtual absl::Status ValidateProto(
      const ScopedErrorReporter& error_reporter) const = 0;

  absl::StatusOr<std::string> ToValueString() const {
    static const char* kNullString = "null";
    if (HasValue()) {
      return ToNonNullValueString();
    }
    return absl::StrCat(kNullString);
  }

 protected:
  virtual bool HasValue() const = 0;
  virtual absl::StatusOr<std::string> ToNonNullValueString() const = 0;
};

template <typename T>
class SpecificWrapper : public PrimitiveWrapper {
 public:
  absl::StatusOr<ParseResult> MergeInto(
      Message* target,
      const ScopedErrorReporter& error_reporter) const override {
    if (T::descriptor()->full_name() != target->GetDescriptor()->full_name()) {
      return InvalidArgumentError(absl::StrCat(
          "Type mismatch in SpecificWrapper#MergeInto: Attempted to merge ",
          T::descriptor()->full_name(), " into ",
          target->GetDescriptor()->full_name()));
    }
    target->MergeFrom(*wrapped_);
    return ParseResult::kSucceeded;
  }

  absl::Status Wrap(const ::google::protobuf::Message& message) override {
    if (T::descriptor()->full_name() != message.GetDescriptor()->full_name()) {
      return InvalidArgumentError(absl::StrCat(
          "Type mismatch in SpecificWrapper#Wrap: Attempted to wrap ",
          message.GetDescriptor()->full_name(), " with wrapper for ",
          T::descriptor()->full_name()));
    }
    wrapped_ = dynamic_cast<const T*>(&message);
    return absl::OkStatus();
  }

  const T* GetWrapped() const { return wrapped_; }

 protected:
  const T* wrapped_;
  std::unique_ptr<T> managed_memory_;

  void WrapAndManage(std::unique_ptr<T>&& t) {
    managed_memory_ = std::move(t);
    wrapped_ = managed_memory_.get();
  }
};

// Note that Xhtml types require a special case that sits between
// SpecificWrapper and ExtensibleWrapper because they do not support extensions.
template <typename XhtmlLike>
class XhtmlWrapper : public SpecificWrapper<XhtmlLike> {
 public:
  bool HasValue() const override { return true; }

  bool HasElement() const override { return this->GetWrapped()->has_id(); }

  // Xhtml can't have extensions, it's always valid
  absl::Status ValidateProto(
      const ScopedErrorReporter& error_reporter) const override {
    return absl::OkStatus();
  }

  absl::StatusOr<std::unique_ptr<::google::protobuf::Message>> GetElement()
      const override {
    std::unique_ptr<Message> element =
        absl::WrapUnique(this->GetWrapped()->New());
    XhtmlLike* typed_element = dynamic_cast<XhtmlLike*>(element.get());
    if (this->GetWrapped()->has_id()) {
      *typed_element->mutable_id() = this->GetWrapped()->id();
    }

    return std::move(element);
  }

  absl::StatusOr<ParseResult> Parse(
      const internal::FhirJson& json, const absl::TimeZone& default_time_zone,
      const ScopedErrorReporter& error_reporter) override {
    absl::StatusOr<std::string> string_value = json.asString();
    if (!string_value.ok()) {
      FHIR_RETURN_IF_ERROR(
          error_reporter.ReportFhirFatal(absl::InvalidArgumentError(
              "Unable to parse to XHTML: JSON primitive must be string.")));
      return ParseResult::kFailed;
    }
    std::unique_ptr<XhtmlLike> wrapped = absl::make_unique<XhtmlLike>();
    wrapped->set_value(*string_value);
    this->WrapAndManage(std::move(wrapped));
    return ParseResult::kSucceeded;
  }

 protected:
  absl::StatusOr<std::string> ToNonNullValueString() const override {
    return ToJsonStringValue(this->GetWrapped()->value());
  }
};

template <typename T>
class ExtensibleWrapper : public SpecificWrapper<T> {
 public:
  absl::Status ValidateProto(
      const ScopedErrorReporter& error_reporter) const override {
    FHIR_ASSIGN_OR_RETURN(const bool has_no_value_extension,
                          HasPrimitiveHasNoValue(*this->GetWrapped()));
    const T& typed = dynamic_cast<const T&>(*this->GetWrapped());
    if (typed.extension_size() == 1 && has_no_value_extension) {
      FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirError(
          absl::StrCat(T::descriptor()->full_name(),
                       " must have either extensions or value.")));
      return absl::OkStatus();
    }
    return ValidateProtoTypeSpecific(has_no_value_extension, error_reporter);
  }

  absl::StatusOr<std::unique_ptr<::google::protobuf::Message>> GetElement()
      const override {
    std::unique_ptr<Message> element =
        absl::WrapUnique(this->GetWrapped()->New());
    T* typed_element = dynamic_cast<T*>(element.get());
    if (this->GetWrapped()->has_id()) {
      *typed_element->mutable_id() = this->GetWrapped()->id();
    }

    for (const auto& extension : this->GetWrapped()->extension()) {
      *typed_element->add_extension() = extension;
    }
    for (const char* internal_url : *kConversionOnlyExtensionUrls) {
      FHIR_RETURN_IF_ERROR(ClearExtensionsWithUrl(internal_url, typed_element));
    }

    return std::move(element);
  }

  bool HasValue() const override {
    for (const auto& extension : this->GetWrapped()->extension()) {
      if (extension.url().value() == kPrimitiveHasNoValueUrl &&
          extension.value().boolean().value()) {
        return false;
      }
    }
    return true;
  }

  bool HasElement() const override {
    if (this->GetWrapped()->has_id()) return true;

    for (const auto& extension : this->GetWrapped()->extension()) {
      bool is_conversion_only_extension = false;
      for (const char* internal_url : *kConversionOnlyExtensionUrls) {
        if (extension.url().value() == internal_url) {
          is_conversion_only_extension = true;
          break;
        }
      }
      if (!is_conversion_only_extension) return true;
    }
    return false;
  }

 protected:
  virtual absl::Status ValidateProtoTypeSpecific(
      bool has_no_value_extension,
      const ScopedErrorReporter& error_reporter) const = 0;

  absl::Status InitializeNull() {
    this->managed_memory_ = absl::make_unique<T>();
    FHIR_RETURN_IF_ERROR(
        BuildHasNoValueExtension(this->managed_memory_->add_extension()));
    this->wrapped_ = this->managed_memory_.get();
    return absl::OkStatus();
  }
};

template <typename T>
bool MatchesFhirRegexExtension(absl::string_view string_value) {
  static const RE2* valid_regex_pattern = [] {
    const std::string regex_string = GetValueRegex(T::descriptor());
    return regex_string.empty() ? nullptr : new RE2(regex_string);
  }();
  return (valid_regex_pattern == nullptr ||
          RE2::FullMatch(string_value, *valid_regex_pattern));
}

// Should be used when a regex failure means a primitive is unparsable, such as
// decimal or datetime.
inline absl::Status ReportRegexFatal(
    const Descriptor* descriptor, const ScopedErrorReporter& error_reporter) {
  return error_reporter.ReportFhirFatal(absl::InvalidArgumentError(
      absl::StrCat("Unparseable JSON string for ", descriptor->full_name())));
}

// Should be used when a regex failure means a primitive is unparsable, such as
// decimal or datetime.
inline absl::Status ReportRegexError(
    const Descriptor* descriptor, const ScopedErrorReporter& error_reporter) {
  return error_reporter.ReportFhirError(
      absl::StrCat("Invalid input for ", descriptor->full_name()));
}

// Template for wrappers that expect the input to be a JSON string type,
// and don't care about the default time zone.
template <typename T>
class StringInputWrapper : public ExtensibleWrapper<T> {
 public:
  absl::StatusOr<ParseResult> Parse(
      const internal::FhirJson& json, const absl::TimeZone& default_time_zone,
      const ScopedErrorReporter& error_reporter) override {
    if (json.isNull()) {
      FHIR_RETURN_IF_ERROR(this->InitializeNull());
      return ParseResult::kSucceeded;
    }
    absl::StatusOr<std::string> string_value = json.asString();
    if (!string_value.ok()) {
      FHIR_RETURN_IF_ERROR(
          error_reporter.ReportFhirFatal(absl::InvalidArgumentError(
              absl::StrCat("Unable to parse to ", T::descriptor()->full_name(),
                           ": JSON primitive must be string."))));
      return ParseResult::kFailed;
    }
    FHIR_ASSIGN_OR_RETURN(ParseResult result,
                          ParseString(*string_value, error_reporter));
    if (result == ParseResult::kFailed) {
      return ParseResult::kFailed;
    }
    if (!MatchesFhirRegexExtension<T>(*string_value)) {
      // The input string is parsable, but not valid fhir
      // This records a fhir error, but does not early return
      FHIR_RETURN_IF_ERROR(ReportRegexError(T::descriptor(), error_reporter));
    }

    return ParseResult::kSucceeded;
  }

 protected:
  virtual absl::StatusOr<ParseResult> ParseString(
      const std::string& json_string,
      const ScopedErrorReporter& error_reporter) = 0;
};

// Template for wrappers that represent data as a string.
template <typename T>
class StringTypeWrapper : public StringInputWrapper<T> {
 public:
  absl::StatusOr<std::string> ToNonNullValueString() const override {
    return ToJsonStringValue(this->GetWrapped()->value());
  }

  absl::Status ValidateProtoTypeSpecific(
      const bool has_no_value_extension,
      const ScopedErrorReporter& error_reporter) const override {
    if (has_no_value_extension) {
      if (!this->GetWrapped()->value().empty()) {
        FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirError(
            absl::StrCat(T::descriptor()->full_name(),
                         " has both a value, and a "
                         "PrimitiveHasNoValueExtension.")));
      }
      return absl::OkStatus();
    }
    if (!MatchesFhirRegexExtension<T>(this->GetWrapped()->value())) {
      FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirError(
          absl::StrCat("Input failed regex requirement for: ",
                       T::descriptor()->full_name())));
    }
    return absl::OkStatus();
  }

 protected:
  absl::StatusOr<ParseResult> ParseString(
      const std::string& json_string,
      const ScopedErrorReporter& error_reporter) override {
    std::unique_ptr<T> wrapped = absl::make_unique<T>();
    wrapped->set_value(json_string);
    this->WrapAndManage(std::move(wrapped));
    return ParseResult::kSucceeded;
  }
};

// Date Formats that are expected to include time zones.
static const std::unordered_map<std::string, std::string>* const tz_formatters =
    new std::unordered_map<std::string, std::string>{
        {"SECOND", "%E4Y-%m-%dT%H:%M:%S%Ez"},
        {"MILLISECOND", "%E4Y-%m-%dT%H:%M:%E3S%Ez"},
        {"MICROSECOND", "%E4Y-%m-%dT%H:%M:%E6S%Ez"}};
// Date Formats that are expected to not include time zones, and use the default
// time zone.
static const std::unordered_map<std::string, std::string>* const
    no_tz_formatters = new std::unordered_map<std::string, std::string>{
        {"YEAR", "%E4Y"}, {"MONTH", "%E4Y-%m"}, {"DAY", "%E4Y-%m-%d"}};

// Template for wrappers that represent data as Timelike primitives
// E.g.: Date, DateTime, Instant, etc.
template <typename T>
class TimeTypeWrapper : public ExtensibleWrapper<T> {
 public:
  absl::StatusOr<std::string> ToNonNullValueString() const override {
    const T& timelike = *this->GetWrapped();
    absl::Time absolute_time = absl::FromUnixMicros(timelike.value_us());

    if (timelike.timezone().empty()) {
      return InvalidArgumentError(absl::StrFormat(
          "Cannot print %s: Missing timezone", T::descriptor()->full_name()));
    }

    if (timelike.precision() == T::PRECISION_UNSPECIFIED) {
      return InvalidArgumentError(absl::StrFormat(
          "Cannot print %s: Missing precision", T::descriptor()->full_name()));
    }

    FHIR_ASSIGN_OR_RETURN(absl::TimeZone time_zone,
                          BuildTimeZoneFromString(timelike.timezone()));

    auto format_iter =
        tz_formatters->find(T::Precision_Name(timelike.precision()));
    if (format_iter == tz_formatters->end()) {
      format_iter =
          no_tz_formatters->find(T::Precision_Name(timelike.precision()));
    }
    if (format_iter == no_tz_formatters->end()) {
      return InvalidArgumentError(
          absl::StrFormat("Invalid precision on %s", T::descriptor()->name()));
    }
    std::string value = absl::StrCat(
        "\"", absl::FormatTime(format_iter->second, absolute_time, time_zone),
        "\"");
    return (timelike.timezone() == "Z")
               ? absl::StrReplaceAll(value, {{"+00:00", "Z"}})
               : value;
  }

  absl::Status ValidateProtoTypeSpecific(
      const bool has_no_value_extension,
      const ScopedErrorReporter& error_reporter) const override {
    const T* wrapped = this->GetWrapped();
    if (has_no_value_extension) {
      if (wrapped->value_us() != 0) {
        return error_reporter.ReportFhirError(
            absl::StrCat(T::descriptor()->full_name(),
                         " has PrimitiveNoValueExtension but has a value."));
      }
      if (wrapped->precision() != T::PRECISION_UNSPECIFIED) {
        return error_reporter.ReportFhirError(absl::StrCat(
            T::descriptor()->full_name(),
            " has PrimitiveNoValueExtension but has a specified precision."));
      }
      if (!wrapped->timezone().empty()) {
        return error_reporter.ReportFhirError(absl::StrCat(
            T::descriptor()->full_name(),
            " has PrimitiveNoValueExtension but has a specified timezone."));
      }
    } else if (wrapped->precision() == T::PRECISION_UNSPECIFIED) {
      return error_reporter.ReportFhirError(
          absl::StrCat(T::descriptor()->full_name(), " is missing precision."));
    } else if (wrapped->timezone().empty()) {
      return error_reporter.ReportFhirError(
          absl::StrCat(T::descriptor()->full_name(), " is missing TimeZone."));
    }
    return absl::OkStatus();
  }

 protected:
  absl::StatusOr<ParseResult> Parse(
      const internal::FhirJson& json, const absl::TimeZone& default_time_zone,
      const ScopedErrorReporter& error_reporter) override {
    if (json.isNull()) {
      FHIR_RETURN_IF_ERROR(this->InitializeNull());
      return ParseResult::kSucceeded;
    }
    if (!json.isString()) {
      FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirFatal(InvalidArgumentError(
          absl::StrCat("Cannot parse as ", T::descriptor()->full_name(),
                       ": it is not a string value."))));
      return ParseResult::kFailed;
    }
    const std::string json_string = json.asString().value();

    // Note that for time-like types to be parseable, they must match the FHIR
    // regex.
    if (!MatchesFhirRegexExtension<T>(json_string)) {
      FHIR_RETURN_IF_ERROR(ReportRegexFatal(T::descriptor(), error_reporter));
      return ParseResult::kFailed;
    }

    static const LazyRE2 fractional_seconds_regex{
        R"regex(T\d+:\d+:\d+(?:\.(\d+))?[zZ\+-])regex"};
    std::string fractional_seconds;
    if (RE2::PartialMatch(json_string, *fractional_seconds_regex,
                          &fractional_seconds)) {
      std::string precision =
          fractional_seconds.length() == 0
              ? "SECOND"
              : (fractional_seconds.length() <= 3 ? "MILLISECOND"
                                                  : "MICROSECOND");

      auto format_iter = tz_formatters->find(precision);
      std::string err;
      absl::Time time;
      if (absl::ParseTime(format_iter->second, json_string, &time, &err)) {
        FHIR_ASSIGN_OR_RETURN(const std::string time_zone_string,
                              ParseTimeZoneString(json_string));
        FHIR_RETURN_IF_ERROR(
            SetValue(time, time_zone_string, format_iter->first));
        return ParseResult::kSucceeded;
      }
    }

    // These formats do not include timezones, and thus use the default time
    // zone.
    for (std::pair<std::string, std::string> format : *no_tz_formatters) {
      std::string err;
      absl::Time time;
      if (absl::ParseTime(format.second, json_string, default_time_zone, &time,
                          &err)) {
        std::string timezone_name = default_time_zone.name();

        // Clean up the fixed timezone string that is returned from the
        // absl::Timezone library.
        if (absl::StartsWith(timezone_name, "Fixed/UTC")) {
          // TODO(b/244184211): Evaluate whether we want to keep the seconds
          // offset.
          static const LazyRE2 kFixedTimezoneRegex{
              "Fixed\\/UTC([+-]\\d\\d:\\d\\d):\\d\\d"};
          std::string fixed_timezone_name;
          if (RE2::FullMatch(timezone_name, *kFixedTimezoneRegex,
                             &fixed_timezone_name)) {
            timezone_name = fixed_timezone_name;
          } else {
            FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirFatal(
                InvalidArgumentError("Invalid fixed timezone format")));
            return ParseResult::kFailed;
          }
        }
        FHIR_RETURN_IF_ERROR(SetValue(time, timezone_name, format.first));
        return ParseResult::kSucceeded;
      }
    }
    return InvalidArgumentError(
        absl::StrCat("Invalid ", T::descriptor()->full_name()));
  }

 private:
  absl::Status SetValue(absl::Time time, const std::string& timezone_string,
                        const std::string& precision_string) {
    std::unique_ptr<T> wrapped = absl::make_unique<T>();
    wrapped->set_value_us(absl::ToUnixMicros(time));
    wrapped->set_timezone(timezone_string);
    const EnumDescriptor* precision_enum_descriptor =
        T::descriptor()->FindEnumTypeByName("Precision");
    if (!precision_enum_descriptor) {
      return InvalidArgumentError(absl::StrCat("Message ",
                                               T::descriptor()->full_name(),
                                               " has no precision enum type"));
    }
    const EnumValueDescriptor* precision =
        precision_enum_descriptor->FindValueByName(precision_string);
    if (!precision) {
      return InvalidArgumentError(absl::StrCat("Unrecognized precision on ",
                                               T::descriptor()->full_name()));
    }
    const FieldDescriptor* precision_field =
        T::descriptor()->FindFieldByName("precision");
    if (!precision_field) {
      return InvalidArgumentError(absl::StrCat(T::descriptor()->full_name(),
                                               " has no precision field."));
    }
    wrapped->GetReflection()->SetEnum(wrapped.get(), precision_field,
                                      precision);
    this->WrapAndManage(std::move(wrapped));
    return absl::OkStatus();
  }

  static absl::StatusOr<std::string> ParseTimeZoneString(
      const std::string& date_string) {
    static const LazyRE2 TIMEZONE_PATTERN = {
        "(Z|(\\+|-)((0[0-9]|1[0-3]):[0-5][0-9]|14:00))$"};
    std::string time_zone_string;
    if (RE2::PartialMatch(date_string, *TIMEZONE_PATTERN, &time_zone_string)) {
      return time_zone_string;
    }
    return InvalidArgumentError(
        absl::StrCat("Invalid ", T::descriptor()->full_name(),
                     " has missing or badly formatted timezone."));
  }
};

// Template for Wrappers that expect integers as json input.
template <typename T>
class IntegerTypeWrapper : public ExtensibleWrapper<T> {
 public:
  absl::StatusOr<ParseResult> Parse(
      const internal::FhirJson& json, const absl::TimeZone& default_time_zone,
      const ScopedErrorReporter& error_reporter) override {
    if (json.isNull()) {
      FHIR_RETURN_IF_ERROR(this->InitializeNull());
      return ParseResult::kSucceeded;
    }
    if (!json.isInt()) {
      error_reporter.ReportFhirFatal(InvalidArgumentError(
          absl::StrCat("Cannot parse as ", T::descriptor()->full_name(),
                       json.isString() ? "  It is a quoted string." : "")));
      return ParseResult::kFailed;
    }
    // Before we can treat the json value as an int, we need to make sure
    // it fits into bounds of the corresponding datatype.
    FHIR_RETURN_IF_ERROR(ValidateInteger(json.asInt().value(), error_reporter));
    std::unique_ptr<T> wrapped = absl::make_unique<T>();
    wrapped->set_value(json.asInt().value());
    this->WrapAndManage(std::move(wrapped));
    return ParseResult::kSucceeded;
  }

  absl::StatusOr<std::string> ToNonNullValueString() const override {
    return absl::StrCat(this->GetWrapped()->value());
  }

 protected:
  virtual absl::Status ValidateInteger(
      const int64_t int_value,
      const ScopedErrorReporter& error_reporter) const {
    if (int_value < std::numeric_limits<int32_t>::min() ||
        int_value > std::numeric_limits<int32_t>::max()) {
      FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirFatal(InvalidArgumentError(
          absl::Substitute("Cannot parse as $0: Out of range",
                           T::descriptor()->full_name()))));
    }
    return absl::OkStatus();
  }

  absl::Status ValidateProtoTypeSpecific(
      const bool has_no_value_extension,
      const ScopedErrorReporter& error_reporter) const override {
    if (has_no_value_extension) {
      if (this->GetWrapped()->value() != 0) {
        return error_reporter.ReportFhirError(absl::StrCat(
            T::descriptor()->full_name(),
            " has both a value, and a PrimitiveHasNoValueExtension."));
      }
      return absl::OkStatus();
    }
    return this->ValidateInteger(this->GetWrapped()->value(), error_reporter);
  }
};

template <typename CodeType>
class CodeWrapper : public StringTypeWrapper<CodeType> {
 public:
  absl::Status Wrap(const ::google::protobuf::Message& codelike) override {
    std::unique_ptr<CodeType> wrapped = absl::make_unique<CodeType>();
    FHIR_RETURN_IF_ERROR(CopyCode(codelike, wrapped.get()));
    this->WrapAndManage(std::move(wrapped));
    return absl::OkStatus();
  }

  absl::StatusOr<ParseResult> MergeInto(
      Message* target,
      const ScopedErrorReporter& error_reporter) const override {
    if (IsMessageType<CodeType>(*target)) {
      target->MergeFrom(*this->GetWrapped());
    }
    absl::Status copied_status = CopyCode(*this->GetWrapped(), target);
    if (!copied_status.ok()) {
      FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirFatal(copied_status));
      return ParseResult::kFailed;
    }
    return ParseResult::kSucceeded;
  }

 private:
  absl::Status ValidateCodelike() const {
    const Descriptor* descriptor = this->GetWrapped()->GetDescriptor();
    const Reflection* reflection = this->GetWrapped()->GetReflection();
    const FieldDescriptor* value_field = descriptor->FindFieldByName("value");

    FHIR_ASSIGN_OR_RETURN(const bool has_no_value_extension,
                          HasPrimitiveHasNoValue(*this->GetWrapped()));
    bool has_value = false;
    switch (value_field->cpp_type()) {
      case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        has_value =
            reflection->GetEnumValue(*this->GetWrapped(), value_field) != 0;
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
        std::string str;
        has_value =
            !reflection
                 ->GetStringReference(*this->GetWrapped(), value_field, &str)
                 .empty();
        break;
      }
      default:
        return FailedPreconditionError(
            absl::StrCat(descriptor->full_name(),
                         " should have a value field of type ENUM or STRING."));
    }

    if (has_no_value_extension && has_value) {
      return FailedPreconditionError(
          absl::StrCat(descriptor->full_name(),
                       " has both PrimitiveHasNoValue extension and a value."));
    }
    if (!has_no_value_extension && !has_value) {
      return FailedPreconditionError(
          absl::StrCat(descriptor->full_name(),
                       " has no value, and no PrimitiveHasNoValue extension."));
    }
    if (has_no_value_extension &&
        reflection->FieldSize(*this->GetWrapped(),
                              descriptor->FindFieldByName("extension")) == 1) {
      // The only extension is the "no value" extension.
      return FailedPreconditionError(absl::StrCat(
          descriptor->full_name(), " must have either extensions or value",
          " (not counting the PrimitiveHasNoValue", " extension)."));
    }
    return absl::OkStatus();
  }
};

template <typename Base64BinaryType,
          typename ExtensionType = EXTENSION_TYPE(Base64BinaryType),
          typename StringType = FHIR_DATATYPE(Base64BinaryType, string_value),
          typename PositiveIntType =
              FHIR_DATATYPE(Base64BinaryType, positive_int)>
class Base64BinaryWrapper : public StringInputWrapper<Base64BinaryType> {
 public:
  absl::StatusOr<std::string> ToNonNullValueString() const override {
    std::string escaped;
    absl::Base64Escape(this->GetWrapped()->value(), &escaped);
    FHIR_ASSIGN_OR_RETURN(const ExtensionType* separator_extension,
                          GetOnlyMatchingExtension<ExtensionType>(
                              kBinarySeparatorStrideUrl, *this->GetWrapped()));
    if (separator_extension != nullptr) {
      FHIR_ASSIGN_OR_RETURN(const PositiveIntType* stride,
                            GetOnlySimpleExtensionValue<PositiveIntType>(
                                "stride", *separator_extension));
      FHIR_ASSIGN_OR_RETURN(const StringType* separator,
                            GetOnlySimpleExtensionValue<StringType>(
                                "separator", *separator_extension));

      RE2::GlobalReplace(&escaped, absl::StrCat("(.{", stride->value(), "})"),
                         absl::StrCat("\\1", separator->value()));
      if (absl::EndsWith(escaped, separator->value())) {
        escaped.erase(escaped.length() - separator->value().length());
      }
    }
    return absl::StrCat("\"", escaped, "\"");
  }

  absl::StatusOr<std::unique_ptr<::google::protobuf::Message>> GetElement()
      const override {
    FHIR_ASSIGN_OR_RETURN(auto extension_message,
                          ExtensibleWrapper<Base64BinaryType>::GetElement());

    FHIR_RETURN_IF_ERROR(ClearExtensionsWithUrl(kBinarySeparatorStrideUrl,
                                                extension_message.get()));
    return std::move(extension_message);
  }

 protected:
  absl::Status ValidateProtoTypeSpecific(
      const bool has_no_value_extension,
      const ScopedErrorReporter& error_reporter) const override {
    if (has_no_value_extension) {
      if (!this->GetWrapped()->value().empty()) {
        FHIR_RETURN_IF_ERROR(
            error_reporter.ReportFhirError("Base64Binary has both a value, and "
                                           "a PrimitiveHasNoValueExtension."));
      }
      return absl::OkStatus();
    }
    FHIR_ASSIGN_OR_RETURN(const std::string& as_string, this->ToValueString());
    if (!MatchesFhirRegexExtension<Base64BinaryType>(
            as_string.substr(1, as_string.length() - 2))) {
      FHIR_RETURN_IF_ERROR(
          ReportRegexError(Base64BinaryType::descriptor(), error_reporter));
    }
    return absl::OkStatus();
  }

 private:
  absl::StatusOr<ParseResult> ParseString(
      const std::string& json_string,
      const ScopedErrorReporter& error_reporter) override {
    std::unique_ptr<Base64BinaryType> wrapped =
        absl::make_unique<Base64BinaryType>();
    size_t stride = json_string.find(' ');
    if (stride != std::string::npos) {
      size_t end = stride;
      while (end < json_string.length() && json_string[end] == ' ') {
        end++;
      }
      ExtensionType* separator_stride_extension = wrapped->add_extension();
      separator_stride_extension->mutable_url()->set_value(
          kBinarySeparatorStrideUrl);

      ExtensionType* separator_extension =
          separator_stride_extension->add_extension();
      separator_extension->mutable_url()->set_value("separator");
      separator_extension->mutable_value()->mutable_string_value()->set_value(
          json_string.substr(stride, end - stride));

      ExtensionType* stride_extension =
          separator_stride_extension->add_extension();
      stride_extension->mutable_url()->set_value("stride");
      stride_extension->mutable_value()->mutable_positive_int()->set_value(
          stride);
    }

    std::string unescaped;
    if (!absl::Base64Unescape(json_string, &unescaped)) {
      FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirFatal(
          InvalidArgumentError("Encountered invalid base64 string.")));
      return ParseResult::kFailed;
    }
    wrapped->set_value(unescaped);
    this->WrapAndManage(std::move(wrapped));
    return ParseResult::kSucceeded;
  }
};

template <typename BooleanType>
class BooleanWrapper : public ExtensibleWrapper<BooleanType> {
 protected:
  absl::Status ValidateProtoTypeSpecific(
      const bool has_no_value_extension,
      const ScopedErrorReporter& error_reporter) const override {
    if (has_no_value_extension && this->GetWrapped()->value()) {
      FHIR_RETURN_IF_ERROR(
          error_reporter.ReportFhirError("Boolean has both a value, and a "
                                         "PrimitiveHasNoValueExtension."));
    }
    return absl::OkStatus();
  }

 private:
  absl::StatusOr<ParseResult> Parse(
      const internal::FhirJson& json, const absl::TimeZone& default_time_zone,
      const ScopedErrorReporter& error_reporter) override {
    if (json.isNull()) {
      FHIR_RETURN_IF_ERROR(this->InitializeNull());
      return ParseResult::kSucceeded;
    }
    if (!json.isBool()) {
      FHIR_RETURN_IF_ERROR(InvalidArgumentError(
          absl::StrCat("Cannot parse as Boolean.",
                       json.isString() ? "  It is a quoted string." : "")));
      return ParseResult::kFailed;
    }
    std::unique_ptr<BooleanType> wrapped = absl::make_unique<BooleanType>();
    wrapped->set_value(json.asBool().value());
    this->WrapAndManage(std::move(wrapped));
    return ParseResult::kSucceeded;
  }

  absl::StatusOr<std::string> ToNonNullValueString() const override {
    return absl::StrCat(this->GetWrapped()->value() ? "true" : "false");
  }
};

// Note: This extends StringInputWrapper, but Parse is overridden to also accept
// integer types.
// This is necessary because we cannot use true decimal JSON types without
// risking the data being altered, due to decimal precision.
// Thus, if the input has a decimal point in it, it should have been pre-quoted
// prior to parsing, so it is treated like a string.
// We do not do this for integral types (e.g., 287, -5) because there is no
// risk of loss of precision.
template <typename DecimalType>
class DecimalWrapper : public ExtensibleWrapper<DecimalType> {
 public:
  absl::StatusOr<std::string> ToNonNullValueString() const override {
    return absl::StrCat(this->GetWrapped()->value());
  }

 protected:
  absl::Status ValidateProtoTypeSpecific(
      const bool has_no_value_extension,
      const ScopedErrorReporter& error_reporter) const override {
    if (has_no_value_extension) {
      if (!this->GetWrapped()->value().empty()) {
        FHIR_RETURN_IF_ERROR(
            error_reporter.ReportFhirError("Decimal has both a value, and a "
                                           "PrimitiveHasNoValueExtension."));
      }
      return absl::OkStatus();
    }
    if (!MatchesFhirRegexExtension<DecimalType>(this->GetWrapped()->value())) {
      FHIR_RETURN_IF_ERROR(
          ReportRegexError(DecimalType::descriptor(), error_reporter));
    }
    return absl::OkStatus();
  }

 private:
  absl::StatusOr<ParseResult> Parse(
      const internal::FhirJson& json, const absl::TimeZone& default_time_zone,
      const ScopedErrorReporter& error_reporter) override {
    if (json.isNull()) {
      FHIR_RETURN_IF_ERROR(this->InitializeNull());
      return ParseResult::kSucceeded;
    }
    if (json.isString()) {
      FHIR_ASSIGN_OR_RETURN(const std::string& json_string, json.asString());
      if (!MatchesFhirRegexExtension<DecimalType>(json_string)) {
        FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirFatal(
            InvalidArgumentError("Cannot parse Decimal: fails FHIR regex.")));
        return ParseResult::kFailed;
      }
      // TODO(b/244184211): range check
      std::unique_ptr<DecimalType> wrapped = absl::make_unique<DecimalType>();
      wrapped->set_value(json_string);
      this->WrapAndManage(std::move(wrapped));
      return ParseResult::kSucceeded;
    }
    if (json.isInt()) {
      std::unique_ptr<DecimalType> wrapped = absl::make_unique<DecimalType>();
      wrapped->set_value(absl::StrCat(json.asInt().value()));
      this->WrapAndManage(std::move(wrapped));
      return ParseResult::kSucceeded;
    }
    return InvalidArgumentError(
        "Cannot parse as Decimal: must be a string, integer, or null.  "
        "Numeric types containing decimal points should "
        "have been escaped prior to parsing by JsonFormat.");
  }
};

template <typename PositiveIntType>
class PositiveIntWrapper : public IntegerTypeWrapper<PositiveIntType> {
 protected:
  absl::Status ValidateInteger(
      const int64_t int_value,
      const ScopedErrorReporter& error_reporter) const {
    if (int_value <= 0 || int_value > std::numeric_limits<int32_t>::max()) {
      FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirError(
          absl::Substitute("Cannot parse as $0: must be in range [$1..$2].",
                           PositiveIntType::descriptor()->full_name(), 1,
                           std::numeric_limits<int32_t>::max())));
    }
    return absl::OkStatus();
  }
};

// One day in microseconds: 24L * 60 * 60 * 1000 * 1000
// Hard code expression for web assembly complilation, to avoid range outside of
// long type error.
constexpr uint64_t DAY_IN_US = 86400000000;

template <typename TimeLike>
class TimeWrapper : public StringInputWrapper<TimeLike> {
 public:
  absl::StatusOr<std::string> ToNonNullValueString() const override {
    static const std::unordered_map<int, std::string>* const formatters =
        new std::unordered_map<int, std::string>{
            {TimeLike::Precision::Time_Precision_SECOND, "%H:%M:%S"},
            {TimeLike::Precision::Time_Precision_MILLISECOND, "%H:%M:%E3S"},
            {TimeLike::Precision::Time_Precision_MICROSECOND, "%H:%M:%E6S"}};
    absl::Time absolute_t =
        absl::FromUnixMicros(this->GetWrapped()->value_us());

    const auto format_iter = formatters->find(this->GetWrapped()->precision());
    if (format_iter == formatters->end()) {
      return InvalidArgumentError("Invalid precision on Time.");
    }
    // Note that we use UTC time, regardless of default timezone, because
    // FHIR Time is timezone independent, and represented as micros since epoch.
    return absl::StrCat(
        "\"",
        absl::FormatTime(format_iter->second, absolute_t, absl::UTCTimeZone()),
        "\"");
  }

 protected:
  absl::Status ValidateProtoTypeSpecific(
      const bool has_no_value_extension,
      const ScopedErrorReporter& error_reporter) const override {
    const TimeLike* wrapped = this->GetWrapped();
    if (has_no_value_extension) {
      if (wrapped->value_us() != 0) {
        return error_reporter.ReportFhirError(
            "Time has PrimitiveNoValueExtension but has a value.");
      }
      if (wrapped->precision() !=
          TimeLike::Precision::Time_Precision_PRECISION_UNSPECIFIED) {
        return error_reporter.ReportFhirError(
            "Time has PrimitiveNoValueExtension but has a specified "
            "precision.");
      }
      return absl::OkStatus();
    } else if (wrapped->precision() ==
               TimeLike::Precision::Time_Precision_PRECISION_UNSPECIFIED) {
      return error_reporter.ReportFhirError("Time is missing precision.");
    }
    if (wrapped->value_us() >= DAY_IN_US) {
      return error_reporter.ReportFhirError(
          "Time has value out of range: must be less than a day in "
          "microseconds.");
    }
    return absl::OkStatus();
  }

 private:
  absl::StatusOr<ParseResult> ParseString(
      const std::string& json_string,
      const ScopedErrorReporter& error_reporter) override {
    static LazyRE2 PATTERN{
        "([01][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9])(?:\\.([0-9]+))?"};
    int hours;
    int minutes;
    int seconds;
    std::string fractional_seconds;
    if (!RE2::FullMatch(json_string, *PATTERN, &hours, &minutes, &seconds,
                        &fractional_seconds)) {
      FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirFatal(
          absl::InvalidArgumentError(absl::StrCat(
              "Cannot parse as ", TimeLike::descriptor()->full_name()))));
      return ParseResult::kFailed;
    }
    const int fractional_seconds_length = fractional_seconds.length();
    const uint64_t base_value_us =
        (((hours * 60L) + minutes) * 60L + seconds) * 1000L * 1000L;

    std::unique_ptr<TimeLike> wrapped = absl::make_unique<TimeLike>();
    if (fractional_seconds_length > 3 && fractional_seconds_length <= 6) {
      wrapped->set_precision(TimeLike::Precision::Time_Precision_MICROSECOND);
      const int microseconds = std::stoi(fractional_seconds.append(
          std::string(6 - fractional_seconds_length, '0')));
      wrapped->set_value_us(base_value_us + microseconds);
    } else if (fractional_seconds.length() > 0) {
      wrapped->set_precision(TimeLike::Precision::Time_Precision_MILLISECOND);
      const int milliseconds = std::stoi(fractional_seconds.append(
          std::string(3 - fractional_seconds_length, '0')));
      wrapped->set_value_us(base_value_us + 1000 * milliseconds);
    } else {
      wrapped->set_precision(TimeLike::Precision::Time_Precision_SECOND);
      wrapped->set_value_us(base_value_us);
    }
    this->WrapAndManage(std::move(wrapped));
    return ParseResult::kSucceeded;
  }
};

template <typename UnsignedIntType>
class UnsignedIntWrapper : public IntegerTypeWrapper<UnsignedIntType> {
 protected:
  absl::Status ValidateInteger(
      const int64_t int_value,
      const ScopedErrorReporter& error_reporter) const {
    if (int_value < 0 || int_value > std::numeric_limits<int32_t>::max()) {
      return error_reporter.ReportFhirError(
          absl::Substitute("Cannot parse as $0: must be in range [$1..$2].",
                           UnsignedIntType::descriptor()->full_name(), 0,
                           std::numeric_limits<int32_t>::max()));
    }
    return absl::OkStatus();
  }
};

template <typename Integer64Type>
class Integer64Wrapper : public StringInputWrapper<Integer64Type> {
 public:
  absl::StatusOr<std::string> ToNonNullValueString() const override {
    return absl::StrCat("\"", this->GetWrapped()->value(), "\"");
  }

 protected:
  absl::Status ValidateProtoTypeSpecific(
      const bool has_no_value_extension,
      const ScopedErrorReporter& error_reporter) const override {
    if (has_no_value_extension) {
      if (this->GetWrapped()->value() != 0) {
        return error_reporter.ReportFhirError(absl::StrCat(
            Integer64Type::descriptor()->full_name(),
            " has both a value, and a PrimitiveHasNoValueExtension."));
      }
      return absl::OkStatus();
    }
    // Note that the bounds of the proto type sint64 are the same as the bounds
    // for Integer64, so any value is ok.
    return absl::OkStatus();
  }

 private:
  absl::StatusOr<ParseResult> ParseString(
      const std::string& json_string,
      const ScopedErrorReporter& error_reporter) override {
    int64_t value;
    if (!absl::SimpleAtoi(json_string, &value)) {
      // Note that this is fatal, since it indicates that data is dropped.
      FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirFatal(
          absl::InvalidArgumentError(absl::Substitute(
              "Failed converting \"$0\" to sint64 for Integer64 primitive.",
              json_string))));
      return ParseResult::kFailed;
    }

    std::unique_ptr<Integer64Type> wrapped = absl::make_unique<Integer64Type>();
    wrapped->set_value(value);

    this->WrapAndManage(std::move(wrapped));
    return ParseResult::kSucceeded;
  }
};

}  // namespace primitives_internal

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_PRIMITIVE_WRAPPER_H_
