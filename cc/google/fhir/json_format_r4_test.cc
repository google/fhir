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

#include <unordered_set>

#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/json_format.h"
#include "google/fhir/r4/profiles.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/annotations.pb.h"
#include "proto/r4/core/codes.pb.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/r4/core/profiles/observation_genetics.pb.h"
#include "proto/r4/core/resources/account.pb.h"
#include "proto/r4/core/resources/activity_definition.pb.h"
#include "proto/r4/core/resources/adverse_event.pb.h"
#include "proto/r4/core/resources/allergy_intolerance.pb.h"
#include "proto/r4/core/resources/appointment.pb.h"
#include "proto/r4/core/resources/appointment_response.pb.h"
#include "proto/r4/core/resources/audit_event.pb.h"
#include "proto/r4/core/resources/basic.pb.h"
#include "proto/r4/core/resources/binary.pb.h"
#include "proto/r4/core/resources/biologically_derived_product.pb.h"
#include "proto/r4/core/resources/body_structure.pb.h"
#include "proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/r4/core/resources/capability_statement.pb.h"
#include "proto/r4/core/resources/care_plan.pb.h"
#include "proto/r4/core/resources/care_team.pb.h"
#include "proto/r4/core/resources/catalog_entry.pb.h"
#include "proto/r4/core/resources/charge_item.pb.h"
#include "proto/r4/core/resources/charge_item_definition.pb.h"
#include "proto/r4/core/resources/claim.pb.h"
#include "proto/r4/core/resources/claim_response.pb.h"
#include "proto/r4/core/resources/clinical_impression.pb.h"
#include "proto/r4/core/resources/code_system.pb.h"
#include "proto/r4/core/resources/communication.pb.h"
#include "proto/r4/core/resources/communication_request.pb.h"
#include "proto/r4/core/resources/compartment_definition.pb.h"
#include "proto/r4/core/resources/composition.pb.h"
#include "proto/r4/core/resources/concept_map.pb.h"
#include "proto/r4/core/resources/condition.pb.h"
#include "proto/r4/core/resources/consent.pb.h"
#include "proto/r4/core/resources/contract.pb.h"
#include "proto/r4/core/resources/coverage.pb.h"
#include "proto/r4/core/resources/coverage_eligibility_request.pb.h"
#include "proto/r4/core/resources/coverage_eligibility_response.pb.h"
#include "proto/r4/core/resources/detected_issue.pb.h"
#include "proto/r4/core/resources/device.pb.h"
#include "proto/r4/core/resources/device_definition.pb.h"
#include "proto/r4/core/resources/device_metric.pb.h"
#include "proto/r4/core/resources/device_request.pb.h"
#include "proto/r4/core/resources/device_use_statement.pb.h"
#include "proto/r4/core/resources/diagnostic_report.pb.h"
#include "proto/r4/core/resources/document_manifest.pb.h"
#include "proto/r4/core/resources/document_reference.pb.h"
#include "proto/r4/core/resources/effect_evidence_synthesis.pb.h"
#include "proto/r4/core/resources/encounter.pb.h"
#include "proto/r4/core/resources/endpoint.pb.h"
#include "proto/r4/core/resources/enrollment_request.pb.h"
#include "proto/r4/core/resources/enrollment_response.pb.h"
#include "proto/r4/core/resources/episode_of_care.pb.h"
#include "proto/r4/core/resources/event_definition.pb.h"
#include "proto/r4/core/resources/evidence.pb.h"
#include "proto/r4/core/resources/evidence_variable.pb.h"
#include "proto/r4/core/resources/example_scenario.pb.h"
#include "proto/r4/core/resources/explanation_of_benefit.pb.h"
#include "proto/r4/core/resources/family_member_history.pb.h"
#include "proto/r4/core/resources/flag.pb.h"
#include "proto/r4/core/resources/goal.pb.h"
#include "proto/r4/core/resources/graph_definition.pb.h"
#include "proto/r4/core/resources/group.pb.h"
#include "proto/r4/core/resources/guidance_response.pb.h"
#include "proto/r4/core/resources/healthcare_service.pb.h"
#include "proto/r4/core/resources/imaging_study.pb.h"
#include "proto/r4/core/resources/immunization.pb.h"
#include "proto/r4/core/resources/immunization_evaluation.pb.h"
#include "proto/r4/core/resources/immunization_recommendation.pb.h"
#include "proto/r4/core/resources/implementation_guide.pb.h"
#include "proto/r4/core/resources/insurance_plan.pb.h"
#include "proto/r4/core/resources/invoice.pb.h"
#include "proto/r4/core/resources/library.pb.h"
#include "proto/r4/core/resources/linkage.pb.h"
#include "proto/r4/core/resources/list.pb.h"
#include "proto/r4/core/resources/location.pb.h"
#include "proto/r4/core/resources/measure.pb.h"
#include "proto/r4/core/resources/measure_report.pb.h"
#include "proto/r4/core/resources/media.pb.h"
#include "proto/r4/core/resources/medication.pb.h"
#include "proto/r4/core/resources/medication_administration.pb.h"
#include "proto/r4/core/resources/medication_dispense.pb.h"
#include "proto/r4/core/resources/medication_knowledge.pb.h"
#include "proto/r4/core/resources/medication_request.pb.h"
#include "proto/r4/core/resources/medication_statement.pb.h"
#include "proto/r4/core/resources/medicinal_product.pb.h"
#include "proto/r4/core/resources/medicinal_product_authorization.pb.h"
#include "proto/r4/core/resources/medicinal_product_contraindication.pb.h"
#include "proto/r4/core/resources/medicinal_product_indication.pb.h"
#include "proto/r4/core/resources/medicinal_product_ingredient.pb.h"
#include "proto/r4/core/resources/medicinal_product_interaction.pb.h"
#include "proto/r4/core/resources/medicinal_product_manufactured.pb.h"
#include "proto/r4/core/resources/medicinal_product_packaged.pb.h"
#include "proto/r4/core/resources/medicinal_product_pharmaceutical.pb.h"
#include "proto/r4/core/resources/medicinal_product_undesirable_effect.pb.h"
#include "proto/r4/core/resources/message_definition.pb.h"
#include "proto/r4/core/resources/message_header.pb.h"
#include "proto/r4/core/resources/metadata_resource.pb.h"
#include "proto/r4/core/resources/molecular_sequence.pb.h"
#include "proto/r4/core/resources/naming_system.pb.h"
#include "proto/r4/core/resources/nutrition_order.pb.h"
#include "proto/r4/core/resources/observation.pb.h"
#include "proto/r4/core/resources/observation_definition.pb.h"
#include "proto/r4/core/resources/operation_definition.pb.h"
#include "proto/r4/core/resources/operation_outcome.pb.h"
#include "proto/r4/core/resources/organization.pb.h"
#include "proto/r4/core/resources/organization_affiliation.pb.h"
#include "proto/r4/core/resources/parameters.pb.h"
#include "proto/r4/core/resources/patient.pb.h"
#include "proto/r4/core/resources/payment_notice.pb.h"
#include "proto/r4/core/resources/payment_reconciliation.pb.h"
#include "proto/r4/core/resources/person.pb.h"
#include "proto/r4/core/resources/plan_definition.pb.h"
#include "proto/r4/core/resources/practitioner.pb.h"
#include "proto/r4/core/resources/practitioner_role.pb.h"
#include "proto/r4/core/resources/procedure.pb.h"
#include "proto/r4/core/resources/provenance.pb.h"
#include "proto/r4/core/resources/questionnaire.pb.h"
#include "proto/r4/core/resources/questionnaire_response.pb.h"
#include "proto/r4/core/resources/related_person.pb.h"
#include "proto/r4/core/resources/request_group.pb.h"
#include "proto/r4/core/resources/research_definition.pb.h"
#include "proto/r4/core/resources/research_element_definition.pb.h"
#include "proto/r4/core/resources/research_study.pb.h"
#include "proto/r4/core/resources/research_subject.pb.h"
#include "proto/r4/core/resources/risk_assessment.pb.h"
#include "proto/r4/core/resources/risk_evidence_synthesis.pb.h"
#include "proto/r4/core/resources/schedule.pb.h"
#include "proto/r4/core/resources/search_parameter.pb.h"
#include "proto/r4/core/resources/service_request.pb.h"
#include "proto/r4/core/resources/slot.pb.h"
#include "proto/r4/core/resources/specimen.pb.h"
#include "proto/r4/core/resources/specimen_definition.pb.h"
#include "proto/r4/core/resources/structure_definition.pb.h"
#include "proto/r4/core/resources/structure_map.pb.h"
#include "proto/r4/core/resources/subscription.pb.h"
#include "proto/r4/core/resources/substance.pb.h"
#include "proto/r4/core/resources/substance_nucleic_acid.pb.h"
#include "proto/r4/core/resources/substance_polymer.pb.h"
#include "proto/r4/core/resources/substance_protein.pb.h"
#include "proto/r4/core/resources/substance_reference_information.pb.h"
#include "proto/r4/core/resources/substance_source_material.pb.h"
#include "proto/r4/core/resources/substance_specification.pb.h"
#include "proto/r4/core/resources/supply_delivery.pb.h"
#include "proto/r4/core/resources/supply_request.pb.h"
#include "proto/r4/core/resources/task.pb.h"
#include "proto/r4/core/resources/terminology_capabilities.pb.h"
#include "proto/r4/core/resources/test_report.pb.h"
#include "proto/r4/core/resources/test_script.pb.h"
#include "proto/r4/core/resources/value_set.pb.h"
#include "proto/r4/core/resources/verification_result.pb.h"
#include "proto/r4/core/resources/vision_prescription.pb.h"
#include "testdata/r4/profiles/test.pb.h"
#include "include/json/json.h"

