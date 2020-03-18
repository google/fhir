/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "google/fhir/r4/resource_validation.h"

#include "google/fhir/r4/primitive_handler.h"
#include "google/fhir/resource_validation.h"

namespace google {
namespace fhir {
namespace r4 {

Status ValidateResource(const ::google::protobuf::Message& resource) {
  return ValidateResource(resource, R4PrimitiveHandler::GetInstance());
}

Status ValidateResourceWithFhirPath(const ::google::protobuf::Message& resource) {
  return ValidateResourceWithFhirPath(resource,
                                      R4PrimitiveHandler::GetInstance());
}

}  // namespace r4
}  // namespace fhir
}  // namespace google
