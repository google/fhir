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

#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/json_format.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/stu3/resources.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "testdata/stu3/profiles/test.pb.h"
#include "include/json/json.h"

namespace google {
namespace fhir {


namespace {

using namespace google::fhir::stu3::proto;  // NOLINT

using google::fhir::stu3::proto::Condition;

static const char* const kTimeZoneString = "Australia/Sydney";

// json_path should be relative to fhir root
template <typename R>
R ParseJsonToProto(const std::string& json_path) {
  std::string json = ReadFile(json_path);
  absl::TimeZone tz;
  absl::LoadTimeZone(kTimeZoneString, &tz);
  return JsonFhirStringToProto<R>(json, tz).ValueOrDie();
}

// proto_path should be relative to //testdata/stu3
// json_path should be relative to fhir root
// This difference is because we read most json test data from the spec package,
// rather than the testdata directory.
template <typename R>
void TestParseWithFilepaths(const std::string& proto_path,
                            const std::string& json_path) {
  R from_json = ParseJsonToProto<R>(json_path);
  R from_disk = ReadStu3Proto<R>(proto_path);

  ::google::protobuf::util::MessageDifferencer differencer;
  std::string differences;
  differencer.ReportDifferencesToString(&differences);
  ASSERT_TRUE(differencer.Compare(from_disk, from_json)) << differences;
}

template <typename R>
void TestParse(const std::string& name) {
  TestParseWithFilepaths<R>(
      absl::StrCat("examples/", name, ".prototxt"),
      absl::StrCat("spec/hl7.fhir.core/3.0.1/package/", name + ".json"));
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
  const R proto = ReadStu3Proto<R>(proto_path);
  std::string from_proto = PrettyPrintFhirToJsonString(proto).ValueOrDie();
  std::string from_json = ReadFile(json_path);

  if (ParseJsonStringToValue(from_proto) != ParseJsonStringToValue(from_json)) {
    // This assert will fail, but we get terrible diff messages comparing
    // JsonCPP, so fall back to string diffs.
    ASSERT_EQ(from_proto, from_json);
  }
}

template <typename R>
void TestPrint(const std::string& name) {
  TestPrintWithFilepaths<R>(
      absl::StrCat("examples/", name, ".prototxt"),
      absl::StrCat("spec/hl7.fhir.core/3.0.1/package/", name + ".json"));
}

template <typename R>
void TestPrintForAnalytics(const std::string& name) {
  const R proto =
      ReadStu3Proto<R>(absl::StrCat("examples/", name, ".prototxt"));
  auto result = PrettyPrintFhirToJsonStringForAnalytics(proto);
  ASSERT_TRUE(result.ok()) << result.status();
  std::string from_proto = result.ValueOrDie();
  std::string from_json =
      ReadFile(absl::StrCat("testdata/stu3/bigquery/", name + ".json"));

  if (ParseJsonStringToValue(from_proto) != ParseJsonStringToValue(from_json)) {
    // This assert will fail, but we get terrible diff messages comparing
    // JsonCPP, so fall back to string diffs.
    ASSERT_EQ(from_json, from_proto);
  }
}

template <typename R>
void TestPrintForAnalytics(const std::string& name, bool pretty) {
  const R proto =
      ReadStu3Proto<R>(absl::StrCat("examples/", name, ".prototxt"));
  auto result = pretty ? PrettyPrintFhirToJsonStringForAnalytics(proto)
                       : PrintFhirToJsonStringForAnalytics(proto);
  ASSERT_TRUE(result.ok()) << result.status();
  std::string from_proto = result.ValueOrDie();
  std::string from_json =
      ReadFile(absl::StrCat("testdata/stu3/bigquery/", name + ".json"));

  if (ParseJsonStringToValue(from_proto) != ParseJsonStringToValue(from_json)) {
    // This assert will fail, but we get terrible diff messages comparing
    // JsonCPP, so fall back to string diffs.
    ASSERT_EQ(from_json, from_proto);
  }
}

/* Edge Case Testing */

/** Test parsing of FHIR edge cases. */
TEST(JsonFormatStu3Test, EdgeCasesParse) { TestParse<Patient>("Patient-null"); }

/** Test parsing of FHIR edge casers from ndjson. */
TEST(JsonFormatStu3Test, EdgeCasesNdjsonParse) {
  TestParseWithFilepaths<Patient>("examples/Patient-null.prototxt",
                                  "testdata/stu3/ndjson/Patient-null.ndjson");
}

/** Test parsing to a profile */
TEST(JsonFormatStu3Test, ParseProfile) {
  TestParseWithFilepaths<stu3::testing::TestPatient>(
      "profiles/test_patient-profiled-testpatient.prototxt",
      "testdata/stu3/profiles/test_patient.json");
  // This test is only meaningful because Patient.name is singular in the
  // profile, unlike in the unprofiled resource.  This means it has array
  // syntax in json, but singular syntax in proto.
  // Access the proto field directly, so it will be a compile-time failure if
  // that changes.
  ASSERT_EQ("Duck", ParseJsonToProto<stu3::testing::TestPatient>(
                        "testdata/stu3/profiles/test_patient.json")
                        .name()
                        .given(0)
                        .value());
}

/** Test printing from a profile */
TEST(JsonFormatStu3Test, PrintProfile) {
  TestPrintWithFilepaths<stu3::testing::TestPatient>(
      "profiles/test_patient-profiled-testpatient.prototxt",
      "testdata/stu3/profiles/test_patient.json");
}

/** Test printing of FHIR edge cases. */
TEST(JsonFormatStu3Test, EdgeCasesPrint) { TestPrint<Patient>("Patient-null"); }

TEST(JsonFormatStu3Test, PrettyPrintForAnalytics) {
  TestPrintForAnalytics<Composition>("Composition-example", true);
  TestPrintForAnalytics<Encounter>("Encounter-home", true);
  TestPrintForAnalytics<Observation>("Observation-example-genetics-1", true);
  TestPrintForAnalytics<Patient>("patient-example", true);
}

TEST(JsonFormatStu3Test, PrintForAnalytics) {
  TestPrintForAnalytics<Composition>("Composition-example", false);
  TestPrintForAnalytics<Encounter>("Encounter-home", false);
  TestPrintForAnalytics<Observation>("Observation-example-genetics-1", false);
  TestPrintForAnalytics<Patient>("patient-example", false);
}

/* Resource tests start here. */

/** Test parsing of the Account FHIR resource. */
TEST(JsonFormatStu3Test, AccountParse) {
  TestParse<Account>("Account-example");
  TestParse<Account>("Account-ewg");
}

/** Test parsing of the ActivityDefinition FHIR resource. */
TEST(JsonFormatStu3Test, ActivityDefinitionParse) {
  TestParse<ActivityDefinition>(
      "ActivityDefinition-referralPrimaryCareMentalHealth");
  TestParse<ActivityDefinition>("ActivityDefinition-citalopramPrescription");
  TestParse<ActivityDefinition>(
      "ActivityDefinition-referralPrimaryCareMentalHealth-initial");
  TestParse<ActivityDefinition>("ActivityDefinition-heart-valve-replacement");
  TestParse<ActivityDefinition>("ActivityDefinition-blood-tubes-supply");
}

/** Test parsing of the AdverseEvent FHIR resource. */
TEST(JsonFormatStu3Test, AdverseEventParse) {
  TestParse<AdverseEvent>("AdverseEvent-example");
}

/** Test parsing of the AllergyIntolerance FHIR resource. */
TEST(JsonFormatStu3Test, AllergyIntoleranceParse) {
  TestParse<AllergyIntolerance>("AllergyIntolerance-example");
}

/** Test parsing of the Appointment FHIR resource. */
TEST(JsonFormatStu3Test, AppointmentParse) {
  TestParse<Appointment>("Appointment-example");
  TestParse<Appointment>("Appointment-2docs");
  TestParse<Appointment>("Appointment-examplereq");
}

/** Test parsing of the AppointmentResponse FHIR resource. */
TEST(JsonFormatStu3Test, AppointmentResponseParse) {
  TestParse<AppointmentResponse>("AppointmentResponse-example");
  TestParse<AppointmentResponse>("AppointmentResponse-exampleresp");
}

/** Test parsing of the AuditEvent FHIR resource. */
TEST(JsonFormatStu3Test, AuditEventParse) {
  TestParse<AuditEvent>("AuditEvent-example");
  TestParse<AuditEvent>("AuditEvent-example-disclosure");
  TestParse<AuditEvent>("AuditEvent-example-login");
  TestParse<AuditEvent>("AuditEvent-example-logout");
  TestParse<AuditEvent>("AuditEvent-example-media");
  TestParse<AuditEvent>("AuditEvent-example-pixQuery");
  TestParse<AuditEvent>("AuditEvent-example-search");
  TestParse<AuditEvent>("AuditEvent-example-rest");
}

/** Test parsing of the Basic FHIR resource. */
TEST(JsonFormatStu3Test, BasicParse) {
  TestParse<Basic>("Basic-referral");
  TestParse<Basic>("Basic-classModel");
  TestParse<Basic>("Basic-basic-example-narrative");
}

/** Test parsing of the Binary FHIR resource. */
TEST(JsonFormatStu3Test, BinaryParse) { TestParse<Binary>("Binary-example"); }

/** Test parsing of the BodySite FHIR resource. */
TEST(JsonFormatStu3Test, BodySiteParse) {
  TestParse<BodySite>("BodySite-fetus");
  TestParse<BodySite>("BodySite-skin-patch");
  TestParse<BodySite>("BodySite-tumor");
}

/** Test parsing of the Bundle FHIR resource. */
TEST(JsonFormatStu3Test, BundleParse) {
  TestParse<Bundle>("Bundle-bundle-example");
  TestParse<Bundle>("Bundle-72ac8493-52ac-41bd-8d5d-7258c289b5ea");
  TestParse<Bundle>("Bundle-hla-1");
  TestParse<Bundle>("Bundle-father");
  TestParse<Bundle>("Bundle-b0a5e4277-83c4-4adb-87e2-e3efe3369b6f");
  TestParse<Bundle>("Bundle-3ad0687e-f477-468c-afd5-fcc2bf897819");
  TestParse<Bundle>("patient-examples-cypress-template");
  TestParse<Bundle>("Bundle-b248b1b2-1686-4b94-9936-37d7a5f94b51");
  TestParse<Bundle>("Bundle-3ad0687e-f477-468c-afd5-fcc2bf897809");
  TestParse<Bundle>("Bundle-3ad0687e-f477-468c-afd5-fcc2bf897808");
  TestParse<Bundle>("Bundle-ussg-fht");
  TestParse<Bundle>("Bundle-xds");
}

/** Test parsing of the CapabilityStatement FHIR resource. */
TEST(JsonFormatStu3Test, CapabilityStatementParse) {
  TestParse<CapabilityStatement>("CapabilityStatement-example");
  TestParse<CapabilityStatement>("CapabilityStatement-phr");
}

/** Test parsing of the CarePlan FHIR resource. */
TEST(JsonFormatStu3Test, CarePlanParse) {
  TestParse<CarePlan>("CarePlan-example");
  TestParse<CarePlan>("CarePlan-f001");
  TestParse<CarePlan>("CarePlan-f002");
  TestParse<CarePlan>("CarePlan-f003");
  TestParse<CarePlan>("CarePlan-f201");
  TestParse<CarePlan>("CarePlan-f202");
  TestParse<CarePlan>("CarePlan-f203");
  TestParse<CarePlan>("CarePlan-gpvisit");
  TestParse<CarePlan>("CarePlan-integrate");
  TestParse<CarePlan>("CarePlan-obesity-narrative");
  TestParse<CarePlan>("CarePlan-preg");
}

/** Test parsing of the CareTeam FHIR resource. */
TEST(JsonFormatStu3Test, CareTeamParse) {
  TestParse<CareTeam>("CareTeam-example");
}

/** Test parsing of the ChargeItem FHIR resource. */
TEST(JsonFormatStu3Test, ChargeItemParse) {
  TestParse<ChargeItem>("ChargeItem-example");
}

/** Test parsing of the Claim FHIR resource. */
TEST(JsonFormatStu3Test, ClaimParse) {
  TestParse<Claim>("Claim-100150");
  TestParse<Claim>("Claim-960150");
  TestParse<Claim>("Claim-960151");
  TestParse<Claim>("Claim-100151");
  TestParse<Claim>("Claim-100156");
  TestParse<Claim>("Claim-100152");
  TestParse<Claim>("Claim-100155");
  TestParse<Claim>("Claim-100154");
  TestParse<Claim>("Claim-100153");
  TestParse<Claim>("Claim-760150");
  TestParse<Claim>("Claim-760152");
  TestParse<Claim>("Claim-760151");
  TestParse<Claim>("Claim-860150");
  TestParse<Claim>("Claim-660150");
  TestParse<Claim>("Claim-660151");
  TestParse<Claim>("Claim-660152");
}

/** Test parsing of the ClaimResponse FHIR resource. */
TEST(JsonFormatStu3Test, ClaimResponseParse) {
  TestParse<ClaimResponse>("ClaimResponse-R3500");
}

/** Test parsing of the ClinicalImpression FHIR resource. */
TEST(JsonFormatStu3Test, ClinicalImpressionParse) {
  TestParse<ClinicalImpression>("ClinicalImpression-example");
}

/** Test parsing of the CodeSystem FHIR resource. */
TEST(JsonFormatStu3Test, CodeSystemParse) {
  TestParse<CodeSystem>("codesystem-example");
  TestParse<CodeSystem>("codesystem-example-summary");
  TestParse<CodeSystem>("codesystem-list-example-codes");
}

/** Test parsing of the Communication FHIR resource. */
TEST(JsonFormatStu3Test, CommunicationParse) {
  TestParse<Communication>("Communication-example");
  TestParse<Communication>("Communication-fm-attachment");
  TestParse<Communication>("Communication-fm-solicited");
}

/** Test parsing of the CommunicationRequest FHIR resource. */
TEST(JsonFormatStu3Test, CommunicationRequestParse) {
  TestParse<CommunicationRequest>("CommunicationRequest-example");
  TestParse<CommunicationRequest>("CommunicationRequest-fm-solicit");
}

/** Test parsing of the CompartmentDefinition FHIR resource. */
TEST(JsonFormatStu3Test, CompartmentDefinitionParse) {
  TestParse<CompartmentDefinition>("CompartmentDefinition-example");
}

/** Test parsing of the Composition FHIR resource. */
TEST(JsonFormatStu3Test, CompositionParse) {
  TestParse<Composition>("Composition-example");
}

/** Test parsing of the ConceptMap FHIR resource. */
TEST(JsonFormatStu3Test, ConceptMapParse) {
  TestParse<ConceptMap>("conceptmap-example");
  TestParse<ConceptMap>("conceptmap-example-2");
  TestParse<ConceptMap>("conceptmap-example-specimen-type");
}

/** Test parsing of the Condition FHIR resource. */
TEST(JsonFormatStu3Test, ConditionParse) {
  TestParse<Condition>("Condition-example");
  TestParse<Condition>("Condition-example2");
  TestParse<Condition>("Condition-f001");
  TestParse<Condition>("Condition-f002");
  TestParse<Condition>("Condition-f003");
  TestParse<Condition>("Condition-f201");
  TestParse<Condition>("Condition-f202");
  TestParse<Condition>("Condition-f203");
  TestParse<Condition>("Condition-f204");
  TestParse<Condition>("Condition-f205");
  TestParse<Condition>("Condition-family-history");
  TestParse<Condition>("Condition-stroke");
}

/** Test parsing of the Consent FHIR resource. */
TEST(JsonFormatStu3Test, ConsentParse) {
  TestParse<Consent>("Consent-consent-example-basic");
  TestParse<Consent>("Consent-consent-example-Emergency");
  TestParse<Consent>("Consent-consent-example-grantor");
  TestParse<Consent>("Consent-consent-example-notAuthor");
  TestParse<Consent>("Consent-consent-example-notOrg");
  TestParse<Consent>("Consent-consent-example-notThem");
  TestParse<Consent>("Consent-consent-example-notThis");
  TestParse<Consent>("Consent-consent-example-notTime");
  TestParse<Consent>("Consent-consent-example-Out");
  TestParse<Consent>("Consent-consent-example-pkb");
  TestParse<Consent>("Consent-consent-example-signature");
  TestParse<Consent>("Consent-consent-example-smartonfhir");
}

/** Test parsing of the Contract FHIR resource. */
TEST(JsonFormatStu3Test, ContractParse) {
  TestParse<Contract>("Contract-C-123");
  TestParse<Contract>("Contract-C-2121");
  TestParse<Contract>("Contract-pcd-example-notAuthor");
  TestParse<Contract>("Contract-pcd-example-notLabs");
  TestParse<Contract>("Contract-pcd-example-notOrg");
  TestParse<Contract>("Contract-pcd-example-notThem");
  TestParse<Contract>("Contract-pcd-example-notThis");
}

/** Test parsing of the Coverage FHIR resource. */
TEST(JsonFormatStu3Test, CoverageParse) {
  TestParse<Coverage>("Coverage-9876B1");
  TestParse<Coverage>("Coverage-7546D");
  TestParse<Coverage>("Coverage-7547E");
  TestParse<Coverage>("Coverage-SP1234");
}

/** Test parsing of the DataElement FHIR resource. */
TEST(JsonFormatStu3Test, DataElementParse) {
  TestParse<DataElement>("DataElement-gender");
  TestParse<DataElement>("DataElement-prothrombin");
}

/** Test parsing of the DetectedIssue FHIR resource. */
TEST(JsonFormatStu3Test, DetectedIssueParse) {
  TestParse<DetectedIssue>("DetectedIssue-ddi");
  TestParse<DetectedIssue>("DetectedIssue-allergy");
  TestParse<DetectedIssue>("DetectedIssue-duplicate");
  TestParse<DetectedIssue>("DetectedIssue-lab");
}

/** Test parsing of the Device FHIR resource. */
TEST(JsonFormatStu3Test, DeviceParse) {
  TestParse<Device>("Device-example");
  TestParse<Device>("Device-f001");
  TestParse<Device>("Device-ihe-pcd");
  TestParse<Device>("Device-example-pacemaker");
  TestParse<Device>("Device-software");
  TestParse<Device>("Device-example-udi1");
  TestParse<Device>("Device-example-udi2");
  TestParse<Device>("Device-example-udi3");
  TestParse<Device>("Device-example-udi4");
}

/** Test parsing of the DeviceComponent FHIR resource. */
TEST(JsonFormatStu3Test, DeviceComponentParse) {
  TestParse<DeviceComponent>("DeviceComponent-example");
  TestParse<DeviceComponent>("DeviceComponent-example-prodspec");
}

/** Test parsing of the DeviceMetric FHIR resource. */
TEST(JsonFormatStu3Test, DeviceMetricParse) {
  TestParse<DeviceMetric>("DeviceMetric-example");
}

/** Test parsing of the DeviceRequest FHIR resource. */
TEST(JsonFormatStu3Test, DeviceRequestParse) {
  TestParse<DeviceRequest>("DeviceRequest-example");
  TestParse<DeviceRequest>("DeviceRequest-insulinpump");
}

/** Test parsing of the DeviceUseStatement FHIR resource. */
TEST(JsonFormatStu3Test, DeviceUseStatementParse) {
  TestParse<DeviceUseStatement>("DeviceUseStatement-example");
}

/** Test parsing of the DiagnosticReport FHIR resource. */
TEST(JsonFormatStu3Test, DiagnosticReportParse) {
  TestParse<DiagnosticReport>("DiagnosticReport-101");
  TestParse<DiagnosticReport>("DiagnosticReport-102");
  TestParse<DiagnosticReport>("DiagnosticReport-f001");
  TestParse<DiagnosticReport>("DiagnosticReport-f201");
  TestParse<DiagnosticReport>("DiagnosticReport-f202");
  TestParse<DiagnosticReport>("DiagnosticReport-ghp");
  TestParse<DiagnosticReport>("DiagnosticReport-gingival-mass");
  TestParse<DiagnosticReport>("DiagnosticReport-lipids");
  TestParse<DiagnosticReport>("DiagnosticReport-pap");
  TestParse<DiagnosticReport>("DiagnosticReport-example-pgx");
  TestParse<DiagnosticReport>("DiagnosticReport-ultrasound");
  TestParse<DiagnosticReport>("DiagnosticReport-dg2");
}

/** Test parsing of the DocumentManifest FHIR resource. */
TEST(JsonFormatStu3Test, DocumentManifestParse) {
  TestParse<DocumentManifest>("DocumentManifest-example");
}

/** Test parsing of the DocumentReference FHIR resource. */
TEST(JsonFormatStu3Test, DocumentReferenceParse) {
  TestParse<DocumentReference>("DocumentReference-example");
}

/** Test parsing of the EligibilityRequest FHIR resource. */
TEST(JsonFormatStu3Test, EligibilityRequestParse) {
  TestParse<EligibilityRequest>("EligibilityRequest-52345");
  TestParse<EligibilityRequest>("EligibilityRequest-52346");
}

/** Test parsing of the EligibilityResponse FHIR resource. */
TEST(JsonFormatStu3Test, EligibilityResponseParse) {
  TestParse<EligibilityResponse>("EligibilityResponse-E2500");
  TestParse<EligibilityResponse>("EligibilityResponse-E2501");
  TestParse<EligibilityResponse>("EligibilityResponse-E2502");
  TestParse<EligibilityResponse>("EligibilityResponse-E2503");
}

/** Test parsing of the Encounter FHIR resource. */
TEST(JsonFormatStu3Test, EncounterParse) {
  TestParse<Encounter>("Encounter-example");
  TestParse<Encounter>("Encounter-emerg");
  TestParse<Encounter>("Encounter-f001");
  TestParse<Encounter>("Encounter-f002");
  TestParse<Encounter>("Encounter-f003");
  TestParse<Encounter>("Encounter-f201");
  TestParse<Encounter>("Encounter-f202");
  TestParse<Encounter>("Encounter-f203");
  TestParse<Encounter>("Encounter-home");
  TestParse<Encounter>("Encounter-xcda");
}

/** Test parsing of the Endpoint FHIR resource. */
TEST(JsonFormatStu3Test, EndpointParse) {
  TestParse<Endpoint>("Endpoint-example");
  TestParse<Endpoint>("Endpoint-example-iid");
  TestParse<Endpoint>("Endpoint-example-wadors");
}

/** Test parsing of the EnrollmentRequest FHIR resource. */
TEST(JsonFormatStu3Test, EnrollmentRequestParse) {
  TestParse<EnrollmentRequest>("EnrollmentRequest-22345");
}

/** Test parsing of the EnrollmentResponse FHIR resource. */
TEST(JsonFormatStu3Test, EnrollmentResponseParse) {
  TestParse<EnrollmentResponse>("EnrollmentResponse-ER2500");
}

/** Test parsing of the EpisodeOfCare FHIR resource. */
TEST(JsonFormatStu3Test, EpisodeOfCareParse) {
  TestParse<EpisodeOfCare>("EpisodeOfCare-example");
}

/** Test parsing of the ExpansionProfile FHIR resource. */
TEST(JsonFormatStu3Test, ExpansionProfileParse) {
  TestParse<ExpansionProfile>("ExpansionProfile-example");
}

/** Test parsing of the ExplanationOfBenefit FHIR resource. */
TEST(JsonFormatStu3Test, ExplanationOfBenefitParse) {
  TestParse<ExplanationOfBenefit>("ExplanationOfBenefit-EB3500");
}

/** Test parsing of the FamilyMemberHistory FHIR resource. */
TEST(JsonFormatStu3Test, FamilyMemberHistoryParse) {
  TestParse<FamilyMemberHistory>("FamilyMemberHistory-father");
  TestParse<FamilyMemberHistory>("FamilyMemberHistory-mother");
}

/** Test parsing of the Flag FHIR resource. */
TEST(JsonFormatStu3Test, FlagParse) {
  TestParse<Flag>("Flag-example");
  TestParse<Flag>("Flag-example-encounter");
}

/** Test parsing of the Goal FHIR resource. */
TEST(JsonFormatStu3Test, GoalParse) {
  TestParse<Goal>("Goal-example");
  TestParse<Goal>("Goal-stop-smoking");
}

/** Test parsing of the GraphDefinition FHIR resource. */
TEST(JsonFormatStu3Test, GraphDefinitionParse) {
  TestParse<GraphDefinition>("GraphDefinition-example");
}

/** Test parsing of the Group FHIR resource. */
TEST(JsonFormatStu3Test, GroupParse) {
  TestParse<Group>("Group-101");
  TestParse<Group>("Group-102");
}

/** Test parsing of the GuidanceResponse FHIR resource. */
TEST(JsonFormatStu3Test, GuidanceResponseParse) {
  TestParse<GuidanceResponse>("GuidanceResponse-example");
}

/** Test parsing of the HealthcareService FHIR resource. */
TEST(JsonFormatStu3Test, HealthcareServiceParse) {
  TestParse<HealthcareService>("HealthcareService-example");
}

/** Test parsing of the ImagingManifest FHIR resource. */
TEST(JsonFormatStu3Test, ImagingManifestParse) {
  TestParse<ImagingManifest>("ImagingManifest-example");
}

/** Test parsing of the ImagingStudy FHIR resource. */
TEST(JsonFormatStu3Test, ImagingStudyParse) {
  TestParse<ImagingStudy>("ImagingStudy-example");
  TestParse<ImagingStudy>("ImagingStudy-example-xr");
}

/** Test parsing of the Immunization FHIR resource. */
TEST(JsonFormatStu3Test, ImmunizationParse) {
  TestParse<Immunization>("Immunization-example");
  TestParse<Immunization>("Immunization-historical");
  TestParse<Immunization>("Immunization-notGiven");
}

/** Test parsing of the ImmunizationRecommendation FHIR resource. */
TEST(JsonFormatStu3Test, ImmunizationRecommendationParse) {
  TestParse<ImmunizationRecommendation>("ImmunizationRecommendation-example");
  TestParse<ImmunizationRecommendation>(
      "immunizationrecommendation-target-disease-example");
}

/** Test parsing of the ImplementationGuide FHIR resource. */
TEST(JsonFormatStu3Test, ImplementationGuideParse) {
  TestParse<ImplementationGuide>("ImplementationGuide-example");
}

/** Test parsing of the Library FHIR resource. */
TEST(JsonFormatStu3Test, LibraryParse) {
  TestParse<Library>("Library-library-cms146-example");
  TestParse<Library>("Library-composition-example");
  TestParse<Library>("Library-example");
  TestParse<Library>("Library-library-fhir-helpers-predecessor");
}

/** Test parsing of the Linkage FHIR resource. */
TEST(JsonFormatStu3Test, LinkageParse) {
  TestParse<Linkage>("Linkage-example");
}

/** Test parsing of the List FHIR resource. */
TEST(JsonFormatStu3Test, ListParse) {
  TestParse<List>("List-example");
  TestParse<List>("List-current-allergies");
  TestParse<List>("List-example-double-cousin-relationship");
  TestParse<List>("List-example-empty");
  TestParse<List>("List-f201");
  TestParse<List>("List-genetic");
  TestParse<List>("List-prognosis");
  TestParse<List>("List-med-list");
  TestParse<List>("List-example-simple-empty");
}

/** Test parsing of the Location FHIR resource. */
TEST(JsonFormatStu3Test, LocationParse) {
  TestParse<Location>("Location-1");
  TestParse<Location>("Location-amb");
  TestParse<Location>("Location-hl7");
  TestParse<Location>("Location-ph");
  TestParse<Location>("Location-2");
  TestParse<Location>("Location-ukp");
}

/** Test parsing of the Measure FHIR resource. */
TEST(JsonFormatStu3Test, MeasureParse) {
  TestParse<Measure>("Measure-measure-cms146-example");
  TestParse<Measure>("Measure-component-a-example");
  TestParse<Measure>("Measure-component-b-example");
  TestParse<Measure>("Measure-composite-example");
  TestParse<Measure>("Measure-measure-predecessor-example");
}

/** Test parsing of the MeasureReport FHIR resource. */
TEST(JsonFormatStu3Test, MeasureReportParse) {
  TestParse<MeasureReport>("MeasureReport-measurereport-cms146-cat1-example");
  TestParse<MeasureReport>("MeasureReport-measurereport-cms146-cat2-example");
  TestParse<MeasureReport>("MeasureReport-measurereport-cms146-cat3-example");
}

/** Test parsing of the Media FHIR resource. */
TEST(JsonFormatStu3Test, MediaParse) {
  TestParse<Media>("Media-example");
  TestParse<Media>(
      "Media-1.2.840.11361907579238403408700.3.0.14.19970327150033");
  TestParse<Media>("Media-sound");
  TestParse<Media>("Media-xray");
}

/** Test parsing of the Medication FHIR resource. */
TEST(JsonFormatStu3Test, MedicationParse) {
  TestParse<Medication>("Medication-med0301");
  TestParse<Medication>("Medication-med0302");
  TestParse<Medication>("Medication-med0303");
  TestParse<Medication>("Medication-med0304");
  TestParse<Medication>("Medication-med0305");
  TestParse<Medication>("Medication-med0306");
  TestParse<Medication>("Medication-med0307");
  TestParse<Medication>("Medication-med0308");
  TestParse<Medication>("Medication-med0309");
  TestParse<Medication>("Medication-med0310");
  TestParse<Medication>("Medication-med0311");
  TestParse<Medication>("Medication-med0312");
  TestParse<Medication>("Medication-med0313");
  TestParse<Medication>("Medication-med0314");
  TestParse<Medication>("Medication-med0315");
  TestParse<Medication>("Medication-med0316");
  TestParse<Medication>("Medication-med0317");
  TestParse<Medication>("Medication-med0318");
  TestParse<Medication>("Medication-med0319");
  TestParse<Medication>("Medication-med0320");
  TestParse<Medication>("Medication-med0321");
  TestParse<Medication>("Medication-medicationexample1");
  TestParse<Medication>("Medication-medexample015");
}

/** Test parsing of the MedicationAdministration FHIR resource. */
TEST(JsonFormatStu3Test, MedicationAdministrationParse) {
  TestParse<MedicationAdministration>(
      "MedicationAdministration-medadminexample03");
}

/** Test parsing of the MedicationDispense FHIR resource. */
TEST(JsonFormatStu3Test, MedicationDispenseParse) {
  TestParse<MedicationDispense>("MedicationDispense-meddisp008");
}

/** Test parsing of the MedicationRequest FHIR resource. */
TEST(JsonFormatStu3Test, MedicationRequestParse) {
  TestParse<MedicationRequest>("MedicationRequest-medrx0311");
  TestParse<MedicationRequest>("MedicationRequest-medrx002");
}

/** Test parsing of the MedicationStatement FHIR resource. */
TEST(JsonFormatStu3Test, MedicationStatementParse) {
  TestParse<MedicationStatement>("MedicationStatement-example001");
  TestParse<MedicationStatement>("MedicationStatement-example002");
  TestParse<MedicationStatement>("MedicationStatement-example003");
  TestParse<MedicationStatement>("MedicationStatement-example004");
  TestParse<MedicationStatement>("MedicationStatement-example005");
  TestParse<MedicationStatement>("MedicationStatement-example006");
  TestParse<MedicationStatement>("MedicationStatement-example007");
}

/** Test parsing of the MessageDefinition FHIR resource. */
TEST(JsonFormatStu3Test, MessageDefinitionParse) {
  TestParse<MessageDefinition>("MessageDefinition-example");
}

/** Test parsing of the MessageHeader FHIR resource. */
TEST(JsonFormatStu3Test, MessageHeaderParse) {
  TestParse<MessageHeader>(
      "MessageHeader-1cbdfb97-5859-48a4-8301-d54eab818d68");
}

/** Test parsing of the NamingSystem FHIR resource. */
TEST(JsonFormatStu3Test, NamingSystemParse) {
  TestParse<NamingSystem>("NamingSystem-example");
  TestParse<NamingSystem>("NamingSystem-example-id");
  TestParse<NamingSystem>("NamingSystem-example-replaced");
}

/** Test parsing of the NutritionOrder FHIR resource. */
TEST(JsonFormatStu3Test, NutritionOrderParse) {
  TestParse<NutritionOrder>("NutritionOrder-cardiacdiet");
  TestParse<NutritionOrder>("NutritionOrder-diabeticdiet");
  TestParse<NutritionOrder>("NutritionOrder-diabeticsupplement");
  TestParse<NutritionOrder>("NutritionOrder-energysupplement");
  TestParse<NutritionOrder>("NutritionOrder-enteralbolus");
  TestParse<NutritionOrder>("NutritionOrder-enteralcontinuous");
  TestParse<NutritionOrder>("NutritionOrder-fiberrestricteddiet");
  TestParse<NutritionOrder>("NutritionOrder-infantenteral");
  TestParse<NutritionOrder>("NutritionOrder-proteinsupplement");
  TestParse<NutritionOrder>("NutritionOrder-pureeddiet");
  TestParse<NutritionOrder>("NutritionOrder-pureeddiet-simple");
  TestParse<NutritionOrder>("NutritionOrder-renaldiet");
  TestParse<NutritionOrder>("NutritionOrder-texturemodified");
}

/** Test parsing of the Observation FHIR resource. */
TEST(JsonFormatStu3Test, ObservationParse) {
  TestParse<Observation>("Observation-example");
  TestParse<Observation>("Observation-10minute-apgar-score");
  TestParse<Observation>("Observation-1minute-apgar-score");
  TestParse<Observation>("Observation-20minute-apgar-score");
  TestParse<Observation>("Observation-2minute-apgar-score");
  TestParse<Observation>("Observation-5minute-apgar-score");
  TestParse<Observation>("Observation-blood-pressure");
  TestParse<Observation>("Observation-blood-pressure-cancel");
  TestParse<Observation>("Observation-blood-pressure-dar");
  TestParse<Observation>("Observation-bmd");
  TestParse<Observation>("Observation-bmi");
  TestParse<Observation>("Observation-body-height");
  TestParse<Observation>("Observation-body-length");
  TestParse<Observation>("Observation-body-temperature");
  TestParse<Observation>("Observation-date-lastmp");
  TestParse<Observation>("Observation-example-diplotype1");
  TestParse<Observation>("Observation-eye-color");
  TestParse<Observation>("Observation-f001");
  TestParse<Observation>("Observation-f002");
  TestParse<Observation>("Observation-f003");
  TestParse<Observation>("Observation-f004");
  TestParse<Observation>("Observation-f005");
  TestParse<Observation>("Observation-f202");
  TestParse<Observation>("Observation-f203");
  TestParse<Observation>("Observation-f204");
  TestParse<Observation>("Observation-f205");
  TestParse<Observation>("Observation-f206");
  TestParse<Observation>("Observation-example-genetics-1");
  TestParse<Observation>("Observation-example-genetics-2");
  TestParse<Observation>("Observation-example-genetics-3");
  TestParse<Observation>("Observation-example-genetics-4");
  TestParse<Observation>("Observation-example-genetics-5");
  TestParse<Observation>("Observation-glasgow");
  TestParse<Observation>("Observation-gcs-qa");
  TestParse<Observation>("Observation-example-haplotype1");
  TestParse<Observation>("Observation-example-haplotype2");
  TestParse<Observation>("Observation-head-circumference");
  TestParse<Observation>("Observation-heart-rate");
  TestParse<Observation>("Observation-mbp");
  TestParse<Observation>("Observation-example-phenotype");
  TestParse<Observation>("Observation-respiratory-rate");
  TestParse<Observation>("Observation-ekg");
  TestParse<Observation>("Observation-satO2");
  TestParse<Observation>("Observation-example-TPMT-diplotype");
  TestParse<Observation>("Observation-example-TPMT-haplotype-one");
  TestParse<Observation>("Observation-example-TPMT-haplotype-two");
  TestParse<Observation>("Observation-unsat");
  TestParse<Observation>("Observation-vitals-panel");
}

/** Test parsing of the OperationDefinition FHIR resource. */
TEST(JsonFormatStu3Test, OperationDefinitionParse) {
  TestParse<OperationDefinition>("OperationDefinition-example");
}

/** Test parsing of the OperationOutcome FHIR resource. */
TEST(JsonFormatStu3Test, OperationOutcomeParse) {
  TestParse<OperationOutcome>("OperationOutcome-101");
  TestParse<OperationOutcome>("OperationOutcome-allok");
  TestParse<OperationOutcome>("OperationOutcome-break-the-glass");
  TestParse<OperationOutcome>("OperationOutcome-exception");
  TestParse<OperationOutcome>("OperationOutcome-searchfail");
  TestParse<OperationOutcome>("OperationOutcome-validationfail");
}

/** Test parsing of the Organization FHIR resource. */
TEST(JsonFormatStu3Test, OrganizationParse) {
  TestParse<Organization>("Organization-hl7");
  TestParse<Organization>("Organization-f001");
  TestParse<Organization>("Organization-f002");
  TestParse<Organization>("Organization-f003");
  TestParse<Organization>("Organization-f201");
  TestParse<Organization>("Organization-f203");
  TestParse<Organization>("Organization-1");
  TestParse<Organization>("Organization-2.16.840.1.113883.19.5");
  TestParse<Organization>("Organization-2");
  TestParse<Organization>("Organization-1832473e-2fe0-452d-abe9-3cdb9879522f");
  TestParse<Organization>("Organization-mmanu");
}

/** Test parsing of the Parameters FHIR resource. */
TEST(JsonFormatStu3Test, ParametersParse) {
  TestParse<Parameters>("Parameters-example");
}

/** Test parsing of the Patient FHIR resource. */
TEST(JsonFormatStu3Test, PatientParse) {
  TestParse<Patient>("patient-example");
  TestParse<Patient>("patient-example-a");
  TestParse<Patient>("patient-example-animal");
  TestParse<Patient>("patient-example-b");
  TestParse<Patient>("patient-example-c");
  TestParse<Patient>("patient-example-chinese");
  TestParse<Patient>("patient-example-d");
  TestParse<Patient>("patient-example-dicom");
  TestParse<Patient>("patient-example-f001-pieter");
  TestParse<Patient>("patient-example-f201-roel");
  TestParse<Patient>("patient-example-ihe-pcd");
  TestParse<Patient>("patient-example-proband");
  TestParse<Patient>("patient-example-xcda");
  TestParse<Patient>("patient-example-xds");
  TestParse<Patient>("patient-genetics-example1");
  TestParse<Patient>("patient-glossy-example");
}

/** Test parsing of the PaymentNotice FHIR resource. */
TEST(JsonFormatStu3Test, PaymentNoticeParse) {
  TestParse<PaymentNotice>("PaymentNotice-77654");
}

/** Test parsing of the PaymentReconciliation FHIR resource. */
TEST(JsonFormatStu3Test, PaymentReconciliationParse) {
  TestParse<PaymentReconciliation>("PaymentReconciliation-ER2500");
}

/** Test parsing of the Person FHIR resource. */
TEST(JsonFormatStu3Test, PersonParse) {
  TestParse<Person>("Person-example");
  TestParse<Person>("Person-f002");
}

/** Test parsing of the PlanDefinition FHIR resource. */
TEST(JsonFormatStu3Test, PlanDefinitionParse) {
  TestParse<PlanDefinition>("PlanDefinition-low-suicide-risk-order-set");
  TestParse<PlanDefinition>("PlanDefinition-KDN5");
  TestParse<PlanDefinition>("PlanDefinition-options-example");
  TestParse<PlanDefinition>("PlanDefinition-zika-virus-intervention-initial");
  TestParse<PlanDefinition>("PlanDefinition-protocol-example");
}

/** Test parsing of the Practitioner FHIR resource. */
TEST(JsonFormatStu3Test, PractitionerParse) {
  TestParse<Practitioner>("Practitioner-example");
  TestParse<Practitioner>("Practitioner-f001");
  TestParse<Practitioner>("Practitioner-f002");
  TestParse<Practitioner>("Practitioner-f003");
  TestParse<Practitioner>("Practitioner-f004");
  TestParse<Practitioner>("Practitioner-f005");
  TestParse<Practitioner>("Practitioner-f006");
  TestParse<Practitioner>("Practitioner-f007");
  TestParse<Practitioner>("Practitioner-f201");
  TestParse<Practitioner>("Practitioner-f202");
  TestParse<Practitioner>("Practitioner-f203");
  TestParse<Practitioner>("Practitioner-f204");
  TestParse<Practitioner>("Practitioner-xcda1");
  TestParse<Practitioner>("Practitioner-xcda-author");
}

/** Test parsing of the PractitionerRole FHIR resource. */
TEST(JsonFormatStu3Test, PractitionerRoleParse) {
  TestParse<PractitionerRole>("PractitionerRole-example");
}

/** Test parsing of the Procedure FHIR resource. */
TEST(JsonFormatStu3Test, ProcedureParse) {
  TestParse<Procedure>("Procedure-example");
  TestParse<Procedure>("Procedure-ambulation");
  TestParse<Procedure>("Procedure-appendectomy-narrative");
  TestParse<Procedure>("Procedure-biopsy");
  TestParse<Procedure>("Procedure-colon-biopsy");
  TestParse<Procedure>("Procedure-colonoscopy");
  TestParse<Procedure>("Procedure-education");
  TestParse<Procedure>("Procedure-f001");
  TestParse<Procedure>("Procedure-f002");
  TestParse<Procedure>("Procedure-f003");
  TestParse<Procedure>("Procedure-f004");
  TestParse<Procedure>("Procedure-f201");
  TestParse<Procedure>("Procedure-example-implant");
  TestParse<Procedure>("Procedure-ob");
  TestParse<Procedure>("Procedure-physical-therapy");
}

/** Test parsing of the ProcedureRequest FHIR resource. */
TEST(JsonFormatStu3Test, ProcedureRequestParse) {
  TestParse<ProcedureRequest>("ProcedureRequest-example");
  TestParse<ProcedureRequest>("ProcedureRequest-physiotherapy");
  TestParse<ProcedureRequest>("ProcedureRequest-do-not-turn");
  TestParse<ProcedureRequest>("ProcedureRequest-benchpress");
  TestParse<ProcedureRequest>("ProcedureRequest-ambulation");
  TestParse<ProcedureRequest>("ProcedureRequest-appendectomy-narrative");
  TestParse<ProcedureRequest>("ProcedureRequest-colonoscopy");
  TestParse<ProcedureRequest>("ProcedureRequest-colon-biopsy");
  TestParse<ProcedureRequest>("ProcedureRequest-di");
  TestParse<ProcedureRequest>("ProcedureRequest-education");
  TestParse<ProcedureRequest>("ProcedureRequest-ft4");
  TestParse<ProcedureRequest>("ProcedureRequest-example-implant");
  TestParse<ProcedureRequest>("ProcedureRequest-lipid");
  TestParse<ProcedureRequest>("ProcedureRequest-ob");
  TestParse<ProcedureRequest>("ProcedureRequest-example-pgx");
  TestParse<ProcedureRequest>("ProcedureRequest-physical-therapy");
  TestParse<ProcedureRequest>("ProcedureRequest-subrequest");
  TestParse<ProcedureRequest>("ProcedureRequest-og-example1");
}

/** Test parsing of the ProcessRequest FHIR resource. */
TEST(JsonFormatStu3Test, ProcessRequestParse) {
  TestParse<ProcessRequest>("ProcessRequest-1110");
  TestParse<ProcessRequest>("ProcessRequest-1115");
  TestParse<ProcessRequest>("ProcessRequest-1113");
  TestParse<ProcessRequest>("ProcessRequest-1112");
  TestParse<ProcessRequest>("ProcessRequest-1114");
  TestParse<ProcessRequest>("ProcessRequest-1111");
  TestParse<ProcessRequest>("ProcessRequest-44654");
  TestParse<ProcessRequest>("ProcessRequest-87654");
  TestParse<ProcessRequest>("ProcessRequest-87655");
}

/** Test parsing of the ProcessResponse FHIR resource. */
TEST(JsonFormatStu3Test, ProcessResponseParse) {
  TestParse<ProcessResponse>("ProcessResponse-SR2500");
  TestParse<ProcessResponse>("ProcessResponse-SR2349");
  TestParse<ProcessResponse>("ProcessResponse-SR2499");
}

/** Test parsing of the Provenance FHIR resource. */
TEST(JsonFormatStu3Test, ProvenanceParse) {
  TestParse<Provenance>("Provenance-example");
  TestParse<Provenance>("Provenance-example-biocompute-object");
  TestParse<Provenance>("Provenance-example-cwl");
  TestParse<Provenance>("Provenance-signature");
}

/** Test parsing of the Questionnaire FHIR resource. */
TEST(JsonFormatStu3Test, QuestionnaireParse) {
  TestParse<Questionnaire>("Questionnaire-3141");
  TestParse<Questionnaire>("Questionnaire-bb");
  TestParse<Questionnaire>("Questionnaire-f201");
  TestParse<Questionnaire>("Questionnaire-gcs");
}

/** Test parsing of the QuestionnaireResponse FHIR resource. */
TEST(JsonFormatStu3Test, QuestionnaireResponseParse) {
  TestParse<QuestionnaireResponse>("QuestionnaireResponse-3141");
  TestParse<QuestionnaireResponse>("QuestionnaireResponse-bb");
  TestParse<QuestionnaireResponse>("QuestionnaireResponse-f201");
  TestParse<QuestionnaireResponse>("QuestionnaireResponse-gcs");
  TestParse<QuestionnaireResponse>("QuestionnaireResponse-ussg-fht-answers");
}

/** Test parsing of the ReferralRequest FHIR resource. */
TEST(JsonFormatStu3Test, ReferralRequestParse) {
  TestParse<ReferralRequest>("ReferralRequest-example");
}

/** Test parsing of the RelatedPerson FHIR resource. */
TEST(JsonFormatStu3Test, RelatedPersonParse) {
  TestParse<RelatedPerson>("RelatedPerson-benedicte");
  TestParse<RelatedPerson>("RelatedPerson-f001");
  TestParse<RelatedPerson>("RelatedPerson-f002");
  TestParse<RelatedPerson>("RelatedPerson-peter");
}

/** Test parsing of the RequestGroup FHIR resource. */
TEST(JsonFormatStu3Test, RequestGroupParse) {
  TestParse<RequestGroup>("RequestGroup-example");
  TestParse<RequestGroup>("RequestGroup-kdn5-example");
}

/** Test parsing of the ResearchStudy FHIR resource. */
TEST(JsonFormatStu3Test, ResearchStudyParse) {
  TestParse<ResearchStudy>("ResearchStudy-example");
}

/** Test parsing of the ResearchSubject FHIR resource. */
TEST(JsonFormatStu3Test, ResearchSubjectParse) {
  TestParse<ResearchSubject>("ResearchSubject-example");
}

/** Test parsing of the RiskAssessment FHIR resource. */
TEST(JsonFormatStu3Test, RiskAssessmentParse) {
  TestParse<RiskAssessment>("RiskAssessment-genetic");
  TestParse<RiskAssessment>("RiskAssessment-cardiac");
  TestParse<RiskAssessment>("RiskAssessment-population");
  TestParse<RiskAssessment>("RiskAssessment-prognosis");
}

/** Test parsing of the Schedule FHIR resource. */
TEST(JsonFormatStu3Test, ScheduleParse) {
  TestParse<Schedule>("Schedule-example");
  TestParse<Schedule>("Schedule-exampleloc1");
  TestParse<Schedule>("Schedule-exampleloc2");
}

/** Test parsing of the SearchParameter FHIR resource. */
TEST(JsonFormatStu3Test, SearchParameterParse) {
  TestParse<SearchParameter>("SearchParameter-example");
  TestParse<SearchParameter>("SearchParameter-example-extension");
  TestParse<SearchParameter>("SearchParameter-example-reference");
}

/** Test parsing of the Sequence FHIR resource. */
TEST(JsonFormatStu3Test, SequenceParse) {
  TestParse<Sequence>("Sequence-coord-0-base");
  TestParse<Sequence>("Sequence-coord-1-base");
  TestParse<Sequence>("Sequence-example");
  TestParse<Sequence>("Sequence-fda-example");
  TestParse<Sequence>("Sequence-fda-vcf-comparison");
  TestParse<Sequence>("Sequence-fda-vcfeval-comparison");
  TestParse<Sequence>("Sequence-example-pgx-1");
  TestParse<Sequence>("Sequence-example-pgx-2");
  TestParse<Sequence>("Sequence-example-TPMT-one");
  TestParse<Sequence>("Sequence-example-TPMT-two");
  TestParse<Sequence>("Sequence-graphic-example-1");
  TestParse<Sequence>("Sequence-graphic-example-2");
  TestParse<Sequence>("Sequence-graphic-example-3");
  TestParse<Sequence>("Sequence-graphic-example-4");
  TestParse<Sequence>("Sequence-graphic-example-5");
}

/** Test parsing of the ServiceDefinition FHIR resource. */
TEST(JsonFormatStu3Test, ServiceDefinitionParse) {
  TestParse<ServiceDefinition>("ServiceDefinition-example");
}

/** Test parsing of the Slot FHIR resource. */
TEST(JsonFormatStu3Test, SlotParse) {
  TestParse<Slot>("Slot-example");
  TestParse<Slot>("Slot-1");
  TestParse<Slot>("Slot-2");
  TestParse<Slot>("Slot-3");
}

/** Test parsing of the Specimen FHIR resource. */
TEST(JsonFormatStu3Test, SpecimenParse) {
  TestParse<Specimen>("Specimen-101");
  TestParse<Specimen>("Specimen-isolate");
  TestParse<Specimen>("Specimen-sst");
  TestParse<Specimen>("Specimen-vma-urine");
}

/** Test parsing of the StructureDefinition FHIR resource. */
TEST(JsonFormatStu3Test, StructureDefinitionParse) {
  TestParse<StructureDefinition>("StructureDefinition-example");
}

/** Test parsing of the StructureMap FHIR resource. */
TEST(JsonFormatStu3Test, StructureMapParse) {
  TestParse<StructureMap>("StructureMap-example");
}

/** Test parsing of the Subscription FHIR resource. */
TEST(JsonFormatStu3Test, SubscriptionParse) {
  TestParse<Subscription>("Subscription-example");
  TestParse<Subscription>("Subscription-example-error");
}

/** Test parsing of the Substance FHIR resource. */
TEST(JsonFormatStu3Test, SubstanceParse) {
  TestParse<Substance>("Substance-example");
  TestParse<Substance>("Substance-f205");
  TestParse<Substance>("Substance-f201");
  TestParse<Substance>("Substance-f202");
  TestParse<Substance>("Substance-f203");
  TestParse<Substance>("Substance-f204");
}

/** Test parsing of the SupplyDelivery FHIR resource. */
TEST(JsonFormatStu3Test, SupplyDeliveryParse) {
  TestParse<SupplyDelivery>("SupplyDelivery-simpledelivery");
  TestParse<SupplyDelivery>("SupplyDelivery-pumpdelivery");
}

/** Test parsing of the SupplyRequest FHIR resource. */
TEST(JsonFormatStu3Test, SupplyRequestParse) {
  TestParse<SupplyRequest>("SupplyRequest-simpleorder");
}

/** Test parsing of the Task FHIR resource. */
TEST(JsonFormatStu3Test, TaskParse) {
  TestParse<Task>("Task-example1");
  TestParse<Task>("Task-example2");
  TestParse<Task>("Task-example3");
  TestParse<Task>("Task-example4");
  TestParse<Task>("Task-example5");
  TestParse<Task>("Task-example6");
}

/** Test parsing of the TestReport FHIR resource. */
TEST(JsonFormatStu3Test, TestReportParse) {
  TestParse<TestReport>("TestReport-testreport-example");
}

/** Test parsing of the TestScript FHIR resource. */
TEST(JsonFormatStu3Test, TestScriptParse) {
  TestParse<TestScript>("TestScript-testscript-example");
  TestParse<TestScript>("TestScript-testscript-example-history");
  TestParse<TestScript>("TestScript-testscript-example-multisystem");
  TestParse<TestScript>("TestScript-testscript-example-readtest");
  TestParse<TestScript>("TestScript-testscript-example-rule");
  TestParse<TestScript>("TestScript-testscript-example-search");
  TestParse<TestScript>("TestScript-testscript-example-update");
}

/** Test parsing of the ValueSet FHIR resource. */
TEST(JsonFormatStu3Test, ValueSetParse) {
  TestParse<ValueSet>("valueset-example");
  TestParse<ValueSet>("valueset-example-expansion");
  TestParse<ValueSet>("valueset-example-inactive");
  TestParse<ValueSet>("valueset-example-intensional");
  TestParse<ValueSet>("valueset-example-yesnodontknow");
  TestParse<ValueSet>("valueset-list-example-codes");
}

/** Test parsing of the VisionPrescription FHIR resource. */
TEST(JsonFormatStu3Test, VisionPrescriptionParse) {
  TestParse<VisionPrescription>("VisionPrescription-33123");
  TestParse<VisionPrescription>("VisionPrescription-33124");
}

// Print tests

/* Resource tests start here. */

/** Test printing of the Account FHIR resource. */
TEST(JsonFormatStu3Test, AccountPrint) {
  TestPrint<Account>("Account-example");
  TestPrint<Account>("Account-ewg");
}

/** Test printing of the ActivityDefinition FHIR resource. */
TEST(JsonFormatStu3Test, ActivityDefinitionPrint) {
  TestPrint<ActivityDefinition>(
      "ActivityDefinition-referralPrimaryCareMentalHealth");
  TestPrint<ActivityDefinition>("ActivityDefinition-citalopramPrescription");
  TestPrint<ActivityDefinition>(
      "ActivityDefinition-referralPrimaryCareMentalHealth-initial");
  TestPrint<ActivityDefinition>("ActivityDefinition-heart-valve-replacement");
  TestPrint<ActivityDefinition>("ActivityDefinition-blood-tubes-supply");
}

/** Test printing of the AdverseEvent FHIR resource. */
TEST(JsonFormatStu3Test, AdverseEventPrint) {
  TestPrint<AdverseEvent>("AdverseEvent-example");
}

/** Test printing of the AllergyIntolerance FHIR resource. */
TEST(JsonFormatStu3Test, AllergyIntolerancePrint) {
  TestPrint<AllergyIntolerance>("AllergyIntolerance-example");
}

/** Test printing of the Appointment FHIR resource. */
TEST(JsonFormatStu3Test, AppointmentPrint) {
  TestPrint<Appointment>("Appointment-example");
  TestPrint<Appointment>("Appointment-2docs");
  TestPrint<Appointment>("Appointment-examplereq");
}

/** Test printing of the AppointmentResponse FHIR resource. */
TEST(JsonFormatStu3Test, AppointmentResponsePrint) {
  TestPrint<AppointmentResponse>("AppointmentResponse-example");
  TestPrint<AppointmentResponse>("AppointmentResponse-exampleresp");
}

/** Test printing of the AuditEvent FHIR resource. */
TEST(JsonFormatStu3Test, AuditEventPrint) {
  TestPrint<AuditEvent>("AuditEvent-example");
  TestPrint<AuditEvent>("AuditEvent-example-disclosure");
  TestPrint<AuditEvent>("AuditEvent-example-login");
  TestPrint<AuditEvent>("AuditEvent-example-logout");
  TestPrint<AuditEvent>("AuditEvent-example-media");
  TestPrint<AuditEvent>("AuditEvent-example-pixQuery");
  TestPrint<AuditEvent>("AuditEvent-example-search");
  TestPrint<AuditEvent>("AuditEvent-example-rest");
}

/** Test printing of the Basic FHIR resource. */
TEST(JsonFormatStu3Test, BasicPrint) {
  TestPrint<Basic>("Basic-referral");
  TestPrint<Basic>("Basic-classModel");
  TestPrint<Basic>("Basic-basic-example-narrative");
}

/** Test printing of the Binary FHIR resource. */
TEST(JsonFormatStu3Test, BinaryPrint) { TestPrint<Binary>("Binary-example"); }

/** Test printing of the BodySite FHIR resource. */
TEST(JsonFormatStu3Test, BodySitePrint) {
  TestPrint<BodySite>("BodySite-fetus");
  TestPrint<BodySite>("BodySite-skin-patch");
  TestPrint<BodySite>("BodySite-tumor");
}

/** Test printing of the Bundle FHIR resource. */
TEST(JsonFormatStu3Test, BundlePrint) {
  TestPrint<Bundle>("Bundle-bundle-example");
  TestPrint<Bundle>("Bundle-72ac8493-52ac-41bd-8d5d-7258c289b5ea");
  TestPrint<Bundle>("Bundle-hla-1");
  TestPrint<Bundle>("Bundle-father");
  TestPrint<Bundle>("Bundle-b0a5e4277-83c4-4adb-87e2-e3efe3369b6f");
  TestPrint<Bundle>("Bundle-3ad0687e-f477-468c-afd5-fcc2bf897819");
  TestPrint<Bundle>("patient-examples-cypress-template");
  TestPrint<Bundle>("Bundle-b248b1b2-1686-4b94-9936-37d7a5f94b51");
  TestPrint<Bundle>("Bundle-3ad0687e-f477-468c-afd5-fcc2bf897809");
  TestPrint<Bundle>("Bundle-3ad0687e-f477-468c-afd5-fcc2bf897808");
  TestPrint<Bundle>("Bundle-ussg-fht");
  TestPrint<Bundle>("Bundle-xds");
}

/** Test printing of the CapabilityStatement FHIR resource. */
TEST(JsonFormatStu3Test, CapabilityStatementPrint) {
  TestPrint<CapabilityStatement>("CapabilityStatement-example");
  TestPrint<CapabilityStatement>("CapabilityStatement-phr");
}

/** Test printing of the CarePlan FHIR resource. */
TEST(JsonFormatStu3Test, CarePlanPrint) {
  TestPrint<CarePlan>("CarePlan-example");
  TestPrint<CarePlan>("CarePlan-f001");
  TestPrint<CarePlan>("CarePlan-f002");
  TestPrint<CarePlan>("CarePlan-f003");
  TestPrint<CarePlan>("CarePlan-f201");
  TestPrint<CarePlan>("CarePlan-f202");
  TestPrint<CarePlan>("CarePlan-f203");
  TestPrint<CarePlan>("CarePlan-gpvisit");
  TestPrint<CarePlan>("CarePlan-integrate");
  TestPrint<CarePlan>("CarePlan-obesity-narrative");
  TestPrint<CarePlan>("CarePlan-preg");
}

/** Test printing of the CareTeam FHIR resource. */
TEST(JsonFormatStu3Test, CareTeamPrint) {
  TestPrint<CareTeam>("CareTeam-example");
}

/** Test printing of the ChargeItem FHIR resource. */
TEST(JsonFormatStu3Test, ChargeItemPrint) {
  TestPrint<ChargeItem>("ChargeItem-example");
}

/** Test printing of the Claim FHIR resource. */
TEST(JsonFormatStu3Test, ClaimPrint) {
  TestPrint<Claim>("Claim-100150");
  TestPrint<Claim>("Claim-960150");
  TestPrint<Claim>("Claim-960151");
  TestPrint<Claim>("Claim-100151");
  TestPrint<Claim>("Claim-100156");
  TestPrint<Claim>("Claim-100152");
  TestPrint<Claim>("Claim-100155");
  TestPrint<Claim>("Claim-100154");
  TestPrint<Claim>("Claim-100153");
  TestPrint<Claim>("Claim-760150");
  TestPrint<Claim>("Claim-760152");
  TestPrint<Claim>("Claim-760151");
  TestPrint<Claim>("Claim-860150");
  TestPrint<Claim>("Claim-660150");
  TestPrint<Claim>("Claim-660151");
  TestPrint<Claim>("Claim-660152");
}

/** Test printing of the ClaimResponse FHIR resource. */
TEST(JsonFormatStu3Test, ClaimResponsePrint) {
  TestPrint<ClaimResponse>("ClaimResponse-R3500");
}

/** Test printing of the ClinicalImpression FHIR resource. */
TEST(JsonFormatStu3Test, ClinicalImpressionPrint) {
  TestPrint<ClinicalImpression>("ClinicalImpression-example");
}

/** Test printing of the CodeSystem FHIR resource. */
TEST(JsonFormatStu3Test, CodeSystemPrint) {
  TestPrint<CodeSystem>("codesystem-example");
  TestPrint<CodeSystem>("codesystem-example-summary");
  TestPrint<CodeSystem>("codesystem-list-example-codes");
}

/** Test printing of the Communication FHIR resource. */
TEST(JsonFormatStu3Test, CommunicationPrint) {
  TestPrint<Communication>("Communication-example");
  TestPrint<Communication>("Communication-fm-attachment");
  TestPrint<Communication>("Communication-fm-solicited");
}

/** Test printing of the CommunicationRequest FHIR resource. */
TEST(JsonFormatStu3Test, CommunicationRequestPrint) {
  TestPrint<CommunicationRequest>("CommunicationRequest-example");
  TestPrint<CommunicationRequest>("CommunicationRequest-fm-solicit");
}

/** Test printing of the CompartmentDefinition FHIR resource. */
TEST(JsonFormatStu3Test, CompartmentDefinitionPrint) {
  TestPrint<CompartmentDefinition>("CompartmentDefinition-example");
}

/** Test printing of the Composition FHIR resource. */
TEST(JsonFormatStu3Test, CompositionPrint) {
  TestPrint<Composition>("Composition-example");
}

/** Test printing of the ConceptMap FHIR resource. */
TEST(JsonFormatStu3Test, ConceptMapPrint) {
  TestPrint<ConceptMap>("conceptmap-example");
  TestPrint<ConceptMap>("conceptmap-example-2");
  TestPrint<ConceptMap>("conceptmap-example-specimen-type");
}

/** Test printing of the Condition FHIR resource. */
TEST(JsonFormatStu3Test, ConditionPrint) {
  TestPrint<Condition>("Condition-example");
  TestPrint<Condition>("Condition-example2");
  TestPrint<Condition>("Condition-f001");
  TestPrint<Condition>("Condition-f002");
  TestPrint<Condition>("Condition-f003");
  TestPrint<Condition>("Condition-f201");
  TestPrint<Condition>("Condition-f202");
  TestPrint<Condition>("Condition-f203");
  TestPrint<Condition>("Condition-f204");
  TestPrint<Condition>("Condition-f205");
  TestPrint<Condition>("Condition-family-history");
  TestPrint<Condition>("Condition-stroke");
}

/** Test printing of the Consent FHIR resource. */
TEST(JsonFormatStu3Test, ConsentPrint) {
  TestPrint<Consent>("Consent-consent-example-basic");
  TestPrint<Consent>("Consent-consent-example-Emergency");
  TestPrint<Consent>("Consent-consent-example-grantor");
  TestPrint<Consent>("Consent-consent-example-notAuthor");
  TestPrint<Consent>("Consent-consent-example-notOrg");
  TestPrint<Consent>("Consent-consent-example-notThem");
  TestPrint<Consent>("Consent-consent-example-notThis");
  TestPrint<Consent>("Consent-consent-example-notTime");
  TestPrint<Consent>("Consent-consent-example-Out");
  TestPrint<Consent>("Consent-consent-example-pkb");
  TestPrint<Consent>("Consent-consent-example-signature");
  TestPrint<Consent>("Consent-consent-example-smartonfhir");
}

/** Test printing of the Contract FHIR resource. */
TEST(JsonFormatStu3Test, ContractPrint) {
  TestPrint<Contract>("Contract-C-123");
  TestPrint<Contract>("Contract-C-2121");
  TestPrint<Contract>("Contract-pcd-example-notAuthor");
  TestPrint<Contract>("Contract-pcd-example-notLabs");
  TestPrint<Contract>("Contract-pcd-example-notOrg");
  TestPrint<Contract>("Contract-pcd-example-notThem");
  TestPrint<Contract>("Contract-pcd-example-notThis");
}

/** Test printing of the Coverage FHIR resource. */
TEST(JsonFormatStu3Test, CoveragePrint) {
  TestPrint<Coverage>("Coverage-9876B1");
  TestPrint<Coverage>("Coverage-7546D");
  TestPrint<Coverage>("Coverage-7547E");
  TestPrint<Coverage>("Coverage-SP1234");
}

/** Test printing of the DataElement FHIR resource. */
TEST(JsonFormatStu3Test, DataElementPrint) {
  TestPrint<DataElement>("DataElement-gender");
  TestPrint<DataElement>("DataElement-prothrombin");
}

/** Test printing of the DetectedIssue FHIR resource. */
TEST(JsonFormatStu3Test, DetectedIssuePrint) {
  TestPrint<DetectedIssue>("DetectedIssue-ddi");
  TestPrint<DetectedIssue>("DetectedIssue-allergy");
  TestPrint<DetectedIssue>("DetectedIssue-duplicate");
  TestPrint<DetectedIssue>("DetectedIssue-lab");
}

/** Test printing of the Device FHIR resource. */
TEST(JsonFormatStu3Test, DevicePrint) {
  TestPrint<Device>("Device-example");
  TestPrint<Device>("Device-f001");
  TestPrint<Device>("Device-ihe-pcd");
  TestPrint<Device>("Device-example-pacemaker");
  TestPrint<Device>("Device-software");
  TestPrint<Device>("Device-example-udi1");
  TestPrint<Device>("Device-example-udi2");
  TestPrint<Device>("Device-example-udi3");
  TestPrint<Device>("Device-example-udi4");
}

/** Test printing of the DeviceComponent FHIR resource. */
TEST(JsonFormatStu3Test, DeviceComponentPrint) {
  TestPrint<DeviceComponent>("DeviceComponent-example");
  TestPrint<DeviceComponent>("DeviceComponent-example-prodspec");
}

/** Test printing of the DeviceMetric FHIR resource. */
TEST(JsonFormatStu3Test, DeviceMetricPrint) {
  TestPrint<DeviceMetric>("DeviceMetric-example");
}

/** Test printing of the DeviceRequest FHIR resource. */
TEST(JsonFormatStu3Test, DeviceRequestPrint) {
  TestPrint<DeviceRequest>("DeviceRequest-example");
  TestPrint<DeviceRequest>("DeviceRequest-insulinpump");
}

/** Test printing of the DeviceUseStatement FHIR resource. */
TEST(JsonFormatStu3Test, DeviceUseStatementPrint) {
  TestPrint<DeviceUseStatement>("DeviceUseStatement-example");
}

/** Test printing of the DiagnosticReport FHIR resource. */
TEST(JsonFormatStu3Test, DiagnosticReportPrint) {
  TestPrint<DiagnosticReport>("DiagnosticReport-101");
  TestPrint<DiagnosticReport>("DiagnosticReport-102");
  TestPrint<DiagnosticReport>("DiagnosticReport-f001");
  TestPrint<DiagnosticReport>("DiagnosticReport-f201");
  TestPrint<DiagnosticReport>("DiagnosticReport-f202");
  TestPrint<DiagnosticReport>("DiagnosticReport-ghp");
  TestPrint<DiagnosticReport>("DiagnosticReport-gingival-mass");
  TestPrint<DiagnosticReport>("DiagnosticReport-lipids");
  TestPrint<DiagnosticReport>("DiagnosticReport-pap");
  TestPrint<DiagnosticReport>("DiagnosticReport-example-pgx");
  TestPrint<DiagnosticReport>("DiagnosticReport-ultrasound");
  TestPrint<DiagnosticReport>("DiagnosticReport-dg2");
}

/** Test printing of the DocumentManifest FHIR resource. */
TEST(JsonFormatStu3Test, DocumentManifestPrint) {
  TestPrint<DocumentManifest>("DocumentManifest-example");
}

/** Test printing of the DocumentReference FHIR resource. */
TEST(JsonFormatStu3Test, DocumentReferencePrint) {
  TestPrint<DocumentReference>("DocumentReference-example");
}

/** Test printing of the EligibilityRequest FHIR resource. */
TEST(JsonFormatStu3Test, EligibilityRequestPrint) {
  TestPrint<EligibilityRequest>("EligibilityRequest-52345");
  TestPrint<EligibilityRequest>("EligibilityRequest-52346");
}

/** Test printing of the EligibilityResponse FHIR resource. */
TEST(JsonFormatStu3Test, EligibilityResponsePrint) {
  TestPrint<EligibilityResponse>("EligibilityResponse-E2500");
  TestPrint<EligibilityResponse>("EligibilityResponse-E2501");
  TestPrint<EligibilityResponse>("EligibilityResponse-E2502");
  TestPrint<EligibilityResponse>("EligibilityResponse-E2503");
}

/** Test printing of the Encounter FHIR resource. */
TEST(JsonFormatStu3Test, EncounterPrint) {
  TestPrint<Encounter>("Encounter-example");
  TestPrint<Encounter>("Encounter-emerg");
  TestPrint<Encounter>("Encounter-f001");
  TestPrint<Encounter>("Encounter-f002");
  TestPrint<Encounter>("Encounter-f003");
  TestPrint<Encounter>("Encounter-f201");
  TestPrint<Encounter>("Encounter-f202");
  TestPrint<Encounter>("Encounter-f203");
  TestPrint<Encounter>("Encounter-home");
  TestPrint<Encounter>("Encounter-xcda");
}

/** Test printing of the Endpoint FHIR resource. */
TEST(JsonFormatStu3Test, EndpointPrint) {
  TestPrint<Endpoint>("Endpoint-example");
  TestPrint<Endpoint>("Endpoint-example-iid");
  TestPrint<Endpoint>("Endpoint-example-wadors");
}

/** Test printing of the EnrollmentRequest FHIR resource. */
TEST(JsonFormatStu3Test, EnrollmentRequestPrint) {
  TestPrint<EnrollmentRequest>("EnrollmentRequest-22345");
}

/** Test printing of the EnrollmentResponse FHIR resource. */
TEST(JsonFormatStu3Test, EnrollmentResponsePrint) {
  TestPrint<EnrollmentResponse>("EnrollmentResponse-ER2500");
}

/** Test printing of the EpisodeOfCare FHIR resource. */
TEST(JsonFormatStu3Test, EpisodeOfCarePrint) {
  TestPrint<EpisodeOfCare>("EpisodeOfCare-example");
}

/** Test printing of the ExpansionProfile FHIR resource. */
TEST(JsonFormatStu3Test, ExpansionProfilePrint) {
  TestPrint<ExpansionProfile>("ExpansionProfile-example");
}

/** Test printing of the ExplanationOfBenefit FHIR resource. */
TEST(JsonFormatStu3Test, ExplanationOfBenefitPrint) {
  TestPrint<ExplanationOfBenefit>("ExplanationOfBenefit-EB3500");
}

/** Test printing of the FamilyMemberHistory FHIR resource. */
TEST(JsonFormatStu3Test, FamilyMemberHistoryPrint) {
  TestPrint<FamilyMemberHistory>("FamilyMemberHistory-father");
  TestPrint<FamilyMemberHistory>("FamilyMemberHistory-mother");
}

/** Test printing of the Flag FHIR resource. */
TEST(JsonFormatStu3Test, FlagPrint) {
  TestPrint<Flag>("Flag-example");
  TestPrint<Flag>("Flag-example-encounter");
}

/** Test printing of the Goal FHIR resource. */
TEST(JsonFormatStu3Test, GoalPrint) {
  TestPrint<Goal>("Goal-example");
  TestPrint<Goal>("Goal-stop-smoking");
}

/** Test printing of the GraphDefinition FHIR resource. */
TEST(JsonFormatStu3Test, GraphDefinitionPrint) {
  TestPrint<GraphDefinition>("GraphDefinition-example");
}

/** Test printing of the Group FHIR resource. */
TEST(JsonFormatStu3Test, GroupPrint) {
  TestPrint<Group>("Group-101");
  TestPrint<Group>("Group-102");
}

/** Test printing of the GuidanceResponse FHIR resource. */
TEST(JsonFormatStu3Test, GuidanceResponsePrint) {
  TestPrint<GuidanceResponse>("GuidanceResponse-example");
}

/** Test printing of the HealthcareService FHIR resource. */
TEST(JsonFormatStu3Test, HealthcareServicePrint) {
  TestPrint<HealthcareService>("HealthcareService-example");
}

/** Test printing of the ImagingManifest FHIR resource. */
TEST(JsonFormatStu3Test, ImagingManifestPrint) {
  TestPrint<ImagingManifest>("ImagingManifest-example");
}

/** Test printing of the ImagingStudy FHIR resource. */
TEST(JsonFormatStu3Test, ImagingStudyPrint) {
  TestPrint<ImagingStudy>("ImagingStudy-example");
  TestPrint<ImagingStudy>("ImagingStudy-example-xr");
}

/** Test printing of the Immunization FHIR resource. */
TEST(JsonFormatStu3Test, ImmunizationPrint) {
  TestPrint<Immunization>("Immunization-example");
  TestPrint<Immunization>("Immunization-historical");
  TestPrint<Immunization>("Immunization-notGiven");
}

/** Test printing of the ImmunizationRecommendation FHIR resource. */
TEST(JsonFormatStu3Test, ImmunizationRecommendationPrint) {
  TestPrint<ImmunizationRecommendation>("ImmunizationRecommendation-example");
  TestPrint<ImmunizationRecommendation>("ImmunizationRecommendation-example");
}

/** Test printing of the ImplementationGuide FHIR resource. */
TEST(JsonFormatStu3Test, ImplementationGuidePrint) {
  TestPrint<ImplementationGuide>("ImplementationGuide-example");
}

/** Test printing of the Library FHIR resource. */
TEST(JsonFormatStu3Test, LibraryPrint) {
  TestPrint<Library>("Library-library-cms146-example");
  TestPrint<Library>("Library-composition-example");
  TestPrint<Library>("Library-example");
  TestPrint<Library>("Library-library-fhir-helpers-predecessor");
}

/** Test printing of the Linkage FHIR resource. */
TEST(JsonFormatStu3Test, LinkagePrint) {
  TestPrint<Linkage>("Linkage-example");
}

/** Test printing of the List FHIR resource. */
TEST(JsonFormatStu3Test, ListPrint) {
  TestPrint<List>("List-example");
  TestPrint<List>("List-current-allergies");
  TestPrint<List>("List-example-double-cousin-relationship");
  TestPrint<List>("List-example-empty");
  TestPrint<List>("List-f201");
  TestPrint<List>("List-genetic");
  TestPrint<List>("List-prognosis");
  TestPrint<List>("List-med-list");
  TestPrint<List>("List-example-simple-empty");
}

/** Test printing of the Location FHIR resource. */
TEST(JsonFormatStu3Test, LocationPrint) {
  TestPrint<Location>("Location-1");
  TestPrint<Location>("Location-amb");
  TestPrint<Location>("Location-hl7");
  TestPrint<Location>("Location-ph");
  TestPrint<Location>("Location-2");
  TestPrint<Location>("Location-ukp");
}

/** Test printing of the Measure FHIR resource. */
TEST(JsonFormatStu3Test, MeasurePrint) {
  TestPrint<Measure>("Measure-measure-cms146-example");
  TestPrint<Measure>("Measure-component-a-example");
  TestPrint<Measure>("Measure-component-b-example");
  TestPrint<Measure>("Measure-composite-example");
  TestPrint<Measure>("Measure-measure-predecessor-example");
}

/** Test printing of the MeasureReport FHIR resource. */
TEST(JsonFormatStu3Test, MeasureReportPrint) {
  TestPrint<MeasureReport>("MeasureReport-measurereport-cms146-cat1-example");
  TestPrint<MeasureReport>("MeasureReport-measurereport-cms146-cat2-example");
  TestPrint<MeasureReport>("MeasureReport-measurereport-cms146-cat3-example");
}

/** Test printing of the Media FHIR resource. */
TEST(JsonFormatStu3Test, MediaPrint) {
  TestPrint<Media>("Media-example");
  TestPrint<Media>(
      "Media-1.2.840.11361907579238403408700.3.0.14.19970327150033");
  TestPrint<Media>("Media-sound");
  TestPrint<Media>("Media-xray");
}

/** Test printing of the Medication FHIR resource. */
TEST(JsonFormatStu3Test, MedicationPrint) {
  TestPrint<Medication>("Medication-med0301");
  TestPrint<Medication>("Medication-med0302");
  TestPrint<Medication>("Medication-med0303");
  TestPrint<Medication>("Medication-med0304");
  TestPrint<Medication>("Medication-med0305");
  TestPrint<Medication>("Medication-med0306");
  TestPrint<Medication>("Medication-med0307");
  TestPrint<Medication>("Medication-med0308");
  TestPrint<Medication>("Medication-med0309");
  TestPrint<Medication>("Medication-med0310");
  TestPrint<Medication>("Medication-med0311");
  TestPrint<Medication>("Medication-med0312");
  TestPrint<Medication>("Medication-med0313");
  TestPrint<Medication>("Medication-med0314");
  TestPrint<Medication>("Medication-med0315");
  TestPrint<Medication>("Medication-med0316");
  TestPrint<Medication>("Medication-med0317");
  TestPrint<Medication>("Medication-med0318");
  TestPrint<Medication>("Medication-med0319");
  TestPrint<Medication>("Medication-med0320");
  TestPrint<Medication>("Medication-med0321");
  TestPrint<Medication>("Medication-medicationexample1");
  TestPrint<Medication>("Medication-medexample015");
}

/** Test printing of the MedicationAdministration FHIR resource. */
TEST(JsonFormatStu3Test, MedicationAdministrationPrint) {
  TestPrint<MedicationAdministration>(
      "MedicationAdministration-medadminexample03");
}

/** Test printing of the MedicationDispense FHIR resource. */
TEST(JsonFormatStu3Test, MedicationDispensePrint) {
  TestPrint<MedicationDispense>("MedicationDispense-meddisp008");
}

/** Test printing of the MedicationRequest FHIR resource. */
TEST(JsonFormatStu3Test, MedicationRequestPrint) {
  TestPrint<MedicationRequest>("MedicationRequest-medrx0311");
  TestPrint<MedicationRequest>("MedicationRequest-medrx002");
}

/** Test printing of the MedicationStatement FHIR resource. */
TEST(JsonFormatTest, MedicationStatementPrint) {
  TestPrint<MedicationStatement>("MedicationStatement-example001");
  TestPrint<MedicationStatement>("MedicationStatement-example002");
  TestPrint<MedicationStatement>("MedicationStatement-example003");
  TestPrint<MedicationStatement>("MedicationStatement-example004");
  TestPrint<MedicationStatement>("MedicationStatement-example005");
  TestPrint<MedicationStatement>("MedicationStatement-example006");
  TestPrint<MedicationStatement>("MedicationStatement-example007");
}

/** Test printing of the MessageDefinition FHIR resource. */
TEST(JsonFormatTest, MessageDefinitionPrint) {
  TestPrint<MessageDefinition>("MessageDefinition-example");
}

/** Test printing of the MessageHeader FHIR resource. */
TEST(JsonFormatTest, MessageHeaderPrint) {
  TestPrint<MessageHeader>(
      "MessageHeader-1cbdfb97-5859-48a4-8301-d54eab818d68");
}

/** Test printing of the NamingSystem FHIR resource. */
TEST(JsonFormatTest, NamingSystemPrint) {
  TestPrint<NamingSystem>("NamingSystem-example");
  TestPrint<NamingSystem>("NamingSystem-example-id");
  TestPrint<NamingSystem>("NamingSystem-example-replaced");
}

/** Test printing of the NutritionOrder FHIR resource. */
TEST(JsonFormatTest, NutritionOrderPrint) {
  TestPrint<NutritionOrder>("NutritionOrder-cardiacdiet");
  TestPrint<NutritionOrder>("NutritionOrder-diabeticdiet");
  TestPrint<NutritionOrder>("NutritionOrder-diabeticsupplement");
  TestPrint<NutritionOrder>("NutritionOrder-energysupplement");
  TestPrint<NutritionOrder>("NutritionOrder-enteralbolus");
  TestPrint<NutritionOrder>("NutritionOrder-enteralcontinuous");
  TestPrint<NutritionOrder>("NutritionOrder-fiberrestricteddiet");
  TestPrint<NutritionOrder>("NutritionOrder-infantenteral");
  TestPrint<NutritionOrder>("NutritionOrder-proteinsupplement");
  TestPrint<NutritionOrder>("NutritionOrder-pureeddiet");
  TestPrint<NutritionOrder>("NutritionOrder-pureeddiet-simple");
  TestPrint<NutritionOrder>("NutritionOrder-renaldiet");
  TestPrint<NutritionOrder>("NutritionOrder-texturemodified");
}

/** Test printing of the Observation FHIR resource. */
TEST(JsonFormatTest, ObservationPrint) {
  TestPrint<Observation>("Observation-example");
  TestPrint<Observation>("Observation-10minute-apgar-score");
  TestPrint<Observation>("Observation-1minute-apgar-score");
  TestPrint<Observation>("Observation-20minute-apgar-score");
  TestPrint<Observation>("Observation-2minute-apgar-score");
  TestPrint<Observation>("Observation-5minute-apgar-score");
  TestPrint<Observation>("Observation-blood-pressure");
  TestPrint<Observation>("Observation-blood-pressure-cancel");
  TestPrint<Observation>("Observation-blood-pressure-dar");
  TestPrint<Observation>("Observation-bmd");
  TestPrint<Observation>("Observation-bmi");
  TestPrint<Observation>("Observation-body-height");
  TestPrint<Observation>("Observation-body-length");
  TestPrint<Observation>("Observation-body-temperature");
  TestPrint<Observation>("Observation-date-lastmp");
  TestPrint<Observation>("Observation-example-diplotype1");
  TestPrint<Observation>("Observation-eye-color");
  TestPrint<Observation>("Observation-f001");
  TestPrint<Observation>("Observation-f002");
  TestPrint<Observation>("Observation-f003");
  TestPrint<Observation>("Observation-f004");
  TestPrint<Observation>("Observation-f005");
  TestPrint<Observation>("Observation-f202");
  TestPrint<Observation>("Observation-f203");
  TestPrint<Observation>("Observation-f204");
  TestPrint<Observation>("Observation-f205");
  TestPrint<Observation>("Observation-f206");
  TestPrint<Observation>("Observation-example-genetics-1");
  TestPrint<Observation>("Observation-example-genetics-2");
  TestPrint<Observation>("Observation-example-genetics-3");
  TestPrint<Observation>("Observation-example-genetics-4");
  TestPrint<Observation>("Observation-example-genetics-5");
  TestPrint<Observation>("Observation-glasgow");
  TestPrint<Observation>("Observation-gcs-qa");
  TestPrint<Observation>("Observation-example-haplotype1");
  TestPrint<Observation>("Observation-example-haplotype2");
  TestPrint<Observation>("Observation-head-circumference");
  TestPrint<Observation>("Observation-heart-rate");
  TestPrint<Observation>("Observation-mbp");
  TestPrint<Observation>("Observation-example-phenotype");
  TestPrint<Observation>("Observation-respiratory-rate");
  TestPrint<Observation>("Observation-ekg");
  TestPrint<Observation>("Observation-satO2");
  TestPrint<Observation>("Observation-example-TPMT-diplotype");
  TestPrint<Observation>("Observation-example-TPMT-haplotype-one");
  TestPrint<Observation>("Observation-example-TPMT-haplotype-two");
  TestPrint<Observation>("Observation-unsat");
  TestPrint<Observation>("Observation-vitals-panel");
}

/** Test printing of the OperationDefinition FHIR resource. */
TEST(JsonFormatTest, OperationDefinitionPrint) {
  TestPrint<OperationDefinition>("OperationDefinition-example");
}

/** Test printing of the OperationOutcome FHIR resource. */
TEST(JsonFormatTest, OperationOutcomePrint) {
  TestPrint<OperationOutcome>("OperationOutcome-101");
  TestPrint<OperationOutcome>("OperationOutcome-allok");
  TestPrint<OperationOutcome>("OperationOutcome-break-the-glass");
  TestPrint<OperationOutcome>("OperationOutcome-exception");
  TestPrint<OperationOutcome>("OperationOutcome-searchfail");
  TestPrint<OperationOutcome>("OperationOutcome-validationfail");
}

/** Test printing of the Organization FHIR resource. */
TEST(JsonFormatTest, OrganizationPrint) {
  TestPrint<Organization>("Organization-hl7");
  TestPrint<Organization>("Organization-f001");
  TestPrint<Organization>("Organization-f002");
  TestPrint<Organization>("Organization-f003");
  TestPrint<Organization>("Organization-f201");
  TestPrint<Organization>("Organization-f203");
  TestPrint<Organization>("Organization-1");
  TestPrint<Organization>("Organization-2.16.840.1.113883.19.5");
  TestPrint<Organization>("Organization-2");
  TestPrint<Organization>("Organization-1832473e-2fe0-452d-abe9-3cdb9879522f");
  TestPrint<Organization>("Organization-mmanu");
}

/** Test printing of the Parameters FHIR resource. */
TEST(JsonFormatTest, ParametersPrint) {
  TestPrint<Parameters>("Parameters-example");
}

/** Test printing of the Patient FHIR resource. */
TEST(JsonFormatTest, PatientPrint) {
  TestPrint<Patient>("patient-example");
  TestPrint<Patient>("patient-example-a");
  TestPrint<Patient>("patient-example-animal");
  TestPrint<Patient>("patient-example-b");
  TestPrint<Patient>("patient-example-c");
  TestPrint<Patient>("patient-example-chinese");
  TestPrint<Patient>("patient-example-d");
  TestPrint<Patient>("patient-example-dicom");
  TestPrint<Patient>("patient-example-f001-pieter");
  TestPrint<Patient>("patient-example-f201-roel");
  TestPrint<Patient>("patient-example-ihe-pcd");
  TestPrint<Patient>("patient-example-proband");
  TestPrint<Patient>("patient-example-xcda");
  TestPrint<Patient>("patient-example-xds");
  TestPrint<Patient>("patient-genetics-example1");
  TestPrint<Patient>("patient-glossy-example");
}

/** Test printing of the PaymentNotice FHIR resource. */
TEST(JsonFormatTest, PaymentNoticePrint) {
  TestPrint<PaymentNotice>("PaymentNotice-77654");
}

/** Test printing of the PaymentReconciliation FHIR resource. */
TEST(JsonFormatTest, PaymentReconciliationPrint) {
  TestPrint<PaymentReconciliation>("PaymentReconciliation-ER2500");
}

/** Test printing of the Person FHIR resource. */
TEST(JsonFormatTest, PersonPrint) {
  TestPrint<Person>("Person-example");
  TestPrint<Person>("Person-f002");
}

/** Test printing of the PlanDefinition FHIR resource. */
TEST(JsonFormatTest, PlanDefinitionPrint) {
  TestPrint<PlanDefinition>("PlanDefinition-low-suicide-risk-order-set");
  TestPrint<PlanDefinition>("PlanDefinition-KDN5");
  TestPrint<PlanDefinition>("PlanDefinition-options-example");
  TestPrint<PlanDefinition>("PlanDefinition-zika-virus-intervention-initial");
  TestPrint<PlanDefinition>("PlanDefinition-protocol-example");
}

/** Test printing of the Practitioner FHIR resource. */
TEST(JsonFormatTest, PractitionerPrint) {
  TestPrint<Practitioner>("Practitioner-example");
  TestPrint<Practitioner>("Practitioner-f001");
  TestPrint<Practitioner>("Practitioner-f002");
  TestPrint<Practitioner>("Practitioner-f003");
  TestPrint<Practitioner>("Practitioner-f004");
  TestPrint<Practitioner>("Practitioner-f005");
  TestPrint<Practitioner>("Practitioner-f006");
  TestPrint<Practitioner>("Practitioner-f007");
  TestPrint<Practitioner>("Practitioner-f201");
  TestPrint<Practitioner>("Practitioner-f202");
  TestPrint<Practitioner>("Practitioner-f203");
  TestPrint<Practitioner>("Practitioner-f204");
  TestPrint<Practitioner>("Practitioner-xcda1");
  TestPrint<Practitioner>("Practitioner-xcda-author");
}

/** Test printing of the PractitionerRole FHIR resource. */
TEST(JsonFormatTest, PractitionerRolePrint) {
  TestPrint<PractitionerRole>("PractitionerRole-example");
}

/** Test printing of the Procedure FHIR resource. */
TEST(JsonFormatTest, ProcedurePrint) {
  TestPrint<Procedure>("Procedure-example");
  TestPrint<Procedure>("Procedure-ambulation");
  TestPrint<Procedure>("Procedure-appendectomy-narrative");
  TestPrint<Procedure>("Procedure-biopsy");
  TestPrint<Procedure>("Procedure-colon-biopsy");
  TestPrint<Procedure>("Procedure-colonoscopy");
  TestPrint<Procedure>("Procedure-education");
  TestPrint<Procedure>("Procedure-f001");
  TestPrint<Procedure>("Procedure-f002");
  TestPrint<Procedure>("Procedure-f003");
  TestPrint<Procedure>("Procedure-f004");
  TestPrint<Procedure>("Procedure-f201");
  TestPrint<Procedure>("Procedure-example-implant");
  TestPrint<Procedure>("Procedure-ob");
  TestPrint<Procedure>("Procedure-physical-therapy");
}

/** Test printing of the ProcedureRequest FHIR resource. */
TEST(JsonFormatTest, ProcedureRequestPrint) {
  TestPrint<ProcedureRequest>("ProcedureRequest-example");
  TestPrint<ProcedureRequest>("ProcedureRequest-physiotherapy");
  TestPrint<ProcedureRequest>("ProcedureRequest-do-not-turn");
  TestPrint<ProcedureRequest>("ProcedureRequest-benchpress");
  TestPrint<ProcedureRequest>("ProcedureRequest-ambulation");
  TestPrint<ProcedureRequest>("ProcedureRequest-appendectomy-narrative");
  TestPrint<ProcedureRequest>("ProcedureRequest-colonoscopy");
  TestPrint<ProcedureRequest>("ProcedureRequest-colon-biopsy");
  TestPrint<ProcedureRequest>("ProcedureRequest-di");
  TestPrint<ProcedureRequest>("ProcedureRequest-education");
  TestPrint<ProcedureRequest>("ProcedureRequest-ft4");
  TestPrint<ProcedureRequest>("ProcedureRequest-example-implant");
  TestPrint<ProcedureRequest>("ProcedureRequest-lipid");
  TestPrint<ProcedureRequest>("ProcedureRequest-ob");
  TestPrint<ProcedureRequest>("ProcedureRequest-example-pgx");
  TestPrint<ProcedureRequest>("ProcedureRequest-physical-therapy");
  TestPrint<ProcedureRequest>("ProcedureRequest-subrequest");
  TestPrint<ProcedureRequest>("ProcedureRequest-og-example1");
}

/** Test printing of the ProcessRequest FHIR resource. */
TEST(JsonFormatTest, ProcessRequestPrint) {
  TestPrint<ProcessRequest>("ProcessRequest-1110");
  TestPrint<ProcessRequest>("ProcessRequest-1115");
  TestPrint<ProcessRequest>("ProcessRequest-1113");
  TestPrint<ProcessRequest>("ProcessRequest-1112");
  TestPrint<ProcessRequest>("ProcessRequest-1114");
  TestPrint<ProcessRequest>("ProcessRequest-1111");
  TestPrint<ProcessRequest>("ProcessRequest-44654");
  TestPrint<ProcessRequest>("ProcessRequest-87654");
  TestPrint<ProcessRequest>("ProcessRequest-87655");
}

/** Test printing of the ProcessResponse FHIR resource. */
TEST(JsonFormatTest, ProcessResponsePrint) {
  TestPrint<ProcessResponse>("ProcessResponse-SR2500");
  TestPrint<ProcessResponse>("ProcessResponse-SR2349");
  TestPrint<ProcessResponse>("ProcessResponse-SR2499");
}

/** Test printing of the Provenance FHIR resource. */
TEST(JsonFormatTest, ProvenancePrint) {
  TestPrint<Provenance>("Provenance-example");
  TestPrint<Provenance>("Provenance-example-biocompute-object");
  TestPrint<Provenance>("Provenance-example-cwl");
  TestPrint<Provenance>("Provenance-signature");
}

/** Test printing of the Questionnaire FHIR resource. */
TEST(JsonFormatTest, QuestionnairePrint) {
  TestPrint<Questionnaire>("Questionnaire-3141");
  TestPrint<Questionnaire>("Questionnaire-bb");
  TestPrint<Questionnaire>("Questionnaire-f201");
  TestPrint<Questionnaire>("Questionnaire-gcs");
}

/** Test printing of the QuestionnaireResponse FHIR resource. */
TEST(JsonFormatTest, QuestionnaireResponsePrint) {
  TestPrint<QuestionnaireResponse>("QuestionnaireResponse-3141");
  TestPrint<QuestionnaireResponse>("QuestionnaireResponse-bb");
  TestPrint<QuestionnaireResponse>("QuestionnaireResponse-f201");
  TestPrint<QuestionnaireResponse>("QuestionnaireResponse-gcs");
  TestPrint<QuestionnaireResponse>("QuestionnaireResponse-ussg-fht-answers");
}

/** Test printing of the ReferralRequest FHIR resource. */
TEST(JsonFormatTest, ReferralRequestPrint) {
  TestPrint<ReferralRequest>("ReferralRequest-example");
}

/** Test printing of the RelatedPerson FHIR resource. */
TEST(JsonFormatTest, RelatedPersonPrint) {
  TestPrint<RelatedPerson>("RelatedPerson-benedicte");
  TestPrint<RelatedPerson>("RelatedPerson-f001");
  TestPrint<RelatedPerson>("RelatedPerson-f002");
  TestPrint<RelatedPerson>("RelatedPerson-peter");
}

/** Test printing of the RequestGroup FHIR resource. */
TEST(JsonFormatTest, RequestGroupPrint) {
  TestPrint<RequestGroup>("RequestGroup-example");
  TestPrint<RequestGroup>("RequestGroup-kdn5-example");
}

/** Test printing of the ResearchStudy FHIR resource. */
TEST(JsonFormatTest, ResearchStudyPrint) {
  TestPrint<ResearchStudy>("ResearchStudy-example");
}

/** Test printing of the ResearchSubject FHIR resource. */
TEST(JsonFormatTest, ResearchSubjectPrint) {
  TestPrint<ResearchSubject>("ResearchSubject-example");
}

/** Test printing of the RiskAssessment FHIR resource. */
TEST(JsonFormatTest, RiskAssessmentPrint) {
  TestPrint<RiskAssessment>("RiskAssessment-genetic");
  TestPrint<RiskAssessment>("RiskAssessment-cardiac");
  TestPrint<RiskAssessment>("RiskAssessment-population");
  TestPrint<RiskAssessment>("RiskAssessment-prognosis");
}

/** Test printing of the Schedule FHIR resource. */
TEST(JsonFormatTest, SchedulePrint) {
  TestPrint<Schedule>("Schedule-example");
  TestPrint<Schedule>("Schedule-exampleloc1");
  TestPrint<Schedule>("Schedule-exampleloc2");
}

/** Test printing of the SearchParameter FHIR resource. */
TEST(JsonFormatTest, SearchParameterPrint) {
  TestPrint<SearchParameter>("SearchParameter-example");
  TestPrint<SearchParameter>("SearchParameter-example-extension");
  TestPrint<SearchParameter>("SearchParameter-example-reference");
}

/** Test printing of the Sequence FHIR resource. */
TEST(JsonFormatTest, SequencePrint) {
  TestPrint<Sequence>("Sequence-coord-0-base");
  TestPrint<Sequence>("Sequence-coord-1-base");
  TestPrint<Sequence>("Sequence-example");
  TestPrint<Sequence>("Sequence-fda-example");
  TestPrint<Sequence>("Sequence-fda-vcf-comparison");
  TestPrint<Sequence>("Sequence-fda-vcfeval-comparison");
  TestPrint<Sequence>("Sequence-example-pgx-1");
  TestPrint<Sequence>("Sequence-example-pgx-2");
  TestPrint<Sequence>("Sequence-example-TPMT-one");
  TestPrint<Sequence>("Sequence-example-TPMT-two");
  TestPrint<Sequence>("Sequence-graphic-example-1");
  TestPrint<Sequence>("Sequence-graphic-example-2");
  TestPrint<Sequence>("Sequence-graphic-example-3");
  TestPrint<Sequence>("Sequence-graphic-example-4");
  TestPrint<Sequence>("Sequence-graphic-example-5");
}

/** Test printing of the ServiceDefinition FHIR resource. */
TEST(JsonFormatTest, ServiceDefinitionPrint) {
  TestPrint<ServiceDefinition>("ServiceDefinition-example");
}

/** Test printing of the Slot FHIR resource. */
TEST(JsonFormatTest, SlotPrint) {
  TestPrint<Slot>("Slot-example");
  TestPrint<Slot>("Slot-1");
  TestPrint<Slot>("Slot-2");
  TestPrint<Slot>("Slot-3");
}

/** Test printing of the Specimen FHIR resource. */
TEST(JsonFormatTest, SpecimenPrint) {
  TestPrint<Specimen>("Specimen-101");
  TestPrint<Specimen>("Specimen-isolate");
  TestPrint<Specimen>("Specimen-sst");
  TestPrint<Specimen>("Specimen-vma-urine");
}

/** Test printing of the StructureDefinition FHIR resource. */
TEST(JsonFormatTest, StructureDefinitionPrint) {
  TestPrint<StructureDefinition>("StructureDefinition-example");
}

/** Test printing of the StructureMap FHIR resource. */
TEST(JsonFormatTest, StructureMapPrint) {
  TestPrint<StructureMap>("StructureMap-example");
}

/** Test printing of the Subscription FHIR resource. */
TEST(JsonFormatTest, SubscriptionPrint) {
  TestPrint<Subscription>("Subscription-example");
  TestPrint<Subscription>("Subscription-example-error");
}

/** Test printing of the Substance FHIR resource. */
TEST(JsonFormatTest, SubstancePrint) {
  TestPrint<Substance>("Substance-example");
  TestPrint<Substance>("Substance-f205");
  TestPrint<Substance>("Substance-f201");
  TestPrint<Substance>("Substance-f202");
  TestPrint<Substance>("Substance-f203");
  TestPrint<Substance>("Substance-f204");
}

/** Test printing of the SupplyDelivery FHIR resource. */
TEST(JsonFormatTest, SupplyDeliveryPrint) {
  TestPrint<SupplyDelivery>("SupplyDelivery-simpledelivery");
  TestPrint<SupplyDelivery>("SupplyDelivery-pumpdelivery");
}

/** Test printing of the SupplyRequest FHIR resource. */
TEST(JsonFormatTest, SupplyRequestPrint) {
  TestPrint<SupplyRequest>("SupplyRequest-simpleorder");
}

/** Test printing of the Task FHIR resource. */
TEST(JsonFormatTest, TaskPrint) {
  TestPrint<Task>("Task-example1");
  TestPrint<Task>("Task-example2");
  TestPrint<Task>("Task-example3");
  TestPrint<Task>("Task-example4");
  TestPrint<Task>("Task-example5");
  TestPrint<Task>("Task-example6");
}

/** Test printing of the TestReport FHIR resource. */
TEST(JsonFormatTest, TestReportPrint) {
  TestPrint<TestReport>("TestReport-testreport-example");
}

/** Test printing of the TestScript FHIR resource. */
TEST(JsonFormatTest, TestScriptPrint) {
  TestPrint<TestScript>("TestScript-testscript-example");
  TestPrint<TestScript>("TestScript-testscript-example-history");
  TestPrint<TestScript>("TestScript-testscript-example-multisystem");
  TestPrint<TestScript>("TestScript-testscript-example-readtest");
  TestPrint<TestScript>("TestScript-testscript-example-rule");
  TestPrint<TestScript>("TestScript-testscript-example-search");
  TestPrint<TestScript>("TestScript-testscript-example-update");
}

/** Test printing of the ValueSet FHIR resource. */
TEST(JsonFormatTest, ValueSetPrint) {
  TestPrint<ValueSet>("valueset-example");
  TestPrint<ValueSet>("valueset-example-expansion");
  TestPrint<ValueSet>("valueset-example-inactive");
  TestPrint<ValueSet>("valueset-example-intensional");
  TestPrint<ValueSet>("valueset-example-yesnodontknow");
  TestPrint<ValueSet>("valueset-list-example-codes");
}

/** Test printing of the VisionPrescription FHIR resource. */
TEST(JsonFormatTest, VisionPrescriptionPrint) {
  TestPrint<VisionPrescription>("VisionPrescription-33123");
  TestPrint<VisionPrescription>("VisionPrescription-33124");
}

}  // namespace

}  // namespace fhir
}  // namespace google
