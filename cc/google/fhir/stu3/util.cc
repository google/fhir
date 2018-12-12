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

#include "google/fhir/stu3/util.h"

#include <iterator>
#include <string>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/reflection.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/systems/systems.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "re2/re2.h"

namespace google {
namespace fhir {
namespace stu3 {

using ::google::fhir::stu3::proto::ContainedResource;
using ::google::fhir::stu3::proto::DateTime;
using ::google::fhir::stu3::proto::Id;
using ::google::fhir::stu3::proto::Reference;
using ::google::fhir::stu3::proto::ReferenceId;
using ::google::protobuf::Message;

using std::string;

namespace {

// This is based on the implementation in protobuf/util/internal/utility.h.
string ToSnakeCase(absl::string_view input) {
  bool was_not_underscore = false;  // Initialize to false for case 1 (below)
  bool was_not_cap = false;
  string result;
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

}  // namespace

StatusOr<string> ExtractCodeBySystem(
    const stu3::proto::CodeableConcept& codeable_concept,
    absl::string_view system_value) {
  for (int i = 0; i < codeable_concept.coding_size(); i++) {
    auto coding = codeable_concept.coding(i);
    if (coding.has_system() && coding.has_code() &&
        coding.system().value() == system_value) {
      return coding.code().value();
    }
  }

  return ::tensorflow::errors::NotFound(
      "Cannot find a value for the corresponding system code.");
}

StatusOr<string> ExtractIcdCode(
    const stu3::proto::CodeableConcept& codeable_concept,
    const std::vector<string>& schemes) {
  bool found_response = false;
  StatusOr<string> result;
  for (size_t i = 0; i < schemes.size(); i++) {
    StatusOr<string> s = ExtractCodeBySystem(codeable_concept, schemes[i]);

    if (s.status().code() == ::tensorflow::errors::Code::ALREADY_EXISTS) {
      // Multiple codes, so we can return an error already.
      return s;
    } else if (s.ok()) {
      if (found_response) {
        // We found _another_ code. That shouldn't have happened.
        return ::tensorflow::errors::AlreadyExists("Found more than one code");
      } else {
        result = s;
        found_response = true;
      }
    }
  }
  if (found_response) {
    return result;
  } else {
    return ::tensorflow::errors::NotFound(
        "No ICD code with the provided schemes in concept.");
  }
}

Status SetContainedResource(const Message& resource,
                            ContainedResource* contained) {
  const google::protobuf::OneofDescriptor* resource_oneof =
      ContainedResource::descriptor()->FindOneofByName("oneof_resource");
  const google::protobuf::FieldDescriptor* resource_field = nullptr;
  for (int i = 0; i < resource_oneof->field_count(); i++) {
    const google::protobuf::FieldDescriptor* field = resource_oneof->field(i);
    if (field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      return ::tensorflow::errors::InvalidArgument(
          absl::StrCat("Field ", field->full_name(), "is not a message"));
    }
    if (field->message_type()->name() == resource.GetDescriptor()->name()) {
      resource_field = field;
    }
  }
  if (resource_field == nullptr) {
    return ::tensorflow::errors::InvalidArgument(
        absl::StrCat("Resource type ", resource.GetDescriptor()->name(),
                     " not found in fhir::Bundle::Entry::resource"));
  }
  const google::protobuf::Reflection* ref = contained->GetReflection();
  ref->MutableMessage(contained, resource_field)->CopyFrom(resource);
  return Status::OK();
}

StatusOr<ContainedResource> WrapContainedResource(const Message& resource) {
  ContainedResource contained_resource;
  TF_RETURN_IF_ERROR(SetContainedResource(resource, &contained_resource));
  return contained_resource;
}

bool IsPrimitive(const google::protobuf::Descriptor* descriptor) {
  return descriptor->options().GetExtension(
             stu3::proto::structure_definition_kind) ==
         stu3::proto::StructureDefinitionKindValue::KIND_PRIMITIVE_TYPE;
}

bool IsResource(const google::protobuf::Descriptor* descriptor) {
  return descriptor->options().GetExtension(
             stu3::proto::structure_definition_kind) ==
         stu3::proto::StructureDefinitionKindValue::KIND_RESOURCE;
}

bool IsReference(const google::protobuf::Descriptor* descriptor) {
  return descriptor->options().ExtensionSize(stu3::proto::fhir_reference_type) >
         0;
}

bool HasValueset(const google::protobuf::Descriptor* descriptor) {
  return descriptor->options().HasExtension(stu3::proto::fhir_valueset_url);
}

bool IsChoiceType(const google::protobuf::FieldDescriptor* field) {
  return field->options().GetExtension(stu3::proto::is_choice_type);
}

const string GetFhirProfileBase(const google::protobuf::Descriptor* descriptor) {
  return descriptor->options().GetExtension(stu3::proto::fhir_profile_base);
}

const string GetStructureDefinitionUrl(const google::protobuf::Descriptor* descriptor) {
  return descriptor->options().GetExtension(
      stu3::proto::fhir_structure_definition_url);
}

StatusOr<string> ReferenceProtoToString(const Reference& reference) {
  if (reference.has_uri()) {
    return reference.uri().value();
  } else if (reference.has_fragment()) {
    return absl::StrCat("#", reference.fragment().value());
  }

  const google::protobuf::Reflection* reflection = reference.GetReflection();
  static const google::protobuf::OneofDescriptor* oneof =
      Reference::descriptor()->FindOneofByName("reference");
  const google::protobuf::FieldDescriptor* field =
      reflection->GetOneofFieldDescriptor(reference, oneof);
  if (field == nullptr) {
    return ::tensorflow::errors::NotFound("Reference not set");
  }
  string prefix;
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
  const ReferenceId& id =
      (const ReferenceId&)reflection->GetMessage(reference, field);
  string reference_string = absl::StrCat(prefix, "/", id.value());
  if (id.has_history()) {
    absl::StrAppend(&reference_string, "/_history/", id.history().value());
  }
  return reference_string;
}

Status GetPatient(const Bundle& bundle, const Patient** patient) {
  bool found = false;
  for (const auto& entry : bundle.entry()) {
    if (entry.resource().has_patient()) {
      if (found) {
        return ::tensorflow::errors::AlreadyExists(
            "Found more than one patient in bundle");
      }
      *patient = &entry.resource().patient();
      found = true;
    }
  }
  if (found) {
    return Status::OK();
  } else {
    return ::tensorflow::errors::NotFound("No patient in bundle.");
  }
}

StatusOr<const Message*> GetContainedResource(
    const ContainedResource& contained) {
  const google::protobuf::Reflection* ref = contained.GetReflection();
  // Get the resource field corresponding to this resource.
  const google::protobuf::OneofDescriptor* resource_oneof =
      contained.GetDescriptor()->FindOneofByName("oneof_resource");
  const google::protobuf::FieldDescriptor* field =
      contained.GetReflection()->GetOneofFieldDescriptor(contained,
                                                         resource_oneof);
  if (!field) {
    return ::tensorflow::errors::NotFound("No Bundle Resource found");
  }
  return &(ref->GetMessage(contained, field));
}

absl::Duration GetDurationFromTimelikeElement(const DateTime& datetime) {
  // TODO: handle YEAR and MONTH properly, instead of approximating.
  switch (datetime.precision()) {
    case DateTime::YEAR:
      return absl::Hours(24 * 366);
    case DateTime::MONTH:
      return absl::Hours(24 * 31);
    case DateTime::DAY:
      return absl::Hours(24);
    case DateTime::SECOND:
      return absl::Seconds(1);
    case DateTime::MILLISECOND:
      return absl::Milliseconds(1);
    case DateTime::MICROSECOND:
      return absl::Microseconds(1);
    default:
      LOG(FATAL) << "Unsupported datetime precision: " << datetime.precision();
  }
}

Status GetTimezone(const string& timezone_str, absl::TimeZone* tz) {
  // Try loading the timezone first.
  if (absl::LoadTimeZone(timezone_str, tz)) {
    return Status::OK();
  }
  static const LazyRE2 kFixedTimezoneRegex{"([+-])(\\d\\d):(\\d\\d)"};
  string sign;
  string hour_str;
  string minute_str;
  if (RE2::FullMatch(timezone_str, *kFixedTimezoneRegex, &sign, &hour_str,
                     &minute_str)) {
    int hour = 0;
    int minute = 0;
    int seconds_offset = 0;
    if (!absl::SimpleAtoi(hour_str, &hour) || hour > 14) {
      return ::tensorflow::errors::InvalidArgument(
          absl::StrCat("Invalid timezone format: ", timezone_str));
    }
    seconds_offset += hour * 60 * 60;
    if (!absl::SimpleAtoi(minute_str, &minute) || minute > 59) {
      return ::tensorflow::errors::InvalidArgument(
          absl::StrCat("Invalid timezone format: ", timezone_str));
    }
    seconds_offset += minute * 60;
    if (sign == "-") {
      seconds_offset = -seconds_offset;
    }
    *tz = absl::FixedTimeZone(seconds_offset);
    return Status::OK();
  }

  return ::tensorflow::errors::InvalidArgument(
      absl::StrCat("Invalid timezone format: ", timezone_str));
}

StatusOr<string> GetResourceId(const Message& message) {
  const auto* desc = message.GetDescriptor();
  const google::protobuf::Reflection* ref = message.GetReflection();
  const google::protobuf::FieldDescriptor* field = desc->FindFieldByName("id");
  const Message* entry_message = &message;
  if (field->is_repeated()) {
    return ::tensorflow::errors::InvalidArgument(
        "Unexpected repeated id field");
  }
  if (field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    return ::tensorflow::errors::InvalidArgument(
        "No id field found on message");
  }
  if (field->message_type()->full_name() != Id::descriptor()->full_name()) {
    return ::tensorflow::errors::InvalidArgument(absl::StrCat(
        "id field is not a singular STU3 String: ", desc->full_name()));
  }
  const auto* id_message =
      dynamic_cast<const Id*>(&ref->GetMessage(*entry_message, field));
  return id_message->value();
}

StatusOr<Reference> ReferenceStringToProto(const string& input) {
  static const LazyRE2 kInternalReferenceRegex{
      "([0-9A-Za-z_]+)/([A-Za-z0-9.-]{1,64})(?:/_history/"
      "([A-Za-z0-9.-]{1,64}))?"};
  string resource_type;
  string resource_id;
  string version;
  if (RE2::FullMatch(input, *kInternalReferenceRegex, &resource_type,
                     &resource_id, &version)) {
    const string field_name = absl::StrCat(ToSnakeCase(resource_type), "_id");
    const google::protobuf::FieldDescriptor* field =
        Reference::descriptor()->FindFieldByName(field_name);
    if (field == nullptr) {
      return ::tensorflow::errors::InvalidArgument(
          absl::StrCat("Resource type ", resource_type,
                       " is not valid for a reference (field ", field_name,
                       " does not exist)."));
    }
    Reference reference;
    ReferenceId* reference_id = dynamic_cast<ReferenceId*>(
        reference.GetReflection()->MutableMessage(&reference, field));
    reference_id->set_value(resource_id);
    if (!version.empty()) {
      reference_id->mutable_history()->set_value(version);
    }
    return reference;
  }

  static const LazyRE2 kFragmentReferenceRegex{"#[A-Za-z0-9.-]{1,64}"};
  if (RE2::FullMatch(input, *kFragmentReferenceRegex)) {
    Reference reference;
    reference.mutable_fragment()->set_value(input.substr(1));
    return reference;
  }

  // We're permissive about various full url schemes.
  static LazyRE2 kUrlReference = {"(http|https|urn):.*"};
  if (RE2::FullMatch(input, *kUrlReference)) {
    Reference reference;
    reference.mutable_uri()->set_value(input);
    return reference;
  }
  return ::tensorflow::errors::InvalidArgument(
      absl::StrCat("String \"", input, "\" cannot be parsed as a reference."));
}

string GetReferenceToResource(const Message& message) {
  return absl::StrCat(message.GetDescriptor()->name(), "/",
                      GetResourceId(message).ValueOrDie());
}

Status GetDecimalValue(const stu3::proto::Decimal& decimal, double* value) {
  if (!absl::SimpleAtod(decimal.value(), value)) {
    return ::tensorflow::errors::InvalidArgument(
        absl::StrCat("Invalid decimal: '", decimal.value(), "'"));
  }
  return Status::OK();
}

Status GetResourceFromBundleEntry(const Bundle::Entry& entry,
                                  const Message** result) {
  auto got_value = GetContainedResource(entry.resource());
  TF_RETURN_IF_ERROR(got_value.status());
  *result = got_value.ValueOrDie();
  return Status::OK();
}

StatusOr<const google::protobuf::RepeatedFieldRef<stu3::proto::Extension>>
GetResourceExtensionsFromBundleEntry(const Bundle::Entry& entry) {
  const Message* resource;
  TF_RETURN_IF_ERROR(GetResourceFromBundleEntry(entry, &resource));
  const google::protobuf::Reflection* ref = resource->GetReflection();
  // Get the bundle field corresponding to this resource.
  const google::protobuf::FieldDescriptor* field =
      resource->GetDescriptor()->FindFieldByName("extension");
  if (field == nullptr) {
    return ::tensorflow::errors::NotFound("No extension field.");
  }
  return ref->GetRepeatedFieldRef<stu3::proto::Extension>(*resource, field);
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
