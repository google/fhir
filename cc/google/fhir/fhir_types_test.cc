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
#include "proto/r4/core/datatypes.pb.h"
#include "proto/r4/core/extensions.pb.h"
#include "proto/r4/core/profiles/observation_bodyheight.pb.h"
#include "proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/r4/uscore.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/extensions.pb.h"
#include "proto/stu3/resources.pb.h"
#include "testdata/r4/profiles/test.pb.h"
#include "testdata/stu3/profiles/test.pb.h"

namespace google {
namespace fhir {

namespace {

#define FHIR_SIMPLE_TYPE_TEST(type)                         \
  TEST(FhirTypesTest, IsStu3##type) {                       \
    ASSERT_TRUE(Is##type(stu3::proto::type()));             \
    ASSERT_TRUE(Is##type(stu3::proto::type::descriptor())); \
  }                                                         \
  TEST(FhirTypesTest, IsR4##type) {                         \
    ASSERT_TRUE(Is##type(r4::core::type()));                \
    ASSERT_TRUE(Is##type(r4::core::type::descriptor()));    \
  }

#define FHIR_STU3_TYPE_TEST(type, stu3profile)                 \
  TEST(FhirTypesTest, Is##type##Stu3) {                        \
    ASSERT_TRUE(Is##type(stu3::proto::type()));                \
    ASSERT_FALSE(Is##type(stu3profile()));                     \
  }                                                            \
  TEST(FhirTypesTest, IsProfileOf##type##Stu3) {               \
    ASSERT_FALSE(IsProfileOf##type(stu3::proto::type()));      \
    ASSERT_TRUE(IsProfileOf##type(stu3profile()));             \
  }                                                            \
                                                               \
  TEST(FhirTypesTest, IsTypeOrProfileOf##type##Stu3) {         \
    ASSERT_TRUE(IsTypeOrProfileOf##type(stu3::proto::type())); \
    ASSERT_TRUE(IsTypeOrProfileOf##type(stu3profile()));       \
  }

#define FHIR_R4_TYPE_TEST(type, r4profile)                  \
  TEST(FhirTypesTest, Is##type##R4) {                       \
    ASSERT_TRUE(Is##type(r4::core::type()));                \
    ASSERT_FALSE(Is##type(r4profile()));                    \
  }                                                         \
                                                            \
  TEST(FhirTypesTest, IsProfileOf##type##R4) {              \
    ASSERT_FALSE(IsProfileOf##type(r4::core::type()));      \
    ASSERT_TRUE(IsProfileOf##type(r4profile()));            \
  }                                                         \
                                                            \
  TEST(FhirTypesTest, IsTypeOrProfileOf##type##R4) {        \
    ASSERT_TRUE(IsTypeOrProfileOf##type(r4::core::type())); \
    ASSERT_TRUE(IsTypeOrProfileOf##type(r4profile()));      \
  }

#define FHIR_TYPE_TEST(type, stu3profile, r4profile) \
  FHIR_STU3_TYPE_TEST(type, stu3profile);            \
  FHIR_R4_TYPE_TEST(type, r4profile);

FHIR_TYPE_TEST(Bundle, stu3::testing::Bundle, r4::testing::Bundle);
FHIR_TYPE_TEST(Code, stu3::proto::MimeTypeCode, r4::core::Bundle::TypeCode);
FHIR_TYPE_TEST(Extension, stu3::proto::DataElementAdministrativeStatus,
               r4::core::AddressADUse);

// CodeableConcept and Coding only profiled in R4
FHIR_SIMPLE_TYPE_TEST(CodeableConcept);
FHIR_R4_TYPE_TEST(CodeableConcept,
                  r4::core::ObservationBodyheight::CodeableConceptForCode);
FHIR_SIMPLE_TYPE_TEST(Coding);
FHIR_R4_TYPE_TEST(Coding,
                  r4::uscore::PatientUSCoreRaceExtension::OmbCategoryCoding);

FHIR_SIMPLE_TYPE_TEST(Boolean);
FHIR_SIMPLE_TYPE_TEST(String);
FHIR_SIMPLE_TYPE_TEST(Integer);
FHIR_SIMPLE_TYPE_TEST(UnsignedInt);
FHIR_SIMPLE_TYPE_TEST(PositiveInt);
FHIR_SIMPLE_TYPE_TEST(Decimal);
FHIR_SIMPLE_TYPE_TEST(DateTime);
FHIR_SIMPLE_TYPE_TEST(Date);
FHIR_SIMPLE_TYPE_TEST(Time);
FHIR_SIMPLE_TYPE_TEST(Quantity);
FHIR_SIMPLE_TYPE_TEST(SimpleQuantity);

}  // namespace

}  // namespace fhir
}  // namespace google
