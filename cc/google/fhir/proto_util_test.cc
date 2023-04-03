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

#include "google/fhir/proto_util.h"

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/stu3/datatypes.pb.h"
#include "proto/google/fhir/proto/stu3/resources.pb.h"

namespace google {
namespace fhir {

namespace {
using ::google::fhir::stu3::proto::DateTime;
using ::google::fhir::stu3::proto::Encounter;
using ::google::fhir::stu3::proto::Id;
using ::google::fhir::stu3::proto::MedicationRequest;
using ::google::fhir::stu3::proto::Observation;
using ::google::fhir::testutil::EqualsProto;
using ::google::protobuf::Message;

// TODO(b/176912972): These util tests should not be FHIR-specific.

Encounter MakeTestEncounter() {
  Encounter encounter;
  google::protobuf::TextFormat::ParseFromString(R"pb(
                                        id { value: "5446" }
                                        location {
                                          period {
                                            start { value_us: 5 }
                                            end { value_us: 6 }
                                          }
                                        }
                                        location {
                                          period {
                                            start { value_us: 7 }
                                            end { value_us: 8 }
                                          }
                                        }
                                        location {
                                          period {
                                            start { value_us: 9 }
                                            end { value_us: 10 }
                                          }
                                        }
                                      )pb",
                                      &encounter);
  return encounter;
}

TEST(ForEachMessageWithStatus, Ok) {
  const Message& encounter = MakeTestEncounter();
  const google::protobuf::FieldDescriptor* field =
      encounter.GetDescriptor()->FindFieldByName("location");

  auto status = ForEachMessageWithStatus<Encounter::Location>(
      encounter, field,
      [](const Encounter::Location& location) { return absl::OkStatus(); });

  ASSERT_TRUE(status.ok());
}

TEST(ForEachMessageWithStatus, Fail) {
  const Message& encounter = MakeTestEncounter();
  const google::protobuf::FieldDescriptor* field =
      encounter.GetDescriptor()->FindFieldByName("location");

  auto status = ForEachMessageWithStatus<Encounter::Location>(
      encounter, field, [](const Encounter::Location& location) {
        return location.period().start().value_us() == 7
                   ? absl::InvalidArgumentError("it's 7")
                   : absl::OkStatus();
      });

  ASSERT_TRUE(!status.ok());
  ASSERT_EQ(status.message(), "it's 7");
}

TEST(GetSubmessageByPath, Valid) {
  MedicationRequest request;
  ASSERT_TRUE(
      google::protobuf::TextFormat::ParseFromString(R"pb(
                                            dispense_request: {
                                              validity_period: {
                                                start: {
                                                  value_us: 1275350400000000
                                                  timezone: "America/New_York"
                                                  precision: 3
                                                }
                                                end: {
                                                  value_us: 1275436800000000
                                                  timezone: "America/New_York"
                                                  precision: 3
                                                }
                                              }
                                            })pb",
                                          &request));

  DateTime expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"pb(
                                                    value_us: 1275350400000000
                                                    timezone: "America/New_York"
                                                    precision: 3
                                                  )pb",
                                                  &expected));

  auto result = google::fhir::GetSubmessageByPathAndCheckType<DateTime>(
      request, "MedicationRequest.dispenseRequest.validityPeriod.start");
  ASSERT_TRUE(result.ok());
  ASSERT_THAT(*result.value(), EqualsProto(expected));
}

TEST(GetSubmessageByPath, NotFound) {
  const MedicationRequest request;
  auto result = google::fhir::GetSubmessageByPathAndCheckType<DateTime>(
      request, "MedicationRequest.dispenseRequest.validityPeriod.start");
  ASSERT_EQ(result.status().code(), ::absl::StatusCode::kNotFound);
}

TEST(GetMutableSubmessageByPath, NotFoundIsOk) {
  MedicationRequest request;
  DateTime expected;
  auto result = google::fhir::GetMutableSubmessageByPathAndCheckType<DateTime>(
      &request, "MedicationRequest.dispenseRequest.validityPeriod.start");
  ASSERT_TRUE(result.ok());
  ASSERT_THAT(*result.value(), EqualsProto(expected));
}

