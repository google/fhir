/*
 * Copyright 2023 Google LLC
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

#include "google/fhir/r5/primitive_handler.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <string>

#include "google/protobuf/any.pb.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/json_format_results.h"
#include "google/fhir/operation_error_reporter.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/r5/core/codes.pb.h"
#include "proto/google/fhir/proto/r5/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/binary.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/observation.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/operation_outcome.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/patient.pb.h"
#include "proto/google/fhir/proto/r5/fhirproto_extensions.pb.h"

namespace google {
namespace fhir {
namespace r5 {

namespace {

using ::google::fhir::internal::FhirJson;
using ::google::fhir::r5::core::Integer64;
using ::google::fhir::testutil::EqualsProto;

using R5OperationOutcomeErrorHandler =
    OutcomeErrorHandler<r5::core::OperationOutcome, r5::core::IssueSeverityCode,
                        r5::core::IssueTypeCode>;

const absl::TimeZone kTimeZone = []() {
  absl::TimeZone tz;
  absl::LoadTimeZone("Australia/Sydney", &tz);
  return tz;
}();

void TestSuccessfulInteger64(const std::string& value,
                             const int64_t expected_value) {
  std::unique_ptr<FhirJson> json = FhirJson::CreateString(value);
  Integer64 integer64_proto;

  absl::StatusOr<ParseResult> result =
      R5PrimitiveHandler::GetInstance()->ParseInto(
          *json, &integer64_proto,
          ScopedErrorReporter(&FailFastErrorHandler::FailOnErrorOrFatal(),
                              "TestScope"));

  EXPECT_TRUE(result.status().ok());
  EXPECT_EQ(*result, ParseResult::kSucceeded)
      << "Invalid int64 value: " << value;

  Integer64 expected_output;
  expected_output.set_value(expected_value);
  EXPECT_THAT(integer64_proto, EqualsProto(expected_output));
}

TEST(R5PrimitiveHandler, ParseIntoInteger64Succeeds) {
  TestSuccessfulInteger64("5", 5);
  TestSuccessfulInteger64("9223372036854775807",
                          std::numeric_limits<int64_t>::max());
  TestSuccessfulInteger64("-9223372036854775808",
                          std::numeric_limits<int64_t>::min());
}

TEST(R5PrimitiveHandler, ParseIntoInteger64WithIncorrectTypeFails) {
  std::unique_ptr<FhirJson> json = FhirJson::CreateInteger(47);
  Integer64 integer64_proto;

  core::OperationOutcome outcome;
  R5OperationOutcomeErrorHandler handler(&outcome);

  absl::StatusOr<ParseResult> result =
      R5PrimitiveHandler::GetInstance()->ParseInto(
          *json, &integer64_proto, ScopedErrorReporter(&handler, "TestScope"));

  EXPECT_TRUE(result.status().ok());
  EXPECT_EQ(*result, ParseResult::kFailed);
  EXPECT_THAT(
      outcome,
      EqualsProto(
          R"pb(
            issue {
              severity { value: FATAL }
              code { value: STRUCTURE }
              diagnostics {
                value: "Unable to parse to google.fhir.r5.core.Integer64: JSON primitive must be string."
              }
              expression { value: "TestScope" }
            }
          )pb"));
}

TEST(R5PrimitiveHandler, ParseIntoInteger64OutOfBoundsFails) {
  std::unique_ptr<FhirJson> json =
      FhirJson::CreateString("922337203685477580700000");
  Integer64 integer64_proto;

  core::OperationOutcome outcome;
  R5OperationOutcomeErrorHandler handler(&outcome);

  absl::StatusOr<ParseResult> result =
      R5PrimitiveHandler::GetInstance()->ParseInto(
          *json, &integer64_proto, ScopedErrorReporter(&handler, "TestScope"));

  EXPECT_TRUE(result.status().ok());
  EXPECT_EQ(*result, ParseResult::kFailed);
  EXPECT_THAT(
      outcome,
      EqualsProto(
          R"pb(
            issue {
              severity { value: FATAL }
              code { value: STRUCTURE }
              diagnostics {
                value: "Failed converting \"922337203685477580700000\" to sint64 for Integer64 primitive."
              }
              expression { value: "TestScope" }
            }
          )pb"));
}

TEST(R5PrimitiveHandler, ParseIntoInteger64InvalidString) {
  std::unique_ptr<FhirJson> json = FhirJson::CreateString("Hasaan Reddick");
  Integer64 integer64_proto;

  core::OperationOutcome outcome;
  R5OperationOutcomeErrorHandler handler(&outcome);

  absl::StatusOr<ParseResult> result =
      R5PrimitiveHandler::GetInstance()->ParseInto(
          *json, &integer64_proto, ScopedErrorReporter(&handler, "TestScope"));

  EXPECT_TRUE(result.status().ok());
  EXPECT_EQ(*result, ParseResult::kFailed);
  EXPECT_THAT(
      outcome,
      EqualsProto(
          R"pb(
            issue {
              severity { value: FATAL }
              code { value: STRUCTURE }
              diagnostics {
                value: "Failed converting \"Hasaan Reddick\" to sint64 for Integer64 primitive."
              }
              expression { value: "TestScope" }
            }
          )pb"));
}

TEST(R5PrimitiveHandler, WrapPrimitiveProtoWithInteger64Succeeds) {
  Integer64 integer64_proto;
  integer64_proto.set_value(9223372036854775807l);
  auto extension = integer64_proto.add_extension();
  extension->mutable_url()->set_value("www.extension.com");
  extension->mutable_value()->mutable_string_value()->set_value("Jalen Hurts");

  absl::StatusOr<JsonPrimitive> json_primitive =
      R5PrimitiveHandler::GetInstance()->WrapPrimitiveProto(integer64_proto);

  EXPECT_TRUE(json_primitive.status().ok());
  EXPECT_EQ(json_primitive->value, "\"9223372036854775807\"");
  EXPECT_THAT(*json_primitive->element, EqualsProto(R"pb(
    extension {
      url { value: "www.extension.com" }
      value { string_value { value: "Jalen Hurts" } }
    }
  )pb"));
}

// Note that it is not possible to have an invalid Integer64.value, since
// anything that can be store in a sint64 is a valid FHIR Integer64 value.
TEST(R5PrimitiveHandler, ValidateInteger64Succeeds) {
  Integer64 integer64_proto;
  integer64_proto.set_value(9223372036854775807l);

  core::OperationOutcome outcome;
  R5OperationOutcomeErrorHandler handler(&outcome);

  EXPECT_TRUE(
      R5PrimitiveHandler::GetInstance()
          ->ValidatePrimitive(integer64_proto,
                              ScopedErrorReporter(&handler, "TestScope"))
          .ok());

  EXPECT_EQ(outcome.issue_size(), 0);
}

}  // namespace

}  // namespace r5
}  // namespace fhir
}  // namespace google
