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

#include "google/fhir/stu3/profiles.h"

#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/status/status.h"
#include "google/fhir/stu3/extensions.h"
#include "google/fhir/stu3/resource_validation.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/logging.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;
using ::google::fhir::stu3::proto::CodeableConcept;
using ::google::fhir::stu3::proto::Coding;
using ::google::fhir::stu3::proto::CodingWithFixedCode;
using ::google::fhir::stu3::proto::CodingWithFixedSystem;
using ::google::fhir::stu3::proto::Extension;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::FieldOptions;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using ::google::protobuf::RepeatedPtrField;
using ::tensorflow::errors::InvalidArgument;

namespace {

const string GetCodeableConceptUrl() {
  static const string url =
      CodeableConcept::descriptor()->options().GetExtension(
          stu3::proto::fhir_structure_definition_url);
  return url;
}

// Finds any extensions that can be slotted into slices, and moves them.
Status PerformExtensionSlicing(Message* message) {
  const Descriptor* descriptor = message->GetDescriptor();
  const Reflection* reflection = message->GetReflection();

  const FieldDescriptor* extension_field =
      descriptor->FindFieldByName("extension");
  if (!extension_field) {
    // Nothing to slice
    return Status::OK();
  }

  std::unordered_map<string, const FieldDescriptor*> extension_map;
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    if (field->options().HasExtension(proto::fhir_inlined_extension_url)) {
      extension_map[field->options().GetExtension(
          proto::fhir_inlined_extension_url)] = field;
    }
  }

  RepeatedPtrField<Message>* extensions =
      reflection->MutableRepeatedPtrField<Message>(message, extension_field);
  for (auto iter = extensions->begin(); iter != extensions->end();) {
    Extension& raw_extension = dynamic_cast<Extension&>(*iter);
    const string& url = raw_extension.url().value();
    const auto extension_entry_iter = extension_map.find(url);
    if (extension_entry_iter != extension_map.end()) {
      // This extension can be sliced into an inlined field.
      const FieldDescriptor* inlined_field = extension_entry_iter->second;
      if (raw_extension.extension_size() == 0) {
        // This is a simple extension
        // Note that we cannot use ExtensionToMessage from extensions.h
        // here, because there is no top-level extension message we're merging
        // into, it's just a single primitive inlined field.
        const Descriptor* destination_type = inlined_field->message_type();
        const Extension::Value& value = raw_extension.value();
        const FieldDescriptor* value_field =
            value.GetReflection()->GetOneofFieldDescriptor(
                value, value.GetDescriptor()->FindOneofByName("value"));
        if (value_field == nullptr) {
          return InvalidArgument(
              absl::StrCat("Invalid extension: neither value nor extensions "
                           "set on extension ",
                           url));
        }
        if (destination_type != value_field->message_type()) {
          return InvalidArgument(
              "Profiled extension slice is incorrect type: ", url, "should be ",
              destination_type->full_name(), "but is ",
              value_field->message_type()->full_name());
        }
        Message* typed_extension =
            inlined_field->is_repeated()
                ? reflection->AddMessage(message, inlined_field)
                : reflection->MutableMessage(message, inlined_field);
        typed_extension->CopyFrom(
            value.GetReflection()->GetMessage(value, value_field));
      } else {
        // This is a complex extension
        Message* typed_extension =
            inlined_field->is_repeated()
                ? reflection->AddMessage(message, inlined_field)
                : reflection->MutableMessage(message, inlined_field);
        TF_RETURN_IF_ERROR(ExtensionToMessage(raw_extension, typed_extension));
      }
      // Finally, remove the original from the unsliced extension field.
      iter = extensions->erase(iter);
    } else {
      // There is no inlined field for this extension
      iter++;
    }
  }
  return Status::OK();
}

Status SliceCodingsInCodeableConcept(Message* codeable_concept_like) {
  const Descriptor* descriptor = codeable_concept_like->GetDescriptor();
  const Reflection* reflection = codeable_concept_like->GetReflection();

  // For fixed system slices, map from System -> CodingWithFixedSystem slice
  std::unordered_map<string, const FieldDescriptor*> fixed_systems;
  // For fixed code slices, map from System -> {code, CodingWithFixedCode slice}
  std::unordered_map<string, std::tuple<string, const FieldDescriptor*>>
      fixed_codes;
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    const FieldOptions& field_options = field->options();
    if (field_options.HasExtension(proto::fhir_inlined_coding_system)) {
      if (field->is_repeated()) {
        return InvalidArgument("Unexpected repeated coding slice: ",
                               field->full_name());
      }
      const string& fixed_system =
          field_options.GetExtension(proto::fhir_inlined_coding_system);
      if (field_options.HasExtension(proto::fhir_inlined_coding_code)) {
        fixed_codes[fixed_system] = std::make_tuple(
            field_options.GetExtension(proto::fhir_inlined_coding_code), field);
      } else {
        fixed_systems[fixed_system] = field;
      }
    }
  }

  const FieldDescriptor* coding_field = descriptor->FindFieldByName("coding");
  if (!coding_field || !coding_field->is_repeated() ||
      coding_field->message_type()->full_name() !=
          Coding::descriptor()->full_name()) {
    return InvalidArgument(
        "Cannot slice CodeableConcept-like: ", descriptor->full_name(),
        ".  No repeated coding field found.");
  }

  RepeatedPtrField<Message>* codings =
      reflection->MutableRepeatedPtrField<Message>(codeable_concept_like,
                                                   coding_field);
  for (auto iter = codings->begin(); iter != codings->end();) {
    Coding& coding = dynamic_cast<Coding&>(*iter);
    const string& system = coding.system().value();
    auto fixed_systems_iter = fixed_systems.find(system);
    auto fixed_code_iter = fixed_codes.find(system);
    if (fixed_systems_iter != fixed_systems.end()) {
      const FieldDescriptor* field = fixed_systems_iter->second;
      CodingWithFixedSystem* sliced_coding = dynamic_cast<CodingWithFixedSystem*>(
          reflection->MutableMessage(codeable_concept_like, field));
      coding.clear_system();
      if (!sliced_coding->ParseFromString(coding.SerializeAsString())) {
        return InvalidArgument(
            "Unable to parse Coding as CodingWithFixedSystem: ",
            field->full_name());
      }
      iter = codings->erase(iter);
    } else if (fixed_code_iter != fixed_codes.end()) {
      const std::tuple<const string, const FieldDescriptor*> code_field_tuple =
          fixed_code_iter->second;
      const string& code = std::get<0>(code_field_tuple);
      const FieldDescriptor* field = std::get<1>(code_field_tuple);
      if (coding.code().value() != code) {
        return InvalidArgument("Invalid fixed code slice on ",
                               field->full_name(), ": Expected code ", code,
                               " for system ", system,
                               ".  Found: ", coding.code().value());
      }
      CodingWithFixedCode* sliced_coding = dynamic_cast<CodingWithFixedCode*>(
          reflection->MutableMessage(codeable_concept_like, field));
      coding.clear_system();
      coding.clear_code();
      if (!sliced_coding->ParseFromString(coding.SerializeAsString())) {
        return InvalidArgument(
            "Unable to parse Coding as CodingWithFixedCode: ",
            field->full_name());
      }
      iter = codings->erase(iter);
    } else {
      iter++;
    }
  }
  return Status::OK();
}

