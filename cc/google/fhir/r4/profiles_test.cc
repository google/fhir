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

#include "google/fhir/r4/profiles.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/r4/core/resources/encounter.pb.h"
#include "proto/r4/core/resources/observation.pb.h"
#include "proto/r4/core/resources/patient.pb.h"
#include "proto/r4/uscore.pb.h"
#include "testdata/r4/profiles/test.pb.h"

namespace google {
namespace fhir {

namespace {

using ::google::fhir::r4::core::Encounter;
using ::google::fhir::r4::core::Observation;
using ::google::fhir::r4::core::Patient;
using ::google::fhir::r4::testing::TestEncounter;
using ::google::fhir::r4::testing::TestObservation;
using ::google::fhir::r4::testing::TestObservationLvl2;
using ::google::fhir::testutil::EqualsProto;
using ::google::fhir::testutil::EqualsProtoIgnoringReordering;

template <class B>
B GetUnprofiled(const std::string& filename) {
  return ReadProto<B>(absl::StrCat(filename, ".prototxt"));
}

template <class P>
P GetProfiled(const std::string& filename) {
  return ReadProto<P>(absl::StrCat(
      filename, "-profiled-", absl::AsciiStrToLower(P::descriptor()->name()),
      ".prototxt"));
}

template <class B, class P>
void TestDownConvert(const std::string& filename) {
  const B unprofiled = GetUnprofiled<B>(filename);
  P profiled;

  auto status = ConvertToProfileLenientR4(unprofiled, &profiled);
  if (!status.ok()) {
    LOG(ERROR) << status.error_message();
    ASSERT_TRUE(status.ok());
  }
  EXPECT_THAT(profiled, EqualsProto(GetProfiled<P>(filename)));
}

template <class B, class P>
void TestUpConvert(const std::string& filename) {
  const P profiled = GetProfiled<P>(filename);
  B unprofiled;

  auto status = ConvertToProfileLenientR4(profiled, &unprofiled);
  if (!status.ok()) {
    LOG(ERROR) << status.error_message();
    ASSERT_TRUE(status.ok());
  }
  EXPECT_THAT(unprofiled, EqualsProtoIgnoringReordering(ReadProto<B>(
                              absl::StrCat(filename, ".prototxt"))));
}

template <class B, class P>
void TestPair(const std::string& filename) {
  TestDownConvert<B, P>(filename);
  TestUpConvert<B, P>(filename);
}

TEST(ProfilesTest, InvalidInputs) {
  Patient patient;
  Observation observation;
  ASSERT_FALSE(ConvertToProfileLenientR4(patient, &observation).ok());
}

TEST(ProfilesTest, FixedSystem) {
  TestPair<Observation, TestObservation>(
      "testdata/r4/profiles/observation_fixedsystem");
}

TEST(ProfilesTest, ComplexExtension) {
  TestPair<Observation, TestObservation>(
      "testdata/r4/profiles/observation_complexextension");
}

TEST(ProfilesTest, UsCore) {
  TestPair<Patient, ::google::fhir::r4::uscore::USCorePatientProfile>(
      "testdata/r4/profiles/uscore_patient");
}

TEST(ProfilesTest, Normalize) {
  const TestObservation unnormalized = ReadProto<TestObservation>(absl::StrCat(
      "testdata/r4/profiles/observation_complexextension.prototxt"));
  StatusOr<TestObservation> normalized = NormalizeR4(unnormalized);
  if (!normalized.status().ok()) {
    LOG(ERROR) << normalized.status().error_message();
    ASSERT_TRUE(normalized.status().ok());
  }
  EXPECT_THAT(
      normalized.ValueOrDie(),
      EqualsProto(ReadProto<TestObservation>(absl::StrCat(
          "testdata/r4/profiles/"
          "observation_complexextension-profiled-testobservation.prototxt"))));
}

TEST(ProfilesTest, NormalizeBundle) {
  r4::testing::Bundle unnormalized_bundle;

  *unnormalized_bundle.add_entry()
       ->mutable_resource()
       ->mutable_test_observation() = GetUnprofiled<TestObservation>(
      "testdata/r4/profiles/observation_complexextension");
  *unnormalized_bundle.add_entry()
       ->mutable_resource()
       ->mutable_test_observation_lvl2() = GetUnprofiled<TestObservationLvl2>(
      "testdata/r4/profiles/testobservation_lvl2");

  r4::testing::Bundle expected_normalized;
  *expected_normalized.add_entry()
       ->mutable_resource()
       ->mutable_test_observation() = GetProfiled<TestObservation>(
      "testdata/r4/profiles/observation_complexextension");
  *expected_normalized.add_entry()
       ->mutable_resource()
       ->mutable_test_observation_lvl2() = GetProfiled<TestObservationLvl2>(
      "testdata/r4/profiles/testobservation_lvl2");

  StatusOr<r4::testing::Bundle> normalized = NormalizeR4(unnormalized_bundle);
  EXPECT_THAT(normalized.ValueOrDie(),
              EqualsProtoIgnoringReordering(expected_normalized));
}

TEST(ProfilesTest, ProfileOfProfile) {
  TestPair<TestObservation, TestObservationLvl2>(
      "testdata/r4/profiles/testobservation_lvl2");
}

TEST(ProfilesTest, UnableToProfile) {
  const Observation unprofiled = ReadProto<Observation>(
      "testdata/r4/examples/Observation-example-genetics-1.prototxt");
  Patient patient;

  auto lenient_status = ConvertToProfileLenientR4(unprofiled, &patient);
  ASSERT_EQ(tensorflow::error::INVALID_ARGUMENT, lenient_status.code());

  auto strict_status = ConvertToProfileR4(unprofiled, &patient);
  ASSERT_EQ(tensorflow::error::INVALID_ARGUMENT, strict_status.code());
}

TEST(ProfilesTest, MissingRequiredFields) {
  const Observation unprofiled = ReadProto<Observation>(
      "testdata/r4/profiles/observation_fixedsystem.prototxt");
  TestObservation profiled;

  auto lenient_status = ConvertToProfileLenientR4(unprofiled, &profiled);
  ASSERT_EQ(tensorflow::error::OK, lenient_status.code());

  auto strict_status = ConvertToProfileR4(unprofiled, &profiled);
  ASSERT_EQ(tensorflow::error::FAILED_PRECONDITION, strict_status.code());
}

TEST(ProfilesTest, ConvertToInlinedCodeEnum) {
  TestPair<Encounter, TestEncounter>(
      "testdata/r4/profiles/encounter_inlinedcodeenum");
}

}  // namespace

}  // namespace fhir
}  // namespace google
