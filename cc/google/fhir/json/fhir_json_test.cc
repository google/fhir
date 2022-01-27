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

#include "google/fhir/json/fhir_json.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_replace.h"
#include "google/fhir/status/status.h"

using ::testing::Eq;

namespace google {
namespace fhir {
namespace internal {
namespace {

MATCHER_P(EqIgnoringWhiteSpaces, expected_string, "") {
  if (absl::StrReplaceAll(arg,
                          {{" ", ""}, {"\t", ""}, {"\r", ""}, {"\n", ""}}) !=
      absl::StrReplaceAll(expected_string,
                          {{" ", ""}, {"\t", ""}, {"\r", ""}, {"\n", ""}})) {
    *result_listener << "expected " << expected_string << ", actual " << arg;
    return false;
  }
  return true;
}

MATCHER_P(IsOkAndHolds, data, "") { return arg.ok() && arg.value() == data; }

MATCHER_P(InnerStatusIs, status_code, "") {
  return !arg.ok() && arg.status().code() == status_code;
}

MATCHER_P(StatusIs, status_code, "") {
  return !arg.ok() && arg.code() == status_code;
}

TEST(FhirJsonTest, PrimitiveFactoryFunctions) {
  EXPECT_THAT(FhirJson::CreateNull()->toString(),
              EqIgnoringWhiteSpaces("null"));
  EXPECT_THAT(FhirJson::CreateBoolean(true)->toString(),
              EqIgnoringWhiteSpaces("true"));
  EXPECT_THAT(FhirJson::CreateInteger(2)->toString(),
              EqIgnoringWhiteSpaces("2"));
  EXPECT_THAT(FhirJson::CreateUnsigned(2)->toString(),
              EqIgnoringWhiteSpaces("2"));
  EXPECT_THAT(FhirJson::CreateDecimal("3.14")->toString(),
              EqIgnoringWhiteSpaces("3.14"));
  EXPECT_THAT(FhirJson::CreateString("3.14")->toString(),
              EqIgnoringWhiteSpaces("\"3.14\""));
}

TEST(FhirJsonTest, MoveFrom) {
  std::unique_ptr<FhirJson> json = FhirJson::CreateNull();
  EXPECT_TRUE(json->MoveFrom(FhirJson::CreateInteger(2)).ok());
  EXPECT_THAT(json->MoveFrom(FhirJson::CreateBoolean(true)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(FhirJsonTest, ConversionBetweenIntegerTypes) {
  auto negative_int = FhirJson::CreateInteger(-2);
  EXPECT_FALSE(negative_int->isNull());
  EXPECT_FALSE(negative_int->isString());
  EXPECT_FALSE(negative_int->isObject());
  EXPECT_FALSE(negative_int->isArray());
  EXPECT_TRUE(negative_int->isInt());
  EXPECT_THAT(negative_int->asInt(), IsOkAndHolds(-2));
  EXPECT_FALSE(negative_int->isBool());
  EXPECT_THAT(negative_int->asBool(), IsOkAndHolds(true));

  auto unsigned_int = FhirJson::CreateUnsigned(0);
  EXPECT_FALSE(unsigned_int->isNull());
  EXPECT_FALSE(unsigned_int->isString());
  EXPECT_FALSE(unsigned_int->isObject());
  EXPECT_FALSE(unsigned_int->isArray());
  EXPECT_TRUE(unsigned_int->isInt());
  EXPECT_THAT(unsigned_int->asInt(), IsOkAndHolds(0));
  EXPECT_FALSE(unsigned_int->isBool());
  EXPECT_THAT(unsigned_int->asBool(), IsOkAndHolds(false));

  auto boolean = FhirJson::CreateBoolean(false);
  EXPECT_FALSE(boolean->isNull());
  EXPECT_FALSE(boolean->isString());
  EXPECT_FALSE(boolean->isObject());
  EXPECT_FALSE(boolean->isArray());
  EXPECT_FALSE(boolean->isInt());
  EXPECT_THAT(boolean->asInt(), IsOkAndHolds(0));
  EXPECT_TRUE(boolean->isBool());
  EXPECT_THAT(boolean->asBool(), IsOkAndHolds(false));
}

TEST(FhirJsonTest, ConversionForStringTypes) {
  auto string_val = FhirJson::CreateString("abc");
  EXPECT_FALSE(string_val->isNull());
  EXPECT_FALSE(string_val->isObject());
  EXPECT_FALSE(string_val->isArray());
  EXPECT_FALSE(string_val->isInt());
  EXPECT_TRUE(string_val->isString());
  EXPECT_THAT(string_val->asInt(),
              InnerStatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(string_val->asBool(),
              InnerStatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(string_val->asString(), IsOkAndHolds("abc"));

  auto decimal_val = FhirJson::CreateDecimal("3.14");
  EXPECT_FALSE(decimal_val->isNull());
  EXPECT_FALSE(decimal_val->isObject());
  EXPECT_FALSE(decimal_val->isArray());
  EXPECT_FALSE(decimal_val->isInt());
  EXPECT_TRUE(decimal_val->isString());
  EXPECT_THAT(decimal_val->asInt(),
              InnerStatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(decimal_val->asBool(),
              InnerStatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(decimal_val->asString(), IsOkAndHolds("3.14"));
}

TEST(FhirJsonTest, Arrays) {
  auto arr = FhirJson::CreateArray();
  EXPECT_THAT(arr->toString(), EqIgnoringWhiteSpaces("[]"));

  auto value_0 = arr->mutableValueToAppend();
  FHIR_ASSERT_OK(value_0.status());
  FHIR_ASSERT_OK(value_0.value()->MoveFrom(FhirJson::CreateInteger(-2)));
  EXPECT_THAT(arr->toString(), EqIgnoringWhiteSpaces("[-2]"));

  auto value_1 = arr->mutableValueToAppend();
  FHIR_ASSERT_OK(value_1.status());
  FHIR_ASSERT_OK(value_1.value()->MoveFrom(FhirJson::CreateBoolean(false)));
  EXPECT_THAT(arr->toString(), EqIgnoringWhiteSpaces("[-2, false]"));

  EXPECT_THAT(arr->arraySize(), IsOkAndHolds(2));

  auto get_0 = arr->get(0);
  FHIR_ASSERT_OK(get_0.status());
  ASSERT_TRUE(get_0.value()->isInt());
  EXPECT_THAT(get_0.value()->asInt(), IsOkAndHolds(-2));

  auto get_1 = arr->get(1);
  FHIR_ASSERT_OK(get_1.status());
  ASSERT_TRUE(get_1.value()->isBool());
  EXPECT_THAT(get_1.value()->asBool(), IsOkAndHolds(false));

  EXPECT_THAT(arr->get(2), InnerStatusIs(absl::StatusCode::kOutOfRange));
  EXPECT_THAT(arr->get(-1), InnerStatusIs(absl::StatusCode::kOutOfRange));
}

TEST(FhirJsonTest, IllegalUseOfArrayFunctions) {
  auto obj = FhirJson::CreateObject();
  EXPECT_THAT(obj->arraySize(),
              InnerStatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(obj->get(0),
              InnerStatusIs(absl::StatusCode::kFailedPrecondition));

  auto boolean = FhirJson::CreateBoolean(false);
  EXPECT_THAT(boolean->mutableValueToAppend(),
              InnerStatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(FhirJsonTest, Objects) {
  auto obj = FhirJson::CreateObject();
  EXPECT_THAT(obj->toString(), EqIgnoringWhiteSpaces("{}"));

  auto value_a = obj->mutableValueForKey("a");
  FHIR_ASSERT_OK(value_a.status());
  FHIR_ASSERT_OK(value_a.value()->MoveFrom(FhirJson::CreateInteger(-2)));

  EXPECT_THAT(obj->toString(), EqIgnoringWhiteSpaces(R"({
    "a": -2
  })"));

  auto value_b = obj->mutableValueForKey("b");
  FHIR_ASSERT_OK(value_b.status());
  FHIR_ASSERT_OK(value_b.value()->MoveFrom(FhirJson::CreateArray()));

  EXPECT_THAT(obj->toString(), EqIgnoringWhiteSpaces(R"({
    "a": -2,
    "b": []
  })"));

  auto get_a = obj->get("a");
  FHIR_ASSERT_OK(get_a.status());
  ASSERT_TRUE(get_a.value()->isInt());
  EXPECT_THAT(get_a.value()->asInt(), IsOkAndHolds(-2));

  auto get_b = obj->get("b");
  FHIR_ASSERT_OK(get_b.status());
  EXPECT_TRUE(get_b.value()->isArray());

  EXPECT_THAT(obj->mutableValueForKey("b"),
              InnerStatusIs(absl::StatusCode::kAlreadyExists));
  EXPECT_THAT(obj->get("c"), InnerStatusIs(absl::StatusCode::kNotFound));
}

TEST(FhirJsonTest, IllegalUseOfObjectFunctions) {
  auto arr = FhirJson::CreateArray();
  EXPECT_THAT(arr->objectMap(),
              InnerStatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(arr->get("a"),
              InnerStatusIs(absl::StatusCode::kFailedPrecondition));

  auto boolean = FhirJson::CreateBoolean(false);
  EXPECT_THAT(boolean->mutableValueForKey("a"),
              InnerStatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(FhirJsonTest, FormattingOfToString) {
  auto obj = FhirJson::CreateObject();

  FHIR_ASSERT_OK(obj->mutableValueForKey("a").value()->MoveFrom(
      FhirJson::CreateDecimal("3.14")));

  auto value_b = obj->mutableValueForKey("b").value();
  FHIR_ASSERT_OK(value_b->MoveFrom(FhirJson::CreateArray()));
  FHIR_ASSERT_OK(value_b->mutableValueToAppend().value()->MoveFrom(
      FhirJson::CreateBoolean(false)));
  auto array_element = value_b->mutableValueToAppend().value();
  FHIR_ASSERT_OK(array_element->MoveFrom(FhirJson::CreateObject()));
  FHIR_ASSERT_OK(array_element->mutableValueForKey("c").status());

  FHIR_ASSERT_OK(obj->mutableValueForKey("d").value()->MoveFrom(
      FhirJson::CreateString("abc")));

  EXPECT_THAT(obj->toString(), Eq(
                                   R"({
  "a": 3.14,
  "b": [
    false,
    {
      "c": null
    }
  ],
  "d": "abc"
})"));
}

}  // namespace
}  // namespace internal
}  // namespace fhir
}  // namespace google
