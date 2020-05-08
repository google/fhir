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
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/time/civil_time.h"
#include "absl/time/time.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/r4/primitive_handler.h"
#include "google/fhir/status/status.h"
#include "google/fhir/stu3/primitive_handler.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/r4/core/codes.pb.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/r4/core/resources/encounter.pb.h"
#include "proto/r4/core/resources/medication_knowledge.pb.h"
#include "proto/r4/core/resources/observation.pb.h"
#include "proto/r4/core/resources/parameters.pb.h"
#include "proto/r4/core/resources/patient.pb.h"
#include "proto/r4/core/resources/structure_definition.pb.h"
#include "proto/r4/core/resources/value_set.pb.h"
#include "proto/stu3/codes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/metadatatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "proto/stu3/uscore.pb.h"
#include "proto/stu3/uscore_codes.pb.h"

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

void PrintTo(const StatusOr<EvaluationResult>& result, std::ostream* os) {
  if (result.ok()) {
    *os << ::testing::PrintToString(result.ValueOrDie());
  } else {
    *os << "failed to evaluate (" << result.status().code()
        << ") with message \"" << result.status().message() << "\"";
  }
}

namespace {

using ::absl::InvalidArgumentError;
using ::absl::StatusCode;
using ::google::protobuf::Message;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::StrEq;
using ::testing::UnorderedElementsAreArray;
using testutil::EqualsProto;

static ::google::protobuf::TextFormat::Parser parser;  // NOLINT

#define USING(FhirNamespace) \
using FhirNamespace::Boolean; \
using FhirNamespace::Bundle; \
using FhirNamespace::Code; \
using FhirNamespace::CodeableConcept; \
using FhirNamespace::DateTime; \
using FhirNamespace::Decimal; \
using FhirNamespace::Encounter; \
using FhirNamespace::EncounterStatusCode; \
using FhirNamespace::Integer; \
using FhirNamespace::Observation; \
using FhirNamespace::Parameters; \
using FhirNamespace::Patient; \
using FhirNamespace::Period; \
using FhirNamespace::Quantity; \
using FhirNamespace::String; \
using FhirNamespace::StructureDefinition; \

#define FHIR_VERSION_TEST(CaseName, TestName, Body) \
namespace r4test { \
TEST(CaseName, TestName##R4) { \
Body \
} \
} \
namespace stu3test { \
TEST(CaseName, TestName##STU3) { \
Body \
} \
} \

MATCHER(EvalsToEmpty, "") {
  if (!arg.ok()) {
    *result_listener << "evaluation error: " << arg.status().message();
    return false;
  }

  std::vector<const Message*> results = arg.ValueOrDie().GetMessages();

  if (!results.empty()) {
    *result_listener << "has size of " << results.size();
    return false;
  }

  return true;
}

// Matcher for StatusOr<EvaluationResult> that checks to see that the evaluation
// succeeded and evaluated to a single boolean with value of false.
//
// NOTE: Not(EvalsToFalse()) is not the same as EvalsToTrue() as the former
// will match cases where evaluation fails.
MATCHER(EvalsToFalse, "") {
  if (!arg.ok()) {
    *result_listener << "evaluation error: " << arg.status().message();
    return false;
  }

  StatusOr<bool> result = arg.ValueOrDie().GetBoolean();
  if (!result.ok()) {
    *result_listener << "did not resolve to a boolean: "
                     << result.status().message();
    return false;
  }

  if (result.ValueOrDie()) {
    *result_listener << "evaluated to true";
  }

  return !result.ValueOrDie();
}

// Matcher for StatusOr<EvaluationResult> that checks to see that the evaluation
// succeeded and evaluated to a single boolean with value of true.
//
// NOTE: Not(EvalsToTrue()) is not the same as EvalsToFalse() as the former
// will match cases where evaluation fails.
MATCHER(EvalsToTrue, "") {
  if (!arg.ok()) {
    *result_listener << "evaluation error: " << arg.status().message();
    return false;
  }

  StatusOr<bool> result = arg.ValueOrDie().GetBoolean();
  if (!result.ok()) {
    *result_listener << "did not resolve to a boolean: "
                     << result.status().message();
    return false;
  }

  if (!result.ValueOrDie()) {
    *result_listener << "evaluated to false";
  }

  return result.ValueOrDie();
}

// Matcher for StatusOr<EvaluationResult> that checks to see that the evaluation
// succeeded and evaluated to an integer equal to the provided value.
MATCHER_P(EvalsToInteger, expected, "") {
  if (!arg.ok()) {
    return false;
  }

  StatusOr<int> result = arg.ValueOrDie().GetInteger();
  if (!result.ok()) {
    *result_listener << "did not resolve to a integer: "
                     << result.status().message();
    return false;
  }

  return result.ValueOrDie() == expected;
}

MATCHER_P(EvalsToStringThatMatches, string_matcher, "") {
  if (!arg.ok()) {
    *result_listener << "evaluation error: " << arg.status().message();
    return false;
  }

  StatusOr<std::string> result = arg.ValueOrDie().GetString();
  if (!result.ok()) {
    *result_listener << "did not resolve to a string: "
                     << result.status().message();
    return false;
  }

  return string_matcher.impl().MatchAndExplain(result.ValueOrDie(),
                                               result_listener);
}

// Matcher for StatusOr<T> that checks to see that a status is present with the
// provided code.
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
  return ParseFromString<T>(R"proto(
    status { value: TRIAGED }
    id { value: "123" }
    period {
      start: { value_us: 1556750153000 timezone: "America/Los_Angeles" }
    }
    status_history { status { value: ARRIVED } }
  )proto");
}

template <typename T>
T ValidObservation() {
  return ParseFromString<T>(R"proto(
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

StatusOr<const ::google::fhir::PrimitiveHandler*> GetPrimitiveHandler(
    const ::google::protobuf::Descriptor* descriptor) {
  const auto version = GetFhirVersion(descriptor);
  switch (version) {
    case proto::FhirVersion::STU3:
      return ::google::fhir::stu3::Stu3PrimitiveHandler::GetInstance();
    case proto::FhirVersion::R4:
      return ::google::fhir::r4::R4PrimitiveHandler::GetInstance();
    default:
      return InvalidArgumentError(
          absl::StrCat("Unsupported FHIR version for FhirPath: ",
                       FhirVersion_Name(version)));
  }
}

StatusOr<CompiledExpression> Compile(
      const ::google::protobuf::Descriptor* descriptor, const std::string& fhir_path) {
  FHIR_ASSIGN_OR_RETURN(auto primitive_handler,
                        GetPrimitiveHandler(descriptor));
  return CompiledExpression::Compile(descriptor, primitive_handler, fhir_path);
}

namespace r4test {
USING(::google::fhir::r4::core)

template <typename T>
StatusOr<EvaluationResult> Evaluate(const T& message,
    const std::string& expression) {
  FHIR_ASSIGN_OR_RETURN(auto compiled_expression,
                        Compile(message.GetDescriptor(), expression));

  return compiled_expression.Evaluate(message);
}

StatusOr<EvaluationResult> Evaluate(
    const std::string& expression) {
  // FHIRPath assumes a resource object during evaluation, so we use an
  // encounter as a placeholder.
  auto test_encounter = ValidEncounter<r4::core::Encounter>();
  return Evaluate<r4::core::Encounter>(test_encounter, expression);
}
}  // namespace r4test

namespace stu3test {
USING(::google::fhir::stu3::proto)

template <typename T>
StatusOr<EvaluationResult> Evaluate(const T& message,
    const std::string& expression) {
  FHIR_ASSIGN_OR_RETURN(auto compiled_expression,
                        Compile(message.GetDescriptor(), expression));

  return compiled_expression.Evaluate(message);
}

StatusOr<EvaluationResult> Evaluate(
    const std::string& expression) {
  // FHIRPath assumes a resource object during evaluation, so we use an
  // encounter as a placeholder.
  auto test_encounter = ValidEncounter<stu3::proto::Encounter>();
  return Evaluate<stu3::proto::Encounter>(test_encounter, expression);
}
}  // namespace stu3test

template <typename T, typename P>
T ToDateTime(const absl::CivilSecond civil_time,
                    const absl::TimeZone zone,
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

FHIR_VERSION_TEST(FhirPathTest, TestExternalConstants, {
  EXPECT_THAT(Evaluate("%ucum"),
              EvalsToStringThatMatches(StrEq("http://unitsofmeasure.org")));

  EXPECT_THAT(Evaluate("%sct"),
              EvalsToStringThatMatches(StrEq("http://snomed.info/sct")));

  EXPECT_THAT(Evaluate("%loinc"),
              EvalsToStringThatMatches(StrEq("http://loinc.org")));

  EXPECT_THAT(Evaluate("%unknown"), HasStatusCode(StatusCode::kNotFound));
})

FHIR_VERSION_TEST(FhirPathTest, TestExternalConstantsContext, {
  Encounter test_encounter = ValidEncounter<Encounter>();

  auto result = Evaluate(test_encounter, "%context").ValueOrDie();
  EXPECT_THAT(result.GetMessages(),
              UnorderedElementsAreArray({EqualsProto(test_encounter)}));
})

FHIR_VERSION_TEST(
    FhirPathTest, TestExternalConstantsContextReferenceInExpressionParam, {
      Encounter test_encounter = ValidEncounter<Encounter>();

      EXPECT_THAT(
          Evaluate(test_encounter, "%context").ValueOrDie().GetMessages(),
          UnorderedElementsAreArray({EqualsProto(test_encounter)}));
    })

FHIR_VERSION_TEST(FhirPathTest, TestMalformed, {
  auto expr = Compile(Encounter::descriptor(), "expression->not->valid");

  EXPECT_THAT(expr, HasStatusCode(StatusCode::kInternal));
})

FHIR_VERSION_TEST(FhirPathTest, TestGetDirectChild, {
  Encounter test_encounter = ValidEncounter<Encounter>();
  EvaluationResult result = Evaluate(test_encounter, "status").ValueOrDie();

  EXPECT_THAT(
      result.GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.status())}));
})

