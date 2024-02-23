// Copyright 2021 Google LLC
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

#include "google/fhir/fhir_package.h"

#include <stddef.h>
#include <string.h>

#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/cleanup/cleanup.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/json/json_sax_handler.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/testutil/archive.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/code_system.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/search_parameter.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/structure_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/value_set.pb.h"
#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"

namespace google::fhir {

namespace {
using ::google::fhir::testutil::CreateTarFileContaining;
using ::google::fhir::testutil::CreateZipFileContaining;
using ::testing::Eq;
using ::testing::Pointee;
using ::testing::UnorderedElementsAre;

constexpr int kR4DefinitionsCount = 655;
constexpr int kR4CodeSystemsCount = 1062;
constexpr int kR4ValuesetsCount = 1316;
constexpr int kR4SearchParametersCount = 1400;

TEST(FhirPackageTest, LoadSucceeds) {
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<FhirPackage> fhir_package,
      FhirPackage::Load(
          "external/hl7.fhir.r4.core_4.0.1/file/hl7.fhir.r4.core@4.0.1.tgz"));
  EXPECT_EQ(fhir_package->value_sets.size(), kR4ValuesetsCount);
  EXPECT_EQ(fhir_package->code_systems.size(), kR4CodeSystemsCount);
  EXPECT_EQ(fhir_package->structure_definitions.size(), kR4DefinitionsCount);
  EXPECT_EQ(fhir_package->search_parameters.size(), kR4SearchParametersCount);
}

TEST(FhirPackageTest, LoadAndGetResourceSucceedsFromZip) {
  // Define a bunch of fake resources.
  const std::string structure_definition_1 = R"({
    "resourceType": "StructureDefinition",
    "url": "http://sd1",
    "name": "sd1",
    "kind": "complex-type",
    "abstract": false,
    "type": "Extension",
    "status": "draft"
  })";
  const std::string structure_definition_2 = R"({
    "resourceType": "StructureDefinition",
    "url": "http://sd2",
    "name": "sd2",
    "kind": "complex-type",
    "abstract": false,
    "type": "Extension",
    "status": "draft"
  })";
  const std::string search_parameter_1 = R"({
    "resourceType": "SearchParameter",
    "url": "http://sp1",
    "name": "sp1",
    "status": "draft",
    "description": "sp1",
    "code": "facility",
    "base": ["Claim"],
    "type": "reference"
  })";
  const std::string search_parameter_2 = R"({
    "resourceType": "SearchParameter",
    "url": "http://sp2",
    "name": "sp2",
    "status": "draft",
    "description": "sp2",
    "code": "facility",
    "base": ["Claim"],
    "type": "reference"
  })";
  const std::string code_system_1 = R"({
    "resourceType": "CodeSystem",
    "url": "http://cs1",
    "name": "cs1",
    "status": "draft",
    "content": "complete"
  })";
  const std::string code_system_2 = R"({
    "resourceType": "CodeSystem",
    "url": "http://cs2",
    "name": "cs2",
    "status": "draft",
    "content": "complete"
  })";
  const std::string value_set_1 = R"({
    "resourceType": "ValueSet",
    "url": "http://vs1",
    "name": "vs1",
    "status": "draft"
  })";
  const std::string value_set_2 = R"({
    "resourceType": "ValueSet",
    "url": "http://vs2",
    "name": "vs2",
    "status": "draft"
  })";
  // Create a bundle for half of the resources.
  std::string bundle = absl::StrFormat(
      R"({
        "resourceType": "Bundle",
        "entry": [
          {"resource": %s},
          {"resource": %s},
          {
            "resource": {
              "resourceType": "Bundle",
              "entry": [
                {"resource": %s},
                {"resource": %s}
              ]
            }
          }
        ]
      })",
      structure_definition_2, search_parameter_2, code_system_2, value_set_2);

  // Put those resources in a FhirPackage.
  FHIR_ASSERT_OK_AND_ASSIGN(std::string temp_name,
                            CreateZipFileContaining({
                                {"sd1.json", structure_definition_1},
                                {"sp1.json", search_parameter_1},
                                {"cs1.json", code_system_1},
                                {"vs1.json", value_set_1},
                                {"bundle.json", bundle.c_str()},
                            }));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  FHIR_ASSERT_OK_AND_ASSIGN(std::unique_ptr<FhirPackage> fhir_package,
                            FhirPackage::Load(temp_name));

  // Check that we can retrieve all our resources;
  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<r4::core::StructureDefinition> sd_result,
      fhir_package->GetStructureDefinition("http://sd1"));
  EXPECT_EQ(sd_result->url().value(), "http://sd1");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<r4::core::StructureDefinition> sd_result2,
      fhir_package->GetStructureDefinition("http://sd2"));
  EXPECT_EQ(sd_result2->url().value(), "http://sd2");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<r4::core::SearchParameter> sp_result,
      fhir_package->GetSearchParameter("http://sp1"));
  EXPECT_EQ(sp_result->url().value(), "http://sp1");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<r4::core::SearchParameter> sp_result2,
      fhir_package->GetSearchParameter("http://sp2"));
  EXPECT_EQ(sp_result2->url().value(), "http://sp2");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<fhir::r4::core::CodeSystem> cs_result,
      fhir_package->GetCodeSystem("http://cs1"));
  EXPECT_EQ(cs_result->url().value(), "http://cs1");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<fhir::r4::core::CodeSystem> cs_result2,
      fhir_package->GetCodeSystem("http://cs2"));
  EXPECT_EQ(cs_result2->url().value(), "http://cs2");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<fhir::r4::core::ValueSet> vs_result,
      fhir_package->GetValueSet("http://vs1"));
  EXPECT_EQ(vs_result->url().value(), "http://vs1");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<fhir::r4::core::ValueSet> vs_result2,
      fhir_package->GetValueSet("http://vs2"));
  EXPECT_EQ(vs_result2->url().value(), "http://vs2");
}

