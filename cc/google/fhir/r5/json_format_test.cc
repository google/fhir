/*
 * Copyright 2020 Google LLC
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

#include "google/fhir/r5/json_format.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/check.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/json/json_sax_handler.h"
#include "google/fhir/json/test_matchers.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/r5/operation_error_reporter.h"
#include "google/fhir/r5/primitive_handler.h"
#include "google/fhir/r5/resource_validation.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/testutil/generator.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "proto/google/fhir/proto/r5/core/codes.pb.h"
#include "proto/google/fhir/proto/r5/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/account.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/activity_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/actor_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/administrable_product_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/adverse_event.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/allergy_intolerance.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/appointment.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/appointment_response.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/artifact_assessment.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/audit_event.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/basic.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/binary.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/biologically_derived_product.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/biologically_derived_product_dispense.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/body_structure.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/capability_statement.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/care_plan.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/care_team.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/charge_item.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/charge_item_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/citation.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/claim.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/claim_response.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/clinical_impression.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/clinical_use_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/code_system.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/communication.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/communication_request.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/compartment_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/composition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/concept_map.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/condition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/condition_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/consent.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/contract.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/coverage.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/coverage_eligibility_request.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/coverage_eligibility_response.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/detected_issue.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/device.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/device_association.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/device_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/device_dispense.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/device_metric.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/device_request.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/device_usage.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/diagnostic_report.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/document_reference.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/encounter.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/encounter_history.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/endpoint.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/enrollment_request.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/enrollment_response.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/episode_of_care.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/event_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/evidence.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/evidence_report.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/evidence_variable.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/example_scenario.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/explanation_of_benefit.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/family_member_history.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/flag.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/formulary_item.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/genomic_study.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/goal.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/graph_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/group.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/guidance_response.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/healthcare_service.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/imaging_selection.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/imaging_study.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/immunization.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/immunization_evaluation.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/immunization_recommendation.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/implementation_guide.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/ingredient.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/insurance_plan.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/inventory_item.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/inventory_report.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/invoice.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/library.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/linkage.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/list.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/location.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/manufactured_item_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/measure.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/measure_report.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/medication.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/medication_administration.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/medication_dispense.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/medication_knowledge.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/medication_request.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/medication_statement.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/medicinal_product_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/message_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/message_header.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/molecular_sequence.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/naming_system.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/nutrition_intake.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/nutrition_order.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/nutrition_product.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/observation.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/observation_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/operation_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/operation_outcome.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/organization.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/organization_affiliation.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/packaged_product_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/parameters.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/patient.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/payment_notice.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/payment_reconciliation.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/permission.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/person.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/plan_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/practitioner.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/practitioner_role.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/procedure.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/provenance.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/questionnaire.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/questionnaire_response.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/regulated_authorization.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/related_person.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/request_orchestration.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/requirements.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/research_study.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/research_subject.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/risk_assessment.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/schedule.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/search_parameter.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/service_request.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/slot.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/specimen.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/specimen_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/structure_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/structure_map.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/subscription.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/subscription_status.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/subscription_topic.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/substance.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/substance_definition.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/substance_nucleic_acid.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/substance_polymer.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/substance_protein.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/substance_reference_information.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/substance_source_material.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/supply_delivery.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/supply_request.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/task.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/terminology_capabilities.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/test_plan.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/test_report.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/test_script.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/transport.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/value_set.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/verification_result.pb.h"
#include "proto/google/fhir/proto/r5/core/resources/vision_prescription.pb.h"
#include "google/protobuf/text_format.h"

namespace google {
namespace fhir {
namespace r5 {

namespace {

// TODO(b/193902436): We should have tests that run with a large number of
// threads to test concurrency.

using namespace ::google::fhir::r5::core;  // NOLINT
using ::google::fhir::r5::OperationOutcomeErrorHandler;
using ::google::fhir::testutil::EqualsProto;
using ::google::fhir::testutil::IgnoringRepeatedFieldOrdering;
using internal::JsonEq;
using ::testing::Eq;
using ::testing::UnorderedPointwise;

TEST(JsonFormatR5Test, PrintAndParseAllResources) {
  // Populate all fields to test edge cases, but recur only rarely to keep
  // the test fast.
  auto generator_params =
      google::fhir::testutil::RandomValueProvider::DefaultParams();
  generator_params.optional_set_probability = 1.0;
  generator_params.optional_set_ratio_per_level = 0.05;
  generator_params.max_string_length = 200;
  auto value_provider =
      std::make_unique<google::fhir::testutil::RandomValueProvider>(
          generator_params);

  google::fhir::testutil::FhirGenerator generator(
      std::move(value_provider),
      google::fhir::r5::R5PrimitiveHandler::GetInstance());
  const google::protobuf::Descriptor* descriptor =
      google::fhir::r5::core::ContainedResource::GetDescriptor();

  // Test populating all resources by iterating through all oneof fields
  // in the contained resource.
  for (int i = 0; i < descriptor->field_count(); ++i) {
    const google::protobuf::FieldDescriptor* resource_field = descriptor->field(i);

    google::fhir::r5::core::ContainedResource container;
    google::protobuf::Message* resource =
        container.GetReflection()->MutableMessage(&container, resource_field);

    // Skip bundle since all resources are covered elsewhere.
    if (google::fhir::IsMessageType<google::fhir::r5::core::Bundle>(
            *resource)) {
      continue;
    }

    FHIR_ASSERT_OK(generator.Fill(resource));
    FHIR_ASSERT_OK_AND_ASSIGN(
        std::string json, ::google::fhir::r5::PrintFhirToJsonString(*resource));

    // Ensure JSON strings are parsed into the correct proto.
    google::fhir::r5::core::ContainedResource parsed_container;
    google::protobuf::Message* parsed_resource =
        parsed_container.GetReflection()->MutableMessage(&parsed_container,
                                                         resource_field);
    FHIR_ASSERT_OK(::google::fhir::r5::MergeJsonFhirStringIntoProto(
        json, parsed_resource, absl::UTCTimeZone(), false));
    EXPECT_THAT(*resource, EqualsProto(*parsed_resource));

    // Ensure pre-parsed JSON objects are parsed into the correct proto.
    google::fhir::r5::core::ContainedResource parsed_container_from_object;
    google::protobuf::Message* parsed_resource_from_object =
        parsed_container_from_object.GetReflection()->MutableMessage(
            &parsed_container_from_object, resource_field);
    internal::FhirJson parsed_json;
    FHIR_ASSERT_OK(internal::ParseJsonValue(json, parsed_json));
    FHIR_ASSERT_OK(::google::fhir::r5::MergeJsonFhirObjectIntoProto(
        parsed_json, parsed_resource_from_object, absl::UTCTimeZone(), false));
    EXPECT_THAT(*resource, EqualsProto(*parsed_resource_from_object));
  }
}

void AddFatal(OperationOutcome& outcome, absl::string_view diagnostics,
              absl::string_view field) {
  google::protobuf::TextFormat::ParseFromString(
      absl::Substitute(R"pb(
                         severity { value: FATAL }
                         code { value: STRUCTURE }
                         diagnostics { value: "$0" }
                         expression { value: "$1" }
                       )pb",
                       diagnostics, field),
      outcome.add_issue());
}

void AddError(OperationOutcome& outcome, absl::string_view diagnostics,
              absl::string_view field) {
  google::protobuf::TextFormat::ParseFromString(
      absl::Substitute(R"pb(
                         severity { value: ERROR }
                         code { value: VALUE }
                         diagnostics { value: "$0" }
                         expression { value: "$1" }
                       )pb",
                       diagnostics, field),
      outcome.add_issue());
}

TEST(JsonFormatR5Test, ParserErrorHandlerAggregatesErrorsAndFatalsParseFails) {
  std::string raw_json = R"json(
  {
  "resourceType": "Observation",
  "id": "blood-glucose-abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz",
  "status": "finol",
  "category": [
    {"text": "Laboratory"},
    {"text": "with bad field", "squibalop": "schpat"}
  ],
  "subject": {
    "reference": "Patient/example",
    "display": "Amy Shaw"
  },
  "effectiveDateTime": "2005-07-0501",
  "referenceRange": [
    {
      "appliesTo": [
        {
          "coding": [
            {
              "system": " ",
              "code": "normal",
              "display": "Normal Range"
            }
          ],
          "text": "Normal Range"
        }
      ]
    }
  ]})json";

  OperationOutcome outcome;
  OperationOutcomeErrorHandler handler(&outcome);
  Observation resource;
  absl::Status merge_status = ::google::fhir::r5::MergeJsonFhirStringIntoProto(
      raw_json, &resource, absl::LocalTimeZone(), true, handler);

  OperationOutcome expected_outcome;
  OperationOutcomeErrorHandler expected_outcome_handler(&expected_outcome);

  AddFatal(expected_outcome, "No field `squibalop` in CodeableConcept",
           "Observation.category[1]");
  AddFatal(expected_outcome,
           "Unparseable JSON string for google.fhir.r5.core.DateTime",
           "Observation.ofType(DateTime)");
  AddFatal(expected_outcome,
           "Failed to convert `finol` to "
           "google.fhir.r5.core.ObservationStatusCode.Value: No matching enum "
           "found.",
           "Observation.status");
  AddError(expected_outcome, "Invalid input for google.fhir.r5.core.Id",
           "Observation.id");
  AddError(expected_outcome,
           "Input failed regex requirement for: google.fhir.r5.core.Id",
           "Observation.id");
  AddError(expected_outcome, "Invalid input for google.fhir.r5.core.Uri",
           "Observation.referenceRange[0].appliesTo[0].coding[0].system");
  AddError(expected_outcome,
           "Input failed regex requirement for: google.fhir.r5.core.Uri",
           "Observation.referenceRange[0].appliesTo[0].coding[0].system");
  AddError(expected_outcome, "missing-required-field", "Observation.code");
  AddError(expected_outcome, "missing-required-field", "Observation.status");
  AddError(expected_outcome, "empty-oneof", "Observation.effective");

  FHIR_ASSERT_STATUS(
      merge_status,
      "Merge failure when parsing JSON.  See ErrorHandler for more info.");
  EXPECT_THAT(
      handler.GetErrorsAndFatals(),
      UnorderedPointwise(EqualsProto(),
                         expected_outcome_handler.GetErrorsAndFatals()));
}

TEST(JsonFormatR5Test,
     ParserErrorHandlerAggregatesErrorsAndFatalsParseSucceeds) {
  std::string raw_json = R"json(
  {
    "resourceType": "Observation",
    "id":
    "blood-glucose-abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz",
    "status": "final",
    "subject": {
      "reference": "Patient/example",
      "display": "Amy Shaw"
    },
    "effectiveDateTime": "2005-07-05"
  })json";

  OperationOutcome outcome;
  OperationOutcomeErrorHandler handler(&outcome);
  Observation resource;
  absl::Status merge_status = ::google::fhir::r5::MergeJsonFhirStringIntoProto(
      raw_json, &resource, absl::LocalTimeZone(), true, handler);

  OperationOutcome expected_outcome;

  AddError(expected_outcome, "Invalid input for google.fhir.r5.core.Id",
           "Observation.id");
  AddError(expected_outcome,
           "Input failed regex requirement for: google.fhir.r5.core.Id",
           "Observation.id");
  AddError(expected_outcome, "missing-required-field", "Observation.code");

  // Merge succeeds despite data issues.
  FHIR_ASSERT_OK(merge_status);

  EXPECT_THAT(outcome,
              IgnoringRepeatedFieldOrdering(EqualsProto(expected_outcome)));
}

TEST(JsonFormatR5Test, ParseRelativeReferenceSucceeds) {
  std::string raw_json = R"json(
  {
  "resourceType": "Observation",
  "subject": {
    "reference": "Patient/example",
    "display": "Amy Shaw"
  }})json";

  OperationOutcome outcome;
  OperationOutcomeErrorHandler handler(&outcome);
  Observation resource;
  absl::Status merge_status = ::google::fhir::r5::MergeJsonFhirStringIntoProto(
      raw_json, &resource, absl::LocalTimeZone(), true, handler);

  Observation expected;
  ASSERT_TRUE(
      google::protobuf::TextFormat::ParseFromString(R"pb(
                                            subject {
                                              patient_id { value: "example" }
                                              display { value: "Amy Shaw" }
                                            }
                                          )pb",
                                          &expected));

  EXPECT_THAT(resource, EqualsProto(expected));
}

TEST(JsonFormatR5Test, ParseAbsoluteReferenceSucceeds) {
  std::string raw_json = R"json(
  {
  "resourceType": "Observation",
  "subject": {
    "reference": "www.patient.org/123",
    "display": "Amy Shaw"
  }})json";

  OperationOutcome outcome;
  OperationOutcomeErrorHandler handler(&outcome);
  Observation resource;
  absl::Status merge_status = ::google::fhir::r5::MergeJsonFhirStringIntoProto(
      raw_json, &resource, absl::LocalTimeZone(), true, handler);

  Observation expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        subject {
          uri { value: "www.patient.org/123" }
          display { value: "Amy Shaw" }
        }
      )pb",
      &expected));

  EXPECT_THAT(resource, EqualsProto(expected));
}

TEST(JsonFormatR5Test, PrintRelativeReferenceSucceeds) {
  Observation proto_form;
  ASSERT_TRUE(
      google::protobuf::TextFormat::ParseFromString(R"pb(
                                            subject {
                                              patient_id { value: "example" }
                                              display { value: "Amy Shaw" }
                                            }
                                          )pb",
                                          &proto_form));

  OperationOutcome outcome;
  OperationOutcomeErrorHandler handler(&outcome);
  Observation resource;
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string json,
      ::google::fhir::r5::PrettyPrintFhirToJsonString(proto_form));

  EXPECT_EQ(json, R"json({
  "resourceType": "Observation",
  "subject": {
    "reference": "Patient/example",
    "display": "Amy Shaw"
  }
})json");
}

TEST(JsonFormatR5Test, PrintAbsoluteReferenceSucceeds) {
  Observation proto_form;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        subject {
          uri { value: "www.patient.org/123" }
          display { value: "Amy Shaw" }
        }
      )pb",
      &proto_form));

  OperationOutcome outcome;
  OperationOutcomeErrorHandler handler(&outcome);
  Observation resource;
  FHIR_ASSERT_OK_AND_ASSIGN(
      std::string json,
      ::google::fhir::r5::PrettyPrintFhirToJsonString(proto_form));

  EXPECT_EQ(json, R"json({
  "resourceType": "Observation",
  "subject": {
    "reference": "www.patient.org/123",
    "display": "Amy Shaw"
  }
})json");
}

}  // namespace

}  // namespace r5
}  // namespace fhir
}  // namespace google
