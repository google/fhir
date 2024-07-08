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

#include <stdint.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/any.pb.h"
#include "google/protobuf/util/message_differencer.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/time/civil_time.h"
#include "absl/time/time.h"
#include "google/fhir/annotations.h"
#include "google/fhir/fhir_path/utils.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/r4/primitive_handler.h"
#include "google/fhir/references.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/stu3/primitive_handler.h"
#include "google/fhir/terminology/terminology_resolver.h"
#include "google/fhir/testutil/fhir_test_env.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "google/fhir/util.h"
#include "proto/google/fhir/proto/r4/core/codes.pb.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/encounter.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medication_knowledge.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/observation.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/parameters.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/patient.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/structure_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/value_set.pb.h"
#include "proto/google/fhir/proto/r5/core/codes.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/encounter.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/observation.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/parameters.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/patient.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/structure_definition.pb.h"
#include "proto/google/fhir/proto/stu3/codes.pb.h"
#include "proto/google/fhir/proto/stu3/datatypes.pb.h"
#include "proto/google/fhir/proto/stu3/metadatatypes.pb.h"
#include "proto/google/fhir/proto/stu3/resources.pb.h"
#include "proto/google/fhir/proto/stu3/uscore.pb.h"
#include "proto/google/fhir/proto/stu3/uscore_codes.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"

namespace google {
namespace fhir {
namespace fhir_path {

// Provides a human-readable string representation of an EvaluationResult object
// for googletest.
//
// https://github.com/google/googletest/blob/master/googletest/docs/advanced.md#teaching-googletest-how-to-print-your-values
void PrintTo(const EvaluationResult& result, std::ostream* os) {
  *os << "evaluated to [";
  absl::string_view sep("");
  for (const auto message : result.GetMessages()) {
    *os << sep << message->DebugString();
    sep = ", ";
  }
  *os << "]";
}

void PrintTo(const absl::StatusOr<EvaluationResult>& result, std::ostream* os) {
  if (result.ok()) {
    *os << ::testing::PrintToString(result.value());
  } else {
    *os << "failed to evaluate (" << result.status().code()
        << ") with message \"" << result.status().message() << "\"";
  }
}

namespace {

using ::absl::StatusCode;
using ::google::protobuf::Message;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::StrEq;
using ::testing::UnorderedElementsAreArray;
using testutil::EqualsProto;

static ::google::protobuf::TextFormat::Parser parser;  // NOLINT

MATCHER(EvalsToEmpty, "") {
  if (!arg.ok()) {
    *result_listener << "evaluation error: " << arg.status().message();
    return false;
  }

  std::vector<const Message*> results = arg.value().GetMessages();

  if (!results.empty()) {
    *result_listener << "has size of " << results.size();
    return false;
  }

  return true;
}

// Matcher for absl::StatusOr<EvaluationResult> that checks to see that the
// evaluation succeeded and evaluated to a single boolean with value of false.
//
// NOTE: Not(EvalsToFalse()) is not the same as EvalsToTrue() as the former
// will match cases where evaluation fails.
MATCHER(EvalsToFalse, "") {
  if (!arg.ok()) {
    *result_listener << "evaluation error: " << arg.status().message();
    return false;
  }

  absl::StatusOr<bool> result = arg.value().GetBoolean();
  if (!result.ok()) {
    *result_listener << "did not resolve to a boolean: "
                     << result.status().message();
    return false;
  }

  if (result.value()) {
    *result_listener << "evaluated to true";
  }

  return !result.value();
}

// Matcher for absl::StatusOr<EvaluationResult> that checks to see that the
// evaluation succeeded and evaluated to a single boolean with value of true.
//
// NOTE: Not(EvalsToTrue()) is not the same as EvalsToFalse() as the former
// will match cases where evaluation fails.
MATCHER(EvalsToTrue, "") {
  if (!arg.ok()) {
    *result_listener << "evaluation error: " << arg.status().message();
    return false;
  }

  absl::StatusOr<bool> result = arg.value().GetBoolean();
  if (!result.ok()) {
    *result_listener << "did not resolve to a boolean: "
                     << result.status().message();
    return false;
  }

  if (!result.value()) {
    *result_listener << "evaluated to false";
  }

  return result.value();
}

// Matcher for absl::StatusOr<EvaluationResult> that checks to see that the
// evaluation succeeded and evaluated to an integer equal to the provided value.
MATCHER_P(EvalsToInteger, expected, "") {
  if (!arg.ok()) {
    return false;
  }

  absl::StatusOr<int> result = arg.value().GetInteger();
  if (!result.ok()) {
    *result_listener << "did not resolve to a integer: "
                     << result.status().message();
    return false;
  }

  return result.value() == expected;
}

MATCHER_P(EvalsToStringThatMatches, string_matcher, "") {
  if (!arg.ok()) {
    *result_listener << "evaluation error: " << arg.status().message();
    return false;
  }

  absl::StatusOr<std::string> result = arg.value().GetString();
  if (!result.ok()) {
    *result_listener << "did not resolve to a string: "
                     << result.status().message();
    return false;
  }

  return string_matcher.impl().MatchAndExplain(result.value(), result_listener);
}

// Matcher for absl::StatusOr<T>, checks status is okay and uses inner_matcher
// on the messages.
MATCHER_P(IsOkWithMessages, elements_vector, "") {
  if (!arg.ok()) {
    *result_listener << "evaluation error: " << arg.status().message();
    return false;
  }
  if (arg->GetMessages().size() != elements_vector.size()) {
    *result_listener << "size mismatch, expected " << elements_vector.size()
                     << ", actual " << arg->GetMessages().size();
    return false;
  }
  for (int i = 0; i < arg->GetMessages().size(); ++i) {
    if (!google::protobuf::util::MessageDifferencer::Equals(*arg->GetMessages()[i],
                                                  elements_vector[i])) {
      return false;
    }
  }
  return true;
}

// Matcher for absl::StatusOr<T> that checks to see that a status is present
// with the provided code.
MATCHER_P(HasStatusCode, status_code, "") {
  return !arg.ok() && arg.status().code() == status_code;
}

template <typename T>
T ParseFromString(const std::string& str) {
  google::protobuf::TextFormat::Parser parser;
  parser.AllowPartialMessage(true);
  T t;
  EXPECT_TRUE(parser.ParseFromString(str, &t));
  return t;
}

template <typename T>
T ValidEncounter() {
  return ParseFromString<T>(R"pb(
    status { value: PLANNED }
    id { value: "123" }
    period {
      start: { value_us: 1556750153000 timezone: "America/Los_Angeles" }
    }
    status_history { status { value: ARRIVED } }
  )pb");
}

template <>
r5::core::Encounter ValidEncounter() {
  return ParseFromString<r5::core::Encounter>(R"pb(
    status { value: PLANNED }
    id { value: "123" }
    actual_period {
      start: { value_us: 1556750153000 timezone: "America/Los_Angeles" }
    }
  )pb");
}

template <typename T>
T ValidObservation() {
  return ParseFromString<T>(R"pb(
    status { value: FINAL }
    code {
      coding {
        system { value: "foo" }
        code { value: "bar" }
      }
    }
    id { value: "123" }
    effective { date_time { value_us: 0 timezone: "America/Los_Angeles" } }
  )pb");
}

template <typename T, typename P>
T ToDateTime(const absl::CivilSecond civil_time, const absl::TimeZone zone,
             const P& precision) {
  T date_time;
  date_time.set_value_us(absl::ToUnixMicros(absl::FromCivil(civil_time, zone)));
  date_time.set_timezone(zone.name());
  date_time.set_precision(precision);
  return date_time;
}

template <typename P, typename D>
P CreatePeriod(const D& start, const D& end) {
  P period;
  *period.mutable_start() = start;
  *period.mutable_end() = end;
  return period;
}

template <typename T>
class FhirPathTest : public ::testing::Test {
 public:
  static absl::StatusOr<CompiledExpression> Compile(
      const ::google::protobuf::Descriptor* descriptor, const std::string& fhir_path) {
    return CompiledExpression::Compile(
        descriptor, T::PrimitiveHandler::GetInstance(), fhir_path);
  }

  template <typename R>
  static absl::StatusOr<EvaluationResult> Evaluate(
      const R& message, const std::string& expression) {
    FHIR_ASSIGN_OR_RETURN(auto compiled_expression,
                          Compile(message.GetDescriptor(), expression));

    auto result = compiled_expression.Evaluate(message);
    CHECK(!result.ok() ||
          (result.ok() && !result.value().CallbackFunctionName().has_value()));
    return result;
  }

  static absl::StatusOr<EvaluationResult> Evaluate(
      const std::string& expression) {
    // FHIRPath assumes a resource object during evaluation, so we use an
    // encounter as a placeholder.
    auto test_observation = ValidObservation<typename T::Observation>();
    auto result =
        Evaluate<typename T::Observation>(test_observation, expression);
    CHECK(!result.ok() ||
          (result.ok() && !result.value().CallbackFunctionName().has_value()));
    return result;
  }
};

template <typename T>
class Stu3AndR4FhirPathTest : public FhirPathTest<T> {};

struct Stu3CoreTestEnv : public testutil::Stu3CoreTestEnv {
  using EncounterStatusCode = ::google::fhir::stu3::proto::EncounterStatusCode;
};

struct R4CoreTestEnv : public testutil::R4CoreTestEnv {
  using EncounterStatusCode = ::google::fhir::r4::core::EncounterStatusCode;
};

struct R5CoreTestEnv : public testutil::R5CoreTestEnv {
  using EncounterStatusCode = ::google::fhir::r5::core::EncounterStatusCode;
};

using Stu3AndR4TestEnvs = ::testing::Types<Stu3CoreTestEnv, R4CoreTestEnv>;
TYPED_TEST_SUITE(Stu3AndR4FhirPathTest, Stu3AndR4TestEnvs);

using AllVersionEnvs =
    ::testing::Types<Stu3CoreTestEnv, R4CoreTestEnv, R5CoreTestEnv>;
TYPED_TEST_SUITE(FhirPathTest, AllVersionEnvs);

TYPED_TEST(FhirPathTest, TestExternalConstants) {
  EXPECT_THAT(TestFixture::Evaluate("%ucum"),
              EvalsToStringThatMatches(StrEq("http://unitsofmeasure.org")));

  EXPECT_THAT(TestFixture::Evaluate("%sct"),
              EvalsToStringThatMatches(StrEq("http://snomed.info/sct")));

  EXPECT_THAT(TestFixture::Evaluate("%loinc"),
              EvalsToStringThatMatches(StrEq("http://loinc.org")));

  EXPECT_THAT(TestFixture::Evaluate("%unknown"),
              HasStatusCode(StatusCode::kNotFound));
}

TYPED_TEST(FhirPathTest, TestExternalConstantsContext) {
  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();

  auto result = TestFixture::Evaluate(test_encounter, "%context").value();
  EXPECT_THAT(result.GetMessages(),
              UnorderedElementsAreArray({EqualsProto(test_encounter)}));
}

TYPED_TEST(FhirPathTest,
           TestExternalConstantsContextReferenceInExpressionParam) {
  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();

  EXPECT_THAT(
      TestFixture::Evaluate(test_encounter, "%context").value().GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter)}));
}

TYPED_TEST(FhirPathTest, TestMalformed) {
  auto expr = TestFixture::Compile(TypeParam::Encounter::descriptor(),
                                   "expression->not->valid");

  EXPECT_THAT(expr, HasStatusCode(StatusCode::kInternal));
}

TYPED_TEST(FhirPathTest, TestGetDirectChild) {
  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();
  EvaluationResult result =
      TestFixture::Evaluate(test_encounter, "status").value();

  EXPECT_THAT(
      result.GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.status())}));
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestExpressionsStartingWithOptionalType) {
  auto expr = TestFixture::Compile(TypeParam::Encounter::descriptor(),
                                   "Patient.status");
  EXPECT_THAT(expr, HasStatusCode(StatusCode::kNotFound));
  EXPECT_NE(expr.status().message().find("Patient"), std::string::npos);

  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();

  EXPECT_THAT(
      TestFixture::Evaluate(test_encounter, "Encounter.status")
          .value()
          .GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.status())}));

  EXPECT_THAT(
      TestFixture::Evaluate(test_encounter, "Encounter").value().GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter)}));
  EXPECT_THAT(
      TestFixture::Evaluate(test_encounter, "DomainResource.ofType(Encounter)")
          .value()
          .GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter)}));
  EXPECT_THAT(TestFixture::Evaluate(test_encounter, ("Resource as Encounter"))
                  .value()
                  .GetMessages(),
              UnorderedElementsAreArray({EqualsProto(test_encounter)}));
  EXPECT_THAT(
      TestFixture::Evaluate(test_encounter, "Resource.select(Encounter)")
          .value()
          .GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter)}));

  EXPECT_THAT(
      TestFixture::Evaluate(test_encounter,
                            ("Encounter.period | status | period"))
          .value()
          .GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.status()),
                                 EqualsProto(test_encounter.period())}));
  EXPECT_THAT(
      TestFixture::Evaluate(
          test_encounter,
          ("Encounter.period | DomainResource.status | Resource.period"))
          .value()
          .GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.status()),
                                 EqualsProto(test_encounter.period())}));
  EXPECT_THAT(
      TestFixture::Evaluate(test_encounter,
                            ("period | status | Encounter.period"))
          .value()
          .GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.status()),
                                 EqualsProto(test_encounter.period())}));

  EXPECT_THAT(
      TestFixture::Evaluate(test_encounter,
                            ("DomainResource.period.union(Resource.period)"))
          .value()
          .GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.period())}));

  EXPECT_THAT(TestFixture::Evaluate(
                  test_encounter,
                  ("Resource.period.start.exists() implies "
                   "(Encounter.status='planned' or status.exists().not())")),
              EvalsToTrue());
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestElementDefinition) {
  auto expr = TestFixture::Compile(TypeParam::Encounter::descriptor(),
                                   "Element.exists()");
  EXPECT_THAT(expr, HasStatusCode(StatusCode::kNotFound));
  EXPECT_NE(expr.status().message().find("Element"), std::string::npos);

  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();

  EXPECT_THAT(
      TestFixture::Evaluate(test_encounter, "period.where(Element.exists())")
          .value()
          .GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.period())}));
  EXPECT_THAT(TestFixture::Evaluate(test_encounter, "id.where(Element = '123')")
                  .value()
                  .GetMessages(),
              UnorderedElementsAreArray({EqualsProto(test_encounter.id())}));
  EXPECT_THAT(
      TestFixture::Evaluate(test_encounter, "period.select(Element.start)")
          .value()
          .GetMessages(),
      UnorderedElementsAreArray(
          {EqualsProto(test_encounter.period().start())}));
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestGetGrandchild) {
  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();
  EvaluationResult result =
      TestFixture::Evaluate(test_encounter, "period.start").value();

  EXPECT_THAT(result.GetMessages(), UnorderedElementsAreArray({EqualsProto(
                                        test_encounter.period().start())}));
}