TEST(GetSubmessageByPath, BadPath) {
  MedicationRequest request;
  ASSERT_TRUE(
      google::protobuf::TextFormat::ParseFromString(
          R"pb(
            dispense_request: {
              validity_period: { start: { value_us: 12753 } }
            })pb",
          &request));

  auto result = google::fhir::GetSubmessageByPathAndCheckType<DateTime>(
      request, "MedicationRequest.garbageField.validityPeriod.start");
  ASSERT_EQ(result.status().code(), ::absl::StatusCode::kInvalidArgument);
}

TEST(GetSubmessageByPath, WrongRequestedType) {
  MedicationRequest request;
  ASSERT_TRUE(
      google::protobuf::TextFormat::ParseFromString(
          R"pb(
            dispense_request: {
              validity_period: { start: { value_us: 12753 } }
            })pb",
          &request));

  auto result = google::fhir::GetSubmessageByPathAndCheckType<Observation>(
      request, "MedicationRequest.dispenseRequest.validityPeriod.start");
  ASSERT_EQ(result.status().code(), ::absl::StatusCode::kInvalidArgument);
}

TEST(GetSubmessageByPath, WrongResourceType) {
  MedicationRequest request;
  ASSERT_TRUE(
      google::protobuf::TextFormat::ParseFromString(
          R"pb(
            dispense_request: {
              validity_period: { start: { value_us: 12753 } }
            })pb",
          &request));
  auto result = google::fhir::GetSubmessageByPathAndCheckType<DateTime>(
      request, "Encounter.dispenseRequest.validityPeriod.start");
  ASSERT_EQ(result.status().code(), ::absl::StatusCode::kInvalidArgument);
}

TEST(GetSubmessageByPath, HasIndex) {
  const Encounter encounter = MakeTestEncounter();
  DateTime expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"pb(
                                                    value_us: 7
                                                  )pb",
                                                  &expected));
  auto result = google::fhir::GetSubmessageByPathAndCheckType<DateTime>(
      encounter, "Encounter.location[1].period.start");
  ASSERT_TRUE(result.ok());
  ASSERT_THAT(*result.value(), EqualsProto(expected));
}

TEST(GetSubmessageByPath, EndsInIndex) {
  const Encounter encounter = MakeTestEncounter();
  Encounter::Location expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"pb(
                                                    period {
                                                      start { value_us: 7 }
                                                      end { value_us: 8 }
                                                    }
                                                  )pb",
                                                  &expected));
  auto result =
      google::fhir::GetSubmessageByPathAndCheckType<Encounter::Location>(
          encounter, "Encounter.location[1]");
  ASSERT_TRUE(result.ok());
  ASSERT_THAT(*result.value(), EqualsProto(expected));
}

TEST(GetSubmessageByPath, UnindexedRepeatedAtEnd) {
  const Encounter encounter = MakeTestEncounter();
  auto result = google::fhir::GetSubmessageByPathAndCheckType<DateTime>(
      encounter, "Encounter.location");
  ASSERT_EQ(result.status().code(), ::absl::StatusCode::kInvalidArgument);
}

TEST(GetSubmessageByPath, UnindexedRepeatedInMiddle) {
  const Encounter encounter = MakeTestEncounter();
  auto result = google::fhir::GetSubmessageByPathAndCheckType<DateTime>(
      encounter, "Encounter.location.period");
  ASSERT_EQ(result.status().code(), ::absl::StatusCode::kInvalidArgument);
}

TEST(GetSubmessageByPath, Untemplatized) {
  MedicationRequest request;
  ASSERT_TRUE(
      google::protobuf::TextFormat::ParseFromString(R"pb(
                                            dispense_request: {
                                              validity_period: {
                                                start: {
                                                  value_us: 1275350400000000
                                                  timezone: "America/New_York"
                                                  precision: 3
                                                }
                                                end: {
                                                  value_us: 1275436800000000
                                                  timezone: "America/New_York"
                                                  precision: 3
                                                }
                                              }
                                            })pb",
                                          &request));

  DateTime expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"pb(
                                                    value_us: 1275350400000000
                                                    timezone: "America/New_York"
                                                    precision: 3
                                                  )pb",
                                                  &expected));

  const google::protobuf::Message& request_as_message = request;

  auto result = GetSubmessageByPath(
      request_as_message,
      "MedicationRequest.dispenseRequest.validityPeriod.start");
  ASSERT_TRUE(result.ok());
  ASSERT_THAT(*result.value(), EqualsProto(expected));
}

