/*
 * Copyright 2018 Google LLC
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

#ifndef GOOGLE_FHIR_STU3_RESOURCE_VALIDATION_H_
#define GOOGLE_FHIR_STU3_RESOURCE_VALIDATION_H_

#include "google/fhir/fhir_path/fhir_path.h"
#include "google/fhir/status/status.h"

namespace google {
namespace fhir {

// TODO: Invert the default here for FHIRPath handling, and have
// ValidateWithoutFhirPath instead of ValidateWithFhirPath

// Run resource-specific validation on a single FHIR resource.
Status ValidateResource(const ::google::protobuf::Message& resource);

// Run resource-specific validation + FHIRPath validation on a single FHIR
// resource.
Status ValidateResourceWithFhirPath(const ::google::protobuf::Message& resource);

// TODO(nickgeorge, rbrush): Consider integrating handler func into validations
// in this file.
Status ValidateResourceWithFhirPath(const ::google::protobuf::Message& resource,
                                    fhir_path::ViolationHandlerFunc handler);

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_RESOURCE_VALIDATION_H_
