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

#include "google/fhir/error_reporter.h"

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"

namespace google {
namespace fhir {

FailFastErrorReporter* FailFastErrorReporter::FailOnErrorOrFatal() {
  static FailFastErrorReporter* const kInstance =
      new FailFastErrorReporter(FailFastErrorReporter::FAIL_ON_ERROR_OR_FATAL);
  return kInstance;
}

FailFastErrorReporter* FailFastErrorReporter::FailOnFatalOnly() {
  static FailFastErrorReporter* const kInstance =
      new FailFastErrorReporter(FailFastErrorReporter::FAIL_ON_FATAL_ONLY);
  return kInstance;
}

absl::Status ErrorReporter::ReportFhirFatal(absl::string_view field_path,
                                            const absl::Status& status) {
  return ReportFhirFatal(field_path, "", status);
}

absl::Status ErrorReporter::ReportFhirError(absl::string_view field_path,
                                            absl::string_view message) {
  return ReportFhirError(field_path, "", message);
}

absl::Status ErrorReporter::ReportFhirWarning(absl::string_view field_path,
                                              absl::string_view message) {
  return ReportFhirWarning(field_path, "", message);
}

absl::Status ErrorReporter::ReportFhirPathFatal(
    absl::string_view field_path, absl::string_view element_path,
    absl::string_view fhir_path_constraint, const absl::Status& status) {
  return ReportFhirFatal(field_path, element_path, status);
}

absl::Status ErrorReporter::ReportFhirPathError(
    absl::string_view field_path, absl::string_view element_path,
    absl::string_view fhir_path_constraint) {
  return ReportFhirError(field_path, element_path, fhir_path_constraint);
}

absl::Status ErrorReporter::ReportFhirPathWarning(
    absl::string_view field_path, absl::string_view element_path,
    absl::string_view fhir_path_constraint) {
  return ReportFhirWarning(field_path, element_path, fhir_path_constraint);
}

}  // namespace fhir
}  // namespace google
