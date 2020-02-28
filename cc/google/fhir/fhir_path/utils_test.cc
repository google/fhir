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

#include "google/fhir/fhir_path/utils.h"

#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/r4/core/resources/patient.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"

namespace google {
namespace fhir {
namespace fhir_path {
namespace internal {
namespace {

using ::google::fhir::testutil::EqualsProto;
using ::google::protobuf::Message;
using ::google::protobuf::TextFormat;
using ::testing::UnorderedElementsAreArray;

namespace r4 = ::google::fhir::r4::core;
namespace stu3 = ::google::fhir::stu3::proto;

TEST(Utils, RetrieveFieldPrimitive) {
  r4::Boolean primitive;
  primitive.set_value(false);

  std::vector<const Message*> results;
  FHIR_ASSERT_OK(RetrieveField(
      primitive, *r4::Boolean::GetDescriptor()->FindFieldByName("value"),
      &results));

  ASSERT_THAT(results, UnorderedElementsAreArray({EqualsProto(primitive)}));
}

TEST(Utils, RetrieveFieldR4ContainedResource) {
  r4::Bundle_Entry entry;
  ASSERT_TRUE(
      TextFormat::ParseFromString(
          R"proto(resource: {
                    patient: { deceased: { boolean: { value: true } } }
                  })proto",
          &entry));
  r4::Patient patient = entry.resource().patient();

  std::vector<const Message*> results;
  FHIR_ASSERT_OK(RetrieveField(
      entry, *r4::Bundle_Entry::GetDescriptor()->FindFieldByName("resource"),
      &results));

  ASSERT_THAT(results, UnorderedElementsAreArray({EqualsProto(patient)}));
}

TEST(Utils, RetrieveFieldStu3ContainedResource) {
  stu3::Bundle_Entry entry;
  ASSERT_TRUE(
      TextFormat::ParseFromString(
          R"proto(resource: {
                    patient: { deceased: { boolean: { value: true } } }
                  })proto",
          &entry));
  stu3::Patient patient = entry.resource().patient();

  std::vector<const Message*> results;
  FHIR_ASSERT_OK(RetrieveField(
      entry, *stu3::Bundle_Entry::GetDescriptor()->FindFieldByName("resource"),
      &results));

  ASSERT_THAT(results, UnorderedElementsAreArray({EqualsProto(patient)}));
}

TEST(Utils, RetrieveFieldR4Choice) {
  r4::Patient patient;
  ASSERT_TRUE(TextFormat::ParseFromString(
      "deceased: { boolean: { value: true } }", &patient));
  r4::Boolean deceased = patient.deceased().boolean();

  std::vector<const Message*> results;
  FHIR_ASSERT_OK(RetrieveField(
      patient, *r4::Patient::GetDescriptor()->FindFieldByName("deceased"),
      &results));

  ASSERT_THAT(results, UnorderedElementsAreArray({EqualsProto(deceased)}));
}

TEST(Utils, RetrieveFieldStu3Choice) {
  stu3::Patient patient;
  ASSERT_TRUE(TextFormat::ParseFromString(
      "deceased: { boolean: { value: true } }", &patient));
  stu3::Boolean deceased = patient.deceased().boolean();

  std::vector<const Message*> results;
  FHIR_ASSERT_OK(RetrieveField(
      patient, *stu3::Patient::GetDescriptor()->FindFieldByName("deceased"),
      &results));

  ASSERT_THAT(results, UnorderedElementsAreArray({EqualsProto(deceased)}));
}

TEST(Utils, RetrieveFieldRepeated) {
  r4::Patient patient;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"proto(communication: { preferred: { value: true } }
              communication: { preferred: { value: false } })proto",
      &patient));
  r4::Patient_Communication communication1 = patient.communication(0);
  r4::Patient_Communication communication2 = patient.communication(1);

  std::vector<const Message*> results;
  FHIR_ASSERT_OK(RetrieveField(
      patient, *r4::Patient::GetDescriptor()->FindFieldByName("communication"),
      &results));

  ASSERT_THAT(results,
              UnorderedElementsAreArray(
                  {EqualsProto(communication1), EqualsProto(communication2)}));
}

TEST(Utils, FindFieldByJsonName) {
  // Default case
  EXPECT_EQ(FindFieldByJsonName(stu3::Encounter::descriptor(), "period"),
            stu3::Encounter::descriptor()->FindFieldByName("period"));

  // camelCase -> snake_case
  EXPECT_EQ(FindFieldByJsonName(stu3::Encounter::descriptor(), "statusHistory"),
            stu3::Encounter::descriptor()->FindFieldByName("status_history"));

  // Field with a JSON alias.
  EXPECT_EQ(FindFieldByJsonName(stu3::Encounter::descriptor(), "class"),
            stu3::Encounter::descriptor()->FindFieldByName("class_value"));
}

TEST(Utils, HasFieldWithJsonName) {
  EXPECT_TRUE(HasFieldWithJsonName(stu3::ContainedResource::descriptor(),
                                   "deceased"));
  EXPECT_TRUE(HasFieldWithJsonName(r4::ContainedResource::descriptor(),
                                   "deceased"));
}

}  // namespace
}  // namespace internal
}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
