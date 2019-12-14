// Copyright 2019 Google LLC
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

#include "google/fhir/fhir_path/fhir_path.h"

#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/message_differencer.h"
#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "absl/time/civil_time.h"
#include "absl/time/time.h"
#include "google/fhir/status/status.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/r4/core/resources/encounter.pb.h"
#include "proto/stu3/codes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "proto/stu3/uscore.pb.h"
#include "proto/stu3/uscore_codes.pb.h"
#include "tensorflow/core/lib/core/errors.h"

using ::google::fhir::r4::core::DateTime;
using ::google::fhir::r4::core::Period;
using ::google::fhir::r4::core::Quantity;

using ::google::fhir::stu3::proto::Code;
using ::google::fhir::stu3::proto::Coding;
using ::google::fhir::stu3::proto::Encounter;
using ::google::fhir::stu3::proto::Observation;
using ::google::fhir::stu3::proto::Uri;
using ::google::fhir::stu3::proto::ValueSet;
using ::google::fhir::stu3::uscore::UsCoreBirthSexCode;
using ::google::fhir::stu3::uscore::UsCorePatient;

using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::util::MessageDifferencer;
using google::fhir::Status;
using google::fhir::fhir_path::CompiledExpression;
using google::fhir::fhir_path::EvaluationResult;
using google::fhir::fhir_path::MessageValidator;

static ::google::protobuf::TextFormat::Parser parser;  // NOLINT

template <typename T>
T ParseFromString(const std::string& str) {
  google::protobuf::TextFormat::Parser parser;
  parser.AllowPartialMessage(true);
  T t;
  EXPECT_TRUE(parser.ParseFromString(str, &t));
  return t;
}

Encounter ValidEncounter() {
  return ParseFromString<Encounter>(R"proto(
    status { value: TRIAGED }
    id { value: "123" }
    period {
      start: { value_us: 1556750153000 timezone: "America/Los_Angeles" }
    }
  )proto");
}

Observation ValidObservation() {
  return ParseFromString<Observation>(R"proto(
    status { value: FINAL }
    code {
      coding {
        system { value: "foo" }
        code { value: "bar" }
      }
    }
    id { value: "123" }
  )proto");
}

ValueSet ValidValueSet() {
  return ParseFromString<ValueSet>(R"proto(
    url { value: "http://example.com/valueset" }
  )proto");
}

UsCorePatient ValidUsCorePatient() {
  return ParseFromString<UsCorePatient>(R"proto(
    identifier {
      system { value: "foo" },
      value: { value: "http://example.com/patient" }
    }
  )proto");
}

google::fhir::StatusOr<EvaluationResult> EvaluateExpressionWithStatus(
    const std::string& expression) {
  // FHIRPath assumes a EvaluateBoolExpression object during evaluation, so we
  // use an encounter as a placeholder.
  auto compiled_expression =
      CompiledExpression::Compile(Encounter::descriptor(), expression);
  if (!compiled_expression.ok()) {
    return google::fhir::StatusOr<EvaluationResult>(
        compiled_expression.status());
  }

  Encounter test_encounter = ValidEncounter();
  return compiled_expression.ValueOrDie().Evaluate(test_encounter);
}


google::fhir::StatusOr<std::string> EvaluateStringExpressionWithStatus(
    const std::string& expression) {
  google::fhir::StatusOr<EvaluationResult> result =
      EvaluateExpressionWithStatus(expression);

  if (!result.ok()) {
    return google::fhir::StatusOr<std::string>(result.status());
  }

  return result.ValueOrDie().GetString();
}

google::fhir::StatusOr<bool> EvaluateBoolExpressionWithStatus(
    const std::string& expression) {
  google::fhir::StatusOr<EvaluationResult> result =
      EvaluateExpressionWithStatus(expression);

  if (!result.ok()) {
    return google::fhir::StatusOr<bool>(result.status());
  }

  return result.ValueOrDie().GetBoolean();
}

bool EvaluateBoolExpression(const std::string& expression) {
  return EvaluateBoolExpressionWithStatus(expression).ValueOrDie();
}

