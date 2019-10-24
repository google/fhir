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

#include "google/fhir/fhir_types.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/stu3/resources.pb.h"
#include "testdata/r4/profiles/test.pb.h"
#include "testdata/stu3/profiles/test.pb.h"

namespace google {
namespace fhir {

namespace {

TEST(FhirTypesTest, IsBundleStu3) {
  ASSERT_TRUE(IsBundle(stu3::proto::Bundle()));
  ASSERT_FALSE(IsBundle(stu3::testing::Bundle()));
}

TEST(FhirTypesTest, IsProfileOfBundleStu3) {
  ASSERT_FALSE(IsProfileOfBundle(stu3::proto::Bundle()));
  ASSERT_TRUE(IsProfileOfBundle(stu3::testing::Bundle()));
}

TEST(FhirTypesTest, IsTypeOrProfileOfBundleStu3) {
  ASSERT_TRUE(IsTypeOrProfileOfBundle(stu3::proto::Bundle()));
  ASSERT_TRUE(IsTypeOrProfileOfBundle(stu3::testing::Bundle()));
}

TEST(FhirTypesTest, IsBundleR4) {
  ASSERT_TRUE(IsBundle(r4::core::Bundle()));
  ASSERT_FALSE(IsBundle(r4::testing::Bundle()));
}

TEST(FhirTypesTest, IsProfileOfBundleR4) {
  ASSERT_FALSE(IsProfileOfBundle(r4::core::Bundle()));
  ASSERT_TRUE(IsProfileOfBundle(r4::testing::Bundle()));
}

TEST(FhirTypesTest, IsTypeOrProfileOfBundleR4) {
  ASSERT_TRUE(IsTypeOrProfileOfBundle(r4::core::Bundle()));
  ASSERT_TRUE(IsTypeOrProfileOfBundle(r4::testing::Bundle()));
}

}  // namespace

}  // namespace fhir
}  // namespace google
