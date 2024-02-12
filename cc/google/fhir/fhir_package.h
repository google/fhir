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

#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include "glog/logging.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/time/time.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/r4/json_format.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "proto/google/fhir/proto/r4/core/resources/code_system.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/implementation_guide.pb.h"
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
// lazily parsed into protocol buffers as requested.
template <typename T>
class ResourceCollection {
 public:
  // An iterator over all valid resources contained in the collection. Invalid
  // resources, such as those with malformed JSON, will not be returned by the
  // iterator.
  class ProtoIterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;

    // value_type is an iterator_trait
    using value_type = T;

    ProtoIterator(
        const ResourceCollection* resource_collection,
        typename absl::flat_hash_map<
            std::string,
            std::shared_ptr<const internal::FhirJson>>::const_iterator uri_iter)
        : resource_collection_(resource_collection), uri_iter_(uri_iter) {
      // Skip over any initial invalid entries so it is safe to dereference.
      this->AdvanceUntilValid();
    }

    std::unique_ptr<T> operator*() const {
      std::string uri = uri_iter_->first;
      absl::StatusOr<std::unique_ptr<T>> resource =
          resource_collection_->Get(uri);

      // The AdvanceUntilValid() calls in the constructor and in operator++
      // should prevent us from attempting to dereference an invalid resource.
      CHECK(resource.ok()) << absl::StrFormat(
          "Attempted to access invalid resource from iterator for uri: %s with "
          "error: %s",
          uri, resource.status().message());

      return std::move(*resource);
    }
    ProtoIterator& operator++() {
      ++uri_iter_;
      // Additionally advance past invalid entries so it is safe to dereference.
      this->AdvanceUntilValid();
      return *this;
    }
    bool operator==(const ProtoIterator& other) const {
      // Perform equality checks between iterators at positions with valid
      // resources. This allows an iterator with no remaining valid resources to
      // equal an iterator at the end of uri_iter_.
      return this->NextValidPosistion().uri_iter_ ==
             other.NextValidPosistion().uri_iter_;
    }
    bool operator!=(const ProtoIterator& other) const {
      return this->NextValidPosistion().uri_iter_ !=
             other.NextValidPosistion().uri_iter_;
    }

   private:
    // Advances uri_iter_ until it points to a valid, parse-able resource. If it
    // currently points to a valid resource, does not advance the iterator.
    void AdvanceUntilValid() {
      while (true) {
        if (this->UriIterExhausted()) {
          return;
        }
        absl::StatusOr<std::unique_ptr<T>> resource =
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
    ProtoIterator NextValidPosistion() const {
      // The constructor calls .AdvanceUntilValid(), so we know the new
      // iterator is pointing to a valid resource.
      return ProtoIterator(this->resource_collection_, this->uri_iter_);
    }
    // Indicates if the URIs being iterated over have been exhausted.
    bool UriIterExhausted() const {
      return uri_iter_ == resource_collection_->resources_by_uri_.end();
    }

    const ResourceCollection* resource_collection_;
    typename absl::flat_hash_map<
        std::string, std::shared_ptr<const internal::FhirJson>>::const_iterator
        uri_iter_;
  };

  // An iterator over the raw JSON contained in the resource collection.
  class JsonIterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = const internal::FhirJson*;

    explicit JsonIterator(
        typename absl::flat_hash_map<
            std::string,
            std::shared_ptr<const internal::FhirJson>>::const_iterator
            resources_iter)
        : resources_iter_(resources_iter) {}

    const internal::FhirJson* operator*() const {
      absl::string_view url = resources_iter_->first;
      const internal::FhirJson* resource = resources_iter_->second.get();

      // The JSON will either be for an individual resource or a bundle. If it's
      // an individual resource, return that JSON. If it's for a bundle, return
      // the JSON for the resource with the given `url` found inside the bundle.
      // Otherwise, we'll return the same JSON for the same bundle once for
      // every resource inside it.
      // For a resource with a resourceType of 'Bundle,' return the child
      // resource with `url` found inside that bundle. For all other resources,
      // return `resource` as-is.
      absl::StatusOr<const internal::FhirJson*> resource_type_json =
          resource->get("resourceType");
      if (!resource_type_json.ok()) {
        return resource;
      }

      absl::StatusOr<std::string> resource_type =
          (*resource_type_json)->asString();
      if (!resource_type.ok()) {
        return resource;
      }

      if (*resource_type != "Bundle") {
        return resource;
      }

      absl::StatusOr<const internal::FhirJson*> resource_from_bundle =
          FindResourceInBundle(url, *resource);
      if (!resource_from_bundle.ok()) {
        return resource;
      }

      return *resource_from_bundle;
    }

    JsonIterator& operator++() {
      ++resources_iter_;
      return *this;
    }

    bool operator==(const JsonIterator& other) const {
      return resources_iter_ == other.resources_iter_;
    }

    bool operator!=(const JsonIterator& other) const {
      return resources_iter_ != other.resources_iter_;
    }

    typename absl::flat_hash_map<
        std::string, std::shared_ptr<const internal::FhirJson>>::const_iterator
        resources_iter_;
  };

