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

#ifndef GOOGLE_FHIR_STU3_PROFILES_LIB_H_
#define GOOGLE_FHIR_STU3_PROFILES_LIB_H_

#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codes.h"
#include "google/fhir/extensions.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/resource_validation.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/logging.h"

namespace google {
namespace fhir {

namespace profiles_internal {

using std::string;
using ::google::fhir::proto::fhir_fixed_system;
using ::google::fhir::proto::fhir_inlined_coding_code;
using ::google::fhir::proto::fhir_inlined_coding_system;
using ::google::fhir::stu3::proto::CodingWithFixedSystem;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::FieldOptions;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using ::google::protobuf::RepeatedPtrField;
using ::tensorflow::errors::InvalidArgument;

struct CodeableConceptSlicingInfo {
  // For fixed system slices, map from System -> legacy CodingWithFixedSystem
  // slice
  // TODO: These annotations only exist for legacy STU3.  Once STU3
  // is converted to inlined types, remove this.
  std::unordered_map<string, const FieldDescriptor*>
      legacy_coding_with_fixed_system;
  // For fixed system slices, map from System -> r4 inlined Coding message
  std::unordered_map<string, const FieldDescriptor*>
      inlined_codings_with_fixed_system;
  // For fixed code slices, map from System -> {code, CodingWithFixedCode slice}
  std::unordered_map<string, std::tuple<string, const FieldDescriptor*>>
      coding_with_fixed_code;
};

//////////////////////////////////////////////////////////////
// Slicing functions begins here
//
// These functions help with going from Unprofiled -> Profiled

// Finds any extensions that can be slotted into slices, and moves them.
template <typename ExtensionLike,
          typename CodeLike = FHIR_DATATYPE(ExtensionLike, code)>
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
    if (HasInlinedExtensionUrl(field)) {
      extension_map[GetInlinedExtensionUrl(field)] = field;
    }
  }

