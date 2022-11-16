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

#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "glog/logging.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/r4/json_format.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/code_system.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/search_parameter.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/structure_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/value_set.pb.h"

namespace google::fhir {

namespace internal {
// Finds the JSON object for the resource with `uri` inside `bundle_json`.
absl::StatusOr<const FhirJson*> FindResourceInBundle(
    absl::string_view uri, const FhirJson& bundle_json);

absl::StatusOr<std::string> GetResourceType(const FhirJson& parsed_json);
absl::StatusOr<std::string> GetResourceUrl(const FhirJson& parsed_json);
}  // namespace internal

// A collection of FHIR resources of a given type.
// Acts as an accessor for resources of a single type from a FHIR Package
// archive containing JSON representations of various resources. Resources are
// lazily read and parsed from the archive as requested. Resources are cached
// for fast repeated access.
template <typename T>
class ResourceCollection {
 private:
  using MaybeParsedResource =
      std::variant<std::unique_ptr<const T>,
                   std::shared_ptr<const internal::FhirJson>>;

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

    Iterator(
        ResourceCollection* resource_collection,
        typename absl::flat_hash_map<std::string, MaybeParsedResource>::iterator
            uri_iter)
        : resource_collection_(resource_collection), uri_iter_(uri_iter) {
      // Skip over any initial invalid entries so it is safe to dereference.
      this->AdvanceUntilValid();
    }

