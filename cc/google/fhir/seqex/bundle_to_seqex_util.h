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

#ifndef GOOGLE_FHIR_SEQEX_BUNDLE_TO_SEQEX_UTIL_H_
#define GOOGLE_FHIR_SEQEX_BUNDLE_TO_SEQEX_UTIL_H_

#include <set>
#include <string>
#include <vector>

#include "google/protobuf/reflection.h"
#include "google/fhir/extensions.h"
#include "google/fhir/seqex/bundle_to_seqex_converter.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "proto/stu3/resources.pb.h"

namespace google {
namespace fhir {
namespace seqex {

namespace internal {

StatusOr<std::vector<stu3::google::EventLabel>> ExtractLabelsFromExtensions(
    const std::set<std::string>& label_names,
    google::protobuf::RepeatedFieldRef<stu3::proto ::Extension> extensions);

void GetTriggerLabelsPairFromExtensions(
    const ::google::protobuf::RepeatedFieldRef<stu3::proto::Extension> extensions,
    const std::set<std::string>& label_names,
    const std::string& trigger_event_name,
    std::vector<TriggerLabelsPair>* trigger_labels_pair,
    int* num_triggers_filtered);

}  // namespace internal


// Group label events by event time, create a trigger proto for each group, and
// format as a TriggerLabelsPair. The output is guaranteed to be sorted.
void GetTriggerLabelsPairFromInputLabels(
    const std::vector<stu3::google::EventLabel>& input_labels,
    std::vector<TriggerLabelsPair>* trigger_labels_pair);

// Extract triggers and labels from the provided bundle, and format as
// TriggerLabelsPair. The output is guaranteed to be sorted.
template <typename BundleLike>
void GetTriggerLabelsPair(const BundleLike& bundle,
                          const std::set<std::string>& label_names,
                          const std::string& trigger_event_name,
                          std::vector<TriggerLabelsPair>* trigger_labels_pair,
                          int* num_triggers_filtered) {
  for (const auto& entry : bundle.entry()) {
    auto result = GetResourceExtensionsFromBundleEntry(entry);
    if (!result.ok()) {
      continue;
    }

    internal::GetTriggerLabelsPairFromExtensions(
        result.ValueOrDie(), label_names, trigger_event_name,
        trigger_labels_pair, num_triggers_filtered);
  }
}

template <typename BundleLike>
std::vector<stu3::google::EventLabel> ExtractLabelsFromBundle(
    const BundleLike& bundle, const std::set<std::string>& label_names) {
  std::vector<stu3::google::EventLabel> labels;
  for (const auto& entry : bundle.entry()) {
    auto result = GetResourceExtensionsFromBundleEntry(entry);
    if (!result.ok()) {
      continue;
    }
    auto extensions = result.ValueOrDie();
    auto labels_result =
        internal::ExtractLabelsFromExtensions(label_names, extensions);
    if (!labels_result.ok()) {
      continue;
    }
    std::vector<stu3::google::EventLabel>& new_labels =
        labels_result.ValueOrDie();
    labels.insert(labels.end(), new_labels.begin(), new_labels.end());
  }
  return labels;
}

}  // namespace seqex
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_SEQEX_BUNDLE_TO_SEQEX_UTIL_H_
