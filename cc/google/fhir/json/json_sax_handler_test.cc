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

#include "google/fhir/json/json_sax_handler.h"

#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "google/fhir/json/test_matchers.h"

namespace google {
namespace fhir {
namespace internal {
namespace {

TEST(JsonSaxHandlerTest, ParseNull) {
  FhirJson json_value;
  EXPECT_THAT(ParseJsonValue("null", json_value), IsOkStatus());
  EXPECT_THAT(json_value, JsonEq(FhirJson::CreateNull().get()));
}

TEST(JsonSaxHandlerTest, ParseSignedInt) {
  FhirJson json_value;
  EXPECT_THAT(ParseJsonValue("-2", json_value), IsOkStatus());
  EXPECT_THAT(json_value, JsonEq(FhirJson::CreateInteger(-2).get()));
}

TEST(JsonSaxHandlerTest, ParseUnsignedInt) {
  FhirJson json_value;
  EXPECT_THAT(ParseJsonValue("2", json_value), IsOkStatus());
  EXPECT_THAT(json_value, JsonEq(FhirJson::CreateUnsigned(2).get()));
}

TEST(JsonSaxHandlerTest, ParseString) {
  FhirJson json_value;
  EXPECT_THAT(ParseJsonValue("\"abc\"", json_value), IsOkStatus());
  EXPECT_THAT(json_value, JsonEq(FhirJson::CreateString("abc").get()));
}

TEST(JsonSaxHandlerTest, ParseNonAsciiUtf8String) {
  FhirJson json_value;
  EXPECT_THAT(ParseJsonValue("\"🔥\"", json_value), IsOkStatus());
  EXPECT_THAT(json_value, JsonEq(FhirJson::CreateString("🔥").get()));
}

TEST(JsonSaxHandlerTest, ParseDecimal) {
  FhirJson json_value;
  EXPECT_THAT(ParseJsonValue("3.14", json_value), IsOkStatus());
  EXPECT_THAT(json_value, JsonEq(FhirJson::CreateDecimal("3.14").get()));
}

TEST(JsonSaxHandlerTest, ParseBoolean) {
  FhirJson json_value;
  EXPECT_THAT(ParseJsonValue("true", json_value), IsOkStatus());
  EXPECT_THAT(json_value, JsonEq(FhirJson::CreateBoolean(true).get()));
}

TEST(JsonSaxHandlerTest, ParseEmptyObject) {
  FhirJson json_value;
  EXPECT_THAT(ParseJsonValue("{}", json_value), IsOkStatus());
  EXPECT_THAT(json_value, JsonEq(FhirJson::CreateObject().get()));
}

TEST(JsonSaxHandlerTest, ParseEmptyArray) {
  FhirJson json_value;
  EXPECT_THAT(ParseJsonValue("[]", json_value), IsOkStatus());
  EXPECT_THAT(json_value, JsonEq(FhirJson::CreateArray().get()));
}

TEST(JsonSaxHandlerTest, ParseDictionary) {
  std::unique_ptr<FhirJson> expected = FhirJson::CreateObject();

  EXPECT_THAT(expected->mutableValueForKey("a").value()->MoveFrom(
                  FhirJson::CreateInteger(-2)),
              IsOkStatus());
  EXPECT_THAT(expected->mutableValueForKey("b").value()->MoveFrom(
                  FhirJson::CreateBoolean(false)),
              IsOkStatus());
  EXPECT_THAT(expected->mutableValueForKey("c").value()->MoveFrom(
                  FhirJson::CreateNull()),
              IsOkStatus());
  EXPECT_THAT(expected->mutableValueForKey("d").value()->MoveFrom(
                  FhirJson::CreateString("dictionary")),
              IsOkStatus());

  FhirJson json_value;
  EXPECT_THAT(ParseJsonValue(R"({
                  "a": -2,
                  "b": false,
                  "c": null,
                  "d": "dictionary"
              })",
                             json_value),
              IsOkStatus());
  EXPECT_THAT(json_value, JsonEq(expected.get()));
}

TEST(JsonSaxHandlerTest, ParseNestedDictionary) {
  std::unique_ptr<FhirJson> expected = FhirJson::CreateObject();

  FhirJson* value_a = expected->mutableValueForKey("a").value();
  EXPECT_THAT(value_a->MoveFrom(FhirJson::CreateObject()), IsOkStatus());

  FhirJson* value_b = value_a->mutableValueForKey("b").value();
  EXPECT_THAT(value_b->MoveFrom(FhirJson::CreateObject()), IsOkStatus());

  FhirJson* value_c = value_b->mutableValueForKey("c").value();
  EXPECT_THAT(value_c->MoveFrom(FhirJson::CreateInteger(-2)), IsOkStatus());

  FhirJson json_value;
  EXPECT_THAT(ParseJsonValue(R"({
                  "a": {
                    "b": {
                      "c": -2
                    }
                  }
              })",
                             json_value),
              IsOkStatus());
  EXPECT_THAT(json_value, JsonEq(expected.get()));
}

