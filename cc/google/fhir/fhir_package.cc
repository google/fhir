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

#include "google/protobuf/text_format.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "absl/types/optional.h"
#include "google/fhir/r4/json_format.h"
#include "proto/google/fhir/proto/profile_config.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/structure_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/value_set.pb.h"
#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"
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

absl::StatusOr<FhirPackage> FhirPackage::Load(absl::string_view zip_file_path) {
  return Load(zip_file_path, absl::optional<PackageInfo>());
}

absl::StatusOr<FhirPackage> FhirPackage::Load(absl::string_view zip_file_path,
                                              const PackageInfo& package_info) {
  return Load(zip_file_path, absl::optional<PackageInfo>(package_info));
}

absl::StatusOr<FhirPackage> FhirPackage::Load(
    absl::string_view zip_file_path,
    const absl::optional<PackageInfo> optional_package_info) {
  struct archive* archive = archive_read_new();
  archive_read_support_filter_all(archive);
  archive_read_support_format_all(archive);

  if (archive_read_open_filename(archive, std::string(zip_file_path).c_str(),
                                 10240) != ARCHIVE_OK) {
    return absl::InvalidArgumentError(
        absl::StrCat("Unable to open zip: ", zip_file_path));
  }

  FhirPackage fhir_package;
  if (optional_package_info.has_value()) {
    fhir_package.package_info = *optional_package_info;
  }
  bool package_info_found = false;
  absl::flat_hash_map<const std::string, const std::string> json_files;

  struct archive_entry* entry;
  while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
    const std::string entry_name = archive_entry_pathname(entry);
    const int64 length = archive_entry_size(entry);
    std::string contents(length, '\0');
    archive_read_data(archive, &contents[0], length);

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
  if (archive_read_free(archive) != ARCHIVE_OK) {
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
