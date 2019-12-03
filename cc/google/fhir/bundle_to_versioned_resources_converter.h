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

#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "proto/version_config.pb.h"

namespace google {
namespace fhir {

namespace internal {

extern const absl::Duration kGapAfterLastEncounterForUnknownDeath;
extern const absl::Duration kGapBeforeFirstEncounterForUnknownBirth;

// Finds the Encounter in the bundle with the last end time.
// Returns ::tensorflow::errors:NotFoundError if there are no encounters
// in the bundle.
template <typename BundleLike,
          typename EncounterLike = BUNDLE_TYPE(BundleLike, encounter)>
StatusOr<const stu3::proto::DateTime*> GetEndOfLastEncounter(
    const BundleLike& bundle) {
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
    return ::tensorflow::errors::NotFound("No encounter found");
  }
  return &(last_encounter->period().end());
}

// Finds the Encounter in the bundle with the earliest start time.
// Returns ::tensorflow::errors:NotFoundError if there are no encounters
// in the bundle.
template <typename BundleLike,
          typename EncounterLike = BUNDLE_TYPE(BundleLike, encounter)>
StatusOr<const stu3::proto::DateTime*> GetStartOfFirstEncounter(
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
    return ::tensorflow::errors::NotFound("No encounter found");
  }
  return &(first_encounter->period().start());
}

const std::vector<
    std::pair<stu3::proto::DateTime, std::unordered_set<std::string>>>
GetSortedOverrides(const proto::ResourceConfig& resource_config,
                   const ::google::protobuf::Message& resource);

// Stamps a resource with a version + time metadata,
// Wraps it into a ContainedResource, and
// Adds it along with a time to a versioned resources list
// Works for any Time-like fhir element that has a value_us, timezone,
// and precision, e.g.: Date, DateTime, Instant.
template <typename ContainedResourceLike, typename T, typename R>
void StampWrapAndAdd(std::vector<ContainedResourceLike>* versioned_resources,
                     const T& timelike, const int version_id, R* resource) {
  stu3::proto::Meta* meta = MutableMetadataFromResource(resource);
  meta->mutable_version_id()->set_value(absl::StrCat(version_id));
  stu3::proto::Instant* last_updated = meta->mutable_last_updated();
  last_updated->set_timezone(timelike.timezone());

  // NOTE: We can't directly set last_updated.precision from timelike.precision,
  // because enums are not shared between Time-like FHIR elements.
  // But they share some string names, so we can try to translate between them.
  // The most coarse precision available to Instant is SECOND, though, so
  // anything more coarse (e.g., Date measured in DAY) will require us to
  // "round up" value to the nearest unit of precision avoid any leaks.
  stu3::proto::Instant::Precision precision;
  const bool parse_successful = stu3::proto::Instant::Precision_Parse(
      T::Precision_Name(timelike.precision()), &precision);
  if (parse_successful) {
    last_updated->set_value_us(timelike.value_us());
    last_updated->set_precision(precision);
  } else {
    last_updated->set_precision(stu3::proto::Instant::SECOND);
    absl::TimeZone tz;
    TF_CHECK_OK(GetTimezone(timelike.timezone(), &tz))
        << "No Timezone on timelike: " << timelike.DebugString();
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
StatusOr<stu3::proto::DateTime> GetDefaultDateTime(
    const proto::ResourceConfig& config, const ::google::protobuf::Message& message);

template <typename R, typename ContainedResourceLike>
void SplitResource(const R& resource, const proto::VersionConfig& config,
                   std::vector<ContainedResourceLike>* versioned_resources,
                   std::map<std::string, int>* counter_stats) {
  // Make a mutable copy of the resource, that we will modify and copy into
  // the result vector along the way.
  std::unique_ptr<R> current_resource(resource.New());
  current_resource->CopyFrom(resource);

  const ::google::protobuf::Descriptor* descriptor = resource.GetDescriptor();
  const std::string& resource_name = descriptor->name();

  const auto& iter = config.resource_config().find(resource_name);
  if (iter == config.resource_config().end()) {
    (*counter_stats)[absl::StrCat("split-failed-no-config-", resource_name)]++;
    return;
  }
  const auto& resource_config = iter->second;

  StatusOr<stu3::proto::DateTime> default_time_status =
      GetDefaultDateTime(resource_config, resource);
  if (!default_time_status.ok()) {
    (*counter_stats)[default_time_status.status().error_message()]++;
    return;
  }
  stu3::proto::DateTime default_time = default_time_status.ValueOrDie();

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
  const std::vector<
      std::pair<stu3::proto::DateTime, std::unordered_set<std::string>>>&
      sorted_overrides = GetSortedOverrides(resource_config, resource);

  // Strip out all fields with overrides, to get the "base" version.
  // This is the version that goes in at the "default" time.
  // TODO: This operates under the assumption that the default time
  // is the _earliest_ available time.  To safeguard against leaks, it should be
  // the _latest_ available time.  In that case, the "default" time version will
  // just be the original proto.
  for (const auto& timestamp_override : resource_config.timestamp_override()) {
    for (const std::string& field_path : timestamp_override.resource_field()) {
      if (field_path.find_last_of('[') == std::string::npos) {
        CHECK(ClearFieldByPath(current_resource.get(), field_path).ok());
      } else {
        CHECK(ClearFieldByPath(current_resource.get(), StripIndex(field_path))
                  .ok());
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
          ::tensorflow::error::Code::NOT_FOUND) {
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
    stu3::proto::DateTime override_time = override_map_entry.first;
    // TODO: Once we've switched the version config to use latest
    // timestamp as default, enforce that here by failing on any override time
    // greater than the default (accounting for precision).
    StampWrapAndAdd(versioned_resources, override_map_entry.first, version++,
                    current_resource.get());
  }
}

template <typename BundleLike, typename PatientLike,
          typename ContainedResourceLike>
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
    StatusOr<const stu3::proto::DateTime*> last_time =
        GetEndOfLastEncounter(bundle);
    if (last_time.ok()) {
      // Use end of last encounter as time of death, with a delay to put it
      // a safe distance in the future.
      stu3::proto::DateTime input_time(*last_time.ValueOrDie());
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
    StatusOr<const stu3::proto::DateTime*> first_time =
        GetStartOfFirstEncounter(bundle);
    if (first_time.ok()) {
      // Use start of first encounter as time for V0 of the patient.
      // Subtract a delay to guarantee it is the first entry into the system.
      stu3::proto::DateTime input_time(*first_time.ValueOrDie());
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
    if (resource.has_patient()) {
      // Patient is slightly different.
      // The first version timestamp comes from the start time of the first
      // encounter in the system.
      // If the patient died, but there is no death timestamp, we need to reach
      // into the last encounter for the time to use.
      // TODO: if this turns out to be a more common pattern,
      // we could have a way to encode this logic into the config proto as a
      // new kind of override.
      internal::SplitPatient(resource.patient(), bundle, config,
                             &versioned_resources, counter_stats);
    } else if (resource.has_claim()) {
      internal::SplitResource(resource.claim(), config, &versioned_resources,
                              counter_stats);
    } else if (resource.has_composition()) {
      internal::SplitResource(resource.composition(), config,
                              &versioned_resources, counter_stats);
    } else if (resource.has_condition()) {
      internal::SplitResource(resource.condition(), config,
                              &versioned_resources, counter_stats);
    } else if (resource.has_encounter()) {
      internal::SplitResource(resource.encounter(), config,
                              &versioned_resources, counter_stats);
    } else if (resource.has_medication_administration()) {
      internal::SplitResource(resource.medication_administration(), config,
                              &versioned_resources, counter_stats);
    } else if (resource.has_medication_request()) {
      internal::SplitResource(resource.medication_request(), config,
                              &versioned_resources, counter_stats);
    } else if (resource.has_observation()) {
      internal::SplitResource(resource.observation(), config,
                              &versioned_resources, counter_stats);
    } else if (resource.has_procedure()) {
      internal::SplitResource(resource.procedure(), config,
                              &versioned_resources, counter_stats);
    } else if (resource.has_procedure_request()) {
      internal::SplitResource(resource.procedure_request(), config,
                              &versioned_resources, counter_stats);
    }
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