TEST(FhirPackageTest, LoadAndGetResourceSucceedsFromTar) {
  // Define a bunch of fake resources.
  const std::string structure_definition_1 = R"({
    "resourceType": "StructureDefinition",
    "url": "http://sd1",
    "name": "sd1",
    "kind": "complex-type",
    "abstract": false,
    "type": "Extension",
    "status": "draft"
  })";
  const std::string structure_definition_2 = R"({
    "resourceType": "StructureDefinition",
    "url": "http://sd2",
    "name": "sd2",
    "kind": "complex-type",
    "abstract": false,
    "type": "Extension",
    "status": "draft"
  })";
  const std::string search_parameter_1 = R"({
    "resourceType": "SearchParameter",
    "url": "http://sp1",
    "name": "sp1",
    "status": "draft",
    "description": "sp1",
    "code": "facility",
    "base": ["Claim"],
    "type": "reference"
  })";
  const std::string search_parameter_2 = R"({
    "resourceType": "SearchParameter",
    "url": "http://sp2",
    "name": "sp2",
    "status": "draft",
    "description": "sp2",
    "code": "facility",
    "base": ["Claim"],
    "type": "reference"
  })";
  const std::string code_system_1 = R"({
    "resourceType": "CodeSystem",
    "url": "http://cs1",
    "name": "cs1",
    "status": "draft",
    "content": "complete"
  })";
  const std::string code_system_2 = R"({
    "resourceType": "CodeSystem",
    "url": "http://cs2",
    "name": "cs2",
    "status": "draft",
    "content": "complete"
  })";
  const std::string value_set_1 = R"({
    "resourceType": "ValueSet",
    "url": "http://vs1",
    "name": "vs1",
    "status": "draft"
  })";
  const std::string value_set_2 = R"({
    "resourceType": "ValueSet",
    "url": "http://vs2",
    "name": "vs2",
    "status": "draft"
  })";
  // Create a bundle for half of the resources.
  std::string bundle = absl::StrFormat(
      R"({
        "resourceType": "Bundle",
        "entry": [
          {"resource": %s},
          {"resource": %s},
          {
            "resource": {
              "resourceType": "Bundle",
              "entry": [
                {"resource": %s},
                {"resource": %s}
              ]
            }
          }
        ]
      })",
      structure_definition_2, search_parameter_2, code_system_2, value_set_2);

  // Put those resources in a FhirPackage.
  FHIR_ASSERT_OK_AND_ASSIGN(std::string temp_name,
                            CreateTarFileContaining({
                                {"sd1.json", structure_definition_1},
                                {"sp1.json", search_parameter_1},
                                {"cs1.json", code_system_1},
                                {"vs1.json", value_set_1},
                                {"bundle.json", bundle.c_str()},
                            }));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  FHIR_ASSERT_OK_AND_ASSIGN(std::unique_ptr<FhirPackage> fhir_package,
                            FhirPackage::Load(temp_name));

  // Check that we can retrieve all our resources;
  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<r4::core::StructureDefinition> sd_result,
      fhir_package->GetStructureDefinition("http://sd1"));
  EXPECT_EQ(sd_result->url().value(), "http://sd1");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<r4::core::StructureDefinition> sd_result2,
      fhir_package->GetStructureDefinition("http://sd2"));
  EXPECT_EQ(sd_result2->url().value(), "http://sd2");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<r4::core::SearchParameter> sp_result,
      fhir_package->GetSearchParameter("http://sp1"));
  EXPECT_EQ(sp_result->url().value(), "http://sp1");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<r4::core::SearchParameter> sp_result2,
      fhir_package->GetSearchParameter("http://sp2"));
  EXPECT_EQ(sp_result2->url().value(), "http://sp2");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<fhir::r4::core::CodeSystem> cs_result,
      fhir_package->GetCodeSystem("http://cs1"));
  EXPECT_EQ(cs_result->url().value(), "http://cs1");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<fhir::r4::core::CodeSystem> cs_result2,
      fhir_package->GetCodeSystem("http://cs2"));
  EXPECT_EQ(cs_result2->url().value(), "http://cs2");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<fhir::r4::core::ValueSet> vs_result,
      fhir_package->GetValueSet("http://vs1"));
  EXPECT_EQ(vs_result->url().value(), "http://vs1");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<fhir::r4::core::ValueSet> vs_result2,
      fhir_package->GetValueSet("http://vs2"));
  EXPECT_EQ(vs_result2->url().value(), "http://vs2");
}

