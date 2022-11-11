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

#include <stdio.h>

#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/cleanup/cleanup.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/json/json_sax_handler.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "proto/google/fhir/proto/profile_config.pb.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/code_system.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/search_parameter.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/structure_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/value_set.pb.h"
#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"

namespace google::fhir {

namespace {
using ::testing::UnorderedElementsAre;

// File contents provided in CreateArchiveContaining calls.
struct ArchiveContents {
  const std::string name;
  const std::string data;
};

// Writes an archive to a temporary file containing the given {name,
// data} structs. The `set_archive_format` argument determines the type
// of archive written. Pass one of libarchive's functions such as
// `archive_write_set_format_ustar` or `archive_write_set_format_zip` to set the
// type of the archive. Functions CreateZipFileContaining and
// CreateTarFileContaining are provided for convenience.
absl::StatusOr<std::string> CreateArchiveContaining(
    const std::vector<ArchiveContents>& contents,
    const std::function<int(struct archive*)>& set_archive_format) {
  int errorp;
  struct archive* archive = archive_write_new();
  errorp = set_archive_format(archive);
  if (errorp != ARCHIVE_OK) {
    return absl::UnavailableError(absl::StrFormat(
        "Failed to create archive due to error code: %d.", errorp));
  }

  std::string temp_name = std::tmpnam(nullptr);
  errorp = archive_write_open_filename(archive, temp_name.c_str());
  if (errorp != ARCHIVE_OK) {
    return absl::UnavailableError(
        absl::StrFormat("Failed to create archive due to error code: %d %s.",
                        errorp, archive_error_string(archive)));
  }
  absl::Cleanup archive_closer = [&archive] {
    archive_write_close(archive);
    archive_write_free(archive);
  };

  for (const auto& file : contents) {
    struct archive_entry* entry = archive_entry_new();
    archive_entry_set_pathname(entry, file.name.c_str());
    archive_entry_set_size(entry, file.data.length());
    archive_entry_set_filetype(entry, AE_IFREG);
    absl::Cleanup entry_closer = [&entry] { archive_entry_free(entry); };

    errorp = archive_write_header(archive, entry);
    if (errorp != ARCHIVE_OK) {
      return absl::UnavailableError(
          absl::StrFormat("Unable to add file %s to archive, error code: %d %s",
                          file.name, errorp, archive_error_string(archive)));
    }

    int64 written =
        archive_write_data(archive, file.data.c_str(), file.data.length());
    if (written < file.data.length()) {
      return absl::UnavailableError(
          absl::StrFormat("Unable to add file %s to archive, error: %s",
                          file.name, archive_error_string(archive)));
    }
  }

  return temp_name;
}
absl::StatusOr<std::string> CreateZipFileContaining(
    const std::vector<ArchiveContents>& contents) {
  return CreateArchiveContaining(contents, [](struct archive* archive) {
    return archive_write_set_format_zip(archive);
  });
}
absl::StatusOr<std::string> CreateTarFileContaining(
    const std::vector<ArchiveContents>& contents) {
  return CreateArchiveContaining(contents, [](struct archive* archive) {
    return archive_write_set_format_ustar(archive);
  });
}

constexpr int kR4DefinitionsCount = 653;
constexpr int kR4CodeSystemsCount = 1062;
constexpr int kR4ValuesetsCount = 1316;
constexpr int kR4SearchParametersCount = 1385;

TEST(FhirPackageTest, LoadSucceeds) {
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<FhirPackage> fhir_package,
      FhirPackage::Load("spec/fhir_r4_package.tgz"));
  EXPECT_EQ(fhir_package->value_sets.size(), kR4ValuesetsCount);
  EXPECT_EQ(fhir_package->code_systems.size(), kR4CodeSystemsCount);
  EXPECT_EQ(fhir_package->structure_definitions.size(), kR4DefinitionsCount);
  EXPECT_EQ(fhir_package->search_parameters.size(), kR4SearchParametersCount);
}

TEST(FhirPackageTest, LoadAndGetResourceSucceeds) {
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
  FHIR_ASSERT_OK_AND_ASSIGN(const r4::core::StructureDefinition* sd_result,
                            fhir_package->GetStructureDefinition("http://sd1"));
  EXPECT_EQ(sd_result->url().value(), "http://sd1");

  FHIR_ASSERT_OK_AND_ASSIGN(const r4::core::StructureDefinition* sd_result2,
                            fhir_package->GetStructureDefinition("http://sd2"));
  EXPECT_EQ(sd_result2->url().value(), "http://sd2");

  FHIR_ASSERT_OK_AND_ASSIGN(const r4::core::SearchParameter* sp_result,
                            fhir_package->GetSearchParameter("http://sp1"));
  EXPECT_EQ(sp_result->url().value(), "http://sp1");

  FHIR_ASSERT_OK_AND_ASSIGN(const r4::core::SearchParameter* sp_result2,
                            fhir_package->GetSearchParameter("http://sp2"));
  EXPECT_EQ(sp_result2->url().value(), "http://sp2");

  FHIR_ASSERT_OK_AND_ASSIGN(const fhir::r4::core::CodeSystem* cs_result,
                            fhir_package->GetCodeSystem("http://cs1"));
  EXPECT_EQ(cs_result->url().value(), "http://cs1");

  FHIR_ASSERT_OK_AND_ASSIGN(const fhir::r4::core::CodeSystem* cs_result2,
                            fhir_package->GetCodeSystem("http://cs2"));
  EXPECT_EQ(cs_result2->url().value(), "http://cs2");

  FHIR_ASSERT_OK_AND_ASSIGN(const fhir::r4::core::ValueSet* vs_result,
                            fhir_package->GetValueSet("http://vs1"));
  EXPECT_EQ(vs_result->url().value(), "http://vs1");

  FHIR_ASSERT_OK_AND_ASSIGN(const fhir::r4::core::ValueSet* vs_result2,
                            fhir_package->GetValueSet("http://vs2"));
  EXPECT_EQ(vs_result2->url().value(), "http://vs2");
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

TEST(FhirPackageManager, GetResourceForAddedPackagesSucceeds) {
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string temp_name,
      CreateZipFileContaining(
          {// The deprecated package_info is preserved in tests to ensure its
           // presence does not break package loading.
           {"package_info.prototxt", "fhir_version: R4\nproto_package: 'Foo'"},
           {"package_info.textproto", "fhir_version: R4\nproto_package: 'Foo'"},
           {"a_value_set.json",
            R"({"resourceType": "ValueSet", "url": "http://value.set/id-1",
               "id": "a-value-set-1", "status": "draft"})"}}));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string another_temp_name,
      CreateZipFileContaining(
          {{"package_info.prototxt", "fhir_version: R4\nproto_package: 'Foo'"},
           {"package_info.textproto", "fhir_version: R4\nproto_package: 'Foo'"},
           {"a_value_set.json",
            R"({"resourceType": "ValueSet", "url": "http://value.set/id-2",
               "id": "a-value-set-2", "status": "draft"})"}}));
  absl::Cleanup another_temp_closer = [&another_temp_name] {
    remove(another_temp_name.c_str());
  };

  FhirPackageManager package_manager = FhirPackageManager();
  FHIR_ASSERT_OK(package_manager.AddPackageAtPath(temp_name));
  FHIR_ASSERT_OK(package_manager.AddPackageAtPath(another_temp_name));

  FHIR_ASSERT_OK_AND_ASSIGN(
      const fhir::r4::core::ValueSet* result1,
      package_manager.GetValueSet("http://value.set/id-1"))
  EXPECT_EQ(result1->url().value(), "http://value.set/id-1");

  FHIR_ASSERT_OK_AND_ASSIGN(
      const fhir::r4::core::ValueSet* result2,
      package_manager.GetValueSet("http://value.set/id-2"));
  EXPECT_EQ(result2->url().value(), "http://value.set/id-2");

  EXPECT_EQ(package_manager.GetValueSet("http://missing-uri").status().code(),
            absl::StatusCode::kNotFound);
}
// Ensures .tar archives are supported.
TEST(FhirPackageManager, GetResourceForTarPackagesSucceeds) {
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string temp_name,
      CreateTarFileContaining(
          {{"a_value_set.json",
            "{\"resourceType\": \"ValueSet\", \"url\": "
            "\"http://value.set/id-1\", "
            "\"id\": \"a-value-set-1\", \"status\": \"draft\"}"}}));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  FhirPackageManager package_manager = FhirPackageManager();
  FHIR_ASSERT_OK(package_manager.AddPackageAtPath(temp_name));

  FHIR_ASSERT_OK_AND_ASSIGN(
      const fhir::r4::core::ValueSet* result,
      package_manager.GetValueSet("http://value.set/id-1"));
  EXPECT_EQ(result->url().value(), "http://value.set/id-1");

  EXPECT_EQ(package_manager.GetValueSet("http://missing-uri").status().code(),
            absl::StatusCode::kNotFound);
}

TEST(FhirPackageManager, GetResourceAgainstEmptyManagerReturnsNothing) {
  FhirPackageManager package_manager = FhirPackageManager();
  EXPECT_EQ(
      package_manager.GetValueSet("http://value.set/id-1").status().code(),
      absl::StatusCode::kNotFound);
}

TEST(FhirPackageManager, GetResourceWithErrorReturnsError) {
  // The first package is empty and will return a NotFoundError status when
  // queried.
  FHIR_ASSERT_OK_AND_ASSIGN(std::string temp_name,
                            CreateZipFileContaining({{"nothing", "{}"}}));
  absl::Cleanup temp_closer = [&temp_name] { remove(temp_name.c_str()); };

  // The second package contains a resource missing required fields, which will
  // return an InvalidArgumentError status when it fails to be parsed into a
  // proto.
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string another_temp_name,
      CreateZipFileContaining(
          {{"a_value_set.json",
            R"({"resourceType": "ValueSet", "url": "http://value.set/id-1"})"}}));
  absl::Cleanup another_temp_closer = [&another_temp_name] {
    remove(another_temp_name.c_str());
  };

  FhirPackageManager package_manager = FhirPackageManager();
  FHIR_ASSERT_OK(package_manager.AddPackageAtPath(temp_name));
  FHIR_ASSERT_OK(package_manager.AddPackageAtPath(another_temp_name));

  EXPECT_EQ(
      package_manager.GetValueSet("http://value.set/id-1").status().code(),
      absl::StatusCode::kInvalidArgument);
}

TEST(ResourceCollectionTest, GetResourceFromCacheSucceeds) {
  auto parsed_json = std::make_unique<internal::FhirJson>();
  FHIR_ASSERT_OK(internal::ParseJsonValue(
      R"({"resourceType": "ValueSet", "url": "http://value.set/id",
                      "id": "hello", "status": "draft"})",
      *parsed_json));

  ResourceCollection<r4::core::ValueSet> collection;
  FHIR_ASSERT_OK(collection.Put(std::move(parsed_json)));

  // Call Get once to cache the resource.
  FHIR_ASSERT_OK_AND_ASSIGN(const fhir::r4::core::ValueSet* result_uncached,
                            collection.Get("http://value.set/id"));
  EXPECT_EQ(result_uncached->id().value(), "hello");

  // Calling Get again should retrieve the cached resource.
  FHIR_ASSERT_OK_AND_ASSIGN(const fhir::r4::core::ValueSet* result_cached,
                            collection.Get("http://value.set/id"));
  EXPECT_EQ(result_cached, result_uncached);
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
  FHIR_ASSERT_OK_AND_ASSIGN(const fhir::r4::core::ValueSet* result1,
                            collection.Get("http://value.set/id1"));
  EXPECT_EQ(result1->id().value(), "hello");

  // Insert and retrieve vs2 to have it cached.
  FHIR_ASSERT_OK(collection.Put(std::move(vs2)));
  EXPECT_TRUE(collection.Get("http://value.set/id2").ok());

  // Retrieve vs2 from the cache.
  FHIR_ASSERT_OK_AND_ASSIGN(const fhir::r4::core::ValueSet* result2,
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

  FHIR_ASSERT_OK_AND_ASSIGN(const fhir::r4::core::ValueSet* result,
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
  for (const fhir::r4::core::ValueSet& value_set : collection) {
    found.push_back(value_set.url().value());
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
  for (const fhir::r4::core::ValueSet& value_set : collection) {
    found.push_back(value_set.url().value());
  }
  EXPECT_THAT(found, UnorderedElementsAre("http://value.set/id-1",
                                          "http://value.set/id-2",
                                          "http://value.set/id-3"));
}

TEST(ResourceCollectionTest, WithNoResourcesIterateEmpty) {
  ResourceCollection<fhir::r4::core::ValueSet> collection;

  std::vector<fhir::r4::core::ValueSet> found;
  for (const fhir::r4::core::ValueSet& value_set : collection) {
    found.push_back(value_set);
  }
  EXPECT_EQ(found.size(), 0);
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

  std::vector<fhir::r4::core::ValueSet> found;
  for (const fhir::r4::core::ValueSet& value_set : collection) {
    found.push_back(value_set);
  }
  EXPECT_EQ(found.size(), 0);
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
  for (const fhir::r4::core::ValueSet& value_set : collection) {
    found.push_back(value_set.url().value());
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
