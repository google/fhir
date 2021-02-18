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

#include "google/fhir/json_util.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "google/fhir/status/status.h"

namespace google::fhir {
namespace {

void TestToJsonStringValue(std::string input, std::string expected) {
  absl::StatusOr<std::string> result = ToJsonStringValue(input);
  FHIR_ASSERT_OK(result.status());
  EXPECT_EQ(*result, expected);
}

TEST(JsonUtilTest, ToJsonStringValueWithOkControlCharacters) {
  TestToJsonStringValue(
      R"(this is a "string"
with two lines)",
      R"("this is a \"string\"\nwith two lines")");
  TestToJsonStringValue("foo\rbar\nbaz\tquux", "\"foo\\rbar\\nbaz\\tquux\"");
}

TEST(JsonUtilTest, ToJsonStringValueWithMultiByteCharacters) {
  // Note that these two expects, along with the next pair, actually assert
  // the same thing, but are presented both as octal bytes and as characters
  // to be obvious about what is being tested.
  TestToJsonStringValue("🔥", "\"🔥\"");
  TestToJsonStringValue("\360\237\224\245", "\"\360\237\224\245\"");

  TestToJsonStringValue("Andr\303\251", "\"Andr\303\251\"");
  TestToJsonStringValue("André", "\"André\"");
}

TEST(JsonUtilTest, ToJsonStringValueControlCharactersPrinted) {
  std::string with_null_char{"\r\n\t\0\u0008", 5};
  TestToJsonStringValue(with_null_char, R"("\r\n\t\u0000\u0008")");
}

TEST(JsonUtilTest, ToJsonStringValueOnlyNullChar) {
  std::string only_null_char{"\0", 1};
  TestToJsonStringValue(only_null_char, R"("\u0000")");
}

}  // namespace
}  // namespace google::fhir
