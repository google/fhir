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

#include "google/fhir/seqex/feature_keys.h"

namespace google {
namespace fhir {
namespace seqex {

const char kEncounterIdFeatureKey[] = "encounterId";
const char kEventIdFeatureKey[] = "eventId";
const char kLabelEncounterIdFeatureKey[] = "currentEncounterId";
const char kLabelTimestampFeatureKey[] = "timestamp";
const char kPatientIdFeatureKey[] = "patientId";
const char kResourceIdFeatureKey[] = "resourceId";
const char kSequenceLengthFeatureKey[] = "sequenceLength";

}  // namespace seqex
}  // namespace fhir
}  // namespace google
