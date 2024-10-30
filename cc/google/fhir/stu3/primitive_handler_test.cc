/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "google/fhir/stu3/primitive_handler.h"

#include <string>
#include <vector>

#include "google/protobuf/any.pb.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "google/fhir/annotations.h"
#include "google/fhir/json_format.h"
#include "google/fhir/stu3/json_format.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/stu3/codes.pb.h"
#include "proto/google/fhir/proto/stu3/datatypes.pb.h"
#include "proto/google/fhir/proto/stu3/fhirproto_extensions.pb.h"
#include "proto/google/fhir/proto/stu3/resources.pb.h"
#include "google/protobuf/message.h"

namespace google {
namespace fhir {
namespace stu3 {

namespace {

using ::google::fhir::testutil::EqualsProto;
using ::testing::Pointee;
using namespace ::google::fhir::stu3::proto;  // NOLINT

static const char* const kTimeZoneString = "Australia/Sydney";
static const absl::TimeZone GetTimeZone() {
  absl::TimeZone tz;
  absl::LoadTimeZone(kTimeZoneString, &tz);
  return tz;
}

static const absl::TimeZone kTimeZone = GetTimeZone();

std::string CamelCaseToLowerUnderscores(absl::string_view src) {
  std::string dst;
  for (auto iter = src.begin(); iter != src.end(); ++iter) {
    if (absl::ascii_isupper(*iter) && iter != src.begin()) {
      dst.push_back('_');
    }
    dst.push_back(absl::ascii_tolower(*iter));
  }
  return dst;
}

std::vector<std::string> ReadLines(const std::string& filename) {
  std::vector<std::string> lines = absl::StrSplit(ReadFile(filename), '\n');
  if (lines.back().empty()) {
    lines.pop_back();
  }
  return lines;
}

template <class W>
void TestJsonValidation() {
  const std::string file_base =
      absl::StrCat("testdata/stu3/validation/",
                   CamelCaseToLowerUnderscores(W::descriptor()->name()));
  std::vector<std::string> valid_lines =
      ReadLines(absl::StrCat(file_base, ".valid.ndjson"));
  for (auto line_iter : valid_lines) {
    // Note that we want to assert that the valid_line holds a valid string
    // value for the primitive, and not that the resulting primitive satisfies
    // all FHIR constraints, hence why we don't perform FHIR validation here.
    // This is necessary because "null" is a valid value for primitives that we
    // need to parse correctly, BUT unless there is also an extension on the
    // primitive, it is invalid as a whole.
    // In other words, "monkey" is an invalid value for Decimal, but "null"
    // *is* valid, even though it is not on its own sufficient to constitute a
    // valid decimal.
    FHIR_ASSERT_OK(
        JsonFhirStringToProtoWithoutValidating<W>(line_iter, kTimeZone)
            .status());
  }
  std::vector<std::string> invalid_lines =
      ReadLines(absl::StrCat(file_base, ".invalid.ndjson"));
  for (auto line_iter : invalid_lines) {
    absl::StatusOr<W> parsed = JsonFhirStringToProto<W>(line_iter, kTimeZone);
    if (parsed.ok()) {
      FAIL() << "Invalid string should have failed parsing: " << line_iter
             << " of type " << W::descriptor()->name() << "\nParsed as:\n"
             << parsed.value().DebugString();
    }
  }
}

template <class W>
void AddPrimitiveHasNoValue(W* primitive) {
  Extension* e = primitive->add_extension();
  e->mutable_url()->set_value(GetStructureDefinitionUrl(
      stu3::fhirproto::PrimitiveHasNoValue::descriptor()));
  e->mutable_value()->mutable_boolean()->set_value(true);
}

template <class W>
void TestBadProto(const W& w) {
  const ScopedErrorReporter reporter(
      &FailFastErrorHandler::FailOnErrorOrFatal(), "");
  absl::Status status =
      Stu3PrimitiveHandler::GetInstance()->ValidatePrimitive(w, reporter);
  ASSERT_FALSE(status.ok()) << "Should have failed: " << w.DebugString();
}

template <class W>
void TestProtoValidationsFromFile(const std::string& file_base,
                                  const bool has_invalid) {
  const std::vector<std::string> valid_proto_strings = absl::StrSplit(
      ReadFile(absl::StrCat(file_base, ".valid.prototxt")), "\n---\n");

  for (auto proto_string_iter : valid_proto_strings) {
    W w = PARSE_STU3_PROTO(proto_string_iter);
    const ScopedErrorReporter reporter(
        &FailFastErrorHandler::FailOnErrorOrFatal(), "");
    FHIR_ASSERT_OK(
        Stu3PrimitiveHandler::GetInstance()->ValidatePrimitive(w, reporter));
  }

  if (has_invalid) {
    const std::vector<std::string> invalid_proto_strings = absl::StrSplit(
        ReadFile(absl::StrCat(file_base, ".invalid.prototxt")), "\n---\n");

    for (auto proto_string_iter : invalid_proto_strings) {
      W w = PARSE_STU3_PROTO(proto_string_iter);
      TestBadProto(w);
    }
  }
}

template <class W>
void TestProtoValidation(const bool has_invalid = true) {
  // Test cases that are common to all primitives

  // It's ok to have no value if there's another extension present
  W only_extensions;
  AddPrimitiveHasNoValue(&only_extensions);
  Extension* e = only_extensions.add_extension();
  e->mutable_url()->set_value("abcd");
  e->mutable_value()->mutable_boolean()->set_value(true);
  const ScopedErrorReporter reporter(
      &FailFastErrorHandler::FailOnErrorOrFatal(), "");
  FHIR_ASSERT_OK(Stu3PrimitiveHandler::GetInstance()->ValidatePrimitive(
      only_extensions, reporter));

  // It's not ok to JUST have a no value extension.
  W just_no_value;
  AddPrimitiveHasNoValue(&just_no_value);
  TestBadProto(just_no_value);

  const std::string file_base =
      absl::StrCat("testdata/stu3/validation/",
                   CamelCaseToLowerUnderscores(W::descriptor()->name()));
  TestProtoValidationsFromFile<W>(file_base, has_invalid);
}

TEST(PrimitiveHandlerTest, ValidReference) {
  Observation obs = ReadStu3Proto<Observation>(
      "validation/observation_valid_reference.prototxt");
  const ScopedErrorReporter reporter(
      &FailFastErrorHandler::FailOnErrorOrFatal(), "Observation");
  FHIR_ASSERT_OK(Stu3PrimitiveHandler::GetInstance()->ValidateReferenceField(
      obs, obs.GetDescriptor()->FindFieldByName("specimen"), reporter));
}

TEST(PrimitiveHandlerTest, InvalidReference) {
  Observation obs = ReadStu3Proto<Observation>(
      "validation/observation_invalid_reference.prototxt");
  const ScopedErrorReporter reporter(
      &FailFastErrorHandler::FailOnErrorOrFatal(), "Observation");
  FHIR_ASSERT_STATUS(
      Stu3PrimitiveHandler::GetInstance()->ValidateReferenceField(
          obs, obs.GetDescriptor()->FindFieldByName("specimen"), reporter),
      "invalid-reference-disallowed-type-Device at Observation.specimen");
}

TEST(PrimitiveValidationTestJson, Base64Binary) {
  TestJsonValidation<Base64Binary>();
}

TEST(PrimitiveValidationTestJson, Boolean) { TestJsonValidation<Boolean>(); }

TEST(PrimitiveValidationTestJson, Code) { TestJsonValidation<Code>(); }

TEST(PrimitiveValidationTestJson, Date) { TestJsonValidation<Date>(); }

TEST(PrimitiveValidationTestJson, DateTime) { TestJsonValidation<DateTime>(); }

TEST(PrimitiveValidationTestJson, Decimal) { TestJsonValidation<Decimal>(); }

TEST(PrimitiveValidationTestJson, Id) { TestJsonValidation<Id>(); }

TEST(PrimitiveValidationTestJson, Instant) { TestJsonValidation<Instant>(); }

TEST(PrimitiveValidationTestJson, Integer) { TestJsonValidation<Integer>(); }

TEST(PrimitiveValidationTestJson, Markdown) { TestJsonValidation<Markdown>(); }

TEST(PrimitiveValidationTestJson, Oid) { TestJsonValidation<Oid>(); }

TEST(PrimitiveValidationTestJson, PositiveInt) {
  TestJsonValidation<PositiveInt>();
}

TEST(PrimitiveValidationTestJson, Reference) {
  TestJsonValidation<Reference>();
}

TEST(PrimitiveValidationTestJson, String) { TestJsonValidation<String>(); }

TEST(PrimitiveValidationTestJson, Time) { TestJsonValidation<Time>(); }

TEST(PrimitiveValidationTestJson, UnsignedInt) {
  TestJsonValidation<UnsignedInt>();
}

TEST(PrimitiveValidationTestJson, Uri) { TestJsonValidation<Uri>(); }

TEST(PrimitiveValidationTestJson, Xhtml) { TestJsonValidation<Xhtml>(); }

TEST(PrimitiveValidationTestProto, Base64Binary) {
  TestProtoValidation<Base64Binary>(false);
}

TEST(PrimitiveValidationTestProto, Boolean) {
  TestProtoValidation<Boolean>(false);
}

TEST(PrimitiveValidationTestProto, Code) { TestProtoValidation<Code>(); }

TEST(PrimitiveValidationTestProto, Date) { TestProtoValidation<Date>(); }

TEST(PrimitiveValidationTestProto, DateTime) {
  TestProtoValidation<DateTime>();
}

TEST(PrimitiveValidationTestProto, Decimal) { TestProtoValidation<Decimal>(); }

TEST(PrimitiveValidationTestProto, Id) { TestProtoValidation<Id>(); }

TEST(PrimitiveValidationTestProto, Instant) { TestProtoValidation<Instant>(); }

TEST(PrimitiveValidationTestProto, Integer) {
  TestProtoValidation<Integer>(false);
}

TEST(PrimitiveValidationTestProto, Markdown) {
  TestProtoValidation<Markdown>(false);
}

TEST(PrimitiveValidationTestProto, Oid) { TestProtoValidation<Oid>(); }

TEST(PrimitiveValidationTestProto, PositiveInt) {
  TestProtoValidation<PositiveInt>();
}

TEST(PrimitiveValidationTestProto, String) {
  TestProtoValidation<String>(false);
}

TEST(PrimitiveValidationTestProto, Time) { TestProtoValidation<Time>(); }

TEST(PrimitiveValidationTestProto, UnsignedInt) {
  TestProtoValidation<UnsignedInt>(false);
}

TEST(PrimitiveValidationTestProto, Uri) { TestProtoValidation<Uri>(false); }

TEST(PrimitiveValidationTestProto, Xhtml) {
  // XHTML can't have extensions - skip over extension tests.
  const std::string file_base =
      absl::StrCat("testdata/stu3/validation/",
                   CamelCaseToLowerUnderscores(Xhtml::descriptor()->name()));
  TestProtoValidationsFromFile<Xhtml>(file_base, false);
}

TEST(PrimitiveValidationTestProto, TypedCode) {
  TestProtoValidation<AdministrativeGenderCode>();
}

TEST(PrimitiveValidationTestProto, StringCode) {
  TestProtoValidation<MimeTypeCode>();
}

HANDLER_TYPE_TEST(String, "mandalorian", Stu3PrimitiveHandler);
HANDLER_TYPE_TEST(Boolean, true, Stu3PrimitiveHandler);
HANDLER_TYPE_TEST(Integer, 87, Stu3PrimitiveHandler);
HANDLER_TYPE_TEST(UnsignedInt, 86, Stu3PrimitiveHandler);
HANDLER_TYPE_TEST(PositiveInt, 85, Stu3PrimitiveHandler);
HANDLER_TYPE_TEST(Decimal, "4.7", Stu3PrimitiveHandler);

TEST(PrimitiveHandlerTest, NewDateTime) {
  DateTime expected_date_time;
  expected_date_time.set_value_us(5000);
  expected_date_time.set_timezone(kTimeZoneString);
  expected_date_time.set_precision(DateTime::DAY);

  std::unique_ptr<::google::protobuf::Message> generated_date_time(
      Stu3PrimitiveHandler::GetInstance()->NewDateTime(
          absl::FromUnixMicros(5000), kTimeZone, DateTimePrecision::kDay));

  ASSERT_THAT(generated_date_time, Pointee(EqualsProto(expected_date_time)));
}

TEST(PrimitiveHandlerTest, NewDateTimeFromString) {
  DateTime expected_date_time_year;
  expected_date_time_year.set_value_us(1546300800000000);
  expected_date_time_year.set_timezone("UTC");
  expected_date_time_year.set_precision(DateTime::YEAR);

  absl::StatusOr<::google::protobuf::Message*> generated_date_time_year_or_status(
      Stu3PrimitiveHandler::GetInstance()->NewDateTime("2019"));
  FHIR_ASSERT_OK(generated_date_time_year_or_status.status());
  std::unique_ptr<::google::protobuf::Message> generated_date_time_year(
      generated_date_time_year_or_status.value());
  ASSERT_THAT(generated_date_time_year,
              Pointee(EqualsProto(expected_date_time_year)));

  DateTime expected_date_time_month;
  expected_date_time_month.set_value_us(1548979200000000);
  expected_date_time_month.set_timezone("UTC");
  expected_date_time_month.set_precision(DateTime::MONTH);

  absl::StatusOr<::google::protobuf::Message*> generated_date_time_month_or_status(
      Stu3PrimitiveHandler::GetInstance()->NewDateTime("2019-02"));
  FHIR_ASSERT_OK(generated_date_time_month_or_status.status());
  std::unique_ptr<::google::protobuf::Message> generated_date_time_month(
      generated_date_time_month_or_status.value());
  ASSERT_THAT(generated_date_time_month,
              Pointee(EqualsProto(expected_date_time_month)));

  DateTime expected_date_time_second;
  expected_date_time_second.set_value_us(1549221133000000);
  expected_date_time_second.set_timezone("-08:00");
  expected_date_time_second.set_precision(DateTime::SECOND);

  absl::StatusOr<::google::protobuf::Message*> generated_date_time_second_or_status(
      Stu3PrimitiveHandler::GetInstance()->NewDateTime(
          "2019-02-03T11:12:13-08:00"));
  FHIR_ASSERT_OK(generated_date_time_second_or_status.status());
  std::unique_ptr<::google::protobuf::Message> generated_date_time_second(
      generated_date_time_second_or_status.value());
  ASSERT_THAT(generated_date_time_second,
              Pointee(EqualsProto(expected_date_time_second)));

  DateTime expected_date_time_ms;
  expected_date_time_ms.set_value_us(1549221133100000);
  expected_date_time_ms.set_timezone("-08:00");
  expected_date_time_ms.set_precision(DateTime::MILLISECOND);

  absl::StatusOr<::google::protobuf::Message*> generated_date_time_ms_or_status(
      Stu3PrimitiveHandler::GetInstance()->NewDateTime(
          "2019-02-03T11:12:13.1-08:00"));
  FHIR_ASSERT_OK(generated_date_time_ms_or_status.status());
  std::unique_ptr<::google::protobuf::Message> generated_date_time_ms(
      generated_date_time_ms_or_status.value());
  ASSERT_THAT(generated_date_time_ms,
              Pointee(EqualsProto(expected_date_time_ms)));

  DateTime expected_date_time_us;
  expected_date_time_us.set_value_us(1549221133100000);
  expected_date_time_us.set_timezone("-08:00");
  expected_date_time_us.set_precision(DateTime::MICROSECOND);

  absl::StatusOr<::google::protobuf::Message*> generated_date_time_us_or_status(
      Stu3PrimitiveHandler::GetInstance()->NewDateTime(
          "2019-02-03T11:12:13.1000-08:00"));
  FHIR_ASSERT_OK(generated_date_time_us_or_status.status());
  std::unique_ptr<::google::protobuf::Message> generated_date_time_us(
      generated_date_time_us_or_status.value());
  ASSERT_THAT(generated_date_time_us,
              Pointee(EqualsProto(expected_date_time_us)));

  // DateTime values are truncated to microsecond precision.
  absl::StatusOr<::google::protobuf::Message*> generated_date_time_ns_or_status(
      Stu3PrimitiveHandler::GetInstance()->NewDateTime(
          "2019-02-03T11:12:13.1000001-08:00"));
  FHIR_ASSERT_OK(generated_date_time_ns_or_status.status());
  std::unique_ptr<::google::protobuf::Message> generated_date_time_ns(
      generated_date_time_ns_or_status.value());
  ASSERT_THAT(generated_date_time_ns,
              Pointee(EqualsProto(expected_date_time_us)));
}

TEST(PrimitiveHandlerTest, DateTimeGetters) {
  DateTime date_time;
  date_time.set_value_us(5000);
  date_time.set_timezone(kTimeZoneString);
  date_time.set_precision(DateTime::DAY);

  absl::StatusOr<absl::Time> extracted_time =
      Stu3PrimitiveHandler::GetInstance()->GetDateTimeValue(date_time);
  FHIR_ASSERT_OK(extracted_time.status());
  ASSERT_EQ(extracted_time.value(), absl::FromUnixMicros(5000));

  absl::StatusOr<absl::TimeZone> extracted_time_zone =
      Stu3PrimitiveHandler::GetInstance()->GetDateTimeZone(date_time);
  FHIR_ASSERT_OK(extracted_time_zone.status());
  ASSERT_EQ(extracted_time_zone.value(), kTimeZone);

  absl::StatusOr<DateTimePrecision> extracted_precision =
      Stu3PrimitiveHandler::GetInstance()->GetDateTimePrecision(date_time);
  FHIR_ASSERT_OK(extracted_precision.status());
  ASSERT_EQ(extracted_precision.value(), DateTimePrecision::kDay);
}

TEST(PrimitiveHandlerTest, GetDateTimeValue) {
  DateTime date_time;
  date_time.set_value_us(5000);

  absl::StatusOr<absl::Time> extracted_time =
      Stu3PrimitiveHandler::GetInstance()->GetDateTimeValue(date_time);
  FHIR_ASSERT_OK(extracted_time.status());
  ASSERT_EQ(extracted_time.value(), absl::FromUnixMicros(5000));

  ASSERT_NE(::absl::OkStatus(),
            Stu3PrimitiveHandler::GetInstance()
                ->GetDateTimeValue(::google::protobuf::Any())
                .status());
}

TEST(PrimitiveHandlerTest, GetDateTimeZone) {
  DateTime date_time;
  date_time.set_timezone(kTimeZoneString);

  absl::StatusOr<absl::TimeZone> extracted_time_zone =
      Stu3PrimitiveHandler::GetInstance()->GetDateTimeZone(date_time);
  FHIR_ASSERT_OK(extracted_time_zone.status());
  ASSERT_EQ(extracted_time_zone.value(), kTimeZone);

  ASSERT_NE(::absl::OkStatus(), Stu3PrimitiveHandler::GetInstance()
                                    ->GetDateTimeZone(::google::protobuf::Any())
                                    .status());
}

TEST(PrimitiveHandlerTest, GetDateTimePrecision) {
  DateTime date_time;
  date_time.set_precision(DateTime::DAY);

  absl::StatusOr<DateTimePrecision> extracted_precision =
      Stu3PrimitiveHandler::GetInstance()->GetDateTimePrecision(date_time);
  FHIR_ASSERT_OK(extracted_precision.status());
  ASSERT_EQ(extracted_precision.value(), DateTimePrecision::kDay);

  ASSERT_NE(::absl::OkStatus(),
            Stu3PrimitiveHandler::GetInstance()
                ->GetDateTimePrecision(::google::protobuf::Any())
                .status());
}

TEST(PrimitiveHandlerTest, GetSimpleQuantityValue) {
  SimpleQuantity simple_quantity;
  simple_quantity.mutable_value()->set_value("1.5");

  absl::StatusOr<std::string> extracted_value =
      Stu3PrimitiveHandler::GetInstance()->GetSimpleQuantityValue(
          simple_quantity);
  FHIR_ASSERT_OK(extracted_value.status());
  ASSERT_EQ(extracted_value.value(), "1.5");

  ASSERT_NE(::absl::OkStatus(),
            Stu3PrimitiveHandler::GetInstance()
                ->GetSimpleQuantityValue(::google::protobuf::Any())
                .status());
}

TEST(PrimitiveHandlerTest, GetSimpleQuantityCode) {
  SimpleQuantity simple_quantity;
  simple_quantity.mutable_code()->set_value("12345");

  absl::StatusOr<std::string> extracted_code =
      Stu3PrimitiveHandler::GetInstance()->GetSimpleQuantityCode(
          simple_quantity);
  FHIR_ASSERT_OK(extracted_code.status());
  ASSERT_EQ(extracted_code.value(), "12345");

  ASSERT_NE(::absl::OkStatus(),
            Stu3PrimitiveHandler::GetInstance()
                ->GetSimpleQuantityCode(::google::protobuf::Any())
                .status());
}

TEST(PrimitiveHandlerTest, GetSimpleQuantitySystem) {
  SimpleQuantity simple_quantity;
  simple_quantity.mutable_system()->set_value("http://example.org");

  absl::StatusOr<std::string> extracted_system =
      Stu3PrimitiveHandler::GetInstance()->GetSimpleQuantitySystem(
          simple_quantity);
  FHIR_ASSERT_OK(extracted_system.status());
  ASSERT_EQ(extracted_system.value(), "http://example.org");

  ASSERT_NE(::absl::OkStatus(),
            Stu3PrimitiveHandler::GetInstance()
                ->GetSimpleQuantitySystem(::google::protobuf::Any())
                .status());
}

TEST(PrimitiveHandlerTest, GetCodingDisplay) {
  Coding coding;
  coding.mutable_display()->set_value("Philadelphia Eagles");

  absl::StatusOr<std::string> extracted_value =
      Stu3PrimitiveHandler::GetInstance()->GetCodingDisplay(coding);
  FHIR_ASSERT_OK(extracted_value.status());
  ASSERT_EQ(extracted_value.value(), "Philadelphia Eagles");

  ASSERT_NE(::absl::OkStatus(),
            Stu3PrimitiveHandler::GetInstance()
                ->GetCodingDisplay(::google::protobuf::Any())
                .status());
}

TEST(PrimitiveHandlerTest, GetCodingCode) {
  Coding coding;
  coding.mutable_code()->set_value("12345");

  absl::StatusOr<std::string> extracted_code =
      Stu3PrimitiveHandler::GetInstance()->GetCodingCode(coding);
  FHIR_ASSERT_OK(extracted_code.status());
  ASSERT_EQ(extracted_code.value(), "12345");

  ASSERT_NE(::absl::OkStatus(), Stu3PrimitiveHandler::GetInstance()
                                    ->GetCodingCode(::google::protobuf::Any())
                                    .status());
}

TEST(PrimitiveHandlerTest, GetCodingSystem) {
  Coding coding;
  coding.mutable_system()->set_value("http://example.org");

  absl::StatusOr<std::string> extracted_system =
      Stu3PrimitiveHandler::GetInstance()->GetCodingSystem(coding);
  FHIR_ASSERT_OK(extracted_system.status());
  ASSERT_EQ(extracted_system.value(), "http://example.org");

  ASSERT_NE(::absl::OkStatus(), Stu3PrimitiveHandler::GetInstance()
                                    ->GetCodingSystem(::google::protobuf::Any())
                                    .status());
}

}  // namespace

}  // namespace stu3
}  // namespace fhir
}  // namespace google