TEST(FhirPackageTest, LoadWithHardlinksInArchiveSucceeds) {
  std::string temp_name = std::tmpnam(nullptr);
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  // Introduce a new scope so all the Cleanup actions occur at its end.
  {
    // Create a tar archive.
    archive* archive = archive_write_new();
    absl::Cleanup archive_closer = [&archive] { archive_write_free(archive); };

    int errorp = archive_write_set_format_ustar(archive);
    ASSERT_EQ(errorp, ARCHIVE_OK) << absl::StrFormat(
        "Failed to create archive due to error code: %d.", errorp);

    errorp = archive_write_open_filename(archive, temp_name.c_str());
    ASSERT_EQ(errorp, ARCHIVE_OK)
        << absl::StrFormat("Failed to create archive due to error code: %d %s.",
                           errorp, archive_error_string(archive));

    // Add an entry for a value set.
    archive_entry* entry = archive_entry_new();
    archive_entry_set_pathname(entry, "a_value_set.json");

    char contents[] =
        R"({"resourceType": "ValueSet", "url": "http://value.set/id",
                "version": "1.0", "id": "a-value-set", "status": "draft"})";
    size_t content_size = strlen(contents);
    archive_entry_set_size(entry, content_size);
    archive_entry_set_filetype(entry, AE_IFREG);
    absl::Cleanup entry_closer = [&entry] { archive_entry_free(entry); };

    errorp = archive_write_header(archive, entry);
    ASSERT_EQ(errorp, ARCHIVE_OK)
        << absl::StrFormat("Unable to add file to archive, error code: %d %s",
                           errorp, archive_error_string(archive));

    la_ssize_t written = archive_write_data(archive, contents, content_size);
    ASSERT_TRUE(written >= content_size)
        << absl::StrFormat("Unable to add file to archive, error: %s",
                           archive_error_string(archive));

    // Add a hard-link to the above entry.
    archive_entry* link_entry = archive_entry_new();
    archive_entry_set_pathname(link_entry, "a_duplicate_value_set.json");
    archive_entry_set_hardlink(link_entry, "a_value_set.json");
    absl::Cleanup link_entry_closer = [&link_entry] {
      archive_entry_free(link_entry);
    };

    errorp = archive_write_header(archive, link_entry);
    ASSERT_EQ(errorp, ARCHIVE_OK)
        << absl::StrFormat("Unable to add file to archive, error code: %d %s",
                           errorp, archive_error_string(archive));
  }

  // Ensure we can load the package with the hardlink.
  FHIR_ASSERT_OK_AND_ASSIGN(std::unique_ptr<FhirPackage> fhir_package,
                            FhirPackage::Load(temp_name));

  // Ensure we can read from it.
  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<fhir::r4::core::ValueSet> value_set,
      fhir_package->GetValueSet("http://value.set/id"));
  EXPECT_EQ(value_set->url().value(), "http://value.set/id");
}

