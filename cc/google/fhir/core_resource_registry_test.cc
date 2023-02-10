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

#include "gtest/gtest.h"
#include "google/fhir/proto_util.h"
#include "proto/google/fhir/proto/profile_config.pb.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/observation.pb.h"
#include "testdata/r4/profiles/test.pb.h"

namespace google {
namespace fhir {

namespace {

TEST(CoreResourceRegistryTest, R4Profile) {
  auto result = GetBaseResourceInstance(r4::testing::TestObservationLvl2());
  ASSERT_TRUE(result.ok());
  ASSERT_TRUE(IsMessageType<r4::core::Observation>(*result.value()));
}

TEST(CoreResourceRegistryTest, R4CoreResource) {
  auto result = GetBaseResourceInstance(r4::core::Observation());
  ASSERT_TRUE(result.ok());
  ASSERT_TRUE(IsMessageType<r4::core::Observation>(*result.value()));
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
