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
#include "google/fhir/seqex/text_tokenizer.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/example/example.pb.h"
#include "tensorflow/core/platform/env.h"

ABSL_DECLARE_FLAG(bool, tokenize_code_text_features);

namespace google {
namespace fhir {
namespace seqex {

using ::google::fhir::testutil::EqualsProto;

class ResourceToExampleTest : public ::testing::Test {
 public:
  void SetUp() override {
    parser_.AllowPartialMessage(true);
    tokenizer_ = std::make_shared<SimpleWordTokenizer>();
  }

 protected:
  google::protobuf::TextFormat::Parser parser_;
  std::shared_ptr<TextTokenizer> tokenizer_;
};

TEST_F(ResourceToExampleTest, Patient) {
  stu3::proto::Patient patient = PARSE_VALID_STU3_PROTO(R"(
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
  ASSERT_TRUE(parser_.ParseFromString(R"proto(
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
  )proto", &expected));

  ::tensorflow::Example output;
  ResourceToExample(patient, *tokenizer_, &output, false);
  EXPECT_THAT(output, EqualsProto(expected));
}

TEST_F(ResourceToExampleTest, PositiveInt) {
  stu3::proto::Encounter input;
  ASSERT_TRUE(parser_.ParseFromString(R"proto(
    diagnosis {
      condition { condition_id { value: "5845379952480009077" } }
      role {
        coding {
          system { value: "http://hl7.org/fhir/diagnosis-role" }
          code { value: "DD" }
        }
      }
      rank { value: 6789 }
    }
  )proto", &input));
  ::tensorflow::Example expected;
  ASSERT_TRUE(parser_.ParseFromString(
      R"proto(
        features {
          feature {
            key: "Encounter.diagnosis.rank"
            value { int64_list { value: 6789 } }
          }
          feature {
            key: "Encounter.diagnosis.role.http-hl7-org-fhir-diagnosis-role"
            value { bytes_list { value: "DD" } }
          }
        })proto",
      &expected));
  ::tensorflow::Example output;
  ResourceToExample(input, *tokenizer_, &output, false);
  EXPECT_THAT(output, EqualsProto(expected));
}

TEST_F(ResourceToExampleTest, HandlesCodeValueAsString) {
  stu3::proto::Binary input;
  ASSERT_TRUE(parser_.ParseFromString(R"proto(
                                        content_type { value: "bin" }
                                      )proto",
                                      &input));
  ::tensorflow::Example expected;
  ASSERT_TRUE(parser_.ParseFromString(
      R"proto(
        features {
          feature {
            key: "Binary.contentType"
            value { bytes_list { value: "bin" } }
          }
        })proto",
      &expected));
  ::tensorflow::Example output;
  ResourceToExample(input, *tokenizer_, &output, false);
  EXPECT_THAT(output, EqualsProto(expected));
}

TEST_F(ResourceToExampleTest, BinaryResourceWithContent) {
  stu3::proto::Binary input;
  ASSERT_TRUE(parser_.ParseFromString(R"proto(
                                        content_type { value: "bin" }
                                        content { value: "09832982033" }
                                      )proto",
                                      &input));
  ::tensorflow::Example expected;
  ASSERT_TRUE(parser_.ParseFromString(
      R"proto(
        features {
          feature {
            key: "Binary.contentType"
            value { bytes_list { value: "bin" } }
          }
        })proto",
      &expected));
  ::tensorflow::Example output;
  ResourceToExample(input, *tokenizer_, &output, false);
  EXPECT_THAT(output, EqualsProto(expected));
}

TEST_F(ResourceToExampleTest, SingletonCodeableConcepts) {
  absl::SetFlag(&FLAGS_tokenize_code_text_features, true);
  stu3::proto::Observation input;
  ASSERT_TRUE(parser_.ParseFromString(
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
  ASSERT_TRUE(parser_.ParseFromString(
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
  ResourceToExample(input, *tokenizer_, &output, false);
  EXPECT_THAT(output, EqualsProto(expected));
}

}  // namespace seqex
}  // namespace fhir
}  // namespace google
