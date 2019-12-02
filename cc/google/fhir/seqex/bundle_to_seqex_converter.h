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

#ifndef GOOGLE_FHIR_SEQEX_BUNDLE_TO_SEQEX_CONVERTER_H_
#define GOOGLE_FHIR_SEQEX_BUNDLE_TO_SEQEX_CONVERTER_H_

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/message.h"
#include "absl/flags/declare.h"
#include "absl/strings/str_split.h"
#include "absl/time/time.h"
#include "google/fhir/bundle_to_versioned_resources_converter.h"
#include "google/fhir/seqex/example_key.h"
#include "google/fhir/seqex/feature_keys.h"
#include "google/fhir/seqex/resource_to_example.h"
#include "google/fhir/status/status.h"
#include "google/fhir/util.h"
#include "proto/stu3/codes.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "proto/stu3/resources.pb.h"
#include "proto/version_config.pb.h"
#include "tensorflow/core/example/example.pb.h"
#include "tensorflow/core/example/feature.pb.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/logging.h"

ABSL_DECLARE_FLAG(int64_t, max_sequence_length);
ABSL_DECLARE_FLAG(std::string, trigger_time_redacted_features);

namespace google {
namespace fhir {
namespace seqex {


typedef std::pair<stu3::google::EventTrigger,
                  std::vector<stu3::google::EventLabel>>
    TriggerLabelsPair;

namespace internal {

extern const char kClassInpatient[];

Status BuildLabelsFromTriggerLabelPair(
    const std::string& patient_id, const std::vector<TriggerLabelsPair>& labels,
    std::map<ExampleKey, tensorflow::Features>* label_map);

// Add resource-independent, label-independent dense features for bagging.
void AddBaggingFeatures(absl::Time event_time,
                        const std::vector<absl::Time>& encounter_start_times,
                        tensorflow::Example* example);

bool FeaturePrefixMatch(const std::string& feature,
                        const std::set<std::string>& prefix_set);

void GetSequenceFeatures(
    absl::Time trigger_timestamp,
    const std::vector<
        std::pair<absl::Time, tensorflow::Example>>::const_iterator& begin,
    const std::vector<
        std::pair<absl::Time, tensorflow::Example>>::const_iterator& end,
    const tensorflow::Features& feature_types,
    const std::set<std::string>& redacted_features_for_example,
    google::protobuf::Map<std::string, ::tensorflow::FeatureList>* feature_list_map);

void GetContextFeatures(
    const std::pair<ExampleKey, tensorflow::Features>& labels,
    const tensorflow::Example& context, const int sequence_length,
    const std::vector<absl::Time>& encounter_start_times,
    google::protobuf::Map<std::string, tensorflow::Feature>* feature_map,
    const bool generate_sequence_label);

class BaseBundleToSeqexConverter {
 public:
  BaseBundleToSeqexConverter(const proto::VersionConfig& fhir_version_config,
                             const bool enable_attribution,
                             const bool generate_sequence_label);
  // Move the iterator to the next key/example pair from the bundle.
  bool Next();

  // True if we have finished conversion for this bundle.
  bool Done();

 protected:
  void Reset();

  void EventSequenceToExamples(
      const std::map<absl::Time, absl::Time>& encounter_boundaries,
      const std::vector<std::pair<std::pair<absl::Time, std::string>,
                                  tensorflow::Example>>& event_sequence);

  proto::VersionConfig version_config_;
  std::set<std::string> redacted_features_;

  // These are computed once per Bundle.
  std::string patient_id_;
  std::vector<std::pair<absl::Time, ::tensorflow::Example>> examples_;
  std::vector<absl::Time> encounter_start_times_;
  ::tensorflow::Example context_;
  // ExampleKey -> labels.
  std::map<struct seqex::ExampleKey, ::tensorflow::Features> label_map_;
  std::map<std::string, int>* counter_stats_ = nullptr;

  bool init_done_;
  std::map<struct seqex::ExampleKey, ::tensorflow::Features>::iterator
      current_label_;
  std::set<std::string> redacted_features_for_example_;
  ::tensorflow::Features feature_types_;

  // The current sequence example.
  struct ExampleKey key_;
  // Internal seqex that contains all data.
  ::tensorflow::SequenceExample seqex_;

  int cached_offset_;

  bool enable_attribution_;

  bool generate_sequence_label_;
};

}  // namespace internal

// This class is not thread-safe.
template <typename BundleLike = stu3::proto::Bundle>
class BundleToSeqexConverter : public internal::BaseBundleToSeqexConverter {
 public:
  using BaseBundleToSeqexConverter::BaseBundleToSeqexConverter;

