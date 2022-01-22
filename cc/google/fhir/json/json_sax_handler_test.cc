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

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/match.h"
#include "include/json/writer.h"

namespace google {
namespace fhir {
namespace internal {
namespace {

MATCHER_P(IsOkWithJson, expected_json_object, "") {
  if (!arg.ok()) {
    *result_listener << "status error: " << arg.status().message();
    return false;
  }
  Json::StyledWriter writer;
  if (arg.value() != expected_json_object) {
    *result_listener << "expected " << writer.write(expected_json_object)
                     << ", actual " << writer.write(arg.value());
    return false;
  }
  return true;
}

TEST(JsonSaxHandlerTest, ParseLiteral) {
  EXPECT_THAT(ParseJsonValue("null"),
              IsOkWithJson(Json::Value(Json::nullValue)));
  EXPECT_THAT(ParseJsonValue("-2"), IsOkWithJson(Json::Value(-2)));
  EXPECT_THAT(ParseJsonValue("2"), IsOkWithJson(Json::Value(2u)));
  EXPECT_THAT(ParseJsonValue("\"abc\""), IsOkWithJson(Json::Value("abc")));
  EXPECT_THAT(ParseJsonValue("3.14"), IsOkWithJson(Json::Value("3.14")));
  EXPECT_THAT(ParseJsonValue("\"3.14\""), IsOkWithJson(Json::Value("3.14")));
  EXPECT_THAT(ParseJsonValue("true"), IsOkWithJson(Json::Value(true)));
  EXPECT_THAT(ParseJsonValue("false"), IsOkWithJson(Json::Value(false)));
  EXPECT_THAT(ParseJsonValue("\"🔥\""), IsOkWithJson(Json::Value("🔥")));
}

TEST(JsonSaxHandlerTest, ParseNull) {
  EXPECT_THAT(ParseJsonValue("null"),
              IsOkWithJson(Json::Value(Json::nullValue)));
}

TEST(JsonSaxHandlerTest, ParseEmptyObject) {
  EXPECT_THAT(ParseJsonValue("{}"),
              IsOkWithJson(Json::Value(Json::objectValue)));
}

TEST(JsonSaxHandlerTest, ParseDictionary) {
  Json::Value expected;
  expected["a"] = Json::Value(-2);
  expected["b"] = Json::Value(false);
  expected["c"] = Json::Value();
  expected["d"] = Json::Value("dictionary");

  EXPECT_THAT(ParseJsonValue(R"({
                  "a": -2,
                  "b": false,
                  "c": null,
                  "d": "dictionary"
              })"),
              IsOkWithJson(expected));
}

TEST(JsonSaxHandlerTest, ParseNestedDictionary) {
  Json::Value expected;
  expected["a"]["b"]["c"] = Json::Value(-2);

  EXPECT_THAT(ParseJsonValue(R"({
                  "a": {
                    "b": {
                      "c": -2
                    }
                  }
              })"),
              IsOkWithJson(expected));
}

TEST(JsonSaxHandlerTest, ParseArray) {
  Json::Value expected(Json::arrayValue);
  EXPECT_THAT(ParseJsonValue("[]"), IsOkWithJson(expected));

  expected.append(Json::Value(false));
  EXPECT_THAT(ParseJsonValue("[false]"), IsOkWithJson(expected));

  expected.append(Json::Value(true));
  expected.append(Json::Value(false));
  expected.append(Json::Value(false));

  EXPECT_THAT(ParseJsonValue("[false, true, false, false]"),
              IsOkWithJson(expected));
}

TEST(JsonSaxHandlerTest, ArrayNestedInDictionary) {
  Json::Value expected;
  expected["aa"].append(Json::Value(-2));
  expected["aa"].append(Json::Value(true));
  expected["aa"].append(Json::Value("hello"));
  expected["bb"] = Json::Value(Json::objectValue);
  expected["cc"] = Json::Value(Json::arrayValue);

  EXPECT_THAT(ParseJsonValue(R"({
                  "aa": [-2, true, "hello"],
                  "bb": {},
                  "cc": []
              })"),
              IsOkWithJson(expected));
}

TEST(JsonSaxHandlerTest, DictionaryNestedInArray) {
  Json::Value expected;
  expected.resize(3);
  expected[0]["aa"] = Json::Value(-2);
  expected[1] = Json::Value(Json::objectValue);
  expected[2]["aa"] = Json::Value(true);

  EXPECT_THAT(ParseJsonValue(R"([
                  {"aa": -2},
                  {},
                  {"aa": true}
              ])"),
              IsOkWithJson(expected));
}

