// Copyright 2020 Google LLC
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

#ifndef GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_TYPES_H_
#define GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_TYPES_H_

#include "google/protobuf/message.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "google/fhir/status/statusor.h"

namespace google::fhir::fhir_path::internal {
enum class FhirPathSystemType {
  kBoolean = 1,
  kString = 2,
  kInteger = 3,
  kDecimal = 4,
  kDate = 5,
  kDateTime = 6,
  kTime = 7,
  kQuantity = 8
};

// Returns the FHIRPath primitive type the provided FHIR primitive maps to.
// Returns a NotFoundError if no mapping exists between the provided FHIR type
// and a FHIRPath primitive type.
//
// See https://www.hl7.org/fhir/fhirpath.html#types
absl::StatusOr<FhirPathSystemType> GetSystemType(
    const ::google::protobuf::Message& fhir_primitive);

// Returns true if the provided FHIR message converts to System.Integer. False
// otherwise.
//
// See https://www.hl7.org/fhir/fhirpath.html#types
bool IsSystemInteger(const ::google::protobuf::Message& message);

// Returns true if the provided FHIR message converts to System.String. False
// otherwise.
//
// See https://www.hl7.org/fhir/fhirpath.html#types
bool IsSystemString(const ::google::protobuf::Message& message);

// Returns true if the provided FHIR message converts to System.Decimal. False
// otherwise.
//
// See https://www.hl7.org/fhir/fhirpath.html#types
bool IsSystemDecimal(const ::google::protobuf::Message& message);

}  // namespace google::fhir::fhir_path::internal

#endif  // GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_TYPES_H_
