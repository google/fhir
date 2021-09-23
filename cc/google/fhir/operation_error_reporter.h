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

#ifndef GOOGLE_FHIR_OPERATION_ERROR_REPORTER_H_
#define GOOGLE_FHIR_OPERATION_ERROR_REPORTER_H_

#include "absl/status/status.h"
#include "google/fhir/error_reporter.h"
#include "proto/google/fhir/proto/r4/core/resources/operation_outcome.pb.h"
#include "proto/google/fhir/proto/r4/core/codes.pb.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/fhirproto.pb.h"

namespace google::fhir {

// Generic Error Reporter for OperationOutcome-like types (e.g. like
// OperationOutcome and ValidationOutcome). This is templatized by the
// OperationOutcome-like type, and the types to use for the IssueSeverityCode
// and IssueTypeCode enums. These are not inferred from the
// OperationOutcome-like type itself, because the generated proto type uses
// internal enum types that do not contain the set of enum values needed by this
// class (at various points they are type asserted back to the types we would
// want).
//
// Conversion issues that can result in data loss are reported as a "structure"
// error type as described at https://www.hl7.org/fhir/valueset-issue-type.html,
// since the item could not be converted into the target structure. Validation
// issues that preserve data use a "value" error type from that value set.
template <typename OperationOutcomeType, typename IssueSeverityCode,
          typename IssueTypeCode>
class OutcomeErrorReporter : public ErrorReporter {
 public:
  explicit OutcomeErrorReporter(OperationOutcomeType* outcome)
      : outcome_(outcome) {}

  absl::Status ReportConversionError(
      absl::string_view element_path,
      const absl::Status& error_status) override {
    return Report(element_path, error_status, IssueTypeCode::STRUCTURE,
                  IssueSeverityCode::ERROR);
  }

  absl::Status ReportValidationError(
      absl::string_view element_path,
      const absl::Status& error_status) override {
    return Report(element_path, error_status, IssueTypeCode::VALUE,
                  IssueSeverityCode::ERROR);
  }

  absl::Status ReportValidationWarning(
      absl::string_view element_path,
      const absl::Status& error_status) override {
    return Report(element_path, error_status, IssueTypeCode::VALUE,
                  IssueSeverityCode::WARNING);
  }

 private:
  absl::Status Report(
      absl::string_view element_path, const absl::Status& error_status,
      typename IssueTypeCode::Value type,
      typename IssueSeverityCode::Value severity) {
    auto issue = outcome_->add_issue();
    issue->mutable_code()->set_value(type);
    issue->mutable_severity()->set_value(severity);

    issue->mutable_diagnostics()
        ->set_value(std::string(error_status.message()));
    issue->add_expression()->set_value(std::string(element_path));

    return absl::OkStatus();
  }

  OperationOutcomeType* outcome_;
};

}  // namespace google::fhir

#endif  // GOOGLE_FHIR_OPERATION_ERROR_REPORTER_H_
