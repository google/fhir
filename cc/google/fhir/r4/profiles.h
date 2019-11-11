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

#ifndef GOOGLE_FHIR_R4_PROFILES_H_
#define GOOGLE_FHIR_R4_PROFILES_H_

#include "google/protobuf/message.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status.h"

namespace google {
namespace fhir {


// If <target> is a profiled type of <source>:
// Converts a resource to a profiled version of that resource.
// If the profile adds new inlined fields for Extensions or Codings within
// CodeableConcepts, and those extensions or codings are present on the base
// message, the data will be reorganized into the new fields in the profile.
// Finally, this runs the Fhir resource validation code on the resulting
// message, to ensure the result complies with the requirements of the profile
// (e.g., fields that are considered required by the profile).
//
// If <target> is a base type of <source>:
// Performs the inverse operation to the above.
// Any data that is in an inlined field in the profiled message,
// that does not exists in the base message will be converted back to the
// original format (e.g., extension).
Status ConvertToProfileR4(const ::google::protobuf::Message& source,
                          ::google::protobuf::Message* target);

Status ConvertToProfileLenientR4(const ::google::protobuf::Message& source,
                                 ::google::protobuf::Message* target);

// Normalizing a profiled proto ensures that all data that CAN be stored in
// profiled fields IS stored in profiled fields.
// E.g., if the message contains an extension in the raw extension field that
// has a corresponding typed field, the return copy will have the data in the
// typed field.
template <typename T>
StatusOr<T> NormalizeR4(const T& message) {
  T normalized;
  FHIR_RETURN_IF_ERROR(ConvertToProfileLenientR4(message, &normalized));
  return normalized;
}

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_R4_PROFILES_H_