  // This API should be called once per bundle. When it returns, either the
  // iterator is Done(), or a valid key/example pair can be accessed using the
  // key() and example() accessors. To move to the next example, call Next();
  bool Begin(const std::string& patient_id, const BundleLike& bundle,
             const std::vector<TriggerLabelsPair>& labels,
             std::map<std::string, int>* counter_stats);

  // Return the example key.
  // Requires: !Done().
  std::string ExampleKey() {
    CHECK(!Done());
    return key_.ToString();
  }

  // Return the example key with a sixteen digit hex prefix based on a hash,
  // which makes it easier to shuffle the outputs.
  // Requires: !Done().
  std::string ExampleKeyWithPrefix() {
    CHECK(!Done());
    return key_.ToStringWithPrefix();
  }

  // Get the current SequenceExample. Requires: !Done().
  const ::tensorflow::SequenceExample& GetExample() {
    CHECK(!Done());
    return seqex_;
  }

  int ExampleSeqLen() {
    CHECK(!Done());
    return key_.end - key_.start;
  }

 private:
  bool Begin(const std::string& patient_id, const BundleLike& bundle,
             const std::map<struct seqex::ExampleKey, ::tensorflow::Features>&
                 label_map,
             std::map<std::string, int>* counter_stats);

  // Get a list of non-overlapping encounter boundaries. For now, we use only
  // inpatient encounters, and merge any encounters that overlap.
  void GetEncounterBoundaries(
      const BundleLike& bundle,
      std::map<absl::Time, absl::Time>* encounter_boundaries);

  // Convert a fhir bundle to a sequence of tf examples.
  // The result is stored in class member variables.
  void BundleToExamples(const BundleLike& bundle);

  // Extract context features from a fhir bundle. The result is stored in
  // class member variables.
  void BundleToContext(const BundleLike& bundle);