  RepeatedPtrField<Message>* extensions =
      reflection->MutableRepeatedPtrField<Message>(message, extension_field);
  for (auto iter = extensions->begin(); iter != extensions->end();) {
    ExtensionLike& raw_extension = dynamic_cast<ExtensionLike&>(*iter);
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
        const auto& value = raw_extension.value();
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
                   CodeLike::descriptor()->full_name()) {
          TF_RETURN_IF_ERROR(ConvertToTypedCode(
              dynamic_cast<const CodeLike&>(src_value), typed_extension));
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

template <typename CodingLike>
StatusOr<bool> CheckCodingWithFixedCode(
    const CodeableConceptSlicingInfo& slicing_info, CodingLike* coding,
    Message* profiled_codeable_concept) {
  const string& system = coding->system().value();
  const string& coding_string = coding->code().value();
  auto fixed_code_iter = slicing_info.coding_with_fixed_code.find(system);

  if (fixed_code_iter != slicing_info.coding_with_fixed_code.end()) {
    const std::tuple<const string, const FieldDescriptor*> code_field_tuple =
        fixed_code_iter->second;
    const string& fixed_code_string = std::get<0>(code_field_tuple);
    if (coding_string == fixed_code_string) {
      const FieldDescriptor* field = std::get<1>(code_field_tuple);
      Message* coding_with_fixed_system =
          MutableOrAddMessage(profiled_codeable_concept, field);
      if (!coding_with_fixed_system->ParseFromString(
              coding->SerializeAsString())) {
        return InvalidArgument(
            "Unable to parse Coding as CodingWithFixedCode.");
      }
      coding_with_fixed_system->DiscardUnknownFields();

      std::vector<const FieldDescriptor*> set_fields;
      coding_with_fixed_system->GetReflection()->ListFields(
          *coding_with_fixed_system, &set_fields);
      if (set_fields.empty()) {
        // If there's no information other than the fixed code and system,
        // we don't need to add a message.
        const Reflection* profiled_codeable_concept_ref =
            profiled_codeable_concept->GetReflection();

        if (field->is_repeated()) {
          profiled_codeable_concept_ref->RemoveLast(profiled_codeable_concept,
                                                    field);
        } else {
          profiled_codeable_concept_ref->ClearField(profiled_codeable_concept,
                                                    field);
        }
      }
      return true;  // was copied into new field
    }
  }
  return false;
}

template <typename CodingLike,
          typename CodeLike = FHIR_DATATYPE(CodingLike, code)>
StatusOr<bool> CheckCodingWithFixedSystem(
    const CodeableConceptSlicingInfo& slicing_info, CodingLike* coding,
    Message* profiled_codeable_concept) {
  const string& system = coding->system().value();
  auto inlined_codings_with_fixed_system_iter =
      slicing_info.inlined_codings_with_fixed_system.find(system);
  if (inlined_codings_with_fixed_system_iter !=
      slicing_info.inlined_codings_with_fixed_system.end()) {
    const FieldDescriptor* target_coding_field =
        inlined_codings_with_fixed_system_iter->second;
    Message* inlined_coding =
        MutableOrAddMessage(profiled_codeable_concept, target_coding_field);
    CodeLike code = coding->code();
    coding->clear_system();
    coding->clear_code();
    if (!inlined_coding->ParseFromString(coding->SerializeAsString())) {
      return InvalidArgument("Unable to parse Coding as ",
                             inlined_coding->GetDescriptor()->full_name(), ": ",
                             target_coding_field->full_name());
    }
    const FieldDescriptor* code_field =
        inlined_coding->GetDescriptor()->FindFieldByName("code");
    if (!code_field ||
        code_field->type() != google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
      return InvalidArgument(
          "Inlined Coding message does not have a valid 'code' field.");
    }
    Message* target_code = MutableOrAddMessage(inlined_coding, code_field);
    FHIR_RETURN_IF_ERROR(ConvertToTypedCode(code, target_code));
    return true;  // was copied into new field
  }
  return false;
}

// TODO: Delete this (and surrounding logic) once STU3 protos are
// updated to use non-legacy fixed systems.
template <typename CodingLike>
StatusOr<bool> CheckLegacyCodingWithFixedSystem(
    const CodeableConceptSlicingInfo& slicing_info, CodingLike* coding,
    Message* profiled_codeable_concept) {
  const string& system = coding->system().value();
  auto legacy_coding_with_fixed_system_iter =
      slicing_info.legacy_coding_with_fixed_system.find(system);
  if (legacy_coding_with_fixed_system_iter !=
      slicing_info.legacy_coding_with_fixed_system.end()) {
    const FieldDescriptor* field = legacy_coding_with_fixed_system_iter->second;
    CodingWithFixedSystem* sliced_coding = dynamic_cast<CodingWithFixedSystem*>(
        MutableOrAddMessage(profiled_codeable_concept, field));
    coding->clear_system();
    if (!sliced_coding->ParseFromString(coding->SerializeAsString())) {
      return InvalidArgument(
          "Unable to parse Coding as CodingWithFixedSystem: ",
          field->full_name());
    }
    return true;  // was copied into new field
  }
  return false;
}

// Examines a single Coding on a CodeableConcept, and attemts to copy it to
// a profiled field if an appropriate one exists.
// Returns true if the field was copied into a profiled field (and thus should
// be removed from its original home), or false if it was not copied (and should
// remain in original home).
// Returns a status failure if it found a profiled field but could not copy into
// it.
template <typename CodingLike,
          typename CodeLike = FHIR_DATATYPE(CodingLike, code)>
StatusOr<bool> SliceOneCoding(const CodeableConceptSlicingInfo& slicing_info,
                              CodingLike* coding,
                              Message* codeable_concept_like) {
  FHIR_ASSIGN_OR_RETURN(const bool sliced_into_fixed_code,
                        CheckCodingWithFixedCode<CodingLike>(
                            slicing_info, coding, codeable_concept_like));
  if (sliced_into_fixed_code) return true;

  FHIR_ASSIGN_OR_RETURN(const bool sliced_into_fixed_system,
                        CheckCodingWithFixedSystem<CodingLike>(
                            slicing_info, coding, codeable_concept_like));
  if (sliced_into_fixed_system) return true;

  FHIR_ASSIGN_OR_RETURN(const bool sliced_into_legacy_fixed_system,
                        CheckLegacyCodingWithFixedSystem<CodingLike>(
                            slicing_info, coding, codeable_concept_like));
  if (sliced_into_legacy_fixed_system) return true;


  return false;  // was not copied
}

// TODO: CodingWithFixedSystem is a legacy pattern that only exists
// in obselete versions of STU3.  Eliminate once STU3 uses the inlined message
// pattern.
template <typename ExtensionLike,
          typename CodingLike = FHIR_DATATYPE(ExtensionLike, coding),
          typename CodeLike = FHIR_DATATYPE(ExtensionLike, code)>
Status SliceCodingsInCodeableConcept(Message* codeable_concept_like) {
  const Descriptor* descriptor = codeable_concept_like->GetDescriptor();
  const Reflection* reflection = codeable_concept_like->GetReflection();

  CodeableConceptSlicingInfo slicing_info;

  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    const FieldOptions& field_options = field->options();
    if (field_options.HasExtension(fhir_inlined_coding_system)) {
      const string& legacy_fixed_system =
          field_options.GetExtension(fhir_inlined_coding_system);
      if (field_options.HasExtension(fhir_inlined_coding_code)) {
        slicing_info.coding_with_fixed_code[legacy_fixed_system] =
            std::make_tuple(
                field_options.GetExtension(fhir_inlined_coding_code), field);
      } else {
        slicing_info.legacy_coding_with_fixed_system[legacy_fixed_system] =
            field;
      }
    } else if (field->message_type()->options().HasExtension(
                   fhir_fixed_system)) {
      slicing_info.inlined_codings_with_fixed_system
          [field->message_type()->options().GetExtension(fhir_fixed_system)] =
          field;
    }
  }

  const FieldDescriptor* coding_field = descriptor->FindFieldByName("coding");
  if (!coding_field || !coding_field->is_repeated() ||
      !IsMessageType<CodingLike>(coding_field->message_type())) {
    return InvalidArgument(
        "Cannot slice CodeableConcept-like: ", descriptor->full_name(),
        ".  No repeated coding field found.");
  }

  RepeatedPtrField<CodingLike>* codings =
      reflection->MutableRepeatedPtrField<CodingLike>(codeable_concept_like,
                                                      coding_field);
  for (auto iter = codings->begin(); iter != codings->end();) {
    FHIR_ASSIGN_OR_RETURN(const bool was_copied_into_profiled_field,
                          SliceOneCoding<CodingLike>(slicing_info, &(*iter),
                                                     codeable_concept_like));
    if (was_copied_into_profiled_field) {
      iter = codings->erase(iter);
    } else {
      iter++;
    }
  }
  return Status::OK();
}

template <typename ExtensionLike, typename CodeableConceptLike = FHIR_DATATYPE(
                                      ExtensionLike, codeable_concept)>
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
    if (IsProfileOf<CodeableConceptLike>(field_type)) {
      if (field->is_repeated()) {
        for (int i = 0; i < reflection->FieldSize(*message, field); i++) {
          const auto& result = SliceCodingsInCodeableConcept<ExtensionLike>(
              reflection->MutableRepeatedMessage(message, field, i));
          FHIR_RETURN_IF_ERROR(result);
        }
      } else {
        if (reflection->HasField(*message, field)) {
          const auto& result = SliceCodingsInCodeableConcept<ExtensionLike>(
              reflection->MutableMessage(message, field));
          FHIR_RETURN_IF_ERROR(result);
        }
      }
    }
  }
  return Status::OK();
}

