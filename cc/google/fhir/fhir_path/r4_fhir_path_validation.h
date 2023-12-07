// Copyright 2020 Google LLC
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

#ifndef GOOGLE_FHIR_FHIR_PATH_R4_FHIR_PATH_VALIDATION_H_
#define GOOGLE_FHIR_FHIR_PATH_R4_FHIR_PATH_VALIDATION_H_

#include "absl/base/macros.h"
#include "google/fhir/fhir_path/fhir_path_validation.h"
#include "google/protobuf/message.h"

namespace google {
namespace fhir {
namespace r4 {

// This class verifies that all fhir_path_constraint annotations on a given
// R4 message are met. It will compile and cache the constraint expressions
// as it encounters them, so users are encouraged to create a single instance of
// this for the lifetime of the process.
//
// This class is thread safe.
class FhirPathValidator : public ::google::fhir::fhir_path::FhirPathValidator {
 public:
  FhirPathValidator();
};

// Returns a shared instance of the R4 message validator.
ABSL_MUST_USE_RESULT
const ::google::fhir::fhir_path::FhirPathValidator* GetFhirPathValidator();

}  // namespace r4
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_VALIDATION_H_
