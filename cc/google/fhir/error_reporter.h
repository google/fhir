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

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"

namespace google::fhir {

// A handler interface for errors encountered during a FHIR process.
//
// Top-level processes accept a reference to an ErrorHandler, and will invoke
// Handle functions with information about the error, along with context
// information about where the error occurred, including both `element_path`,
// and "field_path":
//
// * `element_path` provides the path to the exact element where the
//   violation occurred, including index in repeated fields, E.g.,
//     Foo.bar[2].baz
//   This is useful for reporting exact location of errors.
// * `field_path` provides the field path, without index, E.g.,
//     Foo.bar.baz
//   This is useful for adding special handling logic for all elements of a
//   field.
//
// This uses the severity levels defined by
// http://hl7.org/fhir/valueset-issue-severity.html:
// * A `warning` indicates a data issue with a resource that should be resolved,
//   but does not indicate that the resource should be considered invalid.
// * An `error` indicates a data issue severe enough that the resource should be
//   considered invalid.  Importantly, this severity should *NOT* be used for
//   processes that encounter internal errors (such as status codes) that
//   prevent it from concluding regularly.  These are considered "fatal".
// * A `fatal` indicates either an internal issue (like a function returning a
//   status error) or a data format issue (like an invalid primitive) severe
//   enough that the operation could not be completed successfully.  These
//   should almost never be ignored, and any resulting data should not be used
//   other than for debugging.  For instance, a conversion operation that has a
//   fatal issue could result in data loss.

class ErrorHandler {
 public:
  virtual ~ErrorHandler() {}

  // Handler functions to be implemented by concrete classes.
  // These are divided into ReportFhir{Fatal, Error, Warning} for general
  // errors, and ReportFhirPath{Fatal, Error, Warning} for issues from FHIRPath
  // Expressions.

  // Handles a "Fatal" issue encountered.
  // This indicates a process encountered a code error (e.g., a function call
  // returns a status error), not for issues related to user data.
  //
  // Any resulting data from processes that encounter "Fatal" issues should not
  // be trusted or used for anything other than debugging.
  //
  // Parameters:
  // * status: The error encountered by the reporting process.
  // * element_path: Path to specific element where the error occurred
  //                 (including index into repeated fields).
  // * field_path: Path to the field where the error occurred.  This will be
  //               identical to `element_path`, but without the field index.
  virtual absl::Status HandleFhirFatal(const absl::Status& status,
                                       std::string_view element_path,
                                       std::string_view field_path) = 0;

  // Handles an "Error" issue encountered.
  // This should be used when user data violates a precondition.  For instance,
  // a validation process would report an "Error" if a required field is
  // missing, since the code functioned properly but the data is invalid.
  //
  // Data resulting from processes that encounter Errors can generally be
  // trusted as accurate, since Errors do not indicate problems encountered by
  // code (such as status failures).
  //
  // Parameters:
  // * msg: Error message reported by the reporting process
  // * element_path: Path to specific element where the error occurred
  //                 (including index into repeated fields).
  // * field_path: Path to the field where the error occurred.  This will be
  //               identical to `element_path`, but without the field index.
  virtual absl::Status HandleFhirError(std::string_view msg,
                                       std::string_view element_path,
                                       std::string_view field_path) = 0;

  // Handles a "Warning" issue encountered.
  // This should be used when user data violates a precondition at "Warning"
  // level.  By default, "Warning" level only exists in FHIR for FHIRPath (see
  // HandleFhirPathWarning), but custom implementations can relegate certain
  // classes of errors to "warning" level, for instance for known issues with a
  // data source that the user chooses to ignore.
  //
  // Parameters:
  // * msg: Warning message reported by the reporting process
  // * element_path: Path to specific element where the warning occurred
  //                 (including index into repeated fields).
  // * field_path: Path to the field where the warning occurred.  This will be
  //               identical to `element_path`, but without the field index.
  virtual absl::Status HandleFhirWarning(std::string_view msg,
                                         std::string_view element_path,
                                         std::string_view field_path) = 0;

  // Handles a "Fatal" issue encountered during FHIRPath evaluation.
  // This indicates a process encountered a code error (e.g., a function call
  // returned a status error, or the FHIRPath expression failed to parse), not
  // for issues related to user data.
  //
  // Any resulting data from processes that encounter "Fatal" issues should not
  // be trusted, or used for anything other than debugging.
  //
  // Parameters:
  // * status: The error encountered by the reporting process.
  // * expression: The expression that failed.
  // * element_path: Path to specific element where the error occurred
  //                 (including index into repeated fields).
  // * field_path: Path to the field where the error occurred.  This will be
  //               identical to `element_path`, but without the field index.
  virtual absl::Status HandleFhirPathFatal(const absl::Status& status,
                                           std::string_view expression,
                                           std::string_view element_path,
                                           std::string_view field_path) = 0;

