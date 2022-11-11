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

#ifndef GOOGLE_FHIR_RESOURCE_VALIDATION_H_
#define GOOGLE_FHIR_RESOURCE_VALIDATION_H_

#include "google/protobuf/message.h"
#include "absl/status/status.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/fhir_path/fhir_path_validation.h"
#include "google/fhir/primitive_handler.h"

// TODO(b/181018290): Refactor this into an internal namespace and directory.
namespace google {
namespace fhir {

// Run resource-specific validation on a single FHIR resource and
// report all errors to the given error reporter.
::absl::Status Validate(const ::google::protobuf::Message& resource,
                        const PrimitiveHandler* primitive_handler,
                        fhir_path::FhirPathValidator* message_validator,
                        ErrorHandler& error_handler);

// Run resource-specific validation on a single FHIR resource and
// report all errors to the error reporter, excluding FHIRPath constraints.
//
// This exists only to support backward compatibility in calling methods.
::absl::Status ValidateWithoutFhirPath(
    const ::google::protobuf::Message& resource,
    const PrimitiveHandler* primitive_handler, ErrorHandler& error_handler);

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_RESOURCE_VALIDATION_H_
