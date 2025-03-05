/*
 * Copyright 2023 Google LLC
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

#include "google/fhir/r5/operation_error_reporter.h"

#include <string>
#include <utility>

#include "absl/strings/str_format.h"
#include "proto/google/fhir/proto/r5/core/datatypes.pb.h"

namespace google::fhir::r5 {

std::string FormatIssue(
    const google::fhir::r5::core::OperationOutcome::Issue& issue) {
  // Each element in the expression field references the full FHIR path of one
  // affected element. And since our error reporting tools only report one field
  // per issue right now, we only need the first expression.
  // Note: Our tools only report one field per issue, but may (depending on the
  // error handler) return an `Outcome` object with multiple(ScopedErrorHandler)
  // `Issue`s or just one(FailFastErrorHandler) `Issue`.
  std::string element_path =
      issue.expression().empty() ? "" : issue.expression(0).value();
  return absl::StrFormat("[%s]: %s", std::move(element_path),
                         issue.diagnostics().value());
}

}  // namespace google::fhir::r5
