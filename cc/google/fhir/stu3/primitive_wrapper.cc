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

#include "google/fhir/stu3/primitive_wrapper.h"

#include <ctype.h>

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "absl/memory/memory.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/time/time.h"
#include "google/fhir/status/status.h"
#include "google/fhir/stu3/annotations.h"
#include "google/fhir/stu3/codes.h"
#include "google/fhir/stu3/extensions.h"
#include "google/fhir/stu3/proto_util.h"
#include "google/fhir/stu3/util.h"
#include "proto/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "include/json/json.h"
#include "tensorflow/core/lib/core/errors.h"
#include "re2/re2.h"

namespace google {
namespace fhir {
namespace stu3 {

namespace {

using ::google::fhir::Status;
using ::google::fhir::StatusOr;
using ::google::fhir::stu3::ClearTypedExtensions;
using ::google::fhir::stu3::ConvertToExtension;
using ::google::fhir::stu3::GetRepeatedFromExtension;
using ::google::fhir::stu3::HasValueset;
using ::google::fhir::stu3::google::Base64BinarySeparatorStride;
using ::google::fhir::stu3::google::PrimitiveHasNoValue;
using ::google::fhir::stu3::proto::Base64Binary;
using ::google::fhir::stu3::proto::Boolean;
using ::google::fhir::stu3::proto::Code;
using ::google::fhir::stu3::proto::Date;
using ::google::fhir::stu3::proto::DateTime;
using ::google::fhir::stu3::proto::Decimal;
using ::google::fhir::stu3::proto::Extension;
using ::google::fhir::stu3::proto::Id;
using ::google::fhir::stu3::proto::Instant;
using ::google::fhir::stu3::proto::Integer;
using ::google::fhir::stu3::proto::Markdown;
using ::google::fhir::stu3::proto::Oid;
using ::google::fhir::stu3::proto::PositiveInt;
using ::google::fhir::stu3::proto::String;
using ::google::fhir::stu3::proto::Time;
using ::google::fhir::stu3::proto::UnsignedInt;
using ::google::fhir::stu3::proto::Uri;
using ::google::fhir::stu3::proto::Xhtml;
using ::google::protobuf::Descriptor;
using ::google::protobuf::EnumDescriptor;
using ::google::protobuf::EnumValueDescriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using ::tensorflow::errors::FailedPrecondition;
using ::tensorflow::errors::InvalidArgument;

StatusOr<const Extension*> BuildHasNoValueExtension() {
  PrimitiveHasNoValue msg;
  msg.mutable_value_boolean()->set_value(true);
  Extension* extension = new Extension;
  FHIR_RETURN_IF_ERROR(ConvertToExtension(msg, extension));
  return extension;
}

static const std::vector<const Descriptor*>* const kConversionOnlyExtensions =
    new std::vector<const Descriptor*>{
        PrimitiveHasNoValue::descriptor(),
        Base64BinarySeparatorStride::descriptor(),
    };

class PrimitiveWrapper {
 public:
  virtual ~PrimitiveWrapper() {}
  virtual Status MergeInto(::google::protobuf::Message* target) const = 0;
  virtual Status Parse(const Json::Value& json,
                       const absl::TimeZone& default_time_zone) = 0;
  virtual Status Wrap(const ::google::protobuf::Message&) = 0;
  virtual bool HasElement() const = 0;
  virtual StatusOr<std::unique_ptr<::google::protobuf::Message>> GetElement() const = 0;

  // This is used to validate arbitrary protos - not necessarily the one being
  // wrapped. This allows validation logic to be shared with things besides
  // JSON parsing/printing with minimal memory and code overhead.
  // TODO: Refactor this class to not do a copy when wrapping a
  // proto.  Then, we can make this operate on the wrapped proto without doing
  // extra copies.
  virtual Status ValidateProto(const Message& message) const = 0;

  StatusOr<string> ToValueString() const {
    static const char* kNullString = "null";
    if (HasValue()) {
      return ToNonNullValueString();
    }
    return absl::StrCat(kNullString);
  }

 protected:
  virtual bool HasValue() const = 0;
  virtual StatusOr<string> ToNonNullValueString() const = 0;
};

template <typename T>
class SpecificWrapper : public PrimitiveWrapper {
 public:
  Status MergeInto(Message* target) const override {
    if (T::descriptor()->full_name() != target->GetDescriptor()->full_name()) {
      return InvalidArgument(
          "Type mismatch in SpecificWrapper#MergeInto: ", "Attempted to merge ",
          T::descriptor()->full_name(), " into ",
          target->GetDescriptor()->full_name());
    }
    target->MergeFrom(wrapped_);
    return Status::OK();
  }

