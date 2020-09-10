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

#include "google/fhir/fhir_path/fhir_path_types.h"

#include "absl/strings/string_view.h"
#include "google/fhir/annotations.h"

namespace google::fhir::fhir_path::internal {

using ::google::protobuf::Message;

StatusOr<FhirPathSystemType> GetSystemType(
    const ::google::protobuf::Message& fhir_primitive) {
  static const auto* type_map =
      new absl::flat_hash_map<absl::string_view, FhirPathSystemType>(
          {{"http://hl7.org/fhir/StructureDefinition/boolean",
            FhirPathSystemType::kBoolean},
           {"http://hl7.org/fhir/StructureDefinition/string",
            FhirPathSystemType::kString},
           {"http://hl7.org/fhir/StructureDefinition/uri",
            FhirPathSystemType::kString},
           {"http://hl7.org/fhir/StructureDefinition/url",
            FhirPathSystemType::kString},
           {"http://hl7.org/fhir/StructureDefinition/canonical",
            FhirPathSystemType::kString},
           {"http://hl7.org/fhir/StructureDefinition/code",
            FhirPathSystemType::kString},
           {"http://hl7.org/fhir/StructureDefinition/oid",
            FhirPathSystemType::kString},
           {"http://hl7.org/fhir/StructureDefinition/id",
            FhirPathSystemType::kString},
           {"http://hl7.org/fhir/StructureDefinition/uuid",
            FhirPathSystemType::kString},
           {"http://hl7.org/fhir/StructureDefinition/sid",
            FhirPathSystemType::kString},
           {"http://hl7.org/fhir/StructureDefinition/markdown",
            FhirPathSystemType::kString},
           {"http://hl7.org/fhir/StructureDefinition/base64Binary",
            FhirPathSystemType::kString},
           {"http://hl7.org/fhir/StructureDefinition/integer",
            FhirPathSystemType::kInteger},
           {"http://hl7.org/fhir/StructureDefinition/unsignedInt",
            FhirPathSystemType::kInteger},
           {"http://hl7.org/fhir/StructureDefinition/positiveInt",
            FhirPathSystemType::kInteger},
           {"http://hl7.org/fhir/StructureDefinition/decimal",
            FhirPathSystemType::kDecimal},
           {"http://hl7.org/fhir/StructureDefinition/date",
            FhirPathSystemType::kDate},
           {"http://hl7.org/fhir/StructureDefinition/dateTime",
            FhirPathSystemType::kDateTime},
           {"http://hl7.org/fhir/StructureDefinition/instant",
            FhirPathSystemType::kDateTime},
           {"http://hl7.org/fhir/StructureDefinition/time",
            FhirPathSystemType::kTime},
           {"http://hl7.org/fhir/StructureDefinition/Quantity",
            FhirPathSystemType::kQuantity}});

  auto type =
      type_map->find(GetStructureDefinitionUrl(fhir_primitive.GetDescriptor()));
  if (type == type_map->end()) {
    return absl::NotFoundError(
        absl::StrCat(fhir_primitive.GetTypeName(),
                     " does not map to a FHIRPath primitive type."));
  }
  return type->second;
}

bool IsSystemInteger(const Message& message) {
  StatusOr<FhirPathSystemType> type = GetSystemType(message);
  return type.ok() && type.value() == FhirPathSystemType::kInteger;
}

bool IsSystemString(const Message& message) {
  StatusOr<FhirPathSystemType> type = GetSystemType(message);
  return type.ok() && type.value() == FhirPathSystemType::kString;
}

bool IsSystemDecimal(const Message& message) {
  StatusOr<FhirPathSystemType> type = GetSystemType(message);
  return type.ok() && type.value() == FhirPathSystemType::kDecimal;
}

}  // namespace google::fhir::fhir_path::internal
