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
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "proto/google/fhir/proto/profile_config.pb.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/code_system.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/search_parameter.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/structure_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/value_set.pb.h"
#include "lib/zip.h"

namespace google::fhir {

namespace {
using ::testing::UnorderedElementsAreArray;

// Writes a zip archive to a temporary file containing the given {file_name,
// file_contents} pairs.
absl::StatusOr<std::string> CreateZipFileContaining(
    std::vector<std::pair<const char*, const char*>> zip_contents) {
  std::string temp_name = std::tmpnam(nullptr);

  int errorp;
  zip_t* zip_file = zip_open(temp_name.c_str(), ZIP_CREATE, &errorp);
  if (zip_file == nullptr) {
    return absl::UnavailableError(absl::StrFormat(
        "Failed to create zip file due to error code: %d", errorp));
  }

  for (auto& pair : zip_contents) {
    zip_source_t* source =
        zip_source_buffer(zip_file, pair.second, strlen(pair.second), 0);
    zip_int64_t add_index = zip_file_add(zip_file, pair.first, source, 0);
    if (add_index < 0) {
      return absl::UnavailableError(absl::StrFormat(
          "Unable to add file to zip, error code: %s", zip_strerror(zip_file)));
    }
  }
  zip_close(zip_file);

  return temp_name;
}

constexpr int kR4DefinitionsCount = 653;
constexpr int kR4CodeSystemsCount = 1062;
constexpr int kR4ValuesetsCount = 1316;
constexpr int kR4SearchParametersCount = 1385;

TEST(FhirPackageTest, LoadSucceeds) {
  absl::StatusOr<std::unique_ptr<FhirPackage>> fhir_package =
      FhirPackage::Load("spec/fhir_r4_package.zip");
  ASSERT_TRUE(fhir_package.ok()) << fhir_package.status().message();

  EXPECT_EQ((*fhir_package)->value_sets.size(), kR4ValuesetsCount);
  EXPECT_EQ((*fhir_package)->code_systems.size(), kR4CodeSystemsCount);
  EXPECT_EQ((*fhir_package)->structure_definitions.size(), kR4DefinitionsCount);
  EXPECT_EQ((*fhir_package)->search_parameters.size(),
            kR4SearchParametersCount);
}

TEST(FhirPackageTest, LoadAndGetResourceSucceeds) {
  // Define a bunch of fake resources.
  const char* structure_definition_1 = R"({
    "resourceType": "StructureDefinition",
    "url": "http://sd1",
    "name": "sd1",
    "kind": "complex-type",
    "abstract": false,
    "type": "Extension",
    "status": "draft"
  })";
  const char* structure_definition_2 = R"({
    "resourceType": "StructureDefinition",
    "url": "http://sd2",
    "name": "sd2",
    "kind": "complex-type",
    "abstract": false,
    "type": "Extension",
    "status": "draft"
  })";
  const char* search_parameter_1 = R"({
    "resourceType": "SearchParameter",
    "url": "http://sp1",
    "name": "sp1",
    "status": "draft",
    "description": "sp1",
    "code": "facility",
    "base": ["Claim"],
    "type": "reference"
  })";
  const char* search_parameter_2 = R"({
    "resourceType": "SearchParameter",
    "url": "http://sp2",
    "name": "sp2",
    "status": "draft",
    "description": "sp2",
    "code": "facility",
    "base": ["Claim"],
    "type": "reference"
  })";
  const char* code_system_1 = R"({
    "resourceType": "CodeSystem",
    "url": "http://cs1",
    "name": "cs1",
    "status": "draft",
    "content": "complete"
  })";
  const char* code_system_2 = R"({
    "resourceType": "CodeSystem",
    "url": "http://cs2",
    "name": "cs2",
    "status": "draft",
    "content": "complete"
  })";
  const char* value_set_1 = R"({
    "resourceType": "ValueSet",
    "url": "http://vs1",
    "name": "vs1",
    "status": "draft"
  })";
  const char* value_set_2 = R"({
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
  absl::StatusOr<std::string> temp_name =
      CreateZipFileContaining(std::vector<std::pair<const char*, const char*>>{
          {"sd1.json", structure_definition_1},
          {"sp1.json", search_parameter_1},
          {"cs1.json", code_system_1},
          {"vs1.json", value_set_1},
          {"bundle.json", bundle.c_str()},
      });
  ASSERT_TRUE(temp_name.ok()) << temp_name.status().message();

  absl::StatusOr<std::unique_ptr<FhirPackage>> fhir_package =
      FhirPackage::Load(*temp_name);
  ASSERT_TRUE(fhir_package.ok()) << fhir_package.status().message();

  // Check that we can retrieve all our resources;
  absl::StatusOr<const google::fhir::r4::core::StructureDefinition*> sd_result =
      (*fhir_package)->GetStructureDefinition("http://sd1");
  EXPECT_TRUE(sd_result.ok());
  EXPECT_EQ((*sd_result)->url().value(), "http://sd1");

  sd_result = (*fhir_package)->GetStructureDefinition("http://sd2");
  EXPECT_TRUE(sd_result.ok());
  EXPECT_EQ((*sd_result)->url().value(), "http://sd2");

  absl::StatusOr<const google::fhir::r4::core::SearchParameter*> sp_result =
      (*fhir_package)->GetSearchParameter("http://sp1");
  EXPECT_TRUE(sp_result.ok());
  EXPECT_EQ((*sp_result)->url().value(), "http://sp1");

  sp_result = (*fhir_package)->GetSearchParameter("http://sp2");
  EXPECT_TRUE(sp_result.ok());
  EXPECT_EQ((*sp_result)->url().value(), "http://sp2");

  absl::StatusOr<const google::fhir::r4::core::CodeSystem*> cs_result =
      (*fhir_package)->GetCodeSystem("http://cs1");
  EXPECT_TRUE(cs_result.ok());
  EXPECT_EQ((*cs_result)->url().value(), "http://cs1");

  cs_result = (*fhir_package)->GetCodeSystem("http://cs2");
  EXPECT_TRUE(cs_result.ok());
  EXPECT_EQ((*cs_result)->url().value(), "http://cs2");

  absl::StatusOr<const google::fhir::r4::core::ValueSet*> vs_result =
      (*fhir_package)->GetValueSet("http://vs1");
  EXPECT_TRUE(vs_result.ok());
  EXPECT_EQ((*vs_result)->url().value(), "http://vs1");

  vs_result = (*fhir_package)->GetValueSet("http://vs2");
  EXPECT_TRUE(vs_result.ok());
  EXPECT_EQ((*vs_result)->url().value(), "http://vs2");

  remove(temp_name->c_str());
}

