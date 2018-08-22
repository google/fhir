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

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "google/fhir/seqex/example_key.h"

namespace google {
namespace fhir {
namespace seqex {

using std::string;

class ExampleKeyTest : public ::testing::Test {};

// Tests for util functions
TEST_F(ExampleKeyTest, ParseExampleKeyNoSource) {
  const string key = "000fbcd133bb95ef-Patient/9511:0-1699@6794148180";
  const ExampleKey expected = {
      "Patient/9511",                     // patient_id
      absl::FromUnixSeconds(6794148180),  // trigger_timestamp
      "",                                 // source
      0,                                  // start
      1699                                // end
  };
  ExampleKey output;
  output.FromString(key);
  EXPECT_EQ(expected, output);
}

TEST_F(ExampleKeyTest, ParseExampleKeyNegativeTimestampNoSource) {
  const string key = "f26dd962a28daeb1-Patient/123:0-2@-957312000";
  const ExampleKey expected = {
      "Patient/123",                      // patient_id
      absl::FromUnixSeconds(-957312000),  // trigger_timestamp
      "",                                 // source
      0,                                  // start
      2                                   // end
  };
  ExampleKey output;
  output.FromString(key);
  EXPECT_EQ(expected, output);
}

TEST_F(ExampleKeyTest, ParseExampleKeyDashesNoSource) {
  const string key =
      "00c4061415d4961b-Patient/45b7ca20-5cde-ee51b5d20127:0-67@1265414400";
  const ExampleKey expected = {
      "Patient/45b7ca20-5cde-ee51b5d20127",  // patient_id
      absl::FromUnixSeconds(1265414400),     // trigger_timestamp
      "",                                    // source
      0,                                     // start
      67                                     // end
  };
  ExampleKey output;
  output.FromString(key);
  EXPECT_EQ(expected, output);
}

TEST_F(ExampleKeyTest, GenerateExampleKeyNoSource) {
  const ExampleKey key = {
      "Patient/9511",                     // patient_id
      absl::FromUnixSeconds(6794148180),  // trigger_timestamp
      "",                                 // source
      0,                                  // start
      1699                                // end
  };
  const string expected_key = "1ab84f967a46f259-Patient/9511:0-1699@6794148180";
  EXPECT_EQ(expected_key, key.ToStringWithPrefix());
}

TEST_F(ExampleKeyTest, ParsePatientIdTimestampExampleKey) {
  const string key = "Patient/9511@-6794148180";
  const ExampleKey expected = {
      "Patient/9511",                      // patient_id
      absl::FromUnixSeconds(-6794148180),  // trigger_timestamp
      "",                                  // source
      0,                                   // start
      0                                    // end
  };
  ExampleKey output;
  output.FromPatientIdTimestampString(key);
  EXPECT_EQ(expected, output);
}

TEST_F(ExampleKeyTest, GeneratePatientIdTimestampExampleKey) {
  const string expected_key = "Patient/9511@6794148180";
  const ExampleKey key = {
      "Patient/9511",                     // patient_id
      absl::FromUnixSeconds(6794148180),  // trigger_timestamp
      "",                                 // source
      0,                                  // start
      0                                   // end
  };
  EXPECT_EQ(expected_key, key.ToPatientIdTimestampString());
}

TEST_F(ExampleKeyTest, GeneratePatientIdSourceExampleKey) {
  const string expected_key = "Patient/9511:Encounter/1";
  const ExampleKey key = {
      "Patient/9511",                     // patient_id
      absl::FromUnixSeconds(6794148180),  // trigger_timestamp
      "Encounter/1",                      // source
      0,                                  // start
      0                                   // end
  };
  EXPECT_EQ(expected_key, key.ToPatientIdSourceString());
}

// Tests with sequence_key_with_source
TEST_F(ExampleKeyTest, ParseExampleKey) {
  const string key = "000fbcd133bb95ef-Patient/9511:0-1699@6794148180";
  const ExampleKey expected = {
      "Patient/9511",                     // patient_id
      absl::FromUnixSeconds(6794148180),  // trigger_timestamp
      "",                                 // source
      0,                                  // start
      1699                                // end
  };
  ExampleKey output;
  output.FromString(key);
  EXPECT_EQ(expected, output);
}

TEST_F(ExampleKeyTest, ParseExampleKeyNegativeTimestamp) {
  const string key = "f26dd962a28daeb1-Patient/123:0-2@-957312000:Claim2/1";
  const ExampleKey expected = {
      "Patient/123",                      // patient_id
      absl::FromUnixSeconds(-957312000),  // trigger_timestamp
      "Claim2/1",                         // source
      0,                                  // start
      2                                   // end
  };
  ExampleKey output;
  output.FromString(key);
  EXPECT_EQ(expected, output);
}

TEST_F(ExampleKeyTest, ParseExampleKeyDashes) {
  const string key =
      "00c4061415d4961b-Patient/45b7ca20-5cde-ee51b5d20127:"
      "0-67@1265414400:Observation/c0ffee-deadbeef-123";
  const ExampleKey expected = {
      "Patient/45b7ca20-5cde-ee51b5d20127",   // patient_id
      absl::FromUnixSeconds(1265414400),      // trigger_timestamp
      "Observation/c0ffee-deadbeef-123",      // source
      0,                                      // start
      67                                      // end
  };
  ExampleKey output;
  output.FromString(key);
  EXPECT_EQ(expected, output);
}

}  // namespace seqex
}  // namespace fhir
}  // namespace google