TEST(FhirPackageTest, GetResourceForVersionedUriFindsResource) {
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string temp_name,
      CreateZipFileContaining(
          {{"a_value_set.json",
            R"({"resourceType": "ValueSet", "url": "http://value.set/id",
                "version": "1.0", "id": "a-value-set", "status": "draft"})"}}));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  FHIR_ASSERT_OK_AND_ASSIGN(std::unique_ptr<FhirPackage> fhir_package,
                            FhirPackage::Load(temp_name));

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<fhir::r4::core::ValueSet> value_set,
      fhir_package->GetValueSet("http://value.set/id|1.0"));
  EXPECT_EQ(value_set->url().value(), "http://value.set/id");
}

TEST(FhirPackageTest, GetResourceForVersionedUriReturnsErrorOnVersionMismatch) {
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string temp_name,
      CreateZipFileContaining(
          {{"a_value_set.json",
            R"({"resourceType": "ValueSet", "url": "http://value.set/id",
                "version": "1.0", "id": "a-value-set", "status": "draft"})"}}));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  FHIR_ASSERT_OK_AND_ASSIGN(std::unique_ptr<FhirPackage> fhir_package,
                            FhirPackage::Load(temp_name));

  EXPECT_EQ(
      fhir_package->GetValueSet("http://value.set/id|2.0").status().code(),
      absl::StatusCode::kNotFound);
}

TEST(FhirPackageTest, ResourceWithParseErrorFails) {
  // Define a malformed resource to test parse failures.
  const std::string malformed_struct_def = R"({
    "resourceType": "StructureDefinition",
    "url": "http://malformed_test",
    "name": "malformed_json_without_closing_quote,
    "kind": "complex-type",
    "abstract": false,
    "type": "Extension",
    "status": "draft"
  })";

  // Put those resources in a FhirPackage.
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string temp_name,
      CreateZipFileContaining({
          {"malformed_struct_def.json", malformed_struct_def},
      }));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  EXPECT_EQ(FhirPackage::Load(temp_name).status().code(),
            absl::StatusCode::kInvalidArgument);
}

TEST(FhirPackageTest, GetResourceForMissingUriFindsNothing) {
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string temp_name,
      CreateZipFileContaining(
          {{"a_value_set.json",
            R"({"resourceType": "ValueSet", "url": "http://value.set/id",
               "id": "a-value-set", "status": "draft"})"}}));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  FHIR_ASSERT_OK_AND_ASSIGN(std::unique_ptr<FhirPackage> fhir_package,
                            FhirPackage::Load(temp_name));

  EXPECT_EQ(fhir_package->GetValueSet("missing").status().code(),
            absl::StatusCode::kNotFound);
}

TEST(FhirPackageTest, UntrackedResourceTypeIgnored) {
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string temp_name,
      CreateZipFileContaining({
          {"a_value_set.json",
           R"({"resourceType": "ValueSet", "url": "http://value.set/id",
               "id": "a-value-set", "status": "draft"
          })"},
          {"sample_patient.json",
           R"({"resourceType": "Patient", "id": "dqd"})"},
      }));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  FHIR_ASSERT_OK_AND_ASSIGN(std::unique_ptr<FhirPackage> fhir_package,
                            FhirPackage::Load(temp_name));

  EXPECT_TRUE(fhir_package->GetValueSet("http://value.set/id").ok());
}

