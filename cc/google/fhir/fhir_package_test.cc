// Copyright 2021 Google LLC
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

#include "google/fhir/fhir_package.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "google/fhir/status/statusor.h"

namespace google::fhir {

namespace {

constexpr int kR4DefinitionsCount = 653;
constexpr int kR4CodeSystemsCount = 1062;
constexpr int kR4ValuesetsCount = 1316;
constexpr int kR4SearchParametersCount = 1385;

TEST(FhirPackageTest, LoadTest) {
  absl::StatusOr<FhirPackage> fhir_package =
      FhirPackage::Load("spec/fhir_r4_package.zip");
  ASSERT_TRUE(fhir_package.ok()) << fhir_package.status().message();

  EXPECT_EQ(fhir_package->package_info.proto_package(), "google.fhir.r4.core");
  EXPECT_EQ(fhir_package->value_sets.size(), kR4ValuesetsCount);
  EXPECT_EQ(fhir_package->code_systems.size(), kR4CodeSystemsCount);
  EXPECT_EQ(fhir_package->structure_definitions.size(), kR4DefinitionsCount);
  EXPECT_EQ(fhir_package->search_parameters.size(), kR4SearchParametersCount);
}

}  // namespace

}  // namespace google::fhir