  Status Wrap(const ::google::protobuf::Message& message) override {
    if (T::descriptor()->full_name() != message.GetDescriptor()->full_name()) {
      return InvalidArgument(
          "Type mismatch in SpecificWrapper#Wrap: ", "Attempted to wrap ",
          message.GetDescriptor()->full_name(), " with wrapper for ",
          T::descriptor()->full_name());
    }
    wrapped_.CopyFrom(message);
    return Status::OK();
  }

  T& GetWrapped() { return wrapped_; }

  const T& GetWrapped() const { return wrapped_; }

  bool HasValue() const override {
    for (const Extension& extension : GetWrapped().extension()) {
      if (extension.url().value() ==
              GetPrimitiveHasNoValueExtension()->url().value() &&
          extension.value().boolean().value()) {
        return false;
      }
    }
    return true;
  }

  bool HasElement() const override {
    if (GetWrapped().has_id()) return true;

    const Descriptor* descriptor = GetWrapped().GetDescriptor();
    const Reflection* reflection = GetWrapped().GetReflection();

    const FieldDescriptor* extension_field =
        descriptor->FindFieldByName("extension");
    for (int i = 0; i < reflection->FieldSize(GetWrapped(), extension_field);
         i++) {
      const Extension& extension = dynamic_cast<const Extension&>(
          reflection->GetRepeatedMessage(GetWrapped(), extension_field, i));
      bool is_conversion_only_extension = false;
      for (const Descriptor* internal_extension : *kConversionOnlyExtensions) {
        if (extension.url().value() ==
            internal_extension->options().GetExtension(
                ::google::fhir::stu3::proto::fhir_structure_definition_url)) {
          is_conversion_only_extension = true;
          break;
        }
      }
      if (!is_conversion_only_extension) return true;
    }
    return false;
  }

  StatusOr<std::unique_ptr<::google::protobuf::Message>> GetElement() const override {
    const Descriptor* descriptor = GetWrapped().GetDescriptor();
    const Reflection* reflection = GetWrapped().GetReflection();
    Message* copy = GetWrapped()
                        .GetReflection()
                        ->GetMessageFactory()
                        ->GetPrototype(descriptor)
                        ->New();
    const Reflection* copy_reflection = copy->GetReflection();
    const FieldDescriptor* id_field = descriptor->FindFieldByName("id");
    if (reflection->HasField(GetWrapped(), id_field)) {
      copy_reflection->MutableMessage(copy, id_field)
          ->CopyFrom(reflection->GetMessage(GetWrapped(), id_field));
    }

    const FieldDescriptor* extension_field =
        descriptor->FindFieldByName("extension");
    for (int i = 0; i < reflection->FieldSize(GetWrapped(), extension_field);
         i++) {
      copy_reflection->AddMessage(copy, extension_field)
          ->CopyFrom(
              reflection->GetRepeatedMessage(GetWrapped(), extension_field, i));
    }
    for (const Descriptor* internal_extension : *kConversionOnlyExtensions) {
      FHIR_RETURN_IF_ERROR(ClearTypedExtensions(internal_extension, copy));
    }

    return absl::WrapUnique(copy);
  }

  Status ValidateProto(const Message& message) const override {
    if (message.GetDescriptor()->full_name() != T::descriptor()->full_name()) {
      return InvalidArgument("Error: Mismatch validating message of type ",
                             message.GetDescriptor()->full_name(), " against ",
                             T::descriptor()->full_name(),
                             ".  This indicates an error in validation, rather "
                             "than a problem with data.");
    }
    const T& typed = dynamic_cast<const T&>(message);

    std::vector<PrimitiveHasNoValue> no_value_extensions;
    FHIR_RETURN_IF_ERROR(
        GetRepeatedFromExtension(typed.extension(), &no_value_extensions));
    if (no_value_extensions.size() > 1) {
      return FailedPrecondition(
          T::descriptor()->full_name(),
          " has multiple PrimitiveHasNoValue extensions.");
    }
    const bool has_no_value_extension =
        no_value_extensions.size() == 1 &&
        no_value_extensions.at(0).value_boolean().value();
    if (typed.extension_size() == 1 && has_no_value_extension) {
      return FailedPrecondition(T::descriptor()->full_name(),
                                " must have either extensions or value.");
    }
    return ValidateTypeSpecific(typed, has_no_value_extension);
  }

 protected:
  T wrapped_;
  static StatusOr<T> BuildNullValue();

  virtual Status ValidateTypeSpecific(
      const T& message, const bool has_no_value_extension) const = 0;

