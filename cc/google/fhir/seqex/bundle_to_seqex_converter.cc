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

#include "google/fhir/seqex/bundle_to_seqex_converter.h"

#include <algorithm>
#include <map>
#include <string>
#include <type_traits>
#include <utility>

#include "google/protobuf/descriptor.h"
#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/time/time.h"
#include "google/fhir/bundle_to_versioned_resources_converter.h"
#include "google/fhir/seqex/example_key.h"
#include "google/fhir/seqex/feature_keys.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/version_config.pb.h"
#include "tensorflow/core/platform/env.h"

ABSL_FLAG(int64_t, max_sequence_length, 1000000,
          "Maximum length of emitted sequences");
ABSL_FLAG(std::string, trigger_time_redacted_features, "",
          "Sometimes, labels are directly or indirectly derived from "
          "features that occur at the same time as the trigger. In those "
          "cases, the original features need to be redacted to not make "
          "the prediction task trivial. The list of features specified "
          "here will be retained at all time steps prior to the trigger "
          "time, but removed from the sequence example if they occur at "
          "the trigger time exactly. Names are prefix-matched to the comma-"
          "separated string values in this flag.");

namespace google {
namespace fhir {
namespace seqex {

using ::google::fhir::proto::VersionConfig;
using ::google::protobuf::Message;
using ::tensorflow::Example;
using ::tensorflow::Feature;
using ::tensorflow::Features;

namespace internal {

// From http://hl7.org/fhir/v3/ActCode
const char kClassInpatient[] = "IMP";

bool FeaturePrefixMatch(const std::string& feature,
                        const std::set<std::string>& prefix_set) {
  std::string name = feature;
  while (prefix_set.count(name) == 0 && name.rfind('.') != std::string::npos) {
    name = name.substr(0, name.rfind('.'));
  }
  return prefix_set.count(name) != 0;
}

void GetContextFeatures(const std::pair<ExampleKey, Features>& labels,
                        const Example& context, const int sequence_length,
                        const std::vector<absl::Time>& encounter_start_times,
                        google::protobuf::Map<std::string, Feature>* feature_map,
                        const bool generate_sequence_label) {
  // Set patient context features.
  *feature_map = context.features().feature();

  // Add absolute time to the context
  (*feature_map)[kLabelTimestampFeatureKey].mutable_int64_list()->add_value(
      absl::ToUnixSeconds(labels.first.trigger_timestamp));

  // Get the id of the encounter which overlaps with the label time
  auto encounter_iter = std::upper_bound(encounter_start_times.begin(),
                                         encounter_start_times.end(),
                                         labels.first.trigger_timestamp);
  int encounter_index = encounter_iter - encounter_start_times.begin();
  absl::Time current_encounter_time =
      encounter_index == 0 ? labels.first.trigger_timestamp
                           : encounter_start_times[encounter_index - 1];
  (*feature_map)[kLabelEncounterIdFeatureKey].mutable_int64_list()->add_value(
      absl::ToUnixSeconds(current_encounter_time));
  (*feature_map)[kSequenceLengthFeatureKey].mutable_int64_list()->add_value(
      sequence_length);

  // Add labels to context when not generating sequence labels.
  if (!generate_sequence_label) {
    for (const auto& label : labels.second.feature()) {
      (*feature_map)[label.first] = label.second;
    }
  }
}

void GetSequenceFeatures(
    absl::Time trigger_timestamp,
    const std::vector<std::pair<absl::Time, Example>>::const_iterator& begin,
    const std::vector<std::pair<absl::Time, Example>>::const_iterator& end,
    const Features& feature_types,
    const std::set<std::string>& redacted_features_for_example,
    google::protobuf::Map<std::string, ::tensorflow::FeatureList>* feature_list_map) {
  // Touch all feature lists, so we can keep pointers to map entries from here
  // on.
  for (const auto& empty_feature : feature_types.feature()) {
    (*feature_list_map)[empty_feature.first];
  }
  auto* event_id_feature = &(*feature_list_map)[kEventIdFeatureKey];

  // We'll attempt to incrementally add only new features to the accumulator.
  // The sequence feature time steps are partitioned into three parts:
  // - those that happened long enough ago that they can remain unchanged
  // - those that were recent enough (eventId == current) that they may need
  //   updates, because individual fields may have been redacted
  // - those that are completely new.
  // We deal with these three cases in turn.
  auto iter = begin;
  int sequence_length = 0;

  // Case 1: no updates needed.
  const int64_t now = absl::ToUnixSeconds(trigger_timestamp);
  while (sequence_length < event_id_feature->feature_size() &&
         event_id_feature->feature(sequence_length).int64_list().value(0) ==
             now) {
    iter++;
    sequence_length++;
  }

  // Case 2: overwrite all features (but allocate no new sequence steps)
  while (sequence_length < event_id_feature->feature_size()) {
    const int64_t current_time = absl::ToUnixSeconds(iter->first);
    const auto& current_step = iter->second.features().feature();
    bool has_valid_feature = current_time != now;

    if (current_time == now) {
      for (const auto& feature : current_step) {
        if (redacted_features_for_example.count(feature.first) == 0) {
          has_valid_feature = true;
        }
      }
    }

    // Append to the output
    if (has_valid_feature) {
      for (const auto& empty_feature : feature_types.feature()) {
        const std::string& feature_name = empty_feature.first;
        auto* f =
            (*feature_list_map)[feature_name].mutable_feature(sequence_length);
        if (current_step.count(feature_name) != 0 &&
            (current_time != now ||
             redacted_features_for_example.count(feature_name) == 0)) {
          *f = current_step.at(feature_name);
        } else {
          *f = empty_feature.second;
        }
      }
    }
    iter++;
    sequence_length++;
  }

  // Case 3: add new features.
  while (iter != end) {
    const int64_t delta_time =
        absl::ToInt64Seconds(trigger_timestamp - iter->first);
    const auto& current_step = iter->second.features().feature();

    bool has_valid_feature = delta_time != 0;
    if (delta_time == 0) {
      for (const auto& feature : current_step) {
        if (redacted_features_for_example.count(feature.first) == 0) {
          has_valid_feature = true;
        }
      }
    }
    // Append to the output
    if (has_valid_feature) {
      for (const auto& empty_feature : feature_types.feature()) {
        const std::string& feature_name = empty_feature.first;
        auto* f = (*feature_list_map)[feature_name].add_feature();
        if (current_step.count(feature_name) != 0 &&
            (delta_time != 0 ||
             redacted_features_for_example.count(feature_name) == 0)) {
          *f = current_step.at(feature_name);
        } else {
          *f = empty_feature.second;
        }
      }
    }
    iter++;
    sequence_length++;
  }
}

// Add resource-independent, label-independent dense features for bagging.
void AddBaggingFeatures(absl::Time event_time,
                        const std::vector<absl::Time>& encounter_start_times,
                        Example* example) {
  // The event id is the timestamp of the event.
  int64_t event_id = absl::ToUnixSeconds(event_time);
  (*example->mutable_features()->mutable_feature())[kEventIdFeatureKey]
      .mutable_int64_list()
      ->add_value(event_id);
  // The encounter id is the start time of the encounter containing this event,
  // or min(event_id) for any event that occurs before the first encounter.
  auto iter = std::upper_bound(encounter_start_times.begin(),
                               encounter_start_times.end(), event_time);
  int index = iter - encounter_start_times.begin();
  int64_t encounter_id = absl::ToUnixSeconds(encounter_start_times[index - 1]);
  (*example->mutable_features()->mutable_feature())[kEncounterIdFeatureKey]
      .mutable_int64_list()
      ->add_value(encounter_id);
}

BaseBundleToSeqexConverter::BaseBundleToSeqexConverter(
    const PrimitiveHandler* primitive_handler,
    const proto::VersionConfig& fhir_version_config,
    std::shared_ptr<const TextTokenizer> tokenizer,
    const bool enable_attribution, const bool generate_sequence_label)
    : primitive_handler_(primitive_handler),
      version_config_(fhir_version_config),
      tokenizer_(tokenizer),
      enable_attribution_(enable_attribution),
      generate_sequence_label_(generate_sequence_label) {
  // Split the redacted feature list for easy access.
  redacted_features_ =
      absl::StrSplit(absl::GetFlag(FLAGS_trigger_time_redacted_features), ',');
  // Make sure Done() would return true.
  current_label_ = label_map_.end();
}

bool BaseBundleToSeqexConverter::Next() {
  // We emit multiple examples per bundle, one per label. Examples with
  // timestamps before or at the label timestamp may be included in the sample
  // sequence; we keep the most recent --max_sequence_length ones.
  // TODO: be more principled in which events are ok to use
  // and which are not.
  if (!init_done_) {
    if (generate_sequence_label_) {
      // Only generate one seqex for sequence labels. Use the last trigger time
      // as seqex timestamp.
      current_label_ = std::prev(label_map_.end());
    } else {
      // First call to Next(), from Begin()
      current_label_ = label_map_.begin();
    }
    init_done_ = true;
  } else {
    CHECK(!Done());
    current_label_++;
  }
  if (Done()) {
    return true;
  }

  key_ = current_label_->first;

  auto end = examples_.end();
  int offset = 0;
  int sequence_length = end - examples_.begin();
  if (!generate_sequence_label_) {
    // Find the first element that happened after the trigger time.
    absl::Time trigger_time = key_.trigger_timestamp;
    for (auto it = examples_.begin(); it != examples_.end(); ++it) {
      if (it->first > trigger_time) {
        end = it;
        break;
      }
    }

    sequence_length = end - examples_.begin();
    const int64_t max_sequence_length =
        absl::GetFlag(FLAGS_max_sequence_length);
    if (sequence_length > max_sequence_length) {
      offset = sequence_length - max_sequence_length;
      sequence_length = max_sequence_length;
    }
  }

  // Convert to feature-major. If the requested sequence start is not what
  // we expected, we have to start from scratch.
  if (offset != cached_offset_) {
    // When this happens, example generation will be expensive.
    seqex_.Clear();
    cached_offset_ = offset;
  }

  GetContextFeatures(
      *current_label_, context_, sequence_length, encounter_start_times_,
      seqex_.mutable_context()->mutable_feature(), generate_sequence_label_);
  GetSequenceFeatures(current_label_->first.trigger_timestamp,
                      examples_.begin() + offset, end, feature_types_,
                      redacted_features_for_example_,
                      seqex_.mutable_feature_lists()->mutable_feature_list());
  QCHECK(!seqex_.feature_lists().feature_list().empty())
      << "Empty SequenceExample, Patient ID: "
      << seqex_.context()
             .feature()
             .at(kPatientIdFeatureKey)
             .bytes_list()
             .value(0);

  key_.start = offset;
  key_.end = end - examples_.begin();
  return true;
}

bool BaseBundleToSeqexConverter::Done() {
  return current_label_ == label_map_.end();
}

void BaseBundleToSeqexConverter::Reset() {
  examples_.clear();
  encounter_start_times_.clear();
  context_.Clear();
  label_map_.clear();
  redacted_features_for_example_.clear();
  feature_types_.Clear();
  seqex_.Clear();
  cached_offset_ = 0;
  init_done_ = false;
  patient_id_ = "";
  label_map_.clear();
  counter_stats_ = nullptr;
}

void BaseBundleToSeqexConverter::EventSequenceToExamples(
    const std::map<absl::Time, absl::Time>& encounter_boundaries,
    const std::vector<std::pair<std::pair<absl::Time, std::string>,
                                tensorflow::Example>>& event_sequence) {
  // We use the encounter start times to split the patient timeline.
  if (!event_sequence.empty()) {
    // Keep track of the earliest event time seen in the data.
    encounter_start_times_.push_back(event_sequence.begin()->first.first);
  }
  // Note that we can guarantee the earliest event time is not after
  // encounter_boundaries[0].first, because encounter is one of the resources
  // that used to generate the event sequences.
  // It's fine to have duplicate entries in encounter_start_times_.
  for (const auto& boundary : encounter_boundaries) {
    encounter_start_times_.push_back(boundary.first);
  }

  // Emit features.
  for (const auto& event : event_sequence) {
    tensorflow::Example example = event.second;
    internal::AddBaggingFeatures(event.first.first, encounter_start_times_,
                                 &example);

    (*counter_stats_)["num-examples"]++;
    examples_.push_back(std::make_pair(event.first.first, example));
  }

  // Get a list of all feature types, and the set of redacted features.
  for (const auto& event : examples_) {
    for (const auto& feature : event.second.features().feature()) {
      ::tensorflow::Feature* f =
          &(*feature_types_.mutable_feature())[feature.first];
      if (feature.second.has_bytes_list()) {
        f->mutable_bytes_list()->mutable_value();
      } else if (feature.second.has_int64_list()) {
        f->mutable_int64_list()->mutable_value();
      } else if (feature.second.has_float_list()) {
        f->mutable_float_list()->mutable_value();
      } else {
        LOG(FATAL) << "Invalid feature " << feature.second.DebugString();
      }

      if (internal::FeaturePrefixMatch(feature.first, redacted_features_)) {
        redacted_features_for_example_.insert(feature.first);
      }
    }
  }
}

StatusOr<absl::Time> GetVersionTime(const Message& resource) {
  FHIR_ASSIGN_OR_RETURN(const Message* meta,
                        GetMessageInField(resource, "meta"));
  FHIR_ASSIGN_OR_RETURN(const Message* last_updated,
                        GetMessageInField(*meta, "last_updated"));
  return google::fhir::GetTimeFromTimelikeElement(*last_updated);
}

}  // namespace internal

}  // namespace seqex
}  // namespace fhir
}  // namespace google
