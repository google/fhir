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

#include "google/fhir/fhir_path/fhir_path_validation.h"

#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/fhir_path/r4_fhir_path_validation.h"
#include "google/fhir/fhir_path/stu3_fhir_path_validation.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/r4/core/resources/encounter.pb.h"
#include "proto/r4/core/resources/medication_knowledge.pb.h"
#include "proto/r4/core/resources/observation.pb.h"
#include "proto/r4/core/resources/organization.pb.h"
#include "proto/r4/core/resources/value_set.pb.h"
#include "proto/r4/uscore.pb.h"
#include "proto/r4/uscore_codes.pb.h"
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

using ::google::protobuf::Message;

static ::google::protobuf::TextFormat::Parser parser;  // NOLINT

#define USING(FhirNamespace) \
using FhirNamespace::Boolean; \
using FhirNamespace::Decimal; \
using FhirNamespace::Encounter; \
using FhirNamespace::Observation; \
using FhirNamespace::Organization; \
using FhirNamespace::Period; \
using FhirNamespace::Quantity; \
using FhirNamespace::SimpleQuantity; \
using FhirNamespace::String; \
using FhirNamespace::ValueSet; \

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


template <typename T>
T ParseFromString(const std::string& str) {
  google::protobuf::TextFormat::Parser parser;
  parser.AllowPartialMessage(true);
  T t;
  EXPECT_TRUE(parser.ParseFromString(str, &t));
  return t;
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

template <typename T>
T ValidValueSet() {
  return ParseFromString<T>(R"proto(
    url { value: "http://example.com/valueset" }
  )proto");
}

template <typename T>
T ValidUsCorePatient() {
  return ParseFromString<T>(R"proto(
    identifier {
      system { value: "foo" },
      value: { value: "http://example.com/patient" }
    }
  )proto");
}

namespace r4test {
USING(::google::fhir::r4::core)
typedef ::google::fhir::r4::FhirPathValidator VersionedMessageValidator;
}  // namespace r4test

namespace stu3test {
USING(::google::fhir::stu3::proto)
typedef ::google::fhir::stu3::FhirPathValidator VersionedMessageValidator;
}  // namespace stu3test


FHIR_VERSION_TEST(FhirPathTest, ConstraintViolation, {
  auto organization = ParseFromString<Organization>(R"proto(
    name: {value: 'myorg'}
    telecom: { use: {value: HOME}}
  )proto");

  ValidationResults results =
      VersionedMessageValidator().Validate(organization);
  EXPECT_FALSE(results.IsValid());

  std::vector<ValidationResult> result_vector = results.Results();
  auto result =
      find_if(result_vector.begin(), result_vector.end(), [](auto result) {
        return result.Constraint() == "where(use = 'home').empty()" &&
               result.DebugPath() == "Organization.telecom";
      });
  ASSERT_TRUE(result != result_vector.end());
  ASSERT_FALSE((*result).EvaluationResult().ValueOrDie());
})

FHIR_VERSION_TEST(FhirPathTest, ConstraintSatisfied, {
  Observation observation = ValidObservation<Observation>();

  // Ensure constraint succeeds with a value in the reference range
  // as required by FHIR.
  auto ref_range = observation.add_reference_range();

  auto value = new Decimal();
  value->set_allocated_value(new std::string("123.45"));

  auto high = new SimpleQuantity();
  high->set_allocated_value(value);

  ref_range->set_allocated_high(high);

  EXPECT_TRUE(VersionedMessageValidator().Validate(observation).IsValid());
})

FHIR_VERSION_TEST(FhirPathTest, NestedConstraintViolated, {
  ValueSet value_set = ValidValueSet<ValueSet>();

  auto expansion = new ValueSet::Expansion;

  // Add empty contains structure to violate FHIR constraint.
  expansion->add_contains();
  value_set.mutable_name()->set_value("Placeholder");
  value_set.set_allocated_expansion(expansion);

  ValidationResults results = VersionedMessageValidator().Validate(value_set);
  EXPECT_FALSE(results.IsValid());

  std::vector<ValidationResult> result_vector = results.Results();
  auto result =
      find_if(result_vector.begin(), result_vector.end(), [](auto result) {
        return result.Constraint() ==
               "code.exists() or display.exists()";
      });
  ASSERT_TRUE(result != result_vector.end());
  ASSERT_FALSE((*result).EvaluationResult().ValueOrDie());
})

FHIR_VERSION_TEST(FhirPathTest, NestedConstraintSatisfied, {
  ValueSet value_set = ValidValueSet<ValueSet>();
  value_set.mutable_name()->set_value("Placeholder");

  auto expansion = new ValueSet::Expansion;
  auto contains = expansion->add_contains();

  // Contains struct has value to satisfy FHIR constraint.
  auto proto_string = new String();
  proto_string->set_value("Placeholder value");
  contains->set_allocated_display(proto_string);

  auto proto_boolean = new Boolean();
  proto_boolean->set_value(true);
  contains->set_allocated_abstract(proto_boolean);

  value_set.set_allocated_expansion(expansion);

  EXPECT_TRUE(VersionedMessageValidator().Validate(value_set).IsValid());
})

FHIR_VERSION_TEST(FhirPathTest, MessageLevelConstraint, {
  Period period = ParseFromString<Period>(R"proto(
    start: { value_us: 1556750000000000 timezone: "America/Los_Angeles" }
    end: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
  )proto");

  EXPECT_TRUE(VersionedMessageValidator().Validate(period).IsValid());
})

// TODO: Templatize tests to work with both STU3 and R4
TEST(FhirPathTest, MessageLevelConstraintViolated) {
  auto end_before_start_period = ParseFromString<r4::core::Period>(R"proto(
    start: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
    end: { value_us: 1556750000000000 timezone: "America/Los_Angeles" }
  )proto");

  EXPECT_FALSE(
      r4::FhirPathValidator().Validate(end_before_start_period).IsValid());
}

FHIR_VERSION_TEST(FhirPathTest, NestedMessageLevelConstraint, {
  auto start_with_no_end_encounter = ParseFromString<Encounter>(R"proto(
    status { value: TRIAGED }
    id { value: "123" }
    period {
      start: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
    }
  )proto");

  EXPECT_TRUE(VersionedMessageValidator()
                  .Validate(start_with_no_end_encounter)
                  .IsValid());
})

TEST(FhirPathTest, NestedMessageLevelConstraintViolated) {
  auto end_before_start_encounter = ParseFromString<r4::core::Encounter>(R"proto(
    status { value: TRIAGED }
    id { value: "123" }
    period {
      start: { value_us: 1556750153000000 timezone: "America/Los_Angeles" }
      end: { value_us: 1556750000000000 timezone: "America/Los_Angeles" }
    }
  )proto");

  EXPECT_FALSE(
      r4::FhirPathValidator().Validate(end_before_start_encounter).IsValid());
}

// TODO: Templatize tests to work with both STU3 and R4
TEST(FhirPathTest, ProfiledEmptyExtension) {
  r4::uscore::USCorePatientProfile patient =
      ValidUsCorePatient<r4::uscore::USCorePatientProfile>();
  EXPECT_TRUE(r4::FhirPathValidator().Validate(patient).IsValid());
}

TEST(FhirPathTest, ProfiledWithExtensionsR4) {
  auto patient = ValidUsCorePatient<r4::uscore::USCorePatientProfile>();
  auto race = new r4::uscore::PatientUSCoreRaceExtension();

  r4::uscore::PatientUSCoreRaceExtension::OmbCategoryCoding* coding =
      race->add_omb_category();
  coding->mutable_code()->set_value(
      r4::uscore::OmbRaceCategoriesValueSet::AMERICAN_INDIAN_OR_ALASKA_NATIVE);
  patient.set_allocated_race(race);

  patient.mutable_birthsex()->set_value(r4::uscore::BirthSexValueSet::M);

  EXPECT_TRUE(r4::FhirPathValidator().Validate(patient).IsValid());
}

TEST(FhirPathTest, ProfiledWithExtensionsSTU3) {
  auto patient = ValidUsCorePatient<stu3::uscore::UsCorePatient>();
  auto race = new stu3::uscore::PatientUSCoreRaceExtension();

  stu3::proto::Coding* coding = race->add_omb_category();
  coding->mutable_code()->set_value("urn:oid:2.16.840.1.113883.6.238");
  coding->mutable_code()->set_value("1002-5");
  patient.set_allocated_race(race);

  patient.mutable_birthsex()->set_value(stu3::uscore::UsCoreBirthSexCode::MALE);

  ASSERT_TRUE(stu3::FhirPathValidator().Validate(patient).IsValid());
}

}  // namespace

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
