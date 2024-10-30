// Copyright 2019 Google LLC
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

#include "google/fhir/core_resource_registry.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "google/protobuf/descriptor.pb.h"
#include "absl/container/node_hash_map.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/annotations.h"
#include "google/fhir/status/statusor.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource.pb.h"

namespace google {
namespace fhir {

using ::absl::InvalidArgumentError;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

namespace {

// TODO(b/268518264): Clean up support for profiled protos from core libraries.

// Given a resource descriptor, return the base resource in the core FHIR spec.
// If given a core resource, will return its own url.
absl::StatusOr<std::string> GetCoreStructureDefinition(
    const Descriptor* descriptor) {
  static constexpr absl::string_view kCorePrefix =
      "http://hl7.org/fhir/StructureDefinition/";

  const std::string url = GetStructureDefinitionUrl(descriptor);
  if (absl::StartsWith(url, kCorePrefix)) {
    return url;
  }

  for (int i = 0;
       i < descriptor->options().ExtensionSize(proto::fhir_profile_base); i++) {
    const std::string& profile_base =
        descriptor->options().GetExtension(proto::fhir_profile_base, i);
    if (absl::StartsWith(profile_base, kCorePrefix)) {
      return profile_base;
    }
  }
  return InvalidArgumentError(absl::StrCat("Unable to find core resource for: ",
                                           descriptor->full_name()));
}

// For a given ContainedResource version, returns a registry from resource url
// to an default resource message of that type, for all types in the
// ContainedResource
template <typename ContainedResourceLike>
std::unordered_map<std::string, std::unique_ptr<Message>> BuildRegistry() {
  const ContainedResourceLike contained = ContainedResourceLike();
  const Descriptor* descriptor = contained.GetDescriptor();
  const Reflection* reflection = contained.GetReflection();

  std::unordered_map<std::string, std::unique_ptr<Message>> registry;

  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    const Descriptor* field_type = field->message_type();

    registry[GetStructureDefinitionUrl(field_type)] =
        absl::WrapUnique(reflection->GetMessage(contained, field).New());
  }
  return registry;
}

template <typename ContainedResourceLike>
absl::StatusOr<std::unique_ptr<::google::protobuf::Message>>
GetBaseResourceInstanceForVersion(const ::google::protobuf::Descriptor* descriptor) {
  static const std::unordered_map<std::string, std::unique_ptr<Message>>
      registry = BuildRegistry<ContainedResourceLike>();

  FHIR_ASSIGN_OR_RETURN(const std::string& core_url,
                        GetCoreStructureDefinition(descriptor));
  auto example_iter = registry.find(core_url);

  if (example_iter == registry.end()) {
    return InvalidArgumentError(
        absl::StrCat("Unrecognized core Structure Definition Url: ", core_url));
  }
  return absl::WrapUnique(example_iter->second->New());
}

absl::StatusOr<std::unique_ptr<::google::protobuf::Message>>
GetBaseResourceInstanceFromDescriptor(const Descriptor* descriptor) {
  switch (GetFhirVersion(descriptor)) {
    case proto::R4:
      return GetBaseResourceInstanceForVersion<r4::core::ContainedResource>(
          descriptor);
    default:
      return InvalidArgumentError(absl::StrCat(
          "Unsupported FHIR Version for core_resource_registry for resource: ",
          descriptor->full_name()));
  }
}

}  // namespace

absl::StatusOr<std::unique_ptr<::google::protobuf::Message>> GetBaseResourceInstance(
    const ::google::protobuf::Message& message) {
  return GetBaseResourceInstanceFromDescriptor(message.GetDescriptor());
}

absl::StatusOr<const Descriptor*> GetBaseResourceDescriptor(
    const ::google::protobuf::Descriptor* descriptor) {
  FHIR_ASSIGN_OR_RETURN(std::unique_ptr<Message> instance,
                        GetBaseResourceInstanceFromDescriptor(descriptor));
  return instance->GetDescriptor();
}

}  // namespace fhir
}  // namespace google
