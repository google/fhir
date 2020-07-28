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

#include "google/fhir/seqex/resource_to_example.h"

#include <memory>

#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/memory/memory.h"
#include "google/fhir/r4/primitive_handler.h"
#include "google/fhir/seqex/text_tokenizer.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/fhir_test_env.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/r4/core/resources/binary.pb.h"
#include "proto/r4/core/resources/encounter.pb.h"
#include "proto/r4/core/resources/observation.pb.h"
#include "proto/r4/core/resources/patient.pb.h"
#include "tensorflow/core/example/example.pb.h"
#include "tensorflow/core/platform/env.h"

ABSL_DECLARE_FLAG(bool, tokenize_code_text_features);

namespace google {
namespace fhir {
namespace seqex {

namespace {

using ::google::fhir::testutil::EqualsProto;

template <typename FhirTestEnv>
class ResourceToExampleTest : public ::testing::Test {
 public:
  void SetUp() override {
    this->parser_.AllowPartialMessage(true);
    tokenizer_ = std::make_shared<SimpleWordTokenizer>();
  }

  void ResourceToExample(const google::protobuf::Message& message,
                         const TextTokenizer& tokenizer,
                         ::tensorflow::Example* example,
                         bool enable_attribution) {
    seqex::ResourceToExample(message, *tokenizer_, example, enable_attribution,
                             FhirTestEnv::PrimitiveHandler::GetInstance());
  }

  google::protobuf::TextFormat::Parser parser_;
  std::shared_ptr<TextTokenizer> tokenizer_;
};

using TestEnvs =
    ::testing::Types<testutil::Stu3CoreTestEnv, testutil::R4CoreTestEnv>;
TYPED_TEST_SUITE(ResourceToExampleTest, TestEnvs);

using ResourceToExampleTestR4Only =
    ResourceToExampleTest<testutil::R4CoreTestEnv>;

TYPED_TEST(ResourceToExampleTest, Patient) {
  typename TypeParam::Patient patient = PARSE_VALID_FHIR_PROTO(R"(
    id { value: "1" }
    gender { value: FEMALE }
    birth_date {
      value_us: 2167084800000000  # "2038-09-02T20:00:00-04:00"
      precision: DAY
      timezone: "UTC"
    }
    deceased { date_time {
      value_us: 4915468800000000  # "2125-10-06T20:00:00-04:00"
      timezone: "UTC"
      precision: SECOND
    } }
  )");
  ::tensorflow::Example expected;
  ASSERT_TRUE(this->parser_.ParseFromString(
      R"proto(
        features {
          feature {
            key: "Patient.birthDate"
            value { int64_list { value: 2167084800 } }
          }
          feature {
            key: "Patient.deceased.dateTime"
            value { int64_list { value: 4915468800 } }
          }
          feature {
            key: "Patient.gender"
            value { bytes_list { value: "female" } }
          }
        }
      )proto",
      &expected));

  ::tensorflow::Example output;
  this->ResourceToExample(patient, *this->tokenizer_, &output, false);
  EXPECT_THAT(output, EqualsProto(expected));
}

TEST_F(ResourceToExampleTestR4Only, PositiveInt) {
  r4::core::Encounter input;
  ASSERT_TRUE(this->parser_.ParseFromString(
      R"proto(
        diagnosis {
          condition { condition_id { value: "5845379952480009077" } }
          use {
            coding {
              system { value: "http://hl7.org/fhir/diagnosis-role" }
              code { value: "DD" }
            }
          }
          rank { value: 6789 }
        }
      )proto",
      &input));
  ::tensorflow::Example expected;
  ASSERT_TRUE(this->parser_.ParseFromString(
      R"proto(
        features {
          feature {
            key: "Encounter.diagnosis.rank"
            value { int64_list { value: 6789 } }
          }
          feature {
            key: "Encounter.diagnosis.use.http-hl7-org-fhir-diagnosis-role"
            value { bytes_list { value: "DD" } }
          }
        })proto",
      &expected));
  ::tensorflow::Example output;
  this->ResourceToExample(input, *this->tokenizer_, &output, false);
  EXPECT_THAT(output, EqualsProto(expected));
}

TYPED_TEST(ResourceToExampleTest, HandlesCodeValueAsString) {
  typename TypeParam::Binary input;
  ASSERT_TRUE(this->parser_.ParseFromString(R"proto(
                                              content_type { value: "bin" }
                                            )proto",
                                            &input));
  ::tensorflow::Example expected;
  ASSERT_TRUE(this->parser_.ParseFromString(
      R"proto(
        features {
          feature {
            key: "Binary.contentType"
            value { bytes_list { value: "bin" } }
          }
        })proto",
      &expected));
  ::tensorflow::Example output;
  this->ResourceToExample(input, *this->tokenizer_, &output, false);
  EXPECT_THAT(output, EqualsProto(expected));
}

TEST_F(ResourceToExampleTestR4Only, BinaryResourceWithContent) {
  r4::core::Binary input;
  ASSERT_TRUE(this->parser_.ParseFromString(R"proto(
                                              content_type { value: "bin" }
                                              data { value: "09832982033" }
                                            )proto",
                                            &input));
  ::tensorflow::Example expected;
  ASSERT_TRUE(this->parser_.ParseFromString(
      R"proto(
        features {
          feature {
            key: "Binary.contentType"
            value { bytes_list { value: "bin" } }
          }
        })proto",
      &expected));
  ::tensorflow::Example output;
  this->ResourceToExample(input, *this->tokenizer_, &output, false);
  EXPECT_THAT(output, EqualsProto(expected));
}

TYPED_TEST(ResourceToExampleTest, SingletonCodeableConcepts) {
  absl::SetFlag(&FLAGS_tokenize_code_text_features, true);
  typename TypeParam::Observation input;
  ASSERT_TRUE(this->parser_.ParseFromString(
      R"proto(
        code: {
          coding: {
            system { value: "http://test_codesystem_not_found.org" }
            code { value: "LAB50" }
          }
          text { value: "BILIRUBIN, TOTAL" }
        }
      )proto",
      &input));
  ::tensorflow::Example expected;
  ASSERT_TRUE(this->parser_.ParseFromString(
      R"proto(
        features {
          feature {
            key: "Observation.code.http-test_codesystem_not_found-org"
            value { bytes_list { value: "LAB50" } }
          }
          feature {
            key: "Observation.code.text.tokenized"
            value { bytes_list { value: "bilirubin" value: "total" } }
          }
        })proto",
      &expected));
  ::tensorflow::Example output;
  this->ResourceToExample(input, *this->tokenizer_, &output, false);
  EXPECT_THAT(output, EqualsProto(expected));
}

}  // namespace

}  // namespace seqex
}  // namespace fhir
}  // namespace google
