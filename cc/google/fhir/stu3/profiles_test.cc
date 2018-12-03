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
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/stu3/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "proto/stu3/profiles.pb.h"
#include "proto/stu3/resources.pb.h"
#include "testdata/stu3/profiles/test.pb.h"

namespace google {
namespace fhir {
namespace stu3 {

namespace {

using ::google::fhir::testutil::EqualsProto;
using ::google::fhir::testutil::EqualsProtoIgnoringReordering;

template <class B, class P>
void TestProfile(const string& filename) {
  const B unprofiled = ReadProto<B>(absl::StrCat(filename, ".prototxt"));
  P profiled;

  auto status = ConvertToProfileLenient(unprofiled, &profiled);
  if (!status.ok()) {
    LOG(ERROR) << status.error_message();
    ASSERT_TRUE(status.ok());
  }
  EXPECT_THAT(
      profiled,
      EqualsProto(ReadProto<P>(
          absl::StrCat(
              filename,
              "-profiled-",
              absl::AsciiStrToLower(P::descriptor()->name()),
              ".prototxt"))));
}

template <class B, class P>
void TestConvertToBaseResource(const string& filename) {
  const P profiled = ReadProto<P>(absl::StrCat(
      filename, "-profiled-", absl::AsciiStrToLower(P::descriptor()->name()),
      ".prototxt"));
  B unprofiled;

  auto status = ConvertToBaseResource(profiled, &unprofiled);
  if (!status.ok()) {
    LOG(ERROR) << status.error_message();
    ASSERT_TRUE(status.ok());
  }
  EXPECT_THAT(unprofiled, EqualsProtoIgnoringReordering(ReadProto<B>(
                              absl::StrCat(filename, ".prototxt"))));
}

template <class B, class P>
void TestPair(const string& filename) {
  TestProfile<B, P>(filename);
  TestConvertToBaseResource<B, P>(filename);
}

TEST(ProfilesTest, SimpleExtensions) {
  TestPair<proto::Observation, proto::ObservationGenetics>(
      "testdata/stu3/examples/Observation-example-genetics-1");
}

TEST(ProfilesTest, FixedCoding) {
  TestPair<proto::Observation, proto::Bodyheight>(
      "testdata/stu3/examples/Observation-body-height");
}

TEST(ProfilesTest, VitalSigns) {
  TestPair<proto::Observation, proto::Vitalsigns>(
      "testdata/stu3/examples/Observation-body-height");
}

TEST(ProfilesTest, FixedSystem) {
  TestPair<proto::Observation, ::google::fhir::stu3::testing::TestObservation>(
      "testdata/stu3/profiles/observation_fixedsystem");
}

TEST(ProfilesTest, ComplexExtension) {
  TestPair<proto::Observation, ::google::fhir::stu3::testing::TestObservation>(
      "testdata/stu3/profiles/observation_complexextension");
}

}  // namespace

}  // namespace stu3
}  // namespace fhir
}  // namespace google
