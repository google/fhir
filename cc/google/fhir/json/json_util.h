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

#ifndef GOOGLE_FHIR_JSON_JSON_UTIL_H_
#define GOOGLE_FHIR_JSON_JSON_UTIL_H_

#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"

namespace google::fhir {

// Converts a string_view into the "value" part of a JSON param, adding
// quotes and escaping special characters.
//
// E.g., given the multiline string
// R"(this is
// "my favorite" string)"
// this would return the literal:
// R"("this is \n\"my favorite\" string")"
//
// Per FHIR spec, the only valid control characters in the range [0,32)
// are [\r\n\t].  This returns a status error if it encounters any other
// characters in that range.
// See https://www.hl7.org/fhir/datatypes.html#string
//
// Note that this does NOT escape the "solidus" `/` as `\/`.
// Per JSON spec, this optional but not required, so to maintain a minimal
// touch it is left as is. See section 9:
// http://www.ecma-international.org/publications/files/ECMA-ST/ECMA-404.pdf
absl::StatusOr<std::string> ToJsonStringValue(absl::string_view raw_value);

// Given a proto field, returns the name of the field in FHIR JSON.
absl::string_view FhirJsonName(const google::protobuf::FieldDescriptor* field);

}  // namespace google::fhir

#endif  // GOOGLE_FHIR_JSON_JSON_UTIL_H_
