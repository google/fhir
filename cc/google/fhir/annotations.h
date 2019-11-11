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

#ifndef GOOGLE_FHIR_ANNOTATIONS_H_
#define GOOGLE_FHIR_ANNOTATIONS_H_

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "proto/annotations.pb.h"

namespace google {
namespace fhir {

const std::string& GetStructureDefinitionUrl(
    const ::google::protobuf::Descriptor* descriptor);

const bool IsProfileOf(const ::google::protobuf::Descriptor* descriptor,
                       const ::google::protobuf::Descriptor* potential_base);

// Returns true if the passed-in descriptor is a profile of the template FHIR
// type
template <typename B>
const bool IsProfileOf(const ::google::protobuf::Descriptor* descriptor) {
  return IsProfileOf(descriptor, B::descriptor());
}

// Returns true if the passed-in message is a profile of the template FHIR type
template <typename B>
const bool IsProfileOf(const ::google::protobuf::Message& message) {
  return IsProfileOf<B>(message.GetDescriptor());
}

template <typename B>
const bool IsFhirType(const ::google::protobuf::Descriptor* descriptor) {
  return GetStructureDefinitionUrl(B::descriptor()) ==
         GetStructureDefinitionUrl(descriptor);
}

template <typename B>
const bool IsFhirType(const ::google::protobuf::Message& message) {
  return IsFhirType<B>(message.GetDescriptor());
}

// Returns true if the passed-in descriptor is either of the template FHIR type,
// or a profile of that type.
template <typename B>
const bool IsTypeOrProfileOf(const ::google::protobuf::Descriptor* descriptor) {
  return !GetStructureDefinitionUrl(B::descriptor()).empty() &&
         (IsFhirType<B>(descriptor) || IsProfileOf<B>(descriptor));
}

// Returns true if the passed-in message is either of the template FHIR type,
// or a profile of that type.
template <typename B>
const bool IsTypeOrProfileOf(const ::google::protobuf::Message& message) {
  return IsTypeOrProfileOf<B>(message.GetDescriptor());
}

const bool IsProfile(const ::google::protobuf::Descriptor* descriptor);

const bool IsChoiceType(const ::google::protobuf::FieldDescriptor* field);

const bool IsPrimitive(const ::google::protobuf::Descriptor* descriptor);

const bool IsComplex(const ::google::protobuf::Descriptor* descriptor);

const bool IsResource(const ::google::protobuf::Descriptor* descriptor);

const bool IsReference(const ::google::protobuf::Descriptor* descriptor);

const std::string& GetValueset(const ::google::protobuf::Descriptor* descriptor);

const bool HasValueset(const ::google::protobuf::Descriptor* descriptor);

const std::string& GetFixedSystem(const ::google::protobuf::Descriptor* descriptor);

const bool HasFixedSystem(const ::google::protobuf::Descriptor* descriptor);

const std::string& GetInlinedCodingSystem(
    const ::google::protobuf::FieldDescriptor* field);

const std::string& GetInlinedCodingCode(const ::google::protobuf::FieldDescriptor* field);

const bool HasFixedCodingSystem(const ::google::protobuf::Descriptor* descriptor);

const std::string& GetFixedCodingSystem(const ::google::protobuf::Descriptor* descriptor);

const std::string& GetSourceCodeSystem(
    const ::google::protobuf::EnumValueDescriptor* descriptor);

const bool HasSourceCodeSystem(const ::google::protobuf::EnumValueDescriptor* descriptor);

const std::string& GetValueRegex(const ::google::protobuf::Descriptor* descriptor);

const bool HasInlinedExtensionUrl(const ::google::protobuf::FieldDescriptor* field);

const proto::FhirVersion GetFhirVersion(const ::google::protobuf::Descriptor* descriptor);

const proto::FhirVersion GetFhirVersion(const ::google::protobuf::Message& message);

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_ANNOTATIONS_H_
