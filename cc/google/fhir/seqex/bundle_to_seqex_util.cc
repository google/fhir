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

#include "google/fhir/seqex/bundle_to_seqex_util.h"

#include <sys/types.h>

#include <map>
#include <memory>
#include <utility>

#include "google/protobuf/reflection.h"
#include "absl/time/time.h"
#include "google/fhir/extensions.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status.h"

using google::fhir::StatusOr;
using google::fhir::stu3::google::EventLabel;
using google::fhir::stu3::google::EventTrigger;
using google::fhir::stu3::proto::Bundle;
using google::fhir::stu3::proto::Extension;

namespace google {
namespace fhir {
namespace seqex {


namespace internal {

StatusOr<std::vector<EventLabel>> ExtractLabelsFromExtensions(
    const std::set<std::string>& label_names,
    google::protobuf::RepeatedFieldRef<Extension> extensions) {
  std::vector<EventLabel> labels;
  TF_RETURN_IF_ERROR(
      google::fhir::GetRepeatedFromExtension(extensions, &labels));
  std::vector<EventLabel> target_labels;
  for (const auto& label : labels) {
    if (label_names.count(label.type().code().value()) > 0) {
      target_labels.push_back(label);
    }
  }
  return target_labels;
}

void GetTriggerLabelsPairFromExtensions(
    const ::google::protobuf::RepeatedFieldRef<stu3::proto::Extension> extensions,
    const std::set<std::string>& label_names,
    const std::string& trigger_event_name,
    std::vector<TriggerLabelsPair>* trigger_labels_pair,
    int* num_triggers_filtered) {
  std::vector<stu3::google::EventTrigger> triggers;
  TF_CHECK_OK(google::fhir::GetRepeatedFromExtension(extensions, &triggers));
  // Note that this only joins triggers and labels within the same resource.
  auto labels_result = ExtractLabelsFromExtensions(label_names, extensions);
  std::vector<EventLabel> labels = labels_result.ValueOrDie();
  for (const auto& trigger : triggers) {
    if (trigger.type().code().value() != trigger_event_name) {
      continue;
    }
    absl::Time trigger_time =
        google::fhir::GetTimeFromTimelikeElement(trigger.event_time());
    bool should_keep = true;
    for (const EventLabel& label : labels) {
      if (label.has_event_time() && google::fhir::GetTimeFromTimelikeElement(
                                        label.event_time()) < trigger_time) {
        // If the label happens before the trigger, the trigger should be
        // thrown out. Note that there is no easy way to only throw out one
        // type of label because a missing label would be treated as a
        // negative example, not an example which should be skipped.
        should_keep = false;
        break;
      }
    }
    if (should_keep) {
      trigger_labels_pair->push_back(std::make_pair(trigger, labels));
    } else {
      ++(*num_triggers_filtered);
    }
  }
}

}  // namespace internal

void GetTriggerLabelsPairFromInputLabels(
    const std::vector<EventLabel>& input_labels,
    std::vector<TriggerLabelsPair>* trigger_labels_pair) {
  if (input_labels.empty()) {
    return;
  }
  std::map<absl::Time, uint> trigger_index_for_time;
  for (const auto& label : input_labels) {
    EventLabel trigger_label_template;
    CHECK(label.has_patient() && label.patient().has_patient_id() &&
          !label.patient().patient_id().value().empty())
        << label.DebugString();
    if (!trigger_label_template.has_patient()) {
      *trigger_label_template.mutable_patient() = label.patient();
    } else {
      CHECK_EQ(trigger_label_template.patient().patient_id().value(),
               label.patient().patient_id().value())
          << label.DebugString();
    }
    CHECK(label.has_type()) << label.DebugString();
    absl::Time time = GetTimeFromTimelikeElement(label.event_time());
    if (trigger_index_for_time.find(time) == trigger_index_for_time.end()) {
      trigger_labels_pair->emplace_back();
      *trigger_labels_pair->back().first.mutable_event_time() =
          label.event_time();
      if (label.has_source()) {
        *trigger_labels_pair->back().first.mutable_source() = label.source();
      }
      trigger_index_for_time[time] = trigger_labels_pair->size() - 1;
    }
    EventLabel trigger_labels(trigger_label_template);
    (*trigger_labels_pair)[trigger_index_for_time[time]].second.push_back(
        label);
  }
}

}  // namespace seqex
}  // namespace fhir
}  // namespace google
