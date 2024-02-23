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

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/json/json_sax_handler.h"
#include "google/fhir/r4/json_format.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/value_set.pb.h"
#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"

namespace google::fhir {

namespace {

// Adds the resource described by `resource_json` found within `parent_resource`
// to the appropriate ResourceCollection of the given `fhir_package`. Allows the
// resource to subsequently be retrieved by its URL from the FhirPackage.
// In the case where `resource_json` is located inside a bundle,
// `parent_resource` will be the bundle containing the resource. Otherwise,
// `resource_json` and `parent_resource` will be the same JSON object.
// If the JSON is not a FHIR resource, or not a resource type tracked by the
// FhirPackage, does nothing and returns an OK status.
absl::Status MaybeAddResourceToFhirPackage(
    std::shared_ptr<const internal::FhirJson> parent_resource,
    const internal::FhirJson& resource_json, FhirPackage& fhir_package) {
  if (!resource_json.isObject()) {
    // Not a json object - definitely not a resource.
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
  } else if (*resource_type == "ImplementationGuide") {
    fhir_package.implementation_guide =
        std::make_unique<google::fhir::r4::core::ImplementationGuide>();
    FHIR_RETURN_IF_ERROR(google::fhir::r4::MergeJsonFhirObjectIntoProto(
        resource_json, fhir_package.implementation_guide.get(),
        absl::UTCTimeZone(), /*validate=*/false,
        FailFastErrorHandler::FailOnFatalOnly()));
  } else if (*resource_type == "Bundle") {
    absl::StatusOr<const internal::FhirJson*> entries =
        resource_json.get("entry");
    if (!entries.status().ok()) {
      // No entries, nothing to add.
      return absl::OkStatus();
    }
    FHIR_ASSIGN_OR_RETURN(int num_entries, (*entries)->arraySize());

    for (int i = 0; i < num_entries; ++i) {
      FHIR_ASSIGN_OR_RETURN(const internal::FhirJson* entry,
                            (*entries)->get(i));
      absl::StatusOr<const internal::FhirJson*> resource =
          entry->get("resource");
      if (resource.status().ok()) {
        FHIR_RETURN_IF_ERROR(MaybeAddResourceToFhirPackage(
            parent_resource, **resource, fhir_package));
      }
    }
  }
  // We got a resource type, but it's not one of the ones we track.  Ignore.
  return absl::OkStatus();
}

absl::Status MaybeAddEntryToFhirPackage(absl::string_view entry_name,
                                        absl::string_view resource_json,
                                        FhirPackage& fhir_package) {
  if (!absl::EndsWith(entry_name, ".json")) {
    // Ignore non-JSON files.
    return absl::OkStatus();
  }
  if (absl::EndsWith(entry_name, "ig-r4.json")) {
    // ig-r4.json is a Grahame-special file that should be ignored at all costs.
    // see:
    // https://chat.fhir.org/#narrow/stream/179250-bulk-data/topic/Why.20are.20there.20two.20IG.20resources.3F/near/422507754
    return absl::OkStatus();
  }

  auto parsed_json = std::make_unique<internal::FhirJson>();
  FHIR_RETURN_IF_ERROR(internal::ParseJsonValue(resource_json, *parsed_json));
  internal::FhirJson const* parsed_json_ptr = parsed_json.get();
  return MaybeAddResourceToFhirPackage(std::move(parsed_json), *parsed_json_ptr,
                                       fhir_package);
}

// Opens the archive for reading at `archive_file_path` and returns a unique
// pointer to the archive. The unique pointer will close and free the archive
// when it is destructed.
absl::StatusOr<std::unique_ptr<archive, decltype(&archive_read_free)>>
OpenArchive(absl::string_view archive_file_path) {
  // archive_read_free itself calls archive_read_close, so no further cleanup is
  // required by the caller.
  std::unique_ptr<archive, decltype(&archive_read_free)> archive(
      archive_read_new(), &archive_read_free);
  archive_read_support_filter_all(archive.get());
  archive_read_support_format_all(archive.get());

  int archive_open_error = archive_read_open_filename(
      archive.get(), std::string(archive_file_path).c_str(),
      /*block_size=*/1024);
  if (archive_open_error != ARCHIVE_OK) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "Unable to open archive %s, error code: %d %s.", archive_file_path,
        archive_open_error, archive_error_string(archive.get())));
  }
  return std::move(archive);
}

