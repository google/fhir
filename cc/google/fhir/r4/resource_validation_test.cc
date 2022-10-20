/*
 * Copyright 2021 Google LLC
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

#include "google/fhir/r4/resource_validation.h"

#include <string>

#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/references.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/composition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/encounter.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/observation.pb.h"
#include "proto/google/fhir/proto/r4/fhirproto.pb.h"
#include "testdata/r4/profiles/test.pb.h"

namespace google {
namespace fhir {
namespace r4 {

namespace {

using namespace ::google::fhir::r4::core;  // NOLINT
using ::google::fhir::r4::fhirproto::ValidationOutcome;
using ::google::fhir::testutil::EqualsProtoIgnoringReordering;

template <typename T>
void ValidTest(const absl::string_view name,
               const bool has_resource_id = true) {
  T resource =
      ReadProto<T>(absl::StrCat("testdata/r4/validation/", name, ".prototxt"));
  absl::StatusOr<ValidationOutcome> outcome = Validate(resource);
  FHIR_ASSERT_OK(outcome.status());

  // The ValidationOutcome should be empty except for the subject, if the
  // resource has an ID.
  ValidationOutcome expected;
  if (has_resource_id) {
    FHIR_ASSERT_OK_AND_ASSIGN(
        *expected.mutable_subject(),
        GetReferenceProtoToResource<::google::fhir::r4::core::Reference>(
            resource));
  }
  EXPECT_THAT(*outcome, EqualsProtoIgnoringReordering(expected));
}

template <typename T>
void InvalidTest(absl::string_view name) {
  T resource =
      ReadProto<T>(absl::StrCat("testdata/r4/validation/", name, ".prototxt"));
  std::string error_msg =
      ReadFile(absl::StrCat("testdata/r4/validation/", name, ".result.txt"));
  if (error_msg[error_msg.length() - 1] == '\n') {
    error_msg.erase(error_msg.length() - 1);
  }

  absl::StatusOr<ValidationOutcome> outcome = Validate(resource);
  FHIR_ASSERT_OK(outcome.status());

  ValidationOutcome expected_outcome = ReadProto<ValidationOutcome>(
      absl::StrCat("testdata/r4/validation/", name, ".outcome.prototxt"));
  EXPECT_THAT(*outcome, EqualsProtoIgnoringReordering(expected_outcome));
}

TEST(ResourceValidationTest, MissingRequiredField) {
  InvalidTest<Observation>("observation_invalid_missing_required");
}

TEST(ResourceValidationTest, InvalidPrimitiveField) {
  InvalidTest<Observation>("observation_invalid_primitive");
}

TEST(ResourceValidationTest, ValidReference) {
  ValidTest<Observation>("observation_valid_reference");
}

TEST(ResourceValidationTest, InvalidReference) {
  InvalidTest<Observation>("observation_invalid_reference");
}

TEST(ResourceValidationTest, FHIRPathViolation) {
  InvalidTest<Observation>("observation_invalid_fhirpath_violation");
}

TEST(ResourceValidationTest, RepeatedReferenceValid) {
  ValidTest<Encounter>("encounter_valid_repeated_reference");
}

TEST(ResourceValidationTest, RepeatedReferenceInvalid) {
  InvalidTest<Encounter>("encounter_invalid_repeated_reference");
}

TEST(ResourceValidationTest, EmptyOneof) {
  InvalidTest<Observation>("observation_invalid_empty_oneof");
}

TEST(ResourceValidationTest, EmptyOneofWithoutId) {
  InvalidTest<Observation>("observation_invalid_empty_oneof_without_id");
}

TEST(BundleValidationTest, Valid) { ValidTest<Bundle>("bundle_valid"); }

TEST(EncounterValidationTest, Valid) {
  ValidTest<Encounter>("encounter_valid");
}

TEST(EncounterValidationTest, ValidWithNumericTimezone) {
  ValidTest<Encounter>("encounter_valid_numeric_timezone");
}

TEST(EncounterValidationTest, ValidWithoutResourceId) {
  ValidTest<Encounter>("encounter_valid_without_id", false);
}

TEST(EncounterValidationTest, FhirPathErrorsAndWarningsAreRecordedAsOutcomes) {
  InvalidTest<
      google::fhir::r4::testing::TestPatientWithWarningAndErrorFhirpath>(
      "patient_invalid_fhir_path_violation");
}

TEST(CompositionValidationTest, ValidCompositionWithReferences) {
  ValidTest<Composition>("composition_valid", false);
}

TEST(CompositionValidationTest, InvalidCompositionWithInvalidReferences) {
  InvalidTest<Composition>("composition_invalid_reference");
}

}  // namespace

}  // namespace r4
}  // namespace fhir
}  // namespace google