TEST(ClearFieldByPath, SingularPresent) {
  Encounter encounter = MakeTestEncounter();
  Encounter expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        id { value: "5446" }
        location {
          period {
            start { value_us: 5 }
            end { value_us: 6 }
          }
        }
        location { period { start { value_us: 7 } } }
        location {
          period {
            start { value_us: 9 }
            end { value_us: 10 }
          }
        }
      )pb",
      &expected));
  ASSERT_TRUE(
      ClearFieldByPath(&encounter, "Encounter.location[1].period.end").ok());
  ASSERT_THAT(encounter, EqualsProto(expected));
}

TEST(ClearFieldByPath, SingularAbsent) {
  Encounter encounter = MakeTestEncounter();
  Encounter expected = MakeTestEncounter();
  ASSERT_TRUE(ClearFieldByPath(&encounter, "Encounter.period").ok());
  ASSERT_THAT(encounter, EqualsProto(expected));
}

TEST(ClearFieldByPath, SingularInvalid) {
  Encounter encounter = MakeTestEncounter();
  const auto& status = ClearFieldByPath(&encounter, "Encounter.garbage");
  ASSERT_EQ(status.code(), ::absl::StatusCode::kInvalidArgument);
}

TEST(ClearFieldByPath, RepeatedUnindexedPresent) {
  Encounter encounter = MakeTestEncounter();
  Encounter expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"pb(
                                                    id { value: "5446" }
                                                  )pb",
                                                  &expected));
  ASSERT_TRUE(ClearFieldByPath(&encounter, "Encounter.location").ok());
  ASSERT_THAT(encounter, EqualsProto(expected));
}

TEST(ClearFieldByPath, RepeatedUnindexedAbsent) {
  Encounter encounter;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"pb(
                                                    id { value: "5446" }
                                                  )pb",
                                                  &encounter));
  Encounter expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"pb(
                                                    id { value: "5446" }
                                                  )pb",
                                                  &expected));
  ASSERT_TRUE(ClearFieldByPath(&encounter, "Encounter.location").ok());
  ASSERT_THAT(encounter, EqualsProto(expected));
}

TEST(ClearFieldByPath, RepeatedIndexedPresentFails) {
  Encounter encounter = MakeTestEncounter();
  ASSERT_EQ(ClearFieldByPath(&encounter, "Encounter.location[1]").code(),
            ::absl::StatusCode::kInvalidArgument);
}

TEST(HasSubmessageByPath, SingularPresent) {
  Encounter encounter = MakeTestEncounter();
  auto got =
      HasSubmessageByPath(encounter, "Encounter.location[1].period.start");
  ASSERT_TRUE(got.ok());
  ASSERT_TRUE(got.value());
}

TEST(HasSubmessageByPath, SingularAbsent) {
  Encounter encounter = MakeTestEncounter();
  auto got = HasSubmessageByPath(encounter, "Encounter.location[1].period.id");
  ASSERT_TRUE(got.ok());
  ASSERT_FALSE(got.value());
}

TEST(HasSubmessageByPath, SingularInvalid) {
  Encounter encounter = MakeTestEncounter();
  ASSERT_EQ(HasSubmessageByPath(encounter, "Encounter.location[1].sandwich")
                .status()
                .code(),
            ::absl::StatusCode::kInvalidArgument);
}

TEST(HasSubmessageByPath, RepeatedIndexedPresent) {
  Encounter encounter = MakeTestEncounter();
  auto got = HasSubmessageByPath(encounter, "Encounter.location[1]");
  ASSERT_TRUE(got.ok());
  ASSERT_TRUE(got.value());
}

TEST(HasSubmessageByPath, RepeatedIndexedAbsent) {
  Encounter encounter = MakeTestEncounter();
  auto got = HasSubmessageByPath(encounter, "Encounter.location[10]");
  ASSERT_TRUE(got.ok());
  ASSERT_FALSE(got.value());
}