TEST(JsonSaxHandlerTest, ParseArray) {
  std::unique_ptr<FhirJson> expected = FhirJson::CreateArray();

  FhirJson json_value0;
  EXPECT_THAT(ParseJsonValue("[]", json_value0), IsOkStatus());
  EXPECT_THAT(json_value0, JsonEq(expected.get()));

  EXPECT_THAT(expected->mutableValueToAppend().value()->MoveFrom(
                  FhirJson::CreateBoolean(false)),
              IsOkStatus());
  FhirJson json_value1;
  EXPECT_THAT(ParseJsonValue("[false]", json_value1), IsOkStatus());
  EXPECT_THAT(json_value1, JsonEq(expected.get()));

  EXPECT_THAT(expected->mutableValueToAppend().value()->MoveFrom(
                  FhirJson::CreateBoolean(true)),
              IsOkStatus());
  EXPECT_THAT(expected->mutableValueToAppend().value()->MoveFrom(
                  FhirJson::CreateBoolean(false)),
              IsOkStatus());
  EXPECT_THAT(expected->mutableValueToAppend().value()->MoveFrom(
                  FhirJson::CreateBoolean(false)),
              IsOkStatus());

  FhirJson json_value4;
  EXPECT_THAT(ParseJsonValue("[false, true, false, false]", json_value4),
              IsOkStatus());
  EXPECT_THAT(json_value4, JsonEq(expected.get()));
}

TEST(JsonSaxHandlerTest, ArrayNestedInDictionary) {
  std::unique_ptr<FhirJson> expected = FhirJson::CreateObject();

  FhirJson* value_aa = expected->mutableValueForKey("aa").value();
  EXPECT_THAT(value_aa->MoveFrom(FhirJson::CreateArray()), IsOkStatus());
  EXPECT_THAT(value_aa->mutableValueToAppend().value()->MoveFrom(
                  FhirJson::CreateInteger(-2)),
              IsOkStatus());
  EXPECT_THAT(value_aa->mutableValueToAppend().value()->MoveFrom(
                  FhirJson::CreateBoolean(true)),
              IsOkStatus());
  EXPECT_THAT(value_aa->mutableValueToAppend().value()->MoveFrom(
                  FhirJson::CreateString("hello")),
              IsOkStatus());

  FhirJson* value_bb = expected->mutableValueForKey("bb").value();
  EXPECT_THAT(value_bb->MoveFrom(FhirJson::CreateObject()), IsOkStatus());

  FhirJson* value_cc = expected->mutableValueForKey("cc").value();
  EXPECT_THAT(value_cc->MoveFrom(FhirJson::CreateNull()), IsOkStatus());

  FhirJson json_value;
  EXPECT_THAT(ParseJsonValue(R"({
                  "aa": [-2, true, "hello"],
                  "bb": {},
                  "cc": null
              })",
                             json_value),
              IsOkStatus());
  EXPECT_THAT(json_value, JsonEq(expected.get()));
}

TEST(JsonSaxHandlerTest, DictionaryNestedInArray) {
  std::unique_ptr<FhirJson> expected = FhirJson::CreateArray();

  std::unique_ptr<FhirJson> value_0 = FhirJson::CreateObject();
  EXPECT_THAT(value_0->mutableValueForKey("aa").value()->MoveFrom(
                  FhirJson::CreateInteger(-2)),
              IsOkStatus());
  EXPECT_THAT(
      expected->mutableValueToAppend().value()->MoveFrom(std::move(value_0)),
      IsOkStatus());

  std::unique_ptr<FhirJson> value_1 = FhirJson::CreateObject();
  EXPECT_THAT(
      expected->mutableValueToAppend().value()->MoveFrom(std::move(value_1)),
      IsOkStatus());

  std::unique_ptr<FhirJson> value_2 = FhirJson::CreateObject();
  EXPECT_THAT(value_2->mutableValueForKey("aa").value()->MoveFrom(
                  FhirJson::CreateBoolean(true)),
              IsOkStatus());
  EXPECT_THAT(
      expected->mutableValueToAppend().value()->MoveFrom(std::move(value_2)),
      IsOkStatus());

  FhirJson json_value;
  EXPECT_THAT(ParseJsonValue(R"([
                  {"aa": -2},
                  {},
                  {"aa": true}
              ])",
                             json_value),
              IsOkStatus());
  EXPECT_THAT(json_value, JsonEq(expected.get()));
}

