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

#include "google/fhir/stu3/bundle_to_versioned_resources_converter.h"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/reflection.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/stu3/proto_util.h"
#include "google/fhir/stu3/util.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/lib/hash/hash.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/platform/logging.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;

using ::google::fhir::stu3::proto::Bundle;
using ::google::fhir::stu3::proto::ContainedResource;
using ::google::fhir::stu3::proto::DateTime;
using ::google::fhir::stu3::proto::Encounter;
using ::google::fhir::stu3::proto::Instant;
using ::google::fhir::stu3::proto::Meta;
using ::google::fhir::stu3::proto::Patient;
using ::google::fhir::stu3::proto::ResourceConfig;
using ::google::fhir::stu3::proto::VersionConfig;
using ::google::protobuf::Descriptor;
using ::google::protobuf::Message;

namespace {

static const absl::Duration kGapAfterLastEncounterForUnknownDeath =
    absl::Hours(24) * 365;
static const absl::Duration kGapBeforeFirstEncounterForUnknownBirth =
    absl::Hours(24);

struct DateTimeHash {
  size_t operator()(const DateTime& date_time) const {
    ::tensorflow::uint64 h = std::hash<uint64_t>()(date_time.value_us());
    h = tensorflow::Hash64Combine(h, std::hash<string>()(date_time.timezone()));
    h = tensorflow::Hash64Combine(
        h, std::hash<int32_t>()(date_time.precision()));
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

// Finds the Encounter in the bundle with the last end time.
// Returns ::tensorflow::errors:NotFoundError if there are no encounters
// in the bundle.
StatusOr<const DateTime*> GetEndOfLastEncounter(const Bundle& bundle) {
  const Encounter* last_encounter = nullptr;

  for (const auto& entry : bundle.entry()) {
    const auto& resource = entry.resource();
    if (resource.has_encounter() &&
        (last_encounter == nullptr ||
         resource.encounter().period().end().value_us() >
             last_encounter->period().end().value_us())) {
      last_encounter = &resource.encounter();
    }
  }
  if (last_encounter == nullptr) {
    return ::tensorflow::errors::NotFound("No encounter found");
  }
  return &(last_encounter->period().end());
}

// Finds the Encounter in the bundle with the earliest start time.
// Returns ::tensorflow::errors:NotFoundError if there are no encounters
// in the bundle.
StatusOr<const DateTime*> GetStartOfFirstEncounter(const Bundle& bundle) {
  const Encounter* first_encounter = nullptr;

  for (const auto& entry : bundle.entry()) {
    const auto& resource = entry.resource();
    if (resource.has_encounter() &&
        (first_encounter == nullptr ||
         resource.encounter().period().start().value_us() <
             first_encounter->period().start().value_us())) {
      first_encounter = &resource.encounter();
    }
  }
  if (first_encounter == nullptr) {
    return ::tensorflow::errors::NotFound("No encounter found");
  }
  return &(first_encounter->period().start());
}

// Gets the default date time to use for fields in this message.
// Any fields that aren't present in a TimestampOverride config will be entered
// at this default time.
const DateTime GetDefaultDateTime(const ResourceConfig& config,
                                  const Message& message,
                                  const Bundle& bundle) {
  // Try to find a field indicated by default_timestamp_field.
  if (config.default_timestamp_fields_size() > 0) {
    // Find default datetime from default_timestamp_field in config
    for (const string& default_timestamp_field :
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

  LOG(FATAL) << "default_timestamp_field does not resolve.\nConfig: "
             << config.DebugString() << "\nMessage: " << message.DebugString();
}

// Stamps a resource with a version + time metadata,
// Wraps it into a ContainedResource, and
// Adds it along with a time to a versioned resources list
// Works for any Time-like fhir element that has a value_us, timezone,
// and precision, e.g.: Date, DateTime, Instant.
template <typename T, typename R>
void StampWrapAndAdd(std::vector<ContainedResource>* versioned_resources,
                     const T& timelike, const int version_id, R* resource) {
  Meta* meta = MutableMetadataFromResource(resource);
  meta->mutable_version_id()->set_value(absl::StrCat(version_id));
  Instant* last_updated = meta->mutable_last_updated();
  last_updated->set_timezone(timelike.timezone());

  // NOTE: We can't directly set last_updated.precision from timelike.precision,
  // because enums are not shared between Time-like FHIR elements.
  // But they share some string names, so we can try to translate between them.
  // The most coarse precision available to Instant is SECOND, though, so
  // anything more coarse (e.g., Date measured in DAY) will require us to
  // "round up" value to the nearest unit of precision avoid any leaks.
  Instant::Precision precision;
  const bool parse_successful = Instant::Precision_Parse(
      T::Precision_Name(timelike.precision()), &precision);
  if (parse_successful) {
    last_updated->set_value_us(timelike.value_us());
    last_updated->set_precision(precision);
  } else {
    last_updated->set_precision(Instant::SECOND);
    absl::TimeZone tz;
    TF_CHECK_OK(GetTimezone(timelike.timezone(), &tz))
        << "No Timezone on timelike: " << timelike.DebugString();
    const string precision_string = T::Precision_Name(timelike.precision());
    const auto& breakdown = GetTimeFromTimelikeElement(timelike).In(tz);
    if (precision_string == "DAY") {
      last_updated->set_value_us(absl::ToUnixMicros(
          absl::FromDateTime(breakdown.year, breakdown.month, breakdown.day + 1,
                             0, 0, 0, tz) -
          absl::Seconds(1)));
    } else if (precision_string == "MONTH") {
      last_updated->set_value_us(absl::ToUnixMicros(
          absl::FromDateTime(breakdown.year, breakdown.month + 1, 1, 0, 0, 0,
                             tz) -
          absl::Seconds(1)));
    } else if (precision_string == "YEAR") {
      last_updated->set_value_us(absl::ToUnixMicros(
          absl::FromDateTime(breakdown.year + 1, 1, 1, 0, 0, 0, tz) -
          absl::Seconds(1)));
    } else {
      LOG(FATAL) << "Unknown time precision: "
                 << T::Precision_Name(timelike.precision());
    }
  }
  versioned_resources->push_back(WrapContainedResource(*resource).ValueOrDie());
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
  const string& timestamp_path = original_override.timestamp_field();
  const std::size_t brackets_location = timestamp_path.find("[]");
  if (brackets_location == string::npos) {
    // There are no repeated specs, so we don't need to expand.
    return {original_override};
  }
  CHECK_EQ(string::npos, timestamp_path.find("[]", brackets_location + 2))
      << "Only one level of branching is currently supported.  Found: "
      << timestamp_path;

  // Get the parent of the repeated field.
  const std::size_t dot_before_brackets =
      timestamp_path.find_last_of('.', brackets_location);
  const string parent_path = timestamp_path.substr(0, dot_before_brackets);
  // Get the repeated field name
  const string repeated_field_name = timestamp_path.substr(
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
  const string repeated_field_path =
      absl::StrCat(parent_path, ".", repeated_field_name, "[]");
  for (const string& field_path : original_override.resource_field()) {
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

const std::vector<std::pair<DateTime, std::unordered_set<string>>>
GetSortedOverrides(const ResourceConfig& resource_config,
                   const Message& resource) {
  // First off, build a map from timestamp override time to fields that
  // should be introduced at that time.
  // Also builds a map of all fields with overrides, to ensure fields aren't
  // duplicated.
  // DateTimeHash/Equiv are only using 3 fields (value_us, timezone, and
  // precision) not all the fields.
  std::unordered_map<DateTime, std::unordered_set<string>, DateTimeHash,
                     DateTimeEquiv>
      override_to_fields_map;
  std::unordered_set<string> fields_with_override;
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

      for (const string& field : ts_override.resource_field()) {
        CHECK(fields_with_override.insert(field).second)
            << "Duplicate timestamp overrides for field " << field;
        override_to_fields_map[*time_status.ValueOrDie()].insert(field);
      }
    }
  }
  // Sort the overrides map in descending order,
  // because we'll use it to strip out fields as we move back in time
  // (most recent version has most data, earliest version has least data).
  std::vector<std::pair<DateTime, std::unordered_set<string>>>
      sorted_override_to_fields_pairs(override_to_fields_map.begin(),
                                      override_to_fields_map.end());
  std::sort(sorted_override_to_fields_pairs.begin(),
            sorted_override_to_fields_pairs.end(),
            [](const std::pair<DateTime, std::unordered_set<string>>& a,
               const std::pair<DateTime, std::unordered_set<string>>& b) {
              return a.first.value_us() < b.first.value_us();
            });
  return sorted_override_to_fields_pairs;
}

template <typename R>
void SplitResource(const R& resource, const Bundle& bundle,
                   const VersionConfig& config,
                   std::vector<ContainedResource>* versioned_resources) {
  // Make a mutable copy of the resource, that we will modify and copy into
  // the result vector along the way.
  std::unique_ptr<R> current_resource(resource.New());
  current_resource->CopyFrom(resource);

  const Descriptor* descriptor = resource.GetDescriptor();
  const string& resource_name = descriptor->name();

  const auto& iter = config.resource_config().find(resource_name);
  CHECK(iter != config.resource_config().end())
      << "Missing resource config for resource type " << resource_name;
  const auto& resource_config = iter->second;

  DateTime default_time = GetDefaultDateTime(resource_config, resource, bundle);

  if (resource_config.timestamp_override_size() == 0) {
    // If there are no per-field timestamp overrides, we can just enter the
    // whole resource in at the default time.
    StampWrapAndAdd(versioned_resources, default_time, 0,
                    current_resource.get());
    return;
  }

  // There are per-field timestamp overrides.
  // We're need to produce versions of the resource for each override
  // time field listed.
  // First off, get a list of override pairs, where each pair is
  // override time -> fields that should use that time, sorted in ascending
  // order.
  const std::vector<std::pair<DateTime, std::unordered_set<string>>>&
      sorted_overrides = GetSortedOverrides(resource_config, resource);

  // Strip out all fields with overrides, to get the "base" version.
  // This is the version that goes in at the "default" time.
  // TODO: This operates under the assumption that the default time
  // is the _earliest_ available time.  To safeguard against leaks, it should be
  // the _latest_ available time.  In that case, the "default" time version will
  // just be the original proto.
  for (const auto& override_map_entry : sorted_overrides) {
    for (const string& field_path : override_map_entry.second) {
      if (EndsInIndex(field_path)) {
        // This points to a specific index in a repeated field.
        // We can't just clear an index, but that's ok because the whole field
        // will have overrides, so just clear it all.
        CHECK(ClearFieldByPath(current_resource.get(), StripIndex(field_path))
                  .ok());
      } else {
        CHECK(ClearFieldByPath(current_resource.get(), field_path).ok());
      }
    }
  }
  int version = 0;
  StampWrapAndAdd(versioned_resources, default_time, version++,
                  current_resource.get());

  for (const auto& override_map_entry : sorted_overrides) {
    // Add back any fields that became available at this time.
    for (const string& field_path : override_map_entry.second) {
      const auto& original_submessage_status =
          GetSubmessageByPath(resource, field_path);
      if (original_submessage_status.status().code() ==
          ::tensorflow::error::Code::NOT_FOUND) {
        // Nothing to copy.
        continue;
      }
      if (original_submessage_status.ok()) {
        // We were able to resolve a specific message with this path.
        // This means either
        // A) It's an indexed path to a specific message within a repeated field
        // or  B) it's a path to a singular field.
        const Message& original_submessage =
            *original_submessage_status.ValueOrDie();
        if (EndsInIndex(field_path)) {
          // It's an indexed path to a specific message within a repeated field.
          // Add just that message to the end of the destination repeated field.
          const std::size_t last_dot_index = field_path.find_last_of('.');
          const string parent_path = field_path.substr(0, last_dot_index);
          const string field_name = field_path.substr(last_dot_index + 1);
          Message* destination_parent_field =
              GetMutableSubmessageByPath(current_resource.get(), parent_path)
                  .ValueOrDie();
          destination_parent_field->GetReflection()
              ->AddMessage(
                  destination_parent_field,
                  destination_parent_field->GetDescriptor()
                      ->FindFieldByCamelcaseName(StripIndex(field_name)))
              ->CopyFrom(original_submessage);
        } else {
          // It's a path to a singular field.
          // Just copy from source to destination.
          Message* destination_field =
              GetMutableSubmessageByPath(current_resource.get(), field_path)
                  .ValueOrDie();
          destination_field->CopyFrom(original_submessage);
        }
      } else {
        // Couldn't resolve a single submessage, so this must be a (unindexed)
        // path to an entire repeated field.
        // Copy over the entire contents.
        const std::size_t last_dot_index = field_path.find_last_of('.');
        const string parent_path = field_path.substr(0, last_dot_index);
        const string field_name = field_path.substr(last_dot_index + 1);
        if (!HasSubmessageByPath(resource, parent_path).ValueOrDie()) {
          // The parent field is unpopulated on the source message, so there's
          // nothing to copy.
          continue;
        }
        const Message* source_parent_message =
            GetSubmessageByPath(resource, parent_path).ValueOrDie();
        const google::protobuf::RepeatedFieldRef<Message>& source_repeated_field =
            source_parent_message->GetReflection()
                ->GetRepeatedFieldRef<Message>(
                    *source_parent_message,
                    source_parent_message->GetDescriptor()
                        ->FindFieldByCamelcaseName(field_name));

        Message* target_parent_message =
            GetMutableSubmessageByPath(current_resource.get(), parent_path)
                .ValueOrDie();
        const google::protobuf::MutableRepeatedFieldRef<Message>& target_repeated_field =
            target_parent_message->GetReflection()
                ->GetMutableRepeatedFieldRef<Message>(
                    target_parent_message,
                    target_parent_message->GetDescriptor()
                        ->FindFieldByCamelcaseName(field_name));

        target_repeated_field.CopyFrom(source_repeated_field);
      }
    }
    DateTime override_time = override_map_entry.first;
    // TODO: Once we've switched the version config to use latest
    // timestamp as default, enforce that here by failing on any override time
    // greater than the default (accounting for precision).
    StampWrapAndAdd(versioned_resources, override_map_entry.first, version++,
                    current_resource.get());
  }
}

void SplitPatient(Patient patient, const Bundle& bundle,
                  const VersionConfig& config,
                  std::vector<ContainedResource>* versioned_resources,
                  std::map<string, int>* counter_stats) {
  if (patient.deceased().has_date_time()) {
    // We know when the patient died.  Add two versions of the patient:
    // One at time-of-death, with the death information, and
    // one at the initial time with everything except time-of-death.
    StampWrapAndAdd(versioned_resources, patient.deceased().date_time(), 1,
                    &patient);
  } else if (patient.deceased().boolean().value()) {
    // All we know is that the patient died.
    StatusOr<const DateTime*> last_time = GetEndOfLastEncounter(bundle);
    if (last_time.ok()) {
      // Use end of last encounter as time of death, with a delay to put it
      // a safe distance in the future.
      DateTime input_time(*last_time.ValueOrDie());
      input_time.set_value_us(
          last_time.ValueOrDie()->value_us() +
          absl::ToInt64Microseconds(kGapAfterLastEncounterForUnknownDeath));
      StampWrapAndAdd(versioned_resources, input_time, 1, &patient);
    } else {
      // The patient died, but we have zero knowledge of when, so we can't
      // emit a version.  Make sure to count this with a needs-attention.
      (*counter_stats)["patient-unknown-death-time-needs-attention"]++;
    }
  }
  patient.clear_deceased();
  if (patient.has_birth_date()) {
    // Add initial version at time of birth
    // TODO: Since Birth Date is (maximum) day-level precision,
    // set back a single unit of precision to guarantee it is the first entry
    // into the system. Otherwise, e.g., for a patient with birthdate of DAY
    // precision, the v0 patient will be added at the end of the day, which
    // would be after any labs etc. entered on the first day.
    // This guarantees that the patient would be added at 23:59 the day before.
    // Note:  We could just subtract one second, if we knew for a fact that
    // the value_us lines up perfectly with a precision period, but we don't
    // enforce that anywhere yet.

    StampWrapAndAdd(versioned_resources, patient.birth_date(), 0, &patient);
  } else {
    StatusOr<const DateTime*> first_time = GetStartOfFirstEncounter(bundle);
    if (first_time.ok()) {
      // Use start of first encounter as time for V0 of the patient.
      // Subtract a delay to guarantee it is the first entry into the system.
      DateTime input_time(*first_time.ValueOrDie());
      input_time.set_value_us(
          first_time.ValueOrDie()->value_us() -
          absl::ToInt64Microseconds(kGapBeforeFirstEncounterForUnknownBirth));
      StampWrapAndAdd(versioned_resources, input_time, 0, &patient);
    } else {
      // We have no record of when the patient was born or entered the system.
      // This should be pretty uncommon.
      (*counter_stats)["patient-unknown-entry-time-needs-attention"]++;
    }
  }
}

}  // namespace

std::vector<ContainedResource> BundleToVersionedResources(
    const Bundle& bundle, const VersionConfig& config,
    std::map<string, int>* counter_stats) {
  static auto* oneof_resource_descriptor =
      ContainedResource::descriptor()->FindOneofByName("oneof_resource");
  std::vector<ContainedResource> versioned_resources;
  for (const auto& entry : bundle.entry()) {
    if (!entry.has_resource()) continue;
    (*counter_stats)["num-unversioned-resources-in"]++;
    int initial_size = versioned_resources.size();
    const auto& resource = entry.resource();
    if (resource.has_patient()) {
      // Patient is slightly different.
      // The first version timestamp comes from the start time of the first
      // encounter in the system.
      // If the patient died, but there is no death timestamp, we need to reach
      // into the last encounter for the time to use.
      // TODO: if this turns out to be a more common pattern,
      // we could have a way to encode this logic into the config proto as a
      // new kind of override.
      SplitPatient(resource.patient(), bundle, config, &versioned_resources,
                   counter_stats);
    } else if (resource.has_claim()) {
      SplitResource(resource.claim(), bundle, config, &versioned_resources);
    } else if (resource.has_composition()) {
      SplitResource(resource.composition(), bundle, config,
                    &versioned_resources);
    } else if (resource.has_condition()) {
      SplitResource(resource.condition(), bundle, config, &versioned_resources);
    } else if (resource.has_encounter()) {
      SplitResource(resource.encounter(), bundle, config, &versioned_resources);
    } else if (resource.has_medication_administration()) {
      SplitResource(resource.medication_administration(), bundle, config,
                    &versioned_resources);
    } else if (resource.has_medication_request()) {
      SplitResource(resource.medication_request(), bundle, config,
                    &versioned_resources);
    } else if (resource.has_observation()) {
      SplitResource(resource.observation(), bundle, config,
                    &versioned_resources);
    } else if (resource.has_procedure()) {
      SplitResource(resource.procedure(), bundle, config, &versioned_resources);
    } else if (resource.has_procedure_request()) {
      SplitResource(resource.procedure_request(), bundle, config,
                    &versioned_resources);
    }
    const int new_size = versioned_resources.size();
    const string resource_name =
        resource.GetReflection()
            ->GetOneofFieldDescriptor(resource, oneof_resource_descriptor)
            ->name();
    (*counter_stats)[absl::StrCat("num-", resource_name, "-split-to-",
                                  (new_size - initial_size))]++;
  }
  return versioned_resources;
}

Bundle BundleToVersionedBundle(const Bundle& bundle,
                               const VersionConfig& config,
                               std::map<string, int>* counter_stats) {
  const auto& versioned_resources =
      BundleToVersionedResources(bundle, config, counter_stats);
  Bundle output_bundle;
  for (const auto& resource : versioned_resources) {
    *(output_bundle.add_entry()->mutable_resource()) = resource;
  }
  return output_bundle;
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
