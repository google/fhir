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

#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/resource_validation.h"
#include "google/fhir/test_helper.h"
#include "proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/r4/core/resources/encounter.pb.h"
#include "proto/r4/core/resources/observation.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status_test_util.h"

namespace google {
namespace fhir {

namespace {

using namespace google::fhir::r4::core;  // NOLINT

template <typename T>
void ValidTest(const std::string& name) {
  auto status = ValidateResourceWithFhirPath(
      ReadProto<T>(absl::StrCat("testdata/r4/validation/", name, ".prototxt")));
  EXPECT_TRUE(status.ok()) << status;
}

template <typename T>
void InvalidTest(const std::string& name) {
  auto status = ValidateResourceWithFhirPath(
      ReadProto<T>(absl::StrCat("testdata/r4/validation/", name, ".prototxt")));

  std::string error_msg =
      ReadFile(absl::StrCat("testdata/r4/validation/", name, ".result.txt"));
  if (error_msg[error_msg.length() - 1] == '\n') {
    error_msg.erase(error_msg.length() - 1);
  }

  EXPECT_EQ(status, ::tensorflow::errors::FailedPrecondition(error_msg));
}

TEST(ResourceValidationTest, MissingRequiredField) {
  InvalidTest<Observation>("observation_invalid_missing_required");
}

TEST(ResourceValidationTest, InvalidPrimitiveField) {
  InvalidTest<Observation>("observation_invalid_primitive");
}

TEST(ResourceValidationTest, ValidReference) {
  ValidTest<Observation>("observation_valid_reference");
}

TEST(ResourceValidationTest, InvalidReference) {
  InvalidTest<Observation>("observation_invalid_reference");
}

TEST(ResourceValidationTest, FHIRPathViolation) {
  InvalidTest<Observation>("observation_invalid_fhirpath_violation");
}

TEST(ResourceValidationTest, RepeatedReferenceValid) {
  ValidTest<Encounter>("encounter_valid_repeated_reference");
}

TEST(ResourceValidationTest, RepeatedReferenceInvalid) {
  InvalidTest<Encounter>("encounter_invalid_repeated_reference");
}

TEST(ResourceValidationTest, EmptyOneof) {
  InvalidTest<Observation>("observation_invalid_empty_oneof");
}

TEST(BundleValidationTest, Valid) { ValidTest<Bundle>("bundle_valid"); }

TEST(EncounterValidationTest, StartLaterThanEnd) {
  InvalidTest<Encounter>("encounter_invalid_start_later_than_end");
}

TEST(EncounterValidationTest, StartLaterThanEndButEndHasDayPrecision) {
  ValidTest<Encounter>("encounter_valid_start_later_than_end_day_precision");
}

TEST(EncounterValidationTest, Valid) {
  ValidTest<Encounter>("encounter_valid");
}

TEST(EncounterValidationTest, ValidWithNumericTimezone) {
  ValidTest<Encounter>("encounter_valid_numeric_timezone");
}

}  // namespace

}  // namespace fhir
}  // namespace google
