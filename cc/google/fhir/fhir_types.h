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

#ifndef GOOGLE_FHIR_FHIR_TYPES_H_
#define GOOGLE_FHIR_FHIR_TYPES_H_

#include "google/protobuf/message.h"

namespace google {
namespace fhir {

// Provides version-independent tests for resource type without pulling in
// resource dependencies.

bool IsBundle(const ::google::protobuf::Message& message);
bool IsProfileOfBundle(const ::google::protobuf::Message& message);
bool IsTypeOrProfileOfBundle(const ::google::protobuf::Message& message);
bool IsBundle(const ::google::protobuf::Descriptor* descriptor);
bool IsProfileOfBundle(const ::google::protobuf::Descriptor* descriptor);
bool IsTypeOrProfileOfBundle(const ::google::protobuf::Descriptor* descriptor);

bool IsCodeableConcept(const ::google::protobuf::Message& message);
bool IsProfileOfCodeableConcept(const ::google::protobuf::Message& message);
bool IsTypeOrProfileOfCodeableConcept(const ::google::protobuf::Message& message);
bool IsCodeableConcept(const ::google::protobuf::Descriptor* descriptor);
bool IsProfileOfCodeableConcept(const ::google::protobuf::Descriptor* descriptor);
bool IsTypeOrProfileOfCodeableConcept(const ::google::protobuf::Descriptor* descriptor);

bool IsCoding(const ::google::protobuf::Message& message);
bool IsProfileOfCoding(const ::google::protobuf::Message& message);
bool IsTypeOrProfileOfCoding(const ::google::protobuf::Message& message);
bool IsCoding(const ::google::protobuf::Descriptor* descriptor);
bool IsProfileOfCoding(const ::google::protobuf::Descriptor* descriptor);
bool IsTypeOrProfileOfCoding(const ::google::protobuf::Descriptor* descriptor);

bool IsCode(const ::google::protobuf::Message& message);
bool IsProfileOfCode(const ::google::protobuf::Message& message);
bool IsTypeOrProfileOfCode(const ::google::protobuf::Message& message);
bool IsCode(const ::google::protobuf::Descriptor* descriptor);
bool IsProfileOfCode(const ::google::protobuf::Descriptor* descriptor);
bool IsTypeOrProfileOfCode(const ::google::protobuf::Descriptor* descriptor);

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_FHIR_TYPES_H_