TEST(FhirPackageTest, ResourceWithParseErrorFails) {
  // Define a malformed resource to test parse failures.
  const char* malformed_struct_def = R"({
    "resourceType": "StructureDefinition",
    "url": "http://malformed_test",
    "name": "malformed_json_without_closing_quote,
    "kind": "complex-type",
    "abstract": false,
    "type": "Extension",
    "status": "draft"
  })";

  // Put those resources in a FhirPackage.
  absl::StatusOr<std::string> temp_name =
      CreateZipFileContaining(std::vector<std::pair<const char*, const char*>>{
          {"malformed_struct_def.json", malformed_struct_def},
      });
  ASSERT_TRUE(temp_name.ok()) << temp_name.status().message();

  absl::StatusOr<std::unique_ptr<FhirPackage>> fhir_package =
      FhirPackage::Load(*temp_name);
  ASSERT_FALSE(fhir_package.ok());
  remove(temp_name->c_str());
}

TEST(FhirPackageTest, GetResourceForMissingUriFindsNothing) {
  absl::StatusOr<std::string> temp_name =
      CreateZipFileContaining(std::vector<std::pair<const char*, const char*>>{
          {"a_value_set.json",
           "{\"resourceType\": \"ValueSet\", \"url\": \"http://value.set/id\", "
           "\"id\": \"a-value-set\", \"status\": \"draft\"}"}});
  ASSERT_TRUE(temp_name.ok()) << temp_name.status().message();

  absl::StatusOr<std::unique_ptr<FhirPackage>> fhir_package =
      FhirPackage::Load(*temp_name);
  ASSERT_TRUE(fhir_package.ok()) << fhir_package.status().message();

  EXPECT_FALSE((*fhir_package)->GetValueSet("missing").ok());
  remove(temp_name->c_str());
}

