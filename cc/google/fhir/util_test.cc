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

#include <string>

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
#include "proto/google/fhir/proto/r4/core/resources/patient.pb.h"
#include "proto/google/fhir/proto/stu3/codes.pb.h"
#include "proto/google/fhir/proto/stu3/datatypes.pb.h"
#include "proto/google/fhir/proto/stu3/resources.pb.h"
#include "testdata/r4/profiles/test.pb.h"
#include "google/protobuf/descriptor.h"

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
  FHIR_ASSERT_OK(result.status());
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
  FHIR_ASSERT_OK(SetContainedResource(encounter, &result));
  EXPECT_THAT(result, EqualsProto(expected));
}

TEST(SetContainedResource, InvalidType) {
  DateTime datetime;
  ContainedResource result;
  EXPECT_EQ(SetContainedResource(datetime, &result),
            ::absl::InvalidArgumentError("Resource type DateTime not found in "
                                         "fhir::Bundle::Entry::resource"));
}

TEST(IsResourceFromContainedResourceType, Stu3EncounterStu3ContainedResource) {
  Encounter encounter;
  const auto& statusOrValue =
      IsResourceFromContainedResourceType<ContainedResource>(
          encounter.GetDescriptor());
  EXPECT_TRUE(statusOrValue.ok());
  EXPECT_EQ(statusOrValue.value(), true);
}

TEST(IsResourceFromContainedResourceType, R4MedicationR4ContainedResource) {
  r4::core::Medication medication;
  const auto& statusOrValue =
      IsResourceFromContainedResourceType<r4::core::ContainedResource>(
          medication.GetDescriptor());
  EXPECT_TRUE(statusOrValue.ok());
  EXPECT_EQ(statusOrValue.value(), true);
}

TEST(IsResourceFromContainedResourceType, Stu3EncounterR4ContainedResource) {
  Encounter encounter;
  const auto& statusOrValue =
      IsResourceFromContainedResourceType<r4::core::ContainedResource>(
          encounter.GetDescriptor());
  EXPECT_TRUE(statusOrValue.ok());
  EXPECT_EQ(statusOrValue.value(), false);
}

TEST(IsResourceFromContainedResourceType, R4MedicationStu3ContainedResource) {
  r4::core::Medication medication;

  const auto& statusOrValue =
      IsResourceFromContainedResourceType<ContainedResource>(
          medication.GetDescriptor());
  EXPECT_TRUE(statusOrValue.ok());
  EXPECT_EQ(statusOrValue.value(), false);
}

TEST(IsResourceFromContainedResourceType, TestPatientTestContainedResource) {
  r4::testing::TestPatient patient;
  const auto& statusOrValue =
      IsResourceFromContainedResourceType<r4::testing::ContainedResource>(
          patient.GetDescriptor());
  EXPECT_TRUE(statusOrValue.ok());
  EXPECT_EQ(statusOrValue.value(), true);
}

TEST(IsResourceFromContainedResourceType, CorePatientTestContainedResource) {
  r4::core::Patient patient;
  const auto& statusOrValue =
      IsResourceFromContainedResourceType<r4::testing::ContainedResource>(
          patient.GetDescriptor());
  EXPECT_TRUE(statusOrValue.ok());
  EXPECT_EQ(statusOrValue.value(), false);
}

TEST(IsResourceFromContainedResourceType, TestPatientCoreContainedResource) {
  r4::testing::TestPatient patient;
  const auto& statusOrValue =
      IsResourceFromContainedResourceType<ContainedResource>(
          patient.GetDescriptor());
  EXPECT_TRUE(statusOrValue.ok());
  EXPECT_EQ(statusOrValue.value(), false);
}

TEST(IsResourceFromContainedResourceType, TemplateNotContainedResource) {
  r4::testing::TestPatient patient;
  EXPECT_EQ(IsResourceFromContainedResourceType<r4::testing::TestPatient>(
                patient.GetDescriptor())
                .status(),
            ::absl::NotFoundError(
                "Template type 'google.fhir.r4.testing.TestPatient' is not a "
                "contained resource."));
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

  ASSERT_THAT(**result_statusor, EqualsProto(contained_medication));
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

  ASSERT_THAT(**result_statusor, EqualsProto(contained_encounter));
}