template <typename ExtensionLike, typename CodeableConceptLike = FHIR_DATATYPE(
                                      ExtensionLike, codeable_concept)>
bool CanHaveSlicing(const FieldDescriptor* field) {
  if (IsChoiceType(field)) {
    return false;
  }
  // There are three kinds of subfields that could potentially have slices:
  // 1) Types that are themselves profiles
  // 2) "Backbone" i.e. nested types defined on this message
  // 3) Contained resources of profiled bundles.  These are basically "profiles"
  //    of the base contained resources, but are not actually fhir elements.
  const Descriptor* field_type = field->message_type();
  // TODO: Use an annotation for this.
  if (field->message_type()->name() == "ContainedResource" &&
      (field->message_type()->full_name() !=
           "google.fhir.stu3.proto.ContainedResource" ||
       field->message_type()->full_name() !=
           "google.fhir.r4.proto.ContainedResource")) {
    return true;
  }
  if (IsProfile(field_type)) {
    if (IsProfileOf<CodeableConceptLike>(field_type) ||
        IsProfileOf<ExtensionLike>(field_type)) {
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

template <typename ExtensionLike>
Status PerformSlicing(Message* message) {
  FHIR_RETURN_IF_ERROR(PerformExtensionSlicing<ExtensionLike>(message));

  const auto& cc_slicing_status =
      PerformCodeableConceptSlicing<ExtensionLike>(message);
  FHIR_RETURN_IF_ERROR(cc_slicing_status);
  // TODO: Perform generic slicing

  const Descriptor* descriptor = message->GetDescriptor();
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    const Descriptor* field_type = field->message_type();
    if (!field_type) {
      return InvalidArgument("Encountered unexpected primitive type on field: ",
                             field->full_name());
    }
    if (CanHaveSlicing<ExtensionLike>(field)) {
      for (int j = 0; j < PotentiallyRepeatedFieldSize(*message, field); j++) {
        const auto& ps_status = PerformSlicing<ExtensionLike>(
            MutablePotentiallyRepeatedMessage(message, field, j));
        FHIR_RETURN_IF_ERROR(ps_status);
      }
    }
  }
  return Status::OK();
}

//////////////////////////////////////////////////////////////
// Unslicing functions begins here
//
// These functions help with going from Profiled -> Unprofiled

template <typename ExtensionLike>
Status PerformExtensionUnslicing(const google::protobuf::Message& profiled_message,
                                 google::protobuf::Message* base_message) {
  const Descriptor* profiled_descriptor = profiled_message.GetDescriptor();
  const Descriptor* base_descriptor = base_message->GetDescriptor();
  const Reflection* profiled_reflection = profiled_message.GetReflection();
  const Reflection* base_reflection = base_message->GetReflection();

  for (int i = 0; i < profiled_descriptor->field_count(); i++) {
    const FieldDescriptor* field = profiled_descriptor->field(i);
    if (!HasInlinedExtensionUrl(field) ||
        !FieldHasValue(profiled_message, field)) {
      // We only care about present fields with inlined extension urls.
      continue;
    }
    // Check if the extension slice exists on the base message.
    if (base_descriptor->FindFieldByNumber(field->number()) != nullptr) {
      // The field exists on both.  No special handling is needed.
      continue;
    }
    // We've found a profiled extension field.
    // Convert it into a vanilla extension on the base message.
    ExtensionLike* raw_extension =
        dynamic_cast<ExtensionLike*>(base_reflection->AddMessage(
            base_message, base_descriptor->FindFieldByName("extension")));
    const Message& extension_as_message =
        profiled_reflection->GetMessage(profiled_message, field);
    if (IsProfileOf<ExtensionLike>(extension_as_message)) {
      // This a profile on extension, and therefore a complex extension
      FHIR_RETURN_IF_ERROR(
          ConvertToExtension(extension_as_message, raw_extension));
    } else {
      // This just a raw datatype, and therefore a simple extension
      raw_extension->mutable_url()->set_value(GetInlinedExtensionUrl(field));

      FHIR_RETURN_IF_ERROR(
          SetDatatypeOnExtension(extension_as_message, raw_extension));
    }
  }

  return Status::OK();
}

template <typename CodingLike>
CodingLike* AddRawCoding(Message* concept) {
  return dynamic_cast<CodingLike*>(concept->GetReflection()->AddMessage(
      concept, concept->GetDescriptor()->FindFieldByName("coding")));
}

template <typename ExtensionLike,
          typename CodingLike = FHIR_DATATYPE(ExtensionLike, coding),
          typename CodeLike = FHIR_DATATYPE(ExtensionLike, code)>
Status UnsliceCodingsWithinCodeableConcept(
    const google::protobuf::Message& profiled_concept, Message* target_concept) {
  const Descriptor* profiled_descriptor = profiled_concept.GetDescriptor();
  for (int i = 0; i < profiled_descriptor->field_count(); i++) {
    const FieldDescriptor* profiled_field = profiled_descriptor->field(i);
    // Check if this is a newly profiled coding we need to handle.
    // This is the case if the field is a Profile of Coding AND
    // there is no corresonding profiled field in the base proto.
    if (IsProfileOf<CodingLike>(profiled_field->message_type()) &&
        target_concept->GetDescriptor()->FindFieldByNumber(
            profiled_field->number()) == nullptr) {
      // This is a specialization of Coding.
      // Convert it to a normal coding and add it to the base message.
      const int field_size =
          PotentiallyRepeatedFieldSize(profiled_concept, profiled_field);
      for (int i = 0; i < field_size; i++) {
        const Message& profiled_coding =
            GetPotentiallyRepeatedMessage(profiled_concept, profiled_field, i);
        CodingLike* new_coding = AddRawCoding<CodingLike>(target_concept);

        // Copy over as many fields as we can.
        new_coding->ParseFromString(profiled_coding.SerializeAsString());
        new_coding->DiscardUnknownFields();

        const FieldDescriptor* profiled_code_field =
            profiled_coding.GetDescriptor()->FindFieldByName("code");
        if (profiled_code_field &&
            !IsMessageType<CodeLike>(profiled_code_field->message_type())) {
          const Message& profiled_code =
              profiled_coding.GetReflection()->GetMessage(profiled_coding,
                                                          profiled_code_field);
          TF_RETURN_IF_ERROR(
              ConvertToGenericCode(profiled_code, new_coding->mutable_code()));
        }

        // If we see an inlined system or code on the field, copy that over.
        const string& inlined_system = GetInlinedCodingSystem(profiled_field);
        if (!inlined_system.empty()) {
          new_coding->mutable_system()->set_value(inlined_system);
        }
        const string& inlined_code = GetInlinedCodingCode(profiled_field);
        if (!inlined_code.empty()) {
          new_coding->mutable_code()->set_value(inlined_code);
        }

        // If there is a fixed system on the profiled coding message,
        // copy that over too.
        const string& fixed_system_from_message =
            GetFixedCodingSystem(profiled_coding.GetDescriptor());
        if (!fixed_system_from_message.empty()) {
          new_coding->mutable_system()->set_value(fixed_system_from_message);
        }
      }
      if (field_size == 0 && !GetInlinedCodingCode(profiled_field).empty()) {
        // This is a Fixed-Code field with no additional information provided.
        // Just add the Code and System.
        CodingLike* new_coding = AddRawCoding<CodingLike>(target_concept);
        new_coding->mutable_system()->set_value(
            GetInlinedCodingSystem(profiled_field));
        new_coding->mutable_code()->set_value(
            GetInlinedCodingCode(profiled_field));
      }
    }
  }
  return Status::OK();
}

template <typename ExtensionLike, typename CodeableConceptLike = FHIR_DATATYPE(
                                      ExtensionLike, codeable_concept)>
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
    const FieldDescriptor* profiled_field =
        profiled_descriptor->FindFieldByNumber(base_field->number());
    if (!profiled_field || !FieldHasValue(profiled_message, profiled_field)) {
      continue;
    }
    if (IsProfileOf<CodeableConceptLike>(profiled_field->message_type())) {
      for (int j = 0;
           j < PotentiallyRepeatedFieldSize(profiled_message, profiled_field);
           j++) {
        FHIR_RETURN_IF_ERROR(UnsliceCodingsWithinCodeableConcept<ExtensionLike>(
            GetPotentiallyRepeatedMessage(profiled_message, profiled_field, j),
            MutablePotentiallyRepeatedMessage(base_message, base_field, j)));
      }
    }
  }
  return Status::OK();
}

