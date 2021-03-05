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

namespace google {
namespace fhir {

FailFastErrorReporter* FailFastErrorReporter::Get() {
  static FailFastErrorReporter* const kInstance = new FailFastErrorReporter();
  return kInstance;
}

absl::Status ErrorReporter::ReportFhirPathError(
    absl::string_view element_path, absl::string_view fhir_path_constraint,
    absl::string_view message) {
  absl::Status status = absl::FailedPreconditionError(
      message.empty() ? fhir_path_constraint
                      : absl::StrCat(fhir_path_constraint, ": ", message));
  return ReportError(element_path, status);
}

absl::Status ErrorReporter::ReportFhirPathWarning(
    absl::string_view element_path, absl::string_view fhir_path_constraint,
    absl::string_view message) {
  absl::Status status = absl::FailedPreconditionError(
      message.empty() ? fhir_path_constraint
                      : absl::StrCat(fhir_path_constraint, ": ", message));
  return ReportWarning(element_path, status);
}

}  // namespace fhir
}  // namespace google