TEST(FhirPackageManager, GetResourceForAddedPackagesSucceeds) {
  absl::StatusOr<std::string> temp_name =
      CreateZipFileContaining(std::vector<std::pair<const char*, const char*>>{
          // The deprecated package_info is preserved in tests to ensure its
          // presence does not break package loading.
          {"package_info.prototxt", "fhir_version: R4\nproto_package: 'Foo'"},
          {"a_value_set.json",
           "{\"resourceType\": \"ValueSet\", \"url\": "
           "\"http://value.set/id-1\", "
           "\"id\": \"a-value-set-1\", \"status\": \"draft\"}"}});
  ASSERT_TRUE(temp_name.ok()) << temp_name.status().message();

  absl::StatusOr<std::string> another_temp_name =
      CreateZipFileContaining(std::vector<std::pair<const char*, const char*>>{
          {"package_info.prototxt", "fhir_version: R4\nproto_package: 'Foo'"},
          {"a_value_set.json",
           "{\"resourceType\": \"ValueSet\", \"url\": "
           "\"http://value.set/id-2\", "
           "\"id\": \"a-value-set-2\", \"status\": \"draft\"}"}});
  ASSERT_TRUE(another_temp_name.ok()) << another_temp_name.status().message();

  absl::Status add_package_status;
  FhirPackageManager package_manager = FhirPackageManager();

  add_package_status = package_manager.AddPackageAtPath(*temp_name);
  ASSERT_TRUE(add_package_status.ok()) << add_package_status.message();

  add_package_status = package_manager.AddPackageAtPath(*another_temp_name);
  ASSERT_TRUE(add_package_status.ok()) << add_package_status.message();

  absl::StatusOr<const google::fhir::r4::core::ValueSet*> result;

  result = package_manager.GetValueSet("http://value.set/id-1");
  EXPECT_TRUE(result.ok());
  EXPECT_EQ((*result)->url().value(), "http://value.set/id-1");

  result = package_manager.GetValueSet("http://value.set/id-2");
  EXPECT_TRUE(result.ok());
  EXPECT_EQ((*result)->url().value(), "http://value.set/id-2");

  result = package_manager.GetValueSet("http://missing-uri");
  EXPECT_FALSE(result.ok());

  remove(temp_name->c_str());
  remove(another_temp_name->c_str());
}

TEST(FhirPackageManager, GetResourceAgainstEmptyManagerReturnsNothing) {
  FhirPackageManager package_manager = FhirPackageManager();

  absl::StatusOr<const google::fhir::r4::core::ValueSet*> result =
      package_manager.GetValueSet("http://value.set/id-1");
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), absl::StatusCode::kNotFound);
}

TEST(FhirPackageManager, GetResourceWithErrorReturnsError) {
  // The first package is empty and will return a NotFoundError status when
  // queried.
  absl::StatusOr<std::string> temp_name = CreateZipFileContaining(
      std::vector<std::pair<const char*, const char*>>{{"nothing", "{}"}});
  ASSERT_TRUE(temp_name.ok()) << temp_name.status().message();

  // The second package contains a resource missing required fields, which will
  // return an InvalidArgumentError status when it fails to be parsed into a
  // proto.
  absl::StatusOr<std::string> another_temp_name =
      CreateZipFileContaining(std::vector<std::pair<const char*, const char*>>{
          {"a_value_set.json",
           "{\"resourceType\": \"ValueSet\", \"url\": "
           "\"http://value.set/id-1\" } "}});
  ASSERT_TRUE(another_temp_name.ok()) << another_temp_name.status().message();

  absl::Status add_package_status;
  FhirPackageManager package_manager = FhirPackageManager();

  add_package_status = package_manager.AddPackageAtPath(*temp_name);
  ASSERT_TRUE(add_package_status.ok()) << add_package_status.message();

  add_package_status = package_manager.AddPackageAtPath(*another_temp_name);
  ASSERT_TRUE(add_package_status.ok()) << add_package_status.message();

  absl::StatusOr<const google::fhir::r4::core::ValueSet*> result =
      package_manager.GetValueSet("http://value.set/id-1");
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), absl::StatusCode::kInvalidArgument);

  remove(temp_name->c_str());
  remove(another_temp_name->c_str());
}

TEST(ResourceCollectionTest, GetResourceFromCacheSucceeds) {
  auto collection =
      ResourceCollection<google::fhir::r4::core::ValueSet>("package.zip");
  auto vs = std::make_unique<google::fhir::r4::core::ValueSet>();
  vs->mutable_id()->set_value("hello");
  collection.CacheParsedResource("http://value.set/id", std::move(vs));

  absl::StatusOr<const google::fhir::r4::core::ValueSet*> result =
      collection.GetResource("http://value.set/id");
  ASSERT_TRUE(result.ok()) << result.status().message();
  EXPECT_EQ((*result)->id().value(), "hello");
}

