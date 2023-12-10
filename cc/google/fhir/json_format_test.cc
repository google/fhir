/*
 * Copyright 2023 Google LLC
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

#include "google/fhir/json_format.h"

#include <memory>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/time/time.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/json/json_sax_handler.h"
#include "google/fhir/json/test_matchers.h"
#include "google/fhir/json_format_results.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/r4/primitive_handler.h"
#include "google/fhir/r5/primitive_handler.h"
#include "google/fhir/status/status.h"
#include "proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/bundle_and_contained_resource.pb.h"
#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"

namespace google::fhir {
namespace {

using ::testing::TestWithParam;

struct JsonFile {
  std::string contents;
  std::string name;
};

// Reads a FHIR NPM located at `path`, and returns a list of `JsonFile`s
// representing the resources located in the NPM.
absl::StatusOr<std::vector<JsonFile>> GetResourceJsonFiles(
    const absl::string_view path) {
  std::unique_ptr<archive, decltype(&archive_read_free)> archive(
      archive_read_new(), &archive_read_free);
  archive_read_support_filter_all(archive.get());
  archive_read_support_format_all(archive.get());

  const int archive_open_error =
      archive_read_open_filename(archive.get(), std::string(path).c_str(),
                                 /*block_size=*/1024);
  if (archive_open_error != ARCHIVE_OK) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Unable to open archive $0, error code: $1 $2.", path,
        archive_open_error, archive_error_string(archive.get())));
  }

  std::vector<JsonFile> resource_json;

  while (true) {
    archive_entry* entry;
    int read_next_status = archive_read_next_header(archive.get(), &entry);
    if (read_next_status == ARCHIVE_EOF) {
      break;
    } else if (read_next_status == ARCHIVE_RETRY) {
      continue;
    } else if (read_next_status != ARCHIVE_OK &&
               read_next_status != ARCHIVE_WARN) {
      return absl::InvalidArgumentError(absl::Substitute(
          "Unable to read archive $0, error code: $1 $2.", path,
          read_next_status, archive_error_string(archive.get())));
    }

    // Skip hardlinks, as we don't need to process the same file contents
    // multiple times.
    if (archive_entry_hardlink(entry) != nullptr) {
      continue;
    }

    std::string entry_name = archive_entry_pathname(entry);

    // Skip non-JSON files, along with some package JSON files that are not
    // resources.
    static const absl::flat_hash_set<std::string>* kSkipList =
        new absl::flat_hash_set<std::string>({".index.json", "package.json"});
    const std::string file_name =
        entry_name.substr(entry_name.find_last_of('/') + 1);

    if (!absl::EndsWith(file_name, ".json") ||
        absl::EndsWith(file_name, ".schema.json") ||
        kSkipList->contains(file_name)) {
      continue;
    }

    const la_int64_t length = archive_entry_size(entry);
    std::string contents(length, '\0');
    const la_ssize_t read =
        archive_read_data(archive.get(), &contents[0], length);
    if (read < length) {
      return absl::InvalidArgumentError(absl::Substitute(
          "Unable to read entry $0 from archive $1.", entry_name, path));
    }
    resource_json.push_back({.contents = contents, .name = entry_name});
  }

  return resource_json;
}

template <typename ContainedResourceType>
absl::StatusOr<ContainedResourceType> ToContainedResource(
    const std::string_view json, const PrimitiveHandler* primitive_handler) {
  ContainedResourceType resource;
  Parser parser(primitive_handler);
  absl::StatusOr<ParseResult> status_or_result =
      parser.MergeJsonFhirStringIntoProto(
          json, &resource, absl::UTCTimeZone(),
          /*validate=*/false, FailFastErrorHandler::FailOnFatalOnly());

  if (!status_or_result.ok() || *status_or_result == ParseResult::kFailed) {
    return absl::InternalError(absl::Substitute(
        "Failed parsing $0: $1", json, status_or_result.status().message()));
  }

  return resource;
}

// Check that two strings contain equivalent JSON value.  This prevents failures
// due to formatting.
void CheckJsonEq(const absl::string_view actual,
                 const absl::string_view expected) {
  internal::FhirJson actual_json, expected_json;
  ASSERT_TRUE(internal::ParseJsonValue(actual, actual_json).ok());
  ASSERT_TRUE(internal::ParseJsonValue(expected, expected_json).ok());
  EXPECT_THAT(actual_json, internal::JsonEq(&expected_json));
}

