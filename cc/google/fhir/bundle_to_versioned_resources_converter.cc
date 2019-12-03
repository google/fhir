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
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/lib/hash/hash.h"
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
using ::google::fhir::stu3::proto::DateTime;
using ::google::protobuf::Message;

namespace internal {

const absl::Duration kGapAfterLastEncounterForUnknownDeath =
    absl::Hours(24) * 365;
const absl::Duration kGapBeforeFirstEncounterForUnknownBirth = absl::Hours(24);

struct DateTimeHash {
  size_t operator()(const DateTime& date_time) const {
    ::tensorflow::uint64 h = std::hash<uint64_t>()(date_time.value_us());
    h = tensorflow::Hash64Combine(
        h, std::hash<std::string>()(date_time.timezone()));
    h = tensorflow::Hash64Combine(h,
                                  std::hash<int32_t>()(date_time.precision()));
    return h;
  }
};

struct DateTimeEquiv {
  bool operator()(const DateTime& lhs, const DateTime& rhs) const {
    return lhs.value_us() == rhs.value_us() &&
           lhs.precision() == rhs.precision() &&
           lhs.timezone() == rhs.timezone();
  }
};

// Gets the default date time to use for fields in this message.
// Any fields that aren't present in a TimestampOverride config will be entered
// at this default time.
StatusOr<DateTime> GetDefaultDateTime(const ResourceConfig& config,
                                      const Message& message) {
  // Try to find a field indicated by default_timestamp_field.
  if (config.default_timestamp_fields_size() > 0) {
    // Find default datetime from default_timestamp_field in config
    for (const std::string& default_timestamp_field :
         config.default_timestamp_fields()) {
      const auto& default_date_time_status =
          GetSubmessageByPathAndCheckType<DateTime>(message,
                                                    default_timestamp_field);
      if (default_date_time_status.status().code() !=
          ::tensorflow::error::Code::NOT_FOUND) {
        return *default_date_time_status.ValueOrDie();
      }
    }
  }
  return ::tensorflow::errors::InvalidArgument(
      "split-failed-no-default_timestamp_field-",
      message.GetDescriptor()->name());
}

// Expands a TimestampOverride config for a repeated field.
// Example: a Composition can have several attesters, each of which has a
// timestamp for when that attestment was added.  This can be represented by
//
// timestamp_override {
//   timestamp_field: "Resource.repeatedField[].time"
//   resource_field: "Resource.repeatedField[].fieldToAdd"
//   resource_field: "Resource.repeatedField[].otherFieldToAdd"
// }
//
// indicating that those fields should be added at that repeatedField's time.
// For a field with size N, this will return a vector with N configs,
// each of which will be identical except populating the array brackets with an
// index in 1-N.  E.g., if the above config was expanded with a repeatedField of
// size 2, this would expand to
//
// timestamp_override {
//   timestamp_field: "Resource.repeatedField[0].time"
//   resource_field: "Resource.repeatedField[0].fieldToAdd"
//   resource_field: "Resource.repeatedField[0].otherFieldToAdd"
// }
// timestamp_override {
//   timestamp_field: "Resource.repeatedField[1].time"
//   resource_field: "Resource.repeatedField[1].fieldToAdd"
//   resource_field: "Resource.repeatedField[1].otherFieldToAdd"
// }
//
// It's also fine to add the entire message at that time, e.g.,
// timestamp_override {
//   timestamp_field: "Resource.repeatedField[].time"
//   resource_field: "Resource.repeatedField[]"
// }
//
// This doesn't support arbitrary branching - there must be at most one
// instance of "[]" in the timestamp_field, and if found,
// each resource_field must have an identical branching. I.e., the strings up
// until [] must be identical.
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

const std::vector<std::pair<DateTime, std::unordered_set<std::string>>>
GetSortedOverrides(const ResourceConfig& resource_config,
                   const Message& resource) {
  // First off, build a map from timestamp override time to fields that
  // should be introduced at that time.
  // Also builds a map of all fields with overrides, to ensure fields aren't
  // duplicated.
  // DateTimeHash/Equiv are only using 3 fields (value_us, timezone, and
  // precision) not all the fields.
  std::unordered_map<DateTime, std::unordered_set<std::string>, DateTimeHash,
                     DateTimeEquiv>
      override_to_fields_map;
  std::unordered_set<std::string> fields_with_override;
  for (const auto& orig_ts_override : resource_config.timestamp_override()) {
    std::vector<ResourceConfig::TimestampOverride> expanded_ts_overrides =
        ExpandIfNecessary(orig_ts_override, resource);
    for (const auto& ts_override : expanded_ts_overrides) {
      const auto& time_status = GetSubmessageByPathAndCheckType<DateTime>(
          resource, ts_override.timestamp_field());
      // Ignore if requested override is not present
      if (time_status.status().code() == ::tensorflow::error::Code::NOT_FOUND) {
        continue;
      }

      for (const std::string& field : ts_override.resource_field()) {
        CHECK(fields_with_override.insert(field).second)
            << "Duplicate timestamp overrides for field " << field;
        override_to_fields_map[*time_status.ValueOrDie()].insert(field);
      }
    }
  }
  // Sort the overrides map in descending order,
  // because we'll use it to strip out fields as we move back in time
  // (most recent version has most data, earliest version has least data).
  std::vector<std::pair<DateTime, std::unordered_set<std::string>>>
      sorted_override_to_fields_pairs(override_to_fields_map.begin(),
                                      override_to_fields_map.end());
  std::sort(sorted_override_to_fields_pairs.begin(),
            sorted_override_to_fields_pairs.end(),
            [](const std::pair<DateTime, std::unordered_set<std::string>>& a,
               const std::pair<DateTime, std::unordered_set<std::string>>& b) {
              return a.first.value_us() < b.first.value_us();
            });
  return sorted_override_to_fields_pairs;
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
