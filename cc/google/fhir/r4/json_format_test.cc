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

#include "google/fhir/r4/json_format.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/substitute.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/json/json_sax_handler.h"
#include "google/fhir/json/test_matchers.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/r4/operation_error_reporter.h"
#include "google/fhir/r4/primitive_handler.h"
#include "google/fhir/r4/profiles.h"
#include "google/fhir/status/status.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/generator.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "proto/google/fhir/proto/r4/core/codes.pb.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/profiles/observation_genetics.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/account.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/activity_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/adverse_event.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/allergy_intolerance.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/appointment.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/appointment_response.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/audit_event.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/basic.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/binary.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/biologically_derived_product.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/body_structure.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/capability_statement.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/care_plan.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/care_team.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/catalog_entry.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/charge_item.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/charge_item_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/claim.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/claim_response.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/clinical_impression.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/code_system.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/communication.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/communication_request.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/compartment_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/composition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/concept_map.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/condition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/consent.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/contract.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/coverage.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/coverage_eligibility_request.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/coverage_eligibility_response.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/detected_issue.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/device.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/device_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/device_metric.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/device_request.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/device_use_statement.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/diagnostic_report.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/document_manifest.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/document_reference.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/effect_evidence_synthesis.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/encounter.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/endpoint.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/enrollment_request.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/enrollment_response.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/episode_of_care.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/event_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/evidence.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/evidence_variable.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/example_scenario.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/explanation_of_benefit.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/family_member_history.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/flag.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/goal.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/graph_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/group.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/guidance_response.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/healthcare_service.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/imaging_study.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/immunization.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/immunization_evaluation.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/immunization_recommendation.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/implementation_guide.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/insurance_plan.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/invoice.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/library.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/linkage.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/list.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/location.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/measure.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/measure_report.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/media.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medication.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medication_administration.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medication_dispense.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medication_knowledge.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medication_request.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medication_statement.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medicinal_product.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medicinal_product_authorization.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medicinal_product_contraindication.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medicinal_product_indication.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medicinal_product_ingredient.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medicinal_product_interaction.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medicinal_product_manufactured.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medicinal_product_packaged.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medicinal_product_pharmaceutical.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/medicinal_product_undesirable_effect.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/message_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/message_header.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/metadata_resource.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/molecular_sequence.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/naming_system.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/nutrition_order.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/observation.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/observation_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/operation_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/operation_outcome.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/organization.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/organization_affiliation.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/parameters.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/patient.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/payment_notice.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/payment_reconciliation.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/person.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/plan_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/practitioner.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/practitioner_role.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/procedure.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/provenance.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/questionnaire.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/questionnaire_response.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/related_person.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/request_group.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/research_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/research_element_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/research_study.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/research_subject.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/risk_assessment.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/risk_evidence_synthesis.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/schedule.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/search_parameter.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/service_request.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/slot.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/specimen.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/specimen_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/structure_definition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/structure_map.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/subscription.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/substance.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/substance_nucleic_acid.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/substance_polymer.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/substance_protein.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/substance_reference_information.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/substance_source_material.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/substance_specification.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/supply_delivery.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/supply_request.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/task.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/terminology_capabilities.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/test_report.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/test_script.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/value_set.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/verification_result.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/vision_prescription.pb.h"
#include "testdata/r4/profiles/test.pb.h"

