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

#include "google/protobuf/message.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/operation_error_reporter.h"
#include "proto/google/fhir/proto/r4/core/codes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/operation_outcome.pb.h"
#include "proto/google/fhir/proto/r4/fhirproto.pb.h"

namespace google {
namespace fhir {
namespace r4 {

// Error handler that creates ValidationOutcome records (profiled
// OperationOutcome that includes a subject resource reference).
// Conversion issues that can result in data loss are reported as a "structure"
// error type as described at https://www.hl7.org/fhir/valueset-issue-type.html,
// since the item could not be converted into the target structure. Validation
// issues that preserve data use a "value" error type from that value set.
using ValidationOutcomeErrorHandler =
    OutcomeErrorHandler<::google::fhir::r4::fhirproto::ValidationOutcome,
                        ::google::fhir::r4::core::IssueSeverityCode,
                        ::google::fhir::r4::core::IssueTypeCode>;

// Run resource-specific validation on the given FHIR resource and
// report all errors to the given error reporter. Validation will continue
// processing as long as the ErrorReporter returns an Ok status for all
// errors it is given.
//
// Returns Ok if the error reporter handled all reported errors and
// there was no internal issue (such as a malformed FHIR profile).
::absl::Status Validate(const ::google::protobuf::Message& resource,
                        ::google::fhir::ErrorHandler& handler);

// Run resource-specific validation on the given FHIR resource and
// adds all errors to the returned OperationOutcome. Validation will continue
// through all issues encountered so the given OperationOutcome will provide
// a complete description of any issues.
//
// Returns a ValidationOutcome (a profiled OperationOutcome) with all data
// issues; this will only return an error status if there is some unexpected
// issue like a malformed FHIR profile. If the provided resources does not have
// an ID the ValidationOutcome.subject will be empty.
::absl::StatusOr<::google::fhir::r4::fhirproto::ValidationOutcome>
Validate(const ::google::protobuf::Message& resource);

// Deprecated. Use one of the above Validate functions.
::absl::Status ValidateWithoutFhirPath(const ::google::protobuf::Message& resource);

}  // namespace r4
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_R4_RESOURCE_VALIDATION_H_
