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

#include "google/fhir/core_resource_registry.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "google/fhir/proto_util.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/r4/core/resources/observation.pb.h"
#include "proto/stu3/profile_config.pb.h"
#include "proto/stu3/resources.pb.h"
#include "testdata/r4/profiles/test.pb.h"
#include "testdata/stu3/profiles/test.pb.h"

namespace google {
namespace fhir {

namespace {

TEST(CoreResourceRegistryTest, Stu3Test) {
  auto result = GetBaseResourceInstance(stu3::testing::TestObservationLvl2());
  ASSERT_TRUE(result.ok());
  ASSERT_TRUE(IsMessageType<stu3::proto::Observation>(*result.ValueOrDie()))
      << result.ValueOrDie()->GetDescriptor()->full_name();
}

TEST(CoreResourceRegistryTest, R4Test) {
  auto result = GetBaseResourceInstance(r4::testing::TestObservationLvl2());
  ASSERT_TRUE(result.ok());
  ASSERT_TRUE(IsMessageType<r4::core::Observation>(*result.ValueOrDie()));
}

TEST(CoreResourceRegistryTest, NonResourceIsError) {
  auto result = GetBaseResourceInstance(r4::core::DateTime());
  ASSERT_FALSE(result.ok());
}

TEST(CoreResourceRegistryTest, NonFhirIsError) {
  auto result = GetBaseResourceInstance(proto::PackageInfo());
  ASSERT_FALSE(result.ok());
}

}  // namespace

}  // namespace fhir
}  // namespace google