TEST(ResourceCollectionTest, GetResourceFromCacheHasPointerStability) {
  auto collection =
      ResourceCollection<google::fhir::r4::core::ValueSet>("package.zip");
  auto vs1 = std::make_unique<google::fhir::r4::core::ValueSet>();
  vs1->mutable_id()->set_value("hello");

  auto vs2 = std::make_unique<google::fhir::r4::core::ValueSet>();
  vs2->mutable_id()->set_value("goodbye");

  collection.CacheParsedResource("http://value.set/id1", std::move(vs1));
  absl::StatusOr<const google::fhir::r4::core::ValueSet*> result1 =
      collection.GetResource("http://value.set/id1");
  ASSERT_TRUE(result1.ok()) << result1.status().message();
  EXPECT_EQ((*result1)->id().value(), "hello");

  collection.CacheParsedResource("http://value.set/id2", std::move(vs2));
  absl::StatusOr<const google::fhir::r4::core::ValueSet*> result2 =
      collection.GetResource("http://value.set/id2");
  ASSERT_TRUE(result2.ok()) << result2.status().message();
  EXPECT_EQ((*result2)->id().value(), "goodbye");

  // Ensure the vs1 pointer still works.
  EXPECT_EQ((*result1)->id().value(), "hello");
}

TEST(ResourceCollectionTest, AddGetResourceSucceeds) {
  absl::StatusOr<std::string> temp_name =
      CreateZipFileContaining(std::vector<std::pair<const char*, const char*>>{
          {"a_value_set.json",
           "{\"resourceType\": \"ValueSet\", \"url\": \"http://value.set/id\", "
           "\"id\": \"a-value-set\", \"status\": \"draft\"}"}});
  ASSERT_TRUE(temp_name.ok()) << temp_name.status().message();

  auto collection =
      ResourceCollection<google::fhir::r4::core::ValueSet>(*temp_name);
  collection.AddUriAtPath("http://value.set/id", "a_value_set.json");
  absl::StatusOr<const google::fhir::r4::core::ValueSet*> result =
      collection.GetResource("http://value.set/id");

  remove(temp_name->c_str());
  ASSERT_TRUE(result.ok()) << result.status().message();
  EXPECT_EQ((*result)->id().value(), "a-value-set");
  EXPECT_EQ((*result)->url().value(), "http://value.set/id");
}

TEST(ResourceCollectionTest, AddGetResourceInBundleSucceeds) {
  const char* bundle_contents = R"(
{
  "resourceType": "Bundle",
  "entry": [
    {
      "resource": {
        "resourceType": "Bundle",
        "entry": [
          {
            "resource": {
              "resourceType": "ValueSet",
              "id": "a-different-value-set",
              "url": "http://different-value.set/id",
              "status": "draft"
            }
          },
          {
            "resource": {
              "resourceType": "ValueSet",
              "id": "a-value-set",
              "url": "http://value.set/id",
              "status": "draft"
            }
          }
        ]
      }
    }
  ]
})";
  absl::StatusOr<std::string> temp_name =
      CreateZipFileContaining(std::vector<std::pair<const char*, const char*>>{
          {"a_bundle.json", bundle_contents}});

  ASSERT_TRUE(temp_name.ok()) << temp_name.status().message();

  auto collection =
      ResourceCollection<google::fhir::r4::core::ValueSet>(*temp_name);
  collection.AddUriAtPath("http://value.set/id", "a_bundle.json");
  absl::StatusOr<const google::fhir::r4::core::ValueSet*> result =
      collection.GetResource("http://value.set/id");

  remove(temp_name->c_str());
  ASSERT_TRUE(result.ok()) << result.status().message();
  EXPECT_EQ((*result)->id().value(), "a-value-set");
  EXPECT_EQ((*result)->url().value(), "http://value.set/id");
}

