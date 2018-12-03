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
#include "google/fhir/stu3/codes.h"
#include "google/fhir/stu3/extensions.h"
#include "google/fhir/stu3/proto_util.h"
#include "google/fhir/stu3/resource_validation.h"
#include "google/fhir/stu3/util.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/logging.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;
using ::google::fhir::stu3::proto::Code;
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

//////////////////////////////////////////////////////////////
// Slicing functions begins here
//
// These functions help with going from Unprofiled -> Profiled

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
        const FieldDescriptor* src_datatype_field =
            value.GetReflection()->GetOneofFieldDescriptor(
                value, value.GetDescriptor()->FindOneofByName("value"));
        if (src_datatype_field == nullptr) {
          return InvalidArgument(
              absl::StrCat("Invalid extension: neither value nor extensions "
                           "set on extension ",
                           url));
        }
        Message* typed_extension = MutableOrAddMessage(message, inlined_field);
        const Message& src_value =
            value.GetReflection()->GetMessage(value, src_datatype_field);
        if (destination_type == src_datatype_field->message_type()) {
          typed_extension->CopyFrom(src_value);
        } else if (src_datatype_field->message_type()->full_name() ==
                   Code::descriptor()->full_name()) {
          TF_RETURN_IF_ERROR(ConvertToTypedCode(
              dynamic_cast<const Code&>(src_value), typed_extension));
        } else {
          return InvalidArgument(
              "Profiled extension slice is incorrect type: ", url, "should be ",
              destination_type->full_name(), " but is ",
              src_datatype_field->message_type()->full_name());
        }
      } else {
        // This is a complex extension
        Message* typed_extension = MutableOrAddMessage(message, inlined_field);
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
      !IsMessageType<Coding>(coding_field->message_type())) {
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
          MutableOrAddMessage(codeable_concept_like, field));
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
          MutableOrAddMessage(codeable_concept_like, field));
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
            GetStructureDefinitionUrl(CodeableConcept::descriptor())) {
      if (field->is_repeated()) {
        for (int i = 0; i < reflection->FieldSize(*message, field); i++) {
          FHIR_RETURN_IF_ERROR(SliceCodingsInCodeableConcept(
              reflection->MutableRepeatedMessage(message, field, i)));
        }
      } else {
        if (reflection->HasField(*message, field)) {
          FHIR_RETURN_IF_ERROR(SliceCodingsInCodeableConcept(
              reflection->MutableMessage(message, field)));
        }
      }
    }
  }
  return Status::OK();
}

bool CanHaveSlicing(const FieldDescriptor* field) {
  if (IsChoiceType(field)) {
    return false;
  }
  // There are two kinds of subfields that could potentially have slices:
  // 1) Types that are themselves profiles
  // 2) "Backbone" i.e. nested types defined on this message
  const Descriptor* field_type = field->message_type();
  const string& profile_base = GetFhirProfileBase(field_type);
  if (!profile_base.empty()) {
    if (profile_base ==
            GetStructureDefinitionUrl(CodeableConcept::descriptor()) ||
        profile_base == GetStructureDefinitionUrl(Extension::descriptor())) {
      // Profiles on Extensions and CodeableConcepts are the slices themselves,
      // rather than elements that *have* slices.
      return false;
    }
    return true;
  }
  // The type is a nested message defined on this type if its full name starts
  // with the full name of the containing type.
  return field_type->full_name().rfind(field->containing_type()->full_name(),
                                       0) == 0;
}

Status PerformSlicing(Message* message) {
  FHIR_RETURN_IF_ERROR(PerformExtensionSlicing(message));
  FHIR_RETURN_IF_ERROR(PerformCodeableConceptSlicing(message));
  // TODO: Perform generic slicing

  const Descriptor* descriptor = message->GetDescriptor();
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    const Descriptor* field_type = field->message_type();
    if (!field_type) {
      return InvalidArgument("Encountered unexpected primitive type on field: ",
                             field->full_name());
    }
    if (CanHaveSlicing(field)) {
      for (int j = 0; j < PotentiallyRepeatedFieldSize(*message, field); j++) {
        FHIR_RETURN_IF_ERROR(PerformSlicing(
            MutablePotentiallyRepeatedMessage(message, field, j)));
      }
    }
  }
  return Status::OK();
}