DateTime ToDateTime(const absl::CivilSecond& civil_time,
                    const absl::TimeZone& zone,
                    const DateTime::Precision& precision) {
  DateTime date_time;
  date_time.set_value_us(absl::ToUnixMicros(absl::FromCivil(civil_time, zone)));
  date_time.set_timezone(zone.name());
  date_time.set_precision(precision);
  return date_time;
}

// Helper to evaluate boolean expressions on periods with the
// given start and end times.
bool EvaluateOnPeriod(const CompiledExpression& expression,
                      const DateTime& start, const DateTime& end) {
  Period period;
  *period.mutable_start() = start;
  *period.mutable_end() = end;
  EvaluationResult result = expression.Evaluate(period).ValueOrDie();
  return result.GetBoolean().ValueOrDie();
}

TEST(FhirPathTest, TestMalformed) {
  auto expr = CompiledExpression::Compile(Encounter::descriptor(),
                                          "expression->not->valid");

  EXPECT_FALSE(expr.ok());
}

TEST(FhirPathTest, TestGetDirectChild) {
  auto expr = CompiledExpression::Compile(Encounter::descriptor(), "status")
                  .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  ASSERT_EQ(1, result.GetMessages().size());

  const Message* status = result.GetMessages()[0];

  EXPECT_TRUE(MessageDifferencer::Equals(*status, test_encounter.status()));
}

TEST(FhirPathTest, TestGetGrandchild) {
  auto expr =
      CompiledExpression::Compile(Encounter::descriptor(), "period.start")
          .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  ASSERT_EQ(1, result.GetMessages().size());

  const Message* status = result.GetMessages()[0];

  EXPECT_TRUE(
      MessageDifferencer::Equals(*status, test_encounter.period().start()));
}

TEST(FhirPathTest, TestGetEmptyGrandchild) {
  auto expr = CompiledExpression::Compile(Encounter::descriptor(), "period.end")
                  .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  EXPECT_EQ(0, result.GetMessages().size());
}

TEST(FhirPathTest, TestNoSuchField) {
  auto root_expr =
      CompiledExpression::Compile(Encounter::descriptor(), "bogusrootfield");

  EXPECT_FALSE(root_expr.ok());
  EXPECT_NE(root_expr.status().error_message().find("bogusrootfield"),
            std::string::npos);

  auto child_expr = CompiledExpression::Compile(Encounter::descriptor(),
                                                "period.boguschildfield");

  EXPECT_FALSE(child_expr.ok());
  EXPECT_NE(child_expr.status().error_message().find("boguschildfield"),
            std::string::npos);
}

TEST(FhirPathTest, TestNoSuchFunction) {
  auto root_expr = CompiledExpression::Compile(Encounter::descriptor(),
                                               "period.bogusfunction()");

  EXPECT_FALSE(root_expr.ok());
  EXPECT_NE(root_expr.status().error_message().find("bogusfunction"),
            std::string::npos);
}

