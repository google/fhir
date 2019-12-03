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

#include "google/fhir/util.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/time/time.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/stu3/codes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/metadatatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {

namespace {

using ::google::fhir::stu3::proto::Bundle;
using ::google::fhir::stu3::proto::CodeableConcept;
using ::google::fhir::stu3::proto::ContainedResource;
using ::google::fhir::stu3::proto::Date;
using ::google::fhir::stu3::proto::DateTime;
using ::google::fhir::stu3::proto::Encounter;
using ::google::fhir::testutil::EqualsProto;

TEST(GetTimeFromTimelikeElement, DateTime) {
  DateTime with_time;
  with_time.set_value_us(1517166000000000L);  // 2018-01-28 19:00:00
  DateTime time_not_set;

  EXPECT_EQ(
      absl::ToUnixMicros(google::fhir::GetTimeFromTimelikeElement(with_time)),
      1517166000000000L);
  EXPECT_EQ(absl::ToUnixMicros(
                google::fhir::GetTimeFromTimelikeElement(time_not_set)),
            0L);
}

TEST(GetTimeFromTimelikeElement, Date) {
  Date with_time;
  with_time.set_value_us(1517166000000000L);  // 2018-01-28 19:00:00
  Date time_not_set;

  EXPECT_EQ(
      absl::ToUnixMicros(google::fhir::GetTimeFromTimelikeElement(with_time)),
      1517166000000000L);
  EXPECT_EQ(absl::ToUnixMicros(
                google::fhir::GetTimeFromTimelikeElement(time_not_set)),
            0L);
}

TEST(GetTimezone, TimeZoneStr) {
  absl::TimeZone tz;

  // Valid cases.
  ASSERT_TRUE(GetTimezone("Z", &tz).ok());
  ASSERT_EQ(tz.name(), "UTC");
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

TEST(BuildTimeZoneFromString, ValidAndInvalidInputs) {
  // Valid cases.
  // "Z" and "UTC" are valid specifiers of the UTC time zone.
  ASSERT_EQ(BuildTimeZoneFromString("Z").ValueOrDie().name(), "UTC");
  ASSERT_EQ(BuildTimeZoneFromString("UTC").ValueOrDie().name(), "UTC");
  // {+,-}HH:MM specifies a UTC offset of up to 14 h.
  ASSERT_EQ(BuildTimeZoneFromString("-06:00").ValueOrDie().name(),
            "Fixed/UTC-06:00:00");
  ASSERT_EQ(BuildTimeZoneFromString("+06:00").ValueOrDie().name(),
            "Fixed/UTC+06:00:00");
  ASSERT_EQ(BuildTimeZoneFromString("-06:30").ValueOrDie().name(),
            "Fixed/UTC-06:30:00");
  ASSERT_EQ(BuildTimeZoneFromString("-14:00").ValueOrDie().name(),
            "Fixed/UTC-14:00:00");
  ASSERT_EQ(BuildTimeZoneFromString("+14:00").ValueOrDie().name(),
            "Fixed/UTC+14:00:00");
  // tz dataase time zone names are also accepted.
  ASSERT_EQ(BuildTimeZoneFromString("America/New_York").ValueOrDie().name(),
            "America/New_York");

  // Error cases.
  // Time offset string must be signed.
  ASSERT_FALSE(BuildTimeZoneFromString("06:30").ok());
  // Offsets must be within +/- 14h from UTC.
  ASSERT_FALSE(BuildTimeZoneFromString("-15:30").ok());
  // Minute field must be 00..59.
  ASSERT_FALSE(BuildTimeZoneFromString("-12:60").ok());
  // Invalid string that formatting-wise looks like it could have been a
  // tz database time zone name.
  ASSERT_FALSE(BuildTimeZoneFromString("America/Not_A_Place").ok());
}

TEST(WrapContainedResource, Valid) {
  Encounter encounter;
  encounter.mutable_id()->set_value("47");

  ContainedResource expected;
  *(expected.mutable_encounter()) = encounter;
  auto result = WrapContainedResource<ContainedResource>(encounter);
  ASSERT_TRUE(result.status().ok());
  EXPECT_THAT(result.ValueOrDie(), EqualsProto(expected));
}

TEST(WrapContainedResource, InvalidType) {
  DateTime datetime;
  EXPECT_EQ(WrapContainedResource<ContainedResource>(datetime).status(),
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

TEST(GetPatient, StatusOr) {
  Bundle bundle;
  bundle.add_entry()
      ->mutable_resource()
      ->mutable_patient()
      ->mutable_id()
      ->set_value("5");

  EXPECT_EQ(GetPatient(bundle).ValueOrDie()->id().value(), "5");
}

}  // namespace
}  // namespace fhir
}  // namespace google