TYPED_TEST(FhirPathTest, TestFieldNameIsReservedWord) {
  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();
  test_encounter.mutable_text()->mutable_div()->set_value("some string");
  absl::StatusOr<EvaluationResult> result =
      TestFixture::Evaluate(test_encounter, "text.div");

  ASSERT_THAT(result, HasStatusCode(StatusCode::kInternal));
  EXPECT_TRUE(absl::StrContains(result.status().message(),
                                "Unknown terminal type: div"));

  result = TestFixture::Evaluate(test_encounter, "text.`div`");
  FHIR_ASSERT_OK(result.status());
  EXPECT_THAT(
      result.value().GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.text().div())}));
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestGetEmptyGrandchild) {
  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "period.end"),
      EvalsToEmpty());
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestFieldExists) {
  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();
  test_encounter.mutable_class_value()->mutable_display()->set_value("foo");

  EvaluationResult root_result =
      TestFixture::Evaluate(test_encounter, "period").value();
  EXPECT_THAT(
      root_result.GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.period())}));

  // Tests the conversion from camelCase to snake_case
  EvaluationResult camel_case_result =
      TestFixture::Evaluate(test_encounter, "statusHistory").value();
  EXPECT_THAT(camel_case_result.GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(test_encounter.status_history(0))}));

  // Test that the json_name field annotation is used when searching for a
  // field.
  EvaluationResult json_name_alias_result =
      TestFixture::Evaluate(test_encounter, "class").value();
  EXPECT_THAT(
      json_name_alias_result.GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.class_value())}));
}

TYPED_TEST(FhirPathTest, TestEvaluateReferenceField) {
  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();

  // Check that empty reference object results in empty collection.
  EvaluationResult result =
      TestFixture::Evaluate(test_encounter, "serviceProvider.reference")
          .value();
  EXPECT_TRUE(result.GetMessages().empty());

  // Check that a string representation of the FHIR reference is created.
  test_encounter.mutable_service_provider()
      ->mutable_organization_id()
      ->set_value("1");

  result = TestFixture::Evaluate(test_encounter, "serviceProvider.reference")
               .value();
  EXPECT_THAT(result.GetString().value(), Eq("Organization/1"));

  // Check that retrieval works for a different representation of the reference.
  test_encounter.mutable_service_provider()->mutable_uri()->set_value(
      "Organization/2");
  result = TestFixture::Evaluate(test_encounter, "serviceProvider.reference")
               .value();
  EXPECT_THAT(result.GetString().value(), Eq("Organization/2"));
}

TYPED_TEST(FhirPathTest, TestNoSuchField) {
  auto root_expr = TestFixture::Compile(TypeParam::Encounter::descriptor(),
                                        "bogusrootfield");

  EXPECT_THAT(root_expr, HasStatusCode(StatusCode::kNotFound));
  EXPECT_NE(root_expr.status().message().find("bogusrootfield"),
            std::string::npos);

  auto child_expr = TestFixture::Compile(TypeParam::Encounter::descriptor(),
                                         "participant.boguschildfield");

  EXPECT_THAT(child_expr, HasStatusCode(StatusCode::kNotFound));
  EXPECT_NE(child_expr.status().message().find("boguschildfield"),
            std::string::npos);

  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "(participant | status).boguschildfield"),
      EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestNoSuchFunction) {
  auto root_expr = TestFixture::Compile(TypeParam::Encounter::descriptor(),
                                        "participant.bogusfunction()");

  EXPECT_THAT(root_expr, HasStatusCode(StatusCode::kNotFound));
  EXPECT_NE(root_expr.status().message().find("bogusfunction"),
            std::string::npos);
}

