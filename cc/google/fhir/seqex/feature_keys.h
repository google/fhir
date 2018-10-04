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

#ifndef GOOGLE_FHIR_SEQEX_FEATURE_KEYS_H_
#define GOOGLE_FHIR_SEQEX_FEATURE_KEYS_H_

namespace google {
namespace fhir {
namespace seqex {

extern const char kEncounterIdFeatureKey[];
extern const char kEventIdFeatureKey[];
extern const char kLabelEncounterIdFeatureKey[];
extern const char kLabelTimestampFeatureKey[];
extern const char kPatientIdFeatureKey[];
extern const char kResourceIdFeatureKey[];
extern const char kSequenceLengthFeatureKey[];

}  // namespace seqex
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_SEQEX_FEATURE_KEYS_H_
