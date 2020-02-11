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

// FHIRPath tests specific to STU3
// TODO: many of these are redundant with fhir_path_test.cc
// and should be removed when those tests start to handle multiple version.
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "absl/time/civil_time.h"
#include "absl/time/time.h"
#include "google/fhir/fhir_path/fhir_path.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/stu3/codes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/metadatatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "proto/stu3/uscore.pb.h"
#include "proto/stu3/uscore_codes.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace fhir_path {

namespace {

using stu3::proto::Boolean;
using stu3::proto::Code;
using stu3::proto::CodeableConcept;
using stu3::proto::Decimal;
using stu3::proto::Encounter;
using stu3::proto::Observation;
using stu3::proto::Period;
using stu3::proto::Quantity;
using stu3::proto::SimpleQuantity;
using stu3::proto::String;
using stu3::proto::StructureDefinition;
using stu3::proto::ValueSet;
using stu3::uscore::UsCoreBirthSexCode;
using stu3::uscore::UsCorePatient;

using testutil::EqualsProto;

using ::google::protobuf::Message;
using testing::ElementsAreArray;
using testing::IsEmpty;
using testing::UnorderedElementsAreArray;

static ::google::protobuf::TextFormat::Parser parser;  // NOLINT

MATCHER(EvalsToEmpty, "") {
  return arg.ok() && arg.ValueOrDie().GetMessages().empty();
}

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
    status_history { status { value: ARRIVED } }
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

template <typename T>
StatusOr<EvaluationResult> Evaluate(const T& message,
    const std::string& expression) {
  FHIR_ASSIGN_OR_RETURN(auto compiled_expression,
      CompiledExpression::Compile(message.GetDescriptor(), expression));

  return compiled_expression.Evaluate(message);
}

StatusOr<EvaluationResult> EvaluateExpressionWithStatus(
    const std::string& expression) {
  // FHIRPath assumes a EvaluateBoolExpression object during evaluation, so we
  // use an encounter as a placeholder.
  auto compiled_expression =
      CompiledExpression::Compile(Encounter::descriptor(), expression);
  if (!compiled_expression.ok()) {
    return StatusOr<EvaluationResult>(compiled_expression.status());
  }

  Encounter test_encounter = ValidEncounter();
  return compiled_expression.ValueOrDie().Evaluate(test_encounter);
}

StatusOr<EvaluationResult> Evaluate(const std::string& expression) {
  return EvaluateExpressionWithStatus(expression);
}

StatusOr<std::string> EvaluateStringExpressionWithStatus(
    const std::string& expression) {
  StatusOr<EvaluationResult> result = EvaluateExpressionWithStatus(expression);

  if (!result.ok()) {
    return StatusOr<std::string>(result.status());
  }

  return result.ValueOrDie().GetString();
}

StatusOr<bool> EvaluateBoolExpressionWithStatus(const std::string& expression) {
  StatusOr<EvaluationResult> result = EvaluateExpressionWithStatus(expression);

  if (!result.ok()) {
    return StatusOr<bool>(result.status());
  }

  return result.ValueOrDie().GetBoolean();
}

bool EvaluateBoolExpression(const std::string& expression) {
  return EvaluateBoolExpressionWithStatus(expression).ValueOrDie();
}

TEST(FhirPathTest, TestExternalConstantsContext) {
  Encounter test_encounter = ValidEncounter();

  auto result = CompiledExpression::Compile(Encounter::descriptor(), "%context")
                    .ValueOrDie()
                    .Evaluate(test_encounter)
                    .ValueOrDie();
  EXPECT_THAT(result.GetMessages(),
              UnorderedElementsAreArray({EqualsProto(test_encounter)}));
}

TEST(FhirPathTest, TestExternalConstantsContextReferenceInExpressionParam) {
  Encounter test_encounter = ValidEncounter();

  auto result = CompiledExpression::Compile(Encounter::descriptor(),
                                            "status.select(%context)")
                    .ValueOrDie()
                    .Evaluate(test_encounter)
                    .ValueOrDie();
  EXPECT_THAT(result.GetMessages(),
              UnorderedElementsAreArray({EqualsProto(test_encounter)}));
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

  EXPECT_THAT(*status, EqualsProto(test_encounter.status()));
}

TEST(FhirPathTest, TestGetGrandchild) {
  auto expr =
      CompiledExpression::Compile(Encounter::descriptor(), "period.start")
          .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  ASSERT_EQ(1, result.GetMessages().size());

  const Message* status = result.GetMessages()[0];

  EXPECT_THAT(*status, EqualsProto(test_encounter.period().start()));
}

TEST(FhirPathTest, TestGetEmptyGrandchild) {
  auto expr = CompiledExpression::Compile(Encounter::descriptor(), "period.end")
                  .ValueOrDie();

  Encounter test_encounter = ValidEncounter();

  EvaluationResult result = expr.Evaluate(test_encounter).ValueOrDie();

  EXPECT_EQ(0, result.GetMessages().size());
}

TEST(FhirPathTest, TestFieldExists) {
  Encounter test_encounter = ValidEncounter();
  test_encounter.mutable_class_value()->mutable_display()->set_value("foo");

  auto root_expr =
      CompiledExpression::Compile(Encounter::descriptor(), "period")
          .ValueOrDie();
  EvaluationResult root_result =
      root_expr.Evaluate(test_encounter).ValueOrDie();
  EXPECT_THAT(
      root_result.GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.period())}));

  // Tests the conversion from camelCase to snake_case
  auto camel_case_expr =
      CompiledExpression::Compile(Encounter::descriptor(), "statusHistory")
          .ValueOrDie();
  EvaluationResult camel_case_result =
      camel_case_expr.Evaluate(test_encounter).ValueOrDie();
  EXPECT_THAT(camel_case_result.GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(test_encounter.status_history(0))}));

  // Test that the json_name field annotation is used when searching for a
  // field.
  auto json_name_alias_expr =
      CompiledExpression::Compile(Encounter::descriptor(), "class")
          .ValueOrDie();
  EvaluationResult json_name_alias_result =
      json_name_alias_expr.Evaluate(test_encounter).ValueOrDie();
  EXPECT_THAT(
      json_name_alias_result.GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.class_value())}));
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

  EXPECT_THAT(Evaluate(ValidEncounter(), "(period | status).boguschildfield"),
              EvalsToEmpty());
}