TEST(FhirPackageTest, NonResourceIgnored) {
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string temp_name,
      CreateZipFileContaining({
          {"a_value_set.json",
           R"({"resourceType": "ValueSet", "url": "http://value.set/id",
               "id": "a-value-set", "status": "draft"})"},
          {"random_file.json", R"({"foo": "bar", "baz": "quux"})"},
      }));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  FHIR_ASSERT_OK_AND_ASSIGN(std::unique_ptr<FhirPackage> fhir_package,
                            FhirPackage::Load(temp_name));

  EXPECT_TRUE(fhir_package->GetValueSet("http://value.set/id").ok());
}

TEST(FhirPackageTest, LoadWithUserProvidedHandlerCallsHandler) {
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string temp_name,
      CreateZipFileContaining({
          {"a_value_set.json",
           R"({"resourceType": "ValueSet", "url": "http://value.set/id",
               "id": "a-value-set", "status": "draft"})"},
          {"random_file.json", R"({"foo": "bar", "baz": "quux"})"},
      }));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  // Pass a handler which counts the number of entries in the archive.
  int count = 0;
  auto entry_counter = [&count](absl::string_view entry_name,
                                absl::string_view contents,
                                FhirPackage& package) {
    count++;
    return absl::OkStatus();
  };

  FHIR_ASSERT_OK_AND_ASSIGN(std::unique_ptr<FhirPackage> fhir_package,
                            FhirPackage::Load(temp_name, entry_counter));

  EXPECT_EQ(count, 2);
  // Ensure we can read the package as normal.
  EXPECT_TRUE(fhir_package->GetValueSet("http://value.set/id").ok());
}

TEST(FhirPackageTest, WithResourcesIterateJsonSucceeds) {
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string temp_name,
      CreateZipFileContaining(
          {{"vs1.json",
            R"({"resourceType": "ValueSet", "url": "http://value.set/id-1",
                "id": "a-value-set-1", "status": "draft"})"},
           {"vs2.json",
            R"({"resourceType": "ValueSet", "url": "http://value.set/id-2",
                "id": "a-value-set-2", "status": "draft"})"},
           {"vs3.json",
            R"({"resourceType": "ValueSet", "url": "http://value.set/id-3",
                "id": "a-value-set-3", "status": "draft"})"},
           {"bundle.json",
            R"({"resourceType": "Bundle", "entry": [
                 {"resource": {
                   "resourceType": "ValueSet", "url": "http://value.set/id-b1",
                   "id": "bundled-a-value-set-1", "status": "draft"}},
                 {"resource": {
                   "resourceType": "ValueSet", "url": "http://value.set/id-b2",
                   "id": "bundled-a-value-set-2", "status": "draft"}}]})"}}));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  FHIR_ASSERT_OK_AND_ASSIGN(std::unique_ptr<FhirPackage> fhir_package,
                            FhirPackage::Load(temp_name));

  // Collect all the JSON objects returned by the json_begin iterator.
  std::vector<const internal::FhirJson*> found;
  for (auto itr = fhir_package->value_sets.json_begin();
       itr != fhir_package->value_sets.json_end(); ++itr) {
    found.push_back(*itr);
  }

  // Ensure we found the expected JSON objects.
  internal::FhirJson expected_1;
  FHIR_ASSERT_OK(internal::ParseJsonValue(
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-1",
          "id": "a-value-set-1", "status": "draft"})",
      expected_1));

  internal::FhirJson expected_2;
  FHIR_ASSERT_OK(internal::ParseJsonValue(
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-2",
          "id": "a-value-set-2", "status": "draft"})",
      expected_2));

  internal::FhirJson expected_3;
  FHIR_ASSERT_OK(internal::ParseJsonValue(
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-3",
          "id": "a-value-set-3", "status": "draft"})",
      expected_3));

  internal::FhirJson expected_bundle_1;
  FHIR_ASSERT_OK(internal::ParseJsonValue(
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-b1",
          "id": "bundled-a-value-set-1", "status": "draft"})",
      expected_bundle_1));

  internal::FhirJson expected_bundle_2;
  FHIR_ASSERT_OK(internal::ParseJsonValue(
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-b2",
          "id": "bundled-a-value-set-2", "status": "draft"})",
      expected_bundle_2));

  EXPECT_THAT(found,
              UnorderedElementsAre(Pointee(Eq(std::ref(expected_1))),
                                   Pointee(Eq(std::ref(expected_2))),
                                   Pointee(Eq(std::ref(expected_3))),
                                   Pointee(Eq(std::ref(expected_bundle_1))),
                                   Pointee(Eq(std::ref(expected_bundle_2)))));
}

