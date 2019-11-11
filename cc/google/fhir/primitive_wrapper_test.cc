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

#include "google/fhir/primitive_wrapper.h"

#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "google/fhir/annotations.h"
#include "google/fhir/json_format.h"
#include "google/fhir/test_helper.h"
#include "proto/stu3/codes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "tensorflow/core/lib/core/status_test_util.h"

namespace google {
namespace fhir {

namespace {

using namespace ::google::fhir::stu3::proto;  // NOLINT

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
    TF_ASSERT_OK(JsonFhirStringToProtoWithoutValidating<W>(line_iter, kTimeZone)
                     .status());
  }
  std::vector<std::string> invalid_lines =
      ReadLines(absl::StrCat(file_base, ".invalid.ndjson"));
  for (auto line_iter : invalid_lines) {
    StatusOr<W> parsed = JsonFhirStringToProto<W>(line_iter, kTimeZone);
    if (parsed.ok()) {
      FAIL() << "Invalid string should have failed parsing: " << line_iter
             << " of type " << W::descriptor()->name() << "\nParsed as:\n"
             << parsed.ValueOrDie().DebugString();
    }
  }
}

template <class W>
void AddPrimitiveHasNoValue(W* primitive) {
  Extension* e = primitive->add_extension();
  e->mutable_url()->set_value(GetStructureDefinitionUrl(
      stu3::google::PrimitiveHasNoValue::descriptor()));
  e->mutable_value()->mutable_boolean()->set_value(true);
}

template <class W>
void TestBadProto(const W& w) {
  Status status = ValidatePrimitive(w);
  ASSERT_FALSE(status.ok()) << "Should have failed: " << w.DebugString();
}

template <class W>
void TestProtoValidationsFromFile(const std::string& file_base,
                                  const bool has_invalid) {
  const std::vector<std::string> valid_proto_strings = absl::StrSplit(
      ReadFile(absl::StrCat(file_base, ".valid.prototxt")), "\n---\n");

  for (auto proto_string_iter : valid_proto_strings) {
    W w = PARSE_STU3_PROTO(proto_string_iter);
    TF_ASSERT_OK(ValidatePrimitive(w)) << w.DebugString();
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
  TF_ASSERT_OK(ValidatePrimitive(only_extensions));

  // It's not ok to JUST have a no value extension.
  W just_no_value;
  AddPrimitiveHasNoValue(&just_no_value);
  TestBadProto(just_no_value);

  const std::string file_base =
      absl::StrCat("testdata/stu3/validation/",
                   CamelCaseToLowerUnderscores(W::descriptor()->name()));
  TestProtoValidationsFromFile<W>(file_base, has_invalid);
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

}  // namespace

}  // namespace fhir
}  // namespace google
