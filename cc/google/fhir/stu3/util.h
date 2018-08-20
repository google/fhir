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

#ifndef GOOGLE_FHIR_STU3_UTIL_H_
#define GOOGLE_FHIR_STU3_UTIL_H_

#include "google/protobuf/message.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;
using ::google::fhir::stu3::proto::CodeableConcept;

// Extract code value for a given system code. Return as soon as we find one.
StatusOr<string> ExtractCodeBySystem(const CodeableConcept& codeable_concept,
                                     absl::string_view system_value);

// Extract the icd code for the given schemes.
StatusOr<string> ExtractIcdCode(const CodeableConcept& codeable_concept,
                                const std::vector<string>& schemes);

template <typename R>
stu3::proto::Meta* MutableMetadataFromResource(R* resource) {
  return resource->mutable_meta();
}

// Builds an absl::Time from a time-like fhir Element.
// Must have a value_us field.
template <class T>
absl::Time GetTimeFromTimelikeElement(const T& timelike) {
  return absl::FromUnixMicros(timelike.value_us());
}

// Populates the resource oneof on ContainedResource with the passed-in
// resource.
Status SetContainedResource(const google::protobuf::Message& resource,
                            stu3::proto::ContainedResource* contained);

// Returns the input resource, wrapped in a ContainedResource
StatusOr<stu3::proto::ContainedResource> WrapContainedResource(
    const google::protobuf::Message& resource);

}  // namespace stu3
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_UTIL_H_