namespace google {
namespace fhir {
namespace r4 {

namespace {

// TODO(b/193902436): We should have tests that run with a large number of
// threads to test concurrency.

using namespace ::google::fhir::r4::core;  // NOLINT
using ::google::fhir::r4::OperationOutcomeErrorHandler;
using ::google::fhir::testutil::EqualsProto;
using ::google::fhir::testutil::IgnoringRepeatedFieldOrdering;
using internal::JsonEq;
using ::testing::Eq;
using ::testing::UnorderedPointwise;

static const char* const kTimeZoneString = "Australia/Sydney";

// json_path should be relative to fhir root
template <typename R>
absl::StatusOr<R> ParseJsonToProto(const std::string& json_path) {
  // Some examples are invalid fhir.
  // See:
  // https://gforge.hl7.org/gf/project/fhir/tracker/?action=TrackerItemEdit&tracker_item_id=24933
  // https://jira.hl7.org/browse/FHIR-26461
  static std::unordered_set<std::string> INVALID_RECORDS{
      "spec/hl7.fhir.r4.examples/4.0.1/package/Bundle-dataelements.json",
      "spec/hl7.fhir.r4.examples/4.0.1/package/Questionnaire-qs1.json",
      "spec/hl7.fhir.r4.examples/4.0.1/package/"
      "Observation-clinical-gender.json",
      "spec/hl7.fhir.r4.examples/4.0.1/package/DeviceMetric-example.json",
      "spec/hl7.fhir.r4.examples/4.0.1/package/DeviceUseStatement-example.json",
      "spec/hl7.fhir.r4.examples/4.0.1/package/"
      "MedicationRequest-medrx0301.json"};

  std::string json = ReadFile(json_path);
  absl::TimeZone tz;
  absl::LoadTimeZone(kTimeZoneString, &tz);
  FHIR_ASSIGN_OR_RETURN(R resource,
                        JsonFhirStringToProtoWithoutValidating<R>(json, tz));

  if (INVALID_RECORDS.find(json_path) == INVALID_RECORDS.end()) {
    FHIR_RETURN_IF_ERROR(
        Validate(resource, FailFastErrorHandler::FailOnErrorOrFatal()));
  }
  return resource;
}

// proto_path should be relative to //testdata/r4
// json_path should be relative to fhir root
// This difference is because we read most json test data from the spec package,
// rather than the testdata directory.
template <typename R>
void TestParseWithFilepaths(const std::string& proto_path,
                            const std::string& json_path) {
  absl::StatusOr<R> from_json_status = ParseJsonToProto<R>(json_path);
  ASSERT_TRUE(from_json_status.ok()) << "Failed parsing: " << json_path << "\n"
                                     << from_json_status.status();
  R from_json = from_json_status.value();
  R from_disk = ReadProto<R>(proto_path);

  ::google::protobuf::util::MessageDifferencer differencer;
  std::string differences;
  differencer.ReportDifferencesToString(&differences);
  EXPECT_TRUE(differencer.Compare(from_disk, from_json)) << differences;
}

template <typename R>
void TestPrintWithFilepaths(const std::string& proto_path,
                            const std::string& json_path) {
  const R proto = ReadProto<R>(proto_path);
  absl::StatusOr<std::string> from_proto_status =
      PrettyPrintFhirToJsonString(proto);
  ASSERT_TRUE(from_proto_status.ok())
      << "Failed Printing on: " << proto_path << ": "
      << from_proto_status.status().message();

  internal::FhirJson from_proto, from_json;
  ASSERT_TRUE(
      internal::ParseJsonValue(from_proto_status.value(), from_proto).ok());
  ASSERT_TRUE(internal::ParseJsonValue(ReadFile(json_path), from_json).ok());
  EXPECT_THAT(from_proto, JsonEq(&from_json));
}

template <typename R>
void TestPrint(const std::string& name) {
  TestPrintWithFilepaths<R>(
      absl::StrCat("testdata/r4/examples/", name, ".prototxt"),
      absl::StrCat("spec/hl7.fhir.r4.examples/4.0.1/package/", name + ".json"));
}

template <typename R>
void TestPairWithFilePaths(const std::string& proto_path,
                           const std::string& json_path) {
  TestPrintWithFilepaths<R>(proto_path, json_path);
  TestParseWithFilepaths<R>(proto_path, json_path);
}

template <typename R>
void TestPair(const std::vector<std::string>& file_names) {
  for (const std::string& name : file_names) {
    TestPairWithFilePaths<R>(
        absl::StrCat("testdata/r4/examples/", name, ".prototxt"),
        absl::StrCat("spec/hl7.fhir.r4.examples/4.0.1/package/",
                     name + ".json"));
  }
}

/** Test printing from a profile */
TEST(JsonFormatR4Test, PrintProfile) {
  TestPrintWithFilepaths<r4::testing::TestPatient>(
      "testdata/r4/profiles/test_patient-profiled-testpatient.prototxt",
      "testdata/r4/profiles/test_patient.json");
}

/** Test parsing to a profile */
TEST(JsonFormatR4Test, ParseProfile) {
  TestParseWithFilepaths<r4::testing::TestPatient>(
      "testdata/r4/profiles/test_patient-profiled-testpatient.prototxt",
      "testdata/r4/profiles/test_patient.json");
}

// Test parsing to a profile fails if the parsed resource doesn't match the
// profile
TEST(JsonFormatR4Test, ParseProfileMismatch) {
  EXPECT_FALSE(ParseJsonToProto<r4::testing::TestPatient>(
                   "testdata/r4/profiles/test_patient_multiple_names.json")
                   .ok());
}

template <typename R>
void TestPrintForAnalytics(const std::string& proto_filepath,
                           const std::string& json_filepath, bool pretty) {
  R proto = ReadProto<R>(proto_filepath);
  if (IsProfile(R::descriptor())) {
    proto = NormalizeR4(proto).value();
  }
  auto result = pretty ? PrettyPrintFhirToJsonStringForAnalytics(proto)
                       : PrintFhirToJsonStringForAnalytics(proto);
  ASSERT_TRUE(result.ok()) << "Failed PrintForAnalytics on: " << proto_filepath
                           << "\n"
                           << result.status();

  internal::FhirJson from_proto, from_json;
  ASSERT_TRUE(internal::ParseJsonValue(result.value(), from_proto).ok());
  ASSERT_TRUE(
      internal::ParseJsonValue(ReadFile(json_filepath), from_json).ok());
  EXPECT_THAT(from_proto.toString(), Eq(from_json.toString()));
}

template <typename R>
void TestPrintForAnalyticsWithFilepath(const std::string& proto_filepath,
                                       const std::string& json_filepath,
                                       bool pretty = false) {
  TestPrintForAnalytics<R>(proto_filepath, json_filepath, pretty);
}

template <typename R>
void TestPrintForAnalytics(const std::string& name) {
  TestPrintForAnalyticsWithFilepath<R>(
      absl::StrCat("testdata/r4/examples/", name, ".prototxt"),
      absl::StrCat("testdata/r4/bigquery/", name + ".json"));
}

template <typename R>
void TestPrettyPrintForAnalytics(const std::string& name) {
  TestPrintForAnalyticsWithFilepath<R>(
      absl::StrCat("testdata/r4/examples/", name, ".prototxt"),
      absl::StrCat("testdata/r4/bigquery/", name + ".json"),
      /*pretty=*/true);
}

TEST(JsonFormatR4Test, PrintForAnalytics) {
  TestPrintForAnalytics<Composition>("Composition-example");
  // TODO(b/304628360): Update Java, python and other potential json FHIR
  // parsers to print a string representation of contained resources.
  TestPrintForAnalytics<Encounter>("Encounter-home-contained");
  TestPrintForAnalytics<Observation>("Observation-example-genetics-1");
  TestPrintForAnalytics<Patient>("Patient-example");
}

TEST(JsonFormatR4Test, PrettyPrintForAnalytics) {
  TestPrettyPrintForAnalytics<Encounter>("Encounter-home-pretty");
  TestPrettyPrintForAnalytics<Composition>("Composition-example");
  TestPrettyPrintForAnalytics<Observation>("Observation-example-genetics-1");
  TestPrettyPrintForAnalytics<Patient>("Patient-example");
}

/** Test printing from a profile */
TEST(JsonFormatR4Test, PrintProfileForAnalytics) {
  TestPrintForAnalyticsWithFilepath<r4::testing::TestPatient>(
      "testdata/r4/profiles/test_patient-profiled-testpatient.prototxt",
      "testdata/r4/bigquery/TestPatient.json");
}

TEST(JsonFormatR4Test, PrintAnalyticsElementIdsDropped) {
  TestPrintForAnalyticsWithFilepath<Location>(
      "testdata/jsonformat/location_element_with_ids.prototxt",
      "testdata/jsonformat/location_element_with_ids_analytic.json");
}

TEST(JsonFormatR4Test, PrintForAnalyticsWithContained) {
  r4::testing::TestPatient patient = ReadProto<r4::testing::TestPatient>(
      "testdata/r4/profiles/test_patient-profiled-testpatient.prototxt");
  r4::testing::TestObservation observation =
      ReadProto<r4::testing::TestObservation>(
          "examples/Observation-example-genetics-1.prototxt");

  r4::testing::ContainedResource contained;
  *contained.mutable_test_observation() = observation;

  patient.add_contained()->PackFrom(contained);

  auto result = PrettyPrintFhirToJsonStringForAnalytics(patient);
  ASSERT_TRUE(result.ok())
      << "Failed PrintForAnalytics on Patient with Contained\n"
      << result.status();

  internal::FhirJson from_proto, from_json;
  ASSERT_TRUE(internal::ParseJsonValue(result.value(), from_proto).ok());
  ASSERT_TRUE(
      internal::ParseJsonValue(
          ReadFile("testdata/r4/examples/with_contained.json"), from_json)
          .ok());
  EXPECT_THAT(from_proto, JsonEq(&from_json));
}

TEST(JsonFormatR4Test, WithEmptyContainedResourcePrintsValidJson) {
  const Parameters proto = ReadProto<Parameters>(
      "testdata/r4/examples/Parameters-empty-resource.prototxt");
  absl::StatusOr<std::string> from_proto_status =
      PrettyPrintFhirToJsonString(proto);
  ASSERT_TRUE(from_proto_status.ok());
  std::string expected =
      ReadFile("testdata/r4/examples/Parameters-empty-resource.json");
  ASSERT_THAT(expected.back(), Eq('\n'));
  expected = expected.substr(0, expected.length() - 1);
  ASSERT_THAT(*from_proto_status, Eq(expected));
}

/**
 * Tests parsing/printing for a resource that has a primitive extension nested
 * choice field.
 */
TEST(JsonFormatR4Test, WithPrimitiveExtensionNestedChoiceTypeField) {
  std::vector<std::string> files{"CodeSystem-v2-0003", "CodeSystem-v2-0061"};
  TestPair<CodeSystem>(files);
}

TEST(JsonFormatR4Test, TestAccount) {
  std::vector<std::string> files{"Account-ewg", "Account-example"};
  TestPair<Account>(files);
}

TEST(JsonFormatR4Test, TestActivityDefinition) {
  std::vector<std::string> files{
      "ActivityDefinition-administer-zika-virus-exposure-assessment",
      "ActivityDefinition-blood-tubes-supply",
      "ActivityDefinition-citalopramPrescription",
      "ActivityDefinition-heart-valve-replacement",
      "ActivityDefinition-provide-mosquito-prevention-advice",
      "ActivityDefinition-referralPrimaryCareMentalHealth-initial",
      "ActivityDefinition-referralPrimaryCareMentalHealth",
      "ActivityDefinition-serum-dengue-virus-igm",
      "ActivityDefinition-serum-zika-dengue-virus-igm"};
  TestPair<ActivityDefinition>(files);
}

TEST(JsonFormatR4Test, TestAdverseEvent) {
  std::vector<std::string> files{"AdverseEvent-example"};
  TestPair<AdverseEvent>(files);
}

TEST(JsonFormatR4Test, TestAllergyIntolerance) {
  std::vector<std::string> files{
      "AllergyIntolerance-example",    "AllergyIntolerance-fishallergy",
      "AllergyIntolerance-medication", "AllergyIntolerance-nka",
      "AllergyIntolerance-nkda",       "AllergyIntolerance-nkla"};
  TestPair<AllergyIntolerance>(files);
}

TEST(JsonFormatR4Test, TestAppointment) {
  std::vector<std::string> files{"Appointment-2docs", "Appointment-example",
                                 "Appointment-examplereq"};
  TestPair<Appointment>(files);
}

TEST(JsonFormatR4Test, TestAppointmentResponse) {
  std::vector<std::string> files{"AppointmentResponse-example",
                                 "AppointmentResponse-exampleresp"};
  TestPair<AppointmentResponse>(files);
}

TEST(JsonFormatR4Test, TestAuditEvent) {
  std::vector<std::string> files{"AuditEvent-example-disclosure",
                                 "AuditEvent-example-error",
                                 "AuditEvent-example",
                                 "AuditEvent-example-login",
                                 "AuditEvent-example-logout",
                                 "AuditEvent-example-media",
                                 "AuditEvent-example-pixQuery",
                                 "AuditEvent-example-rest",
                                 "AuditEvent-example-search"};
  TestPair<AuditEvent>(files);
}

TEST(JsonFormatR4Test, TestBasic) {
  std::vector<std::string> files{"Basic-basic-example-narrative",
                                 "Basic-classModel", "Basic-referral"};
  TestPair<Basic>(files);
}

TEST(JsonFormatR4Test, TestBinary) {
  std::vector<std::string> files{"Binary-example", "Binary-f006"};
  TestPair<Binary>(files);
}

TEST(JsonFormatR4Test, TestBiologicallyDerivedProduct) {
  std::vector<std::string> files{"BiologicallyDerivedProduct-example"};
  TestPair<BiologicallyDerivedProduct>(files);
}

TEST(JsonFormatR4Test, TestBodyStructure) {
  std::vector<std::string> files{
      "BodyStructure-fetus", "BodyStructure-skin-patch", "BodyStructure-tumor"};
  TestPair<BodyStructure>(files);
}

// Split bundles into several tests to enable better sharding.
TEST(JsonFormatR4Test, TestBundlePt1) {
  std::vector<std::string> files{
      "Bundle-101",
      "Bundle-10bb101f-a121-4264-a920-67be9cb82c74",
      "Bundle-3a0707d3-549e-4467-b8b8-5a2ab3800efe",
      "Bundle-3ad0687e-f477-468c-afd5-fcc2bf897808",
      "Bundle-3ad0687e-f477-468c-afd5-fcc2bf897809",
      "Bundle-3ad0687e-f477-468c-afd5-fcc2bf897819",
      "Bundle-72ac8493-52ac-41bd-8d5d-7258c289b5ea",
      "Bundle-b0a5e4277-83c4-4adb-87e2-e3efe3369b6f",
      "Bundle-b248b1b2-1686-4b94-9936-37d7a5f94b51",
  };
  TestPair<Bundle>(files);
}

TEST(JsonFormatR4Test, TestBundlePt2) {
  std::vector<std::string> files{
      "Bundle-bundle-example",
      "Bundle-bundle-references",
      "Bundle-bundle-request-medsallergies",
      "Bundle-bundle-request-simplesummary",
      "Bundle-bundle-response",
      "Bundle-bundle-response-medsallergies",
      "Bundle-bundle-response-simplesummary",
      "Bundle-bundle-search-warning",
      "Bundle-bundle-transaction",
  };
  TestPair<Bundle>(files);
}

TEST(JsonFormatR4Test, TestBundlePt4) {
  std::vector<std::string> files{"Bundle-conceptmaps", "Bundle-dataelements"};
  TestPair<Bundle>(files);
}

TEST(JsonFormatR4Test, TestBundlePt3) {
  std::vector<std::string> files{"Bundle-dg2",
                                 "Bundle-extensions",
                                 "Bundle-externals",
                                 "Bundle-f001",
                                 "Bundle-f202",
                                 "Bundle-father",
                                 "Bundle-ghp",
                                 "Bundle-hla-1",
                                 "Bundle-lipids",
                                 "Bundle-lri-example",
                                 "Bundle-micro",
                                 "Bundle-profiles-others",
                                 "Bundle-registry",
                                 "Bundle-report",
                                 "Bundle-searchParams",
                                 "Bundle-terminologies",
                                 "Bundle-types",
                                 "Bundle-ussg-fht",
                                 "Bundle-valueset-expansions",
                                 "Bundle-xds"};
  TestPair<Bundle>(files);
}

TEST(JsonFormatR4Test, TestResourcesBundle) {
  std::vector<std::string> files{
      "Bundle-resources",
  };
  TestPair<Bundle>(files);
}

// TEST(JsonFormatR4Test, TestValuesetsBundle) {
//   std::vector<std::string> files{
//       "Bundle-valuesets",
//   };
//   TestPair<Bundle>(files);
// }

// TEST(JsonFormatR4Test, TestV2ValuesetsBundle) {
//   std::vector<std::string> files{
//       "Bundle-v2-valuesets",
//   };
//   TestPair<Bundle>(files);
// }
//
// TEST(JsonFormatR4Test, TestV3ValuesetsBundle) {
//   std::vector<std::string> files{
//       "Bundle-v3-valuesets",
//   };
//   TestPair<Bundle>(files);
// }

TEST(JsonFormatR4Test, TestCapabilityStatement) {
  std::vector<std::string> files{"CapabilityStatement-base2",
                                 "CapabilityStatement-base",
                                 "CapabilityStatement-example",
                                 "CapabilityStatement-knowledge-repository",
                                 "CapabilityStatement-measure-processor",
                                 "CapabilityStatement-messagedefinition",
                                 "CapabilityStatement-phr",
                                 "CapabilityStatement-terminology-server"};
  TestPair<CapabilityStatement>(files);
}

TEST(JsonFormatR4Test, TestCarePlan) {
  std::vector<std::string> files{
      "CarePlan-example",   "CarePlan-f001",
      "CarePlan-f002",      "CarePlan-f003",
      "CarePlan-f201",      "CarePlan-f202",
      "CarePlan-f203",      "CarePlan-gpvisit",
      "CarePlan-integrate", "CarePlan-obesity-narrative",
      "CarePlan-preg"};
  TestPair<CarePlan>(files);
}

TEST(JsonFormatR4Test, TestCareTeam) {
  std::vector<std::string> files{"CareTeam-example"};
  TestPair<CareTeam>(files);
}

TEST(JsonFormatR4Test, TestCatalogEntry) {
  std::vector<std::string> files{"CatalogEntry-example"};
  TestPair<CatalogEntry>(files);
}

TEST(JsonFormatR4Test, TestChargeItem) {
  std::vector<std::string> files{"ChargeItem-example"};
  TestPair<ChargeItem>(files);
}

TEST(JsonFormatR4Test, TestChargeItemDefinition) {
  std::vector<std::string> files{"ChargeItemDefinition-device",
                                 "ChargeItemDefinition-ebm"};
  TestPair<ChargeItemDefinition>(files);
}

TEST(JsonFormatR4Test, TestClaim) {
  std::vector<std::string> files{
      "Claim-100150",   "Claim-100151", "Claim-100152", "Claim-100153",
      "Claim-100154",   "Claim-100155", "Claim-100156", "Claim-660150",
      "Claim-660151",   "Claim-660152", "Claim-760150", "Claim-760151",
      "Claim-760152",   "Claim-860150", "Claim-960150", "Claim-960151",
      "Claim-MED-00050"};
  TestPair<Claim>(files);
}

TEST(JsonFormatR4Test, TestClaimResponse) {
  std::vector<std::string> files{"ClaimResponse-R3500", "ClaimResponse-R3501",
                                 "ClaimResponse-R3502", "ClaimResponse-R3503",
                                 "ClaimResponse-UR3503"};
  TestPair<ClaimResponse>(files);
}

TEST(JsonFormatR4Test, TestClinicalImpression) {
  std::vector<std::string> files{"ClinicalImpression-example"};
  TestPair<ClinicalImpression>(files);
}

TEST(JsonFormatR4Test, TestCodeSystem) {
  std::vector<std::string> files{"CodeSystem-example",
                                 "CodeSystem-list-example-codes"};
  TestPair<CodeSystem>(files);
}

TEST(JsonFormatR4Test, TestCommunication) {
  std::vector<std::string> files{"Communication-example",
                                 "Communication-fm-attachment",
                                 "Communication-fm-solicited"};
  TestPair<Communication>(files);
}

TEST(JsonFormatR4Test, TestCommunicationRequest) {
  std::vector<std::string> files{"CommunicationRequest-example",
                                 "CommunicationRequest-fm-solicit"};
  TestPair<CommunicationRequest>(files);
}

TEST(JsonFormatR4Test, TestCompartmentDefinition) {
  std::vector<std::string> files{"CompartmentDefinition-device",
                                 "CompartmentDefinition-encounter",
                                 "CompartmentDefinition-example",
                                 "CompartmentDefinition-patient",
                                 "CompartmentDefinition-practitioner",
                                 "CompartmentDefinition-relatedPerson"};
  TestPair<CompartmentDefinition>(files);
}

TEST(JsonFormatR4Test, TestComposition) {
  std::vector<std::string> files{"Composition-example",
                                 "Composition-example-mixed"};
  TestPair<Composition>(files);
}

TEST(JsonFormatR4Test, TestCondition) {
  std::vector<std::string> files{
      "Condition-example2", "Condition-example",        "Condition-f001",
      "Condition-f002",     "Condition-f003",           "Condition-f201",
      "Condition-f202",     "Condition-f203",           "Condition-f204",
      "Condition-f205",     "Condition-family-history", "Condition-stroke"};
  TestPair<Condition>(files);
}

TEST(JsonFormatR4Test, TestConsent) {
  std::vector<std::string> files{"Consent-consent-example-basic",
                                 "Consent-consent-example-Emergency",
                                 "Consent-consent-example-grantor",
                                 "Consent-consent-example-notAuthor",
                                 "Consent-consent-example-notOrg",
                                 "Consent-consent-example-notThem",
                                 "Consent-consent-example-notThis",
                                 "Consent-consent-example-notTime",
                                 "Consent-consent-example-Out",
                                 "Consent-consent-example-pkb",
                                 "Consent-consent-example-signature",
                                 "Consent-consent-example-smartonfhir"};
  TestPair<Consent>(files);
}

TEST(JsonFormatR4Test, TestContract) {
  std::vector<std::string> files{"Contract-C-123",
                                 "Contract-C-2121",
                                 "Contract-INS-101",
                                 "Contract-pcd-example-notAuthor",
                                 "Contract-pcd-example-notLabs",
                                 "Contract-pcd-example-notOrg",
                                 "Contract-pcd-example-notThem",
                                 "Contract-pcd-example-notThis"};
  TestPair<Contract>(files);
}

TEST(JsonFormatR4Test, TestCoverage) {
  std::vector<std::string> files{"Coverage-7546D", "Coverage-7547E",
                                 "Coverage-9876B1", "Coverage-SP1234"};
  TestPair<Coverage>(files);
}

TEST(JsonFormatR4Test, TestCoverageEligibilityRequest) {
  std::vector<std::string> files{"CoverageEligibilityRequest-52345",
                                 "CoverageEligibilityRequest-52346"};
  TestPair<CoverageEligibilityRequest>(files);
}

TEST(JsonFormatR4Test, TestCoverageEligibilityResponse) {
  std::vector<std::string> files{
      "CoverageEligibilityResponse-E2500", "CoverageEligibilityResponse-E2501",
      "CoverageEligibilityResponse-E2502", "CoverageEligibilityResponse-E2503"};
  TestPair<CoverageEligibilityResponse>(files);
}

TEST(JsonFormatR4Test, TestDetectedIssue) {
  std::vector<std::string> files{"DetectedIssue-allergy", "DetectedIssue-ddi",
                                 "DetectedIssue-duplicate",
                                 "DetectedIssue-lab"};
  TestPair<DetectedIssue>(files);
}

TEST(JsonFormatR4Test, TestDevice) {
  std::vector<std::string> files{"Device-example", "Device-f001"};
  TestPair<Device>(files);
}

TEST(JsonFormatR4Test, TestDeviceDefinition) {
  std::vector<std::string> files{"DeviceDefinition-example"};
  TestPair<DeviceDefinition>(files);
}

TEST(JsonFormatR4Test, TestDeviceMetric) {
  std::vector<std::string> files{"DeviceMetric-example"};
  TestPair<DeviceMetric>(files);
}

TEST(JsonFormatR4Test, TestDeviceRequest) {
  std::vector<std::string> files{
      "DeviceRequest-example", "DeviceRequest-insulinpump",
      "DeviceRequest-left-lens", "DeviceRequest-right-lens"};
  TestPair<DeviceRequest>(files);
}

TEST(JsonFormatR4Test, TestDeviceUseStatement) {
  std::vector<std::string> files{"DeviceUseStatement-example"};
  TestPair<DeviceUseStatement>(files);
}

TEST(JsonFormatR4Test, TestDiagnosticReport) {
  std::vector<std::string> files{
      "DiagnosticReport-102",  "DiagnosticReport-example-pgx",
      "DiagnosticReport-f201", "DiagnosticReport-gingival-mass",
      "DiagnosticReport-pap",  "DiagnosticReport-ultrasound"};
  TestPair<DiagnosticReport>(files);
}

TEST(JsonFormatR4Test, TestDocumentManifest) {
  std::vector<std::string> files{"DocumentManifest-654789",
                                 "DocumentManifest-example"};
  TestPair<DocumentManifest>(files);
}

TEST(JsonFormatR4Test, TestDocumentReference) {
  std::vector<std::string> files{"DocumentReference-example"};
  TestPair<DocumentReference>(files);
}

TEST(JsonFormatR4Test, TestEffectEvidenceSynthesis) {
  std::vector<std::string> files{"EffectEvidenceSynthesis-example"};
  TestPair<EffectEvidenceSynthesis>(files);
}

TEST(JsonFormatR4Test, TestEncounter) {
  std::vector<std::string> files{"Encounter-emerg", "Encounter-example",
                                 "Encounter-f001",  "Encounter-f002",
                                 "Encounter-f003",  "Encounter-f201",
                                 "Encounter-f202",  "Encounter-f203",
                                 "Encounter-home",  "Encounter-xcda"};
  TestPair<Encounter>(files);
}

TEST(JsonFormatR4Test, TestEndpoint) {
  std::vector<std::string> files{"Endpoint-direct-endpoint",
                                 "Endpoint-example-iid", "Endpoint-example",
                                 "Endpoint-example-wadors"};
  TestPair<Endpoint>(files);
}

TEST(JsonFormatR4Test, TestEnrollmentRequest) {
  std::vector<std::string> files{"EnrollmentRequest-22345"};
  TestPair<EnrollmentRequest>(files);
}

TEST(JsonFormatR4Test, TestEnrollmentResponse) {
  std::vector<std::string> files{"EnrollmentResponse-ER2500"};
  TestPair<EnrollmentResponse>(files);
}

TEST(JsonFormatR4Test, TestEpisodeOfCare) {
  std::vector<std::string> files{"EpisodeOfCare-example"};
  TestPair<EpisodeOfCare>(files);
}

TEST(JsonFormatR4Test, TestEventDefinition) {
  std::vector<std::string> files{"EventDefinition-example"};
  TestPair<EventDefinition>(files);
}

TEST(JsonFormatR4Test, TestEvidence) {
  std::vector<std::string> files{"Evidence-example"};
  TestPair<Evidence>(files);
}

TEST(JsonFormatR4Test, TestEvidenceVariable) {
  std::vector<std::string> files{"EvidenceVariable-example"};
  TestPair<EvidenceVariable>(files);
}

TEST(JsonFormatR4Test, TestExampleScenario) {
  std::vector<std::string> files{"ExampleScenario-example"};
  TestPair<ExampleScenario>(files);
}

TEST(JsonFormatR4Test, TestExplanationOfBenefit) {
  std::vector<std::string> files{"ExplanationOfBenefit-EB3500",
                                 "ExplanationOfBenefit-EB3501"};
  TestPair<ExplanationOfBenefit>(files);
}

TEST(JsonFormatR4Test, TestFamilyMemberHistory) {
  std::vector<std::string> files{"FamilyMemberHistory-father",
                                 "FamilyMemberHistory-mother"};
  TestPair<FamilyMemberHistory>(files);
}

TEST(JsonFormatR4Test, TestFlag) {
  std::vector<std::string> files{"Flag-example-encounter", "Flag-example"};
  TestPair<Flag>(files);
}

TEST(JsonFormatR4Test, TestGoal) {
  std::vector<std::string> files{"Goal-example", "Goal-stop-smoking"};
  TestPair<Goal>(files);
}

TEST(JsonFormatR4Test, TestGraphDefinition) {
  std::vector<std::string> files{"GraphDefinition-example"};
  TestPair<GraphDefinition>(files);
}

TEST(JsonFormatR4Test, TestGroup) {
  std::vector<std::string> files{"Group-101", "Group-102",
                                 "Group-example-patientlist", "Group-herd1"};
  TestPair<Group>(files);
}

TEST(JsonFormatR4Test, TestGuidanceResponse) {
  std::vector<std::string> files{"GuidanceResponse-example"};
  TestPair<GuidanceResponse>(files);
}

TEST(JsonFormatR4Test, TestHealthcareService) {
  std::vector<std::string> files{"HealthcareService-example"};
  TestPair<HealthcareService>(files);
}

TEST(JsonFormatR4Test, TestImagingStudy) {
  std::vector<std::string> files{"ImagingStudy-example",
                                 "ImagingStudy-example-xr"};
  TestPair<ImagingStudy>(files);
}

TEST(JsonFormatR4Test, TestImmunization) {
  std::vector<std::string> files{
      "Immunization-example", "Immunization-historical",
      "Immunization-notGiven", "Immunization-protocol",
      "Immunization-subpotent"};
  TestPair<Immunization>(files);
}

TEST(JsonFormatR4Test, TestImmunizationEvaluation) {
  std::vector<std::string> files{"ImmunizationEvaluation-example",
                                 "ImmunizationEvaluation-notValid"};
  TestPair<ImmunizationEvaluation>(files);
}

TEST(JsonFormatR4Test, TestImmunizationRecommendation) {
  std::vector<std::string> files{"ImmunizationRecommendation-example"};
  TestPair<ImmunizationRecommendation>(files);
}

TEST(JsonFormatR4Test, TestImplementationGuide) {
  // "ImplementationGuide-fhir" and "ig-r4"do not parse because it contains a
  // reference to an invalid resource
  // https://gforge.hl7.org/gf/project/fhir/tracker/?action=TrackerItemEdit&tracker_item_id=22489
  std::vector<std::string> files{"ImplementationGuide-example"};
  TestPair<ImplementationGuide>(files);
}

TEST(JsonFormatR4Test, TestInsurancePlan) {
  std::vector<std::string> files{"InsurancePlan-example"};
  TestPair<InsurancePlan>(files);
}

TEST(JsonFormatR4Test, TestInvoice) {
  std::vector<std::string> files{"Invoice-example"};
  TestPair<Invoice>(files);
}

TEST(JsonFormatR4Test, TestLibrary) {
  std::vector<std::string> files{
      "Library-composition-example",
      "Library-example",
      "Library-hiv-indicators",
      "Library-library-cms146-example",
      "Library-library-exclusive-breastfeeding-cds-logic",
      "Library-library-exclusive-breastfeeding-cqm-logic",
      "Library-library-fhir-helpers",
      "Library-library-fhir-helpers-predecessor",
      "Library-library-fhir-model-definition",
      "Library-library-quick-model-definition",
      "Library-omtk-logic",
      "Library-omtk-modelinfo",
      "Library-opioidcds-common",
      "Library-opioidcds-recommendation-04",
      "Library-opioidcds-recommendation-05",
      "Library-opioidcds-recommendation-07",
      "Library-opioidcds-recommendation-08",
      "Library-opioidcds-recommendation-10",
      "Library-opioidcds-recommendation-11",
      "Library-suiciderisk-orderset-logic",
      "Library-zika-virus-intervention-logic"};
  TestPair<Library>(files);
}

TEST(JsonFormatR4Test, TestLinkage) {
  std::vector<std::string> files{"Linkage-example"};
  TestPair<Linkage>(files);
}

TEST(JsonFormatR4Test, TestList) {
  std::vector<std::string> files{"List-current-allergies",
                                 "List-example-double-cousin-relationship",
                                 "List-example-empty",
                                 "List-example",
                                 "List-example-simple-empty",
                                 "List-f201",
                                 "List-genetic",
                                 "List-long",
                                 "List-med-list",
                                 "List-prognosis"};
  TestPair<List>(files);
}

TEST(JsonFormatR4Test, TestLocation) {
  std::vector<std::string> files{"Location-1",   "Location-2",  "Location-amb",
                                 "Location-hl7", "Location-ph", "Location-ukp"};
  TestPair<Location>(files);
}

TEST(JsonFormatR4Test, TestMeasure) {
  std::vector<std::string> files{"Measure-component-a-example",
                                 "Measure-component-b-example",
                                 "Measure-composite-example",
                                 "Measure-hiv-indicators",
                                 "Measure-measure-cms146-example",
                                 "Measure-measure-exclusive-breastfeeding",
                                 "Measure-measure-predecessor-example"};
  TestPair<Measure>(files);
}

TEST(JsonFormatR4Test, TestMeasureReport) {
  std::vector<std::string> files{
      "MeasureReport-hiv-indicators",
      "MeasureReport-measurereport-cms146-cat1-example",
      "MeasureReport-measurereport-cms146-cat2-example",
      "MeasureReport-measurereport-cms146-cat3-example"};
  TestPair<MeasureReport>(files);
}

TEST(JsonFormatR4Test, TestMedia) {
  std::vector<std::string> files{
      "Media-1.2.840.11361907579238403408700.3.1.04.19970327150033",
      "Media-example", "Media-sound", "Media-xray"};
  TestPair<Media>(files);
}

TEST(JsonFormatR4Test, TestMedication) {
  std::vector<std::string> files{
      "Medication-med0301",           "Medication-med0302",
      "Medication-med0303",           "Medication-med0304",
      "Medication-med0305",           "Medication-med0306",
      "Medication-med0307",           "Medication-med0308",
      "Medication-med0309",           "Medication-med0310",
      "Medication-med0311",           "Medication-med0312",
      "Medication-med0313",           "Medication-med0314",
      "Medication-med0315",           "Medication-med0316",
      "Medication-med0317",           "Medication-med0318",
      "Medication-med0319",           "Medication-med0320",
      "Medication-med0321",           "Medication-medexample015",
      "Medication-medicationexample1"};
  TestPair<Medication>(files);
}

TEST(JsonFormatR4Test, TestMedicationAdministration) {
  std::vector<std::string> files{"MedicationAdministration-medadmin0301",
                                 "MedicationAdministration-medadmin0302",
                                 "MedicationAdministration-medadmin0303",
                                 "MedicationAdministration-medadmin0304",
                                 "MedicationAdministration-medadmin0305",
                                 "MedicationAdministration-medadmin0306",
                                 "MedicationAdministration-medadmin0307",
                                 "MedicationAdministration-medadmin0308",
                                 "MedicationAdministration-medadmin0309",
                                 "MedicationAdministration-medadmin0310",
                                 "MedicationAdministration-medadmin0311",
                                 "MedicationAdministration-medadmin0312",
                                 "MedicationAdministration-medadmin0313",
                                 "MedicationAdministration-medadminexample03"};
  TestPair<MedicationAdministration>(files);
}

TEST(JsonFormatR4Test, TestMedicationDispense) {
  std::vector<std::string> files{
      "MedicationDispense-meddisp008",  "MedicationDispense-meddisp0301",
      "MedicationDispense-meddisp0302", "MedicationDispense-meddisp0303",
      "MedicationDispense-meddisp0304", "MedicationDispense-meddisp0305",
      "MedicationDispense-meddisp0306", "MedicationDispense-meddisp0307",
      "MedicationDispense-meddisp0308", "MedicationDispense-meddisp0309",
      "MedicationDispense-meddisp0310", "MedicationDispense-meddisp0311",
      "MedicationDispense-meddisp0312", "MedicationDispense-meddisp0313",
      "MedicationDispense-meddisp0314", "MedicationDispense-meddisp0315",
      "MedicationDispense-meddisp0316", "MedicationDispense-meddisp0317",
      "MedicationDispense-meddisp0318", "MedicationDispense-meddisp0319",
      "MedicationDispense-meddisp0320", "MedicationDispense-meddisp0321",
      "MedicationDispense-meddisp0322", "MedicationDispense-meddisp0324",
      "MedicationDispense-meddisp0325", "MedicationDispense-meddisp0326",
      "MedicationDispense-meddisp0327", "MedicationDispense-meddisp0328",
      "MedicationDispense-meddisp0329", "MedicationDispense-meddisp0330",
      "MedicationDispense-meddisp0331"};
  TestPair<MedicationDispense>(files);
}

TEST(JsonFormatR4Test, TestMedicationKnowledge) {
  std::vector<std::string> files{"MedicationKnowledge-example"};
  TestPair<MedicationKnowledge>(files);
}

TEST(JsonFormatR4Test, TestMedicationRequest) {
  std::vector<std::string> files{
      "MedicationRequest-medrx002",  "MedicationRequest-medrx0301",
      "MedicationRequest-medrx0302", "MedicationRequest-medrx0303",
      "MedicationRequest-medrx0304", "MedicationRequest-medrx0305",
      "MedicationRequest-medrx0306", "MedicationRequest-medrx0307",
      "MedicationRequest-medrx0308", "MedicationRequest-medrx0309",
      "MedicationRequest-medrx0310", "MedicationRequest-medrx0311",
      "MedicationRequest-medrx0312", "MedicationRequest-medrx0313",
      "MedicationRequest-medrx0314", "MedicationRequest-medrx0315",
      "MedicationRequest-medrx0316", "MedicationRequest-medrx0317",
      "MedicationRequest-medrx0318", "MedicationRequest-medrx0319",
      "MedicationRequest-medrx0320", "MedicationRequest-medrx0321",
      "MedicationRequest-medrx0322", "MedicationRequest-medrx0323",
      "MedicationRequest-medrx0324", "MedicationRequest-medrx0325",
      "MedicationRequest-medrx0326", "MedicationRequest-medrx0327",
      "MedicationRequest-medrx0328", "MedicationRequest-medrx0329",
      "MedicationRequest-medrx0330", "MedicationRequest-medrx0331",
      "MedicationRequest-medrx0332", "MedicationRequest-medrx0333",
      "MedicationRequest-medrx0334", "MedicationRequest-medrx0335",
      "MedicationRequest-medrx0336", "MedicationRequest-medrx0337",
      "MedicationRequest-medrx0338", "MedicationRequest-medrx0339"};
  TestPair<MedicationRequest>(files);
}

TEST(JsonFormatR4Test, TestMedicationStatement) {
  std::vector<std::string> files{
      "MedicationStatement-example001", "MedicationStatement-example002",
      "MedicationStatement-example003", "MedicationStatement-example004",
      "MedicationStatement-example005", "MedicationStatement-example006",
      "MedicationStatement-example007"};
  TestPair<MedicationStatement>(files);
}

TEST(JsonFormatR4Test, TestMedicinalProduct) {
  std::vector<std::string> files{"MedicinalProduct-example"};
  TestPair<MedicinalProduct>(files);
}

TEST(JsonFormatR4Test, TestMedicinalProductAuthorization) {
  std::vector<std::string> files{"MedicinalProductAuthorization-example"};
  TestPair<MedicinalProductAuthorization>(files);
}

TEST(JsonFormatR4Test, TestMedicinalProductContraindication) {
  std::vector<std::string> files{"MedicinalProductContraindication-example"};
  TestPair<MedicinalProductContraindication>(files);
}

TEST(JsonFormatR4Test, TestMedicinalProductIndication) {
  std::vector<std::string> files{"MedicinalProductIndication-example"};
  TestPair<MedicinalProductIndication>(files);
}

TEST(JsonFormatR4Test, TestMedicinalProductIngredient) {
  std::vector<std::string> files{"MedicinalProductIngredient-example"};
  TestPair<MedicinalProductIngredient>(files);
}

TEST(JsonFormatR4Test, TestMedicinalProductInteraction) {
  std::vector<std::string> files{"MedicinalProductInteraction-example"};
  TestPair<MedicinalProductInteraction>(files);
}

TEST(JsonFormatR4Test, TestMedicinalProductManufactured) {
  std::vector<std::string> files{"MedicinalProductManufactured-example"};
  TestPair<MedicinalProductManufactured>(files);
}

TEST(JsonFormatR4Test, TestMedicinalProductPackaged) {
  std::vector<std::string> files{"MedicinalProductPackaged-example"};
  TestPair<MedicinalProductPackaged>(files);
}

TEST(JsonFormatR4Test, TestMedicinalProductPharmaceutical) {
  std::vector<std::string> files{"MedicinalProductPharmaceutical-example"};
  TestPair<MedicinalProductPharmaceutical>(files);
}

TEST(JsonFormatR4Test, TestMedicinalProductUndesirableEffect) {
  std::vector<std::string> files{"MedicinalProductUndesirableEffect-example"};
  TestPair<MedicinalProductUndesirableEffect>(files);
}

TEST(JsonFormatR4Test, TestMessageDefinition) {
  std::vector<std::string> files{"MessageDefinition-example",
                                 "MessageDefinition-patient-link-notification",
                                 "MessageDefinition-patient-link-response"};
  TestPair<MessageDefinition>(files);
}

TEST(JsonFormatR4Test, TestMessageHeader) {
  std::vector<std::string> files{
      "MessageHeader-1cbdfb97-5859-48a4-8301-d54eab818d68"};
  TestPair<MessageHeader>(files);
}

TEST(JsonFormatR4Test, TestMolecularSequence) {
  std::vector<std::string> files{"MolecularSequence-breastcancer",
                                 "MolecularSequence-coord-0-base",
                                 "MolecularSequence-coord-1-base",
                                 "MolecularSequence-example",
                                 "MolecularSequence-example-pgx-1",
                                 "MolecularSequence-example-pgx-2",
                                 "MolecularSequence-example-TPMT-one",
                                 "MolecularSequence-example-TPMT-two",
                                 "MolecularSequence-fda-example",
                                 "MolecularSequence-fda-vcf-comparison",
                                 "MolecularSequence-fda-vcfeval-comparison",
                                 "MolecularSequence-graphic-example-1",
                                 "MolecularSequence-graphic-example-2",
                                 "MolecularSequence-graphic-example-3",
                                 "MolecularSequence-graphic-example-4",
                                 "MolecularSequence-graphic-example-5",
                                 "MolecularSequence-sequence-complex-variant"};
  TestPair<MolecularSequence>(files);
}

TEST(JsonFormatR4Test, TestNamingSystem) {
  std::vector<std::string> files{"NamingSystem-example-id",
                                 "NamingSystem-example"};
  TestPair<NamingSystem>(files);
}

TEST(JsonFormatR4Test, TestNutritionOrder) {
  std::vector<std::string> files{
      "NutritionOrder-cardiacdiet",         "NutritionOrder-diabeticdiet",
      "NutritionOrder-diabeticsupplement",  "NutritionOrder-energysupplement",
      "NutritionOrder-enteralbolus",        "NutritionOrder-enteralcontinuous",
      "NutritionOrder-fiberrestricteddiet", "NutritionOrder-infantenteral",
      "NutritionOrder-proteinsupplement",   "NutritionOrder-pureeddiet",
      "NutritionOrder-pureeddiet-simple",   "NutritionOrder-renaldiet",
      "NutritionOrder-texturemodified"};
  TestPair<NutritionOrder>(files);
}

TEST(JsonFormatR4Test, TestObservation) {
  std::vector<std::string> files{"Observation-10minute-apgar-score",
                                 "Observation-1minute-apgar-score",
                                 "Observation-20minute-apgar-score",
                                 "Observation-2minute-apgar-score",
                                 "Observation-5minute-apgar-score",
                                 "Observation-656",
                                 "Observation-abdo-tender",
                                 "Observation-alcohol-type",
                                 "Observation-bgpanel",
                                 "Observation-bloodgroup",
                                 "Observation-blood-pressure-cancel",
                                 "Observation-blood-pressure-dar",
                                 "Observation-blood-pressure",
                                 "Observation-bmd",
                                 "Observation-bmi",
                                 "Observation-bmi-using-related",
                                 "Observation-body-height",
                                 "Observation-body-length",
                                 "Observation-body-temperature",
                                 "Observation-clinical-gender",
                                 "Observation-date-lastmp",
                                 "Observation-decimal",
                                 "Observation-ekg",
                                 "Observation-example-diplotype1",
                                 "Observation-example-genetics-1",
                                 "Observation-example-genetics-2",
                                 "Observation-example-genetics-3",
                                 "Observation-example-genetics-4",
                                 "Observation-example-genetics-5",
                                 "Observation-example-genetics-brcapat",
                                 "Observation-example-haplotype1",
                                 "Observation-example-haplotype2",
                                 "Observation-example",
                                 "Observation-example-phenotype",
                                 "Observation-example-TPMT-diplotype",
                                 "Observation-example-TPMT-haplotype-one",
                                 "Observation-example-TPMT-haplotype-two",
                                 "Observation-eye-color",
                                 "Observation-f001",
                                 "Observation-f002",
                                 "Observation-f003",
                                 "Observation-f004",
                                 "Observation-f005",
                                 "Observation-f202",
                                 "Observation-f203",
                                 "Observation-f204",
                                 "Observation-f205",
                                 "Observation-f206",
                                 "Observation-gcs-qa",
                                 "Observation-glasgow",
                                 "Observation-head-circumference",
                                 "Observation-heart-rate",
                                 "Observation-herd1",
                                 "Observation-map-sitting",
                                 "Observation-mbp",
                                 "Observation-respiratory-rate",
                                 "Observation-rhstatus",
                                 "Observation-satO2",
                                 "Observation-secondsmoke",
                                 "Observation-trachcare",
                                 "Observation-unsat",
                                 "Observation-vitals-panel",
                                 "Observation-vomiting",
                                 "Observation-vp-oyster"};
  TestPair<Observation>(files);
}

TEST(JsonFormatR4Test, TestObservationDefinition) {
  std::vector<std::string> files{"ObservationDefinition-example"};
  TestPair<ObservationDefinition>(files);
}

TEST(JsonFormatR4Test, TestOperationDefinition) {
  std::vector<std::string> files{
      "OperationDefinition-ActivityDefinition-apply",
      "OperationDefinition-ActivityDefinition-data-requirements",
      "OperationDefinition-CapabilityStatement-conforms",
      "OperationDefinition-CapabilityStatement-implements",
      "OperationDefinition-CapabilityStatement-subset",
      "OperationDefinition-CapabilityStatement-versions",
      "OperationDefinition-ChargeItemDefinition-apply",
      "OperationDefinition-Claim-submit",
      "OperationDefinition-CodeSystem-find-matches",
      "OperationDefinition-CodeSystem-lookup",
      "OperationDefinition-CodeSystem-subsumes",
      "OperationDefinition-CodeSystem-validate-code",
      "OperationDefinition-Composition-document",
      "OperationDefinition-ConceptMap-closure",
      "OperationDefinition-ConceptMap-translate",
      "OperationDefinition-CoverageEligibilityRequest-submit",
      "OperationDefinition-Encounter-everything",
      "OperationDefinition-example",
      "OperationDefinition-Group-everything",
      "OperationDefinition-Library-data-requirements",
      "OperationDefinition-List-find",
      "OperationDefinition-Measure-care-gaps",
      "OperationDefinition-Measure-collect-data",
      "OperationDefinition-Measure-data-requirements",
      "OperationDefinition-Measure-evaluate-measure",
      "OperationDefinition-Measure-submit-data",
      "OperationDefinition-MedicinalProduct-everything",
      "OperationDefinition-MessageHeader-process-message",
      "OperationDefinition-NamingSystem-preferred-id",
      "OperationDefinition-Observation-lastn",
      "OperationDefinition-Observation-stats",
      "OperationDefinition-Patient-everything",
      "OperationDefinition-Patient-match",
      "OperationDefinition-PlanDefinition-apply",
      "OperationDefinition-PlanDefinition-data-requirements",
      "OperationDefinition-Resource-convert",
      "OperationDefinition-Resource-graph",
      "OperationDefinition-Resource-graphql",
      "OperationDefinition-Resource-meta-add",
      "OperationDefinition-Resource-meta-delete",
      "OperationDefinition-Resource-meta",
      "OperationDefinition-Resource-validate",
      "OperationDefinition-StructureDefinition-questionnaire",
      "OperationDefinition-StructureDefinition-snapshot",
      "OperationDefinition-StructureMap-transform",
      "OperationDefinition-ValueSet-expand",
      "OperationDefinition-ValueSet-validate-code"};
  TestPair<OperationDefinition>(files);
}

TEST(JsonFormatR4Test, TestOperationOutcome) {
  std::vector<std::string> files{"OperationOutcome-101",
                                 "OperationOutcome-allok",
                                 "OperationOutcome-break-the-glass",
                                 "OperationOutcome-exception",
                                 "OperationOutcome-searchfail",
                                 "OperationOutcome-validationfail"};
  TestPair<OperationOutcome>(files);
}

TEST(JsonFormatR4Test, TestOrganization) {
  std::vector<std::string> files{
      "Organization-1832473e-2fe0-452d-abe9-3cdb9879522f",
      "Organization-1",
      "Organization-2.16.840.1.113883.19.5",
      "Organization-2",
      "Organization-3",
      "Organization-f001",
      "Organization-f002",
      "Organization-f003",
      "Organization-f201",
      "Organization-f203",
      "Organization-hl7",
      "Organization-hl7pay",
      "Organization-mmanu"};
  TestPair<Organization>(files);
}

TEST(JsonFormatR4Test, TestOrganizationAffiliation) {
  std::vector<std::string> files{"OrganizationAffiliation-example",
                                 "OrganizationAffiliation-orgrole1",
                                 "OrganizationAffiliation-orgrole2"};
  TestPair<OrganizationAffiliation>(files);
}

TEST(JsonFormatR4Test, TestPatient) {
  std::vector<std::string> files{"Patient-animal",
                                 "Patient-ch-example",
                                 "Patient-dicom",
                                 "Patient-example",
                                 "Patient-f001",
                                 "Patient-f201",
                                 "Patient-genetics-example1",
                                 "Patient-glossy",
                                 "Patient-ihe-pcd",
                                 "Patient-infant-fetal",
                                 "Patient-infant-mom",
                                 "Patient-infant-twin-1",
                                 "Patient-infant-twin-2",
                                 "Patient-mom",
                                 "Patient-newborn",
                                 "Patient-pat1",
                                 "Patient-pat2",
                                 "Patient-pat3",
                                 "Patient-pat4",
                                 "Patient-proband",
                                 "Patient-xcda",
                                 "Patient-xds"};
  TestPair<Patient>(files);
}

TEST(JsonFormatR4Test, TestPaymentNotice) {
  std::vector<std::string> files{"PaymentNotice-77654"};
  TestPair<PaymentNotice>(files);
}

TEST(JsonFormatR4Test, TestPaymentReconciliation) {
  std::vector<std::string> files{"PaymentReconciliation-ER2500"};
  TestPair<PaymentReconciliation>(files);
}

TEST(JsonFormatR4Test, TestPerson) {
  std::vector<std::string> files{"Person-example", "Person-f002",
                                 "Person-grahame", "Person-pd", "Person-pp"};
  TestPair<Person>(files);
}

TEST(JsonFormatR4Test, TestPlanDefinition) {
  std::vector<std::string> files{
      "PlanDefinition-chlamydia-screening-intervention",
      "PlanDefinition-example-cardiology-os",
      "PlanDefinition-exclusive-breastfeeding-intervention-01",
      "PlanDefinition-exclusive-breastfeeding-intervention-02",
      "PlanDefinition-exclusive-breastfeeding-intervention-03",
      "PlanDefinition-exclusive-breastfeeding-intervention-04",
      "PlanDefinition-KDN5",
      "PlanDefinition-low-suicide-risk-order-set",
      "PlanDefinition-opioidcds-04",
      "PlanDefinition-opioidcds-05",
      "PlanDefinition-opioidcds-07",
      "PlanDefinition-opioidcds-08",
      "PlanDefinition-opioidcds-10",
      "PlanDefinition-opioidcds-11",
      "PlanDefinition-options-example",
      "PlanDefinition-protocol-example",
      "PlanDefinition-zika-virus-intervention-initial",
      "PlanDefinition-zika-virus-intervention"};
  TestPair<PlanDefinition>(files);
}

TEST(JsonFormatR4Test, TestPractitioner) {
  std::vector<std::string> files{
      "Practitioner-example", "Practitioner-f001",       "Practitioner-f002",
      "Practitioner-f003",    "Practitioner-f004",       "Practitioner-f005",
      "Practitioner-f006",    "Practitioner-f007",       "Practitioner-f201",
      "Practitioner-f202",    "Practitioner-f203",       "Practitioner-f204",
      "Practitioner-xcda1",   "Practitioner-xcda-author"};
  TestPair<Practitioner>(files);
}

TEST(JsonFormatR4Test, TestPractitionerRole) {
  std::vector<std::string> files{"PractitionerRole-example"};
  TestPair<PractitionerRole>(files);
}

TEST(JsonFormatR4Test, TestProcedure) {
  std::vector<std::string> files{"Procedure-ambulation",
                                 "Procedure-appendectomy-narrative",
                                 "Procedure-biopsy",
                                 "Procedure-colon-biopsy",
                                 "Procedure-colonoscopy",
                                 "Procedure-education",
                                 "Procedure-example-implant",
                                 "Procedure-example",
                                 "Procedure-f001",
                                 "Procedure-f002",
                                 "Procedure-f003",
                                 "Procedure-f004",
                                 "Procedure-f201",
                                 "Procedure-HCBS",
                                 "Procedure-ob",
                                 "Procedure-physical-therapy"};
  TestPair<Procedure>(files);
}

TEST(JsonFormatR4Test, TestProvenance) {
  std::vector<std::string> files{
      "Provenance-consent-signature", "Provenance-example-biocompute-object",
      "Provenance-example-cwl", "Provenance-example", "Provenance-signature"};
  TestPair<Provenance>(files);
}

TEST(JsonFormatR4Test, TestQuestionnaire) {
  std::vector<std::string> files{
      "Questionnaire-3141",
      "Questionnaire-bb",
      "Questionnaire-f201",
      "Questionnaire-gcs",
      "Questionnaire-phq-9-questionnaire",
      "Questionnaire-qs1",
      "Questionnaire-zika-virus-exposure-assessment"};
  TestPair<Questionnaire>(files);
}

TEST(JsonFormatR4Test, TestQuestionnaireResponse) {
  std::vector<std::string> files{
      "QuestionnaireResponse-3141", "QuestionnaireResponse-bb",
      "QuestionnaireResponse-f201", "QuestionnaireResponse-gcs",
      "QuestionnaireResponse-ussg-fht-answers"};
  TestPair<QuestionnaireResponse>(files);
}

TEST(JsonFormatR4Test, TestRelatedPerson) {
  std::vector<std::string> files{
      "RelatedPerson-benedicte", "RelatedPerson-f001", "RelatedPerson-f002",
      "RelatedPerson-newborn-mom", "RelatedPerson-peter"};
  TestPair<RelatedPerson>(files);
}

TEST(JsonFormatR4Test, TestRequestGroup) {
  std::vector<std::string> files{"RequestGroup-example",
                                 "RequestGroup-kdn5-example"};
  TestPair<RequestGroup>(files);
}

TEST(JsonFormatR4Test, TestResearchDefinition) {
  std::vector<std::string> files{"ResearchDefinition-example"};
  TestPair<ResearchDefinition>(files);
}

TEST(JsonFormatR4Test, TestResearchElementDefinition) {
  std::vector<std::string> files{"ResearchElementDefinition-example"};
  TestPair<ResearchElementDefinition>(files);
}

TEST(JsonFormatR4Test, TestResearchStudy) {
  std::vector<std::string> files{"ResearchStudy-example"};
  TestPair<ResearchStudy>(files);
}

TEST(JsonFormatR4Test, TestResearchSubject) {
  std::vector<std::string> files{"ResearchSubject-example"};
  TestPair<ResearchSubject>(files);
}

TEST(JsonFormatR4Test, TestRiskAssessment) {
  std::vector<std::string> files{
      "RiskAssessment-breastcancer-risk", "RiskAssessment-cardiac",
      "RiskAssessment-genetic",           "RiskAssessment-population",
      "RiskAssessment-prognosis",         "RiskAssessment-riskexample"};
  TestPair<RiskAssessment>(files);
}

TEST(JsonFormatR4Test, TestRiskEvidenceSynthesis) {
  std::vector<std::string> files{"RiskEvidenceSynthesis-example"};
  TestPair<RiskEvidenceSynthesis>(files);
}

TEST(JsonFormatR4Test, TestSchedule) {
  std::vector<std::string> files{"Schedule-example", "Schedule-exampleloc1",
                                 "Schedule-exampleloc2"};
  TestPair<Schedule>(files);
}

TEST(JsonFormatR4Test, TestServiceRequest) {
  std::vector<std::string> files{"ServiceRequest-ambulation",
                                 "ServiceRequest-appendectomy-narrative",
                                 "ServiceRequest-benchpress",
                                 "ServiceRequest-colon-biopsy",
                                 "ServiceRequest-colonoscopy",
                                 "ServiceRequest-di",
                                 "ServiceRequest-do-not-turn",
                                 "ServiceRequest-education",
                                 "ServiceRequest-example-implant",
                                 "ServiceRequest-example",
                                 "ServiceRequest-example-pgx",
                                 "ServiceRequest-ft4",
                                 "ServiceRequest-lipid",
                                 "ServiceRequest-myringotomy",
                                 "ServiceRequest-ob",
                                 "ServiceRequest-og-example1",
                                 "ServiceRequest-physical-therapy",
                                 "ServiceRequest-physiotherapy",
                                 "ServiceRequest-subrequest",
                                 "ServiceRequest-vent"};
  TestPair<ServiceRequest>(files);
}

TEST(JsonFormatR4Test, TestSlot) {
  std::vector<std::string> files{"Slot-1", "Slot-2", "Slot-3", "Slot-example"};
  TestPair<Slot>(files);
}

TEST(JsonFormatR4Test, TestSpecimen) {
  std::vector<std::string> files{"Specimen-101", "Specimen-isolate",
                                 "Specimen-pooled-serum", "Specimen-sst",
                                 "Specimen-vma-urine"};
  TestPair<Specimen>(files);
}

TEST(JsonFormatR4Test, TestSpecimenDefinition) {
  std::vector<std::string> files{"SpecimenDefinition-2364"};
  TestPair<SpecimenDefinition>(files);
}

TEST(JsonFormatR4Test, TestStructureMap) {
  std::vector<std::string> files{"StructureMap-example",
                                 "StructureMap-supplyrequest-transform"};
  TestPair<StructureMap>(files);
}

TEST(JsonFormatR4Test, TestStructureDefinition) {
  std::vector<std::string> files{"StructureDefinition-Coding",
                                 "StructureDefinition-lipidprofile"};
  TestPair<StructureDefinition>(files);
}

TEST(JsonFormatR4Test, TestSubscription) {
  std::vector<std::string> files{"Subscription-example-error",
                                 "Subscription-example"};
  TestPair<Subscription>(files);
}

TEST(JsonFormatR4Test, TestSubstance) {
  std::vector<std::string> files{"Substance-example", "Substance-f201",
                                 "Substance-f202",    "Substance-f203",
                                 "Substance-f204",    "Substance-f205"};
  TestPair<Substance>(files);
}

TEST(JsonFormatR4Test, TestSubstanceSpecification) {
  std::vector<std::string> files{"SubstanceSpecification-example"};
  TestPair<SubstanceSpecification>(files);
}

TEST(JsonFormatR4Test, TestSupplyDelivery) {
  std::vector<std::string> files{"SupplyDelivery-pumpdelivery",
                                 "SupplyDelivery-simpledelivery"};
  TestPair<SupplyDelivery>(files);
}

TEST(JsonFormatR4Test, TestSupplyRequest) {
  std::vector<std::string> files{"SupplyRequest-simpleorder"};
  TestPair<SupplyRequest>(files);
}

TEST(JsonFormatR4Test, TestTask) {
  std::vector<std::string> files{
      "Task-example1",    "Task-example2",    "Task-example3",
      "Task-example4",    "Task-example5",    "Task-example6",
      "Task-fm-example1", "Task-fm-example2", "Task-fm-example3",
      "Task-fm-example4", "Task-fm-example5", "Task-fm-example6"};
  TestPair<Task>(files);
}

TEST(JsonFormatR4Test, TestTerminologyCapabilities) {
  std::vector<std::string> files{"TerminologyCapabilities-example"};
  TestPair<TerminologyCapabilities>(files);
}

TEST(JsonFormatR4Test, TestTestReport) {
  std::vector<std::string> files{"TestReport-testreport-example"};
  TestPair<TestReport>(files);
}

TEST(JsonFormatR4Test, TestTestScript) {
  std::vector<std::string> files{"TestScript-testscript-example-history",
                                 "TestScript-testscript-example",
                                 "TestScript-testscript-example-multisystem",
                                 "TestScript-testscript-example-readtest",
                                 "TestScript-testscript-example-search",
                                 "TestScript-testscript-example-update"};
  TestPair<TestScript>(files);
}

TEST(JsonFormatR4Test, TestVerificationResult) {
  std::vector<std::string> files{"VerificationResult-example"};
  TestPair<VerificationResult>(files);
}

TEST(JsonFormatR4Test, TestVisionPrescription) {
  std::vector<std::string> files{"VisionPrescription-33123",
                                 "VisionPrescription-33124"};
  TestPair<VisionPrescription>(files);
}

TEST(JsonFormatR4Test, PrintAndParseAllResources) {
  // Populate all fields to test edge cases, but recur only rarely to keep
  // the test fast.
  auto generator_params =
      google::fhir::testutil::RandomValueProvider::DefaultParams();
  generator_params.optional_set_probability = 1.0;
  generator_params.optional_set_ratio_per_level = 0.05;
  generator_params.max_string_length = 200;
  auto value_provider =
      absl::make_unique<google::fhir::testutil::RandomValueProvider>(
          generator_params);

  google::fhir::testutil::FhirGenerator generator(
      std::move(value_provider),
      google::fhir::r4::R4PrimitiveHandler::GetInstance());
  const google::protobuf::Descriptor* descriptor =
      google::fhir::r4::core::ContainedResource::GetDescriptor();

  // Test populating all resources by iterating through all oneof fields
  // in the contained resource.
  for (int i = 0; i < descriptor->field_count(); ++i) {
    const google::protobuf::FieldDescriptor* resource_field = descriptor->field(i);

    google::fhir::r4::core::ContainedResource container;
    google::protobuf::Message* resource =
        container.GetReflection()->MutableMessage(&container, resource_field);

    // Skip bundle since all resources are covered elsewhere.
    if (google::fhir::IsMessageType<google::fhir::r4::core::Bundle>(
            *resource)) {
      continue;
    }

    FHIR_ASSERT_OK(generator.Fill(resource));
    FHIR_ASSERT_OK_AND_ASSIGN(
        std::string json, ::google::fhir::r4::PrintFhirToJsonString(*resource));

    // Ensure JSON strings are parsed into the correct proto.
    google::fhir::r4::core::ContainedResource parsed_container;
    google::protobuf::Message* parsed_resource =
        parsed_container.GetReflection()->MutableMessage(&parsed_container,
                                                         resource_field);
    FHIR_ASSERT_OK(::google::fhir::r4::MergeJsonFhirStringIntoProto(
        json, parsed_resource, absl::UTCTimeZone(), false));
    EXPECT_THAT(*resource, EqualsProto(*parsed_resource));

    // Ensure pre-parsed JSON objects are parsed into the correct proto.
    google::fhir::r4::core::ContainedResource parsed_container_from_object;
    google::protobuf::Message* parsed_resource_from_object =
        parsed_container_from_object.GetReflection()->MutableMessage(
            &parsed_container_from_object, resource_field);
    internal::FhirJson parsed_json;
    FHIR_ASSERT_OK(internal::ParseJsonValue(json, parsed_json));
    FHIR_ASSERT_OK(::google::fhir::r4::MergeJsonFhirObjectIntoProto(
        parsed_json, parsed_resource_from_object, absl::UTCTimeZone(), false));
    EXPECT_THAT(*resource, EqualsProto(*parsed_resource_from_object));
  }
}

TEST(JsonFormatR4Test, InvalidControlCharactersReturnsError) {
  const Observation proto = ReadProto<Observation>(
      "testdata/jsonformat/observation_invalid_unicode.prototxt");

  ASSERT_FALSE(PrettyPrintFhirToJsonString(proto).ok());
}

TEST(JsonFormatR4Test, PadsThreeDigitYearToFourCharacters) {
  TestPairWithFilePaths<Observation>(
      "testdata/jsonformat/observation_three_digit_year.prototxt",
      "testdata/jsonformat/observation_three_digit_year.json");
}

TEST(JsonFormatR4Test, DecimalCornerCases) {
  TestPairWithFilePaths<Observation>(
      "testdata/jsonformat/observation_decimal_corner_cases.prototxt",
      "testdata/jsonformat/observation_decimal_corner_cases.json");
}

TEST(JsonFormatR4Test, ScientificNotationCornerCases) {
  TestPairWithFilePaths<Observation>(
      "testdata/jsonformat/"
      "observation_scientific_notation_corner_cases.prototxt",
      "testdata/jsonformat/observation_scientific_notation_corner_cases.json");
}

TEST(JsonFormatR4Test, NdjsonLocation) {
  TestPairWithFilePaths<Location>(
      "testdata/jsonformat/location_ndjson.prototxt",
      "testdata/jsonformat/location_ndjson.json");
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

TEST(JsonFormatR4Test, ParserErrorHandlerAggregatesErrorsAndFatalsParseFails) {
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
  absl::Status merge_status = ::google::fhir::r4::MergeJsonFhirStringIntoProto(
      raw_json, &resource, absl::LocalTimeZone(), true, handler);

  OperationOutcome expected_outcome;
  OperationOutcomeErrorHandler expected_outcome_handler(&expected_outcome);

  AddFatal(expected_outcome, "No field `squibalop` in CodeableConcept",
           "Observation.category[1]");
  AddFatal(expected_outcome,
           "Unparseable JSON string for google.fhir.r4.core.DateTime",
           "Observation.ofType(DateTime)");
  AddFatal(expected_outcome,
           "Failed to convert `finol` to "
           "google.fhir.r4.core.ObservationStatusCode.Value: No matching enum "
           "found.",
           "Observation.status");
  AddError(expected_outcome, "Invalid input for google.fhir.r4.core.Id",
           "Observation.id");
  AddError(expected_outcome,
           "Input failed regex requirement for: google.fhir.r4.core.Id",
           "Observation.id");
  AddError(expected_outcome, "Invalid input for google.fhir.r4.core.Uri",
           "Observation.referenceRange[0].appliesTo[0].coding[0].system");
  AddError(expected_outcome,
           "Input failed regex requirement for: google.fhir.r4.core.Uri",
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

TEST(JsonFormatR4Test,
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
  absl::Status merge_status = ::google::fhir::r4::MergeJsonFhirStringIntoProto(
      raw_json, &resource, absl::LocalTimeZone(), true, handler);

  OperationOutcome expected_outcome;

  AddError(expected_outcome, "Invalid input for google.fhir.r4.core.Id",
           "Observation.id");
  AddError(expected_outcome,
           "Input failed regex requirement for: google.fhir.r4.core.Id",
           "Observation.id");
  AddError(expected_outcome, "missing-required-field", "Observation.code");

  // Merge succeeds despite data issues.
  FHIR_ASSERT_OK(merge_status);

  EXPECT_THAT(outcome,
              IgnoringRepeatedFieldOrdering(EqualsProto(expected_outcome)));
}

}  // namespace

}  // namespace r4
}  // namespace fhir
}  // namespace google
