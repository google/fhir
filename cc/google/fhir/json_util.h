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

#ifndef GOOGLE_FHIR_JSON_UTIL_H_
#define GOOGLE_FHIR_JSON_UTIL_H_

#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"

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
// Note that this will print all control characters with their escape
// counterparts. While the FHIR spec states that "Strings SHOULD not contain
// Unicode character points below 32, except for u0009 (horizontal tab), u0010
// (carriage return) and u0013 (line feed)", we take a permissive stance in what
// it allows, and will not reject or otherwise surpress other control
// characters.
// See: https://www.hl7.org/fhir/datatypes.html#string
//
// Note that this does NOT escape the "solidus" `/` as `\/`.
// Per JSON spec, this optional but not required, so to maintain a minimal
// touch it is left as is. See section 9:
// http://www.ecma-international.org/publications/files/ECMA-ST/ECMA-404.pdf
absl::StatusOr<std::string> ToJsonStringValue(absl::string_view raw_value);

}  // namespace google::fhir

#endif  // GOOGLE_FHIR_JSON_UTIL_H_
