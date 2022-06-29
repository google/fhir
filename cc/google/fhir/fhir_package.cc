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
#include <utility>

#include "google/protobuf/text_format.h"
#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/json/json_sax_handler.h"
#include "google/fhir/status/status.h"
#include "proto/google/fhir/proto/r4/core/resources/structure_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/value_set.pb.h"
#include "lib/zip.h"

namespace google::fhir {

namespace {

// Adds the resource described by `resource_json` located at `resource_path`
// within its FHIR package .zip file to the appropriate ResourceCollection of
// the given `fhir_package`. Allows the resource to subsequently be retrieved by
// its URL from the FhirPackage.
absl::Status AddResourceToFhirPackage(const internal::FhirJson& resource_json,
                                      absl::string_view resource_path,
                                      FhirPackage& fhir_package) {
  FHIR_ASSIGN_OR_RETURN(const std::string resource_type,
                        GetResourceType(resource_json));
  FHIR_ASSIGN_OR_RETURN(const std::string resource_url,
                        GetResourceUrl(resource_json));
  if (resource_url.empty()) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "Unhandled JSON entry: %s resource has missing URL", resource_path));
  }

  if (resource_type == "ValueSet") {
    fhir_package.value_sets.AddUriAtPath(resource_url, resource_path);
  } else if (resource_type == "CodeSystem") {
    fhir_package.code_systems.AddUriAtPath(resource_url, resource_path);
  } else if (resource_type == "StructureDefinition") {
    fhir_package.structure_definitions.AddUriAtPath(resource_url,
                                                    resource_path);
  } else if (resource_type == "SearchParameter") {
    fhir_package.search_parameters.AddUriAtPath(resource_url, resource_path);
  } else if (resource_type == "Bundle") {
    FHIR_ASSIGN_OR_RETURN(const internal::FhirJson* entries,
                          resource_json.get("entry"));
    FHIR_ASSIGN_OR_RETURN(int num_entries, entries->arraySize());

    for (int i = 0; i < num_entries; ++i) {
      FHIR_ASSIGN_OR_RETURN(const internal::FhirJson* entry, entries->get(i));
      FHIR_ASSIGN_OR_RETURN(const internal::FhirJson* resource,
                            entry->get("resource"));
      FHIR_RETURN_IF_ERROR(
          AddResourceToFhirPackage(*resource, resource_path, fhir_package));
    }
  } else {
    return absl::InvalidArgumentError(absl::StrFormat(
        "Unhandled JSON entry: %s for unexpected resource type %s.",
        resource_path, resource_type));
  }
  return absl::OkStatus();
}

absl::Status AddResourceToFhirPackage(absl::string_view resource_json,
                                      absl::string_view resource_path,
                                      FhirPackage& fhir_package) {
  internal::FhirJson parsed_json;
  FHIR_RETURN_IF_ERROR(
      internal::ParseJsonValue(std::string(resource_json), parsed_json));
  return AddResourceToFhirPackage(parsed_json, resource_path, fhir_package);
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

absl::StatusOr<std::unique_ptr<FhirPackage>> FhirPackage::Load(
    absl::string_view zip_file_path) {
  int zip_open_error;
  zip_t* archive =
      zip_open(std::string(zip_file_path).c_str(), ZIP_RDONLY, &zip_open_error);
  if (archive == nullptr) {
    return absl::InvalidArgumentError(
        absl::StrFormat("Unable to open zip: %s error code: %d", zip_file_path,
                        zip_open_error));
  }

  // We can't use make_unique here because the constructor is private.
  auto fhir_package =
      std::unique_ptr<FhirPackage>(new FhirPackage(zip_file_path));
  absl::Status parse_errors;

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

    // Ignore deprecated package_info data.
    if (absl::EndsWith(entry_stat.name, "package_info.prototxt") ||
        absl::EndsWith(entry_stat.name, "package_info.textproto")) {
      continue;
    }

    if (absl::EndsWith(entry_stat.name, ".json")) {
      absl::Status add_status =
          AddResourceToFhirPackage(contents, entry_stat.name, *fhir_package);
      if (!add_status.ok()) {
        parse_errors.Update(absl::InvalidArgumentError(
            absl::StrFormat("Unhandled JSON entry: %s due to error: %s",
                            entry_stat.name, add_status.message())));
      }
    }
  }
  if (zip_close(archive) != 0) {
    return absl::InternalError(
        absl::StrCat("Failed Freeing Zip: ", zip_file_path));
  }

  if (!parse_errors.ok()) {
    return parse_errors;
  }
  return std::move(fhir_package);
}

void FhirPackageManager::AddPackage(std::unique_ptr<FhirPackage> package) {
  packages_.push_back(std::move(package));
}

absl::Status FhirPackageManager::AddPackageAtPath(absl::string_view path) {
  FHIR_ASSIGN_OR_RETURN(std::unique_ptr<FhirPackage> package,
                        FhirPackage::Load(path));
  AddPackage(std::move(package));
  return absl::OkStatus();
}

}  // namespace google::fhir