TEST(HasSubmessageByPath, RepeatedIndexedInvalid) {
  Encounter encounter = MakeTestEncounter();
  ASSERT_EQ(
      HasSubmessageByPath(encounter, "Encounter.sandwich[1]").status().code(),
      ::absl::StatusCode::kInvalidArgument);
}

TEST(HasSubmessageByPath, RepeatedUnindexed) {
  Encounter encounter = MakeTestEncounter();
  ASSERT_EQ(
      HasSubmessageByPath(encounter, "Encounter.location").status().code(),
      ::absl::StatusCode::kInvalidArgument);
}

TEST(EndsInIndex, True) {
  ASSERT_TRUE(EndsInIndex("A.b.c[5]"));

  int index;
  ASSERT_TRUE(EndsInIndex("A.b.c[5]", &index));
  ASSERT_EQ(5, index);
}

TEST(EndsInIndex, False) {
  ASSERT_FALSE(EndsInIndex("A.b.c"));

  int index;
  ASSERT_FALSE(EndsInIndex("A.b.c", &index));
}

TEST(StripIndex, Present) { ASSERT_EQ("A.b.c", StripIndex("A.b.c[5]")); }

TEST(StripIndex, Absent) { ASSERT_EQ("A.b.c", StripIndex("A.b.c")); }

TEST(ProtoUtilTest, GetMessageInField) {
  const Encounter encounter = MakeTestEncounter();
  ASSERT_THAT(GetMessageInField(encounter, "id").value(),
              EqualsProto(encounter.id()));
}

TEST(ProtoUtilTest, GetMessageInField_MissingField) {
  const Encounter encounter = MakeTestEncounter();
  ASSERT_EQ(GetMessageInField(encounter, "garbage").status().code(),
            absl::StatusCode::kInvalidArgument);
}

TEST(ProtoUtilTest, GetMessageInField_WrongType) {
  const Encounter encounter = MakeTestEncounter();
  ASSERT_EQ(GetMessageInField(encounter.id(), "value").status().code(),
            absl::StatusCode::kInvalidArgument);
}

TEST(ProtoUtilTest, MutableMessageInField) {
  Encounter encounter = MakeTestEncounter();
  ASSERT_THAT(MutableMessageInField(&encounter, "id").value(),
              EqualsProto(encounter.id()));
}

TEST(ProtoUtilTest, MutableMessageInField_MissingField) {
  Encounter encounter = MakeTestEncounter();
  ASSERT_EQ(MutableMessageInField(&encounter, "garbage").status().code(),
            absl::StatusCode::kInvalidArgument);
}

TEST(ProtoUtilTest, MutableMessageInField_WrongType) {
  Encounter encounter = MakeTestEncounter();
  ASSERT_EQ(
      MutableMessageInField(encounter.mutable_id(), "value").status().code(),
      absl::StatusCode::kInvalidArgument);
}

TEST(ProtoUtilTest, TypedMutableMessageInFieldSucceeds) {
  Encounter encounter = MakeTestEncounter();
  ASSERT_THAT(MutableMessageInField<Id>(&encounter, "id").value(),
              EqualsProto(encounter.id()));
}

TEST(ProtoUtilTest, TypedMutableMessageInFieldWrongTypeReturnsStatus) {
  Encounter encounter = MakeTestEncounter();
  ASSERT_EQ(MutableMessageInField<DateTime>(&encounter, "id").status().code(),
            absl::StatusCode::kInvalidArgument);
}

TEST(ProtoUtilTest, TypedMutableMessageInFieldMissingMessageReturnsStatus) {
  Encounter encounter = MakeTestEncounter();
  ASSERT_EQ(
      MutableMessageInField<DateTime>(&encounter, "garbage").status().code(),
      absl::StatusCode::kInvalidArgument);
}

TEST(ProtoUtilTest, TypedMutableMessageInFieldPrimitiveReturnsStatus) {
  Encounter encounter = MakeTestEncounter();
  ASSERT_EQ(MutableMessageInField<DateTime>(encounter.mutable_id(), "value")
                .status()
                .code(),
            absl::StatusCode::kInvalidArgument);
}

}  // namespace
}  // namespace fhir
}  // namespace google
