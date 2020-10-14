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

#include "google/protobuf/descriptor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "google/fhir/status/status.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medication.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medication_request.pb.h"
#include "proto/google/fhir/proto/stu3/codes.pb.h"
#include "proto/google/fhir/proto/stu3/datatypes.pb.h"
#include "proto/google/fhir/proto/stu3/resources.pb.h"
#include "testdata/r4/profiles/test.pb.h"

namespace google {
namespace fhir {

namespace {

using ::google::fhir::stu3::proto::Bundle;
using ::google::fhir::stu3::proto::ContainedResource;
using ::google::fhir::stu3::proto::Date;
using ::google::fhir::stu3::proto::DateTime;
using ::google::fhir::stu3::proto::Decimal;
using ::google::fhir::stu3::proto::Encounter;
using ::google::fhir::testutil::EqualsProto;

using ::google::fhir::stu3::proto::AllergyIntolerance;
using ::google::fhir::stu3::proto::EncounterStatusCode;
using ::google::fhir::stu3::proto::Patient;

using ::google::protobuf::Descriptor;
using ::google::protobuf::Message;

TEST(GetResourceFromBundleEntryTest, GetResourceFromEncounter) {
  Bundle::Entry entry;
  Encounter* encounter = entry.mutable_resource()->mutable_encounter();
  encounter->mutable_status()->set_value(EncounterStatusCode::FINISHED);
  encounter->mutable_id()->set_value("2468");

  const google::protobuf::Message* resource = GetResourceFromBundleEntry(entry).value();
  auto* desc = resource->GetDescriptor();
  EXPECT_EQ("Encounter", desc->name());
  const Encounter* result_encounter = dynamic_cast<const Encounter*>(resource);
  std::string resource_id;

  EXPECT_EQ(EncounterStatusCode::FINISHED, result_encounter->status().value());
  EXPECT_EQ("2468", result_encounter->id().value());
}

TEST(GetResourceFromBundleEntryTest, GetResourceFromPatient) {
  Bundle::Entry entry;
  Patient* patient = entry.mutable_resource()->mutable_patient();
  patient->mutable_birth_date()->set_value_us(-77641200000000);
  patient->mutable_id()->set_value("2468");

  const google::protobuf::Message* resource = GetResourceFromBundleEntry(entry).value();
  auto* desc = resource->GetDescriptor();
  EXPECT_EQ("Patient", desc->name());
  const Patient* result_patient = dynamic_cast<const Patient*>(resource);

  EXPECT_EQ(-77641200000000, result_patient->birth_date().value_us());
  EXPECT_EQ("2468", result_patient->id().value());
}

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

TEST(BuildTimeZoneFromString, ValidAndInvalidInputs) {
  // Valid cases.
  // "Z" and "UTC" are valid specifiers of the UTC time zone.
  ASSERT_EQ(BuildTimeZoneFromString("Z").value().name(), "UTC");
  ASSERT_EQ(BuildTimeZoneFromString("UTC").value().name(), "UTC");
  // {+,-}HH:MM specifies a UTC offset of up to 14 h.
  ASSERT_EQ(BuildTimeZoneFromString("-06:00").value().name(),
            "Fixed/UTC-06:00:00");
  ASSERT_EQ(BuildTimeZoneFromString("+06:00").value().name(),
            "Fixed/UTC+06:00:00");
  ASSERT_EQ(BuildTimeZoneFromString("-06:30").value().name(),
            "Fixed/UTC-06:30:00");
  ASSERT_EQ(BuildTimeZoneFromString("-14:00").value().name(),
            "Fixed/UTC-14:00:00");
  ASSERT_EQ(BuildTimeZoneFromString("+14:00").value().name(),
            "Fixed/UTC+14:00:00");
  // tz dataase time zone names are also accepted.
  ASSERT_EQ(BuildTimeZoneFromString("America/New_York").value().name(),
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
  EXPECT_THAT(result.value(), EqualsProto(expected));
}

TEST(WrapContainedResource, InvalidType) {
  DateTime datetime;
  EXPECT_EQ(WrapContainedResource<ContainedResource>(datetime).status(),
            ::absl::InvalidArgumentError("Resource type DateTime not found in "
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
            ::absl::InvalidArgumentError("Resource type DateTime not found in "
                                         "fhir::Bundle::Entry::resource"));
}

TEST(GetPatient, StatusOr) {
  Bundle bundle;
  bundle.add_entry()
      ->mutable_resource()
      ->mutable_patient()
      ->mutable_id()
      ->set_value("5");

  EXPECT_EQ(GetPatient(bundle).value()->id().value(), "5");
}

TEST(GetTypedContainedResource, Valid) {
  ContainedResource contained;
  contained.mutable_allergy_intolerance()->mutable_id()->set_value("47");

  ASSERT_EQ("47", google::fhir::GetTypedContainedResource<AllergyIntolerance>(
                      contained)
                      .value()
                      ->id()
                      .value());
}

TEST(GetTypedContainedResource, WrongType) {
  ContainedResource contained;
  contained.mutable_allergy_intolerance()->mutable_id()->set_value("47");

  EXPECT_EQ(google::fhir::GetTypedContainedResource<Patient>(contained)
                .status()
                .code(),
            ::absl::StatusCode::kNotFound);
}

TEST(GetTypedContainedResource, BadType) {
  ContainedResource contained;
  contained.mutable_allergy_intolerance()->mutable_id()->set_value("47");

  EXPECT_EQ(
      google::fhir::GetTypedContainedResource<DateTime>(contained).status(),
      ::absl::InvalidArgumentError(
          "No resource field found for type DateTime"));
}

TEST(GetDecimalValue, NormalValue) {
  Decimal decimal;
  decimal.set_value("1.2345e3");

  EXPECT_EQ(GetDecimalValue(decimal).value(), 1234.5);
}

TEST(GetDecimalValue, NotANumber) {
  Decimal decimal;
  decimal.set_value("NaN");

  EXPECT_EQ(GetDecimalValue(decimal).status(),
            ::absl::InvalidArgumentError("Invalid decimal: 'NaN'"));
}

TEST(GetDecimalValue, PositiveInfinity) {
  Decimal decimal;
  decimal.set_value("1e+1000");

  EXPECT_EQ(GetDecimalValue(decimal).status(),
            ::absl::InvalidArgumentError("Invalid decimal: '1e+1000'"));
}

TEST(GetDecimalValue, NegativeInfinity) {
  Decimal decimal;
  decimal.set_value("-1e+1000");

  EXPECT_EQ(GetDecimalValue(decimal).status(),
            ::absl::InvalidArgumentError("Invalid decimal: '-1e+1000'"));
}

TEST(GetMutablePatient, StatusOr) {
  Bundle bundle;
  bundle.add_entry()
      ->mutable_resource()
      ->mutable_patient()
      ->mutable_id()
      ->set_value("5");

  EXPECT_EQ(GetMutablePatient(&bundle).value()->mutable_id()->value(), "5");
}

TEST(Util, UnpackAnyAsContainedResourceR4) {
  r4::core::MedicationRequest med_req = PARSE_VALID_FHIR_PROTO(
      R"proto(
        medication { reference { medication_id { value: "med" } } }
        status { value: ACTIVE }
        intent { value: ORDER }
        id { value: "1" }
        subject { patient_id { value: "14" } }
        authored_on {
          value_us: 1421240400000000
          timezone: "Australia/Sydney"
          precision: DAY
        }
      )proto");

  const r4::core::ContainedResource contained_medication =
      PARSE_VALID_FHIR_PROTO(
          R"proto(
            medication {
              id { value: "med" }
              code {
                coding {
                  system { value: "http://hl7.org/fhir/sid/ndc" }
                  code { value: "123" }
                }
              }
            }
          )proto");

  med_req.add_contained()->PackFrom(contained_medication);

  auto result_statusor = UnpackAnyAsContainedResource(med_req.contained(0));
  FHIR_ASSERT_OK(result_statusor.status());

  std::unique_ptr<Message> result = absl::WrapUnique(result_statusor.value());

  ASSERT_THAT(*result, EqualsProto(contained_medication));
}

TEST(Util, UnpackAnyAsContainedResourceR4WrongAny) {
  r4::core::Boolean boolean;
  r4::core::MedicationRequest med_req;
  med_req.add_contained()->PackFrom(boolean);
  auto result_status =
      UnpackAnyAsContainedResource(med_req.contained(0)).status();
  ASSERT_EQ(result_status.message(),
            "google.protobuf.Any messages must store a ContainedResource. Got "
            "\"google.fhir.r4.core.Boolean\".");
}

TEST(Util, UnpackAnyAsContainedResourceR4CustomFactory) {
  r4::core::MedicationRequest med_req = PARSE_VALID_FHIR_PROTO(
      R"proto(
        medication { reference { medication_id { value: "med" } } }
        status { value: ACTIVE }
        intent { value: ORDER }
        id { value: "1" }
        subject { patient_id { value: "14" } }
        authored_on {
          value_us: 1421240400000000
          timezone: "Australia/Sydney"
          precision: DAY
        }
      )proto");

  const r4::core::ContainedResource contained_medication =
      PARSE_VALID_FHIR_PROTO(
          R"proto(
            medication {
              id { value: "med" }
              code {
                coding {
                  system { value: "http://hl7.org/fhir/sid/ndc" }
                  code { value: "123" }
                }
              }
            }
          )proto");

  med_req.add_contained()->PackFrom(contained_medication);

  // Use a message factory that just returns a custom instance
  auto local_memory = absl::make_unique<r4::core::ContainedResource>();
  auto result_statusor = UnpackAnyAsContainedResource(
      med_req.contained(0),
      [&](const Descriptor* descriptor) -> absl::StatusOr<Message*> {
        return local_memory.get();
      });

  FHIR_ASSERT_OK(result_statusor.status());
  Message* result = result_statusor.value();

  // Compare memory addresses
  ASSERT_EQ(result, local_memory.get());
}

TEST(Util, UnpackAnyAsContainedResourceR4NonStandardIG) {
  r4::testing::TestObservation med_req = PARSE_VALID_FHIR_PROTO(
      R"proto(
        status { value: FINAL }
        code { sys_a { code { value: "blah" } } }
      )proto");

  r4::testing::ContainedResource contained_encounter = PARSE_VALID_FHIR_PROTO(
      R"proto(
        test_encounter {
          status { value: FINISHED }
          class_value {
            system { value: "foo" }
            code { value: "bar" }
          }
          priority { act { code { value: EM } } }
        }
      )proto");

  med_req.add_contained()->PackFrom(contained_encounter);

  auto result_statusor = UnpackAnyAsContainedResource(med_req.contained(0));
  FHIR_ASSERT_OK(result_statusor.status());

  std::unique_ptr<Message> result = absl::WrapUnique(result_statusor.value());

  ASSERT_THAT(*result, EqualsProto(contained_encounter));
}

}  // namespace
}  // namespace fhir
}  // namespace google