TEST(GetResourceDescriptorByName, Success) {
  absl::StatusOr<const Descriptor*> result = GetResourceDescriptorByName(
      r4::testing::ContainedResource::descriptor(), "TestPatient");
  FHIR_ASSERT_OK(result.status());
  EXPECT_EQ(*result, r4::testing::TestPatient::descriptor());
}

TEST(GetResourceDescriptorByName, InvalidArgument) {
  absl::StatusOr<const Descriptor*> result = GetResourceDescriptorByName(
      r4::core::Boolean::descriptor(), "TestPatient");
  ASSERT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), absl::StatusCode::kInvalidArgument);
}

TEST(GetResourceDescriptorByName, NotFound) {
  absl::StatusOr<const Descriptor*> result = GetResourceDescriptorByName(
      r4::testing::ContainedResource::descriptor(), "Elephant");
  ASSERT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), absl::StatusCode::kNotFound);
}

TEST(GetContainedResource, UnprofiledSucceeds) {
  r4::core::ContainedResource container;
  r4::core::Patient patient;
  patient.mutable_id()->set_value("foo");
  *container.mutable_patient() = patient;

  absl::StatusOr<const Message*> resource = GetContainedResource(container);
  FHIR_ASSERT_OK(resource.status());
  EXPECT_THAT(*resource, EqualsProto(patient));
}

TEST(MutableContainedResource, UnprofiledSucceeds) {
  r4::core::ContainedResource container;
  r4::core::Patient patient;
  patient.mutable_id()->set_value("foo");
  *container.mutable_patient() = patient;

  absl::StatusOr<Message*> resource = MutableContainedResource(&container);
  FHIR_ASSERT_OK(resource.status());
  EXPECT_THAT(*resource, EqualsProto(patient));
}

TEST(GetContainedResource, ProfiledSucceeds) {
  r4::testing::ContainedResource container;
  r4::testing::TestPatient patient;
  patient.mutable_id()->set_value("foo");
  *container.mutable_test_patient() = patient;

  absl::StatusOr<const Message*> resource = GetContainedResource(container);
  FHIR_ASSERT_OK(resource.status());
  EXPECT_THAT(*resource, EqualsProto(patient));
}

TEST(MutableContainedResource, ProfiledSucceeds) {
  r4::testing::ContainedResource container;
  r4::testing::TestPatient patient;
  patient.mutable_id()->set_value("foo");
  *container.mutable_test_patient() = patient;

  absl::StatusOr<Message*> resource = MutableContainedResource(&container);
  FHIR_ASSERT_OK(resource.status());
  EXPECT_THAT(*resource, EqualsProto(patient));
}

TEST(GetContainedResource, NonContainedResourceFailsWithInvalidArgument) {
  r4::testing::TestPatient patient;
  patient.mutable_id()->set_value("foo");

  absl::StatusOr<const Message*> resource = GetContainedResource(patient);
  EXPECT_THAT(resource.status().code(), absl::StatusCode::kInvalidArgument);
}

TEST(MutableContainedResource, NonContainedResourceFailsWithInvalidArgument) {
  r4::testing::TestPatient patient;
  patient.mutable_id()->set_value("foo");

  absl::StatusOr<Message*> resource = MutableContainedResource(&patient);
  EXPECT_THAT(resource.status().code(), absl::StatusCode::kInvalidArgument);
}

TEST(GetContainedResource, EmptyFailsWithNotFound) {
  r4::testing::ContainedResource container;
  absl::StatusOr<const Message*> resource = GetContainedResource(container);
  EXPECT_THAT(resource.status().code(), absl::StatusCode::kNotFound);
}

TEST(MutableContainedResource, EmptyFailsWithNotFound) {
  r4::testing::ContainedResource container;
  absl::StatusOr<Message*> resource = MutableContainedResource(&container);
  EXPECT_THAT(resource.status().code(), absl::StatusCode::kNotFound);
}

}  // namespace
}  // namespace fhir
}  // namespace google