TEST(FhirPackageTest, LoadWithUserProvidedHandlerReturnsErrorsFromHandler) {
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string temp_name,
      CreateZipFileContaining({
          {"a_value_set.json",
           R"({"resourceType": "ValueSet", "url": "http://value.set/id",
               "id": "a-value-set", "status": "draft"})"},
          {"random_file.json", R"({"foo": "bar", "baz": "quux"})"},
      }));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  // Pass a handler returning a non-ok status and ensure Load returns an error.
  auto unhappy_handler = [](absl::string_view entry_name,
                            absl::string_view contents, FhirPackage& package) {
    return absl::InvalidArgumentError("oh no!");
  };

  EXPECT_EQ(FhirPackage::Load(temp_name, unhappy_handler).status().code(),
            absl::StatusCode::kInvalidArgument);
}

TEST(FhirPackageTest, ImplementationGuideEmptyForPackageWithoutIG) {
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string temp_name,
      CreateZipFileContaining(
          {{"a_value_set.json",
            R"({"resourceType": "ValueSet", "url": "http://value.set/id",
                "version": "1.0", "id": "a-value-set", "status": "draft"})"}}));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  FHIR_ASSERT_OK_AND_ASSIGN(std::unique_ptr<FhirPackage> fhir_package,
                            FhirPackage::Load(temp_name));

  EXPECT_EQ(fhir_package->implementation_guide, nullptr);
}

TEST(FhirPackageTest, IgR4Ignored) {
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string temp_name,
      CreateZipFileContaining(
          {{"ig-r4.json", "gobbledygook ( that \" doesn't parse"}}));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  FHIR_ASSERT_OK_AND_ASSIGN(std::unique_ptr<FhirPackage> fhir_package,
                            FhirPackage::Load(temp_name));

  EXPECT_EQ(fhir_package->implementation_guide, nullptr);
}

TEST(FhirPackageTest, ImplementationGuidePopulatedForPackageWithIG) {
  FHIR_ASSERT_OK_AND_ASSIGN(std::string temp_name,
                            CreateZipFileContaining({{"ig.json",
                                                      R"json(
            {
              "resourceType": "ImplementationGuide",
              "url": "igurl"
            }
            )json"}}));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  FHIR_ASSERT_OK_AND_ASSIGN(std::unique_ptr<FhirPackage> fhir_package,
                            FhirPackage::Load(temp_name));

  EXPECT_EQ(fhir_package->implementation_guide->url().value(), "igurl");
}

TEST(FhirPackageTest, ImplementationGuideWithInvalidIdSucceeds) {
  FHIR_ASSERT_OK_AND_ASSIGN(std::string temp_name,
                            CreateZipFileContaining({{"ig.json",
                                                      R"json(
            {
              "resourceType": "ImplementationGuide",
              "url": "igurl",
              "id": "thisisaverylongidstringthatshouldntbevalidbutmostexternalvalidatorsdontseemtocarethatthereisa64charlimitonids"
            }
            )json"}}));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  FHIR_ASSERT_OK_AND_ASSIGN(std::unique_ptr<FhirPackage> fhir_package,
                            FhirPackage::Load(temp_name));

  EXPECT_EQ(fhir_package->implementation_guide->id().value(),
            "thisisaverylongidstringthatshouldntbevalidbutmostexternalvalidator"
            "sdontseemtocarethatthereisa64charlimitonids");
}

TEST(ResourceCollectionTest, GetResourceFromCacheHasPointerStability) {
  ResourceCollection<fhir::r4::core::ValueSet> collection;

  auto vs1 = std::make_unique<internal::FhirJson>();
  FHIR_ASSERT_OK(internal::ParseJsonValue(
      R"({"resourceType": "ValueSet", "url": "http://value.set/id1",
                      "id": "hello", "status": "draft"})",
      *vs1));

  auto vs2 = std::make_unique<internal::FhirJson>();
  FHIR_ASSERT_OK(internal::ParseJsonValue(
      R"({"resourceType": "ValueSet", "url": "http://value.set/id2",
                      "id": "goodbye", "status": "draft"})",
      *vs2));

  // Insert and retrieve vs1 to have it cached.
  FHIR_ASSERT_OK(collection.Put(std::move(vs1)));
  ASSERT_TRUE(collection.Get("http://value.set/id1").ok());

  // Retrieve vs1 from the cache
  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<fhir::r4::core::ValueSet> result1,
      collection.Get("http://value.set/id1"));
  EXPECT_EQ(result1->id().value(), "hello");

  // Insert and retrieve vs2 to have it cached.
  FHIR_ASSERT_OK(collection.Put(std::move(vs2)));
  EXPECT_TRUE(collection.Get("http://value.set/id2").ok());

  // Retrieve vs2 from the cache.
  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<fhir::r4::core::ValueSet> result2,
      collection.Get("http://value.set/id2"));
  EXPECT_EQ(result2->id().value(), "goodbye");

  // Ensure the vs1 pointer still works.
  EXPECT_EQ(result1->id().value(), "hello");
}

