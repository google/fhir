/*
 * Copyright 2021 Google LLC
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

#ifndef GOOGLE_FHIR_R4_OPERATION_ERROR_REPORTER_H_
#define GOOGLE_FHIR_R4_OPERATION_ERROR_REPORTER_H_

#include "google/fhir/operation_error_reporter.h"
#include "proto/google/fhir/proto/r4/core/resources/operation_outcome.pb.h"
#include "proto/google/fhir/proto/r4/core/codes.pb.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"

namespace google::fhir::r4 {

// Error handler that creates FHIR R4 OperationOutcome records.
// Conversion issues that can result in data loss are reported as a "structure"
// error type as described at https://www.hl7.org/fhir/valueset-issue-type.html,
// since the item could not be converted into the target structure. Validation
// issues that preserve data use a "value" error type from that value set.
using OperationOutcomeErrorHandler =
    OutcomeErrorHandler<::google::fhir::r4::core::OperationOutcome,
                        ::google::fhir::r4::core::IssueSeverityCode,
                        ::google::fhir::r4::core::IssueTypeCode>;

}  // namespace google::fhir::r4

#endif  // GOOGLE_FHIR_R4_OPERATION_ERROR_REPORTER_H_