  static Status ValidateString(const string& input) {
    static const RE2* regex_pattern = [] {
      const string value_regex_string = GetValueRegex(T::descriptor());
      return value_regex_string.empty() ? nullptr
                                        : new RE2(value_regex_string);
    }();
    return regex_pattern == nullptr || RE2::FullMatch(input, *regex_pattern)
               ? Status::OK()
               : InvalidArgument("Invalid input for ",
                                 T::descriptor()->full_name(), ": ", input);
  }
};

template <>
bool SpecificWrapper<Xhtml>::HasValue() const {
  return true;
}

template <>
bool SpecificWrapper<Xhtml>::HasElement() const {
  return GetWrapped().has_id();
}

template <class T>
StatusOr<T> SpecificWrapper<T>::BuildNullValue() {
  T t;
  *(t.add_extension()) = *GetPrimitiveHasNoValueExtension();
  return t;
}

template <>
StatusOr<Xhtml> SpecificWrapper<Xhtml>::BuildNullValue() {
  return InvalidArgument("Unexpected null xhtml");
}

// Xhtml can't have extensions, it's always valid.
template <>
Status SpecificWrapper<Xhtml>::ValidateProto(const Message& message) const {
  return Status::OK();
}

// Template for wrappers that expect the input to be a JSON string type,
// and don't care about the default time zone.
template <typename T>
class StringInputWrapper : public SpecificWrapper<T> {
 public:
  Status Parse(const Json::Value& json,
               const absl::TimeZone& default_time_zone) override {
    if (json.isNull()) {
      FHIR_ASSIGN_OR_RETURN(this->wrapped_, this->BuildNullValue());
      return Status::OK();
    }
    if (!json.isString()) {
      return InvalidArgument("Cannot parse ", json.toStyledString(), " as ",
                             T::descriptor()->full_name(),
                             ": it is not a string value.");
    }
    return ParseString(json.asString());
  }

 protected:
  virtual Status ParseString(const string& json_string) = 0;
};

// Template for wrappers that represent data as a string.
template <typename T>
class StringTypeWrapper : public StringInputWrapper<T> {
 public:
  StatusOr<string> ToNonNullValueString() const override {
    return StatusOr<string>(
        Json::valueToQuotedString(this->GetWrapped().value().c_str()));
  }

  Status ValidateTypeSpecific(
      const T& message, const bool has_no_value_extension) const override {
    if (has_no_value_extension) {
      return message.value().empty()
                 ? Status::OK()
                 : FailedPrecondition(T::descriptor()->full_name(),
                                      " has both a value, and a "
                                      "PrimitiveHasNoValueExtension.");
    }
    Status string_validation = this->ValidateString(message.value());
    return string_validation.ok()
               ? Status::OK()
               : FailedPrecondition(string_validation.error_message());
  }

 protected:
  Status ParseString(const string& json_string) override {
    FHIR_RETURN_IF_ERROR(this->ValidateString(json_string));
    this->GetWrapped().set_value(json_string);
    return Status::OK();
  }
};

// Date Formats that are expected to include time zones.
static const std::unordered_map<string, string>* const tz_formatters =
    new std::unordered_map<string, string>{
        {"SECOND", "%Y-%m-%dT%H:%M:%S%Ez"},
        {"MILLISECOND", "%Y-%m-%dT%H:%M:%E3S%Ez"},
        {"MICROSECOND", "%Y-%m-%dT%H:%M:%E6S%Ez"}};
// Note: %E#S accepts UP TO # decimal places, so we need to be sure to iterate
// from most restrictive to least restrictive when checking input strings.
static const std::vector<string>* const tz_formatters_iteration_order =
    new std::vector<string>{"SECOND", "MILLISECOND", "MICROSECOND"};
// Date Formats that are expected to not include time zones, and use the default
// time zone.
static const std::unordered_map<string, string>* const no_tz_formatters =
    new std::unordered_map<string, string>{
        {"YEAR", "%Y"}, {"MONTH", "%Y-%m"}, {"DAY", "%Y-%m-%d"}};

// Template for wrappers that represent data as Timelike primitives
// E.g.: Date, DateTime, Instant, etc.
template <typename T>
class TimeTypeWrapper : public SpecificWrapper<T> {
 public:
  StatusOr<string> ToNonNullValueString() const override {
    const T& timelike = this->GetWrapped();
    absl::Time absolute_time = absl::FromUnixMicros(timelike.value_us());
    FHIR_ASSIGN_OR_RETURN(absl::TimeZone time_zone,
                          BuildTimeZoneFromString(timelike.timezone()));

    auto format_iter =
        tz_formatters->find(T::Precision_Name(timelike.precision()));
    if (format_iter == tz_formatters->end()) {
      format_iter =
          no_tz_formatters->find(T::Precision_Name(timelike.precision()));
    }
    if (format_iter == no_tz_formatters->end()) {
      return InvalidArgument("Invalid precision on Time: ",
                             timelike.DebugString());
    }
    string value = absl::StrCat(
        "\"", absl::FormatTime(format_iter->second, absolute_time, time_zone),
        "\"");
    return (timelike.timezone() == "Z")
               ? ::tensorflow::str_util::StringReplace(
                     value, "+00:00", "Z", /* replace_all = */ false)
               : value;
  }

