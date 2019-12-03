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
#include "google/protobuf/reflection.h"
#include "absl/memory/memory.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "google/fhir/annotations.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "re2/re2.h"

namespace google {
namespace fhir {

using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using ::tensorflow::errors::InvalidArgument;

namespace {

// This is based on the implementation in protobuf/util/internal/utility.h.
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

StatusOr<const FieldDescriptor*> GetReferenceFieldForResource(
    const Message& reference, const std::string& resource_type) {
  const std::string field_name =
      absl::StrCat(ToSnakeCase(resource_type), "_id");
  const FieldDescriptor* field =
      reference.GetDescriptor()->FindFieldByName(field_name);
  if (field == nullptr) {
    return InvalidArgument(absl::StrCat("Resource type ", resource_type,
                                        " is not valid for a reference (field ",
                                        field_name, " does not exist)."));
  }
  return field;
}

Status PopulateTypedReferenceId(const std::string& resource_id,
                                const std::string& version,
                                Message* reference_id) {
  FHIR_RETURN_IF_ERROR(SetPrimitiveStringValue(reference_id, resource_id));
  if (!version.empty()) {
    const FieldDescriptor* history_field =
        reference_id->GetDescriptor()->FindFieldByName("history");
    if (history_field == nullptr) {
      return InvalidArgument(
          absl::StrCat("Not a valid ReferenceId message: ",
                       reference_id->GetDescriptor()->full_name(),
                       ".  Field history does not exist)."));
    }
    FHIR_RETURN_IF_ERROR(
        SetPrimitiveStringValue(reference_id->GetReflection()->MutableMessage(
                                    reference_id, history_field),
                                version));
  }
  return Status::OK();
}

template <typename ReferenceLike,
          typename ReferenceIdLike = REFERENCE_ID_TYPE(ReferenceLike)>
StatusOr<std::string> ReferenceProtoToStringInternal(
    const ReferenceLike& reference) {
  if (reference.has_uri()) {
    return reference.uri().value();
  } else if (reference.has_fragment()) {
    return absl::StrCat("#", reference.fragment().value());
  }

  const Reflection* reflection = reference.GetReflection();
  static const google::protobuf::OneofDescriptor* oneof =
      ReferenceLike::descriptor()->FindOneofByName("reference");
  const FieldDescriptor* field =
      reflection->GetOneofFieldDescriptor(reference, oneof);
  if (field == nullptr) {
    return ::tensorflow::errors::NotFound("Reference not set");
  }
  std::string prefix;
  bool start = true;
  for (const char c : field->name()) {
    if (start) {
      start = false;
      prefix.push_back(c + 'A' - 'a');
    } else if (c == '_') {
      start = true;
    } else {
      prefix.push_back(c);
    }
  }
  static LazyRE2 re = {"Id$"};
  RE2::Replace(&prefix, *re, "");
  const ReferenceIdLike& id =
      (const ReferenceIdLike&)reflection->GetMessage(reference, field);
  std::string reference_string = absl::StrCat(prefix, "/", id.value());
  if (id.has_history()) {
    absl::StrAppend(&reference_string, "/_history/", id.history().value());
  }
  return reference_string;
}

Status ReferenceStringToProto(const std::string& input, Message* reference) {
  const Descriptor* descriptor = reference->GetDescriptor();
  const Reflection* reflection = reference->GetReflection();
  const FieldDescriptor* uri_field = descriptor->FindFieldByName("uri");
  FHIR_RETURN_IF_ERROR(SetPrimitiveStringValue(
      reflection->MutableMessage(reference, uri_field), input));
  return SplitIfRelativeReference(reference);
}

}  // namespace

StatusOr<std::string> ReferenceProtoToString(
    const stu3::proto::Reference& reference) {
  return ReferenceProtoToStringInternal(reference);
}

StatusOr<std::string> ReferenceProtoToString(
    const r4::core::Reference& reference) {
  return ReferenceProtoToStringInternal(reference);
}

// TODO: Split these into separate files, each that accepts only
// one type.
StatusOr<std::string> ReferenceMessageToString(
    const ::google::protobuf::Message& reference) {
  if (IsMessageType<stu3::proto::Reference>(reference)) {
    return ReferenceProtoToString(
        dynamic_cast<const stu3::proto::Reference&>(reference));
  } else if (IsMessageType<r4::core::Reference>(reference)) {
    return ReferenceProtoToString(
        dynamic_cast<const r4::core::Reference&>(reference));
  }
  return InvalidArgument(
      "Invalid Reference type for ReferenceMessageToString: ",
      reference.GetDescriptor()->full_name());
}

namespace {

template <class TypedDateTime>
absl::Duration InternalGetDurationFromTimelikeElement(
    const TypedDateTime& datetime) {
  // TODO: handle YEAR and MONTH properly, instead of approximating.
  switch (datetime.precision()) {
    case TypedDateTime::YEAR:
      return absl::Hours(24 * 366);
    case TypedDateTime::MONTH:
      return absl::Hours(24 * 31);
    case TypedDateTime::DAY:
      return absl::Hours(24);
    case TypedDateTime::SECOND:
      return absl::Seconds(1);
    case TypedDateTime::MILLISECOND:
      return absl::Milliseconds(1);
    case TypedDateTime::MICROSECOND:
      return absl::Microseconds(1);
    default:
      LOG(FATAL) << "Unsupported datetime precision: " << datetime.precision();
  }
}

}  // namespace

absl::Duration GetDurationFromTimelikeElement(
    const stu3::proto::DateTime& datetime) {
  return InternalGetDurationFromTimelikeElement(datetime);
}

absl::Duration GetDurationFromTimelikeElement(
    const r4::core::DateTime& datetime) {
  return InternalGetDurationFromTimelikeElement(datetime);
}

Status GetTimezone(const std::string& timezone_str, absl::TimeZone* tz) {
  FHIR_ASSIGN_OR_RETURN(*tz, BuildTimeZoneFromString(timezone_str));
  return Status::OK();
}

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
    return InvalidArgument("Unable to parse timezone: ", time_zone_string);
  }
  return tz;
}