template <typename TestSuite>
std::string GetTestName(
    const testing::TestParamInfo<typename TestSuite::ParamType>& info) {
  absl::string_view name = info.param.name;
  name = name.substr(name.find_last_of('/') + 1);
  return absl::StrReplaceAll(name, {{"-", "_"}, {".", "_"}});
}

// Checks to ensure we are successfully loading the resource examples from the
// R4 NPM, and not just trivially passing.
TEST(R4JsonFormatTest, NpmSanityTest) {
  absl::StatusOr<std::vector<JsonFile>> files = GetResourceJsonFiles(
      "external/hl7.fhir.r4.core_4.0.1/file/hl7.fhir.r4.core@4.0.1.tgz");
  FHIR_ASSERT_OK(files.status());

  EXPECT_EQ(files->size(), 4578);
  for (const auto& file : *files) {
    EXPECT_GT(file.contents.size(), 0);
  }
}

using R4JsonFormatTest = TestWithParam<JsonFile>;

// Tests that the parser and printer can losslessly convert examples
// resources. Given a JSON file, converts it to a Resource proto, and then
// back to JSON. The resulting JSON should be identical in content (if not
// formatting) to the original JSON.
TEST_P(R4JsonFormatTest, RoundTripTest) {
  const JsonFile& file = GetParam();

  absl::StatusOr<r4::core::ContainedResource> resource =
      ToContainedResource<r4::core::ContainedResource>(
          file.contents, r4::R4PrimitiveHandler::GetInstance());

  FHIR_ASSERT_OK(resource.status());

  Printer printer(r4::R4PrimitiveHandler::GetInstance());

  absl::StatusOr<std::string> printed_json =
      printer.PrintFhirToJsonString(*resource);

  CHECK_OK(printed_json.status());
  CheckJsonEq(printed_json.value(), file.contents);
}

INSTANTIATE_TEST_SUITE_P(
    R4Tests, R4JsonFormatTest,
    testing::ValuesIn(
        GetResourceJsonFiles(
            "external/hl7.fhir.r4.core_4.0.1/file/hl7.fhir.r4.core@4.0.1.tgz")
            .value()),
    GetTestName<R4JsonFormatTest>);

// Checks to ensure we are successfully loading the resource examples from the
// R5 NPM, and not just trivially passing.
TEST(R5JsonFormatTest, NpmSanityTest) {
  absl::StatusOr<std::vector<JsonFile>> files = GetResourceJsonFiles(
      "external/hl7.fhir.r5.core_5.0.0/file/hl7.fhir.r5.core@5.0.0.tgz");
  FHIR_ASSERT_OK(files.status());

  EXPECT_EQ(files->size(), 2968);
  for (const auto& file : *files) {
    EXPECT_GT(file.contents.size(), 0);
  }
}

using R5JsonFormatTest = TestWithParam<JsonFile>;

// Tests that the parser and printer can losslessly convert examples
// resources. Given a JSON file, converts it to a Resource proto, and then
// back to JSON. The resulting JSON should be identical in content (if not
// formatting) to the original JSON.
TEST_P(R5JsonFormatTest, RoundTripTest) {
  const JsonFile& file = GetParam();

  absl::StatusOr<r5::core::ContainedResource> resource =
      ToContainedResource<r5::core::ContainedResource>(
          file.contents, r5::R5PrimitiveHandler::GetInstance());

  FHIR_ASSERT_OK(resource.status());

  Printer printer(r5::R5PrimitiveHandler::GetInstance());

  absl::StatusOr<std::string> printed_json =
      printer.PrintFhirToJsonString(*resource);

  CHECK_OK(printed_json.status());
  CheckJsonEq(printed_json.value(), file.contents);
}

INSTANTIATE_TEST_SUITE_P(
    R5Tests, R5JsonFormatTest,
    testing::ValuesIn(
        GetResourceJsonFiles(
            "external/hl7.fhir.r5.core_5.0.0/file/hl7.fhir.r5.core@5.0.0.tgz")
            .value()),
    GetTestName<R5JsonFormatTest>);
}  // namespace
}  // namespace google::fhir
