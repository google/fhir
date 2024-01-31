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

#include "google/fhir/testutil/generator.h"

#include <memory>

#include "gtest/gtest.h"
#include "absl/time/time.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/r4/primitive_handler.h"
#include "google/fhir/status/status.h"
#include "google/fhir/testutil/fhir_test_env.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/r4/core/resources/observation.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/patient.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/plan_definition.pb.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace fhir {
namespace testutil {

namespace {

bool IsRequiredField(const google::protobuf::FieldDescriptor* field) {
  return field->options().HasExtension(
             ::google::fhir::proto::validation_requirement) &&
         field->options().GetExtension(
             ::google::fhir::proto::validation_requirement) ==
             ::google::fhir::proto::REQUIRED_BY_FHIR;
}

template <typename T>
class FhirGeneratorTest : public ::testing::Test {};

using TesEnvs =
    ::testing::Types<testutil::R4CoreTestEnv, testutil::Stu3CoreTestEnv>;
TYPED_TEST_SUITE(FhirGeneratorTest, TesEnvs);

TYPED_TEST(FhirGeneratorTest, TestAllRootFieldsSet) {
  // Create a random value provider that fills all non-recursive fields.
  RandomValueProvider::Params params = RandomValueProvider::DefaultParams();
  params.optional_set_probability = 1;
  FhirGenerator generator(absl::make_unique<RandomValueProvider>(params),
                          TypeParam::PrimitiveHandler::GetInstance());

  typename TypeParam::Patient patient;
  FHIR_ASSERT_OK(generator.Fill(&patient));

  for (int i = 0; i < patient.GetDescriptor()->field_count(); ++i) {
    const google::protobuf::FieldDescriptor* field = patient.GetDescriptor()->field(i);

    // "Any" fields not yet populated so skip them.
    if (!IsMessageType<::google::protobuf::Any>(field->message_type())) {
      EXPECT_TRUE(FieldHasValue(patient, field)) << field->full_name();
    }
  }
}

TYPED_TEST(FhirGeneratorTest, TestOnlyRequiredAndIdFieldsSet) {
  // Create a random value provider that fills only required fields.
  RandomValueProvider::Params params = RandomValueProvider::DefaultParams();
  params.optional_set_probability = 0;
  FhirGenerator generator(absl::make_unique<RandomValueProvider>(params),
                          TypeParam::PrimitiveHandler::GetInstance());
  typename TypeParam::Patient patient;
  FHIR_ASSERT_OK(generator.Fill(&patient));

  for (int i = 0; i < patient.GetDescriptor()->field_count(); ++i) {
    const google::protobuf::FieldDescriptor* field = patient.GetDescriptor()->field(i);
    if (IsRequiredField(field) || field->name() == "id") {
      EXPECT_TRUE(FieldHasValue(patient, field)) << field->full_name();
    } else {
      EXPECT_FALSE(FieldHasValue(patient, field)) << field->full_name();
    }
  }
}

// Test to ensure FHIR references use generic FHIR identifiers
// when the target resource type isn't known.
TEST(FhirGeneratorFieldsTest, TestUntypedReference) {
  // Use the R4 Observation focus field since it has a
  // Reference(Any) field.
  ::google::fhir::r4::core::Observation observation;
  RandomValueProvider::Params params = RandomValueProvider::DefaultParams();
  params.optional_set_probability = 1;
  FhirGenerator generator(
      absl::make_unique<RandomValueProvider>(params),
      ::google::fhir::r4::R4PrimitiveHandler::GetInstance());

  FHIR_ASSERT_OK(generator.Fill(&observation));
  ASSERT_GT(observation.focus_size(), 0);
  ASSERT_FALSE(observation.focus(0).has_uri());
  ASSERT_TRUE(observation.focus(0).has_identifier());
}

TEST(RandomValueProviderTest, TestMaxRecursionDepth) {
  RandomValueProvider::Params params = RandomValueProvider::DefaultParams();
  params.optional_set_probability = 1;
  // Disable decay in probability due to recursion
  params.optional_set_ratio_per_level = 1;
  // ... but max out at 2 levels deep.
  params.max_recursion_depth = 2;
  FhirGenerator generator(
      absl::make_unique<RandomValueProvider>(params),
      ::google::fhir::r4::R4PrimitiveHandler::GetInstance());

  ::google::fhir::r4::core::PlanDefinition plan_definition;

  FHIR_ASSERT_OK(generator.Fill(&plan_definition));

  ASSERT_GT(plan_definition.action_size(), 0);
  ASSERT_GT(plan_definition.action(0).action_size(), 0);
  ASSERT_EQ(plan_definition.action(0).action(0).action_size(), 0);
}

TEST(RandomValueProviderTest, TestFillExtensionsDefaultTrue) {
  RandomValueProvider::Params params = RandomValueProvider::DefaultParams();
  params.optional_set_probability = 1;
  params.optional_set_ratio_per_level = 1;
  params.max_recursion_depth = 1;
  FhirGenerator generator(
      std::make_unique<RandomValueProvider>(params),
      ::google::fhir::r4::R4PrimitiveHandler::GetInstance());

  ::google::fhir::r4::core::PlanDefinition plan_definition;

  FHIR_ASSERT_OK(generator.Fill(&plan_definition));

  EXPECT_GT(plan_definition.extension_size(), 0);
  EXPECT_GT(plan_definition.action(0).extension_size(), 0);
}

TEST(RandomValueProviderTest, TestFillExtensionsFalse) {
  RandomValueProvider::Params params = RandomValueProvider::DefaultParams();
  params.optional_set_probability = 1;
  params.optional_set_ratio_per_level = 1;
  params.max_recursion_depth = 1;
  params.fill_extensions = false;
  FhirGenerator generator(
      std::make_unique<RandomValueProvider>(params),
      ::google::fhir::r4::R4PrimitiveHandler::GetInstance());

  ::google::fhir::r4::core::PlanDefinition plan_definition;

  FHIR_ASSERT_OK(generator.Fill(&plan_definition));

  EXPECT_EQ(plan_definition.extension_size(), 0);
  EXPECT_EQ(plan_definition.action(0).extension_size(), 0);
}

}  // namespace

}  // namespace testutil
}  // namespace fhir
}  // namespace google
