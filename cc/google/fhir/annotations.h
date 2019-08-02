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

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/message.h"
#include "proto/annotations.pb.h"

namespace google {
namespace fhir {

using std::string;
const string& GetStructureDefinitionUrl(const ::google::protobuf::Descriptor* descriptor);

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

// Returns true if the passed-in descriptor is either of the template FHIR type,
// or a profile of that type.
template <typename B>
const bool IsTypeOrProfileOf(const ::google::protobuf::Descriptor* descriptor) {
  const string& actual_type = GetStructureDefinitionUrl(descriptor);
  return (!actual_type.empty() &&
          actual_type == GetStructureDefinitionUrl(B::descriptor())) ||
         IsProfileOf<B>(descriptor);
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

const bool IsResource(const ::google::protobuf::Descriptor* descriptor);

const bool IsReference(const ::google::protobuf::Descriptor* descriptor);

const bool HasValueset(const ::google::protobuf::Descriptor* descriptor);

const string& GetInlinedCodingSystem(const ::google::protobuf::FieldDescriptor* field);

const string& GetInlinedCodingCode(const ::google::protobuf::FieldDescriptor* field);

const string& GetFixedCodingSystem(const ::google::protobuf::Descriptor* descriptor);

const string& GetValueRegex(const ::google::protobuf::Descriptor* descriptor);

const bool HasInlinedExtensionUrl(const ::google::protobuf::FieldDescriptor* field);

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_ANNOTATIONS_H_