  Status ValidateTypeSpecific(
      const T& message, const bool has_no_value_extension) const override {
    if (has_no_value_extension) {
      if (message.value_us() != 0) {
        return FailedPrecondition(
            T::descriptor()->full_name(),
            " has PrimitiveNoValueExtension but has a value.");
      }
      if (message.precision() != T::PRECISION_UNSPECIFIED) {
        return FailedPrecondition(
            T::descriptor()->full_name(),
            " has PrimitiveNoValueExtension but has a specified precision.");
      }
      if (!message.timezone().empty()) {
        return FailedPrecondition(
            T::descriptor()->full_name(),
            " has PrimitiveNoValueExtension but has a specified timezone.");
      }
    } else if (message.precision() == T::PRECISION_UNSPECIFIED) {
      return FailedPrecondition(T::descriptor()->full_name(),
                                " is missing precision.");
    } else if (message.timezone().empty()) {
      return FailedPrecondition(T::descriptor()->full_name(),
                                " is missing TimeZone.");
    }
    return Status::OK();
  }

 protected:
  Status Parse(const Json::Value& json,
               const absl::TimeZone& default_time_zone) override {
    if (json.isNull()) {
      FHIR_ASSIGN_OR_RETURN(this->wrapped_, this->BuildNullValue());
      return Status::OK();
    }
    if (!json.isString()) {
      return InvalidArgument("Cannot parse ", json.toStyledString(), " as ",
                             T::descriptor()->full_name(),
                             ": it is not a string value.");
    }
    const string& json_string = json.asString();
    FHIR_RETURN_IF_ERROR(this->ValidateString(json_string));
    // Note that this will handle any level of precision - it's up to various
    // wrappers' validation pattern to ensure that the precision of the value
    // is valid.  There's no risk of accidentally using an invalid precision
    // though, as it will fail to find an appropriate precision enum type.
    for (string precision : *tz_formatters_iteration_order) {
      auto format_iter = tz_formatters->find(precision);
      string err;
      absl::Time time;
      if (absl::ParseTime(format_iter->second, json_string, &time, &err)) {
        FHIR_ASSIGN_OR_RETURN(const string time_zone_string,
                              ParseTimeZoneString(json_string));
        return SetValue(time, time_zone_string, format_iter->first);
      }
    }

    // These formats do not include timezones, and thus use the default time
    // zone.
    for (std::pair<string, string> format : *no_tz_formatters) {
      string err;
      absl::Time time;
      if (absl::ParseTime(format.second, json_string, default_time_zone, &time,
                          &err)) {
        string timezone_name = default_time_zone.name();

        // Clean up the fixed timezone string that is returned from the
        // absl::Timezone library.
        if (absl::StartsWith(timezone_name, "Fixed/UTC")) {
          // TODO: Evaluate whether we want to keep the seconds offset.
          static const LazyRE2 kFixedTimezoneRegex{
              "Fixed\\/UTC([+-]\\d\\d:\\d\\d):\\d\\d"};
          string fixed_timezone_name;
          if (RE2::FullMatch(timezone_name, *kFixedTimezoneRegex,
                             &fixed_timezone_name)) {
            timezone_name = fixed_timezone_name;
          } else {
            return InvalidArgument("Invalid fixed timezone format: ",
                                   timezone_name);
          }
        }
        return SetValue(time, timezone_name, format.first);
      }
    }
    return InvalidArgument("Invalid ", T::descriptor()->full_name(), ": ",
                           json_string);
  }

 private:
  Status SetValue(absl::Time time, const string& timezone_string,
                  const string& precision_string) {
    T& wrapped = this->GetWrapped();
    wrapped.set_value_us(ToUnixMicros(time));
    wrapped.set_timezone(timezone_string);
    const EnumDescriptor* precision_enum_descriptor =
        T::descriptor()->FindEnumTypeByName("Precision");
    if (!precision_enum_descriptor) {
      return InvalidArgument("Message ", T::descriptor()->full_name(),
                             " has no precision enum type");
    }
    const EnumValueDescriptor* precision =
        precision_enum_descriptor->FindValueByName(precision_string);
    if (!precision) {
      return InvalidArgument(precision_enum_descriptor->full_name(),
                             " has no enum value ", precision_string);
    }
    const FieldDescriptor* precision_field =
        T::descriptor()->FindFieldByName("precision");
    if (!precision_field) {
      return InvalidArgument(T::descriptor()->full_name(),
                             " has no precision field.");
    }
    wrapped.GetReflection()->SetEnum(&wrapped, precision_field, precision);
    return Status::OK();
  }