  // Handles an "Error" issue encountered during FHIRPath evaluation.
  // This indicates an Error-level FHIRPath requirement that was not met by a
  // resource, not a code error encountered while trying to evaluate the
  // expression.
  //
  // Parameters:
  // * expression: The unmet expression
  // * element_path: Path to specific element where the error occurred
  //                 (including index into repeated fields).
  // * field_path: Path to the field where the error occurred.  This will be
  //               identical to `element_path`, but without the field index.
  virtual absl::Status HandleFhirPathError(std::string_view expression,
                                           std::string_view element_path,
                                           std::string_view field_path) = 0;

  // Handles a "Warning" issue encountered during FHIRPath evaluation.
  // This indicates a Warning-level FHIRPath requirement that was not met by a
  // resource, not a code error encountered while trying to evaluate the
  // expression.
  //
  // Parameters:
  // * expression: The unmet expression
  // * element_path: Path to specific element where the error occurred
  //                 (including index into repeated fields).
  // * field_path: Path to the field where the error occurred.  This will be
  //               identical to `element_path`, but without the field index.
  virtual absl::Status HandleFhirPathWarning(std::string_view expression,
                                             std::string_view element_path,
                                             std::string_view field_path) = 0;
};

// ErrorReporter object for invoking an ErrorHandler with context scope.
// Top-level process APIs should accept an ErrorHandler reference, and wrap it
// in an ErrorReporter, which they pass around.  ErrorScope objects allow
// entering and exiting scopes on the ErrorReporter while traversing a FHIR
// resource.  The process can then call Report functions on the ErrorReporter,
// which will forward to the ErrorHandler along with the element and field paths
// derived from the scope.
//
// Scopes are entered by creating an ErrorScope object, and scopes are exited
// automatically when the ErrorScope object is destroyed.
// Ex.
//
// void HandleResource(const Message& resource, ErrorReporter* reporter) {
//   ErrorScope resource_scope(reporter, resource.GetDescriptor()->name());
//
//   reporter->ReportFhirError("err-msg");
//   // element_path -> ResourceName
//   // field_path -> ResourceName
//
//   const FieldDescriptor* repeated_field = /* ... */;
//   for (int i = 0;
//        i < resource.GetReflection()->FieldSize(resource, repeated_field);
//        ++i) {
//     ErrorScope element_scope(reporter, repeated_field->jsonName(), i);
//
//     reporter->ReportFhirError("err-msg");
//     // element_path -> ResourceName.fieldName[i]
//     // field_path -> ResourceName.fieldName
//   }
//
// The Reporting API also has signatures that take field name (and optionally
// index) for convenience - these enter before reporting and exit after.
class ErrorReporter final {
  friend class ErrorScope;

 public:
  explicit ErrorReporter(ErrorHandler* handler) : handler_(*handler) {}

  // Report functions that forward to Handle functions of the same name on
  // the ErrorHandler implementation wrapped by this object.
  //
  // Optional `field_name` and `index` parameters allow entering a scope just
  // for the duration of the report.E.g.,
  //
  // reporter->ReportFhirError("err-msg", field_name, index);
  //
  // is equivalent to
  //
  // ErrorScope scope(reporter, field_name, index);
  // reporter->ReportFhirError("err-msg");
  // scope.~ErrorScope();
  absl::Status ReportFhirFatal(const absl::Status& status,
                               absl::string_view field_name = "",
                               std::optional<uint> index = std::nullopt);
  absl::Status ReportFhirError(std::string_view msg,
                               absl::string_view field_name = "",
                               std::optional<uint> index = std::nullopt);
  absl::Status ReportFhirWarning(std::string_view msg,
                                 absl::string_view field_name = "",
                                 std::optional<uint> index = std::nullopt);
  absl::Status ReportFhirPathFatal(const absl::Status& status,
                                   std::string_view expression,
                                   absl::string_view field_name = "",
                                   std::optional<uint> index = std::nullopt);
  absl::Status ReportFhirPathError(std::string_view expression,
                                   absl::string_view field_name = "",
                                   std::optional<uint> index = std::nullopt);
  absl::Status ReportFhirPathWarning(std::string_view expression,
                                     absl::string_view field_name = "",
                                     std::optional<uint> index = std::nullopt);

 private:
  ErrorHandler& handler_;
  std::vector<std::pair<std::string, std::optional<uint>>> scope_stack_;

