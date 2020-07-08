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

#include "google/protobuf/descriptor.h"
#include "gtest/gtest.h"
#include "absl/time/time.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/testutil/fhir_test_env.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/r4/core/resources/patient.pb.h"

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
  FhirGenerator generator(absl::make_unique<RandomValueProvider>(1.0),
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

TYPED_TEST(FhirGeneratorTest, TestOnlyRequiredFieldsSet) {
  // Create a random value provider that fills only required fields.
  FhirGenerator generator(absl::make_unique<RandomValueProvider>(0.0),
                          TypeParam::PrimitiveHandler::GetInstance());
  typename TypeParam::Patient patient;
  FHIR_ASSERT_OK(generator.Fill(&patient));

  for (int i = 0; i < patient.GetDescriptor()->field_count(); ++i) {
    const google::protobuf::FieldDescriptor* field = patient.GetDescriptor()->field(i);
    if (IsRequiredField(field)) {
      EXPECT_TRUE(FieldHasValue(patient, field)) << field->full_name();
    } else {
      EXPECT_FALSE(FieldHasValue(patient, field)) << field->full_name();
    }
  }
}

}  // namespace

}  // namespace testutil
}  // namespace fhir
}  // namespace google
