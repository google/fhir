// Copyright 2020 Google LLC
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

#include "google/fhir/references.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/time/time.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/stu3/codes.pb.h"
#include "proto/google/fhir/proto/stu3/datatypes.pb.h"
#include "proto/google/fhir/proto/stu3/resources.pb.h"

namespace google {
namespace fhir {

namespace {

using ::google::fhir::stu3::proto::Encounter;
using ::google::fhir::stu3::proto::Patient;
using ::google::fhir::stu3::proto::Reference;
using ::google::fhir::testutil::EqualsProto;

TEST(GetResourceIdFromReferenceTest, PatientId) {
  Reference r;
  r.mutable_patient_id()->set_value("123");
  auto got = google::fhir::GetResourceIdFromReference<Patient>(r);
  FHIR_CHECK_OK(got.status());
  EXPECT_EQ(got.value(), "123");
}

TEST(GetResourceIdFromReferenceTest, PatientIdAsUri) {
  Reference r;
  r.mutable_uri()->set_value("Patient/123");
  auto got = google::fhir::GetResourceIdFromReference<Patient>(r);
  FHIR_CHECK_OK(got.status());
  EXPECT_EQ(got.value(), "123");
}

TEST(GetResourceIdFromReferenceTest, EncounterId) {
  Reference r;
  r.mutable_encounter_id()->set_value("456");
  auto got = google::fhir::GetResourceIdFromReference<Encounter>(r);
  FHIR_CHECK_OK(got.status());
  EXPECT_EQ(got.value(), "456");
}

TEST(GetResourceIdFromReferenceTest, EncounterIdAsUri) {
  Reference r;
  r.mutable_uri()->set_value("Encounter/456");
  auto got = google::fhir::GetResourceIdFromReference<Encounter>(r);
  FHIR_CHECK_OK(got.status());
  EXPECT_EQ(got.value(), "456");
}

TEST(GetResourceIdFromReferenceTest, ReferenceIsEmpty) {
  Reference r;
  auto got = google::fhir::GetResourceIdFromReference<Encounter>(r);
  EXPECT_EQ(got.status(), ::absl::NotFoundError("Reference is not populated."));
}

TEST(GetResourceIdFromReferenceTest, ReferenceDoesNotMatch) {
  Reference r;
  r.mutable_uri()->set_value("Patient/123");
  auto got = google::fhir::GetResourceIdFromReference<Encounter>(r);
  EXPECT_EQ(got.status(), ::absl::InvalidArgumentError(
                              "Reference `Patient/123` does not point to a "
                              "resource of type `Encounter`"));
}

TEST(GetReferenceProtoToResourceTest, UnprofiledProto) {
  Encounter resource;
  resource.mutable_id()->set_value("foo");

  Reference expected;
  expected.mutable_encounter_id()->set_value("foo");
  EXPECT_THAT(*google::fhir::GetReferenceProtoToResource<Reference>(resource),
              EqualsProto(expected));
}

TEST(ReferencesTest, RelativeReferenceSplit) {
  Reference r;
  r.mutable_uri()->set_value("Patient/123");
  ASSERT_TRUE(google::fhir::SplitIfRelativeReference(&r).ok());
  EXPECT_EQ(r.patient_id().value(), "123");
  EXPECT_TRUE(r.uri().value().empty());
}

TEST(ReferencesTest, UnrecognizedRelativeReferenceStaysAbsolute) {
  Reference r;
  r.mutable_uri()->set_value("Garbage/123");
  ASSERT_TRUE(google::fhir::SplitIfRelativeReference(&r).ok());
  EXPECT_EQ(r.uri().value(), "Garbage/123");
}

}  // namespace
}  // namespace fhir
}  // namespace google