TEST(ResourceCollectionTest, PutGetResourceSucceeds) {
  auto parsed_json = std::make_unique<internal::FhirJson>();
  FHIR_ASSERT_OK(internal::ParseJsonValue(
      R"({"resourceType": "ValueSet", "url": "http://value.set/id",
                      "id": "a-value-set", "status": "draft"})",
      *parsed_json));

  ResourceCollection<fhir::r4::core::ValueSet> collection;
  FHIR_ASSERT_OK(collection.Put(std::move(parsed_json)));

  FHIR_ASSERT_OK_AND_ASSIGN(
      const std::unique_ptr<fhir::r4::core::ValueSet> result,
      collection.Get("http://value.set/id"));
  EXPECT_EQ(result->id().value(), "a-value-set");
  EXPECT_EQ(result->url().value(), "http://value.set/id");
}

TEST(ResourceCollectionTest, WithValidResourcesIterateSucceeds) {
  std::vector<std::string> resources = {
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-1",
          "id": "a-value-set-1", "status": "draft"})",
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-2",
          "id": "a-value-set-2", "status": "draft"})",
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-3",
          "id": "a-value-set-3", "status": "draft"})",
  };

  ResourceCollection<fhir::r4::core::ValueSet> collection;
  for (const std::string& resource : resources) {
    auto parsed_json = std::make_unique<internal::FhirJson>();
    FHIR_ASSERT_OK(internal::ParseJsonValue(resource, *parsed_json));
    FHIR_ASSERT_OK(collection.Put(std::move(parsed_json)));
  }

  std::vector<std::string> found;
  for (const std::unique_ptr<fhir::r4::core::ValueSet> value_set : collection) {
    found.push_back(value_set->url().value());
  }
  EXPECT_THAT(found, UnorderedElementsAre("http://value.set/id-1",
                                          "http://value.set/id-2",
                                          "http://value.set/id-3"));
}

TEST(ResourceCollectionTest, WithValidCachedResourcesIterateSucceeds) {
  std::vector<std::string> resources = {
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-1",
          "id": "a-value-set-1", "status": "draft"})",
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-2",
          "id": "a-value-set-2", "status": "draft"})",
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-3",
          "id": "a-value-set-3", "status": "draft"})",
  };

  ResourceCollection<fhir::r4::core::ValueSet> collection;
  for (const std::string& resource : resources) {
    auto parsed_json = std::make_unique<internal::FhirJson>();
    FHIR_ASSERT_OK(internal::ParseJsonValue(resource, *parsed_json));
    FHIR_ASSERT_OK(collection.Put(std::move(parsed_json)));
  }

  // Get some resources to cache them before iterating.
  ASSERT_TRUE(collection.Get("http://value.set/id-1").ok());
  ASSERT_TRUE(collection.Get("http://value.set/id-3").ok());

  std::vector<std::string> found;
  for (const std::unique_ptr<fhir::r4::core::ValueSet> value_set : collection) {
    found.push_back(value_set->url().value());
  }
  EXPECT_THAT(found, UnorderedElementsAre("http://value.set/id-1",
                                          "http://value.set/id-2",
                                          "http://value.set/id-3"));
}