  // Converts a resource to one or more examples.
  template <typename R>
  void ConvertResourceToExamples(
      R resource,
      std::vector<std::pair<std::pair<absl::Time, std::string>,
                            ::tensorflow::Example>>* event_sequence) {
    // Conversion from versioned resource to example is 1-1.
    const absl::Time version_time = google::fhir::GetTimeFromTimelikeElement(
        resource.meta().last_updated());
    ::tensorflow::Example example;
    seqex::ResourceToExample(resource, &example, enable_attribution_);
    if (enable_attribution_) {
      (*example.mutable_features()
            ->mutable_feature())[seqex::kResourceIdFeatureKey]
          .mutable_bytes_list()
          ->add_value(GetReferenceToResource(resource));
    }
    event_sequence->push_back(std::make_pair(
        std::make_pair(version_time, GetReferenceToResource(resource)),
        example));
  }
};

typedef BundleToSeqexConverter<stu3::proto::Bundle>
    UnprofiledBundleToSeqexConverter;

// Class implementation below.

template <typename BundleLike>
bool BundleToSeqexConverter<BundleLike>::Begin(
    const std::string& patient_id, const BundleLike& bundle,
    const std::map<struct seqex::ExampleKey, ::tensorflow::Features>& label_map,
    std::map<std::string, int>* counter_stats) {
  Reset();
  patient_id_ = patient_id;
  label_map_ = label_map;
  counter_stats_ = counter_stats;
  if (label_map_.empty()) {
    current_label_ = label_map_.end();  // mark done
    init_done_ = true;
    return false;
  }
  BundleLike versioned_bundle =
      BundleToVersionedBundle(bundle, version_config_, counter_stats_);
  BundleToExamples(versioned_bundle);
  BundleToContext(versioned_bundle);
  return Next();
}

template <typename BundleLike>
bool BundleToSeqexConverter<BundleLike>::Begin(
    const std::string& patient_id, const BundleLike& bundle,
    const std::vector<TriggerLabelsPair>& labels,
    std::map<std::string, int>* counter_stats) {
  std::map<struct ExampleKey, ::tensorflow::Features> label_map;
  // TODO: Fail gracefully.
  CHECK(
      internal::BuildLabelsFromTriggerLabelPair(patient_id, labels, &label_map)
          .ok());
  return Begin(patient_id, bundle, label_map, counter_stats);
}

// Get a list of non-overlapping encounter boundaries. For now, we use only
// inpatient encounters, and merge any encounters that overlap.

template <typename BundleLike>
void BundleToSeqexConverter<BundleLike>::GetEncounterBoundaries(
    const BundleLike& bundle,
    std::map<absl::Time, absl::Time>* encounter_boundaries) {
  // List inpatient encounter start and end times.
  std::map<absl::Time, absl::Time> inpatient_encounters;
  for (const auto& entry : bundle.entry()) {
    if (entry.resource().has_encounter()) {
      const auto& encounter = entry.resource().encounter();
      if (encounter.class_value().code().value() != internal::kClassInpatient) {
        (*counter_stats_)["num-encounter-id-not-inpatient"]++;
      } else if (!encounter.period().has_start() ||
                 !encounter.period().has_end()) {
        (*counter_stats_)["num-encounter-id-missing-times"]++;
      } else {
        // Keep.
        (*counter_stats_)["num-encounter-id-valid"]++;
        absl::Time start =
            absl::FromUnixMicros(encounter.period().start().value_us());
        absl::Time end =
            absl::FromUnixMicros(encounter.period().end().value_us());
        inpatient_encounters[start] = end;
      }
    }
  }
  // Merge overlapping encounters.
  if (!inpatient_encounters.empty()) {
    absl::Time current_start, current_end;
    // Note that the standard guarantees the iteration order of std:map.
    for (const auto& e : inpatient_encounters) {
      if (!encounter_boundaries->empty() && current_end > e.first) {
        (*counter_stats_)["num-encounter-id-merged"]++;
        current_end = std::max(current_end, e.second);
      } else {
        (*counter_stats_)["num-encounter-id-kept"]++;
        current_start = e.first;
        current_end = e.second;
      }
      (*encounter_boundaries)[current_start] = current_end;
    }
  }
}

template <typename BundleLike>
void BundleToSeqexConverter<BundleLike>::BundleToExamples(
    const BundleLike& bundle) {
  // Make a sequence sorted by timestamp.
  std::vector<
      std::pair<std::pair<absl::Time, std::string>, tensorflow::Example>>
      event_sequence;
  for (const auto& entry : bundle.entry()) {
    if (entry.resource().has_claim()) {
      ConvertResourceToExamples(entry.resource().claim(), &event_sequence);
    }
    if (entry.resource().has_composition()) {
      ConvertResourceToExamples(entry.resource().composition(),
                                &event_sequence);
    }
    if (entry.resource().has_condition()) {
      ConvertResourceToExamples(entry.resource().condition(), &event_sequence);
    }
    if (entry.resource().has_encounter()) {
      ConvertResourceToExamples(entry.resource().encounter(), &event_sequence);
    }
    if (entry.resource().has_medication_administration()) {
      ConvertResourceToExamples(entry.resource().medication_administration(),
                                &event_sequence);
    }
    if (entry.resource().has_medication_request()) {
      ConvertResourceToExamples(entry.resource().medication_request(),
                                &event_sequence);
    }
    if (entry.resource().has_observation()) {
      ConvertResourceToExamples(entry.resource().observation(),
                                &event_sequence);
    }
    if (entry.resource().has_procedure()) {
      ConvertResourceToExamples(entry.resource().procedure(), &event_sequence);
    }
    if (entry.resource().has_procedure_request()) {
      ConvertResourceToExamples(entry.resource().procedure_request(),
                                &event_sequence);
    }
  }

  if (generate_sequence_label_) {
    // TODO: Either delete or fix seconds_until_label.
    for (auto entry : label_map_) {
      tensorflow::Example example;
      *example.mutable_features() = entry.second;
      event_sequence.push_back(std::make_pair(
          std::make_pair(entry.first.trigger_timestamp, "label"), example));
    }
  }

  // Sort in time order, and then in case of a tie, sort by resource-id.
  // Note: we need the sorting to be deterministic so that we can do a
  // meaningful diff in data across different runs of the binary.
  std::sort(event_sequence.begin(), event_sequence.end(),
            [](const std::pair<std::pair<absl::Time, std::string>,
                               tensorflow::Example>& a,
               const std::pair<std::pair<absl::Time, std::string>,
                               tensorflow::Example>& b) {
              return a.first < b.first;
            });

  // Get a list of encounter boundary times.
  std::map<absl::Time, absl::Time> encounter_boundaries;
  GetEncounterBoundaries(bundle, &encounter_boundaries);

  EventSequenceToExamples(encounter_boundaries, event_sequence);
}

template <typename BundleLike>
void BundleToSeqexConverter<BundleLike>::BundleToContext(
    const BundleLike& bundle) {
  // Add patient features to the context.
  for (const auto& entry : bundle.entry()) {
    if (entry.resource().has_patient()) {
      auto patient = entry.resource().patient();
      if (google::fhir::GetMetadataFromResource(patient).version_id().value() !=
          "0") {
        // We're only interested in the V0 patient for the context.
        continue;
      }
      patient.clear_meta();
      patient.clear_deceased();
      ResourceToExample(patient, &context_, enable_attribution_);
      CHECK(patient.has_id());
      // Add patientId to context feature for cross validation.
      (*context_.mutable_features()->mutable_feature())[kPatientIdFeatureKey]
          .mutable_bytes_list()
          ->add_value(patient.id().value());
    }
  }
}

}  // namespace seqex
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_SEQEX_BUNDLE_TO_SEQEX_CONVERTER_H_