TEST(ResourceCollectionTest, WithValidResourcesIterateSucceeds) {
  absl::StatusOr<std::string> temp_name =
      CreateZipFileContaining(std::vector<std::pair<const char*, const char*>>{
          {"a_value_set.json",
           "{\"resourceType\": \"ValueSet\", \"url\": "
           "\"http://value.set/id-1\", "
           "\"id\": \"a-value-set-1\", \"status\": \"draft\"}"},
          {"another_value_set.json",
           "{\"resourceType\": \"ValueSet\", \"url\": "
           "\"http://value.set/id-2\", "
           "\"id\": \"a-value-set-2\", \"status\": \"draft\"}"},
          {"yet_another_value_set.json",
           "{\"resourceType\": \"ValueSet\", \"url\": "
           "\"http://value.set/id-3\", "
           "\"id\": \"a-value-set-3\", \"status\": \"draft\"}"},
      });
  ASSERT_TRUE(temp_name.ok()) << temp_name.status().message();

  auto collection =
      ResourceCollection<google::fhir::r4::core::ValueSet>(*temp_name);
  for (auto& uri_path : std::vector<std::pair<std::string, std::string>>{
           {"http://value.set/id-1", "a_value_set.json"},
           {"http://value.set/id-2", "another_value_set.json"},
           {"http://value.set/id-3", "yet_another_value_set.json"}}) {
    collection.AddUriAtPath(uri_path.first, uri_path.second);
  }

  std::vector<std::string> found;
  for (const google::fhir::r4::core::ValueSet& value_set : collection) {
    found.emplace_back(value_set.url().value());
  }
  EXPECT_THAT(found, UnorderedElementsAreArray(std::vector<std::string>{
                         "http://value.set/id-1",
                         "http://value.set/id-2",
                         "http://value.set/id-3",
                     }));
  remove(temp_name->c_str());
}

TEST(ResourceCollectionTest, WithNoResourcesIterateEmpty) {
  absl::StatusOr<std::string> temp_name = CreateZipFileContaining(
      std::vector<std::pair<const char*, const char*>>{});
  ASSERT_TRUE(temp_name.ok()) << temp_name.status().message();

  auto collection =
      ResourceCollection<google::fhir::r4::core::ValueSet>(*temp_name);

  std::vector<google::fhir::r4::core::ValueSet> found;
  for (const google::fhir::r4::core::ValueSet& value_set : collection) {
    found.emplace_back(value_set);
  }
  EXPECT_EQ(found.size(), 0);
  remove(temp_name->c_str());
}

TEST(ResourceCollectionTest, WithInvalidResourcesIterateEmpty) {
  absl::StatusOr<std::string> temp_name =
      CreateZipFileContaining(std::vector<std::pair<const char*, const char*>>{
          {"a_value_set.json", "{\"resourceType\": \"invalid"},
          {"another_value_set.json", "{\"resourceType\": \"invalid"},
          {"yet_another_value_set.json", "{\"resourceType\": \"invalid"},
      });
  ASSERT_TRUE(temp_name.ok()) << temp_name.status().message();

  auto collection =
      ResourceCollection<google::fhir::r4::core::ValueSet>(*temp_name);
  for (auto& uri_path : std::vector<std::pair<std::string, std::string>>{
           {"http://value.set/id-1", "a_value_set.json"},
           {"http://value.set/id-2", "another_value_set.json"},
           {"http://value.set/id-3", "yet_another_value_set.json"}}) {
    collection.AddUriAtPath(uri_path.first, uri_path.second);
  }

  std::vector<google::fhir::r4::core::ValueSet> found;
  for (const google::fhir::r4::core::ValueSet& value_set : collection) {
    found.emplace_back(value_set);
  }
  EXPECT_EQ(found.size(), 0);
  remove(temp_name->c_str());
}