TEST(ResourceCollectionTest, WithNoResourcesIterateEmpty) {
  ResourceCollection<fhir::r4::core::ValueSet> collection;

  int count = 0;
  for (const std::unique_ptr<fhir::r4::core::ValueSet> value_set : collection) {
    count++;
  }
  EXPECT_EQ(count, 0);
}

TEST(ResourceCollectionTest, WithInvalidResourcesIterateEmpty) {
  // These resources are invalid due to missing fields.
  std::vector<std::string> resources = {
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-1"})",
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-2"})",
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-3"})",
  };

  ResourceCollection<fhir::r4::core::ValueSet> collection;
  for (const std::string& resource : resources) {
    auto parsed_json = std::make_unique<internal::FhirJson>();
    FHIR_ASSERT_OK(internal::ParseJsonValue(resource, *parsed_json));
    FHIR_ASSERT_OK(collection.Put(std::move(parsed_json)));
  }

  int count = 0;
  for (const std::unique_ptr<fhir::r4::core::ValueSet> value_set : collection) {
    count++;
  }
  EXPECT_EQ(count, 0);
}

TEST(ResourceCollectionTest, WithValidAndInvalidResourcesIterateSkipsInvalid) {
  std::vector<std::string> resources = {
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-invalid-1"})",
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-invalid-2"})",
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-1",
          "id": "a-value-set-1", "status": "draft"})",
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-invalid-3"})",
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-2",
          "id": "a-value-set-2", "status": "draft"})",
      R"({"resourceType": "ValueSet", "url": "http://value.set/id-invalid-4"})",
  };

  ResourceCollection<fhir::r4::core::ValueSet> collection;
  for (const std::string& resource : resources) {
    auto parsed_json = std::make_unique<internal::FhirJson>();
    FHIR_ASSERT_OK(internal::ParseJsonValue(resource, *parsed_json));
    FHIR_ASSERT_OK(collection.Put(std::move(parsed_json)));
  }

  std::vector<std::string> found;
  for (const std::unique_ptr<fhir::r4::core::ValueSet> value_set : collection) {
    found.push_back(value_set->url().value());
  }
  EXPECT_THAT(found, UnorderedElementsAre("http://value.set/id-1",
                                          "http://value.set/id-2"));
}

TEST(ResourceCollectionTest, AddGetResourceWithEmptyCollectionReturnsNotFound) {
  ResourceCollection<fhir::r4::core::ValueSet> collection;
  EXPECT_EQ(collection.Get("http://value.set/id").status().code(),
            absl::StatusCode::kNotFound);
}

TEST(ResourceCollectionTest,
     AddGetResourceWithMissingBundleParseErrorReturnsError) {
  // The bundle contains a resource missing required fields such as
  // resourceType.
  const std::string bundle_contents = R"(
{
  "resourceType": "Bundle",
  "entry": [
    {
      "resource": {
        "url": "http://value.set/id"
      }
    }
  ]
})";
  auto parsed_json = std::make_unique<internal::FhirJson>();
  FHIR_ASSERT_OK(internal::ParseJsonValue(bundle_contents, *parsed_json));

  FHIR_ASSERT_OK_AND_ASSIGN(const internal::FhirJson* entries,
                            parsed_json->get("entry"));
  FHIR_ASSERT_OK_AND_ASSIGN(const internal::FhirJson* entry1, entries->get(0));
  FHIR_ASSERT_OK_AND_ASSIGN(const internal::FhirJson* resource,
                            entry1->get("resource"));

  ResourceCollection<fhir::r4::core::ValueSet> collection;
  FHIR_ASSERT_OK(collection.Put(std::move(parsed_json), *resource));

  EXPECT_FALSE(collection.Get("http://value.set/id").ok());
}

}  // namespace
}  // namespace google::fhir
