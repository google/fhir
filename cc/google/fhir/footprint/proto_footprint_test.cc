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

#include "google/fhir/footprint/proto_footprint.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "google/fhir/test_helper.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r4/uscore.pb.h"
#include "proto/google/fhir/proto/stu3/datatypes.pb.h"
#include "proto/google/fhir/proto/stu3/resources.pb.h"
#include "proto/google/fhir/proto/stu3/uscore.pb.h"

namespace google {
namespace fhir {

namespace {

void TestDatatypeFile(const std::string& filename,
                      const google::protobuf::FileDescriptor* file) {
  EXPECT_EQ(ReadFile(filename), ComputeDatatypesFootprint(file));
}

void TestResource(const std::string& filename, const google::protobuf::Descriptor* type) {
  EXPECT_EQ(ReadFile(filename), ComputeResourceFootprint(type));
}

void TestResourceFile(const std::string& filename,
                      const google::protobuf::FileDescriptor* type) {
  EXPECT_EQ(ReadFile(filename), ComputeResourceFileFootprint(type));
}

TEST(ProtoFootprintTest, Stu3Datatypes) {
  TestDatatypeFile("testdata/stu3/footprint/stu3_datatypes.fp",
                   google::fhir::stu3::proto::String::descriptor()->file());
}

TEST(ProtoFootprintTest, R4Datatypes) {
  TestDatatypeFile("testdata/r4/footprint/r4_datatypes.fp",
                   google::fhir::r4::core::String::descriptor()->file());
}

TEST(ProtoFootprintTest, Stu3Bundle) {
  TestResource("testdata/stu3/footprint/stu3_contained_resource.fp",
               google::fhir::stu3::proto::ContainedResource::descriptor());
}

TEST(ProtoFootprintTest, R4Bundle) {
  TestResource("testdata/r4/footprint/r4_contained_resource.fp",
               google::fhir::r4::core::ContainedResource::descriptor());
}

TEST(ProtoFootprintTest, Stu3UsCore) {
  TestResourceFile(
      "testdata/stu3/footprint/stu3_uscore_resources.fp",
      google::fhir::stu3::uscore::UsCorePatient::descriptor()->file());
}

TEST(ProtoFootprintTest, R4UsCore) {
  TestResourceFile(
      "testdata/r4/footprint/r4_uscore_resources.fp",
      google::fhir::r4::uscore::USCorePatientProfile::descriptor()->file());
}

}  // namespace

}  // namespace fhir
}  // namespace google
