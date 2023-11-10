/*
 * Copyright 2020 Google LLC
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

#include "google/fhir/dstu2/primitive_handler.h"

#include "google/protobuf/any.pb.h"
#include "google/protobuf/message.h"
#include "gtest/gtest.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "google/fhir/annotations.h"
#include "google/fhir/dstu2/json_format.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/status/status.h"
#include "google/fhir/test_helper.h"
#include "proto/google/fhir/proto/dstu2/datatypes.pb.h"
#include "proto/google/fhir/proto/dstu2/fhirproto_extensions.pb.h"
#include "proto/google/fhir/proto/dstu2/resources.pb.h"

namespace google {
namespace fhir {
namespace dstu2 {

namespace {

static const char* const kTimeZoneString = "Australia/Sydney";
static const absl::TimeZone GetTimeZone() {
  absl::TimeZone tz;
  absl::LoadTimeZone(kTimeZoneString, &tz);
  return tz;
}

static const absl::TimeZone kTimeZone = GetTimeZone();

std::string CamelCaseToLowerUnderscores(const std::string& src) {
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
      absl::StrCat("testdata/dstu2/validation/",
                   CamelCaseToLowerUnderscores(W::descriptor()->name()));
  std::vector<std::string> valid_lines =
      ReadLines(absl::StrCat(file_base, ".valid.ndjson"));
  for (auto line_iter : valid_lines) {
    // Note that we want to assert that the valid_line holds a valid string
    // value for the primitive, and not that the resulting primitive
    // satisfies all FHIR constraints, hence why we don't perform FHIR
    // validation here. This is necessary because "null" is a valid value for
    // primitives that we need to parse correctly, BUT unless there is also an
    // extension on the primitive, it is invalid as a whole. In other words,
    // "monkey" is an invalid value for Decimal, but "null" *is* valid, even
    // though it is not on its own sufficient to constitute a valid decimal.
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

TEST(PrimitiveValidationTestJson, Base64Binary) {
  TestJsonValidation<proto::Base64Binary>();
}

TEST(PrimitiveValidationTestJson, Boolean) {
  TestJsonValidation<proto::Boolean>();
}

TEST(PrimitiveValidationTestJson, Code) { TestJsonValidation<proto::Code>(); }

TEST(PrimitiveValidationTestJson, Date) { TestJsonValidation<proto::Date>(); }

TEST(PrimitiveValidationTestJson, DateTime) {
  TestJsonValidation<proto::DateTime>();
}

TEST(PrimitiveValidationTestJson, Decimal) {
  TestJsonValidation<proto::Decimal>();
}

TEST(PrimitiveValidationTestJson, Id) { TestJsonValidation<proto::Id>(); }

TEST(PrimitiveValidationTestJson, Instant) {
  TestJsonValidation<proto::Instant>();
}

TEST(PrimitiveValidationTestJson, Integer) {
  TestJsonValidation<proto::Integer>();
}

TEST(PrimitiveValidationTestJson, Markdown) {
  TestJsonValidation<proto::Markdown>();
}

TEST(PrimitiveValidationTestJson, Oid) { TestJsonValidation<proto::Oid>(); }

TEST(PrimitiveValidationTestJson, PositiveInt) {
  TestJsonValidation<proto::PositiveInt>();
}

TEST(PrimitiveValidationTestJson, String) {
  TestJsonValidation<proto::String>();
}

TEST(PrimitiveValidationTestJson, Time) { TestJsonValidation<proto::Time>(); }

TEST(PrimitiveValidationTestJson, UnsignedInt) {
  TestJsonValidation<proto::UnsignedInt>();
}

TEST(PrimitiveValidationTestJson, Uri) { TestJsonValidation<proto::Uri>(); }

template <class W>
void TestBadProto(const W& w) {
  const ScopedErrorReporter reporter(
      &FailFastErrorHandler::FailOnErrorOrFatal(), "");
  absl::Status status =
      DSTU2PrimitiveHandler::GetInstance()->ValidatePrimitive(w, reporter);
  ASSERT_FALSE(status.ok()) << "Should have failed: " << w.DebugString();
}

template <class W>
void TestProtoValidationsFromFile(const std::string& file_base,
                                  const bool has_invalid) {
  const std::vector<std::string> valid_proto_strings = absl::StrSplit(
      ReadFile(absl::StrCat(file_base, ".valid.prototxt")), "\n---\n");
  const ScopedErrorReporter reporter(
      &FailFastErrorHandler::FailOnErrorOrFatal(), "");

  for (auto proto_string_iter : valid_proto_strings) {
    W w = PARSE_STU3_PROTO(proto_string_iter);
    FHIR_ASSERT_OK(
        DSTU2PrimitiveHandler::GetInstance()->ValidatePrimitive(w, reporter));
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
void AddPrimitiveHasNoValue(W* primitive) {
  proto::Extension* e = primitive->add_extension();
  e->mutable_url()->set_value(GetStructureDefinitionUrl(
      dstu2::fhirproto::PrimitiveHasNoValue::descriptor()));
  e->mutable_value()->mutable_boolean()->set_value(true);
}

template <class W>
void TestProtoValidation(const bool has_invalid = true) {
  // Test cases that are common to all primitives

  // It's ok to have no value if there's another extension present
  W only_extensions;
  AddPrimitiveHasNoValue(&only_extensions);
  proto::Extension* e = only_extensions.add_extension();
  e->mutable_url()->set_value("abcd");
  e->mutable_value()->mutable_boolean()->set_value(true);

  const ScopedErrorReporter reporter(
      &FailFastErrorHandler::FailOnErrorOrFatal(), "");
  FHIR_ASSERT_OK(DSTU2PrimitiveHandler::GetInstance()->ValidatePrimitive(
      only_extensions, reporter));

  // It's not ok to JUST have a no value extension.
  W just_no_value;
  AddPrimitiveHasNoValue(&just_no_value);
  TestBadProto(just_no_value);

  const std::string file_base =
      absl::StrCat("testdata/dstu2/validation/",
                   CamelCaseToLowerUnderscores(W::descriptor()->name()));
  TestProtoValidationsFromFile<W>(file_base, has_invalid);
}

TEST(PrimitiveValidationTestProto, Base64Binary) {
  TestProtoValidation<proto::Base64Binary>(false);
}

TEST(PrimitiveValidationTestProto, Boolean) {
  TestProtoValidation<proto::Boolean>(false);
}

TEST(PrimitiveValidationTestProto, Code) { TestProtoValidation<proto::Code>(); }

TEST(PrimitiveValidationTestProto, Date) { TestProtoValidation<proto::Date>(); }

TEST(PrimitiveValidationTestProto, DateTime) {
  TestProtoValidation<proto::DateTime>();
}

TEST(PrimitiveValidationTestProto, Decimal) {
  TestProtoValidation<proto::Decimal>();
}

TEST(PrimitiveValidationTestProto, Id) { TestProtoValidation<proto::Id>(); }

TEST(PrimitiveValidationTestProto, Instant) {
  TestProtoValidation<proto::Instant>();
}

TEST(PrimitiveValidationTestProto, Integer) {
  TestProtoValidation<proto::Integer>(false);
}

TEST(PrimitiveValidationTestProto, Markdown) {
  TestProtoValidation<proto::Markdown>(false);
}

TEST(PrimitiveValidationTestProto, Oid) { TestProtoValidation<proto::Oid>(); }

TEST(PrimitiveValidationTestProto, PositiveInt) {
  TestProtoValidation<proto::PositiveInt>();
}

TEST(PrimitiveValidationTestProto, String) {
  TestProtoValidation<proto::String>(false);
}

TEST(PrimitiveValidationTestProto, Time) { TestProtoValidation<proto::Time>(); }

TEST(PrimitiveValidationTestProto, UnsignedInt) {
  TestProtoValidation<proto::UnsignedInt>(false);
}

TEST(PrimitiveValidationTestProto, Uri) {
  TestProtoValidation<proto::Uri>(false);
}

HANDLER_TYPE_TEST(String, "mandalorian", DSTU2PrimitiveHandler);
HANDLER_TYPE_TEST(Boolean, true, DSTU2PrimitiveHandler);
HANDLER_TYPE_TEST(Integer, 87, DSTU2PrimitiveHandler);
HANDLER_TYPE_TEST(PositiveInt, 99, DSTU2PrimitiveHandler);
HANDLER_TYPE_TEST(UnsignedInt, 203, DSTU2PrimitiveHandler);
HANDLER_TYPE_TEST(Decimal, "4.7", DSTU2PrimitiveHandler);

}  // namespace

}  // namespace dstu2
}  // namespace fhir
}  // namespace google
