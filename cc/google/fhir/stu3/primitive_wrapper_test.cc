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

#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "google/fhir/stu3/json_format.h"
#include "google/fhir/stu3/test_helper.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/status_test_util.h"

namespace google {
namespace fhir {
namespace stu3 {

namespace {

using namespace ::google::fhir::stu3::proto;  // NOLINT

static const char* const kTimeZoneString = "Australia/Sydney";
static const absl::TimeZone GetTimeZone() {
  absl::TimeZone tz;
  absl::LoadTimeZone(kTimeZoneString, &tz);
  return tz;
}

static const absl::TimeZone kTimeZone = GetTimeZone();

string CamelCaseToLowerUnderscores(const string& src) {
  string dst;
  for (auto iter = src.begin(); iter != src.end(); ++iter) {
    if (absl::ascii_isupper(*iter) && iter != src.begin()) {
      dst.push_back('_');
    }
    dst.push_back(absl::ascii_tolower(*iter));
  }
  return dst;
}

std::vector<string> ReadLines(const string& filename) {
  std::vector<string> lines = absl::StrSplit(ReadFile(filename), '\n');
  if (lines.back().empty()) {
    lines.pop_back();
  }
  return lines;
}

template <class W>
void TestValidation() {
  const string file_base =
      absl::StrCat("testdata/stu3/validation/",
                   CamelCaseToLowerUnderscores(W::descriptor()->name()));
  std::vector<string> valid_lines =
      ReadLines(absl::StrCat(file_base, ".valid.ndjson"));
  for (auto line_iter : valid_lines) {
    TF_ASSERT_OK(JsonFhirStringToProto<W>(line_iter, kTimeZone).status());
  }
  std::vector<string> invalid_lines =
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

TEST(PrimitiveValidationTest, Base64Binary) { TestValidation<Base64Binary>(); }

TEST(PrimitiveValidationTest, Boolean) { TestValidation<Boolean>(); }

TEST(PrimitiveValidationTest, Code) { TestValidation<Code>(); }

TEST(PrimitiveValidationTest, Date) { TestValidation<Date>(); }

TEST(PrimitiveValidationTest, DateTime) { TestValidation<DateTime>(); }

TEST(PrimitiveValidationTest, Decimal) { TestValidation<Decimal>(); }

TEST(PrimitiveValidationTest, Id) { TestValidation<Id>(); }

TEST(PrimitiveValidationTest, Instant) { TestValidation<Instant>(); }

TEST(PrimitiveValidationTest, Integer) { TestValidation<Integer>(); }

TEST(PrimitiveValidationTest, Markdown) { TestValidation<Markdown>(); }

TEST(PrimitiveValidationTest, Oid) { TestValidation<Oid>(); }

TEST(PrimitiveValidationTest, PositiveInt) { TestValidation<PositiveInt>(); }

TEST(PrimitiveValidationTest, Reference) { TestValidation<Reference>(); }

TEST(PrimitiveValidationTest, String) { TestValidation<String>(); }

TEST(PrimitiveValidationTest, Time) { TestValidation<Time>(); }

TEST(PrimitiveValidationTest, UnsignedInt) { TestValidation<UnsignedInt>(); }

TEST(PrimitiveValidationTest, Uri) { TestValidation<Uri>(); }

TEST(PrimitiveValidationTest, Xhtml) { TestValidation<Xhtml>(); }

}  // namespace

}  // namespace stu3
}  // namespace fhir
}  // namespace google