//////////////////////////////////////////////////////////////
// Unslicing functions begins here
//
// These functions help with going from Profiled -> Unprofiled

Status PerformExtensionUnslicing(const google::protobuf::Message& profiled_message,
                                 google::protobuf::Message* base_message) {
  const Descriptor* profiled_descriptor = profiled_message.GetDescriptor();
  const Descriptor* base_descriptor = base_message->GetDescriptor();
  const Reflection* profiled_reflection = profiled_message.GetReflection();
  const Reflection* base_reflection = base_message->GetReflection();

  for (int i = 0; i < profiled_descriptor->field_count(); i++) {
    const FieldDescriptor* field = profiled_descriptor->field(i);
    if (!field->options().HasExtension(proto::fhir_inlined_extension_url) ||
        !FieldHasValue(profiled_message, field)) {
      // We only care about present fields with inlined extension urls.
      continue;
    }
    // We've found a profiled extension field.
    // Convert it into a vanilla extension on the base message.
    Extension* raw_extension =
        dynamic_cast<Extension*>(base_reflection->AddMessage(
            base_message, base_descriptor->FindFieldByName("extension")));
    const Message& extension_as_message =
        profiled_reflection->GetMessage(profiled_message, field);
    if (GetFhirProfileBase(extension_as_message.GetDescriptor()) ==
        GetStructureDefinitionUrl(Extension::descriptor())) {
      // This a profile on extension, and therefore a complex extension
      FHIR_RETURN_IF_ERROR(
          ConvertToExtension(extension_as_message, raw_extension));
    } else {
      // This just a raw datatype, and therefore a simple extension
      raw_extension->mutable_url()->set_value(
          field->options().GetExtension(proto::fhir_inlined_extension_url));
      FHIR_RETURN_IF_ERROR(
          SetDatatypeOnExtension(extension_as_message, raw_extension));
    }
  }

  return Status::OK();
}

Status UnsliceCodingsWithinCodeableConcept(
    const google::protobuf::Message& profiled_concept, CodeableConcept* target_concept) {
  const Descriptor* profiled_descriptor = profiled_concept.GetDescriptor();
  for (int i = 0; i < profiled_descriptor->field_count(); i++) {
    const FieldDescriptor* profiled_field = profiled_descriptor->field(i);
    if (GetFhirProfileBase(profiled_field->message_type()) ==
        GetStructureDefinitionUrl(Coding::descriptor())) {
      // This is a specialization of Coding.
      // Convert it to a normal coding and add it to the base message.
      for (int i = 0;
           i < PotentiallyRepeatedFieldSize(profiled_concept, profiled_field);
           i++) {
        const Message& profiled_coding =
            GetPotentiallyRepeatedMessage(profiled_concept, profiled_field, i);
        Coding* new_coding = target_concept->add_coding();

        // Copy over as many fields as we can.
        new_coding->ParseFromString(profiled_coding.SerializeAsString());
        new_coding->DiscardUnknownFields();

        // If we see an inlined system or code, copy that over.
        const string& inlined_system = profiled_field->options().GetExtension(
            proto::fhir_inlined_coding_system);
        if (!inlined_system.empty()) {
          new_coding->mutable_system()->set_value(inlined_system);
        }

        const string& inlined_code = profiled_field->options().GetExtension(
            proto::fhir_inlined_coding_code);
        if (!inlined_code.empty()) {
          new_coding->mutable_code()->set_value(inlined_code);
        }
      }
    }
  }
  return Status::OK();
}