TEST(ResourceCollectionTest, WithValidAndInvalidResourcesIterateSkipsInvalid) {
  absl::StatusOr<std::string> temp_name =
      CreateZipFileContaining(std::vector<std::pair<const char*, const char*>>{
          {"a_bad_value_set.json", "{\"resourceType\": \"invalid"},
          {"another_bad_value_set.json", "{\"resourceType\": \"invalid"},
          {"yet_bad_another_value_set.json", "{\"resourceType\": \"invalid"},
          {"a_value_set.json",
           "{\"resourceType\": \"ValueSet\", \"url\": "
           "\"http://value.set/id-1\", "
           "\"id\": \"a-value-set-1\", \"status\": \"draft\"}"},
          {"another_value_set.json",
           "{\"resourceType\": \"ValueSet\", \"url\": "
           "\"http://value.set/id-2\", "
           "\"id\": \"a-value-set-2\", \"status\": \"draft\"}"},
      });
  ASSERT_TRUE(temp_name.ok()) << temp_name.status().message();

  auto collection =
      ResourceCollection<google::fhir::r4::core::ValueSet>(*temp_name);
  for (auto& uri_path : std::vector<std::pair<std::string, std::string>>{
           {"http://value.set/id-bad-1", "a_bad_value_set.json"},
           {"http://value.set/id-1", "a_value_set.json"},
           {"http://value.set/id-bad-2", "another_bad_value_set.json"},
           {"http://value.set/id-2", "another_value_set.json"},
           {"http://value.set/id-bad-3", "yet_bad_another_value_set.json"},
       }) {
    collection.AddUriAtPath(uri_path.first, uri_path.second);
  }

  std::vector<std::string> found;
  for (const google::fhir::r4::core::ValueSet& value_set : collection) {
    found.emplace_back(value_set.url().value());
  }
  EXPECT_THAT(found, UnorderedElementsAreArray(std::vector<std::string>{
                         "http://value.set/id-1",
                         "http://value.set/id-2",
                     }));
  remove(temp_name->c_str());
}

TEST(ResourceCollectionTest, AddGetResourceWithUriMappingFails) {
  auto collection =
      ResourceCollection<google::fhir::r4::core::ValueSet>("missing.zip");
  absl::StatusOr<const google::fhir::r4::core::ValueSet*> result =
      collection.GetResource("http://value.set/id");

  ASSERT_FALSE(result.ok());
}

TEST(ResourceCollectionTest, AddGetResourceWithNoZipFails) {
  auto collection =
      ResourceCollection<google::fhir::r4::core::ValueSet>("missing.zip");
  collection.AddUriAtPath("http://value.set/id", "a_value_set.json");
  absl::StatusOr<const google::fhir::r4::core::ValueSet*> result =
      collection.GetResource("http://value.set/id");

  ASSERT_FALSE(result.ok());
}

TEST(ResourceCollectionTest, AddGetResourceWithZipMissingEntryFails) {
  absl::StatusOr<std::string> temp_name = CreateZipFileContaining(
      std::vector<std::pair<const char*, const char*>>{});
  ASSERT_TRUE(temp_name.ok()) << temp_name.status().message();

  auto collection =
      ResourceCollection<google::fhir::r4::core::ValueSet>(*temp_name);
  collection.AddUriAtPath("http://value.set/id", "a_value_set.json");
  absl::StatusOr<const google::fhir::r4::core::ValueSet*> result =
      collection.GetResource("http://value.set/id");

  remove(temp_name->c_str());
  ASSERT_FALSE(result.ok());
}

TEST(ResourceCollectionTest, AddGetResourceWithBadResourceJsonFails) {
  absl::StatusOr<std::string> temp_name =
      CreateZipFileContaining(std::vector<std::pair<const char*, const char*>>{
          {"a_value_set.json", "bad_json"}});
  ASSERT_TRUE(temp_name.ok()) << temp_name.status().message();

  auto collection =
      ResourceCollection<google::fhir::r4::core::ValueSet>(*temp_name);
  collection.AddUriAtPath("http://value.set/id", "a_value_set.json");
  absl::StatusOr<const google::fhir::r4::core::ValueSet*> result =
      collection.GetResource("http://value.set/id");

  remove(temp_name->c_str());
  ASSERT_FALSE(result.ok());
}

TEST(ResourceCollectionTest, AddGetResourceWithMissingBundleEntryFails) {
  const char* bundle_contents = R"(
{
  "resourceType": "Bundle",
  "entry": [
    {
      "resource": {
        "resourceType": "ValueSet",
        "id": "a-different-value-set",
        "url": "http://different-value.set/id",
        "status": "draft"
      }
    }
  ]
})";
  absl::StatusOr<std::string> temp_name =
      CreateZipFileContaining(std::vector<std::pair<const char*, const char*>>{
          {"a_bundle.json", bundle_contents}});

  ASSERT_TRUE(temp_name.ok()) << temp_name.status().message();

  auto collection =
      ResourceCollection<google::fhir::r4::core::ValueSet>(*temp_name);
  collection.AddUriAtPath("http://value.set/id", "a_bundle.json");
  absl::StatusOr<const google::fhir::r4::core::ValueSet*> result =
      collection.GetResource("http://value.set/id");

  remove(temp_name->c_str());
  ASSERT_FALSE(result.ok());
}

}  // namespace

}  // namespace google::fhir
