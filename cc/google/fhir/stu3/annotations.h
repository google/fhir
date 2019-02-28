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

#ifndef GOOGLE_FHIR_STU3_ANNOTATIONS_H_
#define GOOGLE_FHIR_STU3_ANNOTATIONS_H_


#include "google/protobuf/message.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;

const string& GetFhirProfileBase(const ::google::protobuf::Descriptor* descriptor);

const string& GetStructureDefinitionUrl(const ::google::protobuf::Descriptor* descriptor);

// Check that returns a FailedPrecondition status if the passed-in message is
// a profile of the template type.
template <typename B>
const bool IsProfileOf(const ::google::protobuf::Message& message) {
  const string& actual_base = GetFhirProfileBase(message.GetDescriptor());
  return !actual_base.empty() &&
         actual_base == GetStructureDefinitionUrl(B::descriptor());
}

const bool IsChoiceType(const ::google::protobuf::FieldDescriptor* field);

const bool IsPrimitive(const ::google::protobuf::Descriptor* descriptor);

const bool IsResource(const ::google::protobuf::Descriptor* descriptor);

const bool IsReference(const ::google::protobuf::Descriptor* descriptor);

const bool HasValueset(const ::google::protobuf::Descriptor* descriptor);

const string& GetInlinedCodingSystem(const ::google::protobuf::FieldDescriptor* field);

const string& GetInlinedCodingCode(const ::google::protobuf::FieldDescriptor* field);

}  // namespace stu3
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_ANNOTATIONS_H_
