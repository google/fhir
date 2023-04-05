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

#include "google/fhir/stu3/profiles.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/stu3/profiles.pb.h"
#include "proto/google/fhir/proto/stu3/resources.pb.h"
#include "proto/google/fhir/proto/stu3/uscore.pb.h"
#include "testdata/stu3/profiles/test.pb.h"

namespace google {
namespace fhir {

namespace {

using ::google::fhir::stu3::proto::Observation;
using ::google::fhir::stu3::proto::OperationOutcome;
using ::google::fhir::stu3::proto::Patient;
using ::google::fhir::stu3::testing::TestObservation;
using ::google::fhir::stu3::testing::TestObservationLvl2;
using ::google::fhir::testutil::EqualsProto;
using ::google::fhir::testutil::IgnoringRepeatedFieldOrdering;

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
  P expected_profiled = GetProfiled<P>(filename);

  auto result = stu3::ConvertToProfile(unprofiled, &profiled);
  // Ignore conversion OperationOutcome structure for bi-directional tests;
  // failure modes are tested elsewhere.
  EXPECT_TRUE(result.ok());
  EXPECT_THAT(profiled, EqualsProto(expected_profiled));

  // Test deprecated method as well.
  profiled.Clear();
  FHIR_ASSERT_OK(ConvertToProfileLenientStu3(unprofiled, &profiled));
  EXPECT_THAT(profiled, EqualsProto(expected_profiled));
}

template <class B, class P>
void TestUpConvert(const std::string& filename) {
  const P profiled = GetProfiled<P>(filename);
  B unprofiled;
  B expected_unprofiled = ReadProto<B>(absl::StrCat(filename, ".prototxt"));

  auto result = stu3::ConvertToProfile(profiled, &unprofiled);
  // Ignore conversion OperationOutcome structure for bi-directional tests;
  // failure modes are tested elsewhere.
  EXPECT_TRUE(result.ok());
  EXPECT_THAT(unprofiled,
              IgnoringRepeatedFieldOrdering(EqualsProto(expected_unprofiled)));

  // Test deprecated method as well.
  unprofiled.Clear();
  FHIR_ASSERT_OK(ConvertToProfileLenientStu3(profiled, &unprofiled));
  EXPECT_THAT(unprofiled,
              IgnoringRepeatedFieldOrdering(EqualsProto(expected_unprofiled)));
}

template <class B, class P>
void TestPair(const std::string& filename) {
  TestDownConvert<B, P>(filename);
  TestUpConvert<B, P>(filename);
}

TEST(ProfilesTest, IncompatibleTypes) {
  Patient patient;
  TestObservation observation;
  ASSERT_FALSE(ConvertToProfileLenientStu3(patient, &observation).ok());
}

TEST(ProfilesTest, SimpleExtensions) {
  TestPair<Observation, stu3::proto::ObservationGenetics>(
      "testdata/stu3/examples/Observation-example-genetics-1");
}

TEST(ProfilesTest, FixedCoding) {
  TestPair<Observation, stu3::proto::Bodyheight>(
      "testdata/stu3/examples/Observation-body-height");
}

TEST(ProfilesTest, VitalSigns) {
  TestPair<Observation, stu3::proto::Vitalsigns>(
      "testdata/stu3/examples/Observation-body-height");
}

TEST(ProfilesTest, FixedSystem) {
  TestPair<Observation, TestObservation>(
      "testdata/stu3/profiles/observation_fixedsystem");
}

TEST(ProfilesTest, ComplexExtension) {
  TestPair<Observation, TestObservation>(
      "testdata/stu3/profiles/observation_complexextension");
}

TEST(ProfilesTest, UsCore) {
  TestPair<Patient, ::google::fhir::stu3::uscore::UsCorePatient>(
      "testdata/stu3/profiles/uscore_patient");
}

TEST(ProfilesTest, Normalize) {
  const stu3::testing::TestObservation unnormalized =
      ReadProto<stu3::testing::TestObservation>(
          "testdata/stu3/profiles/observation_complexextension.prototxt");
  absl::StatusOr<stu3::testing::TestObservation> normalized =
      NormalizeStu3(unnormalized);
  if (!normalized.status().ok()) {
    LOG(ERROR) << normalized.status().message();
    ASSERT_TRUE(normalized.status().ok());
  }
  EXPECT_THAT(
      normalized.value(),
      EqualsProto(ReadProto<stu3::testing::TestObservation>(
          "testdata/stu3/profiles/"
          "observation_complexextension-profiled-testobservation.prototxt")));
}

TEST(ProfilesTest, NormalizeAndValidate_Invalid) {
  TestObservation unnormalized = ReadProto<TestObservation>(
      "testdata/stu3/profiles/observation_complexextension.prototxt");
  unnormalized.clear_status();
  absl::StatusOr<TestObservation> normalized =
      NormalizeAndValidateStu3(unnormalized);
  EXPECT_FALSE(normalized.ok());
  EXPECT_EQ(normalized.status().message(),
            "missing-required-field at TestObservation.status");
}

TEST(ProfilesTest, NormalizeAndValidate_Valid) {
  const TestObservation unnormalized = ReadProto<TestObservation>(
      "testdata/stu3/profiles/observation_complexextension.prototxt");
  absl::StatusOr<TestObservation> normalized =
      NormalizeAndValidateStu3(unnormalized);
  FHIR_ASSERT_OK(normalized.status());
  EXPECT_THAT(
      normalized.value(),
      EqualsProto(ReadProto<TestObservation>(
          "testdata/stu3/profiles/"
          "observation_complexextension-profiled-testobservation.prototxt")));
}

TEST(ProfilesTest, NormalizeBundle) {
  stu3::testing::Bundle unnormalized_bundle;

  *unnormalized_bundle.add_entry()
       ->mutable_resource()
       ->mutable_test_observation() =
      GetUnprofiled<stu3::testing::TestObservation>(
          "testdata/stu3/profiles/observation_complexextension");
  *unnormalized_bundle.add_entry()
       ->mutable_resource()
       ->mutable_test_observation_lvl2() =
      GetUnprofiled<stu3::testing::TestObservationLvl2>(
          "testdata/stu3/profiles/testobservation_lvl2");

  stu3::testing::Bundle expected_normalized;
  *expected_normalized.add_entry()
       ->mutable_resource()
       ->mutable_test_observation() =
      GetProfiled<stu3::testing::TestObservation>(
          "testdata/stu3/profiles/observation_complexextension");
  *expected_normalized.add_entry()
       ->mutable_resource()
       ->mutable_test_observation_lvl2() =
      GetProfiled<stu3::testing::TestObservationLvl2>(
          "testdata/stu3/profiles/testobservation_lvl2");

  absl::StatusOr<stu3::testing::Bundle> normalized =
      NormalizeStu3(unnormalized_bundle);
  EXPECT_THAT(normalized.value(),
              IgnoringRepeatedFieldOrdering(EqualsProto(expected_normalized)));
}

TEST(ProfilesTest, ProfileOfProfile) {
  TestPair<TestObservation, TestObservationLvl2>(
      "testdata/stu3/profiles/testobservation_lvl2");
}

TEST(ProfilesTest, UnableToProfile) {
  const Observation unprofiled = ReadProto<Observation>(
      "testdata/stu3/examples/Observation-example-genetics-1.prototxt");
  Patient patient;

  auto lenient_status = ConvertToProfileLenientStu3(unprofiled, &patient);
  ASSERT_EQ(absl::StatusCode::kInvalidArgument, lenient_status.code());

  auto strict_status = ConvertToProfileStu3(unprofiled, &patient);
  ASSERT_EQ(absl::StatusCode::kInvalidArgument, strict_status.code());
}

TEST(ProfilesTest, MissingRequiredFields) {
  const Observation unprofiled = ReadProto<Observation>(
      "testdata/stu3/profiles/observation_fixedsystem.prototxt");
  TestObservation profiled;

  auto lenient_status = ConvertToProfileLenientStu3(unprofiled, &profiled);
  ASSERT_EQ(absl::StatusCode::kOk, lenient_status.code());

  auto strict_status = ConvertToProfileStu3(unprofiled, &profiled);
  ASSERT_EQ(absl::StatusCode::kFailedPrecondition, strict_status.code())
      << "Incorrect status: " << strict_status.message();

  auto outcome = stu3::ConvertToProfile(unprofiled, &profiled);
  EXPECT_TRUE(outcome.ok());

  OperationOutcome expected;
  google::protobuf::TextFormat::ParseFromString(
      R"pb(
        issue {
          severity { value: ERROR }
          code { value: VALUE }
          diagnostics { value: "missing-required-field" }
          expression { value: "TestObservation.component[0].code" }
        }
      )pb",
      &expected);

  EXPECT_THAT(outcome.value(), EqualsProto(expected));
}

}  // namespace

}  // namespace fhir
}  // namespace google
