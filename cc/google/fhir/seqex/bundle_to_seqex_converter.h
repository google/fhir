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

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/message.h"
#include "absl/time/time.h"
#include "google/fhir/seqex/example_key.h"
#include "google/fhir/seqex/feature_keys.h"
#include "google/fhir/seqex/resource_to_example.h"
#include "google/fhir/stu3/util.h"
#include "proto/stu3/google_extensions.pb.h"
#include "proto/stu3/resources.pb.h"
#include "proto/stu3/version_config.pb.h"
#include "tensorflow/core/example/example.pb.h"
#include "tensorflow/core/example/feature.pb.h"
#include "tensorflow/core/platform/logging.h"

namespace google {
namespace fhir {
namespace seqex {

using std::string;

typedef std::pair<stu3::google::EventTrigger,
                  std::vector<stu3::google::EventLabel>>
    TriggerLabelsPair;

// This class is not thread-safe.
class BundleToSeqexConverter {
 public:
  BundleToSeqexConverter(const stu3::proto::VersionConfig& fhir_version_config,
                         const bool enable_attribution,
                         const bool generate_sequence_label);

  // This API should be called once per bundle. When it returns, either the
  // iterator is Done(), or a valid key/example pair can be accessed using the
  // key() and example() accessors. To move to the next example, call Next();
  bool Begin(const string& patient_id, const stu3::proto::Bundle& bundle,
             const std::vector<TriggerLabelsPair>& labels,
             std::map<string, int>* counter_stats);

  // Move the iterator to the next key/example pair from the bundle.
  bool Next();

  // Return the example key.
  // Requires: !Done().
  string ExampleKey() {
    CHECK(!Done());
    return key_.ToString();
  }

  // Return the example key with a sixteen digit hex prefix based on a hash,
  // which makes it easier to shuffle the outputs.
  // Requires: !Done().
  string ExampleKeyWithPrefix() {
    CHECK(!Done());
    return key_.ToStringWithPrefix();
  }

  // Get the current SequenceExample. Requires: !Done().
  const ::tensorflow::SequenceExample& GetExample() {
    CHECK(!Done());
    return seqex_to_return_;
  }

  int ExampleSeqLen() {
    CHECK(!Done());
    return key_.end - key_.start;
  }

  // True if we have finished conversion for this bundle.
  bool Done() { return current_label_ == label_map_.end(); }

 private:
  bool Begin(const string& patient_id, const stu3::proto::Bundle& bundle,
             const std::map<struct seqex::ExampleKey, ::tensorflow::Features>&
                 label_map,
             std::map<string, int>* counter_stats);

  // Get a list of non-overlapping encounter boundaries. For now, we use only
  // inpatient encounters, and merge any encounters that overlap.
  void GetEncounterBoundaries(
      const stu3::proto::Bundle& bundle,
      std::map<absl::Time, absl::Time>* encounter_boundaries);

  // Convert a fhir bundle to a sequence of tf examples.
  // The result is stored in class member variables.
  void BundleToExamples(const stu3::proto::Bundle& bundle);

  // Extract context features from a fhir bundle. The result is stored in
  // class member variables.
  void BundleToContext(const stu3::proto::Bundle& bundle);

  // Converts a resource to one or more examples.
  template <typename R>
  void ConvertResourceToExamples(
      R resource, const stu3::proto::Bundle& bundle,
      std::vector<std::pair<std::pair<absl::Time, string>,
                            ::tensorflow::Example>>* event_sequence) {
    // Conversion from versioned resource to example is 1-1.
    const absl::Time version_time =
        google::fhir::stu3::GetTimeFromTimelikeElement(
            resource.meta().last_updated());
    ::tensorflow::Example example;
    seqex::ResourceToExample(resource, &example, enable_attribution_);
    if (enable_attribution_) {
      (*example.mutable_features()
            ->mutable_feature())[seqex::kResourceIdFeatureKey]
          .mutable_bytes_list()
          ->add_value(stu3::GetReferenceToResource(resource));
    }
    event_sequence->push_back(std::make_pair(
        std::make_pair(version_time, stu3::GetReferenceToResource(resource)),
        example));
  }

  stu3::proto::VersionConfig version_config_;
  std::set<string> redacted_features_;

  // These are computed once per Bundle.
  string patient_id_;
  std::vector<std::pair<absl::Time, ::tensorflow::Example>> examples_;
  std::vector<absl::Time> encounter_start_times_;
  ::tensorflow::Example context_;
  // ExampleKey -> labels.
  std::map<struct seqex::ExampleKey, ::tensorflow::Features> label_map_;
  std::map<string, int>* counter_stats_ = nullptr;

  bool init_done_;
  std::map<struct seqex::ExampleKey, ::tensorflow::Features>::iterator
      current_label_;
  std::set<string> redacted_features_for_example_;
  ::tensorflow::Features feature_types_;

  // The current sequence example.
  struct ExampleKey key_;
  // Internal seqex that contains all data.
  ::tensorflow::SequenceExample seqex_;
  // The seqex to be returned when GetExample() is called. It may be a trimmed
  // version of "seqex_".
  ::tensorflow::SequenceExample seqex_to_return_;
  int cached_offset_;

  bool enable_attribution_;

  bool generate_sequence_label_;
};

}  // namespace seqex
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_SEQEX_BUNDLE_TO_SEQEX_CONVERTER_H_
