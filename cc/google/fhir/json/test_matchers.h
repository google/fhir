// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GOOGLE_FHIR_JSON_TEST_MATCHERS_H_
#define GOOGLE_FHIR_JSON_TEST_MATCHERS_H_

#include "gmock/gmock.h"
#include "absl/strings/match.h"

namespace google {
namespace fhir {
namespace internal {

// Matchers for testing within the fhir package.
// Naming convention:
// * Matchers prefixed with "Is" expects `arg` to be an instance of
// absl::Status,
// * matchers prefixed with "Holds" expects `arg` to be an instance of
// absl::StatusOr<class>.

MATCHER(IsOkStatus, "") { return arg.ok(); }

MATCHER_P(IsErrorStatus, status_code, "") {
  return !arg.ok() && arg.code() == status_code;
}

MATCHER_P2(IsErrorStatus, status_code, substr, "") {
  return !arg.ok() && arg.code() == status_code &&
         absl::StrContains(arg.message(), substr);
}

MATCHER_P(HoldsData, data, "") { return arg.ok() && arg.value() == data; }

MATCHER_P(HoldsErrorStatus, status_code, "") {
  return !arg.ok() && arg.status().code() == status_code;
}

// Check for equality between FhirJson objects.
// `arg` is a const reference to FhirJson, `ptr_to_expected_json_object` is
// a pointer to FhirJson, since FhirJson is not copyable and callers may not
// want to move it as it can be still in use.
MATCHER_P(JsonEq, ptr_to_expected_json_object, "") {
  if (arg != *ptr_to_expected_json_object) {
    *result_listener << "expected " << ptr_to_expected_json_object->toString()
                     << ", actual " << arg.toString();
    return false;
  }
  return true;
}

}  // namespace internal
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_JSON_TEST_MATCHERS_H_