  static StatusOr<absl::TimeZone> BuildTimeZoneFromString(
      const string& time_zone_string) {
    if (time_zone_string == "UTC" || time_zone_string == "Z") {
      return absl::UTCTimeZone();
    }
    // We can afford to use a simpler pattern here because we've already
    // validated the timezone above.
    static const LazyRE2 TIMEZONE_PATTERN = {"(\\+|-)(\\d{2}):(\\d{2})"};
    string sign;
    int hours;
    int minutes;
    if (RE2::FullMatch(time_zone_string, *TIMEZONE_PATTERN, &sign, &hours,
                       &minutes)) {
      int seconds_offset = ((hours * 60) + minutes) * 60;
      seconds_offset *= (sign == "-" ? -1 : 1);
      return absl::FixedTimeZone(seconds_offset);
    }
    absl::TimeZone tz;
    if (!absl::LoadTimeZone(time_zone_string, &tz)) {
      return InvalidArgument("Unable to parse timezone: ", time_zone_string);
    }
    return tz;
  }

  static StatusOr<string> ParseTimeZoneString(const string& date_string) {
    static const LazyRE2 TIMEZONE_PATTERN = {
        "(Z|(\\+|-)((0[0-9]|1[0-3]):[0-5][0-9]|14:00))$"};
    string time_zone_string;
    if (RE2::PartialMatch(date_string, *TIMEZONE_PATTERN, &time_zone_string)) {
      return time_zone_string;
    }
    return InvalidArgument(
        "Invalid ", T::descriptor()->full_name(),
        " has missing or badly formatted timezone: ", date_string);
  }
};

// Template for Wrappers that expect integers as json input.
template <typename T>
class IntegerTypeWrapper : public SpecificWrapper<T> {
 public:
  Status Parse(const Json::Value& json,
               const absl::TimeZone& default_time_zone) override {
    if (json.isNull()) {
      FHIR_ASSIGN_OR_RETURN(this->wrapped_, this->BuildNullValue());
      return Status::OK();
    }
    if (json.type() != Json::ValueType::intValue &&
        json.type() != Json::ValueType::uintValue) {
      return InvalidArgument("Cannot parse ", json.toStyledString(),
                             " as Integer.",
                             json.isString() ? "  It is a quoted string." : "");
    }
    FHIR_RETURN_IF_ERROR(ValidateInteger(json.asInt()));
    this->GetWrapped().set_value(json.asInt());
    return Status::OK();
  }

  StatusOr<string> ToNonNullValueString() const override {
    return absl::StrCat(this->GetWrapped().value());
  }

 protected:
  virtual Status ValidateInteger(const int int_value) const {
    return Status::OK();
  }

  Status ValidateTypeSpecific(
      const T& message, const bool has_no_value_extension) const override {
    if (has_no_value_extension) {
      if (message.value() != 0) {
        return FailedPrecondition(
            T::descriptor()->full_name(),
            " has both a value, and a PrimitiveHasNoValueExtension.");
      }
      return Status::OK();
    }
    Status int_validation = this->ValidateInteger(message.value());
    return int_validation.ok()
               ? Status::OK()
               : FailedPrecondition(int_validation.error_message());
  }
};

Status ValidateCodelike(const Message& message) {
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* reflection = message.GetReflection();
  const FieldDescriptor* value_field = descriptor->FindFieldByName("value");
  const FieldDescriptor* extension_field =
      descriptor->FindFieldByName("extension");

  std::vector<PrimitiveHasNoValue> no_value_extensions;
  const auto& extensions =
      reflection->GetRepeatedFieldRef<Extension>(message, extension_field);
  FHIR_RETURN_IF_ERROR(
      GetRepeatedFromExtension(extensions, &no_value_extensions));
  if (no_value_extensions.size() > 1) {
    return FailedPrecondition(descriptor->full_name(),
                              " has multiple PrimitiveHasNoValue extensions.");
  }
  const bool has_no_value_extension =
      no_value_extensions.size() == 1 &&
      no_value_extensions.at(0).value_boolean().value();
  if (extensions.size() == 1 && has_no_value_extension) {
    return FailedPrecondition(descriptor->full_name(),
                              " must have either extensions or value.");
  }
  const bool has_enum_value =
      reflection->GetEnumValue(message, value_field) != 0;
  if (has_no_value_extension && has_enum_value) {
    return FailedPrecondition(
        descriptor->full_name(),
        " has both PrimitiveHasNoValue extension and an enum value.");
  }
  if (!has_no_value_extension && !has_enum_value) {
    return FailedPrecondition(
        descriptor->full_name(),
        " has no enum value, and no PrimitiveHasNoValue extension.");
  }
  return Status::OK();
}

class CodeWrapper : public StringTypeWrapper<Code> {
 public:
  Status Wrap(const ::google::protobuf::Message& codelike) override {
    return ConvertToGenericCode(codelike, &this->GetWrapped());
  }

  Status MergeInto(Message* target) const override {
    return ConvertToTypedCode(this->GetWrapped(), target);
  }

