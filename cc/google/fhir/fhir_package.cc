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

#include <optional>
#include <string>

#include "google/protobuf/text_format.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "absl/types/optional.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/json/json_sax_handler.h"
#include "google/fhir/r4/json_format.h"
#include "google/fhir/status/status.h"
#include "proto/google/fhir/proto/profile_config.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/structure_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/value_set.pb.h"
#include "lib/zip.h"
#include "re2/re2.h"

namespace google::fhir {

namespace {

using ::google::fhir::proto::PackageInfo;
using ::google::fhir::r4::core::CodeSystem;
using ::google::fhir::r4::core::SearchParameter;
using ::google::fhir::r4::core::StructureDefinition;
using ::google::fhir::r4::core::ValueSet;

absl::StatusOr<std::string> GetResourceType(const std::string& json) {
  static LazyRE2 kResourceTypeRegex = {
      R"regex("resourceType"\s*:\s*"([A-Za-z]*)")regex"};
  std::string resource_type;
  if (RE2::PartialMatch(json, *kResourceTypeRegex, &resource_type)) {
    return resource_type;
  }
  return absl::NotFoundError("No resourceType field in JSON.");
}

}  // namespace

namespace internal {
absl::StatusOr<const FhirJson*> FindResourceInBundle(
    absl::string_view uri, const FhirJson& bundle_json) {
  absl::Status not_found =
      absl::NotFoundError(absl::StrFormat("%s not present in bundle.", uri));

  FHIR_ASSIGN_OR_RETURN(const FhirJson* entries, bundle_json.get("entry"));
  FHIR_ASSIGN_OR_RETURN(int num_entries, entries->arraySize());
  for (int i = 0; i < num_entries; ++i) {
    FHIR_ASSIGN_OR_RETURN(const FhirJson* entry, entries->get(i));
    FHIR_ASSIGN_OR_RETURN(const FhirJson* resource, entry->get("resource"));
    FHIR_ASSIGN_OR_RETURN(std::string resource_url,
                          internal::GetResourceUrl(*resource));
    // Found the resource!
    if (uri == resource_url) {
      return resource;
    }
    // If the resource is a bundle, recursively search through it.
    FHIR_ASSIGN_OR_RETURN(std::string resource_type,
                          internal::GetResourceType(*resource));
    if (resource_type == "Bundle") {
      absl::StatusOr<const FhirJson*> bundle_json =
          FindResourceInBundle(uri, *resource);
      if (bundle_json.ok()) {
        return bundle_json;
      } else {
        not_found.Update(bundle_json.status());
      }
    }
  }
  return not_found;
}

absl::Status ParseResourceFromZip(absl::string_view zip_file_path,
                                  absl::string_view resource_path,
                                  FhirJson& json_value) {
  int zip_open_error;
  zip_t* zip_file =
      zip_open(std::string(zip_file_path).c_str(), ZIP_RDONLY, &zip_open_error);
  if (zip_file == nullptr) {
    return absl::NotFoundError(
        absl::StrFormat("Unable to open zip: %s. Error code: %d", zip_file_path,
                        zip_open_error));
  }

  zip_stat_t resource_file_stat;
  int zip_stat_error = zip_stat(zip_file, std::string(resource_path).c_str(),
                                ZIP_STAT_SIZE, &resource_file_stat);
  if (zip_stat_error != 0) {
    return absl::NotFoundError(
        absl::StrFormat("Unable to stat path %s in zip file %s. The resource "
                        ".zip file may have changed on disk.",
                        resource_path, zip_file_path));
  }

  zip_file_t* resource_file =
      zip_fopen(zip_file, std::string(resource_path).c_str(), 0);
  if (resource_file == nullptr) {
    return absl::NotFoundError(
        absl::StrFormat("Unable to find path %s in zip file %s. The resource "
                        ".zip file may have changed on disk.",
                        resource_path, zip_file_path));
  }

  std::string raw_json(resource_file_stat.size, '\0');
  zip_int64_t read =
      zip_fread(resource_file, &raw_json[0], resource_file_stat.size);

  zip_fclose(resource_file);
  zip_close(zip_file);

  if (read < resource_file_stat.size) {
    return absl::NotFoundError(
        absl::StrFormat("Unable to read path %s in zip file %s.", resource_path,
                        zip_file_path));
  }

  return internal::ParseJsonValue(raw_json, json_value);
}

absl::StatusOr<std::string> GetResourceType(const FhirJson& parsed_json) {
  FHIR_ASSIGN_OR_RETURN(const internal::FhirJson* resource_type_json,
                        parsed_json.get("resourceType"));
  return resource_type_json->asString();
}

absl::StatusOr<std::string> GetResourceUrl(const FhirJson& parsed_json) {
  FHIR_ASSIGN_OR_RETURN(const internal::FhirJson* resource_url_json,
                        parsed_json.get("url"));
  return resource_url_json->asString();
}
}  // namespace internal

absl::StatusOr<FhirPackage> FhirPackage::Load(absl::string_view zip_file_path) {
  return Load(zip_file_path, std::optional<PackageInfo>());
}

absl::StatusOr<FhirPackage> FhirPackage::Load(absl::string_view zip_file_path,
                                              const PackageInfo& package_info) {
  return Load(zip_file_path, std::optional<PackageInfo>(package_info));
}

absl::StatusOr<FhirPackage> FhirPackage::Load(
    absl::string_view zip_file_path,
    const absl::optional<PackageInfo> optional_package_info) {
  int zip_open_error;
  zip_t* archive =
      zip_open(std::string(zip_file_path).c_str(), ZIP_RDONLY, &zip_open_error);
  if (archive == nullptr) {
    return absl::InvalidArgumentError(
        absl::StrFormat("Unable to open zip: %s error code: %d", zip_file_path,
                        zip_open_error));
  }

  FhirPackage fhir_package;
  if (optional_package_info.has_value()) {
    fhir_package.package_info = *optional_package_info;
  }
  bool package_info_found = false;
  absl::flat_hash_map<const std::string, const std::string> json_files;

  zip_file_t* entry;
  zip_stat_t entry_stat;
  zip_int64_t num_entries = zip_get_num_entries(archive, 0);
  for (zip_int64_t i = 0; i < num_entries; ++i) {
    if (zip_stat_index(archive, i, ZIP_STAT_NAME | ZIP_STAT_SIZE,
                       &entry_stat) != 0) {
      return absl::InvalidArgumentError(
          absl::StrFormat("Unable to stat entry %d from zip: %s, error: %s", i,
                          zip_file_path, zip_strerror(archive)));
    }
    std::string entry_name = entry_stat.name;

    entry = zip_fopen_index(archive, i, 0);
    if (entry == nullptr) {
      return absl::InvalidArgumentError(
          absl::StrFormat("Unable to read entry %d from zip: %s, error: %s", i,
                          zip_file_path, zip_strerror(archive)));
    }

    std::string contents(entry_stat.size, '\0');
    zip_int64_t read = zip_fread(entry, &contents[0], entry_stat.size);
    zip_fclose(entry);
    if (read < entry_stat.size) {
      return absl::InvalidArgumentError(
          absl::StrFormat("Unable to stat entry %d from zip: %s, error: %s", i,
                          zip_file_path, zip_strerror(archive)));
    }

    if (absl::EndsWith(entry_name, "package_info.prototxt") ||
        absl::EndsWith(entry_name, "package_info.textproto")) {
      if (optional_package_info.has_value()) {
        LOG(WARNING) << absl::StrCat(
            "Warning: Ignoring PackageInfo in ", zip_file_path,
            " because PackageInfo was passed to loading function.  Use the API "
            "with no PackageInfo to use the one from the zip.");
      } else {
        if (!google::protobuf::TextFormat::ParseFromString(contents,
                                                 &fhir_package.package_info)) {
          return absl::InvalidArgumentError(
              absl::StrCat("Invalid PackageInfo found for ", zip_file_path));
        }
        package_info_found = true;
      }
    } else if (absl::EndsWith(entry_name, ".json")) {
      json_files.insert({entry_name, contents});
    }
  }
  if (zip_close(archive) != 0) {
    return absl::InternalError(
        absl::StrCat("Failed Freeing Zip: ", zip_file_path));
  }

  if (!optional_package_info.has_value() && !package_info_found) {
    return absl::InvalidArgumentError(absl::StrCat(
        "FhirPackage does not have a valid package_info.prototxt: ",
        zip_file_path));
  }

  for (const auto& entry : json_files) {
    const std::string& name = entry.first;
    const std::string& json = entry.second;

    absl::StatusOr<std::string> resource_type = GetResourceType(json);
    if (!resource_type.ok()) {
      LOG(WARNING) << "Unhandled JSON entry: " << name;
      continue;
    }
    if (*resource_type == "ValueSet") {
      fhir_package.value_sets.emplace_back();
      ValueSet& value_set = fhir_package.value_sets.back();
      FHIR_RETURN_IF_ERROR(r4::MergeJsonFhirStringIntoProto(
          json, &value_set, absl::UTCTimeZone(), true));
    } else if (*resource_type == "CodeSystem") {
      fhir_package.code_systems.emplace_back();
      CodeSystem& code_system = fhir_package.code_systems.back();
      FHIR_RETURN_IF_ERROR(r4::MergeJsonFhirStringIntoProto(
          json, &code_system, absl::UTCTimeZone(), true));
    } else if (*resource_type == "StructureDefinition") {
      fhir_package.structure_definitions.emplace_back();
      StructureDefinition& structure_definition =
          fhir_package.structure_definitions.back();
      FHIR_RETURN_IF_ERROR(r4::MergeJsonFhirStringIntoProto(
          json, &structure_definition, absl::UTCTimeZone(), true));
    } else if (*resource_type == "SearchParameter") {
      fhir_package.search_parameters.emplace_back();
      SearchParameter& search_parameter = fhir_package.search_parameters.back();
      FHIR_RETURN_IF_ERROR(r4::MergeJsonFhirStringIntoProto(
          json, &search_parameter, absl::UTCTimeZone(), true));
    }
  }

  return fhir_package;
}

}  // namespace google::fhir
