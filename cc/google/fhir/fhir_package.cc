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

#include <memory>
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

// Adds the resource described by `resource_json` found within `parent_resource`
// to the appropriate ResourceCollection of the given `fhir_package`. Allows the
// resource to subsequently be retrieved by its URL from the FhirPackage.
// In the case where `resource_json` is located inside a bundle,
// `parent_resource` will be the bundle containing the resource. Otherwise,
// `resource_json` and `parent_resource` will be the same JSON object.
// If the JSON is not a FHIR resource, or not a resource type tracked by the
// PackageManager, does nothing and returns an OK status.
absl::Status MaybeAddResourceToFhirPackage(
    std::shared_ptr<const internal::FhirJson> parent_resource,
    const internal::FhirJson& resource_json, FhirPackage& fhir_package) {
  if (!resource_json.isObject()) {
    // Not a json object - definitly not a resource.
    return absl::OkStatus();
  }

  absl::StatusOr<const std::string> resource_type =
      GetResourceType(resource_json);
  if (!resource_type.ok()) {
    // If no resource type key was found, this is not a FHIR resource.  Ignore.
    // Return status failure for any other kind of failure.
    return absl::IsNotFound(resource_type.status()) ? absl::OkStatus()
                                                    : resource_type.status();
  }
  if (*resource_type == "ValueSet") {
    FHIR_RETURN_IF_ERROR(
        fhir_package.value_sets.Put(parent_resource, resource_json));
  } else if (*resource_type == "CodeSystem") {
    FHIR_RETURN_IF_ERROR(
        fhir_package.code_systems.Put(parent_resource, resource_json));
  } else if (*resource_type == "StructureDefinition") {
    FHIR_RETURN_IF_ERROR(
        fhir_package.structure_definitions.Put(parent_resource, resource_json));
  } else if (*resource_type == "SearchParameter") {
    FHIR_RETURN_IF_ERROR(
        fhir_package.search_parameters.Put(parent_resource, resource_json));
  } else if (*resource_type == "Bundle") {
    FHIR_ASSIGN_OR_RETURN(const internal::FhirJson* entries,
                          resource_json.get("entry"));
    FHIR_ASSIGN_OR_RETURN(int num_entries, entries->arraySize());

    for (int i = 0; i < num_entries; ++i) {
      FHIR_ASSIGN_OR_RETURN(const internal::FhirJson* entry, entries->get(i));
      FHIR_ASSIGN_OR_RETURN(const internal::FhirJson* resource,
                            entry->get("resource"));
      FHIR_RETURN_IF_ERROR(MaybeAddResourceToFhirPackage(
          parent_resource, *resource, fhir_package));
    }
  }
  // We got a resource type, but it's not one of the ones we track.  Ignore.
  return absl::OkStatus();
}

absl::Status MaybeAddResourceToFhirPackage(absl::string_view resource_json,
                                           FhirPackage& fhir_package) {
  auto parsed_json = std::make_unique<internal::FhirJson>();
  FHIR_RETURN_IF_ERROR(
      internal::ParseJsonValue(std::string(resource_json), *parsed_json));
  internal::FhirJson const* parsed_json_ptr = parsed_json.get();
  return MaybeAddResourceToFhirPackage(std::move(parsed_json), *parsed_json_ptr,
                                       fhir_package);
}
}  // namespace

namespace internal {
absl::StatusOr<const FhirJson*> FindResourceInBundle(
    absl::string_view uri, const FhirJson& bundle_json) {
  FHIR_ASSIGN_OR_RETURN(const FhirJson* entries, bundle_json.get("entry"));
  FHIR_ASSIGN_OR_RETURN(int num_entries, entries->arraySize());
  for (int i = 0; i < num_entries; ++i) {
    FHIR_ASSIGN_OR_RETURN(const FhirJson* entry, entries->get(i));
    FHIR_ASSIGN_OR_RETURN(const FhirJson* resource, entry->get("resource"));
    FHIR_ASSIGN_OR_RETURN(std::string resource_type,
                          internal::GetResourceType(*resource));

    if (resource_type == "Bundle") {
      // If the resource is a bundle, recursively search through it.
      absl::StatusOr<const FhirJson*> bundle_json =
          FindResourceInBundle(uri, *resource);
      if (bundle_json.ok()) {
        // We found the resource!
        return bundle_json;
      } else if (bundle_json.status().code() != absl::StatusCode::kNotFound) {
        // We encountered a parsing error in the bundle to report.
        return bundle_json.status();
      }
    } else {
      FHIR_ASSIGN_OR_RETURN(std::string resource_url,
                            internal::GetResourceUrl(*resource));
      // Found the resource!
      if (uri == resource_url) {
        return resource;
      }
    }
  }
  return absl::NotFoundError(absl::StrFormat("%s not present in bundle.", uri));
}

absl::StatusOr<std::string> GetResourceType(const FhirJson& parsed_json) {
  FHIR_ASSIGN_OR_RETURN(const internal::FhirJson* resource_type_json,
                        parsed_json.get("resourceType"));
  return resource_type_json->asString();
}

absl::StatusOr<std::string> GetResourceUrl(const FhirJson& parsed_json) {
  FHIR_ASSIGN_OR_RETURN(const internal::FhirJson* resource_url_json,
                        parsed_json.get("url"));
  FHIR_ASSIGN_OR_RETURN(const std::string url, resource_url_json->asString());
  if (url.empty()) {
    return absl::InvalidArgumentError(absl::StrFormat("URL is empty"));
  }
  return url;
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
          MaybeAddResourceToFhirPackage(contents, *fhir_package);
      if (!add_status.ok()) {
        // Concatenate all errors founds while parsing the package.
        std::string error_message =
            absl::StrFormat("Unhandled JSON entry: %s due to error: %s",
                            entry_stat.name, add_status.message());
        if (parse_errors.ok()) {
          parse_errors = absl::InvalidArgumentError(absl::StrCat(
              "Error(s) encountered during parsing: ", error_message));
        } else {
          parse_errors = absl::Status(
              parse_errors.code(),
              absl::StrCat(parse_errors.message(), "; ", error_message));
        }
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
