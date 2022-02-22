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

#ifndef GOOGLE_FHIR_ERROR_REPORTER_H_
#define GOOGLE_FHIR_ERROR_REPORTER_H_

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/strings/str_cat.h"

namespace google {
namespace fhir {

// An ErrorReporter to make all conversion or validation errors visible to
// callers so they can report or handle them as appropriate for the surrounding
// system.
class ErrorReporter {
 public:
  virtual ~ErrorReporter() {}

  /**
   * Reports an error encountered during a process indicating that it could not
   * be completed.  These should almost never be ignored, and indicate that
   * any output objects should not be used other than for debugging.
   *
   * Parameters:
   *  * field_path: the path to the field that where the issue occurred
   *  * element_path: the path to the exact element that failed, including
   * indices
   *  * status: a status message with details on the issue.
   */
  virtual absl::Status ReportError(absl::string_view field_path,
                                   absl::string_view element_path,
                                   const absl::Status& status) = 0;

  /**
   * Variant for reporting errors when the field path is known the but the
   * exact element path is not.
   * By default, this calls the above API with an empty element_path.
   */
  virtual absl::Status ReportError(absl::string_view field_path,
                                   const absl::Status& status);

  /**
   * Reports a failure encountered during a process.  For instance, if
   * a validation process encounters a resource missing a required field, that
   * would be reported using this API.  This should not be used for unexpected
   * error conditions, which should be reported using the ReportError API.
   *
   * Parameters:
   *  * field_path: the path to the field that where the issue occurred.
   *  * element: the path to the exact element that failed, including indices
   *  * message: a message with details on the issue.
   */
  virtual absl::Status ReportFailure(absl::string_view field_path,
                                     absl::string_view element_path,
                                     absl::string_view message) = 0;

  /**
   * Variant for reporting failures when the field path is known the but the
   * exact element path is not.
   * By default, this calls the above API with an empty element_path.
   */
  virtual absl::Status ReportFailure(absl::string_view field_path,
                                     absl::string_view message);

  /**
   * Reports a warning encountered during a process.  These are used to flag
   * issues related to resources that should be raised to the user, but are not
   * sufficient to consider the process to have failed.
   *
   * Parameters:
   *  * field_path: the path to the field that where the issue occurred.
   *  * element_path: the path to the exact element that failed, including
   * indices
   *  * message: a message with details on the issue.
   */
  virtual absl::Status ReportWarning(absl::string_view field_path,
                                     absl::string_view element_path,
                                     absl::string_view message) = 0;
  /**
   * Variant for reporting warnings when the field path is known the but the
   * exact element path is not.
   * By default, this calls the above API with an empty element_path.
   */
  virtual absl::Status ReportWarning(absl::string_view field_path,
                                     absl::string_view message);

  /**
   * Reports an error encountered while processing a FHIRPath expression,
   * such as a malformed express or an unsupported function.
   *
   * Parameters:
   *  * field_path: the path to the field that where the issue occurred.
   *  * element: the path to the exact element that failed, including indices
   *               on repeated fields.
   *  * fhir_path_constraint: the violated FHIRPath constraint expression.
   *  * status: status error to be reported.
   */
  virtual absl::Status ReportFhirPathError(
      absl::string_view field_path, absl::string_view element_path,
      absl::string_view fhir_path_constraint, const absl::Status& status);

  /**
   * Reports a failure due to a successfully evaluated FHIRPath constraint.
   * For instance, if a resource fails validation due to an error-level
   * FHIRPath constraint, it should be reported using this API.
   *
   * Parameters:
   *  * field_path: the path to the field that where the issue occurred.
   *  * element: the path to the exact element that failed, including indices
   *               on repeated fields.
   *  * fhir_path_constraint: the violated FHIRPath constraint expression
   */
  virtual absl::Status ReportFhirPathFailure(
      absl::string_view field_path, absl::string_view element_path,
      absl::string_view fhir_path_constraint);

  /**
   * Reports a failure due to a successfully evaluated FHIRPath constraint.
   * For instance, if a resource fails validation due to an error-level
   * FHIRPath constraint, it should be reported using this API.
   *
   * Parameters:
   *  * field_path: the path to the field that where the issue occurred.
   *  * element: the path to the exact element that failed, including indices
   *               on repeated fields.
   *  * fhir_path_constraint: the violated FHIRPath constraint expression
   */
  virtual absl::Status ReportFhirPathWarning(
      absl::string_view field_path, absl::string_view element_path,
      absl::string_view fhir_path_constraint);
};

// A thread-safe error reporter implementation that simply returns a failure
// status on the first error it encounters. This is primarily for legacy use;
// most users should use an OperationOutcomeErrorReporter or their
// own implementation.
class FailFastErrorReporter : public ErrorReporter {
 public:
  // Returns a singleton instance of a "fast-fail on failure or error" reporter
  // for convenience.
  static FailFastErrorReporter* FailOnErrorOrFailure();

  // Returns a singleton instance of a "fast-fail on error only" reporter for
  // convenience.
  static FailFastErrorReporter* FailOnErrorOnly();

  absl::Status ReportError(absl::string_view field_path,
                           absl::string_view element_path,
                           const absl::Status& status) override {
    return status;
  }

  absl::Status ReportError(absl::string_view field_path,
                           const absl::Status& status) override {
    return status;
  }

  absl::Status ReportFailure(absl::string_view field_path,
                             absl::string_view element_path,
                             absl::string_view message) override {
    if (behavior_ == FAIL_ON_ERROR_OR_FAILURE) {
      return absl::FailedPreconditionError(message);
    }
    return absl::OkStatus();
  }

  absl::Status ReportFailure(absl::string_view field_path,
                             absl::string_view message) override {
    if (behavior_ == FAIL_ON_ERROR_OR_FAILURE) {
      return absl::FailedPreconditionError(message);
    }
    return absl::OkStatus();
  }

  absl::Status ReportWarning(absl::string_view field_path,
                             absl::string_view element_path,
                             absl::string_view message) override {
    // The system should not fail on warnings.
    return absl::OkStatus();
  }

  absl::Status ReportWarning(absl::string_view field_path,
                             absl::string_view message) override {
    // The system should not fail on warnings.
    return absl::OkStatus();
  }

 private:
  enum Behavior { FAIL_ON_ERROR_OR_FAILURE, FAIL_ON_ERROR_ONLY };

  explicit FailFastErrorReporter(Behavior behavior) : behavior_(behavior) {}

  const Behavior behavior_;
};

}  // namespace fhir
}  // namespace google


#endif  // GOOGLE_FHIR_ERROR_REPORTER_H_
