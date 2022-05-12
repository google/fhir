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

#ifndef GOOGLE_FHIR_FHIR_PACKAGE_H_
#define GOOGLE_FHIR_FHIR_PACKAGE_H_

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/r4/json_format.h"
#include "google/fhir/status/status.h"
#include "proto/google/fhir/proto/profile_config.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/code_system.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/search_parameter.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/structure_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/value_set.pb.h"

namespace google::fhir {

namespace internal {
// Finds the JSON object for the resource with `uri` inside `bundle_json`.
absl::StatusOr<const FhirJson*> FindResourceInBundle(
    absl::string_view uri, const FhirJson& bundle_json);

// Parses the resource JSON from the `resource_path` entry within
// the .zip file at `zip_file_path` and places result of the parse into
// `json_value`.
absl::Status ParseResourceFromZip(absl::string_view zip_file_path,
                                  absl::string_view resource_path,
                                  FhirJson& json_value);

absl::StatusOr<std::string> GetResourceType(const FhirJson& parsed_json);
absl::StatusOr<std::string> GetResourceUrl(const FhirJson& parsed_json);
}  // namespace internal

// A collection of FHIR resources of a given type.
// Acts as an accessor for resources of a single type from a FHIR Package zip
// file containing JSON representations of various resources.
// Resources are lazily read and parsed from the zip file as requested.
// Resources are cached for fast repeated access.
// Example:
//    auto collection = ResourceCollection<ValueSet>("zip_file_path");
//    collection.AddUriAtPath("uri-for-resource",
//    "path_within_zip_file_for_resource");
//    absl::StatusOr<ValueSet> result =
//    collection.GetResource("uri-for-resource");
template <typename T>
class ResourceCollection {
 public:
  explicit ResourceCollection(absl::string_view zip_file_path)
      : zip_file_path_(zip_file_path) {}

  // Indicates that the resource for `uri` can be found inside this
  // ResourceCollection's zip file. The `uri` is defined by the resource JSON
  // file at `path` within the zip file. The path may be to a file containing
  // either the uri's resource JSON directly or a Bundle containing the
  // resource.
  void AddUriAtPath(absl::string_view uri, absl::string_view path) {
    resource_paths_for_uris_.insert({std::string(uri), std::string(path)});
  }

  // Retrieves the resource for `uri`. Either returns a previously cached proto
  // for the resource or reads and parses the resource from the
  // ResourceCollection's zip file.
  absl::StatusOr<const T> GetResource(absl::string_view uri) {
    const auto cached = parsed_resources_.find(std::string(uri));
    if (cached != parsed_resources_.end()) {
      return cached->second;
    }

    const auto path = resource_paths_for_uris_.find(std::string(uri));
    if (path == resource_paths_for_uris_.end()) {
      return absl::NotFoundError(
          absl::StrFormat("%s not present in collection", uri));
    }

    absl::StatusOr<const T> parsed =
        ParseProtoFromFile(uri, zip_file_path_, path->second);
    if (parsed.ok()) {
      CacheParsedResource(uri, *parsed);
    }
    return parsed;
  }

  void CacheParsedResource(absl::string_view uri, const T& resource) {
    parsed_resources_.insert({std::string(uri), resource});
  }

 private:
  // Retrieves the resource JSON for `uri` from `zip_file_path` at
  // `resource_path` and parses the JSON into a protocol buffer.
  static absl::StatusOr<const T> ParseProtoFromFile(
      absl::string_view uri, absl::string_view zip_file_path,
      absl::string_view resource_path) {
    internal::FhirJson parsed_json;
    FHIR_RETURN_IF_ERROR(internal::ParseResourceFromZip(
        zip_file_path, resource_path, parsed_json));

    FHIR_ASSIGN_OR_RETURN(std::string resource_type,
                          internal::GetResourceType(parsed_json));
    if (resource_type != T::descriptor()->name()) {
      if (resource_type == "Bundle") {
        FHIR_ASSIGN_OR_RETURN(const internal::FhirJson* bundle_json,
                              FindResourceInBundle(uri, parsed_json));
        return google::fhir::r4::JsonFhirObjectToProto<T>(*bundle_json,
                                                          absl::UTCTimeZone());
      } else {
        return absl::InvalidArgumentError(
            absl::Substitute("Type mismatch in ResourceCollection:  Collection "
                             "is of type $0, but JSON resource is of type $1",
                             T::descriptor()->name(), resource_type));
      }
    }
    return google::fhir::r4::JsonFhirObjectToProto<T>(parsed_json,
                                                      absl::UTCTimeZone());
  }

  const std::string zip_file_path_;
  absl::flat_hash_map<const std::string, T> parsed_resources_;
  absl::flat_hash_map<const std::string, const std::string>
      resource_paths_for_uris_;
};

// Struct representing a FHIR Proto package, including defining resources and
// a PackageInfo proto. This is constructed from a zip file containing these
// files, as generated by the `fhir_package` rule in protogen.bzl.
// TODO: Support versions other than R4
struct FhirPackage {
  proto::PackageInfo package_info;
  std::vector<google::fhir::r4::core::StructureDefinition>
      structure_definitions;
  std::vector<google::fhir::r4::core::SearchParameter> search_parameters;
  std::vector<google::fhir::r4::core::CodeSystem> code_systems;
  std::vector<google::fhir::r4::core::ValueSet> value_sets;

  static absl::StatusOr<FhirPackage> Load(absl::string_view zip_file_path);

  // This Load variant will use the passed-in PackageInfo proto for the
  // FhirPackage, regardless of whether or not one was found in the zip.
  static absl::StatusOr<FhirPackage> Load(
      absl::string_view zip_file_path, const proto::PackageInfo& package_info);

 private:
  FhirPackage() {}
  static absl::StatusOr<FhirPackage> Load(
      absl::string_view zip_file_path,
      const absl::optional<proto::PackageInfo> optional_package_info);
};

}  // namespace google::fhir

#endif  // GOOGLE_FHIR_FHIR_PACKAGE_H_