StatusOr<std::string> GetResourceId(const Message& message) {
  const auto* desc = message.GetDescriptor();
  const Reflection* ref = message.GetReflection();
  const FieldDescriptor* field = desc->FindFieldByName("id");
  if (!field) {
    return InvalidArgument("Error calling GetResourceId: ", desc->full_name(),
                           " has no Id field");
  }
  if (field->is_repeated()) {
    return InvalidArgument("Unexpected repeated id field");
  }
  if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    return InvalidArgument("No id field found on message");
  }
  if (IsMessageType<stu3::proto::Id>(field->message_type())) {
    const auto* id_message =
        dynamic_cast<const stu3::proto::Id*>(&ref->GetMessage(message, field));
    return id_message->value();
  } else if (IsMessageType<r4::core::Id>(field->message_type())) {
    const auto* id_message =
        dynamic_cast<const r4::core::Id*>(&ref->GetMessage(message, field));
    return id_message->value();
  } else {
    return InvalidArgument(
        absl::StrCat("id field is not a valid Id type: ", desc->full_name()));
  }
}

std::string GetReferenceToResource(const Message& message) {
  return absl::StrCat(message.GetDescriptor()->name(), "/",
                      GetResourceId(message).ValueOrDie());
}

Status PopulateReferenceToResource(const Message& resource,
                                   Message* reference) {
  FHIR_ASSIGN_OR_RETURN(const std::string resource_id, GetResourceId(resource));
  FHIR_ASSIGN_OR_RETURN(const FieldDescriptor* reference_id_field,
                        GetReferenceFieldForResource(
                            *reference, resource.GetDescriptor()->name()));
  Message* reference_id =
      reference->GetReflection()->MutableMessage(reference, reference_id_field);
  return PopulateTypedReferenceId(resource_id, "" /* no version */,
                                  reference_id);
}

StatusOr<stu3::proto::Reference> GetTypedReferenceToResourceStu3(
    const ::google::protobuf::Message& resource) {
  stu3::proto::Reference reference;
  FHIR_RETURN_IF_ERROR(PopulateReferenceToResource(resource, &reference));
  return reference;
}

StatusOr<r4::core::Reference> GetTypedReferenceToResourceR4(
    const ::google::protobuf::Message& resource) {
  r4::core::Reference reference;
  FHIR_RETURN_IF_ERROR(PopulateReferenceToResource(resource, &reference));
  return reference;
}

