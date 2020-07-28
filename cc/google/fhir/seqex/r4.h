/*
 * Copyright 2020 Google LLC
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

#ifndef GOOGLE_FHIR_SEQEX_R4_H_
#define GOOGLE_FHIR_SEQEX_R4_H_

#include "google/fhir/r4/primitive_handler.h"
#include "google/fhir/seqex/bundle_to_seqex_converter.h"
#include "google/fhir/seqex/bundle_to_seqex_util.h"
#include "google/fhir/seqex/converter_types.h"
#include "google/fhir/seqex/resource_to_example.h"
#include "google/fhir/seqex/text_tokenizer.h"
#include "proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/r4/core/resources/claim.pb.h"
#include "proto/r4/core/resources/composition.pb.h"
#include "proto/r4/core/resources/condition.pb.h"
#include "proto/r4/core/resources/encounter.pb.h"
#include "proto/r4/core/resources/medication.pb.h"
#include "proto/r4/core/resources/medication_administration.pb.h"
#include "proto/r4/core/resources/medication_request.pb.h"
#include "proto/r4/core/resources/observation.pb.h"
#include "proto/r4/core/resources/patient.pb.h"
#include "proto/r4/core/resources/procedure.pb.h"
#include "proto/r4/core/resources/service_request.pb.h"
#include "proto/r4/google_extensions.pb.h"

namespace google {
namespace fhir {
namespace seqex_r4 {

// Namespace for aliases for seqex functions for use with the R4 FHIR version.

inline void ResourceToExample(const google::protobuf::Message& message,
                              const seqex::TextTokenizer& tokenizer,
                              ::tensorflow::Example* example,
                              bool enable_attribution) {
  ResourceToExample(message, tokenizer, example, enable_attribution,
                    ::google::fhir::r4::R4PrimitiveHandler::GetInstance());
}

typedef seqex::ConverterTypes<google::fhir::r4::google::EventTrigger,
                              google::fhir::r4::google::EventLabel,
                              r4::R4PrimitiveHandler>
    ConverterTypes;

inline void GetTriggerLabelsPairFromInputLabels(
    const std::vector<ConverterTypes::EventLabel>& input_labels,
    std::vector<ConverterTypes::TriggerLabelsPair>* trigger_labels_pair) {
  seqex::GetTriggerLabelsPairFromInputLabels<ConverterTypes>(
      input_labels, trigger_labels_pair);
}

template <typename BundleLike>
inline void GetTriggerLabelsPair(
    const BundleLike& bundle, const std::set<std::string>& label_names,
    const std::string& trigger_event_name,
    std::vector<typename ConverterTypes::TriggerLabelsPair>*
        trigger_labels_pair,
    int* num_triggers_filtered) {
  seqex::GetTriggerLabelsPair<ConverterTypes, BundleLike>(
      bundle, label_names, trigger_event_name, trigger_labels_pair,
      num_triggers_filtered);
}

template <typename BundleType>
class BundleToSeqexConverter
    : public seqex::BundleToSeqexConverter<ConverterTypes, BundleType> {
 public:
  BundleToSeqexConverter(const proto::VersionConfig& fhir_version_config,
                         std::shared_ptr<const seqex::TextTokenizer> tokenizer,
                         const bool enable_attribution,
                         const bool generate_sequence_label)
      : seqex::BundleToSeqexConverter<ConverterTypes, BundleType>(
            fhir_version_config, tokenizer, enable_attribution,
            generate_sequence_label) {}

 protected:
  using ContainedResourceLike = BUNDLE_CONTAINED_RESOURCE(BundleType);

  absl::Status AddContainedResource(
      const ContainedResourceLike& contained,
      std::vector<
          std::pair<std::pair<absl::Time, std::string>, tensorflow::Example>>*
          event_sequence) const override {
    FHIR_RETURN_IF_ERROR(
        BundleToSeqexConverter::AddCommonResource(contained, event_sequence));
    if (contained.has_service_request()) {
      FHIR_RETURN_IF_ERROR(
          (seqex::BundleToSeqexConverter<ConverterTypes, BundleType>::
               ConvertResourceToExamples(contained.service_request(),
                                         event_sequence)));
    }
    return absl::OkStatus();
  }
};

// Concrete unprofiled BundleToSeqexConverter specification for use by
// python-c++ boundary
typedef BundleToSeqexConverter<r4::core::Bundle>
    UnprofiledBundleToSeqexConverter;

}  // namespace seqex_r4
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_SEQEX_R4_H_
