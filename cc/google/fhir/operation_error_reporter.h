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

#include <string_view>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "google/fhir/error_reporter.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/operation_outcome.pb.h"
#include "proto/google/fhir/proto/r4/fhirproto.pb.h"

namespace google::fhir {

// Error handler that aggregates errors into an OperationOutcome (or profile of
// OperationOutcome).
//
// As described in the ErrorHandler interface, FhirError issues indicate
// validation failures but not process failures, and so use the IssueTypeCode of
// `value` as described by https://www.hl7.org/fhir/valueset-issue-type.html,
// while FhirFatal issues indicate that data should not be trusted, and so use
// the `Structure` IssueType Code.
//
// This is templatized by the OperationOutcome-like type, and
// the types to use for the IssueSeverityCode and IssueTypeCode enums. These are
// not inferred from the OperationOutcome-like type itself, because the
// generated proto type uses internal enum types that do not contain the set of
// enum values needed by this class (at various points they are type asserted
// back to the types we would want).
template <typename OperationOutcomeType, typename IssueSeverityCode,
          typename IssueTypeCode>
class OutcomeErrorHandler : public ErrorHandler {
 public:
  explicit OutcomeErrorHandler(OperationOutcomeType* outcome)
      : outcome_(outcome) {}

  static std::vector<typename OperationOutcomeType::Issue> GetWarnings(
      const OperationOutcomeType& outcome) {
    return GetIssuesWithSeverity(outcome, IssueSeverityCode::WARNING);
  }
  std::vector<typename OperationOutcomeType::Issue> GetWarnings() const {
    return GetWarnings(*outcome_);
  }

  static std::vector<typename OperationOutcomeType::Issue> GetErrors(
      const OperationOutcomeType& outcome) {
    return GetIssuesWithSeverity(outcome, IssueSeverityCode::ERROR);
  }
  std::vector<typename OperationOutcomeType::Issue> GetErrors() const {
    return GetErrors(*outcome_);
  }

  static std::vector<typename OperationOutcomeType::Issue> GetFatals(
      const OperationOutcomeType& outcome) {
    return GetIssuesWithSeverity(outcome, IssueSeverityCode::FATAL);
  }
  std::vector<typename OperationOutcomeType::Issue> GetFatals() const {
    return GetFatals(*outcome_);
  }

  static std::vector<typename OperationOutcomeType::Issue> GetErrorsAndFatals(
      const OperationOutcomeType& outcome) {
    std::vector<typename OperationOutcomeType::Issue> all = GetErrors(outcome);
    std::vector<typename OperationOutcomeType::Issue> fatals =
        GetFatals(outcome);
    all.insert(all.end(), fatals.begin(), fatals.end());
    return all;
  }
  std::vector<typename OperationOutcomeType::Issue> GetErrorsAndFatals() const {
    return GetErrorsAndFatals(*outcome_);
  }

  bool HasWarnings() const override { return !GetWarnings().empty(); }

  bool HasErrors() const override { return !GetErrors().empty(); }

  bool HasFatals() const override { return !GetFatals().empty(); }

  absl::Status HandleFhirFatal(const absl::Status& status,
                               absl::string_view element_path,
                               absl::string_view field_path) override {
    Handle(status.message(), IssueTypeCode::STRUCTURE, IssueSeverityCode::FATAL,
           element_path);
    return absl::OkStatus();
  };

  absl::Status HandleFhirError(absl::string_view msg,
                               absl::string_view element_path,
                               absl::string_view field_path) override {
    Handle(msg, IssueTypeCode::VALUE, IssueSeverityCode::ERROR, element_path);
    return absl::OkStatus();
  };

  absl::Status HandleFhirWarning(absl::string_view msg,
                                 absl::string_view element_path,
                                 absl::string_view field_path) override {
    Handle(msg, IssueTypeCode::VALUE, IssueSeverityCode::WARNING, element_path);
    return absl::OkStatus();
  };

  absl::Status HandleFhirPathFatal(const absl::Status& status,
                                   absl::string_view expression,
                                   absl::string_view element_path,
                                   absl::string_view field_path) override {
    Handle(absl::StrCat(expression, ":", status.message()),
           IssueTypeCode::STRUCTURE, IssueSeverityCode::FATAL, element_path);
    return absl::OkStatus();
  };

  absl::Status HandleFhirPathError(absl::string_view expression,
                                   absl::string_view element_path,
                                   absl::string_view field_path) override {
    Handle(expression, IssueTypeCode::VALUE, IssueSeverityCode::ERROR,
           element_path);
    return absl::OkStatus();
  };

  absl::Status HandleFhirPathWarning(absl::string_view expression,
                                     absl::string_view element_path,
                                     absl::string_view field_path) override {
    Handle(expression, IssueTypeCode::VALUE, IssueSeverityCode::WARNING,
           element_path);
    return absl::OkStatus();
  };

 private:
  void Handle(absl::string_view message, typename IssueTypeCode::Value type,
              typename IssueSeverityCode::Value severity,
              absl::string_view element_path) {
    auto issue = outcome_->add_issue();
    issue->mutable_code()->set_value(type);
    issue->mutable_severity()->set_value(severity);

    issue->mutable_diagnostics()
         ->set_value(std::string(message));
    issue->add_expression()->set_value(std::string(element_path));
  }

  static std::vector<typename OperationOutcomeType::Issue>
  GetIssuesWithSeverity(const OperationOutcomeType& operation_outcome,
                        const typename IssueSeverityCode::Value severity) {
    std::vector<typename OperationOutcomeType::Issue> issues;
    for (const auto& issue : operation_outcome.issue()) {
      if (issue.severity().value() == severity) {
        issues.push_back(issue);
      }
    }
    return issues;
  }

  OperationOutcomeType* outcome_;
};

}  // namespace google::fhir

#endif  // GOOGLE_FHIR_OPERATION_ERROR_REPORTER_H_
