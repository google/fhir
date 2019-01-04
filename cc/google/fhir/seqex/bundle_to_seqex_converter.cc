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

#include "gflags/gflags.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/time/time.h"
#include "google/fhir/seqex/example_key.h"
#include "google/fhir/seqex/feature_keys.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/stu3/bundle_to_versioned_resources_converter.h"
#include "google/fhir/stu3/util.h"
#include "google/fhir/systems/systems.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/codes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "proto/stu3/resources.pb.h"
#include "proto/stu3/version_config.pb.h"
#include "tensorflow/core/platform/env.h"

DEFINE_int32(max_sequence_length, 1000000,
             "Maximum length of emitted sequences");
DEFINE_string(trigger_time_redacted_features, "",
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

using std::string;

using ::google::fhir::StatusOr;
using ::google::fhir::stu3::google::EventLabel;
using ::google::fhir::stu3::google::EventTrigger;
using ::google::fhir::stu3::proto::Bundle;
using ::google::fhir::stu3::proto::Encounter;
using ::google::fhir::stu3::proto::EncounterStatusCode;
using ::google::fhir::stu3::proto::Medication;
using ::google::fhir::stu3::proto::Patient;
using ::google::fhir::stu3::proto::ReferenceId;
using ::google::fhir::stu3::proto::VersionConfig;
using ::tensorflow::Example;
using ::tensorflow::Feature;
using ::tensorflow::Features;
using ::tensorflow::Status;

namespace {

// From http://hl7.org/fhir/v3/ActCode
const char kClassInpatient[] = "IMP";
bool FeaturePrefixMatch(const string& feature,
                        const std::set<string>& prefix_set) {
  string name = feature;
  while (prefix_set.count(name) == 0 && name.rfind('.') != string::npos) {
    name = name.substr(0, name.rfind('.'));
  }
  return prefix_set.count(name) != 0;
}

void GetContextFeatures(const std::pair<ExampleKey, Features>& labels,
                        const Example& context, const int sequence_length,
                        const std::vector<absl::Time>& encounter_start_times,
                        google::protobuf::Map<string, Feature>* feature_map,
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

  // Do not add sequence length and context labels when generating sequence
  // label.
  if (!generate_sequence_label) {
    (*feature_map)[kSequenceLengthFeatureKey].mutable_int64_list()->add_value(
        sequence_length);
    // Add labels to context.
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
    const std::set<string>& redacted_features_for_example,
    google::protobuf::Map<string, ::tensorflow::FeatureList>* feature_list_map) {
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
  const int64 now = absl::ToUnixSeconds(trigger_timestamp);
  while (sequence_length < event_id_feature->feature_size() &&
         event_id_feature->feature(sequence_length).int64_list().value(0) ==
             now) {
    iter++;
    sequence_length++;
  }

  // Case 2: overwrite all features (but allocate no new sequence steps)
  while (sequence_length < event_id_feature->feature_size()) {
    const int64 current_time = absl::ToUnixSeconds(iter->first);
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
        const string& feature_name = empty_feature.first;
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
    const int64 delta_time =
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
        const string& feature_name = empty_feature.first;
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
  int64 event_id = absl::ToUnixSeconds(event_time);
  (*example->mutable_features()->mutable_feature())[kEventIdFeatureKey]
      .mutable_int64_list()
      ->add_value(event_id);
  // The encounter id is the start time of the encounter containing this event,
  // or min(event_id) for any event that occurs before the first encounter.
  auto iter = std::upper_bound(encounter_start_times.begin(),
                               encounter_start_times.end(), event_time);
  int index = iter - encounter_start_times.begin();
  int64 encounter_id = absl::ToUnixSeconds(encounter_start_times[index - 1]);
  (*example->mutable_features()->mutable_feature())[kEncounterIdFeatureKey]
      .mutable_int64_list()
      ->add_value(encounter_id);
}

// TODO: StatusOr<Reference>
bool GetReferenceId(const google::protobuf::Message& message, const string& field_name,
                    ReferenceId* reference_id) {
  const google::protobuf::Reflection* reflection = message.GetReflection();

  const string base_name = absl::StrCat(message.GetDescriptor()->name(), ".");
  std::vector<const google::protobuf::FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  for (const auto* field : fields) {
    const string name = absl::StrCat(base_name, field->json_name());
    if (name == field_name) {
      CHECK_EQ(field->cpp_type(), google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
          << "Field " << field_name << " has invalid cpp_type "
          << field->cpp_type();
      CHECK(!field->is_repeated())
          << "Field " << field_name << " is a repeated field";
      CHECK_EQ(field->message_type()->full_name(),
               ReferenceId::descriptor()->full_name())
          << "Field " << field_name << " has invalid message type "
          << field->message_type()->full_name();
      const google::protobuf::Message& child = reflection->GetMessage(message, field);
      reference_id->CopyFrom(child);
      return true;
    } else if (absl::StartsWith(field_name, name) &&
               field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
      const google::protobuf::Message& reference = reflection->GetMessage(message, field);
      string replaced_string;
      absl::string_view::size_type pos = field_name.find(name, 0);
      if (pos != absl::string_view::npos) {
        replaced_string.append(field_name.data(), pos);
        replaced_string.append(reference.GetDescriptor()->name());
        absl::string_view::size_type remaining_pos = pos + name.length();
        replaced_string.append(field_name.data() + remaining_pos,
                               field_name.length() - remaining_pos);
      } else {
        replaced_string = field_name;
      }
      return GetReferenceId(reference, replaced_string, reference_id);
    }
  }
  return false;
}

StatusOr<ExampleKey> ConvertTriggerEventToExampleKey(
    const string& patient_id, const EventTrigger& trigger) {
  if (!trigger.has_event_time()) {
    return ::tensorflow::errors::InvalidArgument("trigger-without-time");
  }
  ExampleKey key;
  key.patient_id = patient_id;

  key.trigger_timestamp = absl::FromUnixMicros(trigger.event_time().value_us());
  if (trigger.has_source()) {
    key.source = stu3::ReferenceProtoToString(trigger.source()).ValueOrDie();
  }
  // ExampleKey here only used in testing code, thus without start / end is OK,
  // but may make production debugging easier.
  key.start = -1;
  key.end = -1;
  return key;
}

Features ConvertCurrentEventLabelToTensorflowFeatures(
    const std::vector<EventLabel>& event_labels, absl::Time trigger_time) {
  Features result;
  for (const auto& event_label : event_labels) {
    Feature class_names;
    Feature integers;
    Feature floats;
    Feature datetime_secs;
    for (const auto& label : event_label.label()) {
      int label_value_types = 0;
      if (label.class_name().has_code()) {
        auto bytes_list = class_names.mutable_bytes_list();
        if (!label.class_name().code().value().empty()) {
          bytes_list->add_value(label.class_name().code().value());
          label_value_types++;
        }
      }
      if (label.has_class_value()) {
        if (label.class_value().has_integer()) {
          integers.mutable_int64_list()->add_value(
              label.class_value().integer().value());
        }
        if (label.class_value().has_decimal()) {
          double value;
          CHECK(stu3::GetDecimalValue(label.class_value().decimal(), &value)
                    .ok());
          floats.mutable_float_list()->add_value(value);
        }
        if (label.class_value().has_date_time()) {
           ::absl::Time date_time = stu3::GetTimeFromTimelikeElement(
               label.class_value().date_time());
           datetime_secs.mutable_int64_list()->add_value(
               absl::ToUnixSeconds(date_time));
        }
        CHECK(!label.class_value().has_boolean());  // TODO: implement
        label_value_types++;
      }
      CHECK_LE(label_value_types, 1);
    }
    // Even for current label, it's nice to have a .class suffix, as there are
    // companion features e.g. label event time for metrics.
    const string label_prefix =
        absl::StrCat("label.", event_label.type().code().value());
    (*result.mutable_feature())[absl::StrCat(label_prefix, ".class")] =
        class_names;
    ::absl::Time event_time =
        stu3::GetTimeFromTimelikeElement(event_label.event_time());
    (*result.mutable_feature())[absl::StrCat(label_prefix, ".timestamp_secs")]
        .mutable_int64_list()
        ->add_value(absl::ToUnixSeconds(event_time));
    if (integers.int64_list().value_size() > 0) {
      (*result
            .mutable_feature())[absl::StrCat(label_prefix, ".value_integer")] =
          integers;
    }
    if (floats.float_list().value_size() > 0) {
      (*result.mutable_feature())[absl::StrCat(label_prefix, ".value_float")] =
          floats;
    }
    if (datetime_secs.int64_list().value_size() > 0) {
      (*result.mutable_feature())[
          absl::StrCat(label_prefix, ".value_datetime_secs")] = datetime_secs;
    }
  }
  return result;
}

Status BuildLabelsFromTriggerLabelPair(
    const string& patient_id, const std::vector<TriggerLabelsPair>& labels,
    std::map<ExampleKey, Features>* label_map) {
  for (const auto& pair : labels) {
    auto result = ConvertTriggerEventToExampleKey(patient_id, pair.first);
    TF_RETURN_IF_ERROR(result.status());
    const ExampleKey key = result.ValueOrDie();
    if (label_map->find(key) == label_map->end()) {
      label_map->insert(
          std::make_pair(key, ConvertCurrentEventLabelToTensorflowFeatures(
                                  pair.second, key.trigger_timestamp)));
    }
  }
  return Status::OK();
}

}  // namespace

BundleToSeqexConverter::BundleToSeqexConverter(
    const stu3::proto::VersionConfig& fhir_version_config,
    const bool enable_attribution, const bool generate_sequence_label)
    : version_config_(fhir_version_config),
      enable_attribution_(enable_attribution),
      generate_sequence_label_(generate_sequence_label) {
  // Split the redacted feature list for easy access.
  redacted_features_ =
      absl::StrSplit(FLAGS_trigger_time_redacted_features, ',');
  // Make sure Done() would return true.
  current_label_ = label_map_.end();
}

bool BundleToSeqexConverter::Begin(
    const string& patient_id, const stu3::proto::Bundle& bundle,
    const std::map<struct ExampleKey, ::tensorflow::Features>& label_map,
    std::map<string, int>* counter_stats) {
  examples_.clear();
  encounter_start_times_.clear();
  context_.Clear();
  label_map_.clear();
  redacted_features_for_example_.clear();
  feature_types_.Clear();
  patient_id_ = patient_id;
  seqex_.Clear();
  cached_offset_ = 0;
  init_done_ = false;
  label_map_ = label_map;
  counter_stats_ = counter_stats;
  if (label_map_.empty()) {
    current_label_ = label_map_.end();  // mark done
    init_done_ = true;
    return false;
  }
  const stu3::proto::Bundle versioned_bundle =
      stu3::BundleToVersionedBundle(bundle, version_config_, counter_stats_);
  BundleToExamples(versioned_bundle);
  BundleToContext(versioned_bundle);
  return Next();
}

bool BundleToSeqexConverter::Begin(const string& patient_id,
                                   const stu3::proto::Bundle& bundle,
                                   const std::vector<TriggerLabelsPair>& labels,
                                   std::map<string, int>* counter_stats) {
  std::map<struct ExampleKey, ::tensorflow::Features> label_map;
  // TODO: Fail gracefully.
  CHECK(BuildLabelsFromTriggerLabelPair(patient_id, labels, &label_map).ok());
  return Begin(patient_id, bundle, label_map, counter_stats);
}

bool BundleToSeqexConverter::Next() {
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
    if (sequence_length > FLAGS_max_sequence_length) {
      offset = sequence_length - FLAGS_max_sequence_length;
      sequence_length = FLAGS_max_sequence_length;
    }
  }

  // Convert to feature-major. If the requested sequence start is not what
  // we expected, we have to start from scratch.
  if (offset != cached_offset_) {
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

  seqex_to_return_ = seqex_;
  return true;
}

// Get a list of non-overlapping encounter boundaries. For now, we use only
// inpatient encounters, and merge any encounters that overlap.
void BundleToSeqexConverter::GetEncounterBoundaries(
    const stu3::proto::Bundle& bundle,
    std::map<absl::Time, absl::Time>* encounter_boundaries) {
  // List inpatient encounter start and end times.
  std::map<absl::Time, absl::Time> inpatient_encounters;
  for (const auto& entry : bundle.entry()) {
    if (entry.resource().has_encounter()) {
      const auto& encounter = entry.resource().encounter();
      if (encounter.class_value().code().value() != kClassInpatient) {
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

void BundleToSeqexConverter::BundleToExamples(
    const stu3::proto::Bundle& bundle) {
  // Make a sequence sorted by timestamp.
  std::vector<std::pair<std::pair<absl::Time, string>, tensorflow::Example>>
      event_sequence;
  for (const auto& entry : bundle.entry()) {
    if (entry.resource().has_claim()) {
      ConvertResourceToExamples(entry.resource().claim(), bundle,
                                &event_sequence);
    }
    if (entry.resource().has_composition()) {
      ConvertResourceToExamples(entry.resource().composition(), bundle,
                                &event_sequence);
    }
    if (entry.resource().has_condition()) {
      ConvertResourceToExamples(entry.resource().condition(), bundle,
                                &event_sequence);
    }
    if (entry.resource().has_encounter()) {
      ConvertResourceToExamples(entry.resource().encounter(), bundle,
                                &event_sequence);
    }
    if (entry.resource().has_medication_administration()) {
      ConvertResourceToExamples(entry.resource().medication_administration(),
                                bundle, &event_sequence);
    }
    if (entry.resource().has_medication_request()) {
      ConvertResourceToExamples(entry.resource().medication_request(), bundle,
                                &event_sequence);
    }
    if (entry.resource().has_observation()) {
      ConvertResourceToExamples(entry.resource().observation(), bundle,
                                &event_sequence);
    }
    if (entry.resource().has_procedure()) {
      ConvertResourceToExamples(entry.resource().procedure(), bundle,
                                &event_sequence);
    }
    if (entry.resource().has_procedure_request()) {
      ConvertResourceToExamples(entry.resource().procedure_request(), bundle,
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
  std::sort(
      event_sequence.begin(), event_sequence.end(),
      [](const std::pair<std::pair<absl::Time, string>, tensorflow::Example>& a,
         const std::pair<std::pair<absl::Time, string>, tensorflow::Example>&
             b) { return a.first < b.first; });

  // Get a list of encounter boundary times.
  std::map<absl::Time, absl::Time> encounter_boundaries;
  GetEncounterBoundaries(bundle, &encounter_boundaries);
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
    AddBaggingFeatures(event.first.first, encounter_start_times_, &example);

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

      if (FeaturePrefixMatch(feature.first, redacted_features_)) {
        redacted_features_for_example_.insert(feature.first);
      }
    }
  }
}

void BundleToSeqexConverter::BundleToContext(
    const stu3::proto::Bundle& bundle) {
  // Add patient features to the context.
  for (const auto& entry : bundle.entry()) {
    if (entry.resource().has_patient()) {
      stu3::proto::Patient patient = entry.resource().patient();
      if (google::fhir::stu3::GetMetadataFromResource(patient)
              .version_id()
              .value() != "0") {
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