namespace google {
namespace fhir {


namespace {

using namespace google::fhir::r4::core;  // NOLINT

static const char* const kTimeZoneString = "Australia/Sydney";

// json_path should be relative to fhir root
template <typename R>
StatusOr<R> ParseJsonToProto(const std::string& json_path) {
  // Some examples are invalid fhir.
  // See:
  // https://gforge.hl7.org/gf/project/fhir/tracker/?action=TrackerItemEdit&tracker_item_id=24933
  static std::unordered_set<std::string> INVALID_RECORDS{
      "spec/hl7.fhir.core/4.0.0/package/Questionnaire-qs1.json",
      "spec/hl7.fhir.core/4.0.0/package/Observation-clinical-gender.json",
      "spec/hl7.fhir.core/4.0.0/package/DeviceMetric-example.json",
      "spec/hl7.fhir.core/4.0.0/package/DeviceUseStatement-example.json",
      "spec/hl7.fhir.core/4.0.0/package/MedicationRequest-medrx0301.json"};

  std::string json = ReadFile(json_path);
  absl::TimeZone tz;
  absl::LoadTimeZone(kTimeZoneString, &tz);
  if (INVALID_RECORDS.find(json_path) == INVALID_RECORDS.end()) {
    return JsonFhirStringToProto<R>(json, tz);
  } else {
    return JsonFhirStringToProtoWithoutValidating<R>(json, tz);
  }
}

// proto_path should be relative to //testdata/r4
// json_path should be relative to fhir root
// This difference is because we read most json test data from the spec package,
// rather than the testdata directory.
template <typename R>
void TestParseWithFilepaths(const std::string& proto_path,
                            const std::string& json_path) {
  StatusOr<R> from_json_status = ParseJsonToProto<R>(json_path);
  ASSERT_TRUE(from_json_status.ok()) << "Failed parsing: " << json_path << "\n"
                                     << from_json_status.status();
  R from_json = from_json_status.ValueOrDie();
  R from_disk = ReadR4Proto<R>(proto_path);

  ::google::protobuf::util::MessageDifferencer differencer;
  std::string differences;
  differencer.ReportDifferencesToString(&differences);
  EXPECT_TRUE(differencer.Compare(from_disk, from_json)) << differences;
}

template <typename R>
void TestParse(const std::string& name) {
  TestParseWithFilepaths<R>(
      absl::StrCat("examples/", name, ".prototxt"),
      absl::StrCat("spec/hl7.fhir.core/4.0.0/package/", name + ".json"));
}

Json::Value ParseJsonStringToValue(const std::string& raw_json) {
  Json::Reader reader;
  Json::Value value;
  reader.parse(raw_json, value);
  return value;
}

template <typename R>
void TestPrintWithFilepaths(const std::string& proto_path,
                            const std::string& json_path) {
  const R proto = ReadR4Proto<R>(proto_path);
  StatusOr<std::string> from_proto_status = PrettyPrintFhirToJsonString(proto);
  ASSERT_TRUE(from_proto_status.ok())
      << "Failed Printing on: " << proto_path << ": "
      << from_proto_status.status().error_message();
  std::string from_proto = from_proto_status.ValueOrDie();
  std::string from_json = ReadFile(absl::StrCat(json_path));

  if (ParseJsonStringToValue(from_proto) != ParseJsonStringToValue(from_json)) {
    // This assert will fail, but we get terrible diff messages comparing
    // JsonCPP, so fall back to string diffs.
    EXPECT_EQ(from_proto, from_json);
  }
}

template <typename R>
void TestPrint(const std::string& name) {
  TestPrintWithFilepaths<R>(
      absl::StrCat("examples/", name, ".prototxt"),
      absl::StrCat("spec/hl7.fhir.core/4.0.0/package/", name + ".json"));
}

template <typename R>
void TestPair(const std::vector<std::string>& file_names) {
  for (const std::string& file : file_names) {
    TestPrint<R>(file);
    TestParse<R>(file);
  }
}

/** Test printing from a profile */
TEST(JsonFormatR4Test, PrintProfile) {
  TestPrintWithFilepaths<r4::testing::TestPatient>(
      "profiles/test_patient-profiled-testpatient.prototxt",
      "testdata/r4/profiles/test_patient.json");
}

/** Test parsing to a profile */
TEST(JsonFormatR4Test, ParseProfile) {
  TestParseWithFilepaths<r4::testing::TestPatient>(
      "profiles/test_patient-profiled-testpatient.prototxt",
      "testdata/r4/profiles/test_patient.json");
}

// Test parsing to a profile fails if the parsed resource doesn't match the
// profile
TEST(JsonFormatR4Test, ParseProfileMismatch) {
  ASSERT_FALSE(ParseJsonToProto<r4::testing::TestPatient>(
                   "testdata/r4/profiles/test_patient_multiple_names.json")
                   .ok());
}

template <typename R>
void TestPrintForAnalytics(const std::string& proto_filepath,
                           const std::string& json_filepath, bool pretty) {
  R proto = ReadR4Proto<R>(proto_filepath);
  if (IsProfile(R::descriptor())) {
    proto = NormalizeR4(proto).ValueOrDie();
  }
  auto result = pretty ? PrettyPrintFhirToJsonStringForAnalytics(proto)
                       : PrintFhirToJsonStringForAnalytics(proto);
  ASSERT_TRUE(result.ok()) << "Failed PrintForAnalytics on: " << proto_filepath
                           << "\n"
                           << result.status();
  std::string from_proto = result.ValueOrDie();
  std::string from_json = ReadFile(json_filepath);

  if (ParseJsonStringToValue(from_proto) != ParseJsonStringToValue(from_json)) {
    // This assert will fail, but we get terrible diff messages comparing
    // JsonCPP, so fall back to string diffs.
    ASSERT_EQ(from_json, from_proto);
  }
}

template <typename R>
void TestPrintForAnalyticsWithFilepath(const std::string& proto_filepath,
                                       const std::string& json_filepath) {
  TestPrintForAnalytics<R>(proto_filepath, json_filepath, true);
  TestPrintForAnalytics<R>(proto_filepath, json_filepath, false);
}

template <typename R>
void TestPrintForAnalytics(const std::string& name) {
  TestPrintForAnalyticsWithFilepath<R>(
      absl::StrCat("examples/", name, ".prototxt"),
      absl::StrCat("testdata/r4/bigquery/", name + ".json"));
}

TEST(JsonFormatR4Test, PrintForAnalytics) {
  TestPrintForAnalytics<Composition>("Composition-example");
  TestPrintForAnalytics<Encounter>("Encounter-home");
  TestPrintForAnalytics<Observation>("Observation-example-genetics-1");
  TestPrintForAnalytics<Patient>("Patient-example");
}

/** Test printing from a profile */
TEST(JsonFormatR4Test, PrintProfileForAnalytics) {
  TestPrintForAnalyticsWithFilepath<r4::testing::TestPatient>(
      "profiles/test_patient-profiled-testpatient.prototxt",
      "testdata/r4/bigquery/TestPatient.json");
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

TEST(JsonFormatR4Test, TestValuesetsBundle) {
  std::vector<std::string> files{
      "Bundle-valuesets",
  };
  TestPair<Bundle>(files);
}

TEST(JsonFormatR4Test, TestV2ValuesetsBundle) {
  std::vector<std::string> files{
      "Bundle-v2-valuesets",
  };
  TestPair<Bundle>(files);
}

TEST(JsonFormatR4Test, TestV3ValuesetsBundle) {
  std::vector<std::string> files{
      "Bundle-v3-valuesets",
  };
  TestPair<Bundle>(files);
}

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
}  // namespace

}  // namespace fhir
}  // namespace google
