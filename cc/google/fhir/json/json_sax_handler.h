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

#ifndef GOOGLE_FHIR_INTERNAL_JSON_SAX_HANDLER_H_
#define GOOGLE_FHIR_INTERNAL_JSON_SAX_HANDLER_H_

#include <memory>
#include <string>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "google/fhir/json/fhir_json.h"

namespace google {
namespace fhir {
namespace internal {

// Parse input string into FhirJson, returns error if `raw_json` is not
// well-formatted.
absl::Status ParseJsonValue(
    absl::string_view raw_json, FhirJson& json_value);

}  // namespace internal
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_INTERNAL_JSON_SAX_HANDLER_H_