    pointer operator->() const {
      std::string uri = uri_iter_->first;
      absl::StatusOr<const T*> resource = resource_collection_->Get(uri);

      // The AdvanceUntilValid() calls in the constructor and in operator++
      // should prevent us from attempting to dereference an invalid resource.
      CHECK(resource.ok()) << absl::StrFormat(
          "Attempted to access invalid resource from iterator for uri: %s with "
          "error: %s",
          uri, resource.status().message());

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
    typename absl::flat_hash_map<std::string, MaybeParsedResource>::iterator
        uri_iter_;
    // Advances uri_iter_ until it points to a valid, parse-able resource. If it
    // currently points to a valid resource, does not advance the iterator.
    void AdvanceUntilValid() {
      while (true) {
        if (this->UriIterExhausted()) {
          return;
        }
        absl::StatusOr<const T*> resource =
            resource_collection_->Get(uri_iter_->first);
        if (resource.ok()) {
          return;
        } else {
          LOG(WARNING) << absl::StrFormat(
              "Unhandled entry for uri: %s with error: %s", uri_iter_->first,
              resource.status().message());
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
      return uri_iter_ == resource_collection_->resources_by_uri_.end();
    }
  };

  ResourceCollection() = default;
  ResourceCollection(const ResourceCollection& other) = default;
  ResourceCollection(ResourceCollection&& other) = default;

  ResourceCollection& operator=(const ResourceCollection& other) = default;
  ResourceCollection& operator=(ResourceCollection&& other) = default;

  Iterator begin() { return Iterator(this, this->resources_by_uri_.begin()); }
  Iterator end() { return Iterator(this, this->resources_by_uri_.end()); }

  typename absl::flat_hash_map<std::string, MaybeParsedResource>::size_type
  size() const {
    return resources_by_uri_.size();
  }

  // Adds the resource represented by `resource_json` into this collection for
  // subsequent lookup via the Get method.
  absl::Status Put(std::shared_ptr<const internal::FhirJson> resource_json) {
    FHIR_ASSIGN_OR_RETURN(std::string resource_url,
                          GetResourceUrl(*resource_json));
    resources_by_uri_.try_emplace(std::move(resource_url),
                                  std::move(resource_json));
    return absl::OkStatus();
  }

  // Adds the resource represented by `resource_json` found inside
  // `parent_bundle` into this collection for subsequent lookup via the Get
  // method.
  absl::Status Put(std::shared_ptr<const internal::FhirJson> parent_bundle,
                   const internal::FhirJson& resource_json) {
    FHIR_ASSIGN_OR_RETURN(std::string resource_url,
                          GetResourceUrl(resource_json));
    resources_by_uri_.try_emplace(std::move(resource_url),
                                  std::move(parent_bundle));
    return absl::OkStatus();
  }

  // Retrieves the resource for `uri`. Parses the JSON supplied in previous
  // Put calls and returns the resulting proto. The protos are cached, so
  // re-accessing the same resource will not result in another parse from the
  // initial JSON.
  absl::StatusOr<const T*> Get(absl::string_view uri) {
    const auto itr = resources_by_uri_.find(uri);
    if (itr == resources_by_uri_.end()) {
      return absl::NotFoundError(
          absl::StrFormat("%s not present in collection", uri));
    }

    return ParseResourceIfNotParsed(uri, itr->second);
  }

 private:
  // If the given resource has already been parsed into a proto, returns a
  // pointer to that proto. If it has not, mutates the resource by replacing the
  // JSON representation of the resource with a proto representation.
  static absl::StatusOr<const T*> ParseResourceIfNotParsed(
      absl::string_view uri, MaybeParsedResource& resource) {
    if (std::unique_ptr<const T>* parsed =
            std::get_if<std::unique_ptr<const T>>(&resource)) {
      return parsed->get();
    }
    return ParseResource(uri, resource);
  }

  // Parses the JSON into a proto and replaces the un-parsed JSON with the
  // parsed proto representation.
  static absl::StatusOr<const T*> ParseResource(absl::string_view uri,
                                                MaybeParsedResource& resource) {
    FHIR_ASSIGN_OR_RETURN(
        std::unique_ptr<const T> parsed,
        ParseProtoFromJson(
            uri,
            *std::get<std::shared_ptr<const internal::FhirJson>>(resource)));

    resource = std::move(parsed);
    return std::get<std::unique_ptr<const T>>(resource).get();
  }

  // Retrieves the resource JSON for `uri` from the given `parsed_json` blob
  // and parses the JSON into a protocol buffer.
  static absl::StatusOr<std::unique_ptr<const T>> ParseProtoFromJson(
      absl::string_view uri, const internal::FhirJson& parsed_json) {
    FHIR_ASSIGN_OR_RETURN(std::string resource_type,
                          internal::GetResourceType(parsed_json));
    std::unique_ptr<T> resource = std::make_unique<T>();

    if (resource_type != T::descriptor()->name()) {
      if (resource_type == "Bundle") {
        FHIR_ASSIGN_OR_RETURN(const internal::FhirJson* bundle_json,
                              FindResourceInBundle(uri, parsed_json));
        FHIR_RETURN_IF_ERROR(google::fhir::r4::MergeJsonFhirObjectIntoProto(
            *bundle_json, resource.get(), absl::UTCTimeZone(), true));
        return std::move(resource);
      } else {
        return absl::InvalidArgumentError(
            absl::Substitute("Type mismatch in ResourceCollection:  Collection "
                             "is of type $0, but JSON resource is of type $1",
                             T::descriptor()->name(), resource_type));
      }
    }
    FHIR_RETURN_IF_ERROR(google::fhir::r4::MergeJsonFhirObjectIntoProto(
        parsed_json, resource.get(), absl::UTCTimeZone(), true));
    return std::move(resource);
  }

  // A map between URIs and the resource for that URI. Elements begin as
  // JSON and are replaced with protos of type T as they are accessed.
  // For resources inside bundles, the JSON will be the bundle's JSON rather
  // than the resource itself. Because bundles can be associated with
  // multiple resources, we use a shared_ptr.
  absl::flat_hash_map<std::string, MaybeParsedResource> resources_by_uri_;
};

// Struct representing a FHIR Proto package. This is constructed from an archive
// containing these files, as generated by the `fhir_package` rule
// in protogen.bzl.
// TODO: Support versions other than R4
// TODO: package metadata from the NPM package format.
struct FhirPackage {
  ResourceCollection<google::fhir::r4::core::StructureDefinition>
      structure_definitions;
  ResourceCollection<google::fhir::r4::core::SearchParameter> search_parameters;
  ResourceCollection<google::fhir::r4::core::CodeSystem> code_systems;
  ResourceCollection<google::fhir::r4::core::ValueSet> value_sets;

  // Builds a FhirPackage instance from the archive at `archive_file_path`.
  // Processes each .json file in the archive for future access as protos via
  // GetStructureDefinition, GetCodeSystem, etc. calls.
  static absl::StatusOr<std::unique_ptr<FhirPackage>> Load(
      absl::string_view archive_file_path);

  // Builds a FhirPackage instance from the archive at `archive_file_path`.
  // Callers may pass a `handle_entry` function to supply custom logic for
  // processing entries from the archive. The function be called with the path
  // for each entry in the archive, the contents of said entry and a FhirPackage
  // instance the function is free to modify. This logic will be applied in
  // addition to the default logic performed in calls to `Load` where no
  // `handle_entry` function is provided. That is, `handle_entry` may be used to
  // supply additional logic to that which is already performed in `Load` calls.
  static absl::StatusOr<std::unique_ptr<FhirPackage>> Load(
      absl::string_view archive_file_path,
      std::function<absl::Status(absl::string_view, absl::string_view,
                                 FhirPackage&)>
          handle_entry);

  absl::StatusOr<const google::fhir::r4::core::StructureDefinition*>
  GetStructureDefinition(absl::string_view uri) {
    return this->structure_definitions.Get(uri);
  }

  absl::StatusOr<const google::fhir::r4::core::SearchParameter*>
  GetSearchParameter(absl::string_view uri) {
    return this->search_parameters.Get(uri);
  }

  absl::StatusOr<const google::fhir::r4::core::CodeSystem*> GetCodeSystem(
      absl::string_view uri) {
    return this->code_systems.Get(uri);
  }

  absl::StatusOr<const google::fhir::r4::core::ValueSet*> GetValueSet(
      absl::string_view uri) {
    return this->value_sets.Get(uri);
  }

 private:
  FhirPackage()
      : structure_definitions(
            ResourceCollection<google::fhir::r4::core::StructureDefinition>()),
        search_parameters(
            ResourceCollection<google::fhir::r4::core::SearchParameter>()),
        code_systems(ResourceCollection<google::fhir::r4::core::CodeSystem>()),
        value_sets(ResourceCollection<google::fhir::r4::core::ValueSet>()) {}
};

namespace internal {

// Finds the first ok response from calling `get_resource` on each package in
// `packages` with the given `uri` that matches `version` if provided. Returns a
// status describing each non-ok status for every `get_resource` call if no ok
// response can be found.
template <typename T>
absl::StatusOr<T> SearchPackagesForResource(
    absl::StatusOr<T> (FhirPackage::*get_resource)(absl::string_view),
    const std::vector<std::unique_ptr<FhirPackage>>& packages,
    absl::string_view uri,
    std::optional<absl::string_view> version = std::nullopt) {
  std::vector<std::string> mismatched_versions;
  for (const std::unique_ptr<FhirPackage>& package : packages) {
    absl::StatusOr<T> resource = (*package.*get_resource)(uri);
    if (resource.ok()) {
      if (version == std::nullopt ||
          (*resource)->version().value() == *version) {
        // We found the resource!
        return resource;
      }
      // Report the different version we found.
      mismatched_versions.push_back((*resource)->version().value());
    } else if (resource.status().code() == absl::StatusCode::kNotFound) {
      // The resource wasn't found, so let's keep looking.
      continue;
    } else {
      // An error occurred while looking for the resource, so
      // let's report it.
      return resource.status();
    }
  }
  if (mismatched_versions.empty()) {
    return absl::NotFoundError(
        absl::StrCat(uri, " not found in package manager."));
  } else {
    return absl::NotFoundError(
        absl::StrCat(uri, " not found in package manager. Found version(s): ",
                     absl::StrJoin(mismatched_versions, ", ")));
  }
}

}  // namespace internal

// Manages access to a collection of FhirPackage instances.
// Allows users to add packages to the package manager and then search all of
// them for a particular resource.
class FhirPackageManager {
 public:
  FhirPackageManager() = default;

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
  GetStructureDefinition(absl::string_view uri) const {
    return internal::SearchPackagesForResource(
        &FhirPackage::GetStructureDefinition, packages_, uri);
  }
  absl::StatusOr<const google::fhir::r4::core::StructureDefinition*>
  GetStructureDefinition(absl::string_view uri,
                         absl::string_view version) const {
    return internal::SearchPackagesForResource(
        &FhirPackage::GetStructureDefinition, packages_, uri, version);
  }

  absl::StatusOr<const google::fhir::r4::core::SearchParameter*>
  GetSearchParameter(absl::string_view uri) const {
    return internal::SearchPackagesForResource(&FhirPackage::GetSearchParameter,
                                               packages_, uri);
  }
  absl::StatusOr<const google::fhir::r4::core::SearchParameter*>
  GetSearchParameter(absl::string_view uri, absl::string_view version) const {
    return internal::SearchPackagesForResource(&FhirPackage::GetSearchParameter,
                                               packages_, uri, version);
  }

  absl::StatusOr<const google::fhir::r4::core::CodeSystem*> GetCodeSystem(
      absl::string_view uri) const {
    return internal::SearchPackagesForResource(&FhirPackage::GetCodeSystem,
                                               packages_, uri);
  }
  absl::StatusOr<const google::fhir::r4::core::CodeSystem*> GetCodeSystem(
      absl::string_view uri, absl::string_view version) const {
    return internal::SearchPackagesForResource(&FhirPackage::GetCodeSystem,
                                               packages_, uri, version);
  }

  absl::StatusOr<const google::fhir::r4::core::ValueSet*> GetValueSet(
      absl::string_view uri) const {
    return internal::SearchPackagesForResource(&FhirPackage::GetValueSet,
                                               packages_, uri);
  }
  absl::StatusOr<const google::fhir::r4::core::ValueSet*> GetValueSet(
      absl::string_view uri, absl::string_view version) const {
    return internal::SearchPackagesForResource(&FhirPackage::GetValueSet,
                                               packages_, uri, version);
  }

 private:
  std::vector<std::unique_ptr<FhirPackage>> packages_;
};

}  // namespace google::fhir

#endif  // GOOGLE_FHIR_FHIR_PACKAGE_H_
