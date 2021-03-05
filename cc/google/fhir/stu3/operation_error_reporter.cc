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

#include "google/fhir/stu3/operation_error_reporter.h"

namespace google {
namespace fhir {
namespace stu3 {

using ::google::fhir::stu3::proto::IssueSeverityCode;
using ::google::fhir::stu3::proto::IssueTypeCode;
using ::google::fhir::stu3::proto::OperationOutcome;

absl::Status OperationOutcomeErrorReporter::ReportError(
    absl::string_view element_path, const absl::Status& error_status) {
  return Report(element_path, error_status, IssueSeverityCode::ERROR);
}

absl::Status OperationOutcomeErrorReporter::ReportWarning(
    absl::string_view element_path, const absl::Status& error_status) {
  return Report(element_path, error_status, IssueSeverityCode::WARNING);
}

absl::Status OperationOutcomeErrorReporter::Report(
    absl::string_view element_path, const absl::Status& error_status,
    IssueSeverityCode::Value severity) {
  OperationOutcome::Issue* issue = outcome_->add_issue();
  issue->mutable_code()->set_value(IssueTypeCode::INVALID);
  issue->mutable_severity()->set_value(severity);

  issue->mutable_diagnostics()
      ->set_value(std::string(error_status.message()));
  issue->add_expression()->set_value(std::string(element_path));

  return absl::OkStatus();
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
