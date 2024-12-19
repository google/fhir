// Copyright 2018 Google LLC
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

#include "google/fhir/codes.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/r4/core/codes.pb.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/encounter.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/family_member_history.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/metadata_resource.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/patient.pb.h"
#include "proto/google/fhir/proto/r4/uscore.pb.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace fhir {

namespace {

using ::google::fhir::testutil::EqualsProto;
using r4::core::Code;
using r4::core::ContainedResource;
using r4::core::Encounter;
using r4::core::FamilyMemberHistory;
using r4::core::MetadataResource;
using r4::core::Patient;
using r4::core::QuestionnaireItemOperatorCode;
using r4::core::ResourceTypeCode;

using ::testing::Test;

void TestCodeForResourceType(const google::protobuf::Message& resource,
                             const ResourceTypeCode::Value value) {
  const auto& statusOrValue =
      GetCodeForResourceType<ResourceTypeCode>(resource);
  EXPECT_TRUE(statusOrValue.ok())
      << "failed getting code for " << resource.GetTypeName();
  EXPECT_EQ(statusOrValue.value(), value);
}

TEST(CodesTest, GetCodeForResourceType) {
  TestCodeForResourceType(Encounter(), ResourceTypeCode::ENCOUNTER);
  TestCodeForResourceType(Patient(), ResourceTypeCode::PATIENT);
  TestCodeForResourceType(FamilyMemberHistory(),
                          ResourceTypeCode::FAMILY_MEMBER_HISTORY);
}

TEST(CodesTest, GetCodeForResourceType_AllContainedTypesValid) {
  google::protobuf::MessageFactory* factory =
      Patient().GetReflection()->GetMessageFactory();
  for (int i = 0; i < ContainedResource::descriptor()->field_count(); i++) {
    const google::protobuf::Descriptor* type =
        ContainedResource::descriptor()->field(i)->message_type();
    if (!IsMessageType<MetadataResource>(type)) {
      // "MetadataResource" is not a real resource, and is just meant as a
      // template
      EXPECT_TRUE(
          GetCodeForResourceType<ResourceTypeCode>(*factory->GetPrototype(type))
              .ok())
          << "Failed to find code for type: " << type->full_name();
    }
  }
}

void TestDescriptorForResourceType(const ::google::protobuf::Message& resource,
                                   const ResourceTypeCode::Value value) {
  const ::google::protobuf::EnumValueDescriptor* value_desc =
      ResourceTypeCode::Value_descriptor()->FindValueByNumber(value);
  const auto statusOrValue =
      GetDescriptorForResourceType<ContainedResource>(value_desc);
  EXPECT_TRUE(statusOrValue.ok())
      << "failed getting descriptor for " << resource.GetTypeName();
  EXPECT_EQ(statusOrValue.value()->full_name(), resource.GetTypeName());
}

TEST(CodesTest, GetDescriptorForResourceType) {
  TestDescriptorForResourceType(Encounter(), ResourceTypeCode::ENCOUNTER);
  TestDescriptorForResourceType(Patient(), ResourceTypeCode::PATIENT);
  TestDescriptorForResourceType(FamilyMemberHistory(),
                                ResourceTypeCode::FAMILY_MEMBER_HISTORY);
}

void TestTypedCodingConversion(const std::string& typed_file,
                               const std::string& untyped_file) {
  auto typed_golden =
      ReadProto<r4::uscore::PatientUSCoreRaceExtension::OmbCategoryCoding>(
          typed_file);
  auto generic_golden = ReadProto<r4::core::Coding>(untyped_file);

  r4::core::Coding generic_test;
  auto status_generic = CopyCoding(typed_golden, &generic_test);
  ASSERT_TRUE(status_generic.ok()) << status_generic.message();
  EXPECT_THAT(generic_test, EqualsProto(generic_golden));

  r4::uscore::PatientUSCoreRaceExtension::OmbCategoryCoding typed_test;
  auto status_typed = CopyCoding(generic_golden, &typed_test);
  ASSERT_TRUE(status_typed.ok()) << status_typed.message();
  EXPECT_THAT(typed_test, EqualsProto(typed_golden));
}

TEST(CodesTest, TypedCodingConversion) {
  // There are two tests here that show that the same Coding will print with
  // different systems depending on what the code is.
  TestTypedCodingConversion("testdata/r4/codes/uscore_omb_1_typed.prototxt",
                            "testdata/r4/codes/uscore_omb_1_raw.prototxt");
  TestTypedCodingConversion("testdata/r4/codes/uscore_omb_2_typed.prototxt",
                            "testdata/r4/codes/uscore_omb_2_raw.prototxt");
}

TEST(CodesTest, CodeStringToEnumValue) {
  auto enum_descriptor = QuestionnaireItemOperatorCode::Value_descriptor();
  auto enum_value_descriptor = enum_descriptor->FindValueByName("GREATER_THAN");
  auto result = CodeStringToEnumValue(">", enum_descriptor);
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(CodeStringToEnumValue(">", enum_descriptor).value()->full_name(),
            enum_value_descriptor->full_name());
}

TEST(CodesTest, GetCodeAsString_StringType) {
  Code code;
  code.set_value("foo");

  ASSERT_EQ("foo", GetCodeAsString(code).value());
}

TEST(CodesTest, GetCodeAsString_EnumType) {
  Patient::GenderCode code;
  code.set_value(r4::core::AdministrativeGenderCode::FEMALE);

  ASSERT_EQ("female", GetCodeAsString(code).value());
}

TEST(CodesTest, GetCodeAsString_InvalidType) {
  r4::core::String not_a_code;
  not_a_code.set_value("foo");
  ASSERT_FALSE(GetCodeAsString(not_a_code).ok());
}

TEST(CodesTest, EnumValueToCodeString) {
  ASSERT_EQ(
      "female",
      EnumValueToCodeString(
          r4::core::AdministrativeGenderCode::Value_descriptor()
              ->FindValueByNumber(r4::core::AdministrativeGenderCode::FEMALE)));
  ASSERT_EQ(
      ">",
      EnumValueToCodeString(
          r4::core::QuestionnaireItemOperatorCode::Value_descriptor()
              ->FindValueByNumber(
                  r4::core::QuestionnaireItemOperatorCode::GREATER_THAN)));
}

}  // namespace

}  // namespace fhir
}  // namespace google
