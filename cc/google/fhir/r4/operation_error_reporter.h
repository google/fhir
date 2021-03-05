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

#include "absl/status/status.h"
#include "google/fhir/error_reporter.h"
#include "proto/google/fhir/proto/r4/core/resources/operation_outcome.pb.h"
#include "proto/google/fhir/proto/r4/core/codes.pb.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"

namespace google {
namespace fhir {
namespace r4 {

// Error reporter that creates FHIR R4 OperationOutcome records.
class OperationOutcomeErrorReporter : public ErrorReporter {
 public:
  explicit OperationOutcomeErrorReporter(
      ::google::fhir::r4::core::OperationOutcome* outcome)
      : outcome_(outcome) {}

  absl::Status ReportError(absl::string_view element_path,
                           const absl::Status& error_status) override;

  absl::Status ReportWarning(absl::string_view element_path,
                             const absl::Status& error_status) override;
 private:
  absl::Status Report(absl::string_view element_path,
                      const absl::Status& error_status,
                      ::google::fhir::r4::core::IssueSeverityCode::Value
                      severity);

  ::google::fhir::r4::core::OperationOutcome* outcome_;
};

}  // namespace r4
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_R4_OPERATION_ERROR_REPORTER_H_
