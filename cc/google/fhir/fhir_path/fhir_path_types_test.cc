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

#include "google/fhir/fhir_path/fhir_path_types.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "google/fhir/util.h"
#include "google/fhir/type_macros.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"

namespace google::fhir::fhir_path::internal {

using testing::Not;
using testing::Pointee;
using testing::Truly;

template <typename T>
void TestType(FhirPathSystemType expected_type) {
  T proto;
  StatusOr<FhirPathSystemType> system_type = GetSystemType(proto);

  if (!system_type.ok()) {
    ADD_FAILURE() << "No system type defined for " << proto.GetTypeName()
                  << ": " << system_type.status();
  }

  EXPECT_EQ(system_type.value(), expected_type)
      << "Wrong system type for " << proto.GetTypeName();
}

template <typename... T>
struct FhirPrimitiveTypes {
  static void VerifySystemTypeMapping(FhirPathSystemType expected_type) {
    (TestType<T>(expected_type), ...);
  }

  static std::vector<std::unique_ptr<::google::protobuf::Message>> GetInstances() {
    std::vector<std::unique_ptr<::google::protobuf::Message>> results;
    (results.push_back(std::make_unique<T>()), ...);
    return results;
  }
};

template <typename BundleType, typename... AdditionalStringTypes>
struct FhirStringTypes
    : public FhirPrimitiveTypes<
          FHIR_DATATYPE(BundleType, string_value),
          FHIR_DATATYPE(BundleType, uri), FHIR_DATATYPE(BundleType, code),
          FHIR_DATATYPE(BundleType, oid), FHIR_DATATYPE(BundleType, id),
          FHIR_DATATYPE(BundleType, markdown),
          FHIR_DATATYPE(BundleType, base64_binary), AdditionalStringTypes...> {
};

template <typename BundleType>
struct FhirTypes {
  using Booleans = FhirPrimitiveTypes<FHIR_DATATYPE(BundleType, boolean)>;

  using Integers = FhirPrimitiveTypes<FHIR_DATATYPE(BundleType, integer),
                                      FHIR_DATATYPE(BundleType, unsigned_int),
                                      FHIR_DATATYPE(BundleType, positive_int)>;

  using Strings = FhirStringTypes<BundleType>;

  using Decimals = FhirPrimitiveTypes<FHIR_DATATYPE(BundleType, decimal)>;

  using Dates = FhirPrimitiveTypes<FHIR_DATATYPE(BundleType, date)>;

  using DateTimes = FhirPrimitiveTypes<FHIR_DATATYPE(BundleType, date_time),
                                       FHIR_DATATYPE(BundleType, instant)>;

  using Times = FhirPrimitiveTypes<FHIR_DATATYPE(BundleType, time)>;

  using Quantities = FhirPrimitiveTypes<FHIR_DATATYPE(BundleType, quantity)>;
};

struct Stu3Types : public FhirTypes<stu3::proto::Bundle> {};

struct R4Types : public FhirTypes<r4::core::Bundle> {
  using Strings = FhirStringTypes<r4::core::Bundle, r4::core::Uuid,
                                  r4::core::Canonical, r4::core::Url>;
};

template <typename T>
class FhirPathTypesTest : public ::testing::Test {};

using TestTypes = ::testing::Types<Stu3Types, R4Types>;
TYPED_TEST_SUITE(FhirPathTypesTest, TestTypes);

TYPED_TEST(FhirPathTypesTest, GetSystemType) {
  TypeParam::Booleans::VerifySystemTypeMapping(FhirPathSystemType::kBoolean);
  TypeParam::Integers::VerifySystemTypeMapping(FhirPathSystemType::kInteger);
  TypeParam::Strings::VerifySystemTypeMapping(FhirPathSystemType::kString);
  TypeParam::Decimals::VerifySystemTypeMapping(FhirPathSystemType::kDecimal);
  TypeParam::Dates::VerifySystemTypeMapping(FhirPathSystemType::kDate);
  TypeParam::DateTimes::VerifySystemTypeMapping(FhirPathSystemType::kDateTime);
  TypeParam::Times::VerifySystemTypeMapping(FhirPathSystemType::kTime);
  TypeParam::Quantities::VerifySystemTypeMapping(FhirPathSystemType::kQuantity);
}

TYPED_TEST(FhirPathTypesTest, IsSystemInteger) {
  EXPECT_THAT(TypeParam::Booleans::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemInteger)))));
  EXPECT_THAT(TypeParam::Integers::GetInstances(),
              Each(Pointee(Truly(IsSystemInteger))));
  EXPECT_THAT(TypeParam::Strings::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemInteger)))));
  EXPECT_THAT(TypeParam::Decimals::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemInteger)))));
  EXPECT_THAT(TypeParam::Dates::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemInteger)))));
  EXPECT_THAT(TypeParam::DateTimes::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemInteger)))));
  EXPECT_THAT(TypeParam::Times::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemInteger)))));
  EXPECT_THAT(TypeParam::Quantities::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemInteger)))));
}

TYPED_TEST(FhirPathTypesTest, IsSystemString) {
  EXPECT_THAT(TypeParam::Booleans::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemString)))));
  EXPECT_THAT(TypeParam::Integers::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemString)))));
  EXPECT_THAT(TypeParam::Strings::GetInstances(),
              Each(Pointee(Truly(IsSystemString))));
  EXPECT_THAT(TypeParam::Decimals::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemString)))));
  EXPECT_THAT(TypeParam::Dates::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemString)))));
  EXPECT_THAT(TypeParam::DateTimes::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemString)))));
  EXPECT_THAT(TypeParam::Times::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemString)))));
  EXPECT_THAT(TypeParam::Quantities::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemString)))));
}

TYPED_TEST(FhirPathTypesTest, IsSystemDecimal) {
  EXPECT_THAT(TypeParam::Booleans::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemDecimal)))));
  EXPECT_THAT(TypeParam::Integers::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemDecimal)))));
  EXPECT_THAT(TypeParam::Strings::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemDecimal)))));
  EXPECT_THAT(TypeParam::Decimals::GetInstances(),
              Each(Pointee(Truly(IsSystemDecimal))));
  EXPECT_THAT(TypeParam::Dates::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemDecimal)))));
  EXPECT_THAT(TypeParam::DateTimes::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemDecimal)))));
  EXPECT_THAT(TypeParam::Times::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemDecimal)))));
  EXPECT_THAT(TypeParam::Quantities::GetInstances(),
              Each(Pointee(Not(Truly(IsSystemDecimal)))));
}

}  // namespace google::fhir::fhir_path::internal