// Splits relative references into their components, for example, "Patient/ABCD"
// will result in the patientId field getting the value "ABCD".
Status SplitIfRelativeReference(Message* reference) {
  const Descriptor* descriptor = reference->GetDescriptor();
  const Reflection* reflection = reference->GetReflection();

  const FieldDescriptor* uri_field = descriptor->FindFieldByName("uri");
  const FieldDescriptor* fragment_field =
      descriptor->FindFieldByName("fragment");
  if (!reflection->HasField(*reference, uri_field)) {
    // There is no uri to split
    return Status::OK();
  }

  const Message& uri = reflection->GetMessage(*reference, uri_field);

  std::string uri_scratch;
  FHIR_ASSIGN_OR_RETURN(const std::string& uri_string,
                        GetPrimitiveStringValue(uri, &uri_scratch));

  static const LazyRE2 kInternalReferenceRegex{
      "([0-9A-Za-z_]+)/([A-Za-z0-9.-]{1,64})(?:/_history/"
      "([A-Za-z0-9.-]{1,64}))?"};
  std::string resource_type;
  std::string resource_id;
  std::string version;
  if (RE2::FullMatch(uri_string, *kInternalReferenceRegex, &resource_type,
                     &resource_id, &version)) {
    FHIR_ASSIGN_OR_RETURN(
        const FieldDescriptor* reference_id_field,
        GetReferenceFieldForResource(*reference, resource_type));
    // Note that we make the reference_id off of the reference before adding it,
    // since adding the reference_id would destroy the uri field, since they are
    // in the same oneof.  This way allows us to copy fields from uri to
    // reference_id without an extra copy.
    std::unique_ptr<Message> reference_id =
        absl::WrapUnique(reflection->GetMessageFactory()
                             ->GetPrototype(reference_id_field->message_type())
                             ->New());
    FHIR_RETURN_IF_ERROR(
        PopulateTypedReferenceId(resource_id, version, reference_id.get()));
    const Message& uri = reflection->GetMessage(*reference, uri_field);
    reference_id->GetTypeName();
    FHIR_RETURN_IF_ERROR(CopyCommonField(uri, reference_id.get(), "id"));
    FHIR_RETURN_IF_ERROR(CopyCommonField(uri, reference_id.get(), "extension"));
    reflection->SetAllocatedMessage(reference, reference_id.release(),
                                    reference_id_field);
    return Status::OK();
  }

  static const LazyRE2 kFragmentReferenceRegex{"#[A-Za-z0-9.-]{1,64}"};
  if (RE2::FullMatch(uri_string, *kFragmentReferenceRegex)) {
    // Note that we make the fragment off of the reference before adding it,
    // since adding the fragment would destroy the uri field, since they are in
    // the same oneof.  This way allows us to copy fields from uri to fragment
    // without an extra copy.
    std::unique_ptr<Message> fragment =
        absl::WrapUnique(reflection->GetMessageFactory()
                             ->GetPrototype(fragment_field->message_type())
                             ->New());
    FHIR_RETURN_IF_ERROR(
        SetPrimitiveStringValue(fragment.get(), uri_string.substr(1)));
    FHIR_RETURN_IF_ERROR(CopyCommonField(uri, fragment.get(), "id"));
    FHIR_RETURN_IF_ERROR(CopyCommonField(uri, fragment.get(), "extension"));
    reflection->SetAllocatedMessage(reference, fragment.release(),
                                    fragment_field);
    return Status::OK();
  }

  // We're permissive about various full url schemes.
  static LazyRE2 kUrlReference = {"(http|https|urn):.*"};
  if (RE2::FullMatch(uri_string, *kUrlReference)) {
    // There's no way to rewrite the URI, but it's valid as is.
    return Status::OK();
  }
  return InvalidArgument(absl::StrCat("String \"", uri_string,
                                      "\" cannot be parsed as a reference."));
}

StatusOr<stu3::proto::Reference> ReferenceStringToProtoStu3(
    const std::string& input) {
  stu3::proto::Reference reference;
  FHIR_RETURN_IF_ERROR(ReferenceStringToProto(input, &reference));
  return reference;
}

StatusOr<r4::core::Reference> ReferenceStringToProtoR4(
    const std::string& input) {
  r4::core::Reference reference;
  FHIR_RETURN_IF_ERROR(ReferenceStringToProto(input, &reference));
  return reference;
}

Status SetPrimitiveStringValue(::google::protobuf::Message* primitive,
                               const std::string& value) {
  const FieldDescriptor* value_field =
      primitive->GetDescriptor()->FindFieldByName("value");
  if (!value_field || value_field->is_repeated() ||
      value_field->type() != FieldDescriptor::Type::TYPE_STRING) {
    return InvalidArgument("Not a valid String-type primitive: ",
                           primitive->GetDescriptor()->full_name());
  }
  primitive->GetReflection()->SetString(primitive, value_field, value);
  return Status::OK();
}

StatusOr<std::string> GetPrimitiveStringValue(
    const ::google::protobuf::Message& primitive, std::string* scratch) {
  const FieldDescriptor* value_field =
      primitive.GetDescriptor()->FindFieldByName("value");
  if (!value_field || value_field->is_repeated() ||
      value_field->type() != FieldDescriptor::Type::TYPE_STRING) {
    return InvalidArgument("Not a valid String-type primitive: ",
                           primitive.GetDescriptor()->full_name());
  }
  return primitive.GetReflection()->GetStringReference(primitive, value_field,
                                                       scratch);
}

Status GetDecimalValue(const stu3::proto::Decimal& decimal, double* value) {
  if (!absl::SimpleAtod(decimal.value(), value)) {
    return InvalidArgument(
        absl::StrCat("Invalid decimal: '", decimal.value(), "'"));
  }
  return Status::OK();
}

}  // namespace fhir
}  // namespace google