FHIR_VERSION_TEST(FhirPathTest, TestGetGrandchild, {
  Encounter test_encounter = ValidEncounter<Encounter>();
  EvaluationResult result =
      Evaluate(test_encounter, "period.start").ValueOrDie();

  EXPECT_THAT(result.GetMessages(), UnorderedElementsAreArray({EqualsProto(
                                        test_encounter.period().start())}));
})

FHIR_VERSION_TEST(FhirPathTest, TestGetEmptyGrandchild, {
  EXPECT_THAT(Evaluate(ValidEncounter<Encounter>(), "period.end"),
              EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestFieldExists, {
  Encounter test_encounter = ValidEncounter<Encounter>();
  test_encounter.mutable_class_value()->mutable_display()->set_value("foo");

  EvaluationResult root_result =
      Evaluate(test_encounter, "period").ValueOrDie();
  EXPECT_THAT(
      root_result.GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.period())}));

  // Tests the conversion from camelCase to snake_case
  EvaluationResult camel_case_result =
      Evaluate(test_encounter, "statusHistory").ValueOrDie();
  EXPECT_THAT(camel_case_result.GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(test_encounter.status_history(0))}));

  // Test that the json_name field annotation is used when searching for a
  // field.
  EvaluationResult json_name_alias_result =
      Evaluate(test_encounter, "class").ValueOrDie();
  EXPECT_THAT(
      json_name_alias_result.GetMessages(),
      UnorderedElementsAreArray({EqualsProto(test_encounter.class_value())}));
})

