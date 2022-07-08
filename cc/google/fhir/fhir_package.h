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

#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/r4/json_format.h"
#include "google/fhir/status/status.h"
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
  // An iterator over all valid resources contained in the collection. Invalid
  // resources, such as those with malformed JSON, will not be returned by the
  // iterator.
  class Iterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = const T*;
    using reference = const T&;

    Iterator(ResourceCollection* resource_collection,
             absl::flat_hash_map<const std::string, const std::string>::iterator
                 uri_iter)
        : resource_collection_(resource_collection), uri_iter_(uri_iter) {
      // Skip over any initial invalid entries so it is safe to dereference.
      this->AdvanceUntilValid();
    }

    pointer operator->() const {
      std::string uri = uri_iter_->first;
      absl::StatusOr<const T*> resource =
          resource_collection_->GetResource(uri);

      // The AdvanceUntilValid() calls in the constructor and in operator++
      // should prevent us from attempting to dereference an invalid resource.
      CHECK(resource.ok()) << absl::StrFormat(
          "Attempted to access invalid resource from iterator for JSON entry: "
          "%s uri: %s with error: %s",
          uri_iter_->second, uri_iter_->first, resource.status().message());

      return *resource;
    }
    reference operator*() const { return *(this->operator->()); }
    Iterator& operator++() {
      ++uri_iter_;
      // Additionally advance past invalid entries so it is safe to dereference.
      this->AdvanceUntilValid();
      return *this;
    }
    bool operator==(const Iterator& other) const {
      // Perform equality checks between iterators at positions with valid
      // resources. This allows an iterator with no remaining valid resources to
      // equal an iterator at the end of uri_iter_.
      return this->NextValidPosistion().uri_iter_ ==
             other.NextValidPosistion().uri_iter_;
    }
    bool operator!=(const Iterator& other) const {
      return this->NextValidPosistion().uri_iter_ !=
             other.NextValidPosistion().uri_iter_;
    }

   private:
    ResourceCollection* resource_collection_;
    absl::flat_hash_map<const std::string, const std::string>::iterator
        uri_iter_;

    // Advances uri_iter_ until it points to a valid, parse-able resource. If it
    // currently points to a valid resource, does not advance the iterator.
    void AdvanceUntilValid() {
      while (true) {
        if (this->UriIterExhausted()) {
          return;
        }
        absl::StatusOr<const T*> resource =
            resource_collection_->GetResource(uri_iter_->first);
        if (resource.ok()) {
          return;
        } else {
          LOG(WARNING) << absl::StrFormat(
              "Unhandled JSON entry: %s for uri: %s with error: %s",
              uri_iter_->second, uri_iter_->first, resource.status().message());
          ++uri_iter_;
        }
      }
    }
    // Builds a new iterator pointing towards the next valid, parse-able
    // resource. If the current iterator points to a valid resource, the
    // returned iterator will be an identical copy.
    Iterator NextValidPosistion() const {
      // The constructor calls .AdvanceUntilValid(), so we know the new
      // iterator is pointing to a valid resource.
      return Iterator(this->resource_collection_, this->uri_iter_);
    }

    // Indicates if the URIs being iterated over have been exhausted.
    bool UriIterExhausted() const {
      return uri_iter_ == resource_collection_->resource_paths_for_uris_.end();
    }
  };
  explicit ResourceCollection(absl::string_view zip_file_path)
      : zip_file_path_(zip_file_path) {}

  Iterator begin() { return Iterator(this, resource_paths_for_uris_.begin()); }
  Iterator end() { return Iterator(this, resource_paths_for_uris_.end()); }
  absl::flat_hash_map<const std::string, const std::string>::size_type size() {
    return resource_paths_for_uris_.size();
  }

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
  absl::StatusOr<const T*> GetResource(absl::string_view uri) {
    const auto cached = parsed_resources_.find(std::string(uri));
    if (cached != parsed_resources_.end()) {
      return &cached->second;
    }

    const auto path = resource_paths_for_uris_.find(std::string(uri));
    if (path == resource_paths_for_uris_.end()) {
      return absl::NotFoundError(
          absl::StrFormat("%s not present in collection", uri));
    }

    FHIR_ASSIGN_OR_RETURN(
        const T parsed, ParseProtoFromFile(uri, zip_file_path_, path->second));
    return CacheParsedResource(uri, parsed);
  }

  const T* CacheParsedResource(absl::string_view uri, const T& resource) {
    auto insert = parsed_resources_.insert({std::string(uri), resource});
    return &(insert.first->second);
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

// Struct representing a FHIR Proto package. This is constructed from a zip
// file containing these files, as generated by the `fhir_package` rule
// in protogen.bzl.
// TODO: Support versions other than R4
// TODO: package metadata from the NPM package format.
struct FhirPackage {
  ResourceCollection<google::fhir::r4::core::StructureDefinition>
      structure_definitions;
  ResourceCollection<google::fhir::r4::core::SearchParameter> search_parameters;
  ResourceCollection<google::fhir::r4::core::CodeSystem> code_systems;
  ResourceCollection<google::fhir::r4::core::ValueSet> value_sets;

  static absl::StatusOr<std::unique_ptr<FhirPackage>> Load(
      absl::string_view zip_file_path);

  absl::StatusOr<const google::fhir::r4::core::StructureDefinition*>
  GetStructureDefinition(absl::string_view uri) {
    return this->structure_definitions.GetResource(uri);
  }

  absl::StatusOr<const google::fhir::r4::core::SearchParameter*>
  GetSearchParameter(absl::string_view uri) {
    return this->search_parameters.GetResource(uri);
  }

  absl::StatusOr<const google::fhir::r4::core::CodeSystem*> GetCodeSystem(
      absl::string_view uri) {
    return this->code_systems.GetResource(uri);
  }

  absl::StatusOr<const google::fhir::r4::core::ValueSet*> GetValueSet(
      absl::string_view uri) {
    return this->value_sets.GetResource(uri);
  }

 private:
  explicit FhirPackage(absl::string_view zip_file_path)
      : structure_definitions(
            ResourceCollection<google::fhir::r4::core::StructureDefinition>(
                zip_file_path)),
        search_parameters(
            ResourceCollection<google::fhir::r4::core::SearchParameter>(
                zip_file_path)),
        code_systems(ResourceCollection<google::fhir::r4::core::CodeSystem>(
            zip_file_path)),
        value_sets(ResourceCollection<google::fhir::r4::core::ValueSet>(
            zip_file_path)) {}
};

namespace internal {
// Finds the first ok response from calling `get_resource` on each package in
// `packages` with the given `uri`. Returns a status describing each non-ok
// status for every `get_resource` call if no ok response can be found.
template <typename T>
absl::StatusOr<T> SearchPackagesForResource(
    absl::StatusOr<T> (FhirPackage::*get_resource)(absl::string_view),
    std::vector<std::unique_ptr<FhirPackage>>& packages,
    absl::string_view uri) {
  absl::Status status;
  for (std::unique_ptr<FhirPackage>& package : packages) {
    absl::StatusOr<T> resource = (*package.*get_resource)(uri);
    if (resource.ok()) {
      return resource;
    } else {
      status.Update(resource.status());
    }
  }
  return status;
}
}  // namespace internal

// Manages access to a collection of FhirPackage instances.
// Allows users to add packages to the package manager and then search all of
// them for a particular resource.
class FhirPackageManager {
 public:
  FhirPackageManager() {}

  // Adds the given `package` to the package manager.
  // Takes ownership of the given `package`.
  void AddPackage(std::unique_ptr<FhirPackage> package);

  // Loads the FHIR package at `path` and adds it to the package manager.
  absl::Status AddPackageAtPath(absl::string_view path);

  // Retrieves a protocol buffer representation of the resource with the given
  // `uri`. Searches the packages added to the package manger for the resource
  // with the given URI. If multiple packages contain the same resource, the
  // package consulted will be non-deterministic.
  absl::StatusOr<const google::fhir::r4::core::StructureDefinition*>
  GetStructureDefinition(absl::string_view uri) {
    return internal::SearchPackagesForResource(
        &FhirPackage::GetStructureDefinition, packages_, uri);
  }
  absl::StatusOr<const google::fhir::r4::core::SearchParameter*>
  GetSearchParameter(absl::string_view uri) {
    return internal::SearchPackagesForResource(&FhirPackage::GetSearchParameter,
                                               packages_, uri);
  }
  absl::StatusOr<const google::fhir::r4::core::CodeSystem*> GetCodeSystem(
      absl::string_view uri) {
    return internal::SearchPackagesForResource(&FhirPackage::GetCodeSystem,
                                               packages_, uri);
  }
  absl::StatusOr<const google::fhir::r4::core::ValueSet*> GetValueSet(
      absl::string_view uri) {
    return internal::SearchPackagesForResource(&FhirPackage::GetValueSet,
                                               packages_, uri);
  }

 private:
  std::vector<std::unique_ptr<FhirPackage>> packages_;
};

}  // namespace google::fhir

#endif  // GOOGLE_FHIR_FHIR_PACKAGE_H_
