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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/r4/operation_error_reporter.h"
#include "google/fhir/references.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/composition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/coverage_eligibility_request.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/encounter.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/observation.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/operation_outcome.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/practitioner_role.pb.h"
#include "proto/google/fhir/proto/r4/fhirproto.pb.h"
#include "proto/google/fhir/proto/r4/fhirproto_extensions.pb.h"
#include "testdata/r4/profiles/test.pb.h"
#include "google/protobuf/text_format.h"

namespace google {
namespace fhir {
namespace r4 {

namespace {

using namespace ::google::fhir::r4::core;  // NOLINT
using ::google::fhir::r4::OperationOutcomeErrorHandler;
using ::google::fhir::testutil::EqualsProto;
using ::google::fhir::testutil::IgnoringRepeatedFieldOrdering;

template <typename T>
void ValidTest(const absl::string_view name, const bool has_resource_id = true,
               const bool validate_reference_ids = false) {
  T resource =
      ReadProto<T>(absl::StrCat("testdata/r4/validation/", name, ".prototxt"));

  OperationOutcome outcome;
  OperationOutcomeErrorHandler error_handler(&outcome);
  FHIR_ASSERT_OK(Validate(resource, error_handler, validate_reference_ids));

  // The ValidationOutcome should be empty.
  OperationOutcome expected;
  EXPECT_THAT(outcome, IgnoringRepeatedFieldOrdering(EqualsProto(expected)));
}

template <typename T>
void InvalidTest(absl::string_view name,
                 const bool validate_reference_ids = false) {
  T resource =
      ReadProto<T>(absl::StrCat("testdata/r4/validation/", name, ".prototxt"));
  std::string error_msg =
      ReadFile(absl::StrCat("testdata/r4/validation/", name, ".result.txt"));
  if (!error_msg.empty()) {
    if (error_msg[error_msg.length() - 1] == '\n') {
      error_msg.erase(error_msg.length() - 1);
    }
  }
  OperationOutcome outcome;
  OperationOutcomeErrorHandler error_handler(&outcome);
  FHIR_ASSERT_OK(Validate(resource, error_handler, validate_reference_ids));

  OperationOutcome expected_outcome = ReadProto<OperationOutcome>(
      absl::StrCat("testdata/r4/validation/", name, ".outcome.prototxt"));
  EXPECT_THAT(outcome,
              IgnoringRepeatedFieldOrdering(EqualsProto(expected_outcome)));
}

TEST(ResourceValidationTest, MissingRequiredField) {
  InvalidTest<Observation>("observation_invalid_missing_required");
}

TEST(ResourceValidationTest,
     ResourceMissingRequiredMatchesDynamicValidatorWithCompatabilityModeOn) {
  InvalidTest<CoverageEligibilityRequest>(
      "coverage_eligibility_request_missing_required_matches_classical");
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

TEST(ResourceValidationTest, EmptyReference) {
  InvalidTest<Observation>("observation_empty_reference");
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

// Test with validate reference id flag false (The default behavior).
TEST(CompositionValidationTest, ValidCompositionWithValidateReferenceIdsFalse) {
  ValidTest<Composition>("composition_valid", false, false);
}
TEST(CompositionValidationTest,
     InvalidCompositionWithValidateReferenceIdsFalse) {
  InvalidTest<Composition>(
      "composition_invalid_reference_with_validate_reference_id_false", false);
}

// Test with validate reference id flag true (The new behavior).
TEST(CompositionValidationTest, ValidCompositionWithValidateReferenceIdsTrue) {
  ValidTest<Composition>("composition_valid", false, true);
}
TEST(CompositionValidationTest,
     InvalidCompositionWithValidateReferenceIdsTrue) {
  InvalidTest<Composition>(
      "composition_invalid_reference_with_validate_reference_id_true", true);
}

TEST(BundleValidationTest, ValidEmptyBundle) {
  InvalidTest<Bundle>("empty_bundle");
}

TEST(PractitionerRole, PractitionerRoleWithNonBoolExpression) {
  ValidTest<PractitionerRole>(
      "practitioner_role_with_ignored_invalid_constraint");
}

}  // namespace

}  // namespace r4
}  // namespace fhir
}  // namespace google
