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

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "proto/google/fhir/proto/profile_config.pb.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
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
  absl::StatusOr<FhirPackage> fhir_package =
      FhirPackage::Load("spec/fhir_r4_package.zip");
  ASSERT_TRUE(fhir_package.ok()) << fhir_package.status().message();

  EXPECT_EQ(fhir_package->package_info.proto_package(), "google.fhir.r4.core");
  EXPECT_EQ(fhir_package->value_sets.size(), kR4ValuesetsCount);
  EXPECT_EQ(fhir_package->code_systems.size(), kR4CodeSystemsCount);
  EXPECT_EQ(fhir_package->structure_definitions.size(), kR4DefinitionsCount);
  EXPECT_EQ(fhir_package->search_parameters.size(), kR4SearchParametersCount);
}

TEST(FhirPackageTest, LoadWithSideloadedPackageInfoSucceeds) {
  proto::PackageInfo package_info;
  package_info.set_proto_package("my.custom.package");
  package_info.set_fhir_version(proto::FhirVersion::R4);

  absl::StatusOr<FhirPackage> fhir_package = FhirPackage::Load(
      "spec/fhir_r4_package.zip", package_info);
  ASSERT_TRUE(fhir_package.ok()) << fhir_package.status().message();

  EXPECT_EQ(fhir_package->package_info.proto_package(), "my.custom.package");
  EXPECT_EQ(fhir_package->value_sets.size(), kR4ValuesetsCount);
  EXPECT_EQ(fhir_package->code_systems.size(), kR4CodeSystemsCount);
  EXPECT_EQ(fhir_package->structure_definitions.size(), kR4DefinitionsCount);
  EXPECT_EQ(fhir_package->search_parameters.size(), kR4SearchParametersCount);
}

TEST(ResourceCollectionTest, GetResourceFromCacheSucceeds) {
  auto collection =
      ResourceCollection<google::fhir::r4::core::ValueSet>("package.zip");
  auto vs = google::fhir::r4::core::ValueSet();
  vs.mutable_id()->set_value("hello");
  collection.CacheParsedResource("http://value.set/id", vs);

  absl::StatusOr<const google::fhir::r4::core::ValueSet*> result =
      collection.GetResource("http://value.set/id");
  ASSERT_TRUE(result.ok()) << result.status().message();
  EXPECT_EQ((*result)->id().value(), "hello");
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
  "url": "http://bundles.of-fun/id",
  "entry": [
    {
      "resource": {
        "resourceType": "Bundle",
        "url": "http://bundles.nested/id",
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
  "url": "http://bundles.of-fun/id",
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
