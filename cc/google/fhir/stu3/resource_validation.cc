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

#include "google/fhir/resource_validation.h"

#include "google/fhir/stu3/primitive_handler.h"
#include "google/fhir/stu3/resource_validation.h"

namespace google {
namespace fhir {
namespace stu3 {

Status ValidateResource(const ::google::protobuf::Message& resource) {
  return ValidateResource(resource, Stu3PrimitiveHandler::GetInstance());
}

Status ValidateResourceWithFhirPath(const ::google::protobuf::Message& resource) {
  return ValidateResourceWithFhirPath(resource,
                                      Stu3PrimitiveHandler::GetInstance());
}

Status ValidateResourceWithFhirPath(const ::google::protobuf::Message& resource,
                                    fhir_path::ViolationHandlerFunc handler) {
  return ValidateResourceWithFhirPath(resource, handler,
                                      Stu3PrimitiveHandler::GetInstance());
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
