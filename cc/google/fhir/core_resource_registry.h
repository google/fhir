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

#ifndef GOOGLE_FHIR_CORE_RESOURCE_REGISTRY_H_
#define GOOGLE_FHIR_CORE_RESOURCE_REGISTRY_H_

#include <memory>
#include <string>

#include "google/protobuf/message.h"
#include "google/fhir/status/statusor.h"

namespace google {
namespace fhir {

// Given a profiled resource, returns a new instance of the core (base)
// resource.
StatusOr<std::unique_ptr<::google::protobuf::Message>> GetBaseResourceInstance(
    const ::google::protobuf::Message& message);

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_CORE_RESOURCE_REGISTRY_H_