TEST(JsonSaxHandlerTest, ErrorParseEmptyInput) {
  FhirJson json_value;
  EXPECT_THAT(ParseJsonValue("", json_value),
              testing::AnyOf(IsErrorStatus(absl::StatusCode::kInvalidArgument,
                                           "unexpected end of input"),
                             IsErrorStatus(absl::StatusCode::kInvalidArgument,
                                           "empty input")));
}

TEST(JsonSaxHandlerTest, ErrorParseInvalidLiteral) {
  FhirJson json_value;
  EXPECT_THAT(
      ParseJsonValue("2f", json_value),
      IsErrorStatus(absl::StatusCode::kInvalidArgument, "invalid literal"));

  EXPECT_THAT(
      ParseJsonValue("True", json_value),
      IsErrorStatus(absl::StatusCode::kInvalidArgument, "invalid literal"));

  EXPECT_THAT(
      ParseJsonValue("NULL", json_value),
      IsErrorStatus(absl::StatusCode::kInvalidArgument, "invalid literal"));

  EXPECT_THAT(
      ParseJsonValue("(2)", json_value),
      IsErrorStatus(absl::StatusCode::kInvalidArgument, "invalid literal"));

  EXPECT_THAT(ParseJsonValue("\"string", json_value),
              IsErrorStatus(absl::StatusCode::kInvalidArgument,
                            "missing closing quote"));
}

TEST(JsonSaxHandlerTest, ErrorParseBracketMismatch) {
  FhirJson json_value0, json_value1;
  EXPECT_THAT(ParseJsonValue("{\"key\": 2", json_value0),
              IsErrorStatus(absl::StatusCode::kInvalidArgument,
                            "unexpected end of input"));

  EXPECT_THAT(
      ParseJsonValue("[}", json_value1),
      IsErrorStatus(absl::StatusCode::kInvalidArgument, "unexpected '}'"));
}

TEST(JsonSaxHandlerTest, ErrorMissingComma) {
  FhirJson json_value0, json_value1, json_value2;
  EXPECT_THAT(ParseJsonValue("null 2", json_value0),
              IsErrorStatus(absl::StatusCode::kInvalidArgument,
                            "unexpected number literal"));

  EXPECT_THAT(ParseJsonValue("[2, {} null]", json_value1),
              IsErrorStatus(absl::StatusCode::kInvalidArgument,
                            "unexpected null literal"));

  EXPECT_THAT(ParseJsonValue("{\"key1\": true \"key2\": false}", json_value2),
              IsErrorStatus(absl::StatusCode::kInvalidArgument,
                            "unexpected string literal"));
}

TEST(JsonSaxHandlerTest, ErrorParseExtraTrailingComma) {
  FhirJson json_value0, json_value1, json_value2;
  EXPECT_THAT(
      ParseJsonValue("{\"key\": 2},", json_value0),
      IsErrorStatus(absl::StatusCode::kInvalidArgument, "unexpected ','"));

  EXPECT_THAT(
      ParseJsonValue("{\"key\": 2,}", json_value1),
      IsErrorStatus(absl::StatusCode::kInvalidArgument, "unexpected '}'"));

  EXPECT_THAT(
      ParseJsonValue("[true, false,]", json_value2),
      IsErrorStatus(absl::StatusCode::kInvalidArgument, "unexpected ']'"));
}

TEST(JsonSaxHandlerTest, ErrorParseInvalidObjectKey) {
  FhirJson json_value0, json_value1;
  EXPECT_THAT(ParseJsonValue("{2}", json_value0),
              IsErrorStatus(absl::StatusCode::kInvalidArgument,
                            " expected string literal"));

  EXPECT_THAT(ParseJsonValue("{non_string_key: 2}", json_value1),
              IsErrorStatus(absl::StatusCode::kInvalidArgument,
                            " expected string literal"));
}

}  // namespace
}  // namespace internal
}  // namespace fhir
}  // namespace google
