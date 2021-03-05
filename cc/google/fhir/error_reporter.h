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
   * Reports the given error during FHIR validation or conversion, indicating
   * the resource does not fully comply with the FHIR specification or profile.
   *
   * If the error can be satisfactorily reported implementations should return
   * an OK status, instructing the FHIR validation logic to proceed.
   * Conversely, if this returns an error status, the validation flow will
   * immediately return this status to the caller.
   *
   * Parameters:
   *  * element_path: a path to the field that where the issue occurred.
   *  * status: a status message with details on the issue.
   */
  virtual absl::Status ReportError(absl::string_view element_path,
                                   const absl::Status& status) = 0;

  /**
   * Reports the given warning during FHIR validation or conversion, indicating
   * the complies with the FHIR specification but may be missing some desired
   * -but-not-required property, like additional fields that are useful to
   * consumers.
   *
   * If the error can be satisfactorily reported implementations should return
   * an OK status, instructing the FHIR validation logic to proceed.
   * Conversely, if this returns an error status, the validation flow will
   * immediately return this status to the caller.
   *
   * Parameters:
   *  * element_path: a path to the field that where the issue occurred.
   *  * status: a status message with details on the issue.
   */
  virtual absl::Status ReportWarning(absl::string_view element_path,
                                     const absl::Status& status) = 0;

  /**
   * Reports a FHIRPath constraint error, as defined by a constraint
   * on the resource profile. By default this simply calls ReportError,
   * but implementations may override in case they need special handling.
   *
   * Parameters:
   *  * element_path: a path to the field that where the issue occurred.
   *  * fhir_path_constraint: the violated FHIRPath constraint expression
   *  * message: optional message describing the error. May be empty.
   */
  virtual absl::Status ReportFhirPathError(absl::string_view element_path,
                                           absl::string_view
                                           fhir_path_constraint,
                                           absl::string_view message);

  /**
   * Reports a FHIRPath constraint warning, as defined by a constraint
   * on the resource profile.  By default this simply calls ReportWarning,
   * but implementations may override in case they need special handling.
   *
   * Parameters:
   *  * element_path: a path to the field that where the issue occurred.
   *  * fhir_path_constraint: the violated FHIRPath constraint expression
   *  * message: optional message describing the warning. May be empty.
   */
  virtual absl::Status ReportFhirPathWarning(absl::string_view element_path,
                                             absl::string_view
                                             fhir_path_constraint,
                                             absl::string_view message);
};

// A thread-safe error reporter implementation that simply returns a failure
// status on the first error it encounters. This is primarily for legacy use;
// most users should use an OperationOutcomeErrorReporter or their
// own implementation.
class FailFastErrorReporter : public ErrorReporter {
 public:
  // Returns a singleton instance of this error reporter for convenience.
  static FailFastErrorReporter* Get();

  absl::Status ReportError(absl::string_view fhir_path,
                           const absl::Status& status) override {
    return status;
  }

  absl::Status ReportWarning(absl::string_view fhir_path,
                           const absl::Status& status) override {
    // The system should not fail on warnings.
    return absl::OkStatus();
  }
};

}  // namespace fhir
}  // namespace google


#endif  // GOOGLE_FHIR_ERROR_REPORTER_H_
