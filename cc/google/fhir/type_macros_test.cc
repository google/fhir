/*
 * Copyright 2024 Google LLC
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

#include "google/fhir/type_macros.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/observation.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/structure_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/observation.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/structure_definition.pb.h"

namespace {

TEST(TypeMacros, BundleContainedResourceR4Works) {
  using T = BUNDLE_CONTAINED_RESOURCE(google::fhir::r4::core::Bundle);
  EXPECT_THAT(T::descriptor()->full_name(),
              "google.fhir.r4.core.ContainedResource");
}

TEST(TypeMacros, BundleContainedResourceR5Works) {
  using T = BUNDLE_CONTAINED_RESOURCE(google::fhir::r5::core::Bundle);
  EXPECT_THAT(T::descriptor()->full_name(),
              "google.fhir.r5.core.ContainedResource");
}

TEST(TypeMacros, BundleTypeR4Works) {
  using T = BUNDLE_TYPE(google::fhir::r4::core::Bundle, observation);
  EXPECT_THAT(T::descriptor()->full_name(), "google.fhir.r4.core.Observation");
}

TEST(TypeMacros, BundleTypeR5Works) {
  using T = BUNDLE_TYPE(google::fhir::r5::core::Bundle, observation);
  EXPECT_THAT(T::descriptor()->full_name(), "google.fhir.r5.core.Observation");
}

TEST(TypeMacros, ExtensionTypeR4Works) {
  using T = EXTENSION_TYPE(google::fhir::r4::core::Bundle);
  EXPECT_THAT(T::descriptor()->full_name(), "google.fhir.r4.core.Extension");
}

TEST(TypeMacros, ExtensionTypeR5Works) {
  using T = EXTENSION_TYPE(google::fhir::r5::core::Bundle);
  EXPECT_THAT(T::descriptor()->full_name(), "google.fhir.r5.core.Extension");
}

TEST(TypeMacros, FhirDatatypeR4Works) {
  using T = FHIR_DATATYPE(google::fhir::r4::core::Observation, code);
  EXPECT_THAT(T::descriptor()->full_name(), "google.fhir.r4.core.Code");
}

TEST(TypeMacros, FhirDatatypeR5Works) {
  using T = FHIR_DATATYPE(google::fhir::r5::core::Observation, code);
  EXPECT_THAT(T::descriptor()->full_name(), "google.fhir.r5.core.Code");
}

TEST(TypeMacros, ReferenceIdTypeR4Works) {
  using T = REFERENCE_ID_TYPE(google::fhir::r4::core::Reference);
  EXPECT_THAT(T::descriptor()->full_name(), "google.fhir.r4.core.ReferenceId");
}

TEST(TypeMacros, ReferenceIdTypeR5Works) {
  using T = REFERENCE_ID_TYPE(google::fhir::r5::core::Reference);
  EXPECT_THAT(T::descriptor()->full_name(), "google.fhir.r5.core.ReferenceId");
}

TEST(TypeMacros, ElementDefinitionTypeR4Works) {
  using T =
      ELEMENT_DEFINITION_TYPE(google::fhir::r4::core::StructureDefinition);
  EXPECT_THAT(T::descriptor()->full_name(),
              "google.fhir.r4.core.ElementDefinition");
}

TEST(TypeMacros, ElementDefinitionTypeR5Works) {
  using T =
      ELEMENT_DEFINITION_TYPE(google::fhir::r5::core::StructureDefinition);
  EXPECT_THAT(T::descriptor()->full_name(),
              "google.fhir.r5.core.ElementDefinition");
}

}  // namespace
