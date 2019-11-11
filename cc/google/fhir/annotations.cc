// Copyright 2019 Google LLC
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

#include "google/fhir/annotations.h"

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/message.h"
#include "proto/annotations.pb.h"

namespace google {
namespace fhir {

const std::string& GetStructureDefinitionUrl(
    const ::google::protobuf::Descriptor* descriptor) {
  return descriptor->options().GetExtension(
      proto::fhir_structure_definition_url);
}

const bool IsProfileOf(const ::google::protobuf::Descriptor* descriptor,
                       const ::google::protobuf::Descriptor* potential_base) {
  const std::string& base_url = GetStructureDefinitionUrl(potential_base);
  for (int i = 0;
       i < descriptor->options().ExtensionSize(proto::fhir_profile_base); i++) {
    if (descriptor->options().GetExtension(proto::fhir_profile_base, i) ==
        base_url) {
      return true;
    }
  }
  return false;
}

const bool IsProfile(const ::google::protobuf::Descriptor* descriptor) {
  return descriptor->options().ExtensionSize(proto::fhir_profile_base) > 0;
}

const bool IsChoiceType(const ::google::protobuf::FieldDescriptor* field) {
  return field->type() == google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE &&
         field->message_type()->options().GetExtension(proto::is_choice_type);
}

const bool IsPrimitive(const ::google::protobuf::Descriptor* descriptor) {
  return descriptor->options().GetExtension(proto::structure_definition_kind) ==
         proto::StructureDefinitionKindValue::KIND_PRIMITIVE_TYPE;
}

const bool IsComplex(const ::google::protobuf::Descriptor* descriptor) {
  return descriptor->options().GetExtension(proto::structure_definition_kind) ==
         proto::StructureDefinitionKindValue::KIND_COMPLEX_TYPE;
}

const bool IsResource(const ::google::protobuf::Descriptor* descriptor) {
  return descriptor->options().GetExtension(proto::structure_definition_kind) ==
         proto::StructureDefinitionKindValue::KIND_RESOURCE;
}

const bool IsReference(const ::google::protobuf::Descriptor* descriptor) {
  return descriptor->options().ExtensionSize(proto::fhir_reference_type) > 0;
}

const std::string& GetValueset(const ::google::protobuf::Descriptor* descriptor) {
  return descriptor->options().GetExtension(proto::fhir_valueset_url);
}

const bool HasValueset(const ::google::protobuf::Descriptor* descriptor) {
  return !GetValueset(descriptor).empty();
}

const std::string& GetFixedSystem(const ::google::protobuf::Descriptor* descriptor) {
  return descriptor->options().GetExtension(proto::fhir_fixed_system);
}

const bool HasFixedSystem(const ::google::protobuf::Descriptor* descriptor) {
  return !GetFixedSystem(descriptor).empty();
}

const std::string& GetInlinedCodingSystem(
    const ::google::protobuf::FieldDescriptor* field) {
  return field->options().GetExtension(proto::fhir_inlined_coding_system);
}

const std::string& GetInlinedCodingCode(
    const ::google::protobuf::FieldDescriptor* field) {
  return field->options().GetExtension(proto::fhir_inlined_coding_code);
}

const std::string& GetFixedCodingSystem(
    const ::google::protobuf::Descriptor* descriptor) {
  return descriptor->options().GetExtension(proto::fhir_fixed_system);
}

const bool HasFixedCodingSystem(const ::google::protobuf::Descriptor* descriptor) {
  return !GetFixedCodingSystem(descriptor).empty();
}

const std::string& GetSourceCodeSystem(
    const ::google::protobuf::EnumValueDescriptor* descriptor) {
  return descriptor->options().GetExtension(proto::source_code_system);
}

const bool HasSourceCodeSystem(
    const ::google::protobuf::EnumValueDescriptor* descriptor) {
  return !GetSourceCodeSystem(descriptor).empty();
}

const std::string& GetValueRegex(const ::google::protobuf::Descriptor* descriptor) {
  return descriptor->options().GetExtension(proto::value_regex);
}

const bool HasInlinedExtensionUrl(const ::google::protobuf::FieldDescriptor* field) {
  return field->options().HasExtension(proto::fhir_inlined_extension_url);
}

const proto::FhirVersion GetFhirVersion(
    const ::google::protobuf::Descriptor* descriptor) {
  return descriptor->file()->options().GetExtension(proto::fhir_version);
}

const proto::FhirVersion GetFhirVersion(const ::google::protobuf::Message& message) {
  return GetFhirVersion(message.GetDescriptor());
}

}  // namespace fhir
}  // namespace google