template <typename ExtensionLike>
Status PerformUnslicing(const google::protobuf::Message& profiled_message,
                        google::protobuf::Message* base_message) {
  FHIR_RETURN_IF_ERROR(
      PerformExtensionUnslicing<ExtensionLike>(profiled_message, base_message));
  FHIR_RETURN_IF_ERROR(PerformCodeableConceptUnslicing<ExtensionLike>(
      profiled_message, base_message));

  const Descriptor* profiled_descriptor = profiled_message.GetDescriptor();
  const Descriptor* base_descriptor = base_message->GetDescriptor();
  for (int i = 0; i < profiled_descriptor->field_count(); i++) {
    const FieldDescriptor* profiled_field = profiled_descriptor->field(i);
    const FieldDescriptor* base_field =
        base_descriptor->FindFieldByNumber(profiled_field->number());
    const Descriptor* field_type = profiled_field->message_type();
    if (!field_type) {
      return InvalidArgument("Encountered unexpected primitive type on field: ",
                             profiled_field->full_name());
    }
    if (CanHaveSlicing<ExtensionLike>(profiled_field)) {
      for (int j = 0;
           j < PotentiallyRepeatedFieldSize(profiled_message, profiled_field);
           j++) {
        FHIR_RETURN_IF_ERROR(PerformUnslicing<ExtensionLike>(
            GetPotentiallyRepeatedMessage(profiled_message, profiled_field, j),
            MutablePotentiallyRepeatedMessage(base_message, base_field, j)));
      }
    }
  }
  return Status::OK();
}