  ResourceCollection() = default;
  ResourceCollection(const ResourceCollection& other) = default;
  ResourceCollection(ResourceCollection&& other) = default;

  ResourceCollection& operator=(const ResourceCollection& other) = default;
  ResourceCollection& operator=(ResourceCollection&& other) = default;

  // Returns an iterator over the protocol buffer representation of all valid
  // resources in the collection.
  ProtoIterator begin() const {
    return ProtoIterator(this, this->resources_by_uri_.cbegin());
  }
  ProtoIterator end() const {
    return ProtoIterator(this, this->resources_by_uri_.cend());
  }

  // Returns an iterator over the raw JSON representation of all resources in
  // the collection.
  JsonIterator json_begin() const {
    return JsonIterator(this->resources_by_uri_.cbegin());
  }

  JsonIterator json_end() const {
    return JsonIterator(this->resources_by_uri_.cend());
  }

  typename absl::flat_hash_map<
      std::string, std::shared_ptr<const internal::FhirJson>>::size_type
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
  // Put calls and returns the resulting proto.
  absl::StatusOr<std::unique_ptr<T>> Get(absl::string_view uri) const {
    const auto itr = resources_by_uri_.find(uri);
    if (itr == resources_by_uri_.end()) {
      return absl::NotFoundError(
          absl::StrFormat("%s not present in collection", uri));
    }

    return ParseProtoFromJson(uri, *itr->second);
  }

 private:
  // Retrieves the resource JSON for `uri` from the given `parsed_json` blob
  // and parses the JSON into a protocol buffer.
  static absl::StatusOr<std::unique_ptr<T>> ParseProtoFromJson(
      absl::string_view uri, const internal::FhirJson& parsed_json) {
    FHIR_ASSIGN_OR_RETURN(std::string resource_type,
                          internal::GetResourceType(parsed_json));
    auto resource = std::make_unique<T>();
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

  // A map between URIs and the JSON of the resource for that URI. For resources
  // inside bundles, the JSON will be the bundle's JSON rather than the resource
  // itself. Because bundles can be associated with multiple resources, we use a
  // shared_ptr.
  absl::flat_hash_map<std::string, std::shared_ptr<const internal::FhirJson>>
      resources_by_uri_;
};

// Struct representing a FHIR Proto package. This is constructed from an archive
// containing these files, as generated by the `fhir_package` rule
// in protogen.bzl.
// TODO(b/204395046): Support versions other than R4
// TODO(b/235876918): package metadata from the NPM package format.
struct FhirPackage {
  ResourceCollection<google::fhir::r4::core::StructureDefinition>
      structure_definitions;
  ResourceCollection<google::fhir::r4::core::SearchParameter> search_parameters;
  ResourceCollection<google::fhir::r4::core::CodeSystem> code_systems;
  ResourceCollection<google::fhir::r4::core::ValueSet> value_sets;
  std::unique_ptr<google::fhir::r4::core::ImplementationGuide>
      implementation_guide;

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

  absl::StatusOr<std::unique_ptr<google::fhir::r4::core::StructureDefinition>>
  GetStructureDefinition(absl::string_view uri) const {
    return this->structure_definitions.Get(uri);
  }

  absl::StatusOr<std::unique_ptr<google::fhir::r4::core::SearchParameter>>
  GetSearchParameter(absl::string_view uri) const {
    return this->search_parameters.Get(uri);
  }

  absl::StatusOr<std::unique_ptr<google::fhir::r4::core::CodeSystem>>
  GetCodeSystem(absl::string_view uri) const {
    return this->code_systems.Get(uri);
  }

  absl::StatusOr<std::unique_ptr<google::fhir::r4::core::ValueSet>> GetValueSet(
      absl::string_view uri) const;

 private:
  FhirPackage()
      : structure_definitions(
            ResourceCollection<google::fhir::r4::core::StructureDefinition>()),
        search_parameters(
            ResourceCollection<google::fhir::r4::core::SearchParameter>()),
        code_systems(ResourceCollection<google::fhir::r4::core::CodeSystem>()),
        value_sets(ResourceCollection<google::fhir::r4::core::ValueSet>()) {}
};

}  // namespace google::fhir

#endif  // GOOGLE_FHIR_FHIR_PACKAGE_H_