TYPED_TEST(FhirPathTest, TestFunctionTopLevelInvocation) {
  EXPECT_THAT(TestFixture::Evaluate("exists()"), EvalsToTrue());
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestFunctionExists) {
  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "period.start.exists()"),
      EvalsToTrue());
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestFunctionExistsNegation) {
  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "period.start.exists().not()"),
      EvalsToFalse());
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestFunctionNotExists) {
  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "period.end.exists()"),
      EvalsToFalse());
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestFunctionNotExistsNegation) {
  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "period.end.exists().not()"),
      EvalsToTrue());
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestFunctionHasValue) {
  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "period.start.hasValue()"),
      EvalsToTrue());
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestLogicalValueFieldExists) {
  // The logical .value field on primitives should return the primitive itself.
  typename TypeParam::Quantity quantity;
  quantity.mutable_value()->set_value("100");
  EXPECT_THAT(TestFixture::Evaluate(quantity, "value.value.exists()"),
              EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TestFunctionExistsWithCriteria) {
  auto observation = ParseFromString<typename TypeParam::CodeableConcept>(R"pb(
    coding {
      system { value: "foo" }
      code { value: "abc" }
    }
    coding {
      system { value: "bar" }
      code { value: "ghi" }
    }
  )pb");

  EXPECT_THAT(
      TestFixture::Evaluate(observation, "coding.exists(system = 'bar')"),
      EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate(observation, "coding.code.exists(true)"),
              EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TestFunctionExistsWithCriteriaNoMatches) {
  EXPECT_THAT(TestFixture::Evaluate("('a' | 'b' | 'c').exists(false)"),
              EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("{}.exists(true)"), EvalsToFalse());
}

TYPED_TEST(FhirPathTest, TestFunctionExistsWithInvalidCriteria) {
  EXPECT_THAT(TestFixture::Evaluate("{}.exists(true, false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(
      TestFixture::Evaluate(ValidObservation<typename TypeParam::Observation>(),
                            "code.coding.exists(does_not_exist = 'foo')"),
      HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(
      TestFixture::Evaluate(ValidObservation<typename TypeParam::Observation>(),
                            "code.coding.exists(foo())"),
      HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestFunctionHasValueNegation) {
  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();
  EXPECT_THAT(
      TestFixture::Evaluate(test_encounter, "period.start.hasValue().not()"),
      EvalsToFalse());

  test_encounter.mutable_period()->clear_start();
  EXPECT_THAT(
      TestFixture::Evaluate(test_encounter, "period.start.hasValue().not()"),
      EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TestFunctionChildren) {
  auto structure_definition =
      ParseFromString<typename TypeParam::StructureDefinition>(R"pb(
        name { value: "foo" }
        context_invariant { value: "bar" }
        snapshot { element { label { value: "snapshot" } } }
        differential { element { label { value: "differential" } } }
      )pb");

  EXPECT_THAT(TestFixture::Evaluate(structure_definition, "children()")
                  .value()
                  .GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(structure_definition.name()),
                   EqualsProto(structure_definition.context_invariant(0)),
                   EqualsProto(structure_definition.snapshot()),
                   EqualsProto(structure_definition.differential())}));

  EXPECT_THAT(
      TestFixture::Evaluate(structure_definition, "children().element")
          .value()
          .GetMessages(),
      UnorderedElementsAreArray(
          {EqualsProto(structure_definition.snapshot().element(0)),
           EqualsProto(structure_definition.differential().element(0))}));

  EXPECT_THAT(TestFixture::Evaluate("(1 | 2).children()"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestFunctionDescendants) {
  auto structure_definition =
      ParseFromString<typename TypeParam::StructureDefinition>(R"pb(
        name { value: "foo" }
        context_invariant { value: "bar" }
        snapshot { element { label { value: "snapshot" } } }
        differential { element { label { value: "differential" } } }
      )pb");

  EXPECT_THAT(
      TestFixture::Evaluate(structure_definition, "descendants()")
          .value()
          .GetMessages(),
      UnorderedElementsAreArray(
          {EqualsProto(structure_definition.name()),
           EqualsProto(structure_definition.context_invariant(0)),
           EqualsProto(structure_definition.snapshot()),
           EqualsProto(structure_definition.snapshot().element(0)),
           EqualsProto(structure_definition.snapshot().element(0).label()),
           EqualsProto(structure_definition.differential()),
           EqualsProto(structure_definition.differential().element(0)),
           EqualsProto(
               structure_definition.differential().element(0).label())}));

  EXPECT_THAT(TestFixture::Evaluate("('foo' | 'bar').descendants()"),
              EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestFunctionDescendantsOnEmptyCollection) {
  EXPECT_THAT(TestFixture::Evaluate("{}.descendants()"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestFunctionDescendantsWithEmptyReference) {
  auto encounter = ParseFromString<typename TypeParam::Encounter>(R"pb(
    meta { extension
           [ {
             url { value: "https://foo.com/bar" }
             value { reference { uri {} } }
           }] }
  )pb");

  EXPECT_THAT(
      TestFixture::Evaluate(encounter, "descendants()").value().GetMessages(),
      UnorderedElementsAreArray(
          {EqualsProto(encounter.meta()),
           EqualsProto(encounter.meta().extension(0)),
           EqualsProto(encounter.meta().extension(0).url()),
           EqualsProto(encounter.meta().extension(0).value().reference())}));
}

TYPED_TEST(FhirPathTest, TestFunctionContains) {
  // Wrong number and/or types of arguments.
  EXPECT_THAT(TestFixture::Evaluate("'foo'.contains()"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("'foo'.contains(1)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("'foo'.contains('a', 'b')"),
              HasStatusCode(StatusCode::kInvalidArgument));

  EXPECT_THAT(TestFixture::Evaluate("'foo'.contains('')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo'.contains('o')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo'.contains('foo')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo'.contains('foob')"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("''.contains('')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("''.contains('foo')"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("{}.contains('foo')"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestFunctionEndsWith) {
  // Missing argument
  EXPECT_THAT(TestFixture::Evaluate("'foo'.endsWith()"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Empty colection argument
  EXPECT_THAT(TestFixture::Evaluate("'foo'.endsWith({})"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Too many arguments
  EXPECT_THAT(TestFixture::Evaluate("'foo'.endsWith('foo', 'foo')"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Wrong argument type
  EXPECT_THAT(TestFixture::Evaluate("'foo'.endsWith(1)"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Function does not exist for non-string type
  EXPECT_THAT(TestFixture::Evaluate("1.endsWith('1')"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Basic cases
  EXPECT_THAT(TestFixture::Evaluate("{}.endsWith('')"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("''.endsWith('')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo'.endsWith('')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo'.endsWith('o')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo'.endsWith('foo')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo'.endsWith('bfoo')"), EvalsToFalse());
}

TYPED_TEST(FhirPathTest, TestFunctionStartsWith) {
  // Missing argument
  EXPECT_THAT(TestFixture::Evaluate("'foo'.startsWith()"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Too many arguments
  EXPECT_THAT(TestFixture::Evaluate("'foo'.startsWith('foo', 'foo')"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Wrong argument type
  EXPECT_THAT(TestFixture::Evaluate("'foo'.startsWith(1)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("'foo'.startsWith(1.0)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("'foo'.startsWith(@2015-02-04T14:34:28Z)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("'foo'.startsWith(true)"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Function does not exist for non-string type
  EXPECT_THAT(TestFixture::Evaluate("1.startsWith(1)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("1.startsWith('1')"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Basic cases
  EXPECT_THAT(TestFixture::Evaluate("{}.startsWith('')"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("''.startsWith('')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo'.startsWith('')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo'.startsWith('f')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo'.startsWith('foo')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo'.startsWith('foob')"),
              EvalsToFalse());
}

TYPED_TEST(FhirPathTest, TestFunctionStartsWithSelfReference) {
  EXPECT_THAT(
      TestFixture::Evaluate(ValidObservation<typename TypeParam::Observation>(),
                            "code.coding.code.startsWith(code.coding.code)"),
      EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TestFunctionStartsWithInvokedOnNonString) {
  EXPECT_THAT(
      TestFixture::Evaluate(ValidObservation<typename TypeParam::Observation>(),
                            "code.startsWith('foo')"),
      HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestFunctionIndexOf) {
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.indexOf('bc')"),
              EvalsToInteger(1));
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.indexOf('x')"),
              EvalsToInteger(-1));
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.indexOf('abcdefg')"),
              EvalsToInteger(0));
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.indexOf('')"),
              EvalsToInteger(0));

  // http://hl7.org/fhirpath/N1/#indexofsubstring-string-integer
  // If the input or substring is empty ({ }), the result is empty ({ }).
  EXPECT_THAT(TestFixture::Evaluate("{}.indexOf('')"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("''.indexOf({})"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestFunctionSubstring) {
  // Test listed examples in
  // http://hl7.org/fhirpath/#substringstart-integer-length-integer-string
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.substring(3)"),
              EvalsToStringThatMatches(StrEq("defg")));
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.substring(1, 2)"),
              EvalsToStringThatMatches(StrEq("bc")));
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.substring(6, 2)"),
              EvalsToStringThatMatches(StrEq("g")));
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.substring(7, 1)"),
              EvalsToEmpty());

  // Test UTF-8 string with multibyte characters.
  EXPECT_THAT(TestFixture::Evaluate("'🔥FHIR'.substring(2)"),
              EvalsToStringThatMatches(StrEq("HIR")));
  EXPECT_THAT(TestFixture::Evaluate("'🔥'.substring(0)"),
              EvalsToStringThatMatches(StrEq("🔥")));
  EXPECT_THAT(TestFixture::Evaluate("'🔥'.substring(0, 1)"),
              EvalsToStringThatMatches(StrEq("🔥")));
  EXPECT_THAT(TestFixture::Evaluate("'🔥'.substring(1, 1)"),
              EvalsToStringThatMatches(StrEq("")));
  EXPECT_THAT(TestFixture::Evaluate("'🔥'.substring(1, 0)"),
              EvalsToStringThatMatches(StrEq("")));

  // Test other requirements in
  // http://hl7.org/fhirpath/#substringstart-integer-length-integer-string
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.substring(-1)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.substring({})"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{}.substring(3)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.substring(3, {})"),
              EvalsToStringThatMatches(StrEq("defg")));
  EXPECT_THAT(TestFixture::Evaluate("('abc' | 'defg').substring(1)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.substring()"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.substring(1, 2, 3)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.substring('ab')"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("123.substring(1)"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestFunctionUpper) {
  EXPECT_THAT(TestFixture::Evaluate("{}.upper()"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("''.upper()"),
              EvalsToStringThatMatches(StrEq("")));
  EXPECT_THAT(TestFixture::Evaluate("'aBa'.upper()"),
              EvalsToStringThatMatches(StrEq("ABA")));
  EXPECT_THAT(TestFixture::Evaluate("'ABA'.upper()"),
              EvalsToStringThatMatches(StrEq("ABA")));
}

TYPED_TEST(FhirPathTest, TestFunctionLower) {
  EXPECT_THAT(TestFixture::Evaluate("{}.lower()"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("''.lower()"),
              EvalsToStringThatMatches(StrEq("")));
  EXPECT_THAT(TestFixture::Evaluate("'aBa'.lower()"),
              EvalsToStringThatMatches(StrEq("aba")));
  EXPECT_THAT(TestFixture::Evaluate("'aba'.lower()"),
              EvalsToStringThatMatches(StrEq("aba")));
}

TYPED_TEST(FhirPathTest, TestFunctionMatches) {
  EXPECT_THAT(TestFixture::Evaluate("{}.matches('')"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("''.matches('')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'a'.matches('a')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'abc'.matches('a')"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("'abc'.matches('...')"), EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TestFunctionReplaceMatches) {
  EXPECT_THAT(TestFixture::Evaluate("{}.replaceMatches('', '')"),
              EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("'a'.replaceMatches('.', 'b')"),
              EvalsToStringThatMatches(StrEq("b")));
}

TYPED_TEST(FhirPathTest, TestFunctionReplace) {
  EXPECT_THAT(TestFixture::Evaluate("{}.replace('', '')"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("''.replace({}, '')"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("''.replace('', {})"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("''.replace('', 'x')"),
              EvalsToStringThatMatches(StrEq("")));
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.replace('x', '123')"),
              EvalsToStringThatMatches(StrEq("abcdefg")));
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.replace('cde', '123')"),
              EvalsToStringThatMatches(StrEq("ab123fg")));
  EXPECT_THAT(TestFixture::Evaluate("'abcdefg'.replace('cde', '')"),
              EvalsToStringThatMatches(StrEq("abfg")));
  EXPECT_THAT(TestFixture::Evaluate("'abc'.replace('', 'x')"),
              EvalsToStringThatMatches(StrEq("xaxbxcx")));
  EXPECT_THAT(TestFixture::Evaluate("'£'.replace('', 'x')"),
              EvalsToStringThatMatches(StrEq("x£x")));
}

TYPED_TEST(FhirPathTest, TestFunctionReplaceMatchesWrongArgCount) {
  absl::StatusOr<EvaluationResult> result =
      TestFixture::Evaluate("''.replaceMatches()");
  EXPECT_THAT(result.status().code(), Eq(absl::StatusCode::kInvalidArgument))
      << result.status();
}

TYPED_TEST(FhirPathTest, TestFunctionReplaceMatchesBadRegex) {
  absl::StatusOr<EvaluationResult> result =
      TestFixture::Evaluate("''.replaceMatches('(', 'a')").status();
  EXPECT_THAT(result.status().code(), Eq(absl::StatusCode::kInvalidArgument))
      << result.status();
}

TYPED_TEST(FhirPathTest, TestFunctionLength) {
  EXPECT_THAT(TestFixture::Evaluate("{}.length()"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("''.length() = 0"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'abc'.length() = 3"), EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("3.length()"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestFunctionToInteger) {
  EXPECT_THAT(TestFixture::Evaluate("1.toInteger()"), EvalsToInteger(1));
  EXPECT_THAT(TestFixture::Evaluate("'2'.toInteger()"), EvalsToInteger(2));

  EXPECT_THAT(TestFixture::Evaluate("(3.3).toInteger()"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("'a'.toInteger()"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("{}.toInteger()"), EvalsToEmpty());
  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "toInteger()"),
      EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("(1 | 2).toInteger()"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestFunctionConvertsToInteger) {
  EXPECT_THAT(TestFixture::Evaluate("1.convertsToInteger()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'2'.convertsToInteger()"), EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("(3.3).convertsToInteger()"),
              EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("'a'.convertsToInteger()"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("{}.convertsToInteger()"), EvalsToEmpty());
  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "convertsToInteger()"),
      EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("(1 | 2).convertsToInteger()"),
              HasStatusCode(StatusCode::kFailedPrecondition));
}

TYPED_TEST(FhirPathTest, TestFunctionToString) {
  EXPECT_THAT(TestFixture::Evaluate("1.toString()"),
              EvalsToStringThatMatches(StrEq("1")));
  EXPECT_THAT(TestFixture::Evaluate("1.1.toString()"),
              EvalsToStringThatMatches(StrEq("1.1")));
  EXPECT_THAT(TestFixture::Evaluate("'foo'.toString()"),
              EvalsToStringThatMatches(StrEq("foo")));
  EXPECT_THAT(TestFixture::Evaluate("true.toString()"),
              EvalsToStringThatMatches(StrEq("true")));
  EXPECT_THAT(TestFixture::Evaluate("{}.toString()"), EvalsToEmpty());
  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "toString()"),
      EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("(1 | 2).toString()"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestFunctionConvertsToString) {
  EXPECT_THAT(TestFixture::Evaluate("1.convertsToString()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.1.convertsToString()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo'.convertsToString()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true.convertsToString()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("{}.convertsToString()"), EvalsToEmpty());
  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "convertsToString()"),
      EvalsToFalse());
  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "status.convertsToString()"),
      EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(1 | 2).convertsToString()"),
              HasStatusCode(StatusCode::kFailedPrecondition));
}

TYPED_TEST(FhirPathTest, TestFunctionToBoolean) {
  EXPECT_THAT(TestFixture::Evaluate("'true'.toBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'t'.toBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'yes'.toBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'y'.toBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'1'.toBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'1.0'.toBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.toBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.0.toBoolean()"), EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("'false'.toBoolean()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("'f'.toBoolean()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("'no'.toBoolean()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("'n'.toBoolean()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("'0'.toBoolean()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("'0.0'.toBoolean()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("0.toBoolean()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("'0.0'.toBoolean()"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("{}.toBoolean()"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("'foo'.toBoolean()"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("2.toBoolean()"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("2.0.toBoolean()"), EvalsToEmpty());
  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "toBoolean()"),
      EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("(1 | 2).toBoolean()"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestFunctionConvertsToBoolean) {
  EXPECT_THAT(TestFixture::Evaluate("'true'.convertsToBoolean()"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'t'.convertsToBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'yes'.convertsToBoolean()"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'y'.convertsToBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'1'.convertsToBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'1.0'.convertsToBoolean()"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.convertsToBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.0.convertsToBoolean()"), EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("'false'.convertsToBoolean()"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'f'.convertsToBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'no'.convertsToBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'n'.convertsToBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'0'.convertsToBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'0.0'.convertsToBoolean()"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("0.convertsToBoolean()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'0.0'.convertsToBoolean()"),
              EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("{}.convertsToBoolean()"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("'foo'.convertsToBoolean()"),
              EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("2.convertsToBoolean()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("2.0.convertsToBoolean()"), EvalsToFalse());
  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "convertsToBoolean()"),
      EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("(1 | 2).convertsToBoolean()"),
              HasStatusCode(StatusCode::kFailedPrecondition));
}

TYPED_TEST(FhirPathTest, TestFunctionTrace) {
  EXPECT_THAT(TestFixture::Evaluate("true.trace('debug')"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("{}.trace('debug')"), EvalsToEmpty());
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestFunctionHasValueComplex) {
  // hasValue should return false for non-primitive types.
  EXPECT_THAT(
      TestFixture::Evaluate(ValidEncounter<typename TypeParam::Encounter>(),
                            "period.hasValue()"),
      EvalsToFalse());
}

TYPED_TEST(FhirPathTest, TestFunctionEmpty) {
  EXPECT_THAT(TestFixture::Evaluate("{}.empty()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true.empty()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("(false | true).empty()"), EvalsToFalse());
}

TYPED_TEST(FhirPathTest, TestFunctionCount) {
  EXPECT_THAT(TestFixture::Evaluate("{}.count()"), EvalsToInteger(0));
  EXPECT_THAT(TestFixture::Evaluate("'a'.count()"), EvalsToInteger(1));
  EXPECT_THAT(TestFixture::Evaluate("('a' | 1).count()"), EvalsToInteger(2));
}

TYPED_TEST(FhirPathTest, TestFunctionFirst) {
  EXPECT_THAT(TestFixture::Evaluate("{}.first()"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("true.first()"), EvalsToTrue());
  EXPECT_TRUE(TestFixture::Evaluate("(false | true).first()").ok());
}

TYPED_TEST(FhirPathTest, TestFunctionLast) {
  EXPECT_THAT(TestFixture::Evaluate("{}.last()"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("true.last()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true.combine(true).last()"),
              EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TestFunctionSingle) {
  EXPECT_THAT(TestFixture::Evaluate("{}.single()"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("true.single()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(false | true).single()"),
              HasStatusCode(StatusCode::kFailedPrecondition));
}

TYPED_TEST(FhirPathTest, TestFunctionTail) {
  EXPECT_THAT(TestFixture::Evaluate("{}.tail()"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("true.tail()"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("true.combine(true).tail()"),
              EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TestFunctionSkip) {
  EXPECT_THAT(TestFixture::Evaluate("{}.skip(-1)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{}.skip(0)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{}.skip(1)"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("(true).skip(-1)"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(true).skip(0)"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(true).skip(1)"), EvalsToEmpty());

  EXPECT_THAT(
      TestFixture::Evaluate("true.combine(true).skip(-1) = true.combine(true)"),
      EvalsToTrue());
  EXPECT_THAT(
      TestFixture::Evaluate("true.combine(true).skip(0) = true.combine(true)"),
      EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true.combine(true).skip(1)"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true.combine(true).skip(2)"),
              EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("(true).skip()"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("(true).skip('1')"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestFunctionTake) {
  EXPECT_THAT(TestFixture::Evaluate("{}.take(-1)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{}.take(0)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{}.take(1)"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("(true).take(-1)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("(true).take(0)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("(true).take(1)"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(true).take(2)"), EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("true.combine(true).take(-1)"),
              EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("true.combine(true).take(0)"),
              EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("true.combine(true).take(1)"),
              EvalsToTrue());
  EXPECT_THAT(
      TestFixture::Evaluate("true.combine(true).take(2) = true.combine(true)"),
      EvalsToTrue());
  EXPECT_THAT(
      TestFixture::Evaluate("true.combine(true).take(3) = true.combine(true)"),
      EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("(true).take()"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("(true).take('1')"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestFunctionOfTypePrimitives) {
  EXPECT_THAT(TestFixture::Evaluate("{}.ofType(Boolean)"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("(true | 1 | 2.0 | 'foo').ofType(Boolean)"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(true | 1 | 2.0 | 'foo').ofType(String)"),
              EvalsToStringThatMatches(StrEq("foo")));

  EXPECT_THAT(TestFixture::Evaluate("(true | 1 | 2.0 | 'foo').ofType(Integer)")
                  .value()
                  .GetMessages(),
              ElementsAreArray({EqualsProto(
                  ParseFromString<typename TypeParam::Integer>("value: 1"))}));
  EXPECT_THAT(
      TestFixture::Evaluate("(true | 1 | 2.0 | 'foo').ofType(Decimal)")
          .value()
          .GetMessages(),
      ElementsAreArray({EqualsProto(
          ParseFromString<typename TypeParam::Decimal>("value: '2.0'"))}));
}

TYPED_TEST(FhirPathTest, TestFunctionOfTypeResources) {
  auto observation =
      ParseFromString<typename TypeParam::Observation>(R"pb()pb");

  EXPECT_THAT(TestFixture::Evaluate(observation, "$this.ofType(Boolean)"),
              EvalsToEmpty());
  EXPECT_THAT(
      TestFixture::Evaluate(observation, "$this.ofType(CodeableConcept)"),
      EvalsToEmpty());

  EvaluationResult as_observation_evaluation_result =
      TestFixture::Evaluate(observation, "$this.ofType(Observation)").value();
  EXPECT_THAT(as_observation_evaluation_result.GetMessages(),
              ElementsAreArray({EqualsProto(observation)}));
}

TYPED_TEST(FhirPathTest, TestFunctionAsPrimitives) {
  EXPECT_THAT(TestFixture::Evaluate("{}.as(Boolean)"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("true.as(Boolean)"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true.as(Decimal)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("true.as(Integer)"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("1.as(Integer)"), EvalsToInteger(1));
  EXPECT_THAT(TestFixture::Evaluate("1.as(Decimal)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("1.as(Boolean)"), EvalsToEmpty());

  EXPECT_EQ(
      TestFixture::Evaluate("1.1.as(Decimal)").value().GetDecimal().value(),
      "1.1");
  EXPECT_THAT(TestFixture::Evaluate("1.1.as(Integer)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("1.1.as(Boolean)"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestFunctionAsResources) {
  auto observation =
      ParseFromString<typename TypeParam::Observation>(R"pb()pb");

  EXPECT_THAT(TestFixture::Evaluate(observation, "$this.as(Boolean)"),
              EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate(observation, "$this.as(CodeableConcept)"),
              EvalsToEmpty());

  EvaluationResult as_observation_evaluation_result =
      TestFixture::Evaluate(observation, "$this.as(Observation)").value();
  EXPECT_THAT(as_observation_evaluation_result.GetMessages(),
              ElementsAreArray({EqualsProto(observation)}));
}

TYPED_TEST(FhirPathTest, TestOperatorAsPrimitives) {
  EXPECT_THAT(TestFixture::Evaluate("{} as Boolean"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("true as Boolean"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true as Decimal"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("true as Integer"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("1 as Integer"), EvalsToInteger(1));
  EXPECT_THAT(TestFixture::Evaluate("1 as Decimal"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("1 as Boolean"), EvalsToEmpty());

  EXPECT_EQ(
      TestFixture::Evaluate("1.1 as Decimal").value().GetDecimal().value(),
      "1.1");
  EXPECT_THAT(TestFixture::Evaluate("1.1 as Integer"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("1.1 as Boolean"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestOperatorAsResources) {
  auto observation =
      ParseFromString<typename TypeParam::Observation>(R"pb()pb");

  EXPECT_THAT(TestFixture::Evaluate(observation, "$this as Boolean"),
              EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate(observation, "$this as CodeableConcept"),
              EvalsToEmpty());

  EvaluationResult as_observation_evaluation_result =
      TestFixture::Evaluate(observation, "$this as Observation").value();
  EXPECT_THAT(as_observation_evaluation_result.GetMessages(),
              ElementsAreArray({EqualsProto(observation)}));
}

TYPED_TEST(FhirPathTest, TestOperatorAsDateTime) {
  auto test_observation = ValidObservation<typename TypeParam::Observation>();
  EvaluationResult result =
      TestFixture::Evaluate(test_observation, "effective as DateTime").value();

  EXPECT_THAT(result.GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(test_observation.effective().date_time())}));
}

TYPED_TEST(FhirPathTest, TestFunctionIsPrimitives) {
  EXPECT_THAT(TestFixture::Evaluate("{}.is(Boolean)"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("true.is(Boolean)"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true.is(Decimal)"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("true.is(Integer)"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("1.is(Integer)"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.is(Decimal)"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("1.is(Boolean)"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("1.1.is(Decimal)"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.1.is(Integer)"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("1.1.is(Boolean)"), EvalsToFalse());
}

TYPED_TEST(FhirPathTest, TestFunctionIsResources) {
  auto observation =
      ParseFromString<typename TypeParam::Observation>(R"pb()pb");

  EXPECT_THAT(TestFixture::Evaluate(observation, "$this.is(Boolean)"),
              EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate(observation, "$this.is(CodeableConcept)"),
              EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate(observation, "$this.is(Observation)"),
              EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TestOperatorIsPrimitives) {
  EXPECT_THAT(TestFixture::Evaluate("{} is Boolean"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("true is Boolean"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true is Decimal"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("true is Integer"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("1 is Integer"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1 is Decimal"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("1 is Boolean"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("1.1 is Decimal"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.1 is Integer"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("1.1 is Boolean"), EvalsToFalse());
}

TYPED_TEST(FhirPathTest, TestOperatorIsResources) {
  auto observation =
      ParseFromString<typename TypeParam::Observation>(R"pb()pb");

  EXPECT_THAT(TestFixture::Evaluate(observation, "$this is Boolean"),
              EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate(observation, "$this is CodeableConcept"),
              EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate(observation, "$this is Observation"),
              EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TestFunctionTailMaintainsOrder) {
  auto codeable_concept =
      ParseFromString<typename TypeParam::CodeableConcept>(R"pb(
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
      )pb");

  auto code_def = ParseFromString<typename TypeParam::Code>("value: 'def'");
  auto code_ghi = ParseFromString<typename TypeParam::Code>("value: 'ghi'");
  EvaluationResult evaluation_result =
      TestFixture::Evaluate(codeable_concept, "coding.tail().code").value();
  EXPECT_THAT(evaluation_result.GetMessages(),
              ElementsAreArray({EqualsProto(code_def), EqualsProto(code_ghi)}));
}

TYPED_TEST(FhirPathTest, TestUnionOperator) {
  EXPECT_THAT(TestFixture::Evaluate("({} | {})"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("(true | {})"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(true | true)"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(false | {})"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("(false | false)"), EvalsToFalse());
}

TYPED_TEST(FhirPathTest, TestUnionOperatorDeduplicationPrimitives) {
  EvaluationResult evaluation_result =
      TestFixture::Evaluate("true | false | 1 | 'foo' | 2 | 1 | 'foo'").value();
  std::vector<const Message*> result = evaluation_result.GetMessages();

  auto true_proto = ParseFromString<typename TypeParam::Boolean>("value: true");
  auto false_proto =
      ParseFromString<typename TypeParam::Boolean>("value: false");
  auto integer_1_proto =
      ParseFromString<typename TypeParam::Integer>("value: 1");
  auto integer_2_proto =
      ParseFromString<typename TypeParam::Integer>("value: 2");
  auto string_foo_proto =
      ParseFromString<typename TypeParam::String>("value: 'foo'");

  ASSERT_THAT(result,
              UnorderedElementsAreArray(
                  {EqualsProto(true_proto), EqualsProto(false_proto),
                   EqualsProto(integer_1_proto), EqualsProto(integer_2_proto),
                   EqualsProto(string_foo_proto)}));
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestUnionOperatorDeduplicationObjects) {
  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();

  EvaluationResult evaluation_result =
      TestFixture::Evaluate(test_encounter,
                            ("period | status | status | period"))
          .value();
  std::vector<const Message*> result = evaluation_result.GetMessages();

  ASSERT_THAT(result, UnorderedElementsAreArray(
                          {EqualsProto(test_encounter.status()),
                           EqualsProto(test_encounter.period())}));
}

TYPED_TEST(FhirPathTest, TestUnionOperatorWhenOneSideHasDuplicates) {
  EXPECT_THAT(TestFixture::Evaluate("22.combine(22) | {}"), EvalsToInteger(22));

  EXPECT_THAT(TestFixture::Evaluate("{} | 33.combine(33)"), EvalsToInteger(33));

  auto integer_22_proto =
      ParseFromString<typename TypeParam::Integer>("value: 22");
  auto integer_33_proto =
      ParseFromString<typename TypeParam::Integer>("value: 33");

  EXPECT_THAT(
      TestFixture::Evaluate("33 | 22.combine(22)").value().GetMessages(),
      ElementsAreArray(
          {EqualsProto(integer_33_proto), EqualsProto(integer_22_proto)}));
}

TYPED_TEST(FhirPathTest, TestUnionFunction) {
  EXPECT_THAT(TestFixture::Evaluate("{}.union({})"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("true.union({})"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true.union(true)"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("false.union({})"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("false.union(false)"), EvalsToFalse());
}

TYPED_TEST(FhirPathTest, TestUnionFunctionDeduplicationPrimitives) {
  EvaluationResult evaluation_result =
      TestFixture::Evaluate(
          "true.union(false).union(1).union('foo').union(2).union(1).union('"
          "foo')")
          .value();
  std::vector<const Message*> result = evaluation_result.GetMessages();

  auto true_proto = ParseFromString<typename TypeParam::Boolean>("value: true");
  auto false_proto =
      ParseFromString<typename TypeParam::Boolean>("value: false");
  auto integer_1_proto =
      ParseFromString<typename TypeParam::Integer>("value: 1");
  auto integer_2_proto =
      ParseFromString<typename TypeParam::Integer>("value: 2");
  auto string_foo_proto =
      ParseFromString<typename TypeParam::String>("value: 'foo'");

  ASSERT_THAT(result,
              UnorderedElementsAreArray(
                  {EqualsProto(true_proto), EqualsProto(false_proto),
                   EqualsProto(integer_1_proto), EqualsProto(integer_2_proto),
                   EqualsProto(string_foo_proto)}));
}

TYPED_TEST(Stu3AndR4FhirPathTest, TestUnionFunctionDeduplicationObjects) {
  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();

  EvaluationResult evaluation_result =
      TestFixture::Evaluate(test_encounter,
                            "period.union(status).union(status).union(period)")
          .value();
  std::vector<const Message*> result = evaluation_result.GetMessages();

  ASSERT_THAT(result, UnorderedElementsAreArray(
                          {EqualsProto(test_encounter.status()),
                           EqualsProto(test_encounter.period())}));
}

TYPED_TEST(FhirPathTest, TestCombine) {
  EXPECT_THAT(TestFixture::Evaluate("{}.combine({})"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("true.combine({})"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("{}.combine(true)"), EvalsToTrue());

  auto true_proto = ParseFromString<typename TypeParam::Boolean>("value: true");
  auto false_proto =
      ParseFromString<typename TypeParam::Boolean>("value: false");
  EvaluationResult evaluation_result =
      TestFixture::Evaluate("true.combine(true).combine(false)").value();
  EXPECT_THAT(evaluation_result.GetMessages(),
              UnorderedElementsAreArray({EqualsProto(true_proto),
                                         EqualsProto(true_proto),
                                         EqualsProto(false_proto)}));
}

TYPED_TEST(FhirPathTest, TestIntersect) {
  EXPECT_THAT(TestFixture::Evaluate("{}.intersect({})"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("true.intersect({})"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("true.intersect(false)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{}.intersect(true)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("true.intersect(true)"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(true | false).intersect(true)"),
              EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("(true.combine(true)).intersect(true))"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(true).intersect(true.combine(true))"),
              EvalsToTrue());

  auto true_proto = ParseFromString<typename TypeParam::Boolean>("value: true");
  auto false_proto =
      ParseFromString<typename TypeParam::Boolean>("value: false");
  EvaluationResult evaluation_result =
      TestFixture::Evaluate("(true | false).intersect(true | false)").value();
  EXPECT_THAT(evaluation_result.GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(true_proto), EqualsProto(false_proto)}));
}

TYPED_TEST(FhirPathTest, TestDistinct) {
  EXPECT_THAT(TestFixture::Evaluate("{}.distinct()"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("true.distinct()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true.combine(true).distinct()"),
              EvalsToTrue());

  auto true_proto = ParseFromString<typename TypeParam::Boolean>("value: true");
  auto false_proto =
      ParseFromString<typename TypeParam::Boolean>("value: false");
  EvaluationResult evaluation_result =
      TestFixture::Evaluate("(true | false).distinct()").value();
  EXPECT_THAT(evaluation_result.GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(true_proto), EqualsProto(false_proto)}));
}

TYPED_TEST(FhirPathTest, TestIsDistinct) {
  EXPECT_THAT(TestFixture::Evaluate("{}.isDistinct()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true.isDistinct()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(true | false).isDistinct()"),
              EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("true.combine(true).isDistinct()"),
              EvalsToFalse());
}

TYPED_TEST(FhirPathTest, TestIndexer) {
  EXPECT_THAT(TestFixture::Evaluate("true[0]"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true[1]"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("false[0]"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("false[1]"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("true['foo']"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("true[(1 | 2)]"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestContains) {
  EXPECT_THAT(TestFixture::Evaluate("true contains true"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(false | true) contains true"),
              EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("true contains false"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("(false | true) contains 1"),
              EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("{} contains true"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("({} contains {})"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("(true contains {})"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("{} contains (true | false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("true contains (true | false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestIn) {
  EXPECT_THAT(TestFixture::Evaluate("true in true"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true in (false | true)"), EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("false in true"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("1 in (false | true)"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("true in {}"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("({} in {})"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("({} in true)"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("(true | false) in {}"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("(true | false) in {}"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestImplies) {
  EXPECT_THAT(TestFixture::Evaluate("(true implies true) = true"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(true implies false) = false"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(true implies {})"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("(false implies true) = true"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(false implies false) = true"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(false implies {}) = true"),
              EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("({} implies true) = true"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("({} implies false)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("({} implies {})"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestWhere) {
  auto observation = ParseFromString<typename TypeParam::CodeableConcept>(R"pb(
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
  )pb");

  auto code_abc = ParseFromString<typename TypeParam::Code>("value: 'abc'");
  auto code_ghi = ParseFromString<typename TypeParam::Code>("value: 'ghi'");
  EvaluationResult evaluation_result =
      TestFixture::Evaluate(observation, "coding.where(system = 'foo').code")
          .value();
  EXPECT_THAT(evaluation_result.GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(code_abc), EqualsProto(code_ghi)}));
}

TYPED_TEST(FhirPathTest, TestWhereNoMatches) {
  EXPECT_THAT(TestFixture::Evaluate("('a' | 'b' | 'c').where(false)"),
              EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{}.where(true)"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestWhereValidatesArguments) {
  EXPECT_THAT(TestFixture::Evaluate("{}.where()"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("{}.where(true)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{}.where(true, false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestWhereWithNonExistentFieldReference) {
  EXPECT_THAT(
      TestFixture::Evaluate(ValidObservation<typename TypeParam::Observation>(),
                            "code.coding.where(does_not_exist = 'foo')"),
      HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestWhereWithNonExistentFunction) {
  EXPECT_THAT(
      TestFixture::Evaluate(ValidObservation<typename TypeParam::Observation>(),
                            "code.coding.where(foo())"),
      HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestAnyTrue) {
  EXPECT_THAT(TestFixture::Evaluate("{}.anyTrue()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("(false).anyTrue()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("(true).anyTrue()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(false | true).anyTrue()"), EvalsToTrue());

  // Verify that anyTrue() fails when called with the wrong number of arguments.
  EXPECT_THAT(TestFixture::Evaluate("{}.anyTrue(true)"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestAnyFalse) {
  EXPECT_THAT(TestFixture::Evaluate("{}.anyFalse()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("(false).anyFalse()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(true).anyFalse()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("(false | true).anyFalse()"),
              EvalsToTrue());

  // Verify that anyFalse() fails when called with the wrong number of
  // arguments.
  EXPECT_THAT(TestFixture::Evaluate("{}.anyFalse(true)"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestAllTrue) {
  EXPECT_THAT(TestFixture::Evaluate("{}.allTrue()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(false).allTrue()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("(true).allTrue()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(false | true).allTrue()"),
              EvalsToFalse());

  // Verify that allTrue() fails when called with the wrong number of arguments.
  EXPECT_THAT(TestFixture::Evaluate("{}.allTrue(true)"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestAllFalse) {
  EXPECT_THAT(TestFixture::Evaluate("{}.allFalse()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(false).allFalse()"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(true).allFalse()"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("(false | true).allFalse()"),
              EvalsToFalse());

  // Verify that alFalse() fails when called with the wrong number of arguments.
  EXPECT_THAT(TestFixture::Evaluate("{}.allFalse(true)"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestAll) {
  EXPECT_THAT(TestFixture::Evaluate("{}.all(false)"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(false).all(true)"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(1 | 2 | 3).all($this < 4)"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(1 | 2 | 3).all($this > 4)"),
              EvalsToFalse());

  // Verify that all() fails when called with the wrong number of arguments.
  EXPECT_THAT(TestFixture::Evaluate("{}.all()"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("{}.all(true, false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestAllReadsFieldFromDifferingTypes) {
  auto structure_definition =
      ParseFromString<typename TypeParam::StructureDefinition>(R"pb(
        snapshot { element {} }
        differential { element {} }
      )pb");

  EXPECT_THAT(
      TestFixture::Evaluate(structure_definition,
                            "(snapshot | differential).all(element.exists())"),
      EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TestSelect) {
  EvaluationResult evaluation_result =
      TestFixture::Evaluate("(1 | 2 | 3).select(($this > 2) | $this)").value();
  std::vector<const Message*> result = evaluation_result.GetMessages();

  auto true_proto = ParseFromString<typename TypeParam::Boolean>("value: true");
  auto false_proto =
      ParseFromString<typename TypeParam::Boolean>("value: false");
  auto integer_1_proto =
      ParseFromString<typename TypeParam::Integer>("value: 1");
  auto integer_2_proto =
      ParseFromString<typename TypeParam::Integer>("value: 2");
  auto integer_3_proto =
      ParseFromString<typename TypeParam::Integer>("value: 3");

  ASSERT_THAT(
      result,
      UnorderedElementsAreArray(
          {EqualsProto(true_proto), EqualsProto(false_proto),
           EqualsProto(false_proto), EqualsProto(integer_1_proto),
           EqualsProto(integer_2_proto), EqualsProto(integer_3_proto)}));
}

TYPED_TEST(FhirPathTest, TestSelectEmptyResult) {
  EXPECT_THAT(TestFixture::Evaluate("{}.select(true)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("(1 | 2 | 3).select({})"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestSelectValidatesArguments) {
  EXPECT_THAT(TestFixture::Evaluate("{}.select()"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("{}.select(true)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{}.select(true, false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestIif) {
  EXPECT_THAT(TestFixture::Evaluate("{}.iif(true, foo)"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // 2 parameter invocations
  EXPECT_THAT(TestFixture::Evaluate("iif(true, 1)"), EvalsToInteger(1));
  EXPECT_THAT(TestFixture::Evaluate("iif(false, 1)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("iif({}, 1)"), EvalsToEmpty());

  // 3 parameters invocations
  EXPECT_THAT(TestFixture::Evaluate("iif(true, 1, 2)"), EvalsToInteger(1));
  EXPECT_THAT(TestFixture::Evaluate("iif(false, 1, 2)"), EvalsToInteger(2));
  EXPECT_THAT(TestFixture::Evaluate("iif({}, 1, 2)"), EvalsToInteger(2));

  EXPECT_THAT(TestFixture::Evaluate("{}.iif(true, false)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("(1 | 2).iif(true, false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestIifValidatesArguments) {
  EXPECT_THAT(TestFixture::Evaluate("{}.iif()"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("{}.iif(true)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("{}.iif(true, false, true, false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestXor) {
  EXPECT_THAT(TestFixture::Evaluate("(true xor true) = false"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(true xor false) = true"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(true xor {})"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("(false xor true) = true"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(false xor false) = false"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("(false xor {})"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("({} xor true)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("({} xor false)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("({} xor {})"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestMultiOrShortCircuit) {
  auto no_end_period = ParseFromString<typename TypeParam::Period>(R"pb(
    start: { value_us: 1556750000000 timezone: "America/Los_Angeles" }
  )pb");

  EXPECT_THAT(
      TestFixture::Evaluate(
          no_end_period,
          "start.hasValue().not() or end.hasValue().not() or start <= end"),
      EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TestOr) {
  EXPECT_THAT(TestFixture::Evaluate("true or true"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true or false"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true or {}"), EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("false or true"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("false or false"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("false or {}"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("{} or true"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("{} or false"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{} or {}"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestAnd) {
  EXPECT_THAT(TestFixture::Evaluate("true and true"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("true and false"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("true and {}"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("false and true"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("false and false"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("false and {}"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("{} and true"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{} and false"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("{} and {}"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestSingletonEvaluationOfCollections) {
  EXPECT_THAT(TestFixture::Evaluate("'string' and true"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'string' and false"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("'string' or false"), EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("1 and true"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1 and false"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("1 or false"), EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TestEmptyLiteral) {
  EXPECT_THAT(TestFixture::Evaluate("{}"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestBooleanLiteral) {
  EXPECT_THAT(TestFixture::Evaluate("true"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("false"), EvalsToFalse());
}

TYPED_TEST(FhirPathTest, TestIntegerLiteral) {
  EXPECT_THAT(TestFixture::Evaluate("42"), EvalsToInteger(42));

  // Ensure evaluation of an out-of-range literal fails.
  const char* overflow_value = "10000000000";
  absl::Status bad_int_status =
      TestFixture::Compile(TypeParam::Encounter::descriptor(), overflow_value)
          .status();

  EXPECT_FALSE(bad_int_status.ok());
  // Failure message should contain the bad string.
  EXPECT_TRUE(bad_int_status.message().find(overflow_value) !=
              std::string::npos);
}

TYPED_TEST(FhirPathTest, TestPolarityOperator) {
  EXPECT_THAT(TestFixture::Evaluate("+1 = 1"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("-(+1) = -1"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("+(-1) = -1"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("-(-1) = 1"), EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("+1.2 = 1.2"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("-(+1.2) = -1.2"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("+(-1.2) = -1.2"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("-(-1.2) = 1.2"), EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate("+{}"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("-{}"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("+(1 | 2)"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestIntegerAddition) {
  EXPECT_THAT(TestFixture::Evaluate("(2 + 3) = 5"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("({} + 3)"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("(2 + {})"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestStringAddition) {
  EXPECT_THAT(TestFixture::Evaluate("('foo' + 'bar') = 'foobar'"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("({} + 'bar')"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("('foo' + {})"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestStringAdditionWithCodeEnum) {
  auto encounter = ValidEncounter<typename TypeParam::Encounter>();
  EXPECT_THAT(TestFixture::Evaluate(encounter, "status + 'foo'"),
              EvalsToStringThatMatches(StrEq("plannedfoo")));
}

TYPED_TEST(FhirPathTest, TestStringConcatenation) {
  EXPECT_THAT(TestFixture::Evaluate("('foo' & 'bar')"),
              EvalsToStringThatMatches(StrEq("foobar")));
  EXPECT_THAT(TestFixture::Evaluate("{} & 'bar'"),
              EvalsToStringThatMatches(StrEq("bar")));
  EXPECT_THAT(TestFixture::Evaluate("'foo' & {}"),
              EvalsToStringThatMatches(StrEq("foo")));
  EXPECT_THAT(TestFixture::Evaluate("{} & {}"),
              EvalsToStringThatMatches(StrEq("")));
}

TYPED_TEST(FhirPathTest, TestStringConcatenationWithCodeEnum) {
  auto encounter = ValidEncounter<typename TypeParam::Encounter>();
  EXPECT_THAT(TestFixture::Evaluate(encounter, "status & 'foo'"),
              EvalsToStringThatMatches(StrEq("plannedfoo")));
}

TYPED_TEST(FhirPathTest, TestEmptyComparisons) {
  EXPECT_THAT(TestFixture::Evaluate("{} = 42"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("42 = {}"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{} = {}"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("{} != 42"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("42 != {}"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{} != {}"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("{} < 42"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("42 < {}"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{} < {}"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("{} > 42"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("42 > {}"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{} > {}"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("{} >= 42"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("42 >= {}"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{} >= {}"), EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate("{} <= 42"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("42 <= {}"), EvalsToEmpty());
  EXPECT_THAT(TestFixture::Evaluate("{} <= {}"), EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TestIntegerComparisons) {
  EXPECT_THAT(TestFixture::Evaluate("42 = 42"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("42 = 43"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("42 != 43"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("42 != 42"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("42 < 43"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("42 < 42"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("43 > 42"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("42 > 42"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("42 >= 42"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("43 >= 42"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("42 >= 43"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("42 <= 42"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("42 <= 43"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("43 <= 42"), EvalsToFalse());
}

TYPED_TEST(FhirPathTest, TestIntegerLikeComparison) {
  auto parameters = ParseFromString<typename TypeParam::Parameters>(R"pb(
    parameter { value { integer { value: -1 } } }
    parameter { value { integer { value: 0 } } }
    parameter { value { integer { value: 1 } } }
    parameter { value { unsigned_int { value: 0 } } }
  )pb");

  // lhs = -1 (signed), rhs = 0 (unsigned)
  EXPECT_THAT(TestFixture::Evaluate(parameters,
                                    "parameter[0].value < parameter[3].value"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate(parameters,
                                    "parameter[0].value <= parameter[3].value"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate(parameters,
                                    "parameter[0].value >= parameter[3].value"),
              EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate(parameters,
                                    "parameter[0].value > parameter[3].value"),
              EvalsToFalse());

  // lhs = 0 (signed), rhs = 0 (unsigned)
  EXPECT_THAT(TestFixture::Evaluate(parameters,
                                    "parameter[1].value < parameter[3].value"),
              EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate(parameters,
                                    "parameter[1].value <= parameter[3].value"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate(parameters,
                                    "parameter[1].value >= parameter[3].value"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate(parameters,
                                    "parameter[1].value > parameter[3].value"),
              EvalsToFalse());

  // lhs = 1 (signed), rhs = 0 (unsigned)
  EXPECT_THAT(TestFixture::Evaluate(parameters,
                                    "parameter[2].value < parameter[3].value"),
              EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate(parameters,
                                    "parameter[2].value <= parameter[3].value"),
              EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate(parameters,
                                    "parameter[2].value >= parameter[3].value"),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate(parameters,
                                    "parameter[2].value > parameter[3].value"),
              EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TestDecimalLiteral) {
  EvaluationResult result = TestFixture::Evaluate("1.25").value();
  EXPECT_EQ("1.25", result.GetDecimal().value());
}

TYPED_TEST(FhirPathTest, TestDecimalComparisons) {
  EXPECT_THAT(TestFixture::Evaluate("1.25 = 1.25"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.25 = 1.3"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("1.25 != 1.26"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.25 != 1.25"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("1.25 < 1.26"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1 < 1.26"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.25 < 1.25"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("1.26 > 1.25"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.26 > 1"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.25 > 1.25"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("1.25 >= 1.25"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.25 >= 1"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.26 >= 1.25"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.25 >= 1.26"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("1.25 <= 1.25"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.25 <= 1.26"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("1.26 <= 1.25"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate("1.26 <= 1"), EvalsToFalse());
}

TYPED_TEST(FhirPathTest, TestStringLiteral) {
  EXPECT_THAT(TestFixture::Evaluate("'foo'"),
              EvalsToStringThatMatches(StrEq("foo")));
}

TYPED_TEST(FhirPathTest, TestStringLiteralEscaping) {
  EXPECT_THAT(TestFixture::Evaluate("'\\\\'"),
              EvalsToStringThatMatches(StrEq("\\")));
  EXPECT_THAT(TestFixture::Evaluate("'\\f'"),
              EvalsToStringThatMatches(StrEq("\f")));
  EXPECT_THAT(TestFixture::Evaluate("'\\n'"),
              EvalsToStringThatMatches(StrEq("\n")));
  EXPECT_THAT(TestFixture::Evaluate("'\\r'"),
              EvalsToStringThatMatches(StrEq("\r")));
  EXPECT_THAT(TestFixture::Evaluate("'\\t'"),
              EvalsToStringThatMatches(StrEq("\t")));
  EXPECT_THAT(TestFixture::Evaluate("'\\\"'"),
              EvalsToStringThatMatches(StrEq("\"")));
  EXPECT_THAT(TestFixture::Evaluate("'\\\''"),
              EvalsToStringThatMatches(StrEq("'")));
  EXPECT_THAT(TestFixture::Evaluate("'\\t'"),
              EvalsToStringThatMatches(StrEq("\t")));
  EXPECT_THAT(TestFixture::Evaluate("'\\u0020'"),
              EvalsToStringThatMatches(StrEq(" ")));

  // Escape sequences that should be ignored (but are not currently.)
  // TODO(b/154666440): These sequences should not be unescaped.
  EXPECT_THAT(TestFixture::Evaluate("'\\x20'"),
              EvalsToStringThatMatches(StrEq(" ")));
  EXPECT_THAT(TestFixture::Evaluate("'\\123'"),
              EvalsToStringThatMatches(StrEq("S")));
  EXPECT_THAT(TestFixture::Evaluate("'\\x00000020'"),
              EvalsToStringThatMatches(StrEq(" ")));
}

TYPED_TEST(FhirPathTest, TestStringComparisons) {
  EXPECT_THAT(TestFixture::Evaluate("'foo' = 'foo'"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo' = 'bar'"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("'foo' != 'bar'"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo' != 'foo'"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("'bar' < 'foo'"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo' < 'foo'"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("'foo' > 'bar'"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo' > 'foo'"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("'foo' >= 'foo'"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo' >= 'bar'"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'bar' >= 'foo'"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate("'foo' <= 'foo'"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'bar' <= 'foo'"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate("'foo' <= 'bar'"), EvalsToFalse());
}

TYPED_TEST(FhirPathTest, TestDateTimeLiteral) {
  using DateTime = typename TypeParam::DateTime;
  using DateTimePrecision = typename TypeParam::DateTime::Precision;

  DateTime millisecond_precision;
  millisecond_precision.set_value_us(1390660214559000);
  millisecond_precision.set_timezone("Z");
  millisecond_precision.set_precision(DateTime::MILLISECOND);
  EXPECT_THAT(
      TestFixture::Evaluate("@2014-01-25T14:30:14.559").value().GetMessages(),
      ElementsAreArray({EqualsProto(millisecond_precision)}));

  DateTime second_precision;
  second_precision.set_value_us(1390660214000000);
  second_precision.set_timezone("Z");
  second_precision.set_precision(DateTime::SECOND);
  EXPECT_THAT(
      TestFixture::Evaluate("@2014-01-25T14:30:14").value().GetMessages(),
      ElementsAreArray({EqualsProto(second_precision)}));

  // TODO(b/154874664): MINUTE precision should be supported.
  EXPECT_THAT(TestFixture::Evaluate("@2014-01-25T14:30"),
              HasStatusCode(StatusCode::kUnimplemented));

  // TODO(b/154874664): HOUR precision should be supported.
  EXPECT_THAT(TestFixture::Evaluate("@2014-01-25T14"),
              HasStatusCode(StatusCode::kUnimplemented));

  EXPECT_THAT(
      TestFixture::Evaluate("@2014-01-25T").value().GetMessages(),
      ElementsAreArray({EqualsProto(ToDateTime<DateTime, DateTimePrecision>(
          absl::CivilSecond(2014, 1, 25, 0, 0, 0), absl::UTCTimeZone(),
          DateTime::DAY))}));

  EXPECT_THAT(
      TestFixture::Evaluate("@2014-01T").value().GetMessages(),
      ElementsAreArray({EqualsProto(ToDateTime<DateTime, DateTimePrecision>(
          absl::CivilSecond(2014, 1, 1, 0, 0, 0), absl::UTCTimeZone(),
          DateTime::MONTH))}));

  EXPECT_THAT(
      TestFixture::Evaluate("@2014T").value().GetMessages(),
      ElementsAreArray({EqualsProto(ToDateTime<DateTime, DateTimePrecision>(
          absl::CivilSecond(2014, 1, 1, 0, 0, 0), absl::UTCTimeZone(),
          DateTime::YEAR))}));
}

TYPED_TEST(FhirPathTest, TestTimeComparisonsWithLiterals) {
  // Test cases form http://hl7.org/fhirpath/#datetime-equality
  // TODO(b/154874664): This should evaluate to true.
  EXPECT_THAT(TestFixture::Evaluate("@2012-01-01T10:30 = @2012-01-01T10:30"),
              HasStatusCode(StatusCode::kUnimplemented));
  // TODO(b/154874664): This should evaluate to false.
  EXPECT_THAT(TestFixture::Evaluate("@2012-01-01T10:30 = @2012-01-01T10:31"),
              HasStatusCode(StatusCode::kUnimplemented));
  // TODO(b/154874664): This should evaluate to empty.
  EXPECT_THAT(TestFixture::Evaluate("@2012-01-01T10:30:31 = @2012-01-01T10:30"),
              HasStatusCode(StatusCode::kUnimplemented));
  // TODO(b/154877869): This should evaluate to true.
  EXPECT_THAT(
      TestFixture::Evaluate("@2012-01-01T10:30:31.0 = @2012-01-01T10:30:31"),
      EvalsToFalse());
  EXPECT_THAT(
      TestFixture::Evaluate("@2012-01-01T10:30:31.1 = @2012-01-01T10:30:31"),
      EvalsToFalse());
  // Additional test case to cover unimplemented example above.
  // TODO(b/155126141): This should evaluate to empty.
  EXPECT_THAT(TestFixture::Evaluate("@2018-03-01T = @2018-03-01T10:30:00"),
              EvalsToFalse());
  EXPECT_THAT(
      TestFixture::Evaluate("@2018-03-01T10:30:00 = @2018-03-01T10:30:00"),
      EvalsToTrue());

  EXPECT_THAT(
      TestFixture::Evaluate(
          "@2017-11-05T01:30:00.0-04:00 < @2017-11-05T01:15:00.0-05:00"),
      EvalsToTrue());
  EXPECT_THAT(
      TestFixture::Evaluate(
          "@2017-11-05T01:30:00.0-04:00 > @2017-11-05T01:15:00.0-05:00"),
      EvalsToFalse());
  // TODO(b/154874878): This should evaluate to true.
  EXPECT_THAT(
      TestFixture::Evaluate(
          "@2017-11-05T01:30:00.0-04:00 = @2017-11-05T00:30:00.0-05:00"),
      EvalsToFalse());
  EXPECT_THAT(
      TestFixture::Evaluate(
          "@2017-11-05T01:30:00.0-04:00 = @2017-11-05T01:15:00.0-05:00"),
      EvalsToFalse());

  // Test cases form http://hl7.org/fhirpath/#greater-than
  EXPECT_THAT(
      TestFixture::Evaluate("@2018-03-01T10:30:00 > @2018-03-01T10:00:00"),
      EvalsToTrue());
  // TODO(b/154874664): This should evaluate to empty.
  EXPECT_THAT(TestFixture::Evaluate("@2018-03-01T10 > @2018-03-01T10:30"),
              HasStatusCode(StatusCode::kUnimplemented));
  EXPECT_THAT(
      TestFixture::Evaluate("@2018-03-01T10:30:00 > @2018-03-01T10:30:00.0"),
      EvalsToFalse());
  // Additional test case to cover unimplemented example above.
  EXPECT_THAT(TestFixture::Evaluate("@2018-03-01T > @2018-03-01T10:30:00"),
              EvalsToEmpty());

  // Test cases form http://hl7.org/fhirpath/#less-than
  EXPECT_THAT(
      TestFixture::Evaluate("@2018-03-01T10:30:00 < @2018-03-01T10:00:00"),
      EvalsToFalse());
  // TODO(b/154874664): This should evaluate to empty.
  EXPECT_THAT(TestFixture::Evaluate("@2018-03-01T10 < @2018-03-01T10:30"),
              HasStatusCode(StatusCode::kUnimplemented));
  EXPECT_THAT(
      TestFixture::Evaluate("@2018-03-01T10:30:00 < @2018-03-01T10:30:00.0"),
      EvalsToFalse());
  // Additional test case to cover unimplemented example above.
  EXPECT_THAT(TestFixture::Evaluate("@2018-03-01T < @2018-03-01T10:30:00"),
              EvalsToEmpty());

  // Test cases from http://hl7.org/fhirpath/#less-or-equal
  EXPECT_THAT(
      TestFixture::Evaluate("@2018-03-01T10:30:00 <= @2018-03-01T10:00:00"),
      EvalsToFalse());
  // TODO(b/154874664): This should evaluate to empty.
  EXPECT_THAT(TestFixture::Evaluate("@2018-03-01T10 <= @2018-03-01T10:30"),
              HasStatusCode(StatusCode::kUnimplemented));
  EXPECT_THAT(
      TestFixture::Evaluate("@2018-03-01T10:30:00 <= @2018-03-01T10:30:00.0"),
      EvalsToTrue());
  // Additional test case to cover unimplemented example above.
  EXPECT_THAT(TestFixture::Evaluate("@2018-03-01T <= @2018-03-01T10:30:00"),
              EvalsToEmpty());

  // Test cases from http://hl7.org/fhirpath/#greater-or-equal
  EXPECT_THAT(
      TestFixture::Evaluate("@2018-03-01T10:30:00 >= @2018-03-01T10:00:00"),
      EvalsToTrue());
  // TODO(b/154874664): This should evaluate to empty.
  EXPECT_THAT(TestFixture::Evaluate("@2018-03-01T10 >= @2018-03-01T10:30"),
              HasStatusCode(StatusCode::kUnimplemented));
  EXPECT_THAT(
      TestFixture::Evaluate("@2018-03-01T10:30:00 >= @2018-03-01T10:30:00.0"),
      EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TimeComparison) {
  using Period = typename TypeParam::Period;

  Period start_before_end_period = ParseFromString<Period>(R"pb(
    start: { value_us: 1556750000000000 timezone: "America/Los_Angeles" }
    end: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
  )pb");
  EXPECT_THAT(TestFixture::Evaluate(start_before_end_period, "start <= end"),
              EvalsToTrue());

  Period end_before_start_period = ParseFromString<Period>(R"pb(
    start: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
    end: { value_us: 1556750000000000 timezone: "America/Los_Angeles" }
  )pb");
  EXPECT_THAT(TestFixture::Evaluate(end_before_start_period, "start <= end"),
              EvalsToFalse());

  Period dst_transition = ParseFromString<Period>(R"pb(
    # 2001-10-28T01:59:00
    start: {
      value_us: 1004248740000000
      timezone: "America/New_York"
      precision: SECOND
    }
    # 2001-10-28T01:00:00 (the 2nd 1 AM of the day)
    end: {
      value_us: 1004248800000000
      timezone: "America/New_York"
      precision: SECOND
    }
  )pb");
  EXPECT_THAT(TestFixture::Evaluate(dst_transition, "start <= end"),
              EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TimeComparison_Instant) {
  auto observation = ParseFromString<typename TypeParam::Observation>(R"pb(
    # 2001-10-28T01:59:00Z
    meta: {
      last_updated: {
        value_us: 1004234340000000
        timezone: "UTC"
        precision: MICROSECOND
      }
    }
  )pb");
  EXPECT_THAT(
      TestFixture::Evaluate(observation, "meta.lastUpdated = meta.lastUpdated"),
      EvalsToTrue());
  EXPECT_THAT(
      TestFixture::Evaluate(observation, "meta.lastUpdated < meta.lastUpdated"),
      EvalsToFalse());
  EXPECT_THAT(
      TestFixture::Evaluate(observation,
                            "meta.lastUpdated < @2001-10-28T01:59:10Z and "
                            "@2001-10-28T01:58:50 < meta.lastUpdated"),
      EvalsToTrue());
}

TYPED_TEST(FhirPathTest, TimeCompareDifferentPrecision) {
  using DateTime = typename TypeParam::DateTime;
  using DateTimePrecision = typename TypeParam::DateTime::Precision;
  using Period = typename TypeParam::Period;

  absl::TimeZone zone;
  absl::LoadTimeZone("America/Los_Angeles", &zone);

  // Ensure comparison returns false on fine-grained checks but true
  // on corresponding coarse-grained checks.
  EXPECT_THAT(TestFixture::Evaluate(
                  CreatePeriod<Period, DateTime>(
                      ToDateTime<DateTime, DateTimePrecision>(
                          absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                          DateTime::SECOND),
                      ToDateTime<DateTime, DateTimePrecision>(
                          absl::CivilDay(2019, 5, 2), zone, DateTime::SECOND)),
                  "start <= end"),
              EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate(
                  CreatePeriod<Period, DateTime>(
                      ToDateTime<DateTime, DateTimePrecision>(
                          absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                          DateTime::SECOND),
                      ToDateTime<DateTime, DateTimePrecision>(
                          absl::CivilDay(2019, 5, 2), zone, DateTime::DAY)),
                  "start <= end"),
              EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate(
                  CreatePeriod<Period, DateTime>(
                      ToDateTime<DateTime, DateTimePrecision>(
                          absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                          DateTime::SECOND),
                      ToDateTime<DateTime, DateTimePrecision>(
                          absl::CivilDay(2019, 5, 1), zone, DateTime::DAY)),
                  "start <= end"),
              EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate(
                  CreatePeriod<Period, DateTime>(
                      ToDateTime<DateTime, DateTimePrecision>(
                          absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                          DateTime::SECOND),
                      ToDateTime<DateTime, DateTimePrecision>(
                          absl::CivilDay(2019, 5, 1), zone, DateTime::MONTH)),
                  "start <= end"),
              EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate(
                  CreatePeriod<Period, DateTime>(
                      ToDateTime<DateTime, DateTimePrecision>(
                          absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                          DateTime::SECOND),
                      ToDateTime<DateTime, DateTimePrecision>(
                          absl::CivilDay(2019, 1, 1), zone, DateTime::MONTH)),
                  "start <= end"),
              EvalsToEmpty());

  EXPECT_THAT(TestFixture::Evaluate(
                  CreatePeriod<Period, DateTime>(
                      ToDateTime<DateTime, DateTimePrecision>(
                          absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                          DateTime::SECOND),
                      ToDateTime<DateTime, DateTimePrecision>(
                          absl::CivilDay(2019, 1, 1), zone, DateTime::YEAR)),
                  "start <= end"),
              EvalsToEmpty());
}

TYPED_TEST(FhirPathTest, TimeCompareMicroseconds) {
  using DateTime = typename TypeParam::DateTime;
  using Period = typename TypeParam::Period;

  // Test edge case for very high precision comparisons.
  DateTime start_micros;
  start_micros.set_value_us(1556750000000011);
  start_micros.set_timezone("America/Los_Angeles");
  start_micros.set_precision(DateTime::MICROSECOND);

  DateTime end_micros = start_micros;
  EXPECT_THAT(TestFixture::Evaluate(
                  CreatePeriod<Period, DateTime>(start_micros, end_micros),
                  "start <= end"),
              EvalsToTrue());

  end_micros.set_value_us(end_micros.value_us() - 1);
  EXPECT_THAT(TestFixture::Evaluate(
                  CreatePeriod<Period, DateTime>(start_micros, end_micros),
                  "start <= end"),
              EvalsToFalse());
}

TYPED_TEST(FhirPathTest, SimpleQuantityComparisons) {
  auto range = ParseFromString<typename TypeParam::Range>(R"pb(
    low {
      value { value: "1.1" }
      system { value: "http://valuesystem.example.org/foo" }
      code { value: "bar" }
    }
    high {
      value { value: "1.2" }
      system { value: "http://valuesystem.example.org/foo" }
      code { value: "bar" }
    }
  )pb");

  EXPECT_THAT(TestFixture::Evaluate(range, "low < low"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate(range, "low <= low"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate(range, "low >= low"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate(range, "low > low"), EvalsToFalse());

  EXPECT_THAT(TestFixture::Evaluate(range, "high < low"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate(range, "high <= low"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate(range, "high >= low"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate(range, "high > low"), EvalsToTrue());

  EXPECT_THAT(TestFixture::Evaluate(range, "low < high"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate(range, "low <= high"), EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate(range, "low >= high"), EvalsToFalse());
  EXPECT_THAT(TestFixture::Evaluate(range, "low > high"), EvalsToFalse());

  // Different quantity codes
  auto range_different_codes = ParseFromString<typename TypeParam::Range>(R"pb(
    low {
      value { value: "1.1" }
      system { value: "http://valuesystem.example.org/foo" }
      code { value: "bar" }
    }
    high {
      value { value: "1.1" }
      system { value: "http://valuesystem.example.org/foo" }
      code { value: "different" }
    }
  )pb");
  EXPECT_THAT(TestFixture::Evaluate(range_different_codes, "low > high"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Different quantity systems
  auto range_different_systems =
      ParseFromString<typename TypeParam::Range>(R"pb(
        low {
          value { value: "1.1" }
          system { value: "http://valuesystem.example.org/foo" }
          code { value: "bar" }
        }
        high {
          value { value: "1.1" }
          system { value: "http://valuesystem.example.org/different" }
          code { value: "bar" }
        }
      )pb");
  EXPECT_THAT(TestFixture::Evaluate(range_different_systems, "low > high"),
              HasStatusCode(StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathTest, TestCompareEnumToString) {
  auto encounter = ValidEncounter<typename TypeParam::Encounter>();

  EXPECT_THAT(TestFixture::Evaluate(encounter, "status = 'planned'"),
              EvalsToTrue());

  encounter.mutable_status()->set_value(
      TypeParam::EncounterStatusCode::CANCELLED);
  EXPECT_THAT(TestFixture::Evaluate(encounter, "status = 'planned'"),
              EvalsToFalse());
}

TYPED_TEST(FhirPathTest, PathNavigationAfterContainedResourceAndValueX) {
  auto bundle = ParseFromString<typename TypeParam::Bundle>(
      R"pb(entry: {
             resource: { patient: { deceased: { boolean: { value: true } } } }
           })pb");

  auto expected = ParseFromString<typename TypeParam::Boolean>("value: true");

  FHIR_ASSERT_OK_AND_ASSIGN(
      auto result, TestFixture::Evaluate(bundle, "entry[0].resource.deceased"));
  EXPECT_THAT(result.GetMessages(), ElementsAreArray({EqualsProto(expected)}));
}

TEST(FhirPathTest, PathNavigationAfterContainedResourceR4Any) {
  auto contained = ParseFromString<r4::core::ContainedResource>(
      "observation { value: { string_value: { value: 'bar' } } } ");
  auto patient = ParseFromString<r4::core::Patient>(
      "deceased: { boolean: { value: true } }");
  patient.add_contained()->PackFrom(contained);

  EXPECT_THAT(FhirPathTest<R4CoreTestEnv>::Evaluate(patient, "contained.value"),
              EvalsToStringThatMatches(StrEq("bar")));
}

TYPED_TEST(FhirPathTest, ResourceReference) {
  auto bundle = ParseFromString<typename TypeParam::Bundle>(
      R"pb(entry: {
             resource: { patient: { deceased: { boolean: { value: true } } } }
           }
           entry: {
             resource: {
               observation: { value: { string_value: { value: "foo" } } }
             }
           }
           entry: {
             resource: {
               bundle: {
                 entry: {
                   resource: {
                     observation: { value: { string_value: { value: "bar" } } }
                   }
                 }
               }
             }
           })pb");

  EXPECT_THAT(TestFixture::Evaluate(bundle, "%resource").value().GetMessages(),
              ElementsAreArray({EqualsProto(bundle)}));

  EXPECT_THAT(
      TestFixture::Evaluate(bundle, "entry[0].resource.select(%resource)")
          .value()
          .GetMessages(),
      ElementsAreArray({EqualsProto(bundle.entry(0).resource().patient())}));

  EXPECT_THAT(
      TestFixture::Evaluate(
          bundle, "entry[0].resource.select(%resource).select(%resource)")
          .value()
          .GetMessages(),
      ElementsAreArray({EqualsProto(bundle.entry(0).resource().patient())}));

  EXPECT_THAT(
      TestFixture::Evaluate(bundle,
                            "entry[0].resource.deceased.select(%resource)")
          .value()
          .GetMessages(),
      ElementsAreArray({EqualsProto(bundle.entry(0).resource().patient())}));

  EXPECT_THAT(
      TestFixture::Evaluate(bundle, "entry[1].resource.select(%resource)")
          .value()
          .GetMessages(),
      ElementsAreArray(
          {EqualsProto(bundle.entry(1).resource().observation())}));

  EXPECT_THAT(
      TestFixture::Evaluate(bundle, "entry[1].resource.value.select(%resource)")
          .value()
          .GetMessages(),
      ElementsAreArray(
          {EqualsProto(bundle.entry(1).resource().observation())}));

  EXPECT_THAT(
      TestFixture::Evaluate(bundle, "entry[2].resource.select(%resource)")
          .value()
          .GetMessages(),
      ElementsAreArray({EqualsProto(bundle.entry(2).resource().bundle())}));

  EXPECT_THAT(
      TestFixture::Evaluate(
          bundle, "entry[2].resource.entry[0].resource.select(%resource)")
          .value()
          .GetMessages(),
      ElementsAreArray({EqualsProto(bundle.entry(2)
                                        .resource()
                                        .bundle()
                                        .entry(0)
                                        .resource()
                                        .observation())}));

  EXPECT_THAT(TestFixture::Evaluate(bundle, "entry.resource.select(%resource)")
                  .value()
                  .GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(bundle.entry(0).resource().patient()),
                   EqualsProto(bundle.entry(1).resource().observation()),
                   EqualsProto(bundle.entry(2).resource().bundle())}));

  // Note: The spec states that %resources resolves to "the resource that
  // contains the original node that is in %context." Given that the literal
  // 'true' is not contained by any resources it is believed that this should
  // result in an error.
  EXPECT_EQ(TestFixture::Evaluate(bundle, "true.select(%resource)")
                .status()
                .message(),
            "No Resource found in ancestry.");

  // Likewise, derived values do not have a defined %resource.
  EXPECT_EQ(
      TestFixture::Evaluate(bundle,
                            "(entry[2].resource.entry[0].resource.value & "
                            "entry[1].resource.value).select(%resource)")
          .status()
          .message(),
      "No Resource found in ancestry.");

  EXPECT_THAT(TestFixture::Evaluate(
                  bundle,
                  "(entry[2].resource.entry[0].resource.value | "
                  "entry[1].resource.value | %resource).select(%resource)")
                  .value()
                  .GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(bundle.entry(2)
                                   .resource()
                                   .bundle()
                                   .entry(0)
                                   .resource()
                                   .observation()),
                   EqualsProto(bundle.entry(1).resource().observation()),
                   EqualsProto(bundle)}));
}

TYPED_TEST(FhirPathTest, MemberOfWithoutTerminologyResolver) {
  auto obs = ParseFromString<typename TypeParam::Observation>(R"(
      code {
        coding {
          code { value: "L" }
        }
      })");

  EXPECT_THAT(
      TestFixture::Evaluate(
          obs, "code.memberOf('http://hl7.org/fhir/ValueSet/FHIR-version ')"),
      HasStatusCode(absl::StatusCode::kFailedPrecondition));
}

const char kAllergyValueSetUrl[] =
    "http://terminology.hl7.org/ValueSet/allergyintolerance-clinical";

const char kAllergyCodeSystemUrl[] =
    "http://terminology.hl7.org/CodeSystem/allergyintolerance-clinical";

const char kRangeValueSetUrl[] = "http://terminology.hl7.org/ValueSet/Range";

// For testing value set related functions.
template <typename T>
class FhirPathTestForValueSets : public FhirPathTest<T> {
 protected:
  // TODO(b/204365134): Refactor this to use an in-memory resolver using real
  // system values.
  class FakeTerminologyResolver : public terminology::TerminologyResolver {
   public:
    absl::StatusOr<absl::flat_hash_set<std::string>> GetCodeStringsInCodeSystem(
        absl::string_view code_system_url) const override {
      return absl::UnimplementedError(
          "GetCodeStringsInCodeSystem not implemented.");
    }

    absl::StatusOr<bool> IsCodeInCodeSystem(
        absl::string_view code_value,
        absl::string_view code_system_url) const override {
      return absl::UnimplementedError("IsInCodeSystem not implemented.");
    }

    absl::StatusOr<absl::flat_hash_set<std::string>> GetCodeStringsInValueSet(
        absl::string_view value_set_url) const override {
      return absl::UnimplementedError(
          "GetCodeStringsInValueSet not implemented.");
    }

    absl::StatusOr<bool> IsCodeInValueSet(
        absl::string_view code_value,
        absl::string_view value_set_url) const override {
      if (value_set_url == kAllergyValueSetUrl) {
        return absl::EndsWith(code_value, "nuts");
      }
      if (value_set_url == kRangeValueSetUrl) {
        return code_value == "H" || code_value == "L";
      }
      return absl::NotFoundError("");
    }

    absl::StatusOr<bool> IsCodeInValueSet(
        absl::string_view code_value, absl::string_view code_system_url,
        absl::string_view value_set_url) const override {
      if (value_set_url == kAllergyValueSetUrl) {
        if (code_system_url != kAllergyCodeSystemUrl) {
          return false;
        }
        return absl::EndsWith(code_value, "nuts");
      }
      if (value_set_url == kRangeValueSetUrl) {
        return code_value == "H" || code_value == "L";
      }
      return absl::NotFoundError("");
    }

    absl::StatusOr<bool> ValueSetHasMultipleCodeSystems(
        absl::string_view value_set_url) const override {
      if (value_set_url == kAllergyValueSetUrl) {
        return true;
      }
      if (value_set_url == kRangeValueSetUrl) {
        return false;
      }
      return absl::NotFoundError("");
    };
  };

  absl::StatusOr<EvaluationResult> Evaluate(const Message& message,
                                            const std::string& expression) {
    FHIR_ASSIGN_OR_RETURN(auto compiled_expression,
                          Compile(message.GetDescriptor(), expression));

    auto result = compiled_expression.Evaluate(message);
    CHECK(!result.ok() ||
          (result.ok() && !result.value().CallbackFunctionName().has_value()));
    return result;
  }

 private:
  FakeTerminologyResolver fake_terminology_resolver_;

  absl::StatusOr<CompiledExpression> Compile(
      const ::google::protobuf::Descriptor* descriptor, const std::string& fhir_path) {
    return CompiledExpression::Compile(descriptor,
                                       T::PrimitiveHandler::GetInstance(),
                                       fhir_path, &fake_terminology_resolver_);
  }
};
TYPED_TEST_SUITE(FhirPathTestForValueSets, AllVersionEnvs);

TYPED_TEST(FhirPathTestForValueSets, MemberOf) {
  auto obs = ParseFromString<typename TypeParam::Observation>(R"pb(
    category {
      coding { code { value: "something else" } }
      coding {
        # system not specified (expect to match)
        code { value: "Peanuts" }
      }
      coding {
        system { value: "wrong_system" }
        code { value: "Walnuts" }
      }
      coding {
        system {
          value: "http://terminology.hl7.org/CodeSystem/allergyintolerance-clinical"
        }
        code { value: "Hazelnuts" }
      }
    }
    code {
      coding { code { value: "L" } }
      text { value: "H" }
    }
    reference_range {}
  )pb");
  auto true_obj = ParseFromString<typename TypeParam::Boolean>("value: true");
  auto false_obj = ParseFromString<typename TypeParam::Boolean>("value: false");

  // memberOf() invoked on "String" objects, the "Range" value set only has
  // one code system in our fake value set repository, so this is OK.
  EXPECT_THAT(
      TestFixture::Evaluate(
          obs, absl::Substitute("code.text.memberOf('$0')", kRangeValueSetUrl)),
      IsOkWithMessages(std::vector<typename TypeParam::Boolean>{true_obj}));

  // The "Allergies" value set has 2 code systems in our fake repository,
  // so evaluating memberOf() gives an error.
  EXPECT_THAT(
      TestFixture::Evaluate(obs, absl::Substitute("code.text.memberOf('$0')",
                                                  kAllergyValueSetUrl)),
      HasStatusCode(absl::StatusCode::kFailedPrecondition));

  // The "not_a_value_set" is not specified in the fake repository, hence
  // memberOf gives a not found error.
  EXPECT_THAT(
      TestFixture::Evaluate(obs, "code.text.memberOf('not_a_value_set')"),
      HasStatusCode(absl::StatusCode::kNotFound));

  // memberOf() cannot be invoked on "ReferenceRange" objects.
  EXPECT_THAT(TestFixture::Evaluate(
                  obs, absl::Substitute("referenceRange.memberOf('$0')",
                                        kRangeValueSetUrl)),
              HasStatusCode(absl::StatusCode::kInvalidArgument));

  // memberOf() invoked on collection of "Code" objects.
  EXPECT_THAT(TestFixture::Evaluate(
                  obs, absl::Substitute("category.coding.code.memberOf('$0')",
                                        kAllergyValueSetUrl)),
              IsOkWithMessages(std::vector<typename TypeParam::Boolean>{
                  false_obj, true_obj, true_obj, true_obj}));
  EXPECT_THAT(TestFixture::Evaluate(
                  obs, "category.coding.code.memberOf('other_value_set')"),
              HasStatusCode(absl::StatusCode::kNotFound));

  // memberOf() cannot be invoked on "Coding" objects.
  EXPECT_THAT(TestFixture::Evaluate(
                  obs, absl::Substitute("category.coding.memberOf('$0')",
                                        kAllergyValueSetUrl)),
              HasStatusCode(absl::StatusCode::kInvalidArgument));

  // memberOf() invoked on "CodeableConcept" objects.
  EXPECT_THAT(
      TestFixture::Evaluate(obs, absl::Substitute("category.memberOf('$0')",
                                                  kAllergyValueSetUrl)),
      IsOkWithMessages(std::vector<typename TypeParam::Boolean>{true_obj}));
  EXPECT_THAT(
      TestFixture::Evaluate(
          obs, absl::Substitute("category.memberOf('$0')", kRangeValueSetUrl)),
      IsOkWithMessages(std::vector<typename TypeParam::Boolean>{false_obj}));
}

// For testing user-defined functions.
template <typename T>
class FhirPathUserDefinedFnsTest : public ::testing::Test {
 public:
  static absl::StatusOr<CompiledExpression> Compile(
      const ::google::protobuf::Descriptor* descriptor, const std::string& fhir_path,
      const std::vector<UserDefinedFunction*>& functions) {
    return CompiledExpression::Compile(descriptor,
                                       T::PrimitiveHandler::GetInstance(),
                                       fhir_path, nullptr, functions);
  }

  template <typename R>
  static absl::StatusOr<EvaluationResult> Evaluate(
      const R& message, const std::string& expression,
      const std::vector<UserDefinedFunction*>& functions) {
    FHIR_ASSIGN_OR_RETURN(
        auto compiled_expression,
        Compile(message.GetDescriptor(), expression, functions));

    return compiled_expression.Evaluate(message);
  }

  static absl::StatusOr<EvaluationResult> Evaluate(
      const std::string& expression,
      const std::vector<UserDefinedFunction*>& functions) {
    // FHIRPath assumes a resource object during evaluation, so we use an
    // encounter as a placeholder.
    auto test_encounter = ValidEncounter<typename T::Encounter>();
    return Evaluate<typename T::Encounter>(test_encounter, expression,
                                           functions);
  }

  // foo() : String
  //   Simply returns the string "foo".
  class FooFunction : public UserDefinedFunction {
   public:
    explicit FooFunction(const std::string& name)
        : UserDefinedFunction(name, {}) {}
    absl::Status ValidateParams(
        std::vector<std::shared_ptr<internal::ExpressionNode>>& params)
        const override {
      if (!params.empty()) {
        return absl::InvalidArgumentError("foo() accepts zero arguments");
      }
      return absl::OkStatus();
    }

    absl::Status Evaluate(
        internal::WorkSpace* work_space,
        const std::vector<internal::WorkspaceMessage>& child_results,
        std::vector<internal::WorkspaceMessage>* results,
        const std::vector<std::shared_ptr<internal::ExpressionNode>>& params...)
        const override {
      Message* result = work_space->GetPrimitiveHandler()->NewString("foo");
      work_space->DeleteWhenFinished(result);
      results->push_back(internal::WorkspaceMessage(result));
      return absl::OkStatus();
    }

    const ::google::protobuf::Descriptor* ReturnType() const override {
      return google::fhir::r4::core::String::descriptor();
    }
  };

  // has(field : identifier): Boolean
  //   Checks if an object has a given field.
  class HasFunction : public UserDefinedFunction {
   public:
    HasFunction()
        : UserDefinedFunction(
              "has", {UserDefinedFunction::ParameterVisitorType::kIdentifier}) {
    }
    absl::Status ValidateParams(
        std::vector<std::shared_ptr<internal::ExpressionNode>>& params)
        const override {
      if (params.size() != 1) {
        return absl::InvalidArgumentError("has() requires 1 argument");
      }
      return absl::OkStatus();
    }

    absl::Status Evaluate(
        internal::WorkSpace* work_space,
        const std::vector<internal::WorkspaceMessage>& child_results,
        std::vector<internal::WorkspaceMessage>* results,
        const std::vector<std::shared_ptr<internal::ExpressionNode>>& params...)
        const override {
      if (child_results.size() > 1) {
        return absl::InvalidArgumentError(
            "Function must be invoked on a single object.");
      }
      if (child_results.empty()) {
        return absl::OkStatus();
      }

      std::vector<internal::WorkspaceMessage> tmp_results;
      absl::Status status = params[0]->Evaluate(work_space, &tmp_results);
      FHIR_RETURN_IF_ERROR(status);
      std::string field;
      FHIR_ASSIGN_OR_RETURN(
          field, GetPrimitiveStringValue(*tmp_results[0].Message(), &field));

      const Descriptor* descriptor =
          child_results[0].Message()->GetDescriptor();
      Message* result = work_space->GetPrimitiveHandler()->NewBoolean(
          internal::HasFieldWithJsonName(descriptor, field));
      work_space->DeleteWhenFinished(result);
      results->push_back(internal::WorkspaceMessage(result));
      return absl::OkStatus();
    }

    const ::google::protobuf::Descriptor* ReturnType() const override {
      return google::fhir::r4::core::String::descriptor();
    }
  };

  // resolve(): Resource
  //   Takes as input a reference and returns the resource pointed to by that
  //   reference if found.
  class ResolveFunction : public UserDefinedFunction {
   public:
    ResolveFunction() : UserDefinedFunction("resolve", {}, true) {}
    absl::Status ValidateParams(
        std::vector<std::shared_ptr<internal::ExpressionNode>>& params)
        const override {
      if (!params.empty()) {
        return absl::InvalidArgumentError("resolve() accepts zero arguments");
      }
      return absl::OkStatus();
    }

    absl::Status Evaluate(
        internal::WorkSpace* work_space,
        const std::vector<internal::WorkspaceMessage>& child_results,
        std::vector<internal::WorkspaceMessage>* results,
        const std::vector<std::shared_ptr<internal::ExpressionNode>>& params...)
        const override {
      // accept a single input for simplicity
      if (child_results.size() > 1) {
        return absl::InvalidArgumentError(
            "Function must be invoked on at most one object.");
      }
      if (child_results.empty()) {
        return absl::OkStatus();
      }
      const Descriptor* descriptor =
          child_results[0].Message()->GetDescriptor();
      if (!IsReference(descriptor)) {
        return absl::OkStatus();
      }
      FHIR_ASSIGN_OR_RETURN(
          std::optional<std::string> reference,
          ReferenceProtoToString(*child_results[0].Message()));
      if (!reference.has_value()) {
        return absl::OkStatus();
      }
      Message* reference_res =
          work_space->GetPrimitiveHandler()->NewString(reference.value());
      work_space->DeleteWhenFinished(reference_res);
      results->push_back(internal::WorkspaceMessage(reference_res));
      return absl::OkStatus();
    }

    const ::google::protobuf::Descriptor* ReturnType() const override { return nullptr; }
  };

  // reverseResolve(sourceReference : identifier): Resource
  //   Takes as input a resource and returns the resource(s) that resolve to the
  //   input resource.
  class ReverseResolveFunction : public UserDefinedFunction {
   public:
    ReverseResolveFunction()
        : UserDefinedFunction(
              "reverseResolve",
              {UserDefinedFunction::ParameterVisitorType::kIdentifier}, true) {}
    absl::Status ValidateParams(
        std::vector<std::shared_ptr<internal::ExpressionNode>>& params)
        const override {
      if (params.size() > 1) {
        return absl::InvalidArgumentError(
            "reverseResolve() requires 1 argument");
      }
      return absl::OkStatus();
    }

    absl::Status Evaluate(
        internal::WorkSpace* work_space,
        const std::vector<internal::WorkspaceMessage>& child_results,
        std::vector<internal::WorkspaceMessage>* results,
        const std::vector<std::shared_ptr<internal::ExpressionNode>>& params...)
        const override {
      if (child_results.size() > 1) {
        return absl::InvalidArgumentError(
            "Function must be invoked on at most one object.");
      }
      if (child_results.empty()) {
        return absl::OkStatus();
      }
      const Descriptor* descriptor =
          child_results[0].Message()->GetDescriptor();
      if (!IsResource(descriptor)) {
        return absl::OkStatus();
      }

      std::vector<internal::WorkspaceMessage> param_res;
      absl::Status status = params[0]->Evaluate(work_space, &param_res);
      FHIR_RETURN_IF_ERROR(status);
      std::string path;
      FHIR_ASSIGN_OR_RETURN(
          path, GetPrimitiveStringValue(*param_res[0].Message(), &path));

      Message* path_res = work_space->GetPrimitiveHandler()->NewString(path);
      work_space->DeleteWhenFinished(path_res);
      results->push_back(internal::WorkspaceMessage(path_res));
      results->push_back(internal::WorkspaceMessage(child_results[0]));
      return absl::OkStatus();
    }

    const ::google::protobuf::Descriptor* ReturnType() const override { return nullptr; }
  };
};

TYPED_TEST_SUITE(FhirPathUserDefinedFnsTest, AllVersionEnvs);

TYPED_TEST(FhirPathUserDefinedFnsTest, TestFooFunction) {
  EXPECT_THAT(TestFixture::Evaluate("foo()", {}),
              HasStatusCode(absl::StatusCode::kNotFound));

  typename TestFixture::FooFunction foo_function("foo");
  EXPECT_THAT(TestFixture::Evaluate("foo(1)", {&foo_function}),
              HasStatusCode(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(TestFixture::Evaluate("foo()", {&foo_function}),
              EvalsToStringThatMatches(StrEq("foo")));
  EXPECT_THAT(TestFixture::Evaluate("{}.foo()", {&foo_function}),
              EvalsToStringThatMatches(StrEq("foo")));
  EXPECT_THAT(TestFixture::Evaluate("'foo' = 1.foo()", {&foo_function}),
              EvalsToTrue());

  typename TestFixture::FooFunction invalid_foo_function("where");
  EXPECT_THAT(TestFixture::Evaluate("where()", {}),
              HasStatusCode(absl::StatusCode::kInvalidArgument));
}

TYPED_TEST(FhirPathUserDefinedFnsTest, TestHasFunction) {
  typename TestFixture::HasFunction has_function;
  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();
  EXPECT_THAT(TestFixture::Evaluate(test_encounter, "Encounter.has(status)",
                                    {&has_function}),
              EvalsToTrue());
  EXPECT_THAT(TestFixture::Evaluate(test_encounter, "Encounter.has(bar)",
                                    {&has_function}),
              EvalsToFalse());
}

TYPED_TEST(FhirPathUserDefinedFnsTest, TestCallbackFunction) {
  typename TestFixture::ResolveFunction resolve_function;
  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();
  test_encounter.mutable_service_provider()
      ->mutable_observation_id()
      ->set_value("123");
  FHIR_ASSERT_OK_AND_ASSIGN(
      auto compiled, TestFixture::Compile(test_encounter.GetDescriptor(),
                                          "Encounter.serviceProvider.resolve()."
                                          "ofType(Observation).where(id='123')",
                                          {&resolve_function}));
  auto result = compiled.Evaluate(test_encounter);
  FHIR_ASSERT_OK(result.status());
  ASSERT_EQ(result.value().CallbackFunctionName().value_or(""), "resolve");
  EXPECT_THAT(result, EvalsToStringThatMatches(StrEq("Observation/123")));

  auto test_observation = ValidObservation<typename TypeParam::Observation>();
  auto final_result =
      compiled.ResumeEvaluation(result.value(), {&test_observation});
  FHIR_ASSERT_OK(final_result.status());
  EXPECT_THAT(final_result.value().GetMessages(),
              UnorderedElementsAreArray({&test_observation}));
  ASSERT_FALSE(final_result.value().CallbackFunctionName().has_value());
}

TYPED_TEST(FhirPathUserDefinedFnsTest, TestCallbackAndUserDefinedFunctions) {
  auto test_encounter = ValidEncounter<typename TypeParam::Encounter>();
  test_encounter.mutable_service_provider()
      ->mutable_observation_id()
      ->set_value("1");
  test_encounter.mutable_subject()->mutable_observation_id()->set_value("2");

  typename TestFixture::ResolveFunction resolve_function;
  typename TestFixture::ReverseResolveFunction reverse_resolve_function;
  typename TestFixture::HasFunction has_function;

  // Order of callback executions for the below expression should be:
  //   subject.resolve() -> serviceProvider.resolve() -> reverseResolve()
  FHIR_ASSERT_OK_AND_ASSIGN(
      auto compiled,
      TestFixture::Compile(
          test_encounter.GetDescriptor(),
          "Encounter in (subject.resolve().where(has(id)) | "
          "serviceProvider.resolve().reverseResolve(Encounter.subject))",
          {&resolve_function, &reverse_resolve_function, &has_function}));

  auto result = compiled.Evaluate(test_encounter);
  FHIR_ASSERT_OK(result.status());
  ASSERT_EQ(result.value().CallbackFunctionName().value_or(""), "resolve");
  EXPECT_THAT(result, EvalsToStringThatMatches(StrEq("Observation/2")));

  auto test_observation2 = ValidObservation<typename TypeParam::Observation>();
  test_observation2.mutable_id()->set_value("2");

  // resume evaluation from subject.resolve()
  result = compiled.ResumeEvaluation(result.value(), {&test_observation2});
  FHIR_ASSERT_OK(result.status());
  ASSERT_EQ(result.value().CallbackFunctionName().value_or(""), "resolve");
  EXPECT_THAT(result, EvalsToStringThatMatches(StrEq("Observation/1")));

  auto test_observation1 = ValidObservation<typename TypeParam::Observation>();
  test_observation1.mutable_id()->set_value("1");

  // resume evaluation from serviceProvider.resolve()
  result = compiled.ResumeEvaluation(result.value(), {&test_observation1});
  FHIR_ASSERT_OK(result.status());
  ASSERT_EQ(result.value().CallbackFunctionName().value_or(""),
            "reverseResolve");
  EXPECT_THAT(
      result.value().GetMessages(),
      ElementsAreArray({EqualsProto(ParseFromString<typename TypeParam::String>(
                            "value: 'Encounter.subject'")),
                        EqualsProto(test_observation1)}));

  // resume evaluation from reverseResolve() - get final result
  result = compiled.ResumeEvaluation(result.value(), {&test_encounter});
  FHIR_ASSERT_OK(result.status());
  ASSERT_FALSE(result.value().CallbackFunctionName().has_value());
  EXPECT_THAT(result.value().GetMessages(),
              ElementsAreArray({
                  EqualsProto(ParseFromString<typename TypeParam::Boolean>(
                      "value: true")),
              }));

  EXPECT_THAT(compiled.ResumeEvaluation(result.value(), {}),
              HasStatusCode(absl::StatusCode::kInvalidArgument));
}

}  // namespace

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