  Status ValidateProto(const Message& message) const override {
    if (IsMessageType<Code>(message)) {
      return SpecificWrapper::ValidateProto(message);
    } else {
      return ValidateCodelike(message);
    }
  }
};

class Base64BinaryWrapper : public StringInputWrapper<Base64Binary> {
 public:
  StatusOr<string> ToNonNullValueString() const override {
    string escaped;
    absl::Base64Escape(GetWrapped().value(), &escaped);
    std::vector<Base64BinarySeparatorStride> separator_extensions;
    FHIR_RETURN_IF_ERROR(GetRepeatedFromExtension(GetWrapped().extension(),
                                                  &separator_extensions));
    if (!separator_extensions.empty()) {
      int stride = separator_extensions[0].stride().value();
      string separator = separator_extensions[0].separator().value();

      RE2::GlobalReplace(&escaped, absl::StrCat("(.{", stride, "})"),
                         absl::StrCat("\\1", separator));
      if (absl::EndsWith(escaped, separator)) {
        escaped.erase(escaped.length() - separator.length());
      }
    }
    return absl::StrCat("\"", escaped, "\"");
  }

  StatusOr<std::unique_ptr<::google::protobuf::Message>> GetElement() const override {
    FHIR_ASSIGN_OR_RETURN(auto extension_message,
                          SpecificWrapper::GetElement());
    FHIR_RETURN_IF_ERROR(ClearTypedExtensions(
        Base64BinarySeparatorStride::descriptor(), extension_message.get()));
    return std::move(extension_message);
  }

 protected:
  Status ValidateTypeSpecific(
      const Base64Binary& message,
      const bool has_no_value_extension) const override {
    if (has_no_value_extension) {
      return message.value().empty() ? Status::OK()
                                     : FailedPrecondition(
                                           "Base64Binary has both a value, and "
                                           "a PrimitiveHasNoValueExtension.");
    }
    Status string_validation = this->ValidateString(message.value());
    return string_validation.ok()
               ? Status::OK()
               : FailedPrecondition(string_validation.error_message());
  }

 private:
  Status ParseString(const string& json_string) override {
    size_t stride = json_string.find(' ');
    if (stride != string::npos) {
      size_t end = stride;
      while (end < json_string.length() && json_string[end] == ' ') {
        end++;
      }
      string separator = json_string.substr(stride, end - stride);
      Base64BinarySeparatorStride separator_stride_extension_msg;
      separator_stride_extension_msg.mutable_separator()->set_value(separator);
      separator_stride_extension_msg.mutable_stride()->set_value(stride);

      FHIR_RETURN_IF_ERROR(ConvertToExtension(separator_stride_extension_msg,
                                              GetWrapped().add_extension()));
    }

    string unescaped;
    if (!absl::Base64Unescape(json_string, &unescaped)) {
      return InvalidArgument("Encountered invalid base64 string.");
    }
    GetWrapped().set_value(unescaped);
    return Status::OK();
  }
};

class BooleanWrapper : public SpecificWrapper<Boolean> {
 protected:
  Status ValidateTypeSpecific(
      const Boolean& message,
      const bool has_no_value_extension) const override {
    if (has_no_value_extension && message.value()) {
      return FailedPrecondition(
          "Boolean has both a value, and a PrimitiveHasNoValueExtension.");
    }
    return Status::OK();
  }

 private:
  Status Parse(const Json::Value& json,
               const absl::TimeZone& default_time_zone) override {
    if (json.isNull()) {
      FHIR_ASSIGN_OR_RETURN(wrapped_, BuildNullValue());
      return Status::OK();
    }
    if (!json.isBool()) {
      return InvalidArgument("Cannot parse ", json.toStyledString(),
                             " as Boolean.",
                             json.isString() ? "  It is a quoted string." : "");
    }
    GetWrapped().set_value(json.asBool());
    return Status::OK();
  }

