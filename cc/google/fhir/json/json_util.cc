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

#include "google/fhir/json/json_util.h"

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "google/fhir/annotations.h"
#include "google/protobuf/descriptor.h"

namespace google::fhir {

absl::StatusOr<std::string> ToJsonStringValue(absl::string_view raw_value) {
  // For special characters that are permissible in FHIR strings,
  // a map from special character to escaped string.
  static const absl::flat_hash_map<char, std::string>*
      CHARACTERS_TO_ESCAPED_CHARACTER_MAP =
          new absl::flat_hash_map<char, std::string>({
              {'\"', "\\\""},
              {'\\', "\\\\"},
              {'\n', "\\n"},
              {'\r', "\\r"},
              {'\t', "\\t"},
          });

  std::string result = "\"";
  // For result string, reserve 110% the size of the original string,
  // to give room for added escape characters.
  // This doesn't need to be exact - result can still resize as needed.
  result.reserve(1.1 * raw_value.length());
  for (const unsigned char char_byte : raw_value) {
    auto replacement = CHARACTERS_TO_ESCAPED_CHARACTER_MAP->find(char_byte);
    if (replacement != CHARACTERS_TO_ESCAPED_CHARACTER_MAP->end()) {
      absl::StrAppend(&result, replacement->second);
    } else if (char_byte < 32) {
      return absl::InvalidArgumentError(absl::StrCat(
          "Invalid control character found in string.  Decimal value:",
          static_cast<int>(char_byte)));
    } else {
      result.push_back(char_byte);
    }
  }
  result.push_back('\"');

  return result;
}

absl::string_view FhirJsonName(const google::protobuf::FieldDescriptor* field) {
  // When translating References between proto and JSON, the unstructured
  // FHIR JSON Reference.reference field maps to the absolute URI field
  // in the proto Reference.
  if (field->name() == "uri" && IsReference(field->containing_type())) {
    return "reference";
  }
  return field->json_name();
}

}  // namespace google::fhir
