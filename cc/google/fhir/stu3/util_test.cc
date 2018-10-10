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

#include "google/fhir/stu3/util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/time/time.h"
#include "google/fhir/systems/systems.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/stu3/codes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/metadatatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace stu3 {

namespace {

using ::google::fhir::stu3::proto::ContainedResource;
using ::google::fhir::stu3::proto::Date;
using ::google::fhir::stu3::proto::DateTime;
using ::google::fhir::stu3::proto::Encounter;
using ::google::fhir::systems::kIcd9Schemes;
using ::google::fhir::testutil::EqualsProto;

TEST(GetTimeFromTimelikeElement, DateTime) {
  DateTime with_time;
  with_time.set_value_us(1517166000000000L);  // 2018-01-28 19:00:00
  DateTime time_not_set;

  EXPECT_EQ(absl::ToUnixMicros(
                google::fhir::stu3::GetTimeFromTimelikeElement(with_time)),
            1517166000000000L);
  EXPECT_EQ(absl::ToUnixMicros(
                google::fhir::stu3::GetTimeFromTimelikeElement(time_not_set)),
            0L);
}

TEST(GetTimeFromTimelikeElement, Date) {
  Date with_time;
  with_time.set_value_us(1517166000000000L);  // 2018-01-28 19:00:00
  Date time_not_set;

  EXPECT_EQ(absl::ToUnixMicros(
                google::fhir::stu3::GetTimeFromTimelikeElement(with_time)),
            1517166000000000L);
  EXPECT_EQ(absl::ToUnixMicros(
                google::fhir::stu3::GetTimeFromTimelikeElement(time_not_set)),
            0L);
}

TEST(GetTimezone, TimeZoneStr) {
  absl::TimeZone tz;

  // Valid cases.
  ASSERT_TRUE(GetTimezone("-06:00", &tz).ok());
  ASSERT_EQ(tz.name(), "Fixed/UTC-06:00:00");
  ASSERT_TRUE(GetTimezone("+06:00", &tz).ok());
  ASSERT_EQ(tz.name(), "Fixed/UTC+06:00:00");
  ASSERT_TRUE(GetTimezone("-06:30", &tz).ok());
  ASSERT_EQ(tz.name(), "Fixed/UTC-06:30:00");

  // Error cases.
  ASSERT_FALSE(GetTimezone("06:30", &tz).ok());
  ASSERT_FALSE(GetTimezone("-15:30", &tz).ok());
  ASSERT_FALSE(GetTimezone("-14:60", &tz).ok());
}

TEST(WrapContainedResource, Valid) {
  Encounter encounter;
  encounter.mutable_id()->set_value("47");

  ContainedResource expected;
  *(expected.mutable_encounter()) = encounter;
  auto result = WrapContainedResource(encounter);
  ASSERT_TRUE(result.status().ok());
  EXPECT_THAT(result.ValueOrDie(), EqualsProto(expected));
}

TEST(WrapContainedResource, InvalidType) {
  DateTime datetime;
  EXPECT_EQ(WrapContainedResource(datetime).status(),
            ::tensorflow::errors::InvalidArgument(
                "Resource type DateTime not found in "
                "fhir::Bundle::Entry::resource"));
}

TEST(SetContainedResource, Valid) {
  Encounter encounter;
  encounter.mutable_id()->set_value("47");

  ContainedResource expected;
  *(expected.mutable_encounter()) = encounter;

  ContainedResource result;
  ASSERT_TRUE(SetContainedResource(encounter, &result).ok());
  EXPECT_THAT(result, EqualsProto(expected));
}

TEST(SetContainedResource, InvalidType) {
  DateTime datetime;
  ContainedResource result;
  EXPECT_EQ(SetContainedResource(datetime, &result),
            ::tensorflow::errors::InvalidArgument(
                "Resource type DateTime not found in "
                "fhir::Bundle::Entry::resource"));
}

TEST(ExtractIcdCodeTest, ExtractIcd9) {
  const char kSystemCode[] = "http://hl7.org/fhir/sid/icd-9-cm";
  const char kCode[] = "1.1";
  CodeableConcept concept;
  auto coding = concept.add_coding();
  coding->mutable_system()->set_value(kSystemCode);
  coding->mutable_code()->set_value(kCode);
  // Shouldn't be extracted.
  coding = concept.add_coding();
  coding->mutable_system()->set_value("random system");
  coding->mutable_code()->set_value("1.2");
  string code;
  auto result = ExtractIcdCode(concept, *kIcd9Schemes);
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(kCode, result.ValueOrDie());
}

TEST(ExtractIcdCodeTest, ExtractIcd9_NotFound) {
  CodeableConcept concept;
  auto coding = concept.add_coding();
  coding->mutable_system()->set_value("random system");
  coding->mutable_code()->set_value("1.2");
  string code;
  auto result = ExtractIcdCode(concept, *kIcd9Schemes);
  ASSERT_FALSE(result.ok());
  EXPECT_EQ(::tensorflow::errors::Code::NOT_FOUND, result.status().code());
}

TEST(ExtractIcdCodeTest, ExtractIcd9_MultiCode) {
  CodeableConcept concept;
  auto coding = concept.add_coding();
  coding->mutable_system()->set_value("http://hl7.org/fhir/sid/icd-9-cm");
  coding->mutable_code()->set_value("1.1");
  coding = concept.add_coding();
  coding->mutable_system()->set_value(
      "http://hl7.org/fhir/sid/icd-9-cm/diagnosis");
  coding->mutable_code()->set_value("1.2");
  string code;
  auto result = ExtractIcdCode(concept, *kIcd9Schemes);
  ASSERT_FALSE(result.ok());
  EXPECT_EQ(::tensorflow::errors::Code::ALREADY_EXISTS, result.status().code());
}

}  // namespace
}  // namespace stu3
}  // namespace fhir
}  // namespace google
