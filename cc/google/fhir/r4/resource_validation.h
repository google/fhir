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

#ifndef GOOGLE_FHIR_R4_RESOURCE_VALIDATION_H_
#define GOOGLE_FHIR_R4_RESOURCE_VALIDATION_H_

#include "absl/status/status.h"
#include "google/fhir/error_reporter.h"
#include "proto/google/fhir/proto/r4/core/resources/operation_outcome.pb.h"
#include "proto/google/fhir/proto/r4/fhirproto.pb.h"
#include "google/protobuf/message.h"

namespace google {
namespace fhir {
namespace r4 {

// Run resource-specific validation on the given FHIR resource and
// report all errors to the given error reporter. Validation will continue
// processing as long as the ErrorReporter returns an Ok status for all
// errors it is given.
//
// If `validate_reference_field_ids` is set to `true`, Reference ids inside FHIR
// resources will be validated and resources with invalid Reference field ids
// will be flagged as invalid.
// TODO(b/243721150): Deprecate this config option after the validate reference
// ids feature has been integrated into the data-pipeline.
//
// Returns Ok if the error reporter handled all reported errors and
// there was no internal issue (such as a malformed FHIR profile).
::absl::Status Validate(const ::google::protobuf::Message& resource,
                        ::google::fhir::ErrorHandler& handler,
                        bool validate_reference_field_ids = false);

// Deprecated. Use the above Validate functions.
::absl::Status ValidateWithoutFhirPath(const ::google::protobuf::Message& resource);

}  // namespace r4
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_R4_RESOURCE_VALIDATION_H_
