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

#include "google/fhir/r4/operation_error_reporter.h"

#include <string>
#include <vector>

#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"

namespace google::fhir::r4 {

std::string FormatIssue(
    const google::fhir::r4::core::OperationOutcome::Issue& issue) {
  std::vector<std::string> err_loc;
  err_loc.reserve(issue.expression_size());
  for (const google::fhir::r4::core::String& expr : issue.expression()) {
    err_loc.push_back(expr.value());
  }
  return absl::StrFormat("[%s]: %s", absl::StrJoin(err_loc, "."),
                         issue.diagnostics().value());
}

}  // namespace google::fhir::r4
