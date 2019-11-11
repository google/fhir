// Copyright 2018 Google LLC
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

#include "google/fhir/resource_validation.h"

#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "google/fhir/test_helper.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status_test_util.h"

namespace google {
namespace fhir {

namespace {

using namespace stu3::proto;  // NOLINT

static google::protobuf::TextFormat::Parser parser;

template <typename T>
T ParseFromString(const std::string& str) {
  google::protobuf::TextFormat::Parser parser;
  parser.AllowPartialMessage(true);
  T t;
  EXPECT_TRUE(parser.ParseFromString(str, &t));
  return t;
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

CodeableConcept ValidCodeableConcept() {
  return ParseFromString<CodeableConcept>(R"proto(coding {
                                                    system { value: "foo" }
                                                    code { value: "bar" }
                                                  })proto");
}

Encounter ValidEncounter() {
  return ParseFromString<Encounter>(R"proto(
    status { value: TRIAGED }
    id { value: "123" }
  )proto");
}

template <typename T>
void ValidTest(const T& proto) {
  auto status = ValidateResource(proto);
  EXPECT_TRUE(status.ok()) << status;
}

template <typename T>
void InvalidTest(const std::string& err_msg, const T& proto) {
  EXPECT_EQ(ValidateResource(proto),
            ::tensorflow::errors::FailedPrecondition(err_msg));
}

TEST(ResourceValidationTest, MissingRequiredField) {
  Observation observation = ValidObservation();
  observation.clear_status();
  InvalidTest("missing-Observation.status", observation);
}

TEST(ResourceValidationTest, InvalidPrimitiveField) {
  Observation observation = ValidObservation();
  observation.mutable_value()->mutable_quantity()->mutable_value()->set_value(
      "1.2.3");
  InvalidTest("invalid-primitive-Observation.value.quantity.value",
              observation);
}

TEST(ResourceValidationTest, ValidReference) {
  Observation observation = ValidObservation();
  observation.add_related()
      ->mutable_target()
      ->mutable_observation_id()
      ->set_value("12345");
  ValidTest(observation);
}

TEST(ResourceValidationTest, InvalidReference) {
  Observation observation = ValidObservation();
  observation.add_related()->mutable_target()->mutable_patient_id()->set_value(
      "12345");
  InvalidTest(
      "invalid-reference-Observation.related.target-disallowed-type-Patient",
      observation);
}

TEST(ResourceValidationTest, RepeatedReferenceValid) {
  Encounter encounter = ValidEncounter();
  encounter.add_account()->mutable_account_id()->set_value("111");
  encounter.add_account()->mutable_account_id()->set_value("222");
  encounter.add_account()->mutable_account_id()->set_value("333");
  ValidTest(encounter);
}

TEST(ResourceValidationTest, RepeatedReferenceInvalid) {
  Encounter encounter = ValidEncounter();
  encounter.add_account()->mutable_account_id()->set_value("111");
  encounter.add_account()->mutable_patient_id()->set_value("222");
  encounter.add_account()->mutable_account_id()->set_value("333");
  InvalidTest("invalid-reference-Encounter.account-disallowed-type-Patient",
              encounter);
}

TEST(ResourceValidationTest, EmptyOneof) {
  Observation observation = ValidObservation();

  auto* component = observation.add_component();
  component->mutable_value();
  *(component->mutable_code()) = ValidCodeableConcept();
  InvalidTest(
      "empty-oneof-google.fhir.stu3.proto.Observation.Component.Value.value",
      observation);
}

TEST(BundleValidationTest, Valid) {
  google::protobuf::TextFormat::Parser parser;
  parser.AllowPartialMessage(true);
  Bundle bundle;
  ASSERT_TRUE(parser.ParseFromString(R"proto(
    type { value: COLLECTION }
    id { value: "123" }
    entry { resource { patient {} } }
  )proto", &bundle));

  EXPECT_TRUE(ValidateResource(bundle).ok());
}

TEST(EncounterValidationTest, StartLaterThanEnd) {
  google::protobuf::TextFormat::Parser parser;
  parser.AllowPartialMessage(true);
  Encounter encounter;
  ASSERT_TRUE(parser.ParseFromString(R"proto(
    id { value: "123" }
    status { value: FINISHED }
    subject { patient_id { value: "4" } }
    period {
      start {
        value_us: 5515680100000000  # "2144-10-13T21:21:40+00:00"
        timezone: "UTC"
        precision: SECOND
      }
      end {
        value_us: 5515679100000000  # "2144-10-13T21:05:00+00:00"
        timezone: "UTC"
        precision: SECOND
      }
    }
  )proto", &encounter));

  EXPECT_EQ(ValidateResource(encounter),
            ::tensorflow::errors::FailedPrecondition(
                "Encounter.period-start-time-later-than-end-time"));
}

TEST(EncounterValidationTest, StartLaterThanEndButEndHasDayPrecision) {
  google::protobuf::TextFormat::Parser parser;
  parser.AllowPartialMessage(true);
  Encounter encounter;
  ASSERT_TRUE(parser.ParseFromString(R"proto(
    id { value: "123" }
    status { value: FINISHED }
    subject { patient_id { value: "4" } }
    period {
      start {
        value_us: 5515680100000000  # "2144-10-13T21:21:40+00:00"
        timezone: "UTC"
        precision: SECOND
      }
      end {
        value_us: 5515679100000000  # "2144-10-13T21:05:00+00:00"
        timezone: "UTC"
        precision: DAY
      }
    }
  )proto", &encounter));

  TF_ASSERT_OK(ValidateResource(encounter));
}

TEST(EncounterValidationTest, Valid) {
  google::protobuf::TextFormat::Parser parser;
  parser.AllowPartialMessage(true);
  Encounter encounter;
  ASSERT_TRUE(parser.ParseFromString(R"proto(
    id { value: "123" }
    status { value: FINISHED }
    subject { patient_id { value: "4" } }
    period {
      start {
        value_us: 5515679100000000  # "2144-10-13T21:05:00+00:00"
        timezone: "UTC"
        precision: SECOND
      }
      end {
        value_us: 5515680100000000  # "2144-10-13T21:21:40+00:00"
        timezone: "UTC"
        precision: SECOND
      }
    }
  )proto", &encounter));

  TF_ASSERT_OK(ValidateResource(encounter));
}

}  // namespace

}  // namespace fhir
}  // namespace google