// Converts from something less specialized to something more specialized
template <typename ExtensionLike>
Status DownConvert(const Message& base_message, Message* profiled_message) {
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
    return InvalidArgument("Unable to convert ", base_descriptor->full_name(),
                           " to ", profiled_descriptor->full_name(),
                           ".  They are not binary compatible.");
  }
  Status slicing_status = PerformSlicing<ExtensionLike>(profiled_message);
  if (!slicing_status.ok()) {
    return InvalidArgument("Unable to slice ", base_descriptor->full_name(),
                           " to ", profiled_descriptor->full_name(), ": ",
                           slicing_status.error_message());
  }
  return Status::OK();
}

// Converts from something more specialized to something less specialized
template <typename ExtensionLike>
Status UpConvert(const google::protobuf::Message& profiled_message,
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
  return PerformUnslicing<ExtensionLike>(profiled_message, base_message);
}

template <typename ExtensionLike>
Status ConvertToProfileLenientInternal(const Message& source, Message* target) {
  const Descriptor* source_descriptor = source.GetDescriptor();
  const Descriptor* target_descriptor = target->GetDescriptor();
  if (source_descriptor->full_name() == target_descriptor->full_name() ||
      IsProfileOf(target_descriptor, source_descriptor)) {
    // If Target is a profile of Source, we want to go from less specialized
    // to more specialized.
    // If they are the same type, Down convert to make sure all profiled fields
    // get normalized.
    return DownConvert<ExtensionLike>(source, target);
  }
  if (IsProfileOf(source_descriptor, target_descriptor)) {
    // Source is a profile of Target, we want to go from more specialized
    // to less specialized.
    return UpConvert<ExtensionLike>(source, target);
  }
  // TODO: Side convert if possible, through a common ancestor.
  return InvalidArgument("Unable to convert ", source_descriptor->full_name(),
                         " to ", target_descriptor->full_name());
}

template <typename ExtensionLike>
Status ConvertToProfileInternal(const Message& source, Message* target) {
  const auto& status =
      ConvertToProfileLenientInternal<ExtensionLike>(source, target);
  FHIR_RETURN_IF_ERROR(status);
  Status validation = ValidateResource(*target);
  if (validation.ok()) {
    return Status::OK();
  }
  return tensorflow::errors::FailedPrecondition(validation.error_message());
}

}  // namespace profiles_internal

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_PROFILES_LIB_H_
