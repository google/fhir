/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GOOGLE_FHIR_STU3_BUNDLE_TO_VERSIONED_RESOURCES_CONVERTER_H_
#define GOOGLE_FHIR_STU3_BUNDLE_TO_VERSIONED_RESOURCES_CONVERTER_H_

#include <map>
#include <string>
#include <unordered_set>
#include <utility>

#include "google/protobuf/message.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/type_macros.h"
#include "google/fhir/util.h"
#include "proto/version_config.pb.h"
#include "tensorflow/core/lib/hash/hash.h"

namespace google {
namespace fhir {

namespace internal {

extern const absl::Duration kGapAfterLastEncounterForUnknownDeath;
extern const absl::Duration kGapBeforeFirstEncounterForUnknownBirth;

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
std::vector<proto::ResourceConfig::TimestampOverride> ExpandIfNecessary(
    const proto::ResourceConfig::TimestampOverride& original_override,
    const ::google::protobuf::Message& resource);

// Finds the Encounter in the bundle with the last end time.
// Returns ::tensorflow::errors:NotFoundError if there are no encounters
// in the bundle.
template <typename BundleLike,
          typename EncounterLike = BUNDLE_TYPE(BundleLike, encounter),
          typename DateTimeLike = FHIR_DATATYPE(BundleLike, date_time)>
StatusOr<const DateTimeLike*> GetEndOfLastEncounter(const BundleLike& bundle) {
  const EncounterLike* last_encounter = nullptr;

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
    return ::absl::NotFoundError("No encounter found");
  }
  return &(last_encounter->period().end());
}

// Finds the Encounter in the bundle with the earliest start time.
// Returns ::tensorflow::errors:NotFoundError if there are no encounters
// in the bundle.
template <typename BundleLike,
          typename EncounterLike = BUNDLE_TYPE(BundleLike, encounter),
          typename DateTimeLike = FHIR_DATATYPE(BundleLike, date_time)>
StatusOr<const DateTimeLike*> GetStartOfFirstEncounter(
    const BundleLike& bundle) {
  const EncounterLike* first_encounter = nullptr;

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
    return ::absl::NotFoundError("No encounter found");
  }
  return &(first_encounter->period().start());
}

template <typename DateTimeLike>
struct DateTimeHash {
  size_t operator()(const DateTimeLike& date_time) const {
    ::tensorflow::uint64 h = std::hash<uint64_t>()(date_time.value_us());
    h = tensorflow::Hash64Combine(
        h, std::hash<std::string>()(date_time.timezone()));
    h = tensorflow::Hash64Combine(h,
                                  std::hash<int32_t>()(date_time.precision()));
    return h;
  }
};

// TODO: determine if timezone should be used in this comparison
template <typename DateTimeLike>
struct DateTimeEquiv {
  bool operator()(const DateTimeLike& lhs, const DateTimeLike& rhs) const {
    return lhs.value_us() == rhs.value_us() &&
           lhs.precision() == rhs.precision() &&
           lhs.timezone() == rhs.timezone();
  }
};

template <typename DateTimeLike>
const std::vector<std::pair<DateTimeLike, std::unordered_set<std::string>>>
GetSortedOverrides(const proto::ResourceConfig& resource_config,
                   const ::google::protobuf::Message& resource) {
  // First off, build a map from timestamp override time to fields that
  // should be introduced at that time.
  // Also builds a map of all fields with overrides, to ensure fields aren't
  // duplicated.
  // DateTimeHash/Equiv are only using 3 fields (value_us, timezone, and
  // precision) not all the fields.
  std::unordered_map<DateTimeLike, std::unordered_set<std::string>,
                     DateTimeHash<DateTimeLike>, DateTimeEquiv<DateTimeLike>>
      override_to_fields_map;
  std::unordered_set<std::string> fields_with_override;
  for (const auto& orig_ts_override : resource_config.timestamp_override()) {
    std::vector<proto::ResourceConfig::TimestampOverride>
        expanded_ts_overrides =
            internal::ExpandIfNecessary(orig_ts_override, resource);
    for (const auto& ts_override : expanded_ts_overrides) {
      const auto& time_status = GetSubmessageByPathAndCheckType<DateTimeLike>(
          resource, ts_override.timestamp_field());
      // Ignore if requested override is not present
      if (time_status.status().code() == ::absl::StatusCode::kNotFound) {
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
  std::vector<std::pair<DateTimeLike, std::unordered_set<std::string>>>
      sorted_override_to_fields_pairs(override_to_fields_map.begin(),
                                      override_to_fields_map.end());
  std::sort(
      sorted_override_to_fields_pairs.begin(),
      sorted_override_to_fields_pairs.end(),
      [](const std::pair<DateTimeLike, std::unordered_set<std::string>>& a,
         const std::pair<DateTimeLike, std::unordered_set<std::string>>& b) {
        return a.first.value_us() < b.first.value_us();
      });
  return sorted_override_to_fields_pairs;
}

// Stamps a resource with a version + time metadata,
// Wraps it into a ContainedResource, and
// Adds it along with a time to a versioned resources list
// Works for any Time-like fhir element that has a value_us, timezone,
// and precision, e.g.: Date, DateTime, Instant.
template <typename ContainedResourceLike, typename T,
          typename InstantLike = FHIR_DATATYPE(T, instant)>
void StampWrapAndAdd(std::vector<ContainedResourceLike>* versioned_resources,
                     const T& timelike, const int version_id,
                     ::google::protobuf::Message* resource) {
  ::google::protobuf::Message* meta =
      MutableMessageInField(resource, "meta").ValueOrDie();
  ::google::protobuf::Message* meta_version_id =
      MutableMessageInField(meta, "version_id").ValueOrDie();
  CHECK(
      SetPrimitiveStringValue(meta_version_id, absl::StrCat(version_id)).ok());

  InstantLike* last_updated = dynamic_cast<InstantLike*>(
      MutableMessageInField(meta, "last_updated").ValueOrDie());
  last_updated->set_timezone(timelike.timezone());

  // NOTE: We can't directly set last_updated.precision from timelike.precision,
  // because enums are not shared between Time-like FHIR elements.
  // But they share some string names, so we can try to translate between them.
  // The most coarse precision available to Instant is SECOND, though, so
  // anything more coarse (e.g., Date measured in DAY) will require us to
  // "round up" value to the nearest unit of precision avoid any leaks.
  typename InstantLike::Precision precision;
  const bool parse_successful = InstantLike::Precision_Parse(
      T::Precision_Name(timelike.precision()), &precision);
  if (parse_successful) {
    last_updated->set_value_us(timelike.value_us());
    last_updated->set_precision(precision);
  } else {
    last_updated->set_precision(InstantLike::SECOND);
    absl::TimeZone tz =
        BuildTimeZoneFromString(timelike.timezone()).ValueOrDie();
    const std::string precision_string =
        T::Precision_Name(timelike.precision());
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
  versioned_resources->push_back(
      WrapContainedResource<ContainedResourceLike>(*resource).ValueOrDie());
}

// Gets the default date time to use for fields in this message.
// Any fields that aren't present in a TimestampOverride config will be entered
// at this default time.
template <typename DateTimeLike>
StatusOr<DateTimeLike> GetDefaultDateTime(const proto::ResourceConfig& config,
                                          const ::google::protobuf::Message& message) {
  // Try to find a field indicated by default_timestamp_field.
  if (config.default_timestamp_fields_size() > 0) {
    // Find default datetime from default_timestamp_field in config
    for (const std::string& default_timestamp_field :
         config.default_timestamp_fields()) {
      const auto& default_date_time_status =
          GetSubmessageByPathAndCheckType<DateTimeLike>(
              message, default_timestamp_field);
      if (default_date_time_status.status().code() !=
          ::absl::StatusCode::kNotFound) {
        return *default_date_time_status.ValueOrDie();
      }
    }
  }
  return ::absl::InvalidArgumentError(
      absl::StrCat("split-failed-no-default_timestamp_field-",
                   message.GetDescriptor()->name()));
}

template <
    typename BundleLike,
    typename ContainedResourceLike = BUNDLE_CONTAINED_RESOURCE(BundleLike),
    typename DateTimeLike = FHIR_DATATYPE(BundleLike, date_time)>
void SplitResource(const ::google::protobuf::Message& resource,
                   const proto::VersionConfig& config,
                   std::vector<ContainedResourceLike>* versioned_resources,
                   std::map<std::string, int>* counter_stats) {
  // Make a mutable copy of the resource, that we will modify and copy into
  // the result vector along the way.
  std::unique_ptr<::google::protobuf::Message> current_resource(resource.New());
  current_resource->CopyFrom(resource);

  const ::google::protobuf::Descriptor* descriptor = resource.GetDescriptor();
  const std::string& resource_name = descriptor->name();

  const auto& iter = config.resource_config().find(resource_name);
  if (iter == config.resource_config().end()) {
    (*counter_stats)[absl::StrCat("split-failed-no-config-", resource_name)]++;
    return;
  }
  const auto& resource_config = iter->second;

  StatusOr<DateTimeLike> default_time_status =
      GetDefaultDateTime<DateTimeLike>(resource_config, resource);
  if (!default_time_status.ok()) {
    (*counter_stats)[std::string(default_time_status.status().message())]++;
    return;
  }
  DateTimeLike default_time = default_time_status.ValueOrDie();

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
  const std::vector<std::pair<DateTimeLike, std::unordered_set<std::string>>>&
      sorted_overrides =
          GetSortedOverrides<DateTimeLike>(resource_config, resource);

  // Strip out all fields with overrides, to get the "base" version.
  // This is the version that goes in at the "default" time.
  // TODO: This operates under the assumption that the default time
  // is the _earliest_ available time.  To safeguard against leaks, it should be
  // the _latest_ available time.  In that case, the "default" time version will
  // just be the original proto.
  for (const auto& timestamp_override : resource_config.timestamp_override()) {
    for (const std::string& field_path : timestamp_override.resource_field()) {
      if (field_path.find_last_of('[') == std::string::npos) {
        FHIR_CHECK_OK(ClearFieldByPath(current_resource.get(), field_path));
      } else {
        FHIR_CHECK_OK(
            ClearFieldByPath(current_resource.get(), StripIndex(field_path)));
      }
    }
  }
  int version = 0;
  StampWrapAndAdd(versioned_resources, default_time, version++,
                  current_resource.get());

  for (const auto& override_map_entry : sorted_overrides) {
    // Add back any fields that became available at this time.
    for (const std::string& field_path : override_map_entry.second) {
      const auto& original_submessage_status =
          GetSubmessageByPath(resource, field_path);
      if (original_submessage_status.status().code() ==
          ::absl::StatusCode::kNotFound) {
        // Nothing to copy.
        continue;
      }
      if (original_submessage_status.ok()) {
        // We were able to resolve a specific message with this path.
        // This means either
        // A) It's an indexed path to a specific message within a repeated field
        // or  B) it's a path to a singular field.
        const ::google::protobuf::Message& original_submessage =
            *original_submessage_status.ValueOrDie();
        if (EndsInIndex(field_path)) {
          // It's an indexed path to a specific message within a repeated field.
          // Add just that message to the end of the destination repeated field.
          const std::size_t last_dot_index = field_path.find_last_of('.');
          const std::string parent_path = field_path.substr(0, last_dot_index);
          const std::string field_name = field_path.substr(last_dot_index + 1);
          ::google::protobuf::Message* destination_parent_field =
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
          ::google::protobuf::Message* destination_field =
              GetMutableSubmessageByPath(current_resource.get(), field_path)
                  .ValueOrDie();
          destination_field->CopyFrom(original_submessage);
        }
      } else {
        // Couldn't resolve a single submessage, so this must be a (unindexed)
        // path to an entire repeated field.
        // Copy over the entire contents.
        const std::size_t last_dot_index = field_path.find_last_of('.');
        const std::string parent_path = field_path.substr(0, last_dot_index);
        const std::string field_name = field_path.substr(last_dot_index + 1);
        if (!HasSubmessageByPath(resource, parent_path).ValueOrDie()) {
          // The parent field is unpopulated on the source message, so there's
          // nothing to copy.
          continue;
        }
        const ::google::protobuf::Message* source_parent_message =
            GetSubmessageByPath(resource, parent_path).ValueOrDie();
        const ::google::protobuf::RepeatedFieldRef<::google::protobuf::Message>&
            source_repeated_field =
                source_parent_message->GetReflection()
                    ->GetRepeatedFieldRef<::google::protobuf::Message>(
                        *source_parent_message,
                        source_parent_message->GetDescriptor()
                            ->FindFieldByCamelcaseName(field_name));

        ::google::protobuf::Message* target_parent_message =
            GetMutableSubmessageByPath(current_resource.get(), parent_path)
                .ValueOrDie();
        const ::google::protobuf::MutableRepeatedFieldRef<::google::protobuf::Message>&
            target_repeated_field =
                target_parent_message->GetReflection()
                    ->GetMutableRepeatedFieldRef<::google::protobuf::Message>(
                        target_parent_message,
                        target_parent_message->GetDescriptor()
                            ->FindFieldByCamelcaseName(field_name));

        target_repeated_field.CopyFrom(source_repeated_field);
      }
    }
    DateTimeLike override_time = override_map_entry.first;
    // TODO: Once we've switched the version config to use latest
    // timestamp as default, enforce that here by failing on any override time
    // greater than the default (accounting for precision).
    StampWrapAndAdd(versioned_resources, override_map_entry.first, version++,
                    current_resource.get());
  }
}

template <typename BundleLike, typename PatientLike,
          typename ContainedResourceLike,
          typename DateTimeLike = FHIR_DATATYPE(BundleLike, date_time)>
void SplitPatient(PatientLike patient, const BundleLike& bundle,
                  const proto::VersionConfig& config,
                  std::vector<ContainedResourceLike>* versioned_resources,
                  std::map<std::string, int>* counter_stats) {
  if (patient.deceased().has_date_time()) {
    // We know when the patient died.  Add two versions of the patient:
    // One at time-of-death, with the death information, and
    // one at the initial time with everything except time-of-death.
    StampWrapAndAdd(versioned_resources, patient.deceased().date_time(), 1,
                    &patient);
  } else if (patient.deceased().boolean().value()) {
    // All we know is that the patient died.
    StatusOr<const DateTimeLike*> last_time = GetEndOfLastEncounter(bundle);
    if (last_time.ok()) {
      // Use end of last encounter as time of death, with a delay to put it
      // a safe distance in the future.
      DateTimeLike input_time(*last_time.ValueOrDie());
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
    StatusOr<const DateTimeLike*> first_time = GetStartOfFirstEncounter(bundle);
    if (first_time.ok()) {
      // Use start of first encounter as time for V0 of the patient.
      // Subtract a delay to guarantee it is the first entry into the system.
      DateTimeLike input_time(*first_time.ValueOrDie());
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

// Helper function for splitting ContainedResources that contain resources that
// are common in FHIR >=STU3 These can be handled statically without needing
// reflection. A return value of true indicates that the contained resource
// contained a common resource, and it was handled successfully.
template <typename BundleLike, typename ContainedResourceLike =
                                   BUNDLE_CONTAINED_RESOURCE(BundleLike)>
void SplitContainedResource(
    const ContainedResourceLike contained_resource, const BundleLike& bundle,
    const proto::VersionConfig& config,
    std::vector<ContainedResourceLike>* versioned_resources,
    std::map<std::string, int>* counter_stats) {
  if (contained_resource.has_patient()) {
    // Patient is slightly different.
    // The first version timestamp comes from the start time of the first
    // encounter in the system.
    // If the patient died, but there is no death timestamp, we need to reach
    // into the last encounter for the time to use.
    // TODO: if this turns out to be a more common pattern,
    // we could have a way to encode this logic into the config proto as a
    // new kind of override.
    internal::SplitPatient(contained_resource.patient(), bundle, config,
                           versioned_resources, counter_stats);
    return;
  }

  const ::google::protobuf::Message* resource =
      GetContainedResource(contained_resource).ValueOrDie();

  internal::SplitResource<BundleLike>(*resource, config, versioned_resources,
                                      counter_stats);
}

}  // namespace internal

const proto::VersionConfig VersionConfigFromFlags();

template <typename BundleLike, typename ContainedResourceLike =
                                   BUNDLE_CONTAINED_RESOURCE(BundleLike)>
std::vector<ContainedResourceLike> BundleToVersionedResources(
    const BundleLike& bundle, const proto::VersionConfig& config,
    std::map<std::string, int>* counter_stats) {
  static auto* oneof_resource_descriptor =
      ContainedResourceLike::descriptor()->FindOneofByName("oneof_resource");
  std::vector<ContainedResourceLike> versioned_resources;
  for (const auto& entry : bundle.entry()) {
    if (!entry.has_resource()) continue;
    (*counter_stats)["num-unversioned-resources-in"]++;
    int initial_size = versioned_resources.size();
    const auto& resource = entry.resource();
    internal::SplitContainedResource(resource, bundle, config,
                                     &versioned_resources, counter_stats);
    const int new_size = versioned_resources.size();
    const std::string resource_name =
        resource.GetReflection()
            ->GetOneofFieldDescriptor(resource, oneof_resource_descriptor)
            ->name();
    (*counter_stats)[absl::StrCat("num-", resource_name, "-split-to-",
                                  (new_size - initial_size))]++;
  }
  return versioned_resources;
}

template <typename BundleLike>
BundleLike BundleToVersionedBundle(const BundleLike& bundle,
                                   const proto::VersionConfig& config,
                                   std::map<std::string, int>* counter_stats) {
  const auto& versioned_resources =
      BundleToVersionedResources(bundle, config, counter_stats);
  BundleLike output_bundle;
  for (const auto& resource : versioned_resources) {
    *(output_bundle.add_entry()->mutable_resource()) = resource;
  }
  return output_bundle;
}

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_BUNDLE_TO_VERSIONED_RESOURCES_CONVERTER_H_
