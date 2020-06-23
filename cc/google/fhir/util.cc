// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "google/fhir/util.h"

#include <iterator>
#include <memory>
#include <string>


#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/reflection.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "google/fhir/annotations.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"

namespace google {
namespace fhir {

using ::absl::InvalidArgumentError;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

StatusOr<absl::TimeZone> BuildTimeZoneFromString(
    const std::string& time_zone_string) {
  if (time_zone_string == "UTC" || time_zone_string == "Z") {
    return absl::UTCTimeZone();
  }

  // The full regex for timezone in FHIR is the last part of
  // http://hl7.org/fhir/datatypes.html#dateTime
  // We split this up into two regex, because 14:00 is a special case.
  static const LazyRE2 MAIN_TIMEZONE_PATTERN = {
      "(\\+|-)(0[0-9]|1[0-3]):([0-5][0-9])"};

  std::string sign;
  int hours;
  int minutes;
  if (RE2::FullMatch(time_zone_string, *MAIN_TIMEZONE_PATTERN, &sign, &hours,
                     &minutes)) {
    int seconds_offset = ((hours * 60) + minutes) * 60;
    seconds_offset *= (sign == "-" ? -1 : 1);
    return absl::FixedTimeZone(seconds_offset);
  }

  // +/- 14:00 is also allowed.
  static const LazyRE2 FOURTEEN_HUNDRED_PATTERN = {"(\\+|-)14:00"};
  std::string sign_fh;
  if (RE2::FullMatch(time_zone_string, *FOURTEEN_HUNDRED_PATTERN, &sign_fh)) {
    int seconds_offset = 14 * 60 * 60;
    seconds_offset *= (sign_fh == "-" ? -1 : 1);
    return absl::FixedTimeZone(seconds_offset);
  }

  absl::TimeZone tz;
  if (!absl::LoadTimeZone(time_zone_string, &tz)) {
    return InvalidArgumentError(
        absl::StrCat("Unable to parse timezone: ", time_zone_string));
  }
  return tz;
}

StatusOr<std::string> GetResourceId(const Message& message) {
  const auto* desc = message.GetDescriptor();
  const FieldDescriptor* field = desc->FindFieldByName("id");
  if (!field) {
    return InvalidArgumentError(
        absl::StrCat("Error calling GetResourceId: ", desc->full_name(),
                     " has no Id field"));
  }
  if (field->is_repeated()) {
    return InvalidArgumentError("Unexpected repeated id field");
  }
  if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    return InvalidArgumentError("No id field found on message");
  }

  std::string id_value;
  return GetPrimitiveStringValue(message, "id", &id_value);
}

bool ResourceHasId(const Message& message) {
  return !GetResourceId(message).ValueOrDie().empty();
}

Status SetPrimitiveStringValue(::google::protobuf::Message* primitive,
                               const std::string& value) {
  const FieldDescriptor* value_field =
      primitive->GetDescriptor()->FindFieldByName("value");
  if (!value_field || value_field->is_repeated() ||
      value_field->type() != FieldDescriptor::Type::TYPE_STRING) {
    return InvalidArgumentError(
        absl::StrCat("Not a valid String-type primitive: ",
                     primitive->GetDescriptor()->full_name()));
  }
  primitive->GetReflection()->SetString(primitive, value_field, value);
  return absl::OkStatus();
}

StatusOr<std::string> GetPrimitiveStringValue(const ::google::protobuf::Message& parent,
                                              const std::string& field_name,
                                              std::string* scratch) {
  const Descriptor* descriptor = parent.GetDescriptor();
  const FieldDescriptor* field = descriptor->FindFieldByName(field_name);
  if (!field || !field->message_type()) {
    return InvalidArgumentError(absl::StrCat(
        "Invalid message for GetPrimitiveStringValue: no message field `",
        field->name(), "` on `", descriptor->full_name()));
  }
  auto result = GetPrimitiveStringValue(
      parent.GetReflection()->GetMessage(parent, field), scratch);
  return result;
}

StatusOr<std::string> GetPrimitiveStringValue(
    const ::google::protobuf::Message& primitive, std::string* scratch) {
  const FieldDescriptor* value_field =
      primitive.GetDescriptor()->FindFieldByName("value");
  if (!value_field || value_field->is_repeated() ||
      value_field->type() != FieldDescriptor::Type::TYPE_STRING) {
    return InvalidArgumentError(
        absl::StrCat("Not a valid String-type primitive: ",
                     primitive.GetDescriptor()->full_name()));
  }
  auto result = primitive.GetReflection()->GetStringReference(
      primitive, value_field, scratch);
  return result;
}

std::string ToSnakeCase(absl::string_view input) {
  bool was_not_underscore = false;  // Initialize to false for case 1 (below)
  bool was_not_cap = false;
  std::string result;
  result.reserve(input.size() << 1);

  for (size_t i = 0; i < input.size(); ++i) {
    if (absl::ascii_isupper(input[i])) {
      // Consider when the current character B is capitalized:
      // 1) At beginning of input:   "B..." => "b..."
      //    (e.g. "Biscuit" => "biscuit")
      // 2) Following a lowercase:   "...aB..." => "...a_b..."
      //    (e.g. "gBike" => "g_bike")
      // 3) At the end of input:     "...AB" => "...ab"
      //    (e.g. "GoogleLAB" => "google_lab")
      // 4) Followed by a lowercase: "...ABc..." => "...a_bc..."
      //    (e.g. "GBike" => "g_bike")
      if (was_not_underscore &&                     //            case 1 out
          (was_not_cap ||                           // case 2 in, case 3 out
           (i + 1 < input.size() &&                 //            case 3 out
            absl::ascii_islower(input[i + 1])))) {  // case 4 in
        // We add an underscore for case 2 and case 4.
        result.push_back('_');
      }
      result.push_back(absl::ascii_tolower(input[i]));
      was_not_underscore = true;
      was_not_cap = false;
    } else {
      result.push_back(input[i]);
      was_not_underscore = input[i] != '_';
      was_not_cap = true;
    }
  }
  return result;
}

StatusOr<absl::Time> GetTimeFromTimelikeElement(
    const ::google::protobuf::Message& timelike) {
  const Descriptor* descriptor = timelike.GetDescriptor();
  const FieldDescriptor* value_us_field =
      descriptor->FindFieldByName("value_us");

  if (!value_us_field ||
      value_us_field->type() != google::protobuf::FieldDescriptor::TYPE_INT64) {
    return absl::InvalidArgumentError(absl::StrCat(
        "No int64 value_us on Time-like: ", descriptor->full_name()));
  }

  return absl::FromUnixMicros(
      timelike.GetReflection()->GetInt64(timelike, value_us_field));
}

}  // namespace fhir
}  // namespace google
