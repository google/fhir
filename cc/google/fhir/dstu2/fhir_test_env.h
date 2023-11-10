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

#ifndef GOOGLE_FHIR_DSTU2_FHIR_TEST_ENV_H_
#define GOOGLE_FHIR_DSTU2_FHIR_TEST_ENV_H_

#include "google/fhir/dstu2/primitive_handler.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/type_macros.h"
#include "google/fhir/util.h"
#include "proto/google/fhir/proto/dstu2/resources.pb.h"

namespace google {
namespace fhir {
namespace dstu2 {

template <typename BundleType, typename PrimitiveHandlerType>
struct Dstu2FhirTestEnv {
  using PrimitiveHandler = PrimitiveHandlerType;

  // Primitives + Datatypes
  using Boolean = FHIR_DATATYPE(BundleType, boolean);
  using Code = FHIR_DATATYPE(BundleType, code);
  using CodeableConcept = FHIR_DATATYPE(BundleType, codeable_concept);
  using Coding = FHIR_DATATYPE(BundleType, coding);
  using DateTime = FHIR_DATATYPE(BundleType, date_time);
  using Decimal = FHIR_DATATYPE(BundleType, decimal);
  using Integer = FHIR_DATATYPE(BundleType, integer);
  using Period = FHIR_DATATYPE(BundleType, period);
  using PositiveInt = FHIR_DATATYPE(BundleType, positive_int);
  using Quantity = FHIR_DATATYPE(BundleType, quantity);
  using Range = FHIR_DATATYPE(BundleType, range);
  using Reference = FHIR_DATATYPE(BundleType, reference);
  using String = FHIR_DATATYPE(BundleType, string_value);
  using UnsignedInt = FHIR_DATATYPE(BundleType, unsigned_int);

  // Resources
  using Bundle = BundleType;
  using ContainedResource = BUNDLE_CONTAINED_RESOURCE(BundleType);

  using Binary = BUNDLE_TYPE(BundleType, binary);
  using Encounter = BUNDLE_TYPE(BundleType, encounter);
  using Claim = BUNDLE_TYPE(BundleType, claim);
  using Condition = BUNDLE_TYPE(BundleType, condition);
  using Composition = BUNDLE_TYPE(BundleType, composition);
  using Medication = BUNDLE_TYPE(BundleType, medication);
  using MedicationAdministration = BUNDLE_TYPE(BundleType,
                                               medication_administration);
  using Observation = BUNDLE_TYPE(BundleType, observation);
  using Organization = BUNDLE_TYPE(BundleType, organization);
  using Parameters = BUNDLE_TYPE(BundleType, parameters);
  using Patient = BUNDLE_TYPE(BundleType, patient);
  using Procedure = BUNDLE_TYPE(BundleType, procedure);
  using StructureDefinition = BUNDLE_TYPE(BundleType, structure_definition);
  using ValueSet = BUNDLE_TYPE(BundleType, value_set);
};

using Dstu2CoreTestEnv =
    Dstu2FhirTestEnv<dstu2::proto::Bundle, dstu2::DSTU2PrimitiveHandler>;

}  // namespace dstu2
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_DSTU2_FHIR_TEST_ENV_H_
