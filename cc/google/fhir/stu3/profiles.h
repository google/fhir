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

#ifndef GOOGLE_FHIR_STU3_PROFILES_H_
#define GOOGLE_FHIR_STU3_PROFILES_H_

#include "google/protobuf/message.h"
#include "google/fhir/status/status.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;

// Converts a resource to a profiled version of that resource.
// If the profile adds new inlined fields for Extensions or Codings within
// CodeableConcepts, and those extensions or codings are present on the base
// message, the data will be reorganized into the new fields in the profile.
// Finally, this runs the Fhir resource validation code on the resulting
// message, to ensure the result complies with the requirements of the profile
// (e.g., fields that are considered required by the profile).
Status ConvertToProfile(const google::protobuf::Message& base_message,
                        google::protobuf::Message* profiled_message);

// Identical to ConvertToProfile, except does not run the validation step.
Status ConvertToProfileLenient(const google::protobuf::Message& base_message,
                               google::protobuf::Message* profiled_message);

// Performs the inverse operation to ConvertToProfile.
// Any data that is in an inlined field in the profiled message,
// that does not exists in the base message will be converted back to the
// original format (e.g., extension).
Status ConvertToBaseResource(const google::protobuf::Message& profiled_message,
                             google::protobuf::Message* base_message);

}  // namespace stu3
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_PROFILES_H_