absl::Status LoadPackage(absl::string_view archive_file_path,
                         std::function<absl::Status(
                             absl::string_view entry_name,
                             absl::string_view contents, FhirPackage& package)>
                             handle_entry,
                         FhirPackage& fhir_package) {
  // The FHIR_ASSIGN_OR_RETURN macro expansion doesn't parse correctly without
  // placing the type inside a 'using' statement.
  using archive_ptr = std::unique_ptr<archive, decltype(&archive_read_free)>;
  FHIR_ASSIGN_OR_RETURN(archive_ptr archive, OpenArchive(archive_file_path));

  absl::Status all_errors;
  archive_entry* entry;
  while (true) {
    int read_next_status = archive_read_next_header(archive.get(), &entry);
    if (read_next_status == ARCHIVE_EOF) {
      break;
    } else if (read_next_status == ARCHIVE_RETRY) {
      continue;
    } else if (read_next_status != ARCHIVE_OK &&
               read_next_status != ARCHIVE_WARN) {
      return absl::InvalidArgumentError(absl::StrFormat(
          "Unable to read archive %s, error code: %d %s.", archive_file_path,
          read_next_status, archive_error_string(archive.get())));
    }

    // Skip hardlinks, as we don't need to process the same file contents
    // multiple times.
    if (archive_entry_hardlink(entry) != nullptr) {
      continue;
    }

    std::string entry_name = archive_entry_pathname(entry);
    // Ignore deprecated package_info data.
    if (absl::EndsWith(entry_name, "package_info.prototxt") ||
        absl::EndsWith(entry_name, "package_info.textproto")) {
      continue;
    }

    la_int64_t length = archive_entry_size(entry);
    std::string contents(length, '\0');
    la_ssize_t read = archive_read_data(archive.get(), &contents[0], length);
    if (read < length) {
      return absl::InvalidArgumentError(
          absl::StrFormat("Unable to read entry %s from archive %s.",
                          entry_name, archive_file_path));
    }

    absl::Status entry_status =
        handle_entry(entry_name, contents, fhir_package);
    if (!entry_status.ok()) {
      // Concatenate all errors founds while processing the package.
      std::string error_message =
          absl::StrFormat("Unhandled JSON entry %s due to error: %s.",
                          entry_name, entry_status.message());
      if (all_errors.ok()) {
        all_errors = absl::InvalidArgumentError(
            absl::StrCat("Error(s) encountered during parsing of ",
                         archive_file_path, ": ", error_message));
      } else {
        all_errors = absl::Status(
            all_errors.code(),
            absl::StrCat(all_errors.message(), "; ", error_message));
      }
    }
  }

  return all_errors;
}

// Concatenates error messages from the two statuses into a single status. If
// one status is ok, returns the other. If both are not ok, returns a new error
// with the code of the first status.
absl::Status ConcatErrors(const absl::Status& status1,
                          const absl::Status& status2) {
  if (status1.ok()) {
    return status2;
  } else if (status2.ok()) {
    return status1;
  } else {
    return absl::Status(status1.code(), absl::StrCat(status1.message(), "; ",
                                                     status2.message()));
  }
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
    absl::string_view archive_file_path) {
  // We can't use make_unique here because the constructor is private.
  auto fhir_package = absl::WrapUnique(new FhirPackage());
  FHIR_RETURN_IF_ERROR(LoadPackage(archive_file_path,
                                   &MaybeAddEntryToFhirPackage, *fhir_package));
  return std::move(fhir_package);
}

absl::StatusOr<std::unique_ptr<FhirPackage>> FhirPackage::Load(
    absl::string_view archive_file_path,
    std::function<absl::Status(absl::string_view entry_name,
                               absl::string_view contents,
                               FhirPackage& package)>
        handle_entry) {
  // We can't use make_unique here because the constructor is private.
  auto fhir_package = absl::WrapUnique(new FhirPackage());

  // Create a lambda which calls the default MaybeAddEntryToFhirPackage handler
  // and also calls the user-provided `handle_entry` function.
  auto handle_entry_wrapper = [&handle_entry](absl::string_view entry_name,
                                              absl::string_view contents,
                                              FhirPackage& package) {
    absl::Status add_entry_status =
        MaybeAddEntryToFhirPackage(entry_name, contents, package);
    absl::Status handle_entry_status =
        handle_entry(entry_name, contents, package);
    return ConcatErrors(add_entry_status, handle_entry_status);
  };

  FHIR_RETURN_IF_ERROR(
      LoadPackage(archive_file_path, handle_entry_wrapper, *fhir_package));
  return std::move(fhir_package);
}

absl::StatusOr<std::unique_ptr<google::fhir::r4::core::ValueSet>>
FhirPackage::GetValueSet(absl::string_view uri) const {
  // Handle "canonical" URLs of the form "$url|$version", e.g.
  // "http://hl7.org/fhir/ValueSet/identifier-use|4.3.0"
  // For these URLs, split on the pipe, resolve the url and return an error if
  // the resource doesn't match the version.
  const std::size_t version_delimiter = uri.find_last_of('|');
  std::string version;
  if (version_delimiter != std::string::npos) {
    version = uri.substr(version_delimiter + 1,  // + 1 to skip the '|' itself
                         std::string::npos);
    uri = uri.substr(0, version_delimiter);
  }

  FHIR_ASSIGN_OR_RETURN(
      std::unique_ptr<google::fhir::r4::core::ValueSet> value_set,
      this->value_sets.Get(uri));

  if (version_delimiter != std::string::npos &&
      value_set->version().value() != version) {
    return absl::NotFoundError(
        absl::StrCat("Found version ", value_set->version().value(),
                     " for resource ", uri, " but ", version, " requested."));
  }

  return std::move(value_set);
}

}  // namespace google::fhir