  /**
   * Returns the field path to the current scope, not including any indexes
   * E.g., "Foo.bar.baz.quux"
   */
  std::string CurrentFieldPath();

  /**
   * Returns the element path to the current scope, including any indexes
   * E.g., "Foo.bar[2].baz.quux[0]"
   */
  std::string CurrentElementPath();

  /**
   * Scope management for Errors.
   * Scopes currently entered are available using the CurrentFieldPath and
   * CurrentElementPath functions.
   *
   * These are private, since Scopes should be managed automatically by
   * ErrorScope objects.
   */
  void EnterScope(absl::string_view scope,
                  std::optional<uint> index = std::nullopt);
  void ExitScope();
};

// Scope Management object for ErrorReporter.
// Enters a scope on creation, and exits scope on destruction.
// See documentation on ErrorReporter for more info.
class ErrorScope {
 public:
  // Enter the scope on construction
  explicit ErrorScope(ErrorReporter* reporter, absl::string_view name,
                      std::optional<std::uint8_t> index = std::nullopt)
      : reporter_(reporter) {
    reporter->EnterScope(name, index);
  }

  // Prevent copying - the scope can only be owned in one place
  ErrorScope(const ErrorScope&) = delete;
  ErrorScope operator=(const ErrorScope&) = delete;

  // Prevent moving - the scope should never outlive the local scope it is
  // created in.
  ErrorScope(ErrorScope&& moved) = delete;
  ErrorScope& operator=(ErrorScope&& moved) = delete;

  ~ErrorScope() {
    if (reporter_ != nullptr) {
      reporter_->ExitScope();
    }
  }

 private:
  ErrorReporter* reporter_;
};

// A thread-safe error handler implementation that simply returns a failure
// status on the first error it encounters. This is primarily for legacy use;
// most users should use an OperationOutcomeErrorReporter or their
// own implementation.
//
// This comes in two modes:
// * FailOnErrorOrFatal will return a status error
//   in response to a Fatal report (i.e., code problem), OR a Error report
//   (i.e., invalid data).  This is useful for simple determinations of
//   validity, as any data issue (above warning) will result in a status
//   failure.
// * FailOnFatalOnly will swallow any data quality issues, and only return a
//   status failure if the process could not be completed successfully.
//   This is useful for a "best-effort" that should finish even if it encounters
//   invalid data.
class FailFastErrorHandler : public ErrorHandler {
 public:
  // Returns a singleton instance of a fast-fail
  static FailFastErrorHandler& FailOnErrorOrFatal();

  // Returns a singleton instance of a "fast-fail on error only" reporter for
  // convenience.
  static FailFastErrorHandler& FailOnFatalOnly();

  absl::Status HandleFhirFatal(const absl::Status& status,
                               std::string_view element_path,
                               std::string_view field_path) override {
    return status;
  }

  absl::Status HandleFhirError(std::string_view msg,
                               std::string_view element_path,
                               std::string_view field_path) override {
    return behavior_ == FAIL_ON_ERROR_OR_FATAL
               ? absl::FailedPreconditionError(
                     absl::Substitute("$0 at $1", msg, element_path))
               : absl::OkStatus();
  }

  absl::Status HandleFhirWarning(std::string_view msg,
                                 std::string_view element_path,
                                 std::string_view field_path) override {
    return absl::OkStatus();
  }

  absl::Status HandleFhirPathFatal(const absl::Status& status,
                                   std::string_view expression,
                                   std::string_view element_path,
                                   std::string_view field_path) override {
    return absl::Status(
        status.code(),
        absl::Substitute("Error evaluating FHIRPath expression `$0`: $1 at $2",
                         expression, status.message(), element_path));
  }

  absl::Status HandleFhirPathError(std::string_view expression,
                                   std::string_view element_path,
                                   std::string_view field_path) override {
    return behavior_ == FAIL_ON_ERROR_OR_FATAL
               ? absl::FailedPreconditionError(absl::Substitute(
                     "Failed expression `$0` at $1", expression, element_path))
               : absl::OkStatus();
  }

  absl::Status HandleFhirPathWarning(std::string_view expression,
                                     std::string_view element_path,
                                     std::string_view field_path) override {
    return absl::OkStatus();
  }

 private:
  enum Behavior { FAIL_ON_ERROR_OR_FATAL, FAIL_ON_FATAL_ONLY };

  explicit FailFastErrorHandler(Behavior behavior) : behavior_(behavior) {}

  const Behavior behavior_;
};

std::optional<uint> IndexOrNullopt(const google::protobuf::FieldDescriptor* field,
                                   uint index);

}  // namespace google::fhir

#endif  // GOOGLE_FHIR_ERROR_REPORTER_H_