  StatusOr<string> ToNonNullValueString() const override {
    return absl::StrCat(GetWrapped().value() ? "true" : "false");
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
class DecimalWrapper : public StringInputWrapper<Decimal> {
 public:
  StatusOr<string> ToNonNullValueString() const override {
    return absl::StrCat(GetWrapped().value());
  }

 protected:
  Status ValidateTypeSpecific(
      const Decimal& message,
      const bool has_no_value_extension) const override {
    if (has_no_value_extension) {
      return message.value().empty() ? Status::OK()
                                     : FailedPrecondition(
                                           "Decimal has both a value, and a "
                                           "PrimitiveHasNoValueExtension.");
    }
    Status string_validation = this->ValidateString(message.value());
    return string_validation.ok()
               ? Status::OK()
               : FailedPrecondition(string_validation.error_message());
  }

 private:
  Status Parse(const Json::Value& json,
               const absl::TimeZone& default_time_zone) override {
    if (json.isNull()) {
      FHIR_ASSIGN_OR_RETURN(this->wrapped_, this->BuildNullValue());
      return Status::OK();
    }
    if (json.isString()) {
      return ParseString(json.asString());
    }
    if (json.isIntegral()) {
      this->GetWrapped().set_value(json.asString());
      return Status::OK();
    }
    return InvalidArgument("Cannot parse ", json.toStyledString(),
                           " as Decimal: must be a string, integer, or null.  "
                           "Numeric types containing decimal points should "
                           "have been escaped prior to parsing by JsonFormat.");
  }

  Status ParseString(const string& json_string) override {
    FHIR_RETURN_IF_ERROR(ValidateString(json_string));
    // TODO: range check
    this->GetWrapped().set_value(json_string);
    return Status::OK();
  }
};

template <>
Status IntegerTypeWrapper<PositiveInt>::ValidateInteger(
    const int int_value) const {
  return int_value > 0
             ? Status::OK()
             : InvalidArgument("Cannot parse ", int_value,
                               " as PositiveInt: must be greater than zero.");
}

constexpr uint64_t DAY_IN_US = 24L * 60 * 60 * 1000 * 1000;

class TimeWrapper : public StringInputWrapper<Time> {
 public:
  StatusOr<string> ToNonNullValueString() const override {
    static const std::unordered_map<int, string>* const formatters =
        new std::unordered_map<int, string>{
            {Time::Precision::Time_Precision_SECOND, "%H:%M:%S"},
            {Time::Precision::Time_Precision_MILLISECOND, "%H:%M:%E3S"},
            {Time::Precision::Time_Precision_MICROSECOND, "%H:%M:%E6S"}};
    absl::Time absolute_t = absl::FromUnixMicros(this->GetWrapped().value_us());

    const auto format_iter = formatters->find(this->GetWrapped().precision());
    if (format_iter == formatters->end()) {
      return InvalidArgument("Invalid precision on Time: ",
                             this->GetWrapped().DebugString());
    }
    // Note that we use UTC time, regardless of default timezone, because
    // FHIR Time is timezone independent, and represented as micros since epoch.
    return absl::StrCat(
        "\"",
        absl::FormatTime(format_iter->second, absolute_t, absl::UTCTimeZone()),
        "\"");
  }

 protected:
  Status ValidateTypeSpecific(
      const Time& message, const bool has_no_value_extension) const override {
    if (has_no_value_extension) {
      if (message.value_us() != 0) {
        return FailedPrecondition(
            "Time has PrimitiveNoValueExtension but has a value.");
      }
      if (message.precision() != Time::PRECISION_UNSPECIFIED) {
        return FailedPrecondition(
            "Time has PrimitiveNoValueExtension but has a specified "
            "precision.");
      }
      return Status::OK();
    } else if (message.precision() == Time::PRECISION_UNSPECIFIED) {
      return FailedPrecondition("Time is missing precision.");
    }
    if (message.value_us() >= DAY_IN_US) {
      return FailedPrecondition(
          "Time has value out of range: must be less than a day in "
          "microseconds.");
    }
    return Status::OK();
  }

 private:
  Status ParseString(const string& json_string) override {
    static LazyRE2 PATTERN{
        "([01][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9])(?:\\.([0-9]+))?"};
    int hours;
    int minutes;
    int seconds;
    string fractional_seconds;
    if (!RE2::FullMatch(json_string, *PATTERN, &hours, &minutes, &seconds,
                        &fractional_seconds)) {
      return InvalidArgument("Invalid Time ", json_string);
    }
    const int fractional_seconds_length = fractional_seconds.length();
    const uint64_t base_value_us =
        (((hours * 60L) + minutes) * 60L + seconds) * 1000L * 1000L;
    if (fractional_seconds_length > 3 && fractional_seconds_length <= 6) {
      GetWrapped().set_precision(Time::Precision::Time_Precision_MICROSECOND);
      const int microseconds = std::stoi(fractional_seconds.append(
          std::string(6 - fractional_seconds_length, '0')));
      GetWrapped().set_value_us(base_value_us + microseconds);
    } else if (fractional_seconds.length() > 0) {
      GetWrapped().set_precision(Time::Precision::Time_Precision_MILLISECOND);
      const int milliseconds = std::stoi(fractional_seconds.append(
          std::string(3 - fractional_seconds_length, '0')));
      GetWrapped().set_value_us(base_value_us + 1000 * milliseconds);
    } else {
      GetWrapped().set_precision(Time::Precision::Time_Precision_SECOND);
      GetWrapped().set_value_us(base_value_us);
    }
    return Status::OK();
  }
};

template <>
Status IntegerTypeWrapper<UnsignedInt>::ValidateInteger(
    const int int_value) const {
  return int_value >= 0
             ? Status::OK()
             : InvalidArgument(
                   "Cannot parse ", int_value,
                   " as PositiveInt: must be greater than or equal to zero.");
}

StatusOr<std::unique_ptr<PrimitiveWrapper>> GetWrapper(
    const Descriptor* target_descriptor) {
  string target_name = target_descriptor->name();
  if (target_name == "Code" || HasValueset(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>((new CodeWrapper()));
  } else if (target_name == "Base64Binary") {
    return std::unique_ptr<PrimitiveWrapper>(new Base64BinaryWrapper());
  } else if (target_name == "Boolean") {
    return std::unique_ptr<PrimitiveWrapper>(new BooleanWrapper());
  } else if (target_name == "Date") {
    return std::unique_ptr<PrimitiveWrapper>(new TimeTypeWrapper<Date>());
  } else if (target_name == "DateTime") {
    return std::unique_ptr<PrimitiveWrapper>(new TimeTypeWrapper<DateTime>());
  } else if (target_name == "Decimal") {
    return std::unique_ptr<PrimitiveWrapper>(new DecimalWrapper());
  } else if (target_name == "Id") {
    return std::unique_ptr<PrimitiveWrapper>(new StringTypeWrapper<Id>());
  } else if (target_name == "Instant") {
    return std::unique_ptr<PrimitiveWrapper>(new TimeTypeWrapper<Instant>());
  } else if (target_name == "Integer") {
    return std::unique_ptr<PrimitiveWrapper>(new IntegerTypeWrapper<Integer>());
  } else if (target_name == "Markdown") {
    return std::unique_ptr<PrimitiveWrapper>(new StringTypeWrapper<Markdown>());
  } else if (target_name == "Oid") {
    return std::unique_ptr<PrimitiveWrapper>(new StringTypeWrapper<Oid>());
  } else if (target_name == "PositiveInt") {
    return std::unique_ptr<PrimitiveWrapper>(
        new IntegerTypeWrapper<PositiveInt>());
  } else if (target_name == "String") {
    return std::unique_ptr<PrimitiveWrapper>(new StringTypeWrapper<String>());
  } else if (target_name == "Time") {
    return std::unique_ptr<PrimitiveWrapper>(new TimeWrapper());
  } else if (target_name == "UnsignedInt") {
    return std::unique_ptr<PrimitiveWrapper>(
        new IntegerTypeWrapper<UnsignedInt>());
  } else if (target_name == "Uri") {
    return std::unique_ptr<PrimitiveWrapper>(new StringTypeWrapper<Uri>());
  } else if (target_name == "Xhtml") {
    return std::unique_ptr<PrimitiveWrapper>(new StringTypeWrapper<Xhtml>());
  } else {
    return InvalidArgument("Unexpected primitive FHIR type: ",
                           target_descriptor->name());
  }
}

}  // namespace

const Extension* const GetPrimitiveHasNoValueExtension() {
  static const Extension* const extension =
      BuildHasNoValueExtension().ValueOrDie();
  return extension;
}

Status ParseInto(const Json::Value& json, absl::TimeZone tz,
                 ::google::protobuf::Message* target) {
  if (json.type() == Json::ValueType::arrayValue ||
      json.type() == Json::ValueType::objectValue) {
    return InvalidArgument("Invalid JSON type for ",
                           absl::StrCat(json.toStyledString()));
  }
  FHIR_ASSIGN_OR_RETURN(std::unique_ptr<PrimitiveWrapper> wrapper,
                        GetWrapper(target->GetDescriptor()));
  FHIR_RETURN_IF_ERROR(wrapper->Parse(json, tz));
  return wrapper->MergeInto(target);
}

StatusOr<JsonPrimitive> WrapPrimitiveProto(const ::google::protobuf::Message& proto) {
  const ::google::protobuf::Descriptor* descriptor = proto.GetDescriptor();
  FHIR_ASSIGN_OR_RETURN(std::unique_ptr<PrimitiveWrapper> wrapper,
                        GetWrapper(descriptor));
  FHIR_RETURN_IF_ERROR(wrapper->Wrap(proto));
  FHIR_ASSIGN_OR_RETURN(const string value, wrapper->ToValueString());
  if (wrapper->HasElement()) {
    FHIR_ASSIGN_OR_RETURN(std::unique_ptr<Message> wrapped,
                          wrapper->GetElement());
    return JsonPrimitive{value, std::move(wrapped)};
  }
  return JsonPrimitive{value, nullptr};
}

Status ValidatePrimitive(const ::google::protobuf::Message& primitive) {
  if (!IsPrimitive(primitive.GetDescriptor())) {
    return InvalidArgument("Not a primitive type: ",
                           primitive.GetDescriptor()->full_name());
  }

  const ::google::protobuf::Descriptor* descriptor = primitive.GetDescriptor();
  // TODO: Once wrapping a proto doesn't involve a copy,
  // wrap directly here.
  FHIR_ASSIGN_OR_RETURN(std::unique_ptr<PrimitiveWrapper> wrapper,
                        GetWrapper(descriptor));
  return wrapper->ValidateProto(primitive);
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
