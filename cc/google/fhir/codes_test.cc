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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "proto/stu3/codes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"

namespace google {
namespace fhir {

namespace {

using stu3::proto::ContainedResource;
using stu3::proto::Encounter;
using stu3::proto::FamilyMemberHistory;
using stu3::proto::Patient;
using stu3::proto::ResourceTypeCode;

using ::testing::Test;

void TestCodeForResourceType(const google::protobuf::Message& resource,
                             const ResourceTypeCode::Value value) {
  const auto& statusOrValue = GetCodeForResourceType(resource);
  EXPECT_TRUE(statusOrValue.ok())
      << "failed getting code for " << resource.GetTypeName();
  EXPECT_EQ(statusOrValue.ValueOrDie(), value);
}

TEST(CodesTest, GetCodeForResourceType) {
  TestCodeForResourceType(
      Encounter(), ResourceTypeCode::Value::ResourceTypeCode_Value_ENCOUNTER);
  TestCodeForResourceType(
      Patient(), ResourceTypeCode::Value::ResourceTypeCode_Value_PATIENT);
  TestCodeForResourceType(
      FamilyMemberHistory(),
      ResourceTypeCode::Value::ResourceTypeCode_Value_FAMILY_MEMBER_HISTORY);
}

TEST(CodesTest, GetCodeForResourceType_AllContainedTypesValid) {
  google::protobuf::MessageFactory* factory =
      Patient().GetReflection()->GetMessageFactory();
  for (int i = 0; i < ContainedResource::descriptor()->field_count(); i++) {
    const google::protobuf::Descriptor* type =
        ContainedResource::descriptor()->field(i)->message_type();
    EXPECT_TRUE(GetCodeForResourceType(*factory->GetPrototype(type)).ok())
        << "Failed to find code for type: " << type->full_name();
  }
}

}  // namespace

}  // namespace fhir
}  // namespace google
