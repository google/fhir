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

#include "gtest/gtest.h"
#include "google/fhir/status/status.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/observation.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/patient.pb.h"

namespace google::fhir {
namespace {

using ::google::fhir::r4::core::Observation;
using ::google::fhir::r4::core::Patient;
using ::google::fhir::r4::core::Reference;

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

TEST(JsonUtilTest, ToJsonStringValueInvalidControlCharactersReturnsError) {
  std::string with_null_char{"foo\0bar", 7};
  absl::StatusOr<std::string> result = ToJsonStringValue(with_null_char);
  EXPECT_FALSE(result.ok());
}

TEST(JsonUtilTest, FhirJsonNameNonReferenceSucceeds) {
  EXPECT_EQ(
      FhirJsonName(Patient::GetDescriptor()->FindFieldByName("implicit_rules")),
      "implicitRules");
}

TEST(JsonUtilTest, FhirJsonNameNonReferenceWithExplicitJsonNameSucceeds) {
  EXPECT_EQ(FhirJsonName(Observation::Component::ValueX::GetDescriptor()
                             ->FindFieldByName("string_value")),
            "string");
}

TEST(JsonUtilTest, FhirJsonNameReferenceUriSucceeds) {
  EXPECT_EQ(FhirJsonName(Reference::GetDescriptor()->FindFieldByName("uri")),
            "reference");
}

TEST(JsonUtilTest, FhirJsonNameReferenceOtherFieldSucceeds) {
  EXPECT_EQ(
      FhirJsonName(Reference::GetDescriptor()->FindFieldByName("fragment")),
      "fragment");
  EXPECT_EQ(
      FhirJsonName(Reference::GetDescriptor()->FindFieldByName("patient_id")),
      "patientId");
}

}  // namespace
}  // namespace google::fhir