Status PerformCodeableConceptSlicing(Message* message) {
  const Descriptor* descriptor = message->GetDescriptor();
  const Reflection* reflection = message->GetReflection();

  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    const Descriptor* field_type = field->message_type();
    if (!field_type) {
      return InvalidArgument("Encountered unexpected primitive field: ",
                             field->full_name());
    }
    if (field_type->options().HasExtension(proto::fhir_profile_base) &&
        field_type->options().GetExtension(proto::fhir_profile_base) ==
            GetCodeableConceptUrl()) {
      if (field->is_repeated()) {
        return InvalidArgument("Unexpected repeated CodeableConcept: ",
                               field->full_name());
      }
      if (reflection->HasField(*message, field)) {
        FHIR_RETURN_IF_ERROR(SliceCodingsInCodeableConcept(
            reflection->MutableMessage(message, field)));
      }
    }
  }
  return Status::OK();
}

Status PerformSlicing(Message* message) {
  FHIR_RETURN_IF_ERROR(PerformExtensionSlicing(message));
  FHIR_RETURN_IF_ERROR(PerformCodeableConceptSlicing(message));
  // TODO: Perform generic slicing

  // There are two kinds of subfields that could potentially have slices:
  // 1) "Backbone" i.e. nested types defined on this message
  // 2) Types that are themselves profiles
  const Descriptor* descriptor = message->GetDescriptor();
  const Reflection* reflection = message->GetReflection();
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    const Descriptor* field_type = field->message_type();
    if (!field_type) {
      return InvalidArgument("Encountered unexpected primitive type on field: ",
                             field->full_name());
    }
    const bool is_nested_type =
        field_type->full_name().rfind(descriptor->full_name(), 0) == 0;
    const bool is_profile =
        field_type->options().HasExtension(proto::fhir_profile_base);
    if (is_nested_type || is_profile) {
      if (field->is_repeated()) {
        for (int i = 0; i < reflection->FieldSize(*message, field); i++) {
          FHIR_RETURN_IF_ERROR(
              PerformSlicing(message->GetReflection()->MutableRepeatedMessage(
                  message, field, i)));
        }
      } else if (reflection->HasField(*message, field)) {
        FHIR_RETURN_IF_ERROR(PerformSlicing(
            message->GetReflection()->MutableMessage(message, field)));
      }
    }
  }
  return Status::OK();
}

}  // namespace

Status ConvertToProfile(const Message& base_message,
                        Message* profiled_message) {
  FHIR_RETURN_IF_ERROR(ConvertToProfileLenient(base_message, profiled_message));
  return ValidateResource(*profiled_message);
}

Status ConvertToProfileLenient(const Message& base_message,
                               Message* profiled_message) {
  const Descriptor* base_descriptor = base_message.GetDescriptor();
  const Descriptor* profiled_descriptor = profiled_message->GetDescriptor();

  // Copy over the base_message message into the profiled_message container.
  // We can't use CopyFrom because the messages are technically different,
  // but since all the fields from the base_message have matching fields
  // in profiled_message, we can serialize the base_message message to bytes,
  // and then deserialise it into the profiled_message container.
  // TODO: check to see how this handles fields that were removed
  // in the proto.  We should return an InvalidArgument.
  if (!profiled_message->ParseFromString(base_message.SerializeAsString())) {
    // TODO: walk up the parent-tree to see if this was a valid
    // request to begin with.  This is non-trivial though, because we only have
    // base by URL, and no way to go from URL to proto in order to get the next
    // level of parent.
    return InvalidArgument(
        "Unable to convert ", base_descriptor->full_name(), " to ",
        profiled_descriptor->full_name(),
        ".  They are not binary compatible.  This could mean the profile ",
        "applies constraints that the resource does not meet.");
  }
  return PerformSlicing(profiled_message);
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