TEST(FhirPathTest, TestNoSuchFunction) {
  auto root_expr = CompiledExpression::Compile(Encounter::descriptor(),
                                               "period.bogusfunction()");

  EXPECT_FALSE(root_expr.ok());
  EXPECT_NE(root_expr.status().error_message().find("bogusfunction"),
            std::string::npos);
}

TEST(FhirPathTest, TestFunctionTopLevelInvocation) {
  EXPECT_TRUE(EvaluateBoolExpression("exists()"));
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

TEST(FhirPathTest, TestFunctionChildren) {
  StructureDefinition structure_definition =
      ParseFromString<StructureDefinition>(R"proto(
        name {value: "foo"}
        context_invariant {value: "bar"}
        snapshot {element {label {value: "snapshot"}}}
        differential {element {label {value: "differential"}}}
      )proto");

  EXPECT_THAT(
      Evaluate(structure_definition, "children()").ValueOrDie().GetMessages(),
      UnorderedElementsAreArray(
          {EqualsProto(structure_definition.name()),
           EqualsProto(structure_definition.context_invariant(0)),
           EqualsProto(structure_definition.snapshot()),
           EqualsProto(structure_definition.differential())}));

  EXPECT_THAT(
      Evaluate(structure_definition, "children().element")
          .ValueOrDie().GetMessages(),
      UnorderedElementsAreArray(
          {EqualsProto(structure_definition.snapshot().element(0)),
           EqualsProto(structure_definition.differential().element(0))}));
}

TEST(FhirPathTest, TestFunctionContains) {
  // Wrong number and/or types of arguments.
  EXPECT_FALSE(EvaluateBoolExpressionWithStatus("'foo'.contains()").ok());
  EXPECT_FALSE(EvaluateBoolExpressionWithStatus("'foo'.contains(1)").ok());
  EXPECT_FALSE(
      EvaluateBoolExpressionWithStatus("'foo'.contains('a', 'b')").ok());

  EXPECT_TRUE(EvaluateBoolExpression("'foo'.contains('')"));
  EXPECT_TRUE(EvaluateBoolExpression("'foo'.contains('o')"));
  EXPECT_TRUE(EvaluateBoolExpression("'foo'.contains('foo')"));
  EXPECT_FALSE(EvaluateBoolExpression("'foo'.contains('foob')"));
  EXPECT_TRUE(EvaluateBoolExpression("''.contains('')"));
  EXPECT_FALSE(EvaluateBoolExpression("''.contains('foo')"));

  EXPECT_THAT(Evaluate("{}.contains('foo')"), EvalsToEmpty());
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

TEST(FhirPathTest, TestFunctionMatches) {
  EXPECT_THAT(Evaluate("{}.matches('')"), EvalsToEmpty());
  EXPECT_TRUE(EvaluateBoolExpression("''.matches('')"));
  EXPECT_TRUE(EvaluateBoolExpression("'a'.matches('a')"));
  EXPECT_FALSE(EvaluateBoolExpression("'abc'.matches('a')"));
  EXPECT_TRUE(EvaluateBoolExpression("'abc'.matches('...')"));
}

TEST(FhirPathTest, TestFunctionLength) {
  EXPECT_THAT(Evaluate("{}.length()"), EvalsToEmpty());
  EXPECT_TRUE(EvaluateBoolExpression("''.length() = 0"));
  EXPECT_TRUE(EvaluateBoolExpression("'abc'.length() = 3"));

  EXPECT_FALSE(EvaluateExpressionWithStatus("3.length()").ok());
}

TEST(FhirPathTest, TestFunctionToInteger) {
  EXPECT_EQ(EvaluateExpressionWithStatus("1.toInteger()")
                .ValueOrDie()
                .GetInteger()
                .ValueOrDie(),
            1);
  EXPECT_EQ(EvaluateExpressionWithStatus("'2'.toInteger()")
                .ValueOrDie()
                .GetInteger()
                .ValueOrDie(),
            2);

  EXPECT_TRUE(EvaluateExpressionWithStatus("(3.3).toInteger()")
                  .ValueOrDie()
                  .GetMessages()
                  .empty());
  EXPECT_TRUE(EvaluateExpressionWithStatus("'a'.toInteger()")
                  .ValueOrDie()
                  .GetMessages()
                  .empty());

  EXPECT_FALSE(EvaluateExpressionWithStatus("(1 | 2).toInteger()").ok());
}

TEST(FhirPathTest, TestFunctionToString) {
  EXPECT_EQ(EvaluateStringExpressionWithStatus("1.toString()").ValueOrDie(),
            "1");
  EXPECT_EQ(EvaluateStringExpressionWithStatus("1.1.toString()").ValueOrDie(),
            "1.1");
  EXPECT_EQ(EvaluateStringExpressionWithStatus("'foo'.toString()").ValueOrDie(),
            "foo");
  EXPECT_EQ(EvaluateStringExpressionWithStatus("true.toString()").ValueOrDie(),
            "true");
  EXPECT_THAT(
      EvaluateExpressionWithStatus("{}.toString()").ValueOrDie().GetMessages(),
      IsEmpty());
  EXPECT_THAT(
      EvaluateExpressionWithStatus("toString()").ValueOrDie().GetMessages(),
      IsEmpty());
  EXPECT_FALSE(EvaluateExpressionWithStatus("(1 | 2).toString()").ok());
}

TEST(FhirPathTest, TestFunctionTrace) {
  EXPECT_TRUE(EvaluateBoolExpression("true.trace('debug')"));
  EXPECT_TRUE(EvaluateExpressionWithStatus("{}.trace('debug')")
                  .ValueOrDie()
                  .GetMessages()
                  .empty());
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

TEST(FhirPathTest, TestFunctionEmpty) {
  EXPECT_TRUE(EvaluateBoolExpression("{}.empty()"));
  EXPECT_FALSE(EvaluateBoolExpression("true.empty()"));
  EXPECT_FALSE(EvaluateBoolExpression("(false | true).empty()"));
}

TEST(FhirPathTest, TestFunctionCount) {
  EXPECT_EQ(EvaluateExpressionWithStatus("{}.count()")
                .ValueOrDie()
                .GetInteger()
                .ValueOrDie(),
            0);
  EXPECT_EQ(EvaluateExpressionWithStatus("'a'.count()")
                .ValueOrDie()
                .GetInteger()
                .ValueOrDie(),
            1);
  EXPECT_EQ(EvaluateExpressionWithStatus("('a' | 1).count()")
                .ValueOrDie()
                .GetInteger()
                .ValueOrDie(),
            2);
}

TEST(FhirPathTest, TestFunctionFirst) {
  EXPECT_THAT(Evaluate("{}.first()"), EvalsToEmpty());
  EXPECT_TRUE(EvaluateBoolExpression("true.first()"));
  EXPECT_TRUE(EvaluateExpressionWithStatus("(false | true).first()").ok());
}

TEST(FhirPathTest, TestFunctionTail) {
  EXPECT_THAT(
      EvaluateExpressionWithStatus("{}.tail()").ValueOrDie().GetMessages(),
      IsEmpty());

  EXPECT_THAT(
      EvaluateExpressionWithStatus("true.tail()").ValueOrDie().GetMessages(),
      IsEmpty());

  EXPECT_TRUE(EvaluateBoolExpression("true.combine(true).tail()"));
}

TEST(FhirPathTest, TestFunctionIsPrimitives) {
  EXPECT_THAT(
      EvaluateExpressionWithStatus("{}.is(Boolean)").ValueOrDie().GetMessages(),
      IsEmpty());

  EXPECT_TRUE(EvaluateBoolExpression("true.is(Boolean)"));
  EXPECT_FALSE(EvaluateBoolExpression("true.is(Decimal)"));
  EXPECT_FALSE(EvaluateBoolExpression("true.is(Integer)"));

  EXPECT_TRUE(EvaluateBoolExpression("1.is(Integer)"));
  EXPECT_FALSE(EvaluateBoolExpression("1.is(Decimal)"));
  EXPECT_FALSE(EvaluateBoolExpression("1.is(Boolean)"));

  EXPECT_TRUE(EvaluateBoolExpression("1.1.is(Decimal)"));
  EXPECT_FALSE(EvaluateBoolExpression("1.1.is(Integer)"));
  EXPECT_FALSE(EvaluateBoolExpression("1.1.is(Boolean)"));
}

TEST(FhirPathTest, TestFunctionIsResources) {
  Observation observation = ParseFromString<Observation>(R"proto()proto");

  EvaluationResult is_boolean_evaluation_result =
      CompiledExpression::Compile(CodeableConcept::descriptor(),
                                  "$this.is(Boolean)")
          .ValueOrDie()
          .Evaluate(observation)
          .ValueOrDie();
  EXPECT_FALSE(is_boolean_evaluation_result.GetBoolean().ValueOrDie());

  EvaluationResult is_codeable_concept_evaluation_result =
      CompiledExpression::Compile(CodeableConcept::descriptor(),
                                  "$this.is(CodeableConcept)")
          .ValueOrDie()
          .Evaluate(observation)
          .ValueOrDie();
  EXPECT_FALSE(is_codeable_concept_evaluation_result.GetBoolean().ValueOrDie());

  EvaluationResult is_observation_evaluation_result =
      CompiledExpression::Compile(CodeableConcept::descriptor(),
                                  "$this.is(Observation)")
          .ValueOrDie()
          .Evaluate(observation)
          .ValueOrDie();
  EXPECT_TRUE(is_observation_evaluation_result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestOperatorIsPrimitives) {
  EXPECT_THAT(
      EvaluateExpressionWithStatus("{} is Boolean").ValueOrDie().GetMessages(),
      IsEmpty());

  EXPECT_TRUE(EvaluateBoolExpression("true is Boolean"));
  EXPECT_FALSE(EvaluateBoolExpression("true is Decimal"));
  EXPECT_FALSE(EvaluateBoolExpression("true is Integer"));

  EXPECT_TRUE(EvaluateBoolExpression("1 is Integer"));
  EXPECT_FALSE(EvaluateBoolExpression("1 is Decimal"));
  EXPECT_FALSE(EvaluateBoolExpression("1 is Boolean"));

  EXPECT_TRUE(EvaluateBoolExpression("1.1 is Decimal"));
  EXPECT_FALSE(EvaluateBoolExpression("1.1 is Integer"));
  EXPECT_FALSE(EvaluateBoolExpression("1.1 is Boolean"));
}

TEST(FhirPathTest, TestOperatorIsResources) {
  Observation observation = ParseFromString<Observation>(R"proto()proto");

  EvaluationResult is_boolean_evaluation_result =
      CompiledExpression::Compile(CodeableConcept::descriptor(),
                                  "$this is Boolean")
          .ValueOrDie()
          .Evaluate(observation)
          .ValueOrDie();
  EXPECT_FALSE(is_boolean_evaluation_result.GetBoolean().ValueOrDie());

  EvaluationResult is_codeable_concept_evaluation_result =
      CompiledExpression::Compile(CodeableConcept::descriptor(),
                                  "$this is CodeableConcept")
          .ValueOrDie()
          .Evaluate(observation)
          .ValueOrDie();
  EXPECT_FALSE(is_codeable_concept_evaluation_result.GetBoolean().ValueOrDie());

  EvaluationResult is_observation_evaluation_result =
      CompiledExpression::Compile(CodeableConcept::descriptor(),
                                  "$this is Observation")
          .ValueOrDie()
          .Evaluate(observation)
          .ValueOrDie();
  EXPECT_TRUE(is_observation_evaluation_result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestFunctionTailMaintainsOrder) {
  CodeableConcept observation = ParseFromString<CodeableConcept>(R"proto(
    coding {
      system { value: "foo" }
      code { value: "abc" }
    }
    coding {
      system { value: "bar" }
      code { value: "def" }
    }
    coding {
      system { value: "foo" }
      code { value: "ghi" }
    }
  )proto");

  Code code_def = ParseFromString<Code>("value: 'def'");
  Code code_ghi = ParseFromString<Code>("value: 'ghi'");
  EvaluationResult evaluation_result =
      CompiledExpression::Compile(CodeableConcept::descriptor(),
                                  "coding.tail().code")
          .ValueOrDie()
          .Evaluate(observation)
          .ValueOrDie();
  EXPECT_THAT(evaluation_result.GetMessages(),
              ElementsAreArray({EqualsProto(code_def), EqualsProto(code_ghi)}));
}

TEST(FhirPathTest, TestUnion) {
  EXPECT_THAT(Evaluate("({} | {})"), EvalsToEmpty());

  EXPECT_TRUE(EvaluateBoolExpression("(true | {}) = true"));
  EXPECT_TRUE(EvaluateBoolExpression("(true | true) = true"));

  EXPECT_TRUE(EvaluateBoolExpression("(false | {}) = false"));
  EXPECT_TRUE(EvaluateBoolExpression("(false | false) = false"));
}

TEST(FhirPathTest, TestUnionDeduplicationObjects) {
  Encounter test_encounter = ValidEncounter();

  EvaluationResult evaluation_result =
      CompiledExpression::Compile(Encounter::descriptor(),
                                  ("period | status | status | period"))
          .ValueOrDie()
          .Evaluate(test_encounter)
          .ValueOrDie();
  std::vector<const Message*> result = evaluation_result.GetMessages();

  ASSERT_THAT(result, UnorderedElementsAreArray(
                          {EqualsProto(test_encounter.status()),
                           EqualsProto(test_encounter.period())}));
}

TEST(FhirPathTest, TestIsDistinct) {
  EXPECT_TRUE(EvaluateBoolExpression("{}.isDistinct()"));
  EXPECT_TRUE(EvaluateBoolExpression("true.isDistinct()"));
  EXPECT_TRUE(EvaluateBoolExpression("(true | false).isDistinct()"));

  EXPECT_FALSE(EvaluateBoolExpression("true.combine(true).isDistinct()"));
}

TEST(FhirPathTest, TestIndexer) {
  EXPECT_TRUE(EvaluateBoolExpression("true[0] = true"));
  EXPECT_THAT(Evaluate("true[1]"), EvalsToEmpty());
  EXPECT_TRUE(EvaluateBoolExpression("false[0] = false"));
  EXPECT_THAT(Evaluate("false[1]"), EvalsToEmpty());

  EXPECT_FALSE(EvaluateBoolExpressionWithStatus("true['foo']").ok());
  EXPECT_FALSE(EvaluateBoolExpressionWithStatus("true[(1 | 2)]").ok());
}

TEST(FhirPathTest, TestContains) {
  EXPECT_TRUE(EvaluateBoolExpression("true contains true"));
  EXPECT_TRUE(EvaluateBoolExpression("(false | true) contains true"));

  EXPECT_FALSE(EvaluateBoolExpression("true contains false"));
  EXPECT_FALSE(EvaluateBoolExpression("(false | true) contains 1"));
  EXPECT_FALSE(EvaluateBoolExpression("{} contains true"));

  EXPECT_THAT(Evaluate("({} contains {})"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("(true contains {})"), EvalsToEmpty());

  EXPECT_FALSE(
      EvaluateBoolExpressionWithStatus("{} contains (true | false)").ok());
  EXPECT_FALSE(
      EvaluateBoolExpressionWithStatus("true contains (true | false)").ok());
}

TEST(FhirPathTest, TestIn) {
  EXPECT_TRUE(EvaluateBoolExpression("true in true"));
  EXPECT_TRUE(EvaluateBoolExpression("true in (false | true)"));

  EXPECT_FALSE(EvaluateBoolExpression("false in true"));
  EXPECT_FALSE(EvaluateBoolExpression("1 in (false | true)"));
  EXPECT_FALSE(EvaluateBoolExpression("true in {}"));

  EXPECT_THAT(Evaluate("({} in {})"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("({} in true)"), EvalsToEmpty());

  EXPECT_FALSE(EvaluateBoolExpressionWithStatus("(true | false) in {}").ok());
  EXPECT_FALSE(EvaluateBoolExpressionWithStatus("(true | false) in {}").ok());
}

TEST(FhirPathTest, TestImplies) {
  EXPECT_TRUE(EvaluateBoolExpression("(true implies true) = true"));
  EXPECT_TRUE(EvaluateBoolExpression("(true implies false) = false"));
  EXPECT_THAT(Evaluate("(true implies {})"), EvalsToEmpty());

  EXPECT_TRUE(EvaluateBoolExpression("(false implies true) = true"));
  EXPECT_TRUE(EvaluateBoolExpression("(false implies false) = true"));
  EXPECT_TRUE(EvaluateBoolExpression("(false implies {}) = true"));

  EXPECT_TRUE(EvaluateBoolExpression("({} implies true) = true"));
  EXPECT_THAT(Evaluate("({} implies false)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("({} implies {})"), EvalsToEmpty());
}

TEST(FhirPathTest, TestWhere) {
  CodeableConcept observation = ParseFromString<CodeableConcept>(R"proto(
    coding {
      system { value: "foo" }
      code { value: "abc" }
    }
    coding {
      system { value: "bar" }
      code { value: "def" }
    }
    coding {
      system { value: "foo" }
      code { value: "ghi" }
    }
  )proto");

  Code code_abc = ParseFromString<Code>("value: 'abc'");
  Code code_ghi = ParseFromString<Code>("value: 'ghi'");
  EvaluationResult evaluation_result =
      CompiledExpression::Compile(CodeableConcept::descriptor(),
                                  "coding.where(system = 'foo').code")
          .ValueOrDie()
          .Evaluate(observation)
          .ValueOrDie();
  EXPECT_THAT(evaluation_result.GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(code_abc), EqualsProto(code_ghi)}));
}

TEST(FhirPathTest, TestWhereNoMatches) {
  EXPECT_THAT(Evaluate("('a' | 'b' | 'c').where(false)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("{}.where(true)"), EvalsToEmpty());
}

TEST(FhirPathTest, TestWhereValidatesArguments) {
  EXPECT_FALSE(EvaluateExpressionWithStatus("{}.where()").ok());
  EXPECT_TRUE(EvaluateExpressionWithStatus("{}.where(true)").ok());
  EXPECT_FALSE(EvaluateExpressionWithStatus("{}.where(true, false)").ok());
}

TEST(FhirPathTest, TestAll) {
  EXPECT_TRUE(EvaluateBoolExpression("{}.all(false)"));
  EXPECT_TRUE(EvaluateBoolExpression("(false).all(true)"));
  EXPECT_TRUE(EvaluateBoolExpression("(1 | 2 | 3).all($this < 4)"));
  EXPECT_FALSE(EvaluateBoolExpression("(1 | 2 | 3).all($this > 4)"));

  // Verify that all() fails when called with the wrong number of arguments.
  EXPECT_FALSE(EvaluateExpressionWithStatus("{}.all()").ok());
  EXPECT_FALSE(EvaluateExpressionWithStatus("{}.all(true, false)").ok());
}

TEST(FhirPathTest, TestAllReadsFieldFromDifferingTypes) {
  StructureDefinition structure_definition =
      ParseFromString<StructureDefinition>(R"proto(
        snapshot { element {} }
        differential { element {} }
      )proto");

  EvaluationResult evaluation_result =
      CompiledExpression::Compile(
          StructureDefinition::descriptor(),
          "(snapshot | differential).all(element.exists())")
          .ValueOrDie()
          .Evaluate(structure_definition)
          .ValueOrDie();
  EXPECT_TRUE(evaluation_result.GetBoolean().ValueOrDie());
}

TEST(FhirPathTest, TestSelectEmptyResult) {
  EXPECT_THAT(Evaluate("{}.where(true)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("(1 | 2 | 3).where(false)"), EvalsToEmpty());
}

TEST(FhirPathTest, TestSelectValidatesArguments) {
  EXPECT_FALSE(EvaluateExpressionWithStatus("{}.select()").ok());
  EXPECT_TRUE(EvaluateExpressionWithStatus("{}.select(true)").ok());
  EXPECT_FALSE(EvaluateExpressionWithStatus("{}.select(true, false)").ok());
}

TEST(FhirPathTest, TestIif) {
  // 2 parameter invocations
  EXPECT_EQ(EvaluateExpressionWithStatus("iif(true, 1)")
                .ValueOrDie()
                .GetInteger()
                .ValueOrDie(),
            1);
  EXPECT_THAT(
      EvaluateExpressionWithStatus("iif(false, 1)").ValueOrDie().GetMessages(),
      IsEmpty());
  EXPECT_THAT(
      EvaluateExpressionWithStatus("iif({}, 1)").ValueOrDie().GetMessages(),
      IsEmpty());

  // 3 parameters invocations
  EXPECT_EQ(EvaluateExpressionWithStatus("iif(true, 1, 2)")
                .ValueOrDie()
                .GetInteger()
                .ValueOrDie(),
            1);
  EXPECT_EQ(EvaluateExpressionWithStatus("iif(false, 1, 2)")
                .ValueOrDie()
                .GetInteger()
                .ValueOrDie(),
            2);
  EXPECT_EQ(EvaluateExpressionWithStatus("iif({}, 1, 2)")
                .ValueOrDie()
                .GetInteger()
                .ValueOrDie(),
            2);

  EXPECT_THAT(EvaluateExpressionWithStatus("{}.iif(true, false)")
                  .ValueOrDie()
                  .GetMessages(),
              IsEmpty());
  EXPECT_FALSE(EvaluateExpressionWithStatus("(1 | 2).iif(true, false)").ok());
}

TEST(FhirPathTest, TestIifValidatesArguments) {
  EXPECT_FALSE(EvaluateExpressionWithStatus("{}.iif()").ok());
  EXPECT_FALSE(EvaluateExpressionWithStatus("{}.iif(true)").ok());
  EXPECT_FALSE(
      EvaluateExpressionWithStatus("{}.iif(true, false, true, false)").ok());
}

TEST(FhirPathTest, TestXor) {
  EXPECT_TRUE(EvaluateBoolExpression("(true xor true) = false"));
  EXPECT_TRUE(EvaluateBoolExpression("(true xor false) = true"));
  EXPECT_THAT(Evaluate("(true xor {})"), EvalsToEmpty());

  EXPECT_TRUE(EvaluateBoolExpression("(false xor true) = true"));
  EXPECT_TRUE(EvaluateBoolExpression("(false xor false) = false"));
  EXPECT_THAT(Evaluate("(false xor {})"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("({} xor true)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("({} xor false)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("({} xor {})"), EvalsToEmpty());
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

TEST(FhirPathTest, TestPolarityOperator) {
  EXPECT_TRUE(EvaluateBoolExpression("+1 = 1"));
  EXPECT_TRUE(EvaluateBoolExpression("-(+1) = -1"));
  EXPECT_TRUE(EvaluateBoolExpression("+(-1) = -1"));
  EXPECT_TRUE(EvaluateBoolExpression("-(-1) = 1"));

  EXPECT_TRUE(EvaluateBoolExpression("+1.2 = 1.2"));
  EXPECT_TRUE(EvaluateBoolExpression("-(+1.2) = -1.2"));
  EXPECT_TRUE(EvaluateBoolExpression("+(-1.2) = -1.2"));
  EXPECT_TRUE(EvaluateBoolExpression("-(-1.2) = 1.2"));

  EXPECT_THAT(Evaluate("+{}"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("-{}"), EvalsToEmpty());

  EXPECT_FALSE(EvaluateExpressionWithStatus("+(1 | 2)").ok());
}

TEST(FhirPathTest, TestIntegerAddition) {
  EXPECT_TRUE(EvaluateBoolExpression("(2 + 3) = 5"));
  EXPECT_THAT(Evaluate("({} + 3)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("(2 + {})"), EvalsToEmpty());
}

TEST(FhirPathTest, TestStringAddition) {
  EXPECT_TRUE(EvaluateBoolExpression("('foo' + 'bar') = 'foobar'"));
  EXPECT_THAT(Evaluate("({} + 'bar')"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("('foo' + {})"), EvalsToEmpty());
}

TEST(FhirPathTest, TestStringConcatenation) {
  EXPECT_EQ(EvaluateStringExpressionWithStatus("('foo' & 'bar')").ValueOrDie(),
            "foobar");
  EXPECT_EQ(EvaluateStringExpressionWithStatus("{} & 'bar'").ValueOrDie(),
            "bar");
  EXPECT_EQ(EvaluateStringExpressionWithStatus("'foo' & {}").ValueOrDie(),
            "foo");
  EXPECT_EQ(EvaluateStringExpressionWithStatus("{} & {}").ValueOrDie(), "");
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

TEST(FhirPathTest, ConstraintSatisfied) {
  Observation observation = ValidObservation();

  // Ensure constraint succeeds with a value in the reference range
  // as required by FHIR.
  auto ref_range = observation.add_reference_range();

  auto value = new Decimal();
  value->set_allocated_value(new std::string("123.45"));

  auto high = new SimpleQuantity();
  high->set_allocated_value(value);

  ref_range->set_allocated_high(high);

  MessageValidator validator;

  EXPECT_TRUE(validator.Validate(observation).ok());
}

TEST(FhirPathTest, NestedConstraintSatisfied) {
  ValueSet value_set = ValidValueSet();
  value_set.mutable_name()->set_value("Placeholder");

  auto expansion = value_set.mutable_expansion();
  auto contains = expansion->add_contains();

  // Contains struct has value to satisfy FHIR constraint.
  contains->mutable_display()->set_value("Placeholder value");
  contains->mutable_abstract()->set_value(true);

  MessageValidator validator;

  FHIR_ASSERT_OK(validator.Validate(value_set));
}

TEST(FhirPathTest, TestCompareEnumToString) {
  auto encounter = ValidEncounter();
  auto is_triaged =
      CompiledExpression::Compile(Encounter::descriptor(), "status = 'triaged'")
          .ValueOrDie();

  FHIR_ASSERT_OK_AND_ASSIGN(auto eval_result, is_triaged.Evaluate(encounter));
  FHIR_ASSERT_OK_AND_CONTAINS(true, eval_result.GetBoolean());
  encounter.mutable_status()->set_value(
      stu3::proto::EncounterStatusCode::FINISHED);
  FHIR_ASSERT_OK_AND_CONTAINS(
      false, is_triaged.Evaluate(encounter).ValueOrDie().GetBoolean());
}

TEST(FhirPathTest, MessageLevelConstraint) {
  Period period = ParseFromString<Period>(R"proto(
    start: { value_us: 1556750000000000 timezone: "America/Los_Angeles" }
    end: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
  )proto");

  MessageValidator validator;
  EXPECT_TRUE(validator.Validate(period).ok());
}

TEST(FhirPathTest, NestedMessageLevelConstraint) {
  auto start_with_no_end_encounter = ParseFromString<Encounter>(R"proto(
    status { value: TRIAGED }
    id { value: "123" }
    period {
      start: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
    }
  )proto");

  MessageValidator validator;
  FHIR_ASSERT_OK(validator.Validate(start_with_no_end_encounter));
}

TEST(FhirPathTest, ProfiledEmptyExtension) {
  UsCorePatient patient = ValidUsCorePatient();
  MessageValidator validator;
  FHIR_ASSERT_OK(validator.Validate(patient));
}

TEST(FhirPathTest, ProfiledWithExtensions) {
  UsCorePatient patient = ValidUsCorePatient();
  auto race = new stu3::uscore::PatientUSCoreRaceExtension();

  stu3::proto::Coding* coding = race->add_omb_category();
  coding->mutable_code()->set_value("urn:oid:2.16.840.1.113883.6.238");
  coding->mutable_code()->set_value("1002-5");
  patient.set_allocated_race(race);

  patient.mutable_birthsex()->set_value(UsCoreBirthSexCode::MALE);

  MessageValidator validator;
  FHIR_ASSERT_OK(validator.Validate(patient));
}

}  // namespace

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