MATCHER_P2(HasErrorStatus, status_code, substr, "") {
  return !arg.ok() && arg.status().code() == status_code &&
         absl::StrContains(arg.status().message(), substr);
}

TEST(JsonSaxHandlerTest, ErrorParseEmptyInput) {
  EXPECT_THAT(ParseJsonValue(""),
              HasErrorStatus(absl::StatusCode::kInvalidArgument,
                             "unexpected end of input"));
}

TEST(JsonSaxHandlerTest, ErrorParseInvalidLiteral) {
  EXPECT_THAT(
      ParseJsonValue("2f"),
      HasErrorStatus(absl::StatusCode::kInvalidArgument, "invalid literal"));

  EXPECT_THAT(
      ParseJsonValue("True"),
      HasErrorStatus(absl::StatusCode::kInvalidArgument, "invalid literal"));

  EXPECT_THAT(
      ParseJsonValue("NULL"),
      HasErrorStatus(absl::StatusCode::kInvalidArgument, "invalid literal"));

  EXPECT_THAT(
      ParseJsonValue("(2)"),
      HasErrorStatus(absl::StatusCode::kInvalidArgument, "invalid literal"));

  EXPECT_THAT(ParseJsonValue("\"string"),
              HasErrorStatus(absl::StatusCode::kInvalidArgument,
                             "missing closing quote"));
}

TEST(JsonSaxHandlerTest, ErrorParseBracketMismatch) {
  EXPECT_THAT(ParseJsonValue("{\"key\": 2"),
              HasErrorStatus(absl::StatusCode::kInvalidArgument,
                             "unexpected end of input"));

  EXPECT_THAT(
      ParseJsonValue("[}"),
      HasErrorStatus(absl::StatusCode::kInvalidArgument, "unexpected '}'"));
}

TEST(JsonSaxHandlerTest, ErrorMissingComma) {
  EXPECT_THAT(ParseJsonValue("null 2"),
              HasErrorStatus(absl::StatusCode::kInvalidArgument,
                             "unexpected number literal"));

  EXPECT_THAT(ParseJsonValue("[2, {} null]"),
              HasErrorStatus(absl::StatusCode::kInvalidArgument,
                             "unexpected null literal"));

  EXPECT_THAT(ParseJsonValue("{\"key1\": true \"key2\": false}"),
              HasErrorStatus(absl::StatusCode::kInvalidArgument,
                             "unexpected string literal"));
}

TEST(JsonSaxHandlerTest, ErrorParseExtraTrailingComma) {
  EXPECT_THAT(
      ParseJsonValue("{\"key\": 2},"),
      HasErrorStatus(absl::StatusCode::kInvalidArgument, "unexpected ','"));

  EXPECT_THAT(
      ParseJsonValue("{\"key\": 2,}"),
      HasErrorStatus(absl::StatusCode::kInvalidArgument, "unexpected '}'"));

  EXPECT_THAT(
      ParseJsonValue("[true, false,]"),
      HasErrorStatus(absl::StatusCode::kInvalidArgument, "unexpected ']'"));
}

TEST(JsonSaxHandlerTest, ErrorParseInvalidObjectKey) {
  EXPECT_THAT(ParseJsonValue("{2}"),
              HasErrorStatus(absl::StatusCode::kInvalidArgument,
                             " expected string literal"));

  EXPECT_THAT(ParseJsonValue("{non_string_key: 2}"),
              HasErrorStatus(absl::StatusCode::kInvalidArgument,
                             " expected string literal"));
}

}  // namespace
}  // namespace internal
}  // namespace fhir
}  // namespace google
