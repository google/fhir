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

#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/text_format.h"
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
#include "google/fhir/r4/operation_error_reporter.h"
#include "google/fhir/r4/primitive_handler.h"
#include "google/fhir/r4/profiles.h"
#include "google/fhir/r4/resource_validation.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
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

inline std::string ReadFile(absl::string_view filename) {
  std::ifstream infile;
  infile.open(
      absl::StrCat(getenv("TEST_SRCDIR"), "/com_google_fhir/", filename));

  std::ostringstream out;
  out << infile.rdbuf();
  return out.str();
}

template <class T>
T ReadProto(absl::string_view filename) {
  T result;
  CHECK(google::protobuf::TextFormat::ParseFromString(ReadFile(filename), &result))
      << "Failed to parse proto in file " << filename;
  return result;
}

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
  TestPrintForAnalytics<Observation>("Observation-example-2");
  TestPrintForAnalytics<Patient>("Patient-example");
}

TEST(JsonFormatR4Test, PrettyPrintForAnalytics) {
  TestPrettyPrintForAnalytics<Encounter>("Encounter-home-pretty");
  TestPrettyPrintForAnalytics<Composition>("Composition-example");
  TestPrettyPrintForAnalytics<Observation>("Observation-example-genetics-1");
  TestPrettyPrintForAnalytics<Observation>("Observation-example-2");
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

TEST(JsonFormatR4Test, PrintAndParseAllResources) {
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

TEST(JsonFormatR4Test, ParseRelativeReferenceSucceeds) {
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
  absl::Status merge_status = ::google::fhir::r4::MergeJsonFhirStringIntoProto(
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

TEST(JsonFormatR4Test, ParseAbsoluteReferenceSucceeds) {
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
  absl::Status merge_status = ::google::fhir::r4::MergeJsonFhirStringIntoProto(
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

TEST(JsonFormatR4Test, PrintRelativeReferenceSucceeds) {
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
      ::google::fhir::r4::PrettyPrintFhirToJsonString(proto_form));

  EXPECT_EQ(json, R"json({
  "resourceType": "Observation",
  "subject": {
    "reference": "Patient/example",
    "display": "Amy Shaw"
  }
})json");
}

TEST(JsonFormatR4Test, PrintAbsoluteReferenceSucceeds) {
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
      ::google::fhir::r4::PrettyPrintFhirToJsonString(proto_form));

  EXPECT_EQ(json, R"json({
  "resourceType": "Observation",
  "subject": {
    "reference": "www.patient.org/123",
    "display": "Amy Shaw"
  }
})json");
}

}  // namespace

}  // namespace r4
}  // namespace fhir
}  // namespace google
