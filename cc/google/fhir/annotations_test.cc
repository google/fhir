// Copyright 2022 Google LLC
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

#include "google/fhir/annotations.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "google/fhir/status/status.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/patient.pb.h"
#include "proto/google/fhir/proto/stu3/resources.pb.h"
#include "testdata/r4/profiles/test.pb.h"

namespace google::fhir {
namespace {

MATCHER_P(HasStatusCode, status_code, "") {
  return !arg.ok() && arg.code() == status_code;
}

TEST(AnnotationsTest, CheckVersionCoreProtoSucceeds) {
  FHIR_ASSERT_OK(CheckVersion(google::fhir::r4::core::Patient(),
                              google::fhir::proto::FhirVersion::R4));

  FHIR_ASSERT_OK(CheckVersion(google::fhir::stu3::proto::Patient(),
                              google::fhir::proto::FhirVersion::STU3));
}

TEST(AnnotationsTest, CheckVersionCoreProtoFails) {
  EXPECT_THAT(CheckVersion(google::fhir::r4::core::Patient(),
                           google::fhir::proto::FhirVersion::STU3),
              HasStatusCode(absl::StatusCode::kInvalidArgument));

  EXPECT_THAT(CheckVersion(google::fhir::stu3::proto::Patient(),
                           google::fhir::proto::FhirVersion::R4),
              HasStatusCode(absl::StatusCode::kInvalidArgument));
}

TEST(AnnotationsTest, CheckVersionProfiledProtoSucceeds) {
  FHIR_ASSERT_OK(CheckVersion(google::fhir::r4::testing::TestPatient(),
                              google::fhir::proto::FhirVersion::R4));
}

TEST(AnnotationsTest, CheckVersionProfiledProtoFails) {
  EXPECT_THAT(CheckVersion(google::fhir::r4::testing::TestPatient(),
                           google::fhir::proto::FhirVersion::STU3),
              HasStatusCode(absl::StatusCode::kInvalidArgument));
}

TEST(AnnotationsTest, CheckVersionContainedResourceSucceeds) {
  FHIR_ASSERT_OK(CheckVersion(google::fhir::r4::core::ContainedResource(),
                              google::fhir::proto::FhirVersion::R4));
  FHIR_ASSERT_OK(CheckVersion(google::fhir::stu3::proto::ContainedResource(),
                              google::fhir::proto::FhirVersion::STU3));
}

TEST(AnnotationsTest, CheckVersionContainedResourceFails) {
  EXPECT_THAT(CheckVersion(google::fhir::r4::core::ContainedResource(),
                           google::fhir::proto::FhirVersion::STU3),
              HasStatusCode(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(CheckVersion(google::fhir::stu3::proto::ContainedResource(),
                           google::fhir::proto::FhirVersion::R4),
              HasStatusCode(absl::StatusCode::kInvalidArgument));
}

}  // namespace

}  // namespace google::fhir
