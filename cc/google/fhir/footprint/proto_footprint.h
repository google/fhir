// Copyright 2020 Google LLC
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

#ifndef GOOGLE_FHIR_FOOTPRINT_PROTO_FOOTPRINT_H_
#define GOOGLE_FHIR_FOOTPRINT_PROTO_FOOTPRINT_H_

#include <string>

#include "google/protobuf/descriptor.h"

namespace google {
namespace fhir {

std::string ComputeResourceFileFootprint(const google::protobuf::FileDescriptor* file);

std::string ComputeDatatypesFootprint(const google::protobuf::FileDescriptor* file);

std::string ComputeResourceFootprint(const google::protobuf::Descriptor* type);

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_FOOTPRINT_PROTO_FOOTPRINT_H_