TEST(FhirPathTest, TestFunctionExists) {
  auto expr = CompiledExpression::Compile(Encounter::descriptor(),
                                          "period.start.exists()")
                  .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  EXPECT_TRUE(result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestFunctionExistsNegation) {
  auto expr = CompiledExpression::Compile(Encounter::descriptor(),
                                          "period.start.exists().not()")
                  .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  EXPECT_FALSE(result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestFunctionNotExists) {
  auto expr = CompiledExpression::Compile(Encounter::descriptor(),
                                          "period.end.exists()")
                  .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  EXPECT_FALSE(result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestFunctionNotExistsNegation) {
  auto expr = CompiledExpression::Compile(Encounter::descriptor(),
                                          "period.end.exists().not()")
                  .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  EXPECT_TRUE(result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestFunctionHasValue) {
  auto expr = CompiledExpression::Compile(Encounter::descriptor(),
                                          "period.start.hasValue()")
                  .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  EXPECT_TRUE(result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestLogicalValueFieldExists) {
  // The logical .value field on primitives should return the primitive itself.
  auto expr = CompiledExpression::Compile(Quantity::descriptor(),
                                          "value.value.exists()")
                  .ValueOrDie();
  Quantity quantity;
  quantity.mutable_value()->set_value("100");
  EvaluationResult result = expr.Evaluate(quantity).ValueOrDie();
  EXPECT_TRUE(result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestFunctionHasValueNegation) {
  auto expr = CompiledExpression::Compile(Encounter::descriptor(),
                                          "period.start.hasValue().not()")
                  .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult has_value_result =
      expr.Evaluate(test_encounter).ValueOrDie();
  EXPECT_FALSE(has_value_result.GetBoolean().ValueOrDie());

  test_encounter.mutable_period()->clear_start();
  EvaluationResult no_value_result = expr.Evaluate(test_encounter).ValueOrDie();
  EXPECT_TRUE(no_value_result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestFunctionStartsWith) {
  // Missing argument
  EXPECT_FALSE(EvaluateBoolExpressionWithStatus("'foo'.startsWith()").ok());

  // Too many arguments
  EXPECT_FALSE(
      EvaluateBoolExpressionWithStatus("'foo'.startsWith('foo', 'foo')").ok());

  // Wrong argument type
  EXPECT_FALSE(EvaluateBoolExpressionWithStatus("'foo'.startsWith(1)").ok());
  EXPECT_FALSE(EvaluateBoolExpressionWithStatus("'foo'.startsWith(1.0)").ok());
  EXPECT_FALSE(EvaluateBoolExpressionWithStatus(
                   "'foo'.startsWith(@2015-02-04T14:34:28Z)")
                   .ok());
  EXPECT_FALSE(EvaluateBoolExpressionWithStatus("'foo'.startsWith(true)").ok());

  // Function does not exist for non-string type
  EXPECT_FALSE(EvaluateBoolExpressionWithStatus("1.startsWith(1)").ok());
  EXPECT_FALSE(EvaluateBoolExpressionWithStatus("1.startsWith('1')").ok());

  // Basic cases
  EXPECT_TRUE(EvaluateBoolExpression("''.startsWith('')"));
  EXPECT_TRUE(EvaluateBoolExpression("'foo'.startsWith('')"));
  EXPECT_TRUE(EvaluateBoolExpression("'foo'.startsWith('f')"));
  EXPECT_TRUE(EvaluateBoolExpression("'foo'.startsWith('foo')"));
  EXPECT_FALSE(EvaluateBoolExpression("'foo'.startsWith('foob')"));
}

TEST(FhirPathTest, TestFunctionStartsWithSelfReference) {
  auto expr = CompiledExpression::Compile(
                  Observation::descriptor(),
                  "code.coding.code.startsWith(code.coding.code)")
                  .ValueOrDie();

  Observation test_observation = ValidObservation();

  EvaluationResult contains_result =
      expr.Evaluate(test_observation).ValueOrDie();
  EXPECT_TRUE(contains_result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestFunctionStartsWithInvokedOnNonString) {
  auto expr = CompiledExpression::Compile(Observation::descriptor(),
                                          "code.startsWith('foo')")
                  .ValueOrDie();

  Observation test_observation = ValidObservation();

  EXPECT_FALSE(expr.Evaluate(test_observation).ok());
}

TEST(FhirPathTest, TestFunctionHasValueComplex) {
  auto expr =
      CompiledExpression::Compile(Encounter::descriptor(), "period.hasValue()")
          .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  // hasValue should return false for non-primitive types.
  EXPECT_FALSE(result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestImplies) {
  EXPECT_TRUE(EvaluateBoolExpression("(true implies true) = true"));
  EXPECT_TRUE(EvaluateBoolExpression("(true implies false) = false"));
  EXPECT_TRUE(EvaluateBoolExpression("(true implies {}) = {}"));

  EXPECT_TRUE(EvaluateBoolExpression("(false implies true) = true"));
  EXPECT_TRUE(EvaluateBoolExpression("(false implies false) = true"));
  EXPECT_TRUE(EvaluateBoolExpression("(false implies {}) = true"));

  EXPECT_TRUE(EvaluateBoolExpression("({} implies true) = true"));
  EXPECT_TRUE(EvaluateBoolExpression("({} implies false) = {}"));
  EXPECT_TRUE(EvaluateBoolExpression("({} implies {}) = {}"));
}

TEST(FhirPathTest, TestOrShortCircuit) {
  auto expr =
      CompiledExpression::Compile(Quantity::descriptor(),
                                  "value.hasValue().not() or value < 100")
          .ValueOrDie();
  Quantity quantity;
  EvaluationResult result = expr.Evaluate(quantity).ValueOrDie();
  EXPECT_TRUE(result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestMultiOrShortCircuit) {
  auto expr =
      CompiledExpression::Compile(
          Period::descriptor(),
          "start.hasValue().not() or end.hasValue().not() or start <= end")
          .ValueOrDie();

  Period no_end_period = ParseFromString<Period>(R"proto(
    start: { value_us: 1556750000000 timezone: "America/Los_Angeles" }
  )proto");

  EvaluationResult result = expr.Evaluate(no_end_period).ValueOrDie();

  EXPECT_TRUE(result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestOrFalseWithEmptyReturnsEmpty) {
  auto expr = CompiledExpression::Compile(Quantity::descriptor(),
                                          "value.hasValue() or value < 100")
                  .ValueOrDie();
  Quantity quantity;
  EvaluationResult result = expr.Evaluate(quantity).ValueOrDie();
  EXPECT_TRUE(result.GetMessages().empty());
}

TEST(FhirPathTest, TestOrOneIsTrue) {
  auto expr = CompiledExpression::Compile(
                  Encounter::descriptor(),
                  "period.start.exists() or period.end.exists()")
                  .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  EXPECT_TRUE(result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestOrNeitherAreTrue) {
  auto expr = CompiledExpression::Compile(
                  Encounter::descriptor(),
                  "hospitalization.exists() or location.exists()")
                  .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  EXPECT_FALSE(result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestAndShortCircuit) {
  auto expr = CompiledExpression::Compile(Quantity::descriptor(),
                                          "value.hasValue() and value < 100")
                  .ValueOrDie();
  Quantity quantity;
  EvaluationResult result = expr.Evaluate(quantity).ValueOrDie();
  EXPECT_FALSE(result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestAndTrueWithEmptyReturnsEmpty) {
  auto expr =
      CompiledExpression::Compile(Quantity::descriptor(),
                                  "value.hasValue().not() and value < 100")
          .ValueOrDie();
  Quantity quantity;
  EvaluationResult result = expr.Evaluate(quantity).ValueOrDie();
  EXPECT_TRUE(result.GetMessages().empty());
}

TEST(FhirPathTest, TestAndOneIsTrue) {
  auto expr = CompiledExpression::Compile(
                  Encounter::descriptor(),
                  "period.start.exists() and period.end.exists()")
                  .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  EXPECT_FALSE(result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestAndBothAreTrue) {
  auto expr =
      CompiledExpression::Compile(Encounter::descriptor(),
                                  "period.start.exists() and status.exists()")
          .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  EXPECT_TRUE(result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestEmptyLiteral) {
  EvaluationResult result = EvaluateExpressionWithStatus("{}").ValueOrDie();

  EXPECT_EQ(0, result.GetMessages().size());
}

TEST(FhirPathTest, TestBooleanLiteral) {
  EXPECT_TRUE(EvaluateBoolExpression("true"));
  EXPECT_FALSE(EvaluateBoolExpression("false"));
}

TEST(FhirPathTest, TestIntegerLiteral) {
  auto expr =
      CompiledExpression::Compile(Encounter::descriptor(), "42").ValueOrDie();

  Encounter test_encounter = ValidEncounter();
  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();
  EXPECT_EQ(42, result.GetInteger().ValueOrDie());

  // Ensure evaluation of an out-of-range literal fails.
  const char* overflow_value = "10000000000";
  Status bad_int_status =
      CompiledExpression::Compile(Encounter::descriptor(), overflow_value)
          .status();

  EXPECT_FALSE(bad_int_status.ok());
  // Failure message should contain the bad string.
  EXPECT_TRUE(bad_int_status.error_message().find(overflow_value) !=
              std::string::npos);
}

TEST(FhirPathTest, TestIntegerComparisons) {
  EXPECT_TRUE(EvaluateBoolExpression("42 = 42"));
  EXPECT_FALSE(EvaluateBoolExpression("42 = 43"));

  EXPECT_TRUE(EvaluateBoolExpression("42 != 43"));
  EXPECT_FALSE(EvaluateBoolExpression("42 != 42"));

  EXPECT_TRUE(EvaluateBoolExpression("42 < 43"));
  EXPECT_FALSE(EvaluateBoolExpression("42 < 42"));

  EXPECT_TRUE(EvaluateBoolExpression("43 > 42"));
  EXPECT_FALSE(EvaluateBoolExpression("42 > 42"));

  EXPECT_TRUE(EvaluateBoolExpression("42 >= 42"));
  EXPECT_TRUE(EvaluateBoolExpression("43 >= 42"));
  EXPECT_FALSE(EvaluateBoolExpression("42 >= 43"));

  EXPECT_TRUE(EvaluateBoolExpression("42 <= 42"));
  EXPECT_TRUE(EvaluateBoolExpression("42 <= 43"));
  EXPECT_FALSE(EvaluateBoolExpression("43 <= 42"));
}

TEST(FhirPathTest, TestDecimalLiteral) {
  auto expr =
      CompiledExpression::Compile(Encounter::descriptor(), "1.25").ValueOrDie();

  Encounter test_encounter = ValidEncounter();
  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  EXPECT_EQ("1.25", result.GetDecimal().ValueOrDie());
}

TEST(FhirPathTest, TestDecimalComparisons) {
  EXPECT_TRUE(EvaluateBoolExpression("1.25 = 1.25"));
  EXPECT_FALSE(EvaluateBoolExpression("1.25 = 1.3"));

  EXPECT_TRUE(EvaluateBoolExpression("1.25 != 1.26"));
  EXPECT_FALSE(EvaluateBoolExpression("1.25 != 1.25"));

  EXPECT_TRUE(EvaluateBoolExpression("1.25 < 1.26"));
  EXPECT_TRUE(EvaluateBoolExpression("1 < 1.26"));
  EXPECT_FALSE(EvaluateBoolExpression("1.25 < 1.25"));

  EXPECT_TRUE(EvaluateBoolExpression("1.26 > 1.25"));
  EXPECT_TRUE(EvaluateBoolExpression("1.26 > 1"));
  EXPECT_FALSE(EvaluateBoolExpression("1.25 > 1.25"));

  EXPECT_TRUE(EvaluateBoolExpression("1.25 >= 1.25"));
  EXPECT_TRUE(EvaluateBoolExpression("1.25 >= 1"));
  EXPECT_TRUE(EvaluateBoolExpression("1.26 >= 1.25"));
  EXPECT_FALSE(EvaluateBoolExpression("1.25 >= 1.26"));

  EXPECT_TRUE(EvaluateBoolExpression("1.25 <= 1.25"));
  EXPECT_TRUE(EvaluateBoolExpression("1.25 <= 1.26"));
  EXPECT_FALSE(EvaluateBoolExpression("1.26 <= 1.25"));
  EXPECT_FALSE(EvaluateBoolExpression("1.26 <= 1"));
}

TEST(FhirPathTest, TestStringLiteral) {
  auto expr = CompiledExpression::Compile(Encounter::descriptor(), "'foo'")
                  .ValueOrDie();

  Encounter test_encounter = ValidEncounter();
  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();
  EXPECT_EQ("foo", result.GetString().ValueOrDie());
}

TEST(FhirPathTest, TestStringLiteralEscaping) {
  EXPECT_EQ("\\", EvaluateStringExpressionWithStatus("'\\\\'").ValueOrDie());
  EXPECT_EQ("\f", EvaluateStringExpressionWithStatus("'\\f'").ValueOrDie());
  EXPECT_EQ("\n", EvaluateStringExpressionWithStatus("'\\n'").ValueOrDie());
  EXPECT_EQ("\r", EvaluateStringExpressionWithStatus("'\\r'").ValueOrDie());
  EXPECT_EQ("\t", EvaluateStringExpressionWithStatus("'\\t'").ValueOrDie());
  EXPECT_EQ("\"", EvaluateStringExpressionWithStatus("'\\\"'").ValueOrDie());
  EXPECT_EQ("'", EvaluateStringExpressionWithStatus("'\\\''").ValueOrDie());
  EXPECT_EQ("\t", EvaluateStringExpressionWithStatus("'\\t'").ValueOrDie());
  EXPECT_EQ(" ", EvaluateStringExpressionWithStatus("'\\u0020'").ValueOrDie());

  // Disallowed escape sequences
  EXPECT_FALSE(EvaluateStringExpressionWithStatus("'\\x20'").ok());
  EXPECT_FALSE(EvaluateStringExpressionWithStatus("'\\123'").ok());
  EXPECT_FALSE(EvaluateStringExpressionWithStatus("'\\x20'").ok());
  EXPECT_FALSE(EvaluateStringExpressionWithStatus("'\\x00000020'").ok());
}

TEST(FhirPathTest, TestStringComparisons) {
  EXPECT_TRUE(EvaluateBoolExpression("'foo' = 'foo'"));
  EXPECT_FALSE(EvaluateBoolExpression("'foo' = 'bar'"));

  EXPECT_TRUE(EvaluateBoolExpression("'foo' != 'bar'"));
  EXPECT_FALSE(EvaluateBoolExpression("'foo' != 'foo'"));

  EXPECT_TRUE(EvaluateBoolExpression("'bar' < 'foo'"));
  EXPECT_FALSE(EvaluateBoolExpression("'foo' < 'foo'"));

  EXPECT_TRUE(EvaluateBoolExpression("'foo' > 'bar'"));
  EXPECT_FALSE(EvaluateBoolExpression("'foo' > 'foo'"));

  EXPECT_TRUE(EvaluateBoolExpression("'foo' >= 'foo'"));
  EXPECT_TRUE(EvaluateBoolExpression("'foo' >= 'bar'"));
  EXPECT_FALSE(EvaluateBoolExpression("'bar' >= 'foo'"));

  EXPECT_TRUE(EvaluateBoolExpression("'foo' <= 'foo'"));
  EXPECT_TRUE(EvaluateBoolExpression("'bar' <= 'foo'"));
  EXPECT_FALSE(EvaluateBoolExpression("'foo' <= 'bar'"));
}

TEST(FhirPathTest, ConstraintViolation) {
  Observation observation = ValidObservation();

  // If a range is present it must have a high or low value,
  // so ensure the constraint fails if it doesn't.
  observation.add_reference_range();

  MessageValidator validator;

  auto callback = [&observation](const Message& bad_message,
                                 const FieldDescriptor* field,
                                 const std::string& constraint) {
    // Ensure the expected bad sub-message is passed to the callback.
    EXPECT_EQ(observation.GetDescriptor()->full_name(),
              bad_message.GetDescriptor()->full_name());
    EXPECT_EQ("referenceRange", field->json_name());

    // Ensure the expected constraint failed.
    EXPECT_EQ("low.exists() or high.exists() or text.exists()", constraint);

    return false;
  };

  std::string err_message =
      absl::StrCat("fhirpath-constraint-violation-Observation.referenceRange: ",
                   "\"low.exists() or high.exists() or text.exists()\"");
  EXPECT_EQ(validator.Validate(observation, callback),
            ::tensorflow::errors::FailedPrecondition(err_message));
}

TEST(FhirPathTest, ConstraintSatisfied) {
  Observation observation = ValidObservation();

  // Ensure constraint succeeds with a value in the reference range
  // as required by FHIR.
  auto ref_range = observation.add_reference_range();

  auto value = new ::google::fhir::stu3::proto::Decimal();
  value->set_allocated_value(new std::string("123.45"));

  auto high = new ::google::fhir::stu3::proto::SimpleQuantity();
  high->set_allocated_value(value);

  ref_range->set_allocated_high(high);

  MessageValidator validator;

  EXPECT_TRUE(validator.Validate(observation).ok());
}

TEST(FhirPathTest, NestedConstraintViolated) {
  ValueSet value_set = ValidValueSet();

  auto expansion = new ::google::fhir::stu3::proto::ValueSet_Expansion;

  // Add empty contains structure to violate FHIR constraint.
  expansion->add_contains();
  value_set.set_allocated_expansion(expansion);

  MessageValidator validator;

  std::string err_message = absl::StrCat(
      "fhirpath-constraint-violation-",
      "Expansion.contains: ", "\"code.exists() or display.exists()\"");

  EXPECT_EQ(validator.Validate(value_set),
            ::tensorflow::errors::FailedPrecondition(err_message));
}

TEST(FhirPathTest, NestedConstraintSatisfied) {
  ValueSet value_set = ValidValueSet();

  auto expansion = new ::google::fhir::stu3::proto::ValueSet_Expansion;

  auto contains = expansion->add_contains();

  // Contains struct has value to satisfy FHIR constraint.
  auto proto_string = new ::google::fhir::stu3::proto::String();
  proto_string->set_value("placeholder display");
  contains->set_allocated_display(proto_string);

  auto proto_boolean = new ::google::fhir::stu3::proto::Boolean();
  proto_boolean->set_value(true);
  contains->set_allocated_abstract(proto_boolean);

  value_set.set_allocated_expansion(expansion);

  MessageValidator validator;

  EXPECT_TRUE(validator.Validate(value_set).ok());
}

TEST(FhirPathTest, TimeComparison) {
  auto start_before_end =
      CompiledExpression::Compile(Period::descriptor(), "start <= end")
          .ValueOrDie();

  Period start_before_end_period = ParseFromString<Period>(R"proto(
    start: { value_us: 1556750000000000 timezone: "America/Los_Angeles" }
    end: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
  )proto");
  EvaluationResult start_before_end_result =
      start_before_end.Evaluate(start_before_end_period).ValueOrDie();
  EXPECT_TRUE(start_before_end_result.GetBoolean().ValueOrDie());

  Period end_before_start_period = ParseFromString<Period>(R"proto(
    start: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
    end: { value_us: 1556750000000000 timezone: "America/Los_Angeles" }
  )proto");
  EvaluationResult end_before_start_result =
      start_before_end.Evaluate(end_before_start_period).ValueOrDie();
  EXPECT_FALSE(end_before_start_result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TimeCompareDifferentPrecision) {
  absl::TimeZone zone;
  absl::LoadTimeZone("America/Los_Angeles", &zone);
  auto start_before_end =
      CompiledExpression::Compile(Period::descriptor(), "start <= end")
          .ValueOrDie();

  // Ensure comparison returns false on fine-grained checks but true
  // on corresponding coarse-grained checks.
  EXPECT_FALSE(EvaluateOnPeriod(
      start_before_end,
      ToDateTime(absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                 DateTime::SECOND),
      ToDateTime(absl::CivilDay(2019, 5, 2), zone, DateTime::SECOND)));

  EXPECT_TRUE(EvaluateOnPeriod(
      start_before_end,
      ToDateTime(absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                 DateTime::SECOND),
      ToDateTime(absl::CivilDay(2019, 5, 2), zone, DateTime::DAY)));

  EXPECT_FALSE(EvaluateOnPeriod(
      start_before_end,
      ToDateTime(absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                 DateTime::SECOND),
      ToDateTime(absl::CivilDay(2019, 5, 1), zone, DateTime::DAY)));

  EXPECT_TRUE(EvaluateOnPeriod(
      start_before_end,
      ToDateTime(absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                 DateTime::SECOND),
      ToDateTime(absl::CivilDay(2019, 5, 1), zone, DateTime::MONTH)));

  EXPECT_FALSE(EvaluateOnPeriod(
      start_before_end,
      ToDateTime(absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                 DateTime::SECOND),
      ToDateTime(absl::CivilDay(2019, 1, 1), zone, DateTime::MONTH)));

  EXPECT_TRUE(EvaluateOnPeriod(
      start_before_end,
      ToDateTime(absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                 DateTime::SECOND),
      ToDateTime(absl::CivilDay(2019, 1, 1), zone, DateTime::YEAR)));

  // Test edge case for very high precision comparisons.
  DateTime start_micros;
  start_micros.set_value_us(1556750000000011);
  start_micros.set_timezone("America/Los_Angeles");
  start_micros.set_precision(DateTime::MICROSECOND);

  DateTime end_micros = start_micros;
  EXPECT_TRUE(EvaluateOnPeriod(start_before_end, start_micros, end_micros));

  end_micros.set_value_us(end_micros.value_us() - 1);
  EXPECT_FALSE(EvaluateOnPeriod(start_before_end, start_micros, end_micros));
}

TEST(FhirPathTest, TestCompareEnumToString) {
  auto encounter = ValidEncounter();
  auto is_triaged =
      CompiledExpression::Compile(Encounter::descriptor(), "status = 'triaged'")
          .ValueOrDie();

  EXPECT_TRUE(
      is_triaged.Evaluate(encounter).ValueOrDie().GetBoolean().ValueOrDie());
  encounter.mutable_status()->set_value(
      google::fhir::stu3::proto::EncounterStatusCode::FINISHED);
  EXPECT_FALSE(
      is_triaged.Evaluate(encounter).ValueOrDie().GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, MessageLevelConstraint) {
  Period period = ParseFromString<Period>(R"proto(
    start: { value_us: 1556750000000000 timezone: "America/Los_Angeles" }
    end: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
  )proto");

  MessageValidator validator;
  EXPECT_TRUE(validator.Validate(period).ok());
}

TEST(FhirPathTest, MessageLevelConstraintViolated) {
  Period end_before_start_period = ParseFromString<Period>(R"proto(
    start: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
    end: { value_us: 1556750000000000 timezone: "America/Los_Angeles" }
  )proto");

  MessageValidator validator;
  EXPECT_FALSE(validator.Validate(end_before_start_period).ok());
}

TEST(FhirPathTest, NestedMessageLevelConstraint) {
  auto start_with_no_end_encounter =
      ParseFromString<::google::fhir::r4::core::Encounter>(R"proto(
        status { value: TRIAGED }
        id { value: "123" }
        period {
          start: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
        }
      )proto");

  MessageValidator validator;
  EXPECT_TRUE(validator.Validate(start_with_no_end_encounter).ok());
}

TEST(FhirPathTest, NestedMessageLevelConstraintViolated) {
  auto end_before_start_encounter =
      ParseFromString<::google::fhir::r4::core::Encounter>(R"proto(
        status { value: TRIAGED }
        id { value: "123" }
        period {
          start: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
          end: { value_us: 1556750000000000 timezone: "America/Los_Angeles" }
        }
      )proto");

  MessageValidator validator;
  EXPECT_FALSE(validator.Validate(end_before_start_encounter).ok());
}

TEST(FhirPathTest, ProfiledEmptyExtension) {
  UsCorePatient patient = ValidUsCorePatient();
  MessageValidator validator;
  EXPECT_TRUE(validator.Validate(patient).ok());
}

TEST(FhirPathTest, ProfiledWithExtensions) {
  UsCorePatient patient = ValidUsCorePatient();
  auto race = new google::fhir::stu3::uscore::PatientUSCoreRaceExtension();

  Coding* coding = race->add_omb_category();

  Uri* uri = new Uri();
  uri->set_value("urn:oid:2.16.840.1.113883.6.238");
  coding->set_allocated_system(uri);

  Code* code = new Code();
  code->set_value("1002-5");
  coding->set_allocated_code(code);
  patient.set_allocated_race(race);

  UsCoreBirthSexCode* birth_sex = new UsCoreBirthSexCode();
  birth_sex->set_value(UsCoreBirthSexCode::MALE);
  patient.set_allocated_birthsex(birth_sex);

  MessageValidator validator;
  EXPECT_TRUE(validator.Validate(patient).ok());
}
