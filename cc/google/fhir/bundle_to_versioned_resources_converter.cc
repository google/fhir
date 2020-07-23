// Copyright 2018 Google LLC
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

#include "google/fhir/bundle_to_versioned_resources_converter.h"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/reflection.h"
#include "absl/flags/flag.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/types.h"

ABSL_FLAG(std::string, fhir_version_config, "version_config.textproto",
          "Configuration file detailing how to split fhir resources into units "
          "on a patient timeline, and which supporting resources to include.");

namespace google {
namespace fhir {

using ::google::fhir::proto::ResourceConfig;
using ::google::fhir::proto::VersionConfig;
using ::google::protobuf::Message;

namespace internal {

const absl::Duration kGapAfterLastEncounterForUnknownDeath =
    absl::Hours(24) * 365;
const absl::Duration kGapBeforeFirstEncounterForUnknownBirth = absl::Hours(24);

std::vector<ResourceConfig::TimestampOverride> ExpandIfNecessary(
    const ResourceConfig::TimestampOverride& original_override,
    const Message& resource) {
  const std::string& timestamp_path = original_override.timestamp_field();
  const std::size_t brackets_location = timestamp_path.find("[]");
  if (brackets_location == std::string::npos) {
    // There are no repeated specs, so we don't need to expand.
    return {original_override};
  }
  CHECK_EQ(std::string::npos, timestamp_path.find("[]", brackets_location + 2))
      << "Only one level of branching is currently supported.  Found: "
      << timestamp_path;

  // Get the parent of the repeated field.
  const std::size_t dot_before_brackets =
      timestamp_path.find_last_of('.', brackets_location);
  const std::string parent_path = timestamp_path.substr(0, dot_before_brackets);
  // Get the repeated field name
  const std::string repeated_field_name = timestamp_path.substr(
      dot_before_brackets + 1, brackets_location - dot_before_brackets - 1);

  if (!HasSubmessageByPath(resource, parent_path).ValueOrDie()) {
    // The parent isn't populated, so this config is irrelevant
    return {};
  }

  // Get the size of the repeated field.
  const Message* parent =
      GetSubmessageByPath(resource, parent_path).ValueOrDie();
  const auto* parent_descriptor = parent->GetDescriptor();
  const auto* repeated_field_descriptor =
      parent_descriptor->FindFieldByCamelcaseName(repeated_field_name);
  const int repeated_field_size =
      parent->GetReflection()->FieldSize(*parent, repeated_field_descriptor);

  // Ensure all the resource_fields have the same repeated field (and no others)
  const std::string repeated_field_path =
      absl::StrCat(parent_path, ".", repeated_field_name, "[]");
  for (const std::string& field_path : original_override.resource_field()) {
    CHECK(absl::StartsWith(field_path, repeated_field_path))
        << "Error in expanding timestamp_override_config: Field " << field_path
        << " does not start with " << repeated_field_path;
  }

  // Now that we know the size of the repeated field, generate an override
  // config for each index.
  const int insert_position = repeated_field_path.length() - 1;
  std::vector<ResourceConfig::TimestampOverride> expanded_overrides;
  for (int i = 0; i < repeated_field_size; i++) {
    ResourceConfig::TimestampOverride indexed_override(original_override);
    indexed_override.mutable_timestamp_field()->insert(insert_position,
                                                       std::to_string(i));
    for (int j = 0; j < indexed_override.resource_field_size(); j++) {
      indexed_override.mutable_resource_field(j)->insert(insert_position,
                                                         std::to_string(i));
    }
    expanded_overrides.push_back(indexed_override);
  }
  return expanded_overrides;
}

}  // namespace internal

const VersionConfig VersionConfigFromFlags() {
  VersionConfig result;
  TF_CHECK_OK(::tensorflow::ReadTextProto(
      ::tensorflow::Env::Default(), absl::GetFlag(FLAGS_fhir_version_config),
      &result));
  return result;
}

}  // namespace fhir
}  // namespace google
