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

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/profiles.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/r4/datatypes.pb.h"
#include "proto/r4/google_extensions.pb.h"
#include "proto/r4/resources.pb.h"
#include "testdata/r4/profiles/test.pb.h"

namespace google {
namespace fhir {

namespace {

using ::google::fhir::r4::proto::Observation;
using ::google::fhir::r4::proto::Patient;
using ::google::fhir::testutil::EqualsProto;
using ::google::fhir::testutil::EqualsProtoIgnoringReordering;

template <class B>
B GetUnprofiled(const string& filename) {
  return ReadProto<B>(absl::StrCat(filename, ".prototxt"));
}

template <class P>
P GetProfiled(const string& filename) {
  return ReadProto<P>(absl::StrCat(
      filename, "-profiled-", absl::AsciiStrToLower(P::descriptor()->name()),
      ".prototxt"));
}

template <class B, class P>
void TestDownConvert(const string& filename) {
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
void TestUpConvert(const string& filename) {
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
void TestPair(const string& filename) {
  TestDownConvert<B, P>(filename);
  TestUpConvert<B, P>(filename);
}

TEST(ProfilesTest, InvalidInputs) {
  Patient patient;
  Observation observation;
  ASSERT_FALSE(ConvertToProfileLenientR4(patient, &observation).ok());
}

TEST(ProfilesTest, FixedSystem) {
  TestPair<Observation, ::google::fhir::r4::testing::TestObservation>(
      "testdata/r4/profiles/observation_fixedsystem");
}

TEST(ProfilesTest, ComplexExtension) {
  TestPair<Observation, ::google::fhir::r4::testing::TestObservation>(
      "testdata/r4/profiles/observation_complexextension");
}

TEST(ProfilesTest, Normalize) {
  const r4::testing::TestObservation unnormalized =
      ReadProto<r4::testing::TestObservation>(absl::StrCat(
          "testdata/r4/profiles/observation_complexextension.prototxt"));
  StatusOr<r4::testing::TestObservation> normalized = NormalizeR4(unnormalized);
  if (!normalized.status().ok()) {
    LOG(ERROR) << normalized.status().error_message();
    ASSERT_TRUE(normalized.status().ok());
  }
  EXPECT_THAT(
      normalized.ValueOrDie(),
      EqualsProto(ReadProto<r4::testing::TestObservation>(absl::StrCat(
          "testdata/r4/profiles/"
          "observation_complexextension-profiled-testobservation.prototxt"))));
}

TEST(ProfilesTest, NormalizeBundle) {
  r4::testing::Bundle unnormalized_bundle;

  *unnormalized_bundle.add_entry()
       ->mutable_resource()
       ->mutable_test_observation() =
      GetUnprofiled<r4::testing::TestObservation>(
          "testdata/r4/profiles/observation_complexextension");
  *unnormalized_bundle.add_entry()
       ->mutable_resource()
       ->mutable_test_observation_lvl2() =
      GetUnprofiled<r4::testing::TestObservationLvl2>(
          "testdata/r4/profiles/testobservation_lvl2");

  r4::testing::Bundle expected_normalized;
  *expected_normalized.add_entry()
       ->mutable_resource()
       ->mutable_test_observation() = GetProfiled<r4::testing::TestObservation>(
      "testdata/r4/profiles/observation_complexextension");
  *expected_normalized.add_entry()
       ->mutable_resource()
       ->mutable_test_observation_lvl2() =
      GetProfiled<r4::testing::TestObservationLvl2>(
          "testdata/r4/profiles/testobservation_lvl2");

  StatusOr<r4::testing::Bundle> normalized = NormalizeR4(unnormalized_bundle);
  EXPECT_THAT(normalized.ValueOrDie(),
              EqualsProtoIgnoringReordering(expected_normalized));
}

TEST(ProfilesTest, ProfileOfProfile) {
  TestPair<::google::fhir::r4::testing::TestObservation,
           ::google::fhir::r4::testing::TestObservationLvl2>(
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
  ::google::fhir::r4::testing::TestObservation profiled;

  auto lenient_status = ConvertToProfileLenientR4(unprofiled, &profiled);
  ASSERT_EQ(tensorflow::error::OK, lenient_status.code());

  auto strict_status = ConvertToProfileR4(unprofiled, &profiled);
  ASSERT_EQ(tensorflow::error::FAILED_PRECONDITION, strict_status.code());
}

}  // namespace

}  // namespace fhir
}  // namespace google