FHIR_VERSION_TEST(FhirPathTest, TestNoSuchField, {
  auto root_expr = Compile(Encounter::descriptor(), "bogusrootfield");

  EXPECT_THAT(root_expr, HasStatusCode(StatusCode::kNotFound));
  EXPECT_NE(root_expr.status().message().find("bogusrootfield"),
            std::string::npos);

  auto child_expr = Compile(Encounter::descriptor(), "period.boguschildfield");

  EXPECT_THAT(child_expr, HasStatusCode(StatusCode::kNotFound));
  EXPECT_NE(child_expr.status().message().find("boguschildfield"),
            std::string::npos);

  EXPECT_THAT(Evaluate(ValidEncounter<Encounter>(),
                       "(period | status).boguschildfield"),
              EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestNoSuchFunction, {
  auto root_expr = Compile(Encounter::descriptor(), "period.bogusfunction()");

  EXPECT_THAT(root_expr, HasStatusCode(StatusCode::kNotFound));
  EXPECT_NE(root_expr.status().message().find("bogusfunction"),
            std::string::npos);
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionTopLevelInvocation, {
  EXPECT_THAT(Evaluate("exists()"), EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionExists, {
  EXPECT_THAT(Evaluate(ValidEncounter<Encounter>(), "period.start.exists()"),
              EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionExistsNegation, {
  EXPECT_THAT(
      Evaluate(ValidEncounter<Encounter>(), "period.start.exists().not()"),
      EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionNotExists, {
  EXPECT_THAT(
      Evaluate(ValidEncounter<Encounter>(), "period.end.exists()"),
      EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionNotExistsNegation, {
  EXPECT_THAT(
      Evaluate(ValidEncounter<Encounter>(), "period.end.exists().not()"),
      EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionHasValue, {
  EXPECT_THAT(Evaluate(ValidEncounter<Encounter>(), "period.start.hasValue()"),
              EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestLogicalValueFieldExists, {
  // The logical .value field on primitives should return the primitive itself.
  Quantity quantity;
  quantity.mutable_value()->set_value("100");
  EXPECT_THAT(Evaluate(quantity, "value.value.exists()"), EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionHasValueNegation, {
  Encounter test_encounter = ValidEncounter<Encounter>();
  EXPECT_THAT(Evaluate(test_encounter, "period.start.hasValue().not()"),
              EvalsToFalse());

  test_encounter.mutable_period()->clear_start();
  EXPECT_THAT(Evaluate(test_encounter, "period.start.hasValue().not()"),
              EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionChildren, {
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
});

FHIR_VERSION_TEST(FhirPathTest, TestFunctionDescendants, {
  StructureDefinition structure_definition =
      ParseFromString<StructureDefinition>(R"proto(
        name {value: "foo"}
        context_invariant {value: "bar"}
        snapshot {element {label {value: "snapshot"}}}
        differential {element {label {value: "differential"}}}
      )proto");

  EXPECT_THAT(
      Evaluate(structure_definition, "descendants()")
          .ValueOrDie()
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
});

FHIR_VERSION_TEST(FhirPathTest, TestFunctionDescendantsOnEmptyCollection, {
  EXPECT_THAT(Evaluate("{}.descendants()"), EvalsToEmpty());
});

FHIR_VERSION_TEST(FhirPathTest, TestFunctionContains, {
  // Wrong number and/or types of arguments.
  EXPECT_THAT(Evaluate("'foo'.contains()"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(Evaluate("'foo'.contains(1)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(Evaluate("'foo'.contains('a', 'b')"),
              HasStatusCode(StatusCode::kInvalidArgument));

  EXPECT_THAT(Evaluate("'foo'.contains('')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo'.contains('o')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo'.contains('foo')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo'.contains('foob')"), EvalsToFalse());
  EXPECT_THAT(Evaluate("''.contains('')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("''.contains('foo')"), EvalsToFalse());

  EXPECT_THAT(Evaluate("{}.contains('foo')"), EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionEndsWith, {
  // Missing argument
  EXPECT_THAT(Evaluate("'foo'.endsWith()"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Empty colection argument
  EXPECT_THAT(Evaluate("'foo'.endsWith({})"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Too many arguments
  EXPECT_THAT(Evaluate("'foo'.endsWith('foo', 'foo')"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Wrong argument type
  EXPECT_THAT(Evaluate("'foo'.endsWith(1)"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Function does not exist for non-string type
  EXPECT_THAT(Evaluate("1.endsWith('1')"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Basic cases
  EXPECT_THAT(Evaluate("{}.endsWith('')"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("''.endsWith('')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo'.endsWith('')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo'.endsWith('o')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo'.endsWith('foo')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo'.endsWith('bfoo')"), EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionStartsWith, {
  // Missing argument
  EXPECT_THAT(Evaluate("'foo'.startsWith()"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Too many arguments
  EXPECT_THAT(Evaluate("'foo'.startsWith('foo', 'foo')"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Wrong argument type
  EXPECT_THAT(Evaluate("'foo'.startsWith(1)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(Evaluate("'foo'.startsWith(1.0)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(Evaluate("'foo'.startsWith(@2015-02-04T14:34:28Z)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(Evaluate("'foo'.startsWith(true)"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Function does not exist for non-string type
  EXPECT_THAT(Evaluate("1.startsWith(1)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(Evaluate("1.startsWith('1')"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Basic cases
  EXPECT_THAT(Evaluate("{}.startsWith('')"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("''.startsWith('')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo'.startsWith('')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo'.startsWith('f')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo'.startsWith('foo')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo'.startsWith('foob')"), EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionStartsWithSelfReference, {
  EXPECT_THAT(Evaluate(ValidObservation<Observation>(),
                       "code.coding.code.startsWith(code.coding.code)"),
              EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionStartsWithInvokedOnNonString, {
  EXPECT_THAT(
      Evaluate(ValidObservation<Observation>(), "code.startsWith('foo')"),
      HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionMatches, {
  EXPECT_THAT(Evaluate("{}.matches('')"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("''.matches('')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'a'.matches('a')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'abc'.matches('a')"), EvalsToFalse());
  EXPECT_THAT(Evaluate("'abc'.matches('...')"), EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionReplaceMatches, {
  EXPECT_THAT(Evaluate("{}.replaceMatches('', '')"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("'a'.replaceMatches('.', 'b')"),
              EvalsToStringThatMatches(StrEq("b")));
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionReplaceMatchesWrongArgCount, {
  StatusOr<EvaluationResult> result = Evaluate("''.replaceMatches()");
  EXPECT_THAT(result.status().code(), Eq(absl::StatusCode::kInvalidArgument))
      << result.status();
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionReplaceMatchesBadRegex, {
  StatusOr<EvaluationResult> result =
      Evaluate("''.replaceMatches('(', 'a')").status();
  EXPECT_THAT(result.status().code(), Eq(absl::StatusCode::kInvalidArgument))
      << result.status();
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionLength, {
  EXPECT_THAT(Evaluate("{}.length()"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("''.length() = 0"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'abc'.length() = 3"), EvalsToTrue());

  EXPECT_THAT(Evaluate("3.length()"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionToInteger, {
  EXPECT_EQ(Evaluate("1.toInteger()")
                .ValueOrDie()
                .GetInteger()
                .ValueOrDie(),
            1);
  EXPECT_EQ(Evaluate("'2'.toInteger()")
                .ValueOrDie()
                .GetInteger()
                .ValueOrDie(),
            2);

  EXPECT_THAT(Evaluate("(3.3).toInteger()"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("'a'.toInteger()"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("(1 | 2).toInteger()"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionToString, {
  EXPECT_THAT(Evaluate("1.toString()"), EvalsToStringThatMatches(StrEq("1")));
  EXPECT_THAT(Evaluate("1.1.toString()"),
              EvalsToStringThatMatches(StrEq("1.1")));
  EXPECT_THAT(Evaluate("'foo'.toString()"),
              EvalsToStringThatMatches(StrEq("foo")));
  EXPECT_THAT(Evaluate("true.toString()"),
              EvalsToStringThatMatches(StrEq("true")));
  EXPECT_THAT(Evaluate("{}.toString()"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("toString()"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("(1 | 2).toString()"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionTrace, {
  EXPECT_THAT(Evaluate("true.trace('debug')"), EvalsToTrue());
  EXPECT_THAT(Evaluate("{}.trace('debug')"), EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionHasValueComplex, {
  // hasValue should return false for non-primitive types.
  EXPECT_THAT(Evaluate(ValidEncounter<Encounter>(), "period.hasValue()"),
              EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionEmpty, {
  EXPECT_THAT(Evaluate("{}.empty()"), EvalsToTrue());
  EXPECT_THAT(Evaluate("true.empty()"), EvalsToFalse());
  EXPECT_THAT(Evaluate("(false | true).empty()"), EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionCount, {
  EXPECT_EQ(Evaluate("{}.count()").ValueOrDie()
                .GetInteger().ValueOrDie(),
            0);
  EXPECT_EQ(Evaluate("'a'.count()").ValueOrDie()
                .GetInteger().ValueOrDie(),
            1);
  EXPECT_EQ(Evaluate("('a' | 1).count()").ValueOrDie()
                .GetInteger().ValueOrDie(),
            2);
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionFirst, {
  EXPECT_THAT(Evaluate("{}.first()"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("true.first()"), EvalsToTrue());
  EXPECT_TRUE(Evaluate("(false | true).first()").ok());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionLast, {
  EXPECT_THAT(Evaluate("{}.last()"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("true.last()"), EvalsToTrue());
  EXPECT_THAT(Evaluate("true.combine(true).last()"), EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionSingle, {
  EXPECT_THAT(Evaluate("{}.single()"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("true.single()"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(false | true).single()"),
              HasStatusCode(StatusCode::kFailedPrecondition));
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionTail, {
  EXPECT_THAT(Evaluate("{}.tail()"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("true.tail()"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("true.combine(true).tail()"), EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionOfTypePrimitives, {
  EXPECT_THAT(Evaluate("{}.ofType(Boolean)"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("(true | 1 | 2.0 | 'foo').ofType(Boolean)"),
              EvalsToTrue());
  EXPECT_THAT(Evaluate("(true | 1 | 2.0 | 'foo').ofType(String)"),
              EvalsToStringThatMatches(StrEq("foo")));

  EXPECT_THAT(
      Evaluate("(true | 1 | 2.0 | 'foo').ofType(Integer)")
          .ValueOrDie()
          .GetMessages(),
      ElementsAreArray({EqualsProto(ParseFromString<Integer>("value: 1"))}));
  EXPECT_THAT(Evaluate("(true | 1 | 2.0 | 'foo').ofType(Decimal)")
                  .ValueOrDie()
                  .GetMessages(),
              ElementsAreArray(
                  {EqualsProto(ParseFromString<Decimal>("value: '2.0'"))}));
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionOfTypeResources, {
  Observation observation = ParseFromString<Observation>(R"proto()proto");

  EXPECT_THAT(Evaluate(observation, "$this.ofType(Boolean)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate(observation, "$this.ofType(CodeableConcept)"),
              EvalsToEmpty());

  EvaluationResult as_observation_evaluation_result =
      Evaluate(observation, "$this.ofType(Observation)").ValueOrDie();
  EXPECT_THAT(as_observation_evaluation_result.GetMessages(),
              ElementsAreArray({EqualsProto(observation)}));
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionAsPrimitives, {
  EXPECT_THAT(Evaluate("{}.as(Boolean)"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("true.as(Boolean)"), EvalsToTrue());
  EXPECT_THAT(Evaluate("true.as(Decimal)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("true.as(Integer)"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("1.as(Integer)"), EvalsToInteger(1));
  EXPECT_THAT(Evaluate("1.as(Decimal)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("1.as(Boolean)"), EvalsToEmpty());

  EXPECT_EQ(Evaluate("1.1.as(Decimal)").ValueOrDie().GetDecimal().ValueOrDie(),
            "1.1");
  EXPECT_THAT(Evaluate("1.1.as(Integer)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("1.1.as(Boolean)"), EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionAsResources, {
  Observation observation = ParseFromString<Observation>(R"proto()proto");

  EXPECT_THAT(Evaluate(observation, "$this.as(Boolean)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate(observation, "$this.as(CodeableConcept)"),
              EvalsToEmpty());

  EvaluationResult as_observation_evaluation_result =
      Evaluate(observation, "$this.as(Observation)").ValueOrDie();
  EXPECT_THAT(as_observation_evaluation_result.GetMessages(),
              ElementsAreArray({EqualsProto(observation)}));;
})

FHIR_VERSION_TEST(FhirPathTest, TestOperatorAsPrimitives, {
  EXPECT_THAT(Evaluate("{} as Boolean"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("true as Boolean"), EvalsToTrue());
  EXPECT_THAT(Evaluate("true as Decimal"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("true as Integer"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("1 as Integer"), EvalsToInteger(1));
  EXPECT_THAT(Evaluate("1 as Decimal"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("1 as Boolean"), EvalsToEmpty());

  EXPECT_EQ(Evaluate("1.1 as Decimal").ValueOrDie().GetDecimal().ValueOrDie(),
            "1.1");
  EXPECT_THAT(Evaluate("1.1 as Integer"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("1.1 as Boolean"), EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestOperatorAsResources, {
  Observation observation = ParseFromString<Observation>(R"proto()proto");

  EXPECT_THAT(Evaluate(observation, "$this as Boolean"), EvalsToEmpty());
  EXPECT_THAT(Evaluate(observation, "$this as CodeableConcept"),
              EvalsToEmpty());

  EvaluationResult as_observation_evaluation_result =
      Evaluate(observation, "$this as Observation").ValueOrDie();
  EXPECT_THAT(as_observation_evaluation_result.GetMessages(),
              ElementsAreArray({EqualsProto(observation)}));
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionIsPrimitives, {
  EXPECT_THAT(Evaluate("{}.is(Boolean)"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("true.is(Boolean)"), EvalsToTrue());
  EXPECT_THAT(Evaluate("true.is(Decimal)"), EvalsToFalse());
  EXPECT_THAT(Evaluate("true.is(Integer)"), EvalsToFalse());

  EXPECT_THAT(Evaluate("1.is(Integer)"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1.is(Decimal)"), EvalsToFalse());
  EXPECT_THAT(Evaluate("1.is(Boolean)"), EvalsToFalse());

  EXPECT_THAT(Evaluate("1.1.is(Decimal)"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1.1.is(Integer)"), EvalsToFalse());
  EXPECT_THAT(Evaluate("1.1.is(Boolean)"), EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionIsResources, {
  Observation observation = ParseFromString<Observation>(R"proto()proto");

  EXPECT_THAT(Evaluate(observation, "$this.is(Boolean)"), EvalsToFalse());
  EXPECT_THAT(Evaluate(observation, "$this.is(CodeableConcept)"),
              EvalsToFalse());
  EXPECT_THAT(Evaluate(observation, "$this.is(Observation)"), EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestOperatorIsPrimitives, {
  EXPECT_THAT(Evaluate("{} is Boolean"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("true is Boolean"), EvalsToTrue());
  EXPECT_THAT(Evaluate("true is Decimal"), EvalsToFalse());
  EXPECT_THAT(Evaluate("true is Integer"), EvalsToFalse());

  EXPECT_THAT(Evaluate("1 is Integer"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1 is Decimal"), EvalsToFalse());
  EXPECT_THAT(Evaluate("1 is Boolean"), EvalsToFalse());

  EXPECT_THAT(Evaluate("1.1 is Decimal"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1.1 is Integer"), EvalsToFalse());
  EXPECT_THAT(Evaluate("1.1 is Boolean"), EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TestOperatorIsResources, {
  Observation observation = ParseFromString<Observation>(R"proto()proto");

  EXPECT_THAT(Evaluate(observation, "$this is Boolean"), EvalsToFalse());
  EXPECT_THAT(Evaluate(observation, "$this is CodeableConcept"),
              EvalsToFalse());
  EXPECT_THAT(Evaluate(observation, "$this is Observation"), EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestFunctionTailMaintainsOrder, {
  CodeableConcept codeable_concept = ParseFromString<CodeableConcept>(R"proto(
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
      Evaluate(codeable_concept, "coding.tail().code").ValueOrDie();
  EXPECT_THAT(evaluation_result.GetMessages(),
              ElementsAreArray({EqualsProto(code_def), EqualsProto(code_ghi)}));
})

FHIR_VERSION_TEST(FhirPathTest, TestUnion, {
  EXPECT_THAT(Evaluate("({} | {})"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("(true | {})"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(true | true)"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(false | {})"), EvalsToFalse());
  EXPECT_THAT(Evaluate("(false | false)"), EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TestUnionDeduplicationPrimitives, {
  EvaluationResult evaluation_result =
      Evaluate("true | false | 1 | 'foo' | 2 | 1 | 'foo'")
          .ValueOrDie();
  std::vector<const Message*> result = evaluation_result.GetMessages();

  Boolean true_proto = ParseFromString<Boolean>("value: true");
  Boolean false_proto = ParseFromString<Boolean>("value: false");
  Integer integer_1_proto = ParseFromString<Integer>("value: 1");
  Integer integer_2_proto = ParseFromString<Integer>("value: 2");
  String string_foo_proto = ParseFromString<String>("value: 'foo'");

  ASSERT_THAT(result,
              UnorderedElementsAreArray(
                  {EqualsProto(true_proto),
                   EqualsProto(false_proto),
                   EqualsProto(integer_1_proto),
                   EqualsProto(integer_2_proto),
                   EqualsProto(string_foo_proto)}));
})

FHIR_VERSION_TEST(FhirPathTest, TestUnionDeduplicationObjects, {
  Encounter test_encounter = ValidEncounter<Encounter>();

  EvaluationResult evaluation_result =
      Evaluate(test_encounter, ("period | status | status | period"))
          .ValueOrDie();
  std::vector<const Message*> result = evaluation_result
          .GetMessages();

  ASSERT_THAT(result,
              UnorderedElementsAreArray(
                  {EqualsProto(test_encounter.status()),
                   EqualsProto(test_encounter.period())}));
})

FHIR_VERSION_TEST(FhirPathTest, TestCombine, {
  EXPECT_THAT(Evaluate("{}.combine({})"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("true.combine({})"), EvalsToTrue());
  EXPECT_THAT(Evaluate("{}.combine(true)"), EvalsToTrue());

  Boolean true_proto = ParseFromString<Boolean>("value: true");
  Boolean false_proto = ParseFromString<Boolean>("value: false");
  EvaluationResult evaluation_result =
      Evaluate("true.combine(true).combine(false)")
          .ValueOrDie();
  EXPECT_THAT(evaluation_result.GetMessages(),
              UnorderedElementsAreArray({EqualsProto(true_proto),
                                         EqualsProto(true_proto),
                                         EqualsProto(false_proto)}));
})

FHIR_VERSION_TEST(FhirPathTest, TestIntersect, {
  EXPECT_THAT(Evaluate("{}.intersect({})"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("true.intersect({})"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("true.intersect(false)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("{}.intersect(true)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("true.intersect(true)"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(true | false).intersect(true)"), EvalsToTrue());

  EXPECT_THAT(Evaluate("(true.combine(true)).intersect(true))"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(true).intersect(true.combine(true))"), EvalsToTrue());

  Boolean true_proto = ParseFromString<Boolean>("value: true");
  Boolean false_proto = ParseFromString<Boolean>("value: false");
  EvaluationResult evaluation_result =
      Evaluate("(true | false).intersect(true | false)")
          .ValueOrDie();
  EXPECT_THAT(evaluation_result.GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(true_proto), EqualsProto(false_proto)}));
})

FHIR_VERSION_TEST(FhirPathTest, TestDistinct, {
  EXPECT_THAT(Evaluate("{}.distinct()"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("true.distinct()"), EvalsToTrue());
  EXPECT_THAT(Evaluate("true.combine(true).distinct()"), EvalsToTrue());

  Boolean true_proto = ParseFromString<Boolean>("value: true");
  Boolean false_proto = ParseFromString<Boolean>("value: false");
  EvaluationResult evaluation_result =
      Evaluate("(true | false).distinct()").ValueOrDie();
  EXPECT_THAT(evaluation_result.GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(true_proto), EqualsProto(false_proto)}));
})

FHIR_VERSION_TEST(FhirPathTest, TestIsDistinct, {
  EXPECT_THAT(Evaluate("{}.isDistinct()"), EvalsToTrue());
  EXPECT_THAT(Evaluate("true.isDistinct()"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(true | false).isDistinct()"), EvalsToTrue());

  EXPECT_THAT(Evaluate("true.combine(true).isDistinct()"), EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TestIndexer, {
  EXPECT_THAT(Evaluate("true[0]"), EvalsToTrue());
  EXPECT_THAT(Evaluate("true[1]"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("false[0]"), EvalsToFalse());
  EXPECT_THAT(Evaluate("false[1]"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("true['foo']"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(Evaluate("true[(1 | 2)]"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestContains, {
  EXPECT_THAT(Evaluate("true contains true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(false | true) contains true"), EvalsToTrue());

  EXPECT_THAT(Evaluate("true contains false"), EvalsToFalse());
  EXPECT_THAT(Evaluate("(false | true) contains 1"), EvalsToFalse());
  EXPECT_THAT(Evaluate("{} contains true"), EvalsToFalse());

  EXPECT_THAT(Evaluate("({} contains {})"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("(true contains {})"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("{} contains (true | false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(Evaluate("true contains (true | false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestIn, {
  EXPECT_THAT(Evaluate("true in true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("true in (false | true)"), EvalsToTrue());

  EXPECT_THAT(Evaluate("false in true"), EvalsToFalse());
  EXPECT_THAT(Evaluate("1 in (false | true)"), EvalsToFalse());
  EXPECT_THAT(Evaluate("true in {}"), EvalsToFalse());

  EXPECT_THAT(Evaluate("({} in {})"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("({} in true)"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("(true | false) in {}"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(Evaluate("(true | false) in {}"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestImplies, {
  EXPECT_THAT(Evaluate("(true implies true) = true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(true implies false) = false"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(true implies {})"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("(false implies true) = true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(false implies false) = true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(false implies {}) = true"), EvalsToTrue());

  EXPECT_THAT(Evaluate("({} implies true) = true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("({} implies false)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("({} implies {})"), EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestWhere, {
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
      Evaluate(observation, "coding.where(system = 'foo').code").ValueOrDie();
  EXPECT_THAT(evaluation_result.GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(code_abc), EqualsProto(code_ghi)}));
})

FHIR_VERSION_TEST(FhirPathTest, TestWhereNoMatches, {
  EXPECT_THAT(Evaluate("('a' | 'b' | 'c').where(false)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("{}.where(true)"), EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestWhereValidatesArguments, {
  EXPECT_THAT(Evaluate("{}.where()"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(Evaluate("{}.where(true)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("{}.where(true, false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestAllTrue, {
  EXPECT_THAT(Evaluate("{}.allTrue()"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(false).allTrue()"), EvalsToFalse());
  EXPECT_THAT(Evaluate("(true).allTrue()"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(false | true).allTrue()"), EvalsToFalse());

  // Verify that allTrue() fails when called with the wrong number of arguments.
  EXPECT_THAT(Evaluate("{}.allTrue(true)"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestAllFalse, {
  EXPECT_THAT(Evaluate("{}.allFalse()"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(false).allFalse()"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(true).allFalse()"), EvalsToFalse());
  EXPECT_THAT(Evaluate("(false | true).allFalse()"), EvalsToFalse());

  // Verify that alFalse() fails when called with the wrong number of arguments.
  EXPECT_THAT(Evaluate("{}.allFalse(true)"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestAll, {
  EXPECT_THAT(Evaluate("{}.all(false)"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(false).all(true)"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(1 | 2 | 3).all($this < 4)"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(1 | 2 | 3).all($this > 4)"), EvalsToFalse());

  // Verify that all() fails when called with the wrong number of arguments.
  EXPECT_THAT(Evaluate("{}.all()"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(Evaluate("{}.all(true, false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestAllReadsFieldFromDifferingTypes, {
  StructureDefinition structure_definition =
      ParseFromString<StructureDefinition>(R"proto(
        snapshot {
          element {}
        }
        differential {
          element {}
        }
      )proto");

  EXPECT_THAT(Evaluate(structure_definition,
                       "(snapshot | differential).all(element.exists())"),
              EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestSelect, {
  EvaluationResult evaluation_result =
      Evaluate("(1 | 2 | 3).select(($this > 2) | $this)")
          .ValueOrDie();
  std::vector<const Message*> result = evaluation_result.GetMessages();

  Boolean true_proto = ParseFromString<Boolean>("value: true");
  Boolean false_proto = ParseFromString<Boolean>("value: false");
  Integer integer_1_proto = ParseFromString<Integer>("value: 1");
  Integer integer_2_proto = ParseFromString<Integer>("value: 2");
  Integer integer_3_proto = ParseFromString<Integer>("value: 3");

  ASSERT_THAT(
      result,
      UnorderedElementsAreArray({
        EqualsProto(true_proto),
        EqualsProto(false_proto),
        EqualsProto(false_proto),
        EqualsProto(integer_1_proto),
        EqualsProto(integer_2_proto),
        EqualsProto(integer_3_proto)
      }));
})

FHIR_VERSION_TEST(FhirPathTest, TestSelectEmptyResult, {
  EXPECT_THAT(Evaluate("{}.where(true)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("(1 | 2 | 3).where(false)"), EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestSelectValidatesArguments, {
  EXPECT_THAT(Evaluate("{}.select()"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(Evaluate("{}.select(true)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("{}.select(true, false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestIif, {
  // 2 parameter invocations
  EXPECT_THAT(Evaluate("iif(true, 1)"), EvalsToInteger(1));
  EXPECT_THAT(Evaluate("iif(false, 1)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("iif({}, 1)"), EvalsToEmpty());

  // 3 parameters invocations
  EXPECT_THAT(Evaluate("iif(true, 1, 2)"), EvalsToInteger(1));
  EXPECT_THAT(Evaluate("iif(false, 1, 2)"), EvalsToInteger(2));
  EXPECT_THAT(Evaluate("iif({}, 1, 2)"), EvalsToInteger(2));

  EXPECT_THAT(Evaluate("{}.iif(true, false)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("(1 | 2).iif(true, false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestIifValidatesArguments, {
  EXPECT_THAT(Evaluate("{}.iif()"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(Evaluate("{}.iif(true)"),
              HasStatusCode(StatusCode::kInvalidArgument));
  EXPECT_THAT(Evaluate("{}.iif(true, false, true, false)"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestXor, {
  EXPECT_THAT(Evaluate("(true xor true) = false"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(true xor false) = true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(true xor {})"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("(false xor true) = true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(false xor false) = false"), EvalsToTrue());
  EXPECT_THAT(Evaluate("(false xor {})"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("({} xor true)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("({} xor false)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("({} xor {})"), EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestMultiOrShortCircuit, {
  Period no_end_period = ParseFromString<Period>(R"proto(
    start: { value_us: 1556750000000 timezone: "America/Los_Angeles" }
  )proto");

  EXPECT_THAT(
      Evaluate(
          no_end_period,
          "start.hasValue().not() or end.hasValue().not() or start <= end"),
      EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestOr, {
  EXPECT_THAT(Evaluate("true or true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("true or false"), EvalsToTrue());
  EXPECT_THAT(Evaluate("true or {}"), EvalsToTrue());

  EXPECT_THAT(Evaluate("false or true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("false or false"), EvalsToFalse());
  EXPECT_THAT(Evaluate("false or {}"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("{} or true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("{} or false"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("{} or {}"), EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestAnd, {
  EXPECT_THAT(Evaluate("true and true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("true and false"), EvalsToFalse());
  EXPECT_THAT(Evaluate("true and {}"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("false and true"), EvalsToFalse());
  EXPECT_THAT(Evaluate("false and false"), EvalsToFalse());
  EXPECT_THAT(Evaluate("false and {}"), EvalsToFalse());

  EXPECT_THAT(Evaluate("{} and true"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("{} and false"), EvalsToFalse());
  EXPECT_THAT(Evaluate("{} and {}"), EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestSingletonEvaluationOfCollections, {
  EXPECT_THAT(Evaluate("'string' and true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'string' and false"), EvalsToFalse());
  EXPECT_THAT(Evaluate("'string' or false"), EvalsToTrue());

  EXPECT_THAT(Evaluate("1 and true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1 and false"), EvalsToFalse());
  EXPECT_THAT(Evaluate("1 or false"), EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestEmptyLiteral, {
  EXPECT_THAT(Evaluate("{}"), EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestBooleanLiteral, {
  EXPECT_THAT(Evaluate("true"), EvalsToTrue());
  EXPECT_THAT(Evaluate("false"), EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TestIntegerLiteral, {
  EvaluationResult result = Evaluate("42").ValueOrDie();
  EXPECT_EQ(42, result.GetInteger().ValueOrDie());

  // Ensure evaluation of an out-of-range literal fails.
  const char* overflow_value = "10000000000";
  Status bad_int_status =
      Compile(Encounter::descriptor(), overflow_value).status();

  EXPECT_FALSE(bad_int_status.ok());
  // Failure message should contain the bad string.
  EXPECT_TRUE(bad_int_status.message().find(overflow_value) !=
              std::string::npos);
})

FHIR_VERSION_TEST(FhirPathTest, TestPolarityOperator, {
  EXPECT_THAT(Evaluate("+1 = 1"), EvalsToTrue());
  EXPECT_THAT(Evaluate("-(+1) = -1"), EvalsToTrue());
  EXPECT_THAT(Evaluate("+(-1) = -1"), EvalsToTrue());
  EXPECT_THAT(Evaluate("-(-1) = 1"), EvalsToTrue());

  EXPECT_THAT(Evaluate("+1.2 = 1.2"), EvalsToTrue());
  EXPECT_THAT(Evaluate("-(+1.2) = -1.2"), EvalsToTrue());
  EXPECT_THAT(Evaluate("+(-1.2) = -1.2"), EvalsToTrue());
  EXPECT_THAT(Evaluate("-(-1.2) = 1.2"), EvalsToTrue());

  EXPECT_THAT(Evaluate("+{}"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("-{}"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("+(1 | 2)"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestIntegerAddition, {
  EXPECT_THAT(Evaluate("(2 + 3) = 5"), EvalsToTrue());
  EXPECT_THAT(Evaluate("({} + 3)"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("(2 + {})"), EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestStringAddition, {
  EXPECT_THAT(Evaluate("('foo' + 'bar') = 'foobar'"), EvalsToTrue());
  EXPECT_THAT(Evaluate("({} + 'bar')"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("('foo' + {})"), EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestStringConcatenation, {
  EXPECT_THAT(Evaluate("('foo' & 'bar')"),
              EvalsToStringThatMatches(StrEq("foobar")));
  EXPECT_THAT(Evaluate("{} & 'bar'"), EvalsToStringThatMatches(StrEq("bar")));
  EXPECT_THAT(Evaluate("'foo' & {}"), EvalsToStringThatMatches(StrEq("foo")));
  EXPECT_THAT(Evaluate("{} & {}"), EvalsToStringThatMatches(StrEq("")));
})

FHIR_VERSION_TEST(FhirPathTest, TestEmptyComparisons, {
  EXPECT_THAT(Evaluate("{} = 42"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("42 = {}"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("{} = {}"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("{} != 42"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("42 != {}"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("{} != {}"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("{} < 42"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("42 < {}"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("{} < {}"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("{} > 42"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("42 > {}"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("{} > {}"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("{} >= 42"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("42 >= {}"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("{} >= {}"), EvalsToEmpty());

  EXPECT_THAT(Evaluate("{} <= 42"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("42 <= {}"), EvalsToEmpty());
  EXPECT_THAT(Evaluate("{} <= {}"), EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TestIntegerComparisons, {
  EXPECT_THAT(Evaluate("42 = 42"), EvalsToTrue());
  EXPECT_THAT(Evaluate("42 = 43"), EvalsToFalse());

  EXPECT_THAT(Evaluate("42 != 43"), EvalsToTrue());
  EXPECT_THAT(Evaluate("42 != 42"), EvalsToFalse());

  EXPECT_THAT(Evaluate("42 < 43"), EvalsToTrue());
  EXPECT_THAT(Evaluate("42 < 42"), EvalsToFalse());

  EXPECT_THAT(Evaluate("43 > 42"), EvalsToTrue());
  EXPECT_THAT(Evaluate("42 > 42"), EvalsToFalse());

  EXPECT_THAT(Evaluate("42 >= 42"), EvalsToTrue());
  EXPECT_THAT(Evaluate("43 >= 42"), EvalsToTrue());
  EXPECT_THAT(Evaluate("42 >= 43"), EvalsToFalse());

  EXPECT_THAT(Evaluate("42 <= 42"), EvalsToTrue());
  EXPECT_THAT(Evaluate("42 <= 43"), EvalsToTrue());
  EXPECT_THAT(Evaluate("43 <= 42"), EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TestIntegerLikeComparison, {
  Parameters parameters =
      ParseFromString<Parameters>(R"proto(
        parameter {value {integer {value: -1}}}
        parameter {value {integer {value: 0}}}
        parameter {value {integer {value: 1}}}
        parameter {value {unsigned_int {value: 0}}}
      )proto");

  // lhs = -1 (signed), rhs = 0 (unsigned)
  EXPECT_THAT(Evaluate(parameters, "parameter[0].value < parameter[3].value"),
              EvalsToTrue());
  EXPECT_THAT(Evaluate(parameters, "parameter[0].value <= parameter[3].value"),
              EvalsToTrue());
  EXPECT_THAT(Evaluate(parameters, "parameter[0].value >= parameter[3].value"),
              EvalsToFalse());
  EXPECT_THAT(Evaluate(parameters, "parameter[0].value > parameter[3].value"),
              EvalsToFalse());

  // lhs = 0 (signed), rhs = 0 (unsigned)
  EXPECT_THAT(Evaluate(parameters, "parameter[1].value < parameter[3].value"),
              EvalsToFalse());
  EXPECT_THAT(Evaluate(parameters, "parameter[1].value <= parameter[3].value"),
              EvalsToTrue());
  EXPECT_THAT(Evaluate(parameters, "parameter[1].value >= parameter[3].value"),
              EvalsToTrue());
  EXPECT_THAT(Evaluate(parameters, "parameter[1].value > parameter[3].value"),
              EvalsToFalse());

  // lhs = 1 (signed), rhs = 0 (unsigned)
  EXPECT_THAT(Evaluate(parameters, "parameter[2].value < parameter[3].value"),
              EvalsToFalse());
  EXPECT_THAT(Evaluate(parameters, "parameter[2].value <= parameter[3].value"),
              EvalsToFalse());
  EXPECT_THAT(Evaluate(parameters, "parameter[2].value >= parameter[3].value"),
              EvalsToTrue());
  EXPECT_THAT(Evaluate(parameters, "parameter[2].value > parameter[3].value"),
              EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TestDecimalLiteral, {
  EvaluationResult result = Evaluate("1.25").ValueOrDie();
  EXPECT_EQ("1.25", result.GetDecimal().ValueOrDie());
})

FHIR_VERSION_TEST(FhirPathTest, TestDecimalComparisons, {
  EXPECT_THAT(Evaluate("1.25 = 1.25"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1.25 = 1.3"), EvalsToFalse());

  EXPECT_THAT(Evaluate("1.25 != 1.26"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1.25 != 1.25"), EvalsToFalse());

  EXPECT_THAT(Evaluate("1.25 < 1.26"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1 < 1.26"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1.25 < 1.25"), EvalsToFalse());

  EXPECT_THAT(Evaluate("1.26 > 1.25"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1.26 > 1"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1.25 > 1.25"), EvalsToFalse());

  EXPECT_THAT(Evaluate("1.25 >= 1.25"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1.25 >= 1"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1.26 >= 1.25"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1.25 >= 1.26"), EvalsToFalse());

  EXPECT_THAT(Evaluate("1.25 <= 1.25"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1.25 <= 1.26"), EvalsToTrue());
  EXPECT_THAT(Evaluate("1.26 <= 1.25"), EvalsToFalse());
  EXPECT_THAT(Evaluate("1.26 <= 1"), EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TestStringLiteral, {
  EXPECT_THAT(Evaluate("'foo'"), EvalsToStringThatMatches(StrEq("foo")));
})

FHIR_VERSION_TEST(FhirPathTest, TestStringLiteralEscaping, {
  EXPECT_THAT(Evaluate("'\\\\'"), EvalsToStringThatMatches(StrEq("\\")));
  EXPECT_THAT(Evaluate("'\\f'"), EvalsToStringThatMatches(StrEq("\f")));
  EXPECT_THAT(Evaluate("'\\n'"), EvalsToStringThatMatches(StrEq("\n")));
  EXPECT_THAT(Evaluate("'\\r'"), EvalsToStringThatMatches(StrEq("\r")));
  EXPECT_THAT(Evaluate("'\\t'"), EvalsToStringThatMatches(StrEq("\t")));
  EXPECT_THAT(Evaluate("'\\\"'"), EvalsToStringThatMatches(StrEq("\"")));
  EXPECT_THAT(Evaluate("'\\\''"), EvalsToStringThatMatches(StrEq("'")));
  EXPECT_THAT(Evaluate("'\\t'"), EvalsToStringThatMatches(StrEq("\t")));
  EXPECT_THAT(Evaluate("'\\u0020'"), EvalsToStringThatMatches(StrEq(" ")));

  // Escape sequences that should be ignored (but are not currently.)
  // TODO: These sequences should not be unescaped.
  EXPECT_THAT(Evaluate("'\\x20'"), EvalsToStringThatMatches(StrEq(" ")));
  EXPECT_THAT(Evaluate("'\\123'"), EvalsToStringThatMatches(StrEq("S")));
  EXPECT_THAT(Evaluate("'\\x00000020'"), EvalsToStringThatMatches(StrEq(" ")));
})

FHIR_VERSION_TEST(FhirPathTest, TestStringComparisons, {
  EXPECT_THAT(Evaluate("'foo' = 'foo'"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo' = 'bar'"), EvalsToFalse());

  EXPECT_THAT(Evaluate("'foo' != 'bar'"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo' != 'foo'"), EvalsToFalse());

  EXPECT_THAT(Evaluate("'bar' < 'foo'"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo' < 'foo'"), EvalsToFalse());

  EXPECT_THAT(Evaluate("'foo' > 'bar'"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo' > 'foo'"), EvalsToFalse());

  EXPECT_THAT(Evaluate("'foo' >= 'foo'"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo' >= 'bar'"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'bar' >= 'foo'"), EvalsToFalse());

  EXPECT_THAT(Evaluate("'foo' <= 'foo'"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'bar' <= 'foo'"), EvalsToTrue());
  EXPECT_THAT(Evaluate("'foo' <= 'bar'"), EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TestDateTimeLiteral, {
  DateTime millisecond_precision;
  millisecond_precision.set_value_us(1390660214559000);
  millisecond_precision.set_timezone("Z");
  millisecond_precision.set_precision(DateTime::MILLISECOND);
  EXPECT_THAT(Evaluate("@2014-01-25T14:30:14.559").ValueOrDie().GetMessages(),
              ElementsAreArray({EqualsProto(millisecond_precision)}));

  DateTime second_precision;
  second_precision.set_value_us(1390660214000000);
  second_precision.set_timezone("Z");
  second_precision.set_precision(DateTime::SECOND);
  EXPECT_THAT(Evaluate("@2014-01-25T14:30:14").ValueOrDie().GetMessages(),
              ElementsAreArray({EqualsProto(second_precision)}));

  // TODO: MINUTE precision should be supported.
  EXPECT_THAT(Evaluate("@2014-01-25T14:30"),
              HasStatusCode(StatusCode::kUnimplemented));

  // TODO: HOUR precision should be supported.
  EXPECT_THAT(Evaluate("@2014-01-25T14"),
              HasStatusCode(StatusCode::kUnimplemented));

  EXPECT_THAT(
      Evaluate("@2014-01-25T").ValueOrDie().GetMessages(),
      ElementsAreArray({EqualsProto(ToDateTime<DateTime, DateTime::Precision>(
          absl::CivilSecond(2014, 1, 25, 0, 0, 0), absl::UTCTimeZone(),
          DateTime::DAY))}));

  EXPECT_THAT(
      Evaluate("@2014-01T").ValueOrDie().GetMessages(),
      ElementsAreArray({EqualsProto(ToDateTime<DateTime, DateTime::Precision>(
          absl::CivilSecond(2014, 1, 1, 0, 0, 0), absl::UTCTimeZone(),
          DateTime::MONTH))}));

  EXPECT_THAT(
      Evaluate("@2014T").ValueOrDie().GetMessages(),
      ElementsAreArray({EqualsProto(ToDateTime<DateTime, DateTime::Precision>(
          absl::CivilSecond(2014, 1, 1, 0, 0, 0), absl::UTCTimeZone(),
          DateTime::YEAR))}));
})

FHIR_VERSION_TEST(FhirPathTest, TestTimeComparisonsWithLiterals, {
  // Test cases form http://hl7.org/fhirpath/#datetime-equality
  // TODO: This should evaluate to true.
  EXPECT_THAT(Evaluate("@2012-01-01T10:30 = @2012-01-01T10:30"),
              HasStatusCode(StatusCode::kUnimplemented));
  // TODO: This should evaluate to false.
  EXPECT_THAT(Evaluate("@2012-01-01T10:30 = @2012-01-01T10:31"),
              HasStatusCode(StatusCode::kUnimplemented));
  // TODO: This should evaluate to empty.
  EXPECT_THAT(Evaluate("@2012-01-01T10:30:31 = @2012-01-01T10:30"),
              HasStatusCode(StatusCode::kUnimplemented));
  // TODO: This should evaluate to true.
  EXPECT_THAT(Evaluate("@2012-01-01T10:30:31.0 = @2012-01-01T10:30:31"),
              EvalsToFalse());
  EXPECT_THAT(Evaluate("@2012-01-01T10:30:31.1 = @2012-01-01T10:30:31"),
              EvalsToFalse());
  // Additional test case to cover unimplemented example above.
  // TODO: This should evaluate to empty.
  EXPECT_THAT(Evaluate("@2018-03-01T = @2018-03-01T10:30:00"), EvalsToFalse());
  EXPECT_THAT(Evaluate("@2018-03-01T10:30:00 = @2018-03-01T10:30:00"),
              EvalsToTrue());

  EXPECT_THAT(
      Evaluate("@2017-11-05T01:30:00.0-04:00 < @2017-11-05T01:15:00.0-05:00"),
      EvalsToTrue());
  EXPECT_THAT(
      Evaluate("@2017-11-05T01:30:00.0-04:00 > @2017-11-05T01:15:00.0-05:00"),
      EvalsToFalse());
  // TODO: This should evaluate to true.
  EXPECT_THAT(
      Evaluate("@2017-11-05T01:30:00.0-04:00 = @2017-11-05T00:30:00.0-05:00"),
      EvalsToFalse());
  EXPECT_THAT(
      Evaluate("@2017-11-05T01:30:00.0-04:00 = @2017-11-05T01:15:00.0-05:00"),
      EvalsToFalse());

  // Test cases form http://hl7.org/fhirpath/#greater-than
  EXPECT_THAT(Evaluate("@2018-03-01T10:30:00 > @2018-03-01T10:00:00"),
              EvalsToTrue());
  // TODO: This should evaluate to empty.
  EXPECT_THAT(Evaluate("@2018-03-01T10 > @2018-03-01T10:30"),
              HasStatusCode(StatusCode::kUnimplemented));
  EXPECT_THAT(Evaluate("@2018-03-01T10:30:00 > @2018-03-01T10:30:00.0"),
              EvalsToFalse());
  // Additional test case to cover unimplemented example above.
  EXPECT_THAT(Evaluate("@2018-03-01T > @2018-03-01T10:30:00"), EvalsToEmpty());

  // Test cases form http://hl7.org/fhirpath/#less-than
  EXPECT_THAT(Evaluate("@2018-03-01T10:30:00 < @2018-03-01T10:00:00"),
              EvalsToFalse());
  // TODO: This should evaluate to empty.
  EXPECT_THAT(Evaluate("@2018-03-01T10 < @2018-03-01T10:30"),
              HasStatusCode(StatusCode::kUnimplemented));
  EXPECT_THAT(Evaluate("@2018-03-01T10:30:00 < @2018-03-01T10:30:00.0"),
              EvalsToFalse());
  // Additional test case to cover unimplemented example above.
  EXPECT_THAT(Evaluate("@2018-03-01T < @2018-03-01T10:30:00"), EvalsToEmpty());

  // Test cases from http://hl7.org/fhirpath/#less-or-equal
  EXPECT_THAT(Evaluate("@2018-03-01T10:30:00 <= @2018-03-01T10:00:00"),
              EvalsToFalse());
  // TODO: This should evaluate to empty.
  EXPECT_THAT(Evaluate("@2018-03-01T10 <= @2018-03-01T10:30"),
              HasStatusCode(StatusCode::kUnimplemented));
  EXPECT_THAT(Evaluate("@2018-03-01T10:30:00 <= @2018-03-01T10:30:00.0"),
              EvalsToTrue());
  // Additional test case to cover unimplemented example above.
  EXPECT_THAT(Evaluate("@2018-03-01T <= @2018-03-01T10:30:00"), EvalsToEmpty());

  // Test cases from http://hl7.org/fhirpath/#greater-or-equal
  EXPECT_THAT(Evaluate("@2018-03-01T10:30:00 >= @2018-03-01T10:00:00"),
              EvalsToTrue());
  // TODO: This should evaluate to empty.
  EXPECT_THAT(Evaluate("@2018-03-01T10 >= @2018-03-01T10:30"),
              HasStatusCode(StatusCode::kUnimplemented));
  EXPECT_THAT(Evaluate("@2018-03-01T10:30:00 >= @2018-03-01T10:30:00.0"),
              EvalsToTrue());
})

FHIR_VERSION_TEST(FhirPathTest, TimeComparison, {
  Period start_before_end_period = ParseFromString<Period>(R"proto(
    start: { value_us: 1556750000000000 timezone: "America/Los_Angeles" }
    end: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
  )proto");
  EXPECT_THAT(Evaluate(start_before_end_period, "start <= end"), EvalsToTrue());

  Period end_before_start_period = ParseFromString<Period>(R"proto(
    start: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
    end: { value_us: 1556750000000000 timezone: "America/Los_Angeles" }
  )proto");
  EXPECT_THAT(Evaluate(end_before_start_period, "start <= end"),
              EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, TimeCompareDifferentPrecision, {
  absl::TimeZone zone;
  absl::LoadTimeZone("America/Los_Angeles", &zone);

  // Ensure comparison returns false on fine-grained checks but true
  // on corresponding coarse-grained checks.
  EXPECT_THAT(
      Evaluate(CreatePeriod<Period, DateTime>(
                   ToDateTime<DateTime, DateTime::Precision>(
                       absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                       DateTime::SECOND),
                   ToDateTime<DateTime, DateTime::Precision>(
                       absl::CivilDay(2019, 5, 2), zone, DateTime::SECOND)),
               "start <= end"),
      EvalsToFalse());

  EXPECT_THAT(
      Evaluate(CreatePeriod<Period, DateTime>(
                   ToDateTime<DateTime, DateTime::Precision>(
                       absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                       DateTime::SECOND),
                   ToDateTime<DateTime, DateTime::Precision>(
                       absl::CivilDay(2019, 5, 2), zone, DateTime::DAY)),
               "start <= end"),
      EvalsToEmpty());

  EXPECT_THAT(
      Evaluate(CreatePeriod<Period, DateTime>(
                   ToDateTime<DateTime, DateTime::Precision>(
                       absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                       DateTime::SECOND),
                   ToDateTime<DateTime, DateTime::Precision>(
                       absl::CivilDay(2019, 5, 1), zone, DateTime::DAY)),
               "start <= end"),
      EvalsToEmpty());

  EXPECT_THAT(
      Evaluate(CreatePeriod<Period, DateTime>(
                   ToDateTime<DateTime, DateTime::Precision>(
                       absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                       DateTime::SECOND),
                   ToDateTime<DateTime, DateTime::Precision>(
                       absl::CivilDay(2019, 5, 1), zone, DateTime::MONTH)),
               "start <= end"),
      EvalsToEmpty());

  EXPECT_THAT(
      Evaluate(CreatePeriod<Period, DateTime>(
                   ToDateTime<DateTime, DateTime::Precision>(
                       absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                       DateTime::SECOND),
                   ToDateTime<DateTime, DateTime::Precision>(
                       absl::CivilDay(2019, 1, 1), zone, DateTime::MONTH)),
               "start <= end"),
      EvalsToEmpty());

  EXPECT_THAT(
      Evaluate(CreatePeriod<Period, DateTime>(
                   ToDateTime<DateTime, DateTime::Precision>(
                       absl::CivilSecond(2019, 5, 2, 22, 33, 53), zone,
                       DateTime::SECOND),
                   ToDateTime<DateTime, DateTime::Precision>(
                       absl::CivilDay(2019, 1, 1), zone, DateTime::YEAR)),
               "start <= end"),
      EvalsToEmpty());
})

FHIR_VERSION_TEST(FhirPathTest, TimeCompareMicroseconds, {
  // Test edge case for very high precision comparisons.
  DateTime start_micros;
  start_micros.set_value_us(1556750000000011);
  start_micros.set_timezone("America/Los_Angeles");
  start_micros.set_precision(DateTime::MICROSECOND);

  DateTime end_micros = start_micros;
  EXPECT_THAT(Evaluate(CreatePeriod<Period, DateTime>(start_micros, end_micros),
                       "start <= end"),
              EvalsToTrue());

  end_micros.set_value_us(end_micros.value_us() - 1);
  EXPECT_THAT(Evaluate(CreatePeriod<Period, DateTime>(start_micros, end_micros),
                       "start <= end"),
              EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, SimpleQuantityComparisons, {
  auto kinetics =
      ParseFromString<r4::core::MedicationKnowledge_Kinetics>(R"proto(
        area_under_curve {
          value { value: "1.1" }
          system { value: "http://valuesystem.example.org/foo" }
          code { value: "bar" }
        }
        area_under_curve {
          value { value: "1.2" }
          system { value: "http://valuesystem.example.org/foo" }
          code { value: "bar" }
        }
        area_under_curve {
          value { value: "1.1" }
          system { value: "http://valuesystem.example.org/foo" }
          code { value: "different" }
        }
        area_under_curve {
          value { value: "1.1" }
          system { value: "http://valuesystem.example.org/different" }
          code { value: "bar" }
        }
      )proto");

  EXPECT_THAT(Evaluate(kinetics, "areaUnderCurve[0] < areaUnderCurve[0]"),
              EvalsToFalse());
  EXPECT_THAT(Evaluate(kinetics, "areaUnderCurve[0] <= areaUnderCurve[0]"),
              EvalsToTrue());
  EXPECT_THAT(Evaluate(kinetics, "areaUnderCurve[0] >= areaUnderCurve[0]"),
              EvalsToTrue());
  EXPECT_THAT(Evaluate(kinetics, "areaUnderCurve[0] > areaUnderCurve[0]"),
              EvalsToFalse());

  EXPECT_THAT(Evaluate(kinetics, "areaUnderCurve[1] < areaUnderCurve[0]"),
              EvalsToFalse());
  EXPECT_THAT(Evaluate(kinetics, "areaUnderCurve[1] <= areaUnderCurve[0]"),
              EvalsToFalse());
  EXPECT_THAT(Evaluate(kinetics, "areaUnderCurve[1] >= areaUnderCurve[0]"),
              EvalsToTrue());
  EXPECT_THAT(Evaluate(kinetics, "areaUnderCurve[1] > areaUnderCurve[0]"),
              EvalsToTrue());

  EXPECT_THAT(Evaluate(kinetics, "areaUnderCurve[0] < areaUnderCurve[1]"),
              EvalsToTrue());
  EXPECT_THAT(Evaluate(kinetics, "areaUnderCurve[0] <= areaUnderCurve[1]"),
              EvalsToTrue());
  EXPECT_THAT(Evaluate(kinetics, "areaUnderCurve[0] >= areaUnderCurve[1]"),
              EvalsToFalse());
  EXPECT_THAT(Evaluate(kinetics, "areaUnderCurve[0] > areaUnderCurve[1]"),
              EvalsToFalse());

  // Different quantity codes
  EXPECT_THAT(Evaluate(kinetics, "areaUnderCurve[0] > areaUnderCurve[2]"),
              HasStatusCode(StatusCode::kInvalidArgument));

  // Different quantity systems
  EXPECT_THAT(Evaluate(kinetics, "areaUnderCurve[0] > areaUnderCurve[3]"),
              HasStatusCode(StatusCode::kInvalidArgument));
})

FHIR_VERSION_TEST(FhirPathTest, TestCompareEnumToString, {
  auto encounter = ValidEncounter<Encounter>();

  EXPECT_THAT(Evaluate(encounter, "status = 'triaged'"), EvalsToTrue());

  encounter.mutable_status()->set_value(
      EncounterStatusCode::FINISHED);
  EXPECT_THAT(Evaluate(encounter, "status = 'triaged'"), EvalsToFalse());
})

FHIR_VERSION_TEST(FhirPathTest, PathNavigationAfterContainedResourceAndValueX, {
  Bundle bundle = ParseFromString<Bundle>(
      R"proto(entry: {
                resource: {
                  patient: { deceased: { boolean: { value: true } } }
                }
              })proto");

  Boolean expected = ParseFromString<Boolean>("value: true");

  FHIR_ASSERT_OK_AND_ASSIGN(auto result,
                            Evaluate(bundle, "entry[0].resource.deceased"));
  EXPECT_THAT(result.GetMessages(), ElementsAreArray({EqualsProto(expected)}));
})

TEST(FhirPathTest, PathNavigationAfterContainedResourceR4Any) {
  auto contained = ParseFromString<r4::core::ContainedResource>(
      "observation { value: { string_value: { value: 'bar' } } } ");
  auto patient = ParseFromString<r4::core::Patient>(
      "deceased: { boolean: { value: true } }");
  patient.add_contained()->PackFrom(contained);

  EXPECT_THAT(r4test::Evaluate(patient, "contained.value"),
              EvalsToStringThatMatches(StrEq("bar")));
}

FHIR_VERSION_TEST(FhirPathTest, ResourceReference, {
  Bundle bundle = ParseFromString<Bundle>(
      R"proto(entry: {
                resource: {
                  patient: { deceased: { boolean: { value: true } } }
                }
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
                        observation: {
                          value: { string_value: { value: "bar" } }
                        }
                      }
                    }
                  }
                }
              })proto");

  EXPECT_THAT(Evaluate(bundle, "%resource").ValueOrDie().GetMessages(),
              ElementsAreArray({EqualsProto(bundle)}));

  EXPECT_THAT(
      Evaluate(bundle, "entry[0].resource.select(%resource)")
          .ValueOrDie()
          .GetMessages(),
      ElementsAreArray({EqualsProto(bundle.entry(0).resource().patient())}));

  EXPECT_THAT(
      Evaluate(bundle, "entry[0].resource.select(%resource).select(%resource)")
          .ValueOrDie()
          .GetMessages(),
      ElementsAreArray({EqualsProto(bundle.entry(0).resource().patient())}));

  EXPECT_THAT(
      Evaluate(bundle, "entry[0].resource.deceased.select(%resource)")
          .ValueOrDie()
          .GetMessages(),
      ElementsAreArray({EqualsProto(bundle.entry(0).resource().patient())}));

  EXPECT_THAT(Evaluate(bundle, "entry[1].resource.select(%resource)")
                  .ValueOrDie()
                  .GetMessages(),
              ElementsAreArray(
                  {EqualsProto(bundle.entry(1).resource().observation())}));

  EXPECT_THAT(Evaluate(bundle, "entry[1].resource.value.select(%resource)")
                  .ValueOrDie()
                  .GetMessages(),
              ElementsAreArray(
                  {EqualsProto(bundle.entry(1).resource().observation())}));

  EXPECT_THAT(
      Evaluate(bundle, "entry[2].resource.select(%resource)")
          .ValueOrDie()
          .GetMessages(),
      ElementsAreArray({EqualsProto(bundle.entry(2).resource().bundle())}));

  EXPECT_THAT(
      Evaluate(bundle, "entry[2].resource.entry[0].resource.select(%resource)")
          .ValueOrDie()
          .GetMessages(),
      ElementsAreArray({EqualsProto(bundle.entry(2)
                                        .resource()
                                        .bundle()
                                        .entry(0)
                                        .resource()
                                        .observation())}));

  EXPECT_THAT(Evaluate(bundle, "entry.resource.select(%resource)")
                  .ValueOrDie()
                  .GetMessages(),
              UnorderedElementsAreArray(
                  {EqualsProto(bundle.entry(0).resource().patient()),
                   EqualsProto(bundle.entry(1).resource().observation()),
                   EqualsProto(bundle.entry(2).resource().bundle())}));

  // Note: The spec states that %resources resolves to "the resource that
  // contains the original node that is in %context." Given that the literal
  // 'true' is not contained by any resources it is believed that this should
  // result in an error.
  EXPECT_EQ(Evaluate(bundle, "true.select(%resource)").status().message(),
            "No Resource found in ancestry.");

  // Likewise, derived values do not have a defined %resource.
  EXPECT_EQ(Evaluate(bundle,
                     "(entry[2].resource.entry[0].resource.value & "
                     "entry[1].resource.value).select(%resource)")
                .status()
                .message(),
            "No Resource found in ancestry.");

  EXPECT_THAT(Evaluate(bundle,
                       "(entry[2].resource.entry[0].resource.value | "
                       "entry[1].resource.value | %resource).select(%resource)")
                  .ValueOrDie()
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
})

}  // namespace

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