Status PerformCodeableConceptUnslicing(const google::protobuf::Message& profiled_message,
                                       google::protobuf::Message* base_message) {
  const Descriptor* profiled_descriptor = profiled_message.GetDescriptor();
  const Descriptor* base_descriptor = base_message->GetDescriptor();

  // Iterate over all the base fields that are of type CodeableConcept,
  // and find the ones that are *not* CodeableConcepts in the profiled message.
  // For these, find any additional codings that are defined as inlined fields
  // on the profiled message, and add them as raw codings on the base message.
  for (int i = 0; i < base_descriptor->field_count(); i++) {
    const FieldDescriptor* base_field = base_descriptor->field(i);
    const FieldDescriptor* profiled_field = profiled_descriptor->field(i);
    if (!FieldHasValue(profiled_message, profiled_field)) {
      continue;
    }
    if (IsMessageType<CodeableConcept>(base_field->message_type()) &&
        !IsMessageType<CodeableConcept>(profiled_field->message_type())) {
      for (int j = 0;
           j < PotentiallyRepeatedFieldSize(profiled_message, profiled_field);
           j++) {
        FHIR_RETURN_IF_ERROR(UnsliceCodingsWithinCodeableConcept(
            GetPotentiallyRepeatedMessage(profiled_message, profiled_field, j),
            dynamic_cast<CodeableConcept*>(MutablePotentiallyRepeatedMessage(
                base_message, base_field, j))));
      }
    }
  }

  return Status::OK();
}

Status PerformUnslicing(const google::protobuf::Message& profiled_message,
                        google::protobuf::Message* base_message) {
  FHIR_RETURN_IF_ERROR(
      PerformExtensionUnslicing(profiled_message, base_message));
  FHIR_RETURN_IF_ERROR(
      PerformCodeableConceptUnslicing(profiled_message, base_message));

  const Descriptor* profiled_descriptor = profiled_message.GetDescriptor();
  const Descriptor* base_descriptor = base_message->GetDescriptor();
  for (int i = 0; i < profiled_descriptor->field_count(); i++) {
    const FieldDescriptor* profiled_field = profiled_descriptor->field(i);
    const FieldDescriptor* base_field = base_descriptor->field(i);
    const Descriptor* field_type = profiled_field->message_type();
    if (!field_type) {
      return InvalidArgument("Encountered unexpected primitive type on field: ",
                             profiled_field->full_name());
    }
    if (CanHaveSlicing(profiled_field)) {
      for (int j = 0;
           j < PotentiallyRepeatedFieldSize(profiled_message, profiled_field);
           j++) {
        FHIR_RETURN_IF_ERROR(PerformUnslicing(
            GetPotentiallyRepeatedMessage(profiled_message, profiled_field, j),
            MutablePotentiallyRepeatedMessage(base_message, base_field, j)));
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

Status ConvertToBaseResource(const google::protobuf::Message& profiled_message,
                             google::protobuf::Message* base_message) {
  const Descriptor* profiled_descriptor = profiled_message.GetDescriptor();
  const Descriptor* base_descriptor = base_message->GetDescriptor();

  // Serialize the profiled message, and re-parse as a base message.
  // This is possible because when generating profiled messages, we ensure that
  // any fields that are common between the two messages share identical field
  // numbers.  Any fields that are only present on the profiled message will
  // show up as unknown fields.
  // We will copy that data over by inspecting these fields on the profiled
  // message.
  // TODO: Benchmark this against other approaches.
  if (!base_message->ParseFromString(profiled_message.SerializeAsString())) {
    return InvalidArgument(
        "Unable to convert ", profiled_descriptor->full_name(), " to ",
        base_descriptor->full_name(), ".  They are not binary compatible.");
  }
  // Unknown fields represent profiled fields that aren't present on the base
  // message.  We toss them out here, and then handled these fields in the
  // profiled message by reading annotations.
  base_message->DiscardUnknownFields();
  return PerformUnslicing(profiled_message, base_message);
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
