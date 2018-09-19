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

#include "google/fhir/stu3/json_format.h"

#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/stu3/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/stu3/resources.pb.h"
#include "include/json/json.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;

namespace {

using namespace google::fhir::stu3::proto;  // NOLINT

using google::fhir::stu3::proto::Condition;

static const char* const kTimeZoneString = "Australia/Sydney";

/** Read the specifed json file from the testdata directory as a String. */
string LoadJson(const string& filename) {
  return ReadFile(absl::StrCat("examples/", filename));
}

template <typename R>
R LoadProto(const string& filename) {
  return ReadProto<R>(absl::StrCat("examples/", filename));
}

template <typename R>
void TestParse(const string& name) {
  string json = LoadJson(name + ".json");
  absl::TimeZone tz;
  absl::LoadTimeZone(kTimeZoneString, &tz);
  R from_json = JsonFhirStringToProto<R>(json, tz).ValueOrDie();
  R from_disk = LoadProto<R>(name + ".prototxt");

  ::google::protobuf::util::MessageDifferencer differencer;
  string differences;
  differencer.ReportDifferencesToString(&differences);
  ASSERT_TRUE(differencer.Compare(from_json, from_disk)) << differences;
}

Json::Value ParseJsonStringToValue(const string& raw_json) {
  Json::Reader reader;
  Json::Value value;
  reader.parse(raw_json, value);
  return value;
}

template <typename R>
void TestPrint(const string& name) {
  const R proto = LoadProto<R>(name + ".prototxt");
  absl::TimeZone tz;
  absl::LoadTimeZone(kTimeZoneString, &tz);
  string from_proto = PrettyPrintFhirToJsonString(proto, tz).ValueOrDie();
  string from_json = LoadJson(name + ".json");

  if (ParseJsonStringToValue(from_proto) != ParseJsonStringToValue(from_json)) {
    // This assert will fail, but we get terrible diff messages comparing
    // JsonCPP, so fall back to string diffs.
    ASSERT_EQ(from_proto, from_json);
  }
}

/* Edge Case Testing */

/** Test parsing of FHIR edge cases. */
TEST(JsonFormatTest, EdgeCasesParse) { TestParse<Patient>("json-edge-cases"); }

/** Test printing of FHIR edge cases. */
TEST(JsonFormatTest, EdgeCasesPrint) {
  TestPrint<Patient>("json-edge-cases");
}

/* Resource tests start here. */

/** Test parsing of the Account FHIR resource. */
TEST(JsonFormatTest, AccountParse) {
  TestParse<Account>("account-example");
  TestParse<Account>("account-example-with-guarantor");
}

/** Test parsing of the ActivityDefinition FHIR resource. */
TEST(JsonFormatTest, ActivityDefinitionParse) {
  TestParse<ActivityDefinition>("activitydefinition-example");
  TestParse<ActivityDefinition>("activitydefinition-medicationorder-example");
  TestParse<ActivityDefinition>("activitydefinition-predecessor-example");
  TestParse<ActivityDefinition>("activitydefinition-procedurerequest-example");
  TestParse<ActivityDefinition>("activitydefinition-supplyrequest-example");
}

/** Test parsing of the AdverseEvent FHIR resource. */
TEST(JsonFormatTest, AdverseEventParse) {
  TestParse<AdverseEvent>("adverseevent-example");
}

/** Test parsing of the AllergyIntolerance FHIR resource. */
TEST(JsonFormatTest, AllergyIntoleranceParse) {
  TestParse<AllergyIntolerance>("allergyintolerance-example");
}

/** Test parsing of the Appointment FHIR resource. */
TEST(JsonFormatTest, AppointmentParse) {
  TestParse<Appointment>("appointment-example");
  TestParse<Appointment>("appointment-example2doctors");
  TestParse<Appointment>("appointment-example-request");
}

/** Test parsing of the AppointmentResponse FHIR resource. */
TEST(JsonFormatTest, AppointmentResponseParse) {
  TestParse<AppointmentResponse>("appointmentresponse-example");
  TestParse<AppointmentResponse>("appointmentresponse-example-req");
}

/** Test parsing of the AuditEvent FHIR resource. */
TEST(JsonFormatTest, AuditEventParse) {
  TestParse<AuditEvent>("auditevent-example");
  TestParse<AuditEvent>("auditevent-example-disclosure");
  TestParse<AuditEvent>("audit-event-example-login");
  TestParse<AuditEvent>("audit-event-example-logout");
  TestParse<AuditEvent>("audit-event-example-media");
  TestParse<AuditEvent>("audit-event-example-pixQuery");
  TestParse<AuditEvent>("audit-event-example-search");
  TestParse<AuditEvent>("audit-event-example-vread");
}

/** Test parsing of the Basic FHIR resource. */
TEST(JsonFormatTest, BasicParse) {
  TestParse<Basic>("basic-example");
  TestParse<Basic>("basic-example2");
  TestParse<Basic>("basic-example-narrative");
}

/** Test parsing of the Binary FHIR resource. */
TEST(JsonFormatTest, BinaryParse) { TestParse<Binary>("binary-example"); }

/** Test parsing of the BodySite FHIR resource. */
TEST(JsonFormatTest, BodySiteParse) {
  TestParse<BodySite>("bodysite-example-fetus");
  TestParse<BodySite>("bodysite-example-skin-patch");
  TestParse<BodySite>("bodysite-example-tumor");
}

/** Test parsing of the Bundle FHIR resource. */
TEST(JsonFormatTest, BundleParse) {
  TestParse<Bundle>("bundle-example");
  TestParse<Bundle>("diagnosticreport-examples-general");
  TestParse<Bundle>("diagnosticreport-hla-genetics-results-example");
  TestParse<Bundle>("document-example-dischargesummary");
  TestParse<Bundle>("endpoint-examples-general-template");
  TestParse<Bundle>("location-examples-general");
  TestParse<Bundle>("patient-examples-cypress-template");
  TestParse<Bundle>("patient-examples-general");
  TestParse<Bundle>("practitioner-examples-general");
  TestParse<Bundle>("practitionerrole-examples-general");
  TestParse<Bundle>("questionnaire-profile-example-ussg-fht");
  TestParse<Bundle>("xds-example");
}

/** Test parsing of the CapabilityStatement FHIR resource. */
TEST(JsonFormatTest, CapabilityStatementParse) {
  TestParse<CapabilityStatement>("capabilitystatement-example");
  TestParse<CapabilityStatement>("capabilitystatement-phr-example");
}

/** Test parsing of the CarePlan FHIR resource. */
TEST(JsonFormatTest, CarePlanParse) {
  TestParse<CarePlan>("careplan-example");
  TestParse<CarePlan>("careplan-example-f001-heart");
  TestParse<CarePlan>("careplan-example-f002-lung");
  TestParse<CarePlan>("careplan-example-f003-pharynx");
  TestParse<CarePlan>("careplan-example-f201-renal");
  TestParse<CarePlan>("careplan-example-f202-malignancy");
  TestParse<CarePlan>("careplan-example-f203-sepsis");
  TestParse<CarePlan>("careplan-example-GPVisit");
  TestParse<CarePlan>("careplan-example-integrated");
  TestParse<CarePlan>("careplan-example-obesity-narrative");
  TestParse<CarePlan>("careplan-example-pregnancy");
}

/** Test parsing of the CareTeam FHIR resource. */
TEST(JsonFormatTest, CareTeamParse) { TestParse<CareTeam>("careteam-example"); }

/** Test parsing of the ChargeItem FHIR resource. */
TEST(JsonFormatTest, ChargeItemParse) {
  TestParse<ChargeItem>("chargeitem-example");
}

/** Test parsing of the Claim FHIR resource. */
TEST(JsonFormatTest, ClaimParse) {
  TestParse<Claim>("claim-example");
  TestParse<Claim>("claim-example-institutional");
  TestParse<Claim>("claim-example-institutional-rich");
  TestParse<Claim>("claim-example-oral-average");
  TestParse<Claim>("claim-example-oral-bridge");
  TestParse<Claim>("claim-example-oral-contained");
  TestParse<Claim>("claim-example-oral-contained-identifier");
  TestParse<Claim>("claim-example-oral-identifier");
  TestParse<Claim>("claim-example-oral-orthoplan");
  TestParse<Claim>("claim-example-pharmacy");
  TestParse<Claim>("claim-example-pharmacy-compound");
  TestParse<Claim>("claim-example-pharmacy-medication");
  TestParse<Claim>("claim-example-professional");
  TestParse<Claim>("claim-example-vision");
  TestParse<Claim>("claim-example-vision-glasses");
  TestParse<Claim>("claim-example-vision-glasses-3tier");
}

/** Test parsing of the ClaimResponse FHIR resource. */
TEST(JsonFormatTest, ClaimResponseParse) {
  TestParse<ClaimResponse>("claimresponse-example");
}

/** Test parsing of the ClinicalImpression FHIR resource. */
TEST(JsonFormatTest, ClinicalImpressionParse) {
  TestParse<ClinicalImpression>("clinicalimpression-example");
}

/** Test parsing of the CodeSystem FHIR resource. */
TEST(JsonFormatTest, CodeSystemParse) {
  TestParse<CodeSystem>("codesystem-example");
  TestParse<CodeSystem>("codesystem-example-summary");
  TestParse<CodeSystem>("codesystem-list-example-codes");
}

/** Test parsing of the Communication FHIR resource. */
TEST(JsonFormatTest, CommunicationParse) {
  TestParse<Communication>("communication-example");
  TestParse<Communication>("communication-example-fm-attachment");
  TestParse<Communication>("communication-example-fm-solicited-attachment");
}

/** Test parsing of the CommunicationRequest FHIR resource. */
TEST(JsonFormatTest, CommunicationRequestParse) {
  TestParse<CommunicationRequest>("communicationrequest-example");
  TestParse<CommunicationRequest>(
      "communicationrequest-example-fm-solicit-attachment");
}

/** Test parsing of the CompartmentDefinition FHIR resource. */
TEST(JsonFormatTest, CompartmentDefinitionParse) {
  TestParse<CompartmentDefinition>("compartmentdefinition-example");
}

/** Test parsing of the Composition FHIR resource. */
TEST(JsonFormatTest, CompositionParse) {
  TestParse<Composition>("composition-example");
}

/** Test parsing of the ConceptMap FHIR resource. */
TEST(JsonFormatTest, ConceptMapParse) {
  TestParse<ConceptMap>("conceptmap-example");
  TestParse<ConceptMap>("conceptmap-example-2");
  TestParse<ConceptMap>("conceptmap-example-specimen-type");
}

/** Test parsing of the Condition FHIR resource. */
TEST(JsonFormatTest, ConditionParse) {
  TestParse<Condition>("condition-example");
  TestParse<Condition>("condition-example2");
  TestParse<Condition>("condition-example-f001-heart");
  TestParse<Condition>("condition-example-f002-lung");
  TestParse<Condition>("condition-example-f003-abscess");
  TestParse<Condition>("condition-example-f201-fever");
  TestParse<Condition>("condition-example-f202-malignancy");
  TestParse<Condition>("condition-example-f203-sepsis");
  TestParse<Condition>("condition-example-f204-renal");
  TestParse<Condition>("condition-example-f205-infection");
  TestParse<Condition>("condition-example-family-history");
  TestParse<Condition>("condition-example-stroke");
}

/** Test parsing of the Consent FHIR resource. */
TEST(JsonFormatTest, ConsentParse) {
  TestParse<Consent>("consent-example");
  TestParse<Consent>("consent-example-Emergency");
  TestParse<Consent>("consent-example-grantor");
  TestParse<Consent>("consent-example-notAuthor");
  TestParse<Consent>("consent-example-notOrg");
  TestParse<Consent>("consent-example-notThem");
  TestParse<Consent>("consent-example-notThis");
  TestParse<Consent>("consent-example-notTime");
  TestParse<Consent>("consent-example-Out");
  TestParse<Consent>("consent-example-pkb");
  TestParse<Consent>("consent-example-signature");
  TestParse<Consent>("consent-example-smartonfhir");
}

/** Test parsing of the Contract FHIR resource. */
TEST(JsonFormatTest, ContractParse) {
  TestParse<Contract>("contract-example");
  TestParse<Contract>("contract-example-42cfr-part2");
  TestParse<Contract>("pcd-example-notAuthor");
  TestParse<Contract>("pcd-example-notLabs");
  TestParse<Contract>("pcd-example-notOrg");
  TestParse<Contract>("pcd-example-notThem");
  TestParse<Contract>("pcd-example-notThis");
}

/** Test parsing of the Coverage FHIR resource. */
TEST(JsonFormatTest, CoverageParse) {
  TestParse<Coverage>("coverage-example");
  TestParse<Coverage>("coverage-example-2");
  TestParse<Coverage>("coverage-example-ehic");
  TestParse<Coverage>("coverage-example-selfpay");
}

/** Test parsing of the DataElement FHIR resource. */
TEST(JsonFormatTest, DataElementParse) {
  TestParse<DataElement>("dataelement-example");
  TestParse<DataElement>("dataelement-labtestmaster-example");
}

/** Test parsing of the DetectedIssue FHIR resource. */
TEST(JsonFormatTest, DetectedIssueParse) {
  TestParse<DetectedIssue>("detectedissue-example");
  TestParse<DetectedIssue>("detectedissue-example-allergy");
  TestParse<DetectedIssue>("detectedissue-example-dup");
  TestParse<DetectedIssue>("detectedissue-example-lab");
}

/** Test parsing of the Device FHIR resource. */
TEST(JsonFormatTest, DeviceParse) {
  TestParse<Device>("device-example");
  TestParse<Device>("device-example-f001-feedingtube");
  TestParse<Device>("device-example-ihe-pcd");
  TestParse<Device>("device-example-pacemaker");
  TestParse<Device>("device-example-software");
  TestParse<Device>("device-example-udi1");
  TestParse<Device>("device-example-udi2");
  TestParse<Device>("device-example-udi3");
  TestParse<Device>("device-example-udi4");
}

/** Test parsing of the DeviceComponent FHIR resource. */
TEST(JsonFormatTest, DeviceComponentParse) {
  TestParse<DeviceComponent>("devicecomponent-example");
  TestParse<DeviceComponent>("devicecomponent-example-prodspec");
}

/** Test parsing of the DeviceMetric FHIR resource. */
TEST(JsonFormatTest, DeviceMetricParse) {
  TestParse<DeviceMetric>("devicemetric-example");
}

/** Test parsing of the DeviceRequest FHIR resource. */
TEST(JsonFormatTest, DeviceRequestParse) {
  TestParse<DeviceRequest>("devicerequest-example");
  TestParse<DeviceRequest>("devicerequest-example-insulinpump");
}

/** Test parsing of the DeviceUseStatement FHIR resource. */
TEST(JsonFormatTest, DeviceUseStatementParse) {
  TestParse<DeviceUseStatement>("deviceusestatement-example");
}

/** Test parsing of the DiagnosticReport FHIR resource. */
TEST(JsonFormatTest, DiagnosticReportParse) {
  TestParse<DiagnosticReport>("diagnosticreport-example");
  TestParse<DiagnosticReport>("diagnosticreport-example-dxa");
  TestParse<DiagnosticReport>("diagnosticreport-example-f001-bloodexam");
  TestParse<DiagnosticReport>("diagnosticreport-example-f201-brainct");
  TestParse<DiagnosticReport>("diagnosticreport-example-f202-bloodculture");
  TestParse<DiagnosticReport>("diagnosticreport-example-ghp");
  TestParse<DiagnosticReport>("diagnosticreport-example-gingival-mass");
  TestParse<DiagnosticReport>("diagnosticreport-example-lipids");
  TestParse<DiagnosticReport>("diagnosticreport-example-papsmear");
  TestParse<DiagnosticReport>("diagnosticreport-example-pgx");
  TestParse<DiagnosticReport>("diagnosticreport-example-ultrasound");
  TestParse<DiagnosticReport>(
      "diagnosticreport-genetics-example-2-familyhistory");
}

/** Test parsing of the DocumentManifest FHIR resource. */
TEST(JsonFormatTest, DocumentManifestParse) {
  TestParse<DocumentManifest>("documentmanifest-example");
}

/** Test parsing of the DocumentReference FHIR resource. */
TEST(JsonFormatTest, DocumentReferenceParse) {
  TestParse<DocumentReference>("documentreference-example");
}

/** Test parsing of the EligibilityRequest FHIR resource. */
TEST(JsonFormatTest, EligibilityRequestParse) {
  TestParse<EligibilityRequest>("eligibilityrequest-example");
  TestParse<EligibilityRequest>("eligibilityrequest-example-2");
}

/** Test parsing of the EligibilityResponse FHIR resource. */
TEST(JsonFormatTest, EligibilityResponseParse) {
  TestParse<EligibilityResponse>("eligibilityresponse-example");
  TestParse<EligibilityResponse>("eligibilityresponse-example-benefits");
  TestParse<EligibilityResponse>("eligibilityresponse-example-benefits-2");
  TestParse<EligibilityResponse>("eligibilityresponse-example-error");
}

/** Test parsing of the Encounter FHIR resource. */
TEST(JsonFormatTest, EncounterParse) {
  TestParse<Encounter>("encounter-example");
  TestParse<Encounter>("encounter-example-emerg");
  TestParse<Encounter>("encounter-example-f001-heart");
  TestParse<Encounter>("encounter-example-f002-lung");
  TestParse<Encounter>("encounter-example-f003-abscess");
  TestParse<Encounter>("encounter-example-f201-20130404");
  TestParse<Encounter>("encounter-example-f202-20130128");
  TestParse<Encounter>("encounter-example-f203-20130311");
  TestParse<Encounter>("encounter-example-home");
  TestParse<Encounter>("encounter-example-xcda");
}

/** Test parsing of the Endpoint FHIR resource. */
TEST(JsonFormatTest, EndpointParse) {
  TestParse<Endpoint>("endpoint-example");
  TestParse<Endpoint>("endpoint-example-iid");
  TestParse<Endpoint>("endpoint-example-wadors");
}

/** Test parsing of the EnrollmentRequest FHIR resource. */
TEST(JsonFormatTest, EnrollmentRequestParse) {
  TestParse<EnrollmentRequest>("enrollmentrequest-example");
}

/** Test parsing of the EnrollmentResponse FHIR resource. */
TEST(JsonFormatTest, EnrollmentResponseParse) {
  TestParse<EnrollmentResponse>("enrollmentresponse-example");
}

/** Test parsing of the EpisodeOfCare FHIR resource. */
TEST(JsonFormatTest, EpisodeOfCareParse) {
  TestParse<EpisodeOfCare>("episodeofcare-example");
}

/** Test parsing of the ExpansionProfile FHIR resource. */
TEST(JsonFormatTest, ExpansionProfileParse) {
  TestParse<ExpansionProfile>("expansionprofile-example");
}

/** Test parsing of the ExplanationOfBenefit FHIR resource. */
TEST(JsonFormatTest, ExplanationOfBenefitParse) {
  TestParse<ExplanationOfBenefit>("explanationofbenefit-example");
}

/** Test parsing of the FamilyMemberHistory FHIR resource. */
TEST(JsonFormatTest, FamilyMemberHistoryParse) {
  TestParse<FamilyMemberHistory>("familymemberhistory-example");
  TestParse<FamilyMemberHistory>("familymemberhistory-example-mother");
}

/** Test parsing of the Flag FHIR resource. */
TEST(JsonFormatTest, FlagParse) {
  TestParse<Flag>("flag-example");
  TestParse<Flag>("flag-example-encounter");
}

/** Test parsing of the Goal FHIR resource. */
TEST(JsonFormatTest, GoalParse) {
  TestParse<Goal>("goal-example");
  TestParse<Goal>("goal-example-stop-smoking");
}

/** Test parsing of the GraphDefinition FHIR resource. */
TEST(JsonFormatTest, GraphDefinitionParse) {
  TestParse<GraphDefinition>("graphdefinition-example");
}

/** Test parsing of the Group FHIR resource. */
TEST(JsonFormatTest, GroupParse) {
  TestParse<Group>("group-example");
  TestParse<Group>("group-example-member");
}

/** Test parsing of the GuidanceResponse FHIR resource. */
TEST(JsonFormatTest, GuidanceResponseParse) {
  TestParse<GuidanceResponse>("guidanceresponse-example");
}

/** Test parsing of the HealthcareService FHIR resource. */
TEST(JsonFormatTest, HealthcareServiceParse) {
  TestParse<HealthcareService>("healthcareservice-example");
}

/** Test parsing of the ImagingManifest FHIR resource. */
TEST(JsonFormatTest, ImagingManifestParse) {
  TestParse<ImagingManifest>("imagingmanifest-example");
}

/** Test parsing of the ImagingStudy FHIR resource. */
TEST(JsonFormatTest, ImagingStudyParse) {
  TestParse<ImagingStudy>("imagingstudy-example");
  TestParse<ImagingStudy>("imagingstudy-example-xr");
}

/** Test parsing of the Immunization FHIR resource. */
TEST(JsonFormatTest, ImmunizationParse) {
  TestParse<Immunization>("immunization-example");
  TestParse<Immunization>("immunization-example-historical");
  TestParse<Immunization>("immunization-example-refused");
}

/** Test parsing of the ImmunizationRecommendation FHIR resource. */
TEST(JsonFormatTest, ImmunizationRecommendationParse) {
  TestParse<ImmunizationRecommendation>("immunizationrecommendation-example");
  TestParse<ImmunizationRecommendation>(
      "immunizationrecommendation-target-disease-example");
}

/** Test parsing of the ImplementationGuide FHIR resource. */
TEST(JsonFormatTest, ImplementationGuideParse) {
  TestParse<ImplementationGuide>("implementationguide-example");
}

/** Test parsing of the Library FHIR resource. */
TEST(JsonFormatTest, LibraryParse) {
  TestParse<Library>("library-cms146-example");
  TestParse<Library>("library-composition-example");
  TestParse<Library>("library-example");
  TestParse<Library>("library-predecessor-example");
}

/** Test parsing of the Linkage FHIR resource. */
TEST(JsonFormatTest, LinkageParse) { TestParse<Linkage>("linkage-example"); }

/** Test parsing of the List FHIR resource. */
TEST(JsonFormatTest, ListParse) {
  TestParse<List>("list-example");
  TestParse<List>("list-example-allergies");
  TestParse<List>("list-example-double-cousin-relationship-pedigree");
  TestParse<List>("list-example-empty");
  TestParse<List>("list-example-familyhistory-f201-roel");
  TestParse<List>("list-example-familyhistory-genetics-profile");
  TestParse<List>("list-example-familyhistory-genetics-profile-annie");
  TestParse<List>("list-example-medlist");
  TestParse<List>("list-example-simple-empty");
}

/** Test parsing of the Location FHIR resource. */
TEST(JsonFormatTest, LocationParse) {
  TestParse<Location>("location-example");
  TestParse<Location>("location-example-ambulance");
  TestParse<Location>("location-example-hl7hq");
  TestParse<Location>("location-example-patients-home");
  TestParse<Location>("location-example-room");
  TestParse<Location>("location-example-ukpharmacy");
}

/** Test parsing of the Measure FHIR resource. */
TEST(JsonFormatTest, MeasureParse) {
  TestParse<Measure>("measure-cms146-example");
  TestParse<Measure>("measure-component-a-example");
  TestParse<Measure>("measure-component-b-example");
  TestParse<Measure>("measure-composite-example");
  TestParse<Measure>("measure-predecessor-example");
}

/** Test parsing of the MeasureReport FHIR resource. */
TEST(JsonFormatTest, MeasureReportParse) {
  TestParse<MeasureReport>("measurereport-cms146-cat1-example");
  TestParse<MeasureReport>("measurereport-cms146-cat2-example");
  TestParse<MeasureReport>("measurereport-cms146-cat3-example");
}

/** Test parsing of the Media FHIR resource. */
TEST(JsonFormatTest, MediaParse) {
  TestParse<Media>("media-example");
  TestParse<Media>("media-example-dicom");
  TestParse<Media>("media-example-sound");
  TestParse<Media>("media-example-xray");
}

/** Test parsing of the Medication FHIR resource. */
TEST(JsonFormatTest, MedicationParse) {
  TestParse<Medication>("medicationexample0301");
  TestParse<Medication>("medicationexample0302");
  TestParse<Medication>("medicationexample0303");
  TestParse<Medication>("medicationexample0304");
  TestParse<Medication>("medicationexample0305");
  TestParse<Medication>("medicationexample0306");
  TestParse<Medication>("medicationexample0307");
  TestParse<Medication>("medicationexample0308");
  TestParse<Medication>("medicationexample0309");
  TestParse<Medication>("medicationexample0310");
  TestParse<Medication>("medicationexample0311");
  TestParse<Medication>("medicationexample0312");
  TestParse<Medication>("medicationexample0313");
  TestParse<Medication>("medicationexample0314");
  TestParse<Medication>("medicationexample0315");
  TestParse<Medication>("medicationexample0316");
  TestParse<Medication>("medicationexample0317");
  TestParse<Medication>("medicationexample0318");
  TestParse<Medication>("medicationexample0319");
  TestParse<Medication>("medicationexample0320");
  TestParse<Medication>("medicationexample0321");
  TestParse<Medication>("medicationexample1");
  TestParse<Medication>("medicationexample15");
}

/** Test parsing of the MedicationAdministration FHIR resource. */
TEST(JsonFormatTest, MedicationAdministrationParse) {
  TestParse<MedicationAdministration>("medicationadministrationexample3");
}

/** Test parsing of the MedicationDispense FHIR resource. */
TEST(JsonFormatTest, MedicationDispenseParse) {
  TestParse<MedicationDispense>("medicationdispenseexample8");
}

/** Test parsing of the MedicationRequest FHIR resource. */
TEST(JsonFormatTest, MedicationRequestParse) {
  TestParse<MedicationRequest>("medicationrequestexample1");
  TestParse<MedicationRequest>("medicationrequestexample2");
}

/** Test parsing of the MedicationStatement FHIR resource. */
TEST(JsonFormatTest, MedicationStatementParse) {
  TestParse<MedicationStatement>("medicationstatementexample1");
  TestParse<MedicationStatement>("medicationstatementexample2");
  TestParse<MedicationStatement>("medicationstatementexample3");
  TestParse<MedicationStatement>("medicationstatementexample4");
  TestParse<MedicationStatement>("medicationstatementexample5");
  TestParse<MedicationStatement>("medicationstatementexample6");
  TestParse<MedicationStatement>("medicationstatementexample7");
}

/** Test parsing of the MessageDefinition FHIR resource. */
TEST(JsonFormatTest, MessageDefinitionParse) {
  TestParse<MessageDefinition>("messagedefinition-example");
}

/** Test parsing of the MessageHeader FHIR resource. */
TEST(JsonFormatTest, MessageHeaderParse) {
  TestParse<MessageHeader>("messageheader-example");
}

/** Test parsing of the NamingSystem FHIR resource. */
TEST(JsonFormatTest, NamingSystemParse) {
  TestParse<NamingSystem>("namingsystem-example");
  TestParse<NamingSystem>("namingsystem-example-id");
  TestParse<NamingSystem>("namingsystem-example-replaced");
}

/** Test parsing of the NutritionOrder FHIR resource. */
TEST(JsonFormatTest, NutritionOrderParse) {
  TestParse<NutritionOrder>("nutritionorder-example-cardiacdiet");
  TestParse<NutritionOrder>("nutritionorder-example-diabeticdiet");
  TestParse<NutritionOrder>("nutritionorder-example-diabeticsupplement");
  TestParse<NutritionOrder>("nutritionorder-example-energysupplement");
  TestParse<NutritionOrder>("nutritionorder-example-enteralbolus");
  TestParse<NutritionOrder>("nutritionorder-example-enteralcontinuous");
  TestParse<NutritionOrder>("nutritionorder-example-fiberrestricteddiet");
  TestParse<NutritionOrder>("nutritionorder-example-infantenteral");
  TestParse<NutritionOrder>("nutritionorder-example-proteinsupplement");
  TestParse<NutritionOrder>("nutritionorder-example-pureeddiet");
  TestParse<NutritionOrder>("nutritionorder-example-pureeddiet-simple");
  TestParse<NutritionOrder>("nutritionorder-example-renaldiet");
  TestParse<NutritionOrder>("nutritionorder-example-texture-modified");
}

/** Test parsing of the Observation FHIR resource. */
TEST(JsonFormatTest, ObservationParse) {
  TestParse<Observation>("observation-example");
  TestParse<Observation>("observation-example-10minute-apgar-score");
  TestParse<Observation>("observation-example-1minute-apgar-score");
  TestParse<Observation>("observation-example-20minute-apgar-score");
  TestParse<Observation>("observation-example-2minute-apgar-score");
  TestParse<Observation>("observation-example-5minute-apgar-score");
  TestParse<Observation>("observation-example-bloodpressure");
  TestParse<Observation>("observation-example-bloodpressure-cancel");
  TestParse<Observation>("observation-example-bloodpressure-dar");
  TestParse<Observation>("observation-example-bmd");
  TestParse<Observation>("observation-example-bmi");
  TestParse<Observation>("observation-example-body-height");
  TestParse<Observation>("observation-example-body-length");
  TestParse<Observation>("observation-example-body-temperature");
  TestParse<Observation>("observation-example-date-lastmp");
  TestParse<Observation>("observation-example-diplotype1");
  TestParse<Observation>("observation-example-eye-color");
  TestParse<Observation>("observation-example-f001-glucose");
  TestParse<Observation>("observation-example-f002-excess");
  TestParse<Observation>("observation-example-f003-co2");
  TestParse<Observation>("observation-example-f004-erythrocyte");
  TestParse<Observation>("observation-example-f005-hemoglobin");
  TestParse<Observation>("observation-example-f202-temperature");
  TestParse<Observation>("observation-example-f203-bicarbonate");
  TestParse<Observation>("observation-example-f204-creatinine");
  TestParse<Observation>("observation-example-f205-egfr");
  TestParse<Observation>("observation-example-f206-staphylococcus");
  TestParse<Observation>("observation-example-genetics-1");
  TestParse<Observation>("observation-example-genetics-2");
  TestParse<Observation>("observation-example-genetics-3");
  TestParse<Observation>("observation-example-genetics-4");
  TestParse<Observation>("observation-example-genetics-5");
  TestParse<Observation>("observation-example-glasgow");
  TestParse<Observation>("observation-example-glasgow-qa");
  TestParse<Observation>("observation-example-haplotype1");
  TestParse<Observation>("observation-example-haplotype2");
  TestParse<Observation>("observation-example-head-circumference");
  TestParse<Observation>("observation-example-heart-rate");
  TestParse<Observation>("observation-example-mbp");
  TestParse<Observation>("observation-example-phenotype");
  TestParse<Observation>("observation-example-respiratory-rate");
  TestParse<Observation>("observation-example-sample-data");
  TestParse<Observation>("observation-example-satO2");
  TestParse<Observation>("observation-example-TPMT-diplotype");
  TestParse<Observation>("observation-example-TPMT-haplotype-one");
  TestParse<Observation>("observation-example-TPMT-haplotype-two");
  TestParse<Observation>("observation-example-unsat");
  TestParse<Observation>("observation-example-vitals-panel");
}

/** Test parsing of the OperationDefinition FHIR resource. */
TEST(JsonFormatTest, OperationDefinitionParse) {
  TestParse<OperationDefinition>("operationdefinition-example");
}

/** Test parsing of the OperationOutcome FHIR resource. */
TEST(JsonFormatTest, OperationOutcomeParse) {
  TestParse<OperationOutcome>("operationoutcome-example");
  TestParse<OperationOutcome>("operationoutcome-example-allok");
  TestParse<OperationOutcome>("operationoutcome-example-break-the-glass");
  TestParse<OperationOutcome>("operationoutcome-example-exception");
  TestParse<OperationOutcome>("operationoutcome-example-searchfail");
  TestParse<OperationOutcome>("operationoutcome-example-validationfail");
}

/** Test parsing of the Organization FHIR resource. */
TEST(JsonFormatTest, OrganizationParse) {
  TestParse<Organization>("organization-example");
  TestParse<Organization>("organization-example-f001-burgers");
  TestParse<Organization>("organization-example-f002-burgers-card");
  TestParse<Organization>("organization-example-f003-burgers-ENT");
  TestParse<Organization>("organization-example-f201-aumc");
  TestParse<Organization>("organization-example-f203-bumc");
  TestParse<Organization>("organization-example-gastro");
  TestParse<Organization>("organization-example-good-health-care");
  TestParse<Organization>("organization-example-insurer");
  TestParse<Organization>("organization-example-lab");
  TestParse<Organization>("organization-example-mmanu");
}

/** Test parsing of the Parameters FHIR resource. */
TEST(JsonFormatTest, ParametersParse) {
  TestParse<Parameters>("parameters-example");
}

/** Test parsing of the Patient FHIR resource. */
TEST(JsonFormatTest, PatientParse) {
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
TEST(JsonFormatTest, PaymentNoticeParse) {
  TestParse<PaymentNotice>("paymentnotice-example");
}

/** Test parsing of the PaymentReconciliation FHIR resource. */
TEST(JsonFormatTest, PaymentReconciliationParse) {
  TestParse<PaymentReconciliation>("paymentreconciliation-example");
}

/** Test parsing of the Person FHIR resource. */
TEST(JsonFormatTest, PersonParse) {
  TestParse<Person>("person-example");
  TestParse<Person>("person-example-f002-ariadne");
}

/** Test parsing of the PlanDefinition FHIR resource. */
TEST(JsonFormatTest, PlanDefinitionParse) {
  TestParse<PlanDefinition>("plandefinition-example");
  TestParse<PlanDefinition>("plandefinition-example-kdn5-simplified");
  TestParse<PlanDefinition>("plandefinition-options-example");
  TestParse<PlanDefinition>("plandefinition-predecessor-example");
  TestParse<PlanDefinition>("plandefinition-protocol-example");
}

/** Test parsing of the Practitioner FHIR resource. */
TEST(JsonFormatTest, PractitionerParse) {
  TestParse<Practitioner>("practitioner-example");
  TestParse<Practitioner>("practitioner-example-f001-evdb");
  TestParse<Practitioner>("practitioner-example-f002-pv");
  TestParse<Practitioner>("practitioner-example-f003-mv");
  TestParse<Practitioner>("practitioner-example-f004-rb");
  TestParse<Practitioner>("practitioner-example-f005-al");
  TestParse<Practitioner>("practitioner-example-f006-rvdb");
  TestParse<Practitioner>("practitioner-example-f007-sh");
  TestParse<Practitioner>("practitioner-example-f201-ab");
  TestParse<Practitioner>("practitioner-example-f202-lm");
  TestParse<Practitioner>("practitioner-example-f203-jvg");
  TestParse<Practitioner>("practitioner-example-f204-ce");
  TestParse<Practitioner>("practitioner-example-xcda1");
  TestParse<Practitioner>("practitioner-example-xcda-author");
}

/** Test parsing of the PractitionerRole FHIR resource. */
TEST(JsonFormatTest, PractitionerRoleParse) {
  TestParse<PractitionerRole>("practitionerrole-example");
}

/** Test parsing of the Procedure FHIR resource. */
TEST(JsonFormatTest, ProcedureParse) {
  TestParse<Procedure>("procedure-example");
  TestParse<Procedure>("procedure-example-ambulation");
  TestParse<Procedure>("procedure-example-appendectomy-narrative");
  TestParse<Procedure>("procedure-example-biopsy");
  TestParse<Procedure>("procedure-example-colon-biopsy");
  TestParse<Procedure>("procedure-example-colonoscopy");
  TestParse<Procedure>("procedure-example-education");
  TestParse<Procedure>("procedure-example-f001-heart");
  TestParse<Procedure>("procedure-example-f002-lung");
  TestParse<Procedure>("procedure-example-f003-abscess");
  TestParse<Procedure>("procedure-example-f004-tracheotomy");
  TestParse<Procedure>("procedure-example-f201-tpf");
  TestParse<Procedure>("procedure-example-implant");
  TestParse<Procedure>("procedure-example-ob");
  TestParse<Procedure>("procedure-example-physical-therapy");
}

/** Test parsing of the ProcedureRequest FHIR resource. */
TEST(JsonFormatTest, ProcedureRequestParse) {
  TestParse<ProcedureRequest>("procedurerequest-example");
  TestParse<ProcedureRequest>("procedurerequest-example2");
  TestParse<ProcedureRequest>("procedurerequest-example3");
  TestParse<ProcedureRequest>("procedurerequest-example4");
  TestParse<ProcedureRequest>("procedurerequest-example-ambulation");
  TestParse<ProcedureRequest>("procedurerequest-example-appendectomy");
  TestParse<ProcedureRequest>("procedurerequest-example-colonoscopy");
  TestParse<ProcedureRequest>("procedurerequest-example-colonoscopy-bx");
  TestParse<ProcedureRequest>("procedurerequest-example-di");
  TestParse<ProcedureRequest>("procedurerequest-example-edu");
  TestParse<ProcedureRequest>("procedurerequest-example-ft4");
  TestParse<ProcedureRequest>("procedurerequest-example-implant");
  TestParse<ProcedureRequest>("procedurerequest-example-lipid");
  TestParse<ProcedureRequest>("procedurerequest-example-ob");
  TestParse<ProcedureRequest>("procedurerequest-example-pgx");
  TestParse<ProcedureRequest>("procedurerequest-example-pt");
  TestParse<ProcedureRequest>("procedurerequest-example-subrequest");
  TestParse<ProcedureRequest>("procedurerequest-genetics-example-1");
}

/** Test parsing of the ProcessRequest FHIR resource. */
TEST(JsonFormatTest, ProcessRequestParse) {
  TestParse<ProcessRequest>("processrequest-example");
  TestParse<ProcessRequest>("processrequest-example-poll-eob");
  TestParse<ProcessRequest>("processrequest-example-poll-exclusive");
  TestParse<ProcessRequest>("processrequest-example-poll-inclusive");
  TestParse<ProcessRequest>("processrequest-example-poll-payrec");
  TestParse<ProcessRequest>("processrequest-example-poll-specific");
  TestParse<ProcessRequest>("processrequest-example-reprocess");
  TestParse<ProcessRequest>("processrequest-example-reverse");
  TestParse<ProcessRequest>("processrequest-example-status");
}

/** Test parsing of the ProcessResponse FHIR resource. */
TEST(JsonFormatTest, ProcessResponseParse) {
  TestParse<ProcessResponse>("processresponse-example");
  TestParse<ProcessResponse>("processresponse-example-error");
  TestParse<ProcessResponse>("processresponse-example-pended");
}

/** Test parsing of the Provenance FHIR resource. */
TEST(JsonFormatTest, ProvenanceParse) {
  TestParse<Provenance>("provenance-example");
  TestParse<Provenance>("provenance-example-biocompute-object");
  TestParse<Provenance>("provenance-example-cwl");
  TestParse<Provenance>("provenance-example-sig");
}

/** Test parsing of the Questionnaire FHIR resource. */
TEST(JsonFormatTest, QuestionnaireParse) {
  TestParse<Questionnaire>("questionnaire-example");
  TestParse<Questionnaire>("questionnaire-example-bluebook");
  TestParse<Questionnaire>("questionnaire-example-f201-lifelines");
  TestParse<Questionnaire>("questionnaire-example-gcs");
}

/** Test parsing of the QuestionnaireResponse FHIR resource. */
TEST(JsonFormatTest, QuestionnaireResponseParse) {
  TestParse<QuestionnaireResponse>("questionnaireresponse-example");
  TestParse<QuestionnaireResponse>("questionnaireresponse-example-bluebook");
  TestParse<QuestionnaireResponse>(
      "questionnaireresponse-example-f201-lifelines");
  TestParse<QuestionnaireResponse>("questionnaireresponse-example-gcs");
  TestParse<QuestionnaireResponse>(
      "questionnaireresponse-example-ussg-fht-answers");
}

/** Test parsing of the ReferralRequest FHIR resource. */
TEST(JsonFormatTest, ReferralRequestParse) {
  TestParse<ReferralRequest>("referralrequest-example");
}

/** Test parsing of the RelatedPerson FHIR resource. */
TEST(JsonFormatTest, RelatedPersonParse) {
  TestParse<RelatedPerson>("relatedperson-example");
  TestParse<RelatedPerson>("relatedperson-example-f001-sarah");
  TestParse<RelatedPerson>("relatedperson-example-f002-ariadne");
  TestParse<RelatedPerson>("relatedperson-example-peter");
}

/** Test parsing of the RequestGroup FHIR resource. */
TEST(JsonFormatTest, RequestGroupParse) {
  TestParse<RequestGroup>("requestgroup-example");
  TestParse<RequestGroup>("requestgroup-kdn5-example");
}

/** Test parsing of the ResearchStudy FHIR resource. */
TEST(JsonFormatTest, ResearchStudyParse) {
  TestParse<ResearchStudy>("researchstudy-example");
}

/** Test parsing of the ResearchSubject FHIR resource. */
TEST(JsonFormatTest, ResearchSubjectParse) {
  TestParse<ResearchSubject>("researchsubject-example");
}

/** Test parsing of the RiskAssessment FHIR resource. */
TEST(JsonFormatTest, RiskAssessmentParse) {
  TestParse<RiskAssessment>("riskassessment-example");
  TestParse<RiskAssessment>("riskassessment-example-cardiac");
  TestParse<RiskAssessment>("riskassessment-example-population");
  TestParse<RiskAssessment>("riskassessment-example-prognosis");
}

/** Test parsing of the Schedule FHIR resource. */
TEST(JsonFormatTest, ScheduleParse) {
  TestParse<Schedule>("schedule-example");
  TestParse<Schedule>("schedule-provider-location1-example");
  TestParse<Schedule>("schedule-provider-location2-example");
}

/** Test parsing of the SearchParameter FHIR resource. */
TEST(JsonFormatTest, SearchParameterParse) {
  TestParse<SearchParameter>("searchparameter-example");
  TestParse<SearchParameter>("searchparameter-example-extension");
  TestParse<SearchParameter>("searchparameter-example-reference");
}

/** Test parsing of the Sequence FHIR resource. */
TEST(JsonFormatTest, SequenceParse) {
  TestParse<Sequence>("coord-0base-example");
  TestParse<Sequence>("coord-1base-example");
  TestParse<Sequence>("sequence-example");
  TestParse<Sequence>("sequence-example-fda");
  TestParse<Sequence>("sequence-example-fda-comparisons");
  TestParse<Sequence>("sequence-example-fda-vcfeval");
  TestParse<Sequence>("sequence-example-pgx-1");
  TestParse<Sequence>("sequence-example-pgx-2");
  TestParse<Sequence>("sequence-example-TPMT-one");
  TestParse<Sequence>("sequence-example-TPMT-two");
  TestParse<Sequence>("sequence-graphic-example-1");
  TestParse<Sequence>("sequence-graphic-example-2");
  TestParse<Sequence>("sequence-graphic-example-3");
  TestParse<Sequence>("sequence-graphic-example-4");
  TestParse<Sequence>("sequence-graphic-example-5");
}

/** Test parsing of the ServiceDefinition FHIR resource. */
TEST(JsonFormatTest, ServiceDefinitionParse) {
  TestParse<ServiceDefinition>("servicedefinition-example");
}

/** Test parsing of the Slot FHIR resource. */
TEST(JsonFormatTest, SlotParse) {
  TestParse<Slot>("slot-example");
  TestParse<Slot>("slot-example-busy");
  TestParse<Slot>("slot-example-tentative");
  TestParse<Slot>("slot-example-unavailable");
}

/** Test parsing of the Specimen FHIR resource. */
TEST(JsonFormatTest, SpecimenParse) {
  TestParse<Specimen>("specimen-example");
  TestParse<Specimen>("specimen-example-isolate");
  TestParse<Specimen>("specimen-example-serum");
  TestParse<Specimen>("specimen-example-urine");
}

/** Test parsing of the StructureDefinition FHIR resource. */
TEST(JsonFormatTest, StructureDefinitionParse) {
  TestParse<StructureDefinition>("structuredefinition-example");
}

/** Test parsing of the StructureMap FHIR resource. */
TEST(JsonFormatTest, StructureMapParse) {
  TestParse<StructureMap>("structuremap-example");
}

/** Test parsing of the Subscription FHIR resource. */
TEST(JsonFormatTest, SubscriptionParse) {
  TestParse<Subscription>("subscription-example");
  TestParse<Subscription>("subscription-example-error");
}

/** Test parsing of the Substance FHIR resource. */
TEST(JsonFormatTest, SubstanceParse) {
  TestParse<Substance>("substance-example");
  TestParse<Substance>("substance-example-amoxicillin-clavulanate");
  TestParse<Substance>("substance-example-f201-dust");
  TestParse<Substance>("substance-example-f202-staphylococcus");
  TestParse<Substance>("substance-example-f203-potassium");
  TestParse<Substance>("substance-example-silver-nitrate-product");
}

/** Test parsing of the SupplyDelivery FHIR resource. */
TEST(JsonFormatTest, SupplyDeliveryParse) {
  TestParse<SupplyDelivery>("supplydelivery-example");
  TestParse<SupplyDelivery>("supplydelivery-example-pumpdelivery");
}

/** Test parsing of the SupplyRequest FHIR resource. */
TEST(JsonFormatTest, SupplyRequestParse) {
  TestParse<SupplyRequest>("supplyrequest-example-simpleorder");
}

/** Test parsing of the Task FHIR resource. */
TEST(JsonFormatTest, TaskParse) {
  TestParse<Task>("task-example1");
  TestParse<Task>("task-example2");
  TestParse<Task>("task-example3");
  TestParse<Task>("task-example4");
  TestParse<Task>("task-example5");
  TestParse<Task>("task-example6");
}

/** Test parsing of the TestReport FHIR resource. */
TEST(JsonFormatTest, TestReportParse) {
  TestParse<TestReport>("testreport-example");
}

/** Test parsing of the TestScript FHIR resource. */
TEST(JsonFormatTest, TestScriptParse) {
  TestParse<TestScript>("testscript-example");
  TestParse<TestScript>("testscript-example-history");
  TestParse<TestScript>("testscript-example-multisystem");
  TestParse<TestScript>("testscript-example-readtest");
  TestParse<TestScript>("testscript-example-rule");
  TestParse<TestScript>("testscript-example-search");
  TestParse<TestScript>("testscript-example-update");
}

/** Test parsing of the ValueSet FHIR resource. */
TEST(JsonFormatTest, ValueSetParse) {
  TestParse<ValueSet>("valueset-example");
  TestParse<ValueSet>("valueset-example-expansion");
  TestParse<ValueSet>("valueset-example-inactive");
  TestParse<ValueSet>("valueset-example-intensional");
  TestParse<ValueSet>("valueset-example-yesnodontknow");
  TestParse<ValueSet>("valueset-list-example-codes");
}

/** Test parsing of the VisionPrescription FHIR resource. */
TEST(JsonFormatTest, VisionPrescriptionParse) {
  TestParse<VisionPrescription>("visionprescription-example");
  TestParse<VisionPrescription>("visionprescription-example-1");
}

// Print tests

/* Resource tests start here. */

/** Test printing of the Account FHIR resource. */
TEST(JsonFormatTest, AccountPrint) {
  TestPrint<Account>("account-example");
  TestPrint<Account>("account-example-with-guarantor");
}

/** Test printing of the ActivityDefinition FHIR resource. */
TEST(JsonFormatTest, ActivityDefinitionPrint) {
  TestPrint<ActivityDefinition>("activitydefinition-example");
  TestPrint<ActivityDefinition>("activitydefinition-medicationorder-example");
  TestPrint<ActivityDefinition>("activitydefinition-predecessor-example");
  TestPrint<ActivityDefinition>("activitydefinition-procedurerequest-example");
  TestPrint<ActivityDefinition>("activitydefinition-supplyrequest-example");
}

/** Test printing of the AdverseEvent FHIR resource. */
TEST(JsonFormatTest, AdverseEventPrint) {
  TestPrint<AdverseEvent>("adverseevent-example");
}

/** Test printing of the AllergyIntolerance FHIR resource. */
TEST(JsonFormatTest, AllergyIntolerancePrint) {
  TestPrint<AllergyIntolerance>("allergyintolerance-example");
}

/** Test printing of the Appointment FHIR resource. */
TEST(JsonFormatTest, AppointmentPrint) {
  TestPrint<Appointment>("appointment-example");
  TestPrint<Appointment>("appointment-example2doctors");
  TestPrint<Appointment>("appointment-example-request");
}

/** Test printing of the AppointmentResponse FHIR resource. */
TEST(JsonFormatTest, AppointmentResponsePrint) {
  TestPrint<AppointmentResponse>("appointmentresponse-example");
  TestPrint<AppointmentResponse>("appointmentresponse-example-req");
}

/** Test printing of the AuditEvent FHIR resource. */
TEST(JsonFormatTest, AuditEventPrint) {
  TestPrint<AuditEvent>("auditevent-example");
  TestPrint<AuditEvent>("auditevent-example-disclosure");
  TestPrint<AuditEvent>("audit-event-example-login");
  TestPrint<AuditEvent>("audit-event-example-logout");
  TestPrint<AuditEvent>("audit-event-example-media");
  TestPrint<AuditEvent>("audit-event-example-pixQuery");
  TestPrint<AuditEvent>("audit-event-example-search");
  TestPrint<AuditEvent>("audit-event-example-vread");
}

/** Test printing of the Basic FHIR resource. */
TEST(JsonFormatTest, BasicPrint) {
  TestPrint<Basic>("basic-example");
  TestPrint<Basic>("basic-example2");
  TestPrint<Basic>("basic-example-narrative");
}

/** Test printing of the Binary FHIR resource. */
TEST(JsonFormatTest, BinaryPrint) { TestPrint<Binary>("binary-example"); }

/** Test printing of the BodySite FHIR resource. */
TEST(JsonFormatTest, BodySitePrint) {
  TestPrint<BodySite>("bodysite-example-fetus");
  TestPrint<BodySite>("bodysite-example-skin-patch");
  TestPrint<BodySite>("bodysite-example-tumor");
}

/** Test printing of the Bundle FHIR resource. */
TEST(JsonFormatTest, BundlePrint) {
  TestPrint<Bundle>("bundle-example");
  TestPrint<Bundle>("diagnosticreport-examples-general");
  TestPrint<Bundle>("diagnosticreport-hla-genetics-results-example");
  TestPrint<Bundle>("document-example-dischargesummary");
  TestPrint<Bundle>("endpoint-examples-general-template");
  TestPrint<Bundle>("location-examples-general");
  TestPrint<Bundle>("patient-examples-cypress-template");
  TestPrint<Bundle>("patient-examples-general");
  TestPrint<Bundle>("practitioner-examples-general");
  TestPrint<Bundle>("practitionerrole-examples-general");
  TestPrint<Bundle>("questionnaire-profile-example-ussg-fht");
  TestPrint<Bundle>("xds-example");
}

/** Test printing of the CapabilityStatement FHIR resource. */
TEST(JsonFormatTest, CapabilityStatementPrint) {
  TestPrint<CapabilityStatement>("capabilitystatement-example");
  TestPrint<CapabilityStatement>("capabilitystatement-phr-example");
}

/** Test printing of the CarePlan FHIR resource. */
TEST(JsonFormatTest, CarePlanPrint) {
  TestPrint<CarePlan>("careplan-example");
  TestPrint<CarePlan>("careplan-example-f001-heart");
  TestPrint<CarePlan>("careplan-example-f002-lung");
  TestPrint<CarePlan>("careplan-example-f003-pharynx");
  TestPrint<CarePlan>("careplan-example-f201-renal");
  TestPrint<CarePlan>("careplan-example-f202-malignancy");
  TestPrint<CarePlan>("careplan-example-f203-sepsis");
  TestPrint<CarePlan>("careplan-example-GPVisit");
  TestPrint<CarePlan>("careplan-example-integrated");
  TestPrint<CarePlan>("careplan-example-obesity-narrative");
  TestPrint<CarePlan>("careplan-example-pregnancy");
}

/** Test printing of the CareTeam FHIR resource. */
TEST(JsonFormatTest, CareTeamPrint) { TestPrint<CareTeam>("careteam-example"); }

/** Test printing of the ChargeItem FHIR resource. */
TEST(JsonFormatTest, ChargeItemPrint) {
  TestPrint<ChargeItem>("chargeitem-example");
}

/** Test printing of the Claim FHIR resource. */
TEST(JsonFormatTest, ClaimPrint) {
  TestPrint<Claim>("claim-example");
  TestPrint<Claim>("claim-example-institutional");
  TestPrint<Claim>("claim-example-institutional-rich");
  TestPrint<Claim>("claim-example-oral-average");
  TestPrint<Claim>("claim-example-oral-bridge");
  TestPrint<Claim>("claim-example-oral-contained");
  TestPrint<Claim>("claim-example-oral-contained-identifier");
  TestPrint<Claim>("claim-example-oral-identifier");
  TestPrint<Claim>("claim-example-oral-orthoplan");
  TestPrint<Claim>("claim-example-pharmacy");
  TestPrint<Claim>("claim-example-pharmacy-compound");
  TestPrint<Claim>("claim-example-pharmacy-medication");
  TestPrint<Claim>("claim-example-professional");
  TestPrint<Claim>("claim-example-vision");
  TestPrint<Claim>("claim-example-vision-glasses");
  TestPrint<Claim>("claim-example-vision-glasses-3tier");
}

/** Test printing of the ClaimResponse FHIR resource. */
TEST(JsonFormatTest, ClaimResponsePrint) {
  TestPrint<ClaimResponse>("claimresponse-example");
}

/** Test printing of the ClinicalImpression FHIR resource. */
TEST(JsonFormatTest, ClinicalImpressionPrint) {
  TestPrint<ClinicalImpression>("clinicalimpression-example");
}

/** Test printing of the CodeSystem FHIR resource. */
TEST(JsonFormatTest, CodeSystemPrint) {
  TestPrint<CodeSystem>("codesystem-example");
  TestPrint<CodeSystem>("codesystem-example-summary");
  TestPrint<CodeSystem>("codesystem-list-example-codes");
}

/** Test printing of the Communication FHIR resource. */
TEST(JsonFormatTest, CommunicationPrint) {
  TestPrint<Communication>("communication-example");
  TestPrint<Communication>("communication-example-fm-attachment");
  TestPrint<Communication>("communication-example-fm-solicited-attachment");
}

/** Test printing of the CommunicationRequest FHIR resource. */
TEST(JsonFormatTest, CommunicationRequestPrint) {
  TestPrint<CommunicationRequest>("communicationrequest-example");
  TestPrint<CommunicationRequest>(
      "communicationrequest-example-fm-solicit-attachment");
}

/** Test printing of the CompartmentDefinition FHIR resource. */
TEST(JsonFormatTest, CompartmentDefinitionPrint) {
  TestPrint<CompartmentDefinition>("compartmentdefinition-example");
}

/** Test printing of the Composition FHIR resource. */
TEST(JsonFormatTest, CompositionPrint) {
  TestPrint<Composition>("composition-example");
}

/** Test printing of the ConceptMap FHIR resource. */
TEST(JsonFormatTest, ConceptMapPrint) {
  TestPrint<ConceptMap>("conceptmap-example");
  TestPrint<ConceptMap>("conceptmap-example-2");
  TestPrint<ConceptMap>("conceptmap-example-specimen-type");
}

/** Test printing of the Condition FHIR resource. */
TEST(JsonFormatTest, ConditionPrint) {
  TestPrint<Condition>("condition-example");
  TestPrint<Condition>("condition-example2");
  TestPrint<Condition>("condition-example-f001-heart");
  TestPrint<Condition>("condition-example-f002-lung");
  TestPrint<Condition>("condition-example-f003-abscess");
  TestPrint<Condition>("condition-example-f201-fever");
  TestPrint<Condition>("condition-example-f202-malignancy");
  TestPrint<Condition>("condition-example-f203-sepsis");
  TestPrint<Condition>("condition-example-f204-renal");
  TestPrint<Condition>("condition-example-f205-infection");
  TestPrint<Condition>("condition-example-family-history");
  TestPrint<Condition>("condition-example-stroke");
}

/** Test printing of the Consent FHIR resource. */
TEST(JsonFormatTest, ConsentPrint) {
  TestPrint<Consent>("consent-example");
  TestPrint<Consent>("consent-example-Emergency");
  TestPrint<Consent>("consent-example-grantor");
  TestPrint<Consent>("consent-example-notAuthor");
  TestPrint<Consent>("consent-example-notOrg");
  TestPrint<Consent>("consent-example-notThem");
  TestPrint<Consent>("consent-example-notThis");
  TestPrint<Consent>("consent-example-notTime");
  TestPrint<Consent>("consent-example-Out");
  TestPrint<Consent>("consent-example-pkb");
  TestPrint<Consent>("consent-example-signature");
  TestPrint<Consent>("consent-example-smartonfhir");
}

/** Test printing of the Contract FHIR resource. */
TEST(JsonFormatTest, ContractPrint) {
  TestPrint<Contract>("contract-example");
  TestPrint<Contract>("contract-example-42cfr-part2");
  TestPrint<Contract>("pcd-example-notAuthor");
  TestPrint<Contract>("pcd-example-notLabs");
  TestPrint<Contract>("pcd-example-notOrg");
  TestPrint<Contract>("pcd-example-notThem");
  TestPrint<Contract>("pcd-example-notThis");
}

/** Test printing of the Coverage FHIR resource. */
TEST(JsonFormatTest, CoveragePrint) {
  TestPrint<Coverage>("coverage-example");
  TestPrint<Coverage>("coverage-example-2");
  TestPrint<Coverage>("coverage-example-ehic");
  TestPrint<Coverage>("coverage-example-selfpay");
}

/** Test printing of the DataElement FHIR resource. */
TEST(JsonFormatTest, DataElementPrint) {
  TestPrint<DataElement>("dataelement-example");
  TestPrint<DataElement>("dataelement-labtestmaster-example");
}

/** Test printing of the DetectedIssue FHIR resource. */
TEST(JsonFormatTest, DetectedIssuePrint) {
  TestPrint<DetectedIssue>("detectedissue-example");
  TestPrint<DetectedIssue>("detectedissue-example-allergy");
  TestPrint<DetectedIssue>("detectedissue-example-dup");
  TestPrint<DetectedIssue>("detectedissue-example-lab");
}

/** Test printing of the Device FHIR resource. */
TEST(JsonFormatTest, DevicePrint) {
  TestPrint<Device>("device-example");
  TestPrint<Device>("device-example-f001-feedingtube");
  TestPrint<Device>("device-example-ihe-pcd");
  TestPrint<Device>("device-example-pacemaker");
  TestPrint<Device>("device-example-software");
  TestPrint<Device>("device-example-udi1");
  TestPrint<Device>("device-example-udi2");
  TestPrint<Device>("device-example-udi3");
  TestPrint<Device>("device-example-udi4");
}

/** Test printing of the DeviceComponent FHIR resource. */
TEST(JsonFormatTest, DeviceComponentPrint) {
  TestPrint<DeviceComponent>("devicecomponent-example");
  TestPrint<DeviceComponent>("devicecomponent-example-prodspec");
}

/** Test printing of the DeviceMetric FHIR resource. */
TEST(JsonFormatTest, DeviceMetricPrint) {
  TestPrint<DeviceMetric>("devicemetric-example");
}

/** Test printing of the DeviceRequest FHIR resource. */
TEST(JsonFormatTest, DeviceRequestPrint) {
  TestPrint<DeviceRequest>("devicerequest-example");
  TestPrint<DeviceRequest>("devicerequest-example-insulinpump");
}

/** Test printing of the DeviceUseStatement FHIR resource. */
TEST(JsonFormatTest, DeviceUseStatementPrint) {
  TestPrint<DeviceUseStatement>("deviceusestatement-example");
}

/** Test printing of the DiagnosticReport FHIR resource. */
TEST(JsonFormatTest, DiagnosticReportPrint) {
  TestPrint<DiagnosticReport>("diagnosticreport-example");
  TestPrint<DiagnosticReport>("diagnosticreport-example-dxa");
  TestPrint<DiagnosticReport>("diagnosticreport-example-f001-bloodexam");
  TestPrint<DiagnosticReport>("diagnosticreport-example-f201-brainct");
  TestPrint<DiagnosticReport>("diagnosticreport-example-f202-bloodculture");
  TestPrint<DiagnosticReport>("diagnosticreport-example-ghp");
  TestPrint<DiagnosticReport>("diagnosticreport-example-gingival-mass");
  TestPrint<DiagnosticReport>("diagnosticreport-example-lipids");
  TestPrint<DiagnosticReport>("diagnosticreport-example-papsmear");
  TestPrint<DiagnosticReport>("diagnosticreport-example-pgx");
  TestPrint<DiagnosticReport>("diagnosticreport-example-ultrasound");
  TestPrint<DiagnosticReport>(
      "diagnosticreport-genetics-example-2-familyhistory");
}

/** Test printing of the DocumentManifest FHIR resource. */
TEST(JsonFormatTest, DocumentManifestPrint) {
  TestPrint<DocumentManifest>("documentmanifest-example");
}

/** Test printing of the DocumentReference FHIR resource. */
TEST(JsonFormatTest, DocumentReferencePrint) {
  TestPrint<DocumentReference>("documentreference-example");
}

/** Test printing of the EligibilityRequest FHIR resource. */
TEST(JsonFormatTest, EligibilityRequestPrint) {
  TestPrint<EligibilityRequest>("eligibilityrequest-example");
  TestPrint<EligibilityRequest>("eligibilityrequest-example-2");
}

/** Test printing of the EligibilityResponse FHIR resource. */
TEST(JsonFormatTest, EligibilityResponsePrint) {
  TestPrint<EligibilityResponse>("eligibilityresponse-example");
  TestPrint<EligibilityResponse>("eligibilityresponse-example-benefits");
  TestPrint<EligibilityResponse>("eligibilityresponse-example-benefits-2");
  TestPrint<EligibilityResponse>("eligibilityresponse-example-error");
}

/** Test printing of the Encounter FHIR resource. */
TEST(JsonFormatTest, EncounterPrint) {
  TestPrint<Encounter>("encounter-example");
  TestPrint<Encounter>("encounter-example-emerg");
  TestPrint<Encounter>("encounter-example-f001-heart");
  TestPrint<Encounter>("encounter-example-f002-lung");
  TestPrint<Encounter>("encounter-example-f003-abscess");
  TestPrint<Encounter>("encounter-example-f201-20130404");
  TestPrint<Encounter>("encounter-example-f202-20130128");
  TestPrint<Encounter>("encounter-example-f203-20130311");
  TestPrint<Encounter>("encounter-example-home");
  TestPrint<Encounter>("encounter-example-xcda");
}

/** Test printing of the Endpoint FHIR resource. */
TEST(JsonFormatTest, EndpointPrint) {
  TestPrint<Endpoint>("endpoint-example");
  TestPrint<Endpoint>("endpoint-example-iid");
  TestPrint<Endpoint>("endpoint-example-wadors");
}

/** Test printing of the EnrollmentRequest FHIR resource. */
TEST(JsonFormatTest, EnrollmentRequestPrint) {
  TestPrint<EnrollmentRequest>("enrollmentrequest-example");
}

/** Test printing of the EnrollmentResponse FHIR resource. */
TEST(JsonFormatTest, EnrollmentResponsePrint) {
  TestPrint<EnrollmentResponse>("enrollmentresponse-example");
}

/** Test printing of the EpisodeOfCare FHIR resource. */
TEST(JsonFormatTest, EpisodeOfCarePrint) {
  TestPrint<EpisodeOfCare>("episodeofcare-example");
}

/** Test printing of the ExpansionProfile FHIR resource. */
TEST(JsonFormatTest, ExpansionProfilePrint) {
  TestPrint<ExpansionProfile>("expansionprofile-example");
}

/** Test printing of the ExplanationOfBenefit FHIR resource. */
TEST(JsonFormatTest, ExplanationOfBenefitPrint) {
  TestPrint<ExplanationOfBenefit>("explanationofbenefit-example");
}

/** Test printing of the FamilyMemberHistory FHIR resource. */
TEST(JsonFormatTest, FamilyMemberHistoryPrint) {
  TestPrint<FamilyMemberHistory>("familymemberhistory-example");
  TestPrint<FamilyMemberHistory>("familymemberhistory-example-mother");
}

/** Test printing of the Flag FHIR resource. */
TEST(JsonFormatTest, FlagPrint) {
  TestPrint<Flag>("flag-example");
  TestPrint<Flag>("flag-example-encounter");
}

/** Test printing of the Goal FHIR resource. */
TEST(JsonFormatTest, GoalPrint) {
  TestPrint<Goal>("goal-example");
  TestPrint<Goal>("goal-example-stop-smoking");
}

/** Test printing of the GraphDefinition FHIR resource. */
TEST(JsonFormatTest, GraphDefinitionPrint) {
  TestPrint<GraphDefinition>("graphdefinition-example");
}

/** Test printing of the Group FHIR resource. */
TEST(JsonFormatTest, GroupPrint) {
  TestPrint<Group>("group-example");
  TestPrint<Group>("group-example-member");
}

/** Test printing of the GuidanceResponse FHIR resource. */
TEST(JsonFormatTest, GuidanceResponsePrint) {
  TestPrint<GuidanceResponse>("guidanceresponse-example");
}

/** Test printing of the HealthcareService FHIR resource. */
TEST(JsonFormatTest, HealthcareServicePrint) {
  TestPrint<HealthcareService>("healthcareservice-example");
}

/** Test printing of the ImagingManifest FHIR resource. */
TEST(JsonFormatTest, ImagingManifestPrint) {
  TestPrint<ImagingManifest>("imagingmanifest-example");
}

/** Test printing of the ImagingStudy FHIR resource. */
TEST(JsonFormatTest, ImagingStudyPrint) {
  TestPrint<ImagingStudy>("imagingstudy-example");
  TestPrint<ImagingStudy>("imagingstudy-example-xr");
}

/** Test printing of the Immunization FHIR resource. */
TEST(JsonFormatTest, ImmunizationPrint) {
  TestPrint<Immunization>("immunization-example");
  TestPrint<Immunization>("immunization-example-historical");
  TestPrint<Immunization>("immunization-example-refused");
}

/** Test printing of the ImmunizationRecommendation FHIR resource. */
TEST(JsonFormatTest, ImmunizationRecommendationPrint) {
  TestPrint<ImmunizationRecommendation>("immunizationrecommendation-example");
  TestPrint<ImmunizationRecommendation>(
      "immunizationrecommendation-target-disease-example");
}

/** Test printing of the ImplementationGuide FHIR resource. */
TEST(JsonFormatTest, ImplementationGuidePrint) {
  TestPrint<ImplementationGuide>("implementationguide-example");
}

/** Test printing of the Library FHIR resource. */
TEST(JsonFormatTest, LibraryPrint) {
  TestPrint<Library>("library-cms146-example");
  TestPrint<Library>("library-composition-example");
  TestPrint<Library>("library-example");
  TestPrint<Library>("library-predecessor-example");
}

/** Test printing of the Linkage FHIR resource. */
TEST(JsonFormatTest, LinkagePrint) { TestPrint<Linkage>("linkage-example"); }

/** Test printing of the List FHIR resource. */
TEST(JsonFormatTest, ListPrint) {
  TestPrint<List>("list-example");
  TestPrint<List>("list-example-allergies");
  TestPrint<List>("list-example-double-cousin-relationship-pedigree");
  TestPrint<List>("list-example-empty");
  TestPrint<List>("list-example-familyhistory-f201-roel");
  TestPrint<List>("list-example-familyhistory-genetics-profile");
  TestPrint<List>("list-example-familyhistory-genetics-profile-annie");
  TestPrint<List>("list-example-medlist");
  TestPrint<List>("list-example-simple-empty");
}

/** Test printing of the Location FHIR resource. */
TEST(JsonFormatTest, LocationPrint) {
  TestPrint<Location>("location-example");
  TestPrint<Location>("location-example-ambulance");
  TestPrint<Location>("location-example-hl7hq");
  TestPrint<Location>("location-example-patients-home");
  TestPrint<Location>("location-example-room");
  TestPrint<Location>("location-example-ukpharmacy");
}

/** Test printing of the Measure FHIR resource. */
TEST(JsonFormatTest, MeasurePrint) {
  TestPrint<Measure>("measure-cms146-example");
  TestPrint<Measure>("measure-component-a-example");
  TestPrint<Measure>("measure-component-b-example");
  TestPrint<Measure>("measure-composite-example");
  TestPrint<Measure>("measure-predecessor-example");
}

/** Test printing of the MeasureReport FHIR resource. */
TEST(JsonFormatTest, MeasureReportPrint) {
  TestPrint<MeasureReport>("measurereport-cms146-cat1-example");
  TestPrint<MeasureReport>("measurereport-cms146-cat2-example");
  TestPrint<MeasureReport>("measurereport-cms146-cat3-example");
}

/** Test printing of the Media FHIR resource. */
TEST(JsonFormatTest, MediaPrint) {
  TestPrint<Media>("media-example");
  TestPrint<Media>("media-example-dicom");
  TestPrint<Media>("media-example-sound");
  TestPrint<Media>("media-example-xray");
}

/** Test printing of the Medication FHIR resource. */
TEST(JsonFormatTest, MedicationPrint) {
  TestPrint<Medication>("medicationexample0301");
  TestPrint<Medication>("medicationexample0302");
  TestPrint<Medication>("medicationexample0303");
  TestPrint<Medication>("medicationexample0304");
  TestPrint<Medication>("medicationexample0305");
  TestPrint<Medication>("medicationexample0306");
  TestPrint<Medication>("medicationexample0307");
  TestPrint<Medication>("medicationexample0308");
  TestPrint<Medication>("medicationexample0309");
  TestPrint<Medication>("medicationexample0310");
  TestPrint<Medication>("medicationexample0311");
  TestPrint<Medication>("medicationexample0312");
  TestPrint<Medication>("medicationexample0313");
  TestPrint<Medication>("medicationexample0314");
  TestPrint<Medication>("medicationexample0315");
  TestPrint<Medication>("medicationexample0316");
  TestPrint<Medication>("medicationexample0317");
  TestPrint<Medication>("medicationexample0318");
  TestPrint<Medication>("medicationexample0319");
  TestPrint<Medication>("medicationexample0320");
  TestPrint<Medication>("medicationexample0321");
  TestPrint<Medication>("medicationexample1");
  TestPrint<Medication>("medicationexample15");
}

/** Test printing of the MedicationAdministration FHIR resource. */
TEST(JsonFormatTest, MedicationAdministrationPrint) {
  TestPrint<MedicationAdministration>("medicationadministrationexample3");
}

/** Test printing of the MedicationDispense FHIR resource. */
TEST(JsonFormatTest, MedicationDispensePrint) {
  TestPrint<MedicationDispense>("medicationdispenseexample8");
}

/** Test printing of the MedicationRequest FHIR resource. */
TEST(JsonFormatTest, MedicationRequestPrint) {
  TestPrint<MedicationRequest>("medicationrequestexample1");
  TestPrint<MedicationRequest>("medicationrequestexample2");
}

/** Test printing of the MedicationStatement FHIR resource. */
TEST(JsonFormatTest, MedicationStatementPrint) {
  TestPrint<MedicationStatement>("medicationstatementexample1");
  TestPrint<MedicationStatement>("medicationstatementexample2");
  TestPrint<MedicationStatement>("medicationstatementexample3");
  TestPrint<MedicationStatement>("medicationstatementexample4");
  TestPrint<MedicationStatement>("medicationstatementexample5");
  TestPrint<MedicationStatement>("medicationstatementexample6");
  TestPrint<MedicationStatement>("medicationstatementexample7");
}

/** Test printing of the MessageDefinition FHIR resource. */
TEST(JsonFormatTest, MessageDefinitionPrint) {
  TestPrint<MessageDefinition>("messagedefinition-example");
}

/** Test printing of the MessageHeader FHIR resource. */
TEST(JsonFormatTest, MessageHeaderPrint) {
  TestPrint<MessageHeader>("messageheader-example");
}

/** Test printing of the NamingSystem FHIR resource. */
TEST(JsonFormatTest, NamingSystemPrint) {
  TestPrint<NamingSystem>("namingsystem-example");
  TestPrint<NamingSystem>("namingsystem-example-id");
  TestPrint<NamingSystem>("namingsystem-example-replaced");
}

/** Test printing of the NutritionOrder FHIR resource. */
TEST(JsonFormatTest, NutritionOrderPrint) {
  TestPrint<NutritionOrder>("nutritionorder-example-cardiacdiet");
  TestPrint<NutritionOrder>("nutritionorder-example-diabeticdiet");
  TestPrint<NutritionOrder>("nutritionorder-example-diabeticsupplement");
  TestPrint<NutritionOrder>("nutritionorder-example-energysupplement");
  TestPrint<NutritionOrder>("nutritionorder-example-enteralbolus");
  TestPrint<NutritionOrder>("nutritionorder-example-enteralcontinuous");
  TestPrint<NutritionOrder>("nutritionorder-example-fiberrestricteddiet");
  TestPrint<NutritionOrder>("nutritionorder-example-infantenteral");
  TestPrint<NutritionOrder>("nutritionorder-example-proteinsupplement");
  TestPrint<NutritionOrder>("nutritionorder-example-pureeddiet");
  TestPrint<NutritionOrder>("nutritionorder-example-pureeddiet-simple");
  TestPrint<NutritionOrder>("nutritionorder-example-renaldiet");
  TestPrint<NutritionOrder>("nutritionorder-example-texture-modified");
}

/** Test printing of the Observation FHIR resource. */
TEST(JsonFormatTest, ObservationPrint) {
  TestPrint<Observation>("observation-example");
  TestPrint<Observation>("observation-example-10minute-apgar-score");
  TestPrint<Observation>("observation-example-1minute-apgar-score");
  TestPrint<Observation>("observation-example-20minute-apgar-score");
  TestPrint<Observation>("observation-example-2minute-apgar-score");
  TestPrint<Observation>("observation-example-5minute-apgar-score");
  TestPrint<Observation>("observation-example-bloodpressure");
  TestPrint<Observation>("observation-example-bloodpressure-cancel");
  TestPrint<Observation>("observation-example-bloodpressure-dar");
  TestPrint<Observation>("observation-example-bmd");
  TestPrint<Observation>("observation-example-bmi");
  TestPrint<Observation>("observation-example-body-height");
  TestPrint<Observation>("observation-example-body-length");
  TestPrint<Observation>("observation-example-body-temperature");
  TestPrint<Observation>("observation-example-date-lastmp");
  TestPrint<Observation>("observation-example-diplotype1");
  TestPrint<Observation>("observation-example-eye-color");
  TestPrint<Observation>("observation-example-f001-glucose");
  TestPrint<Observation>("observation-example-f002-excess");
  TestPrint<Observation>("observation-example-f003-co2");
  TestPrint<Observation>("observation-example-f004-erythrocyte");
  TestPrint<Observation>("observation-example-f005-hemoglobin");
  TestPrint<Observation>("observation-example-f202-temperature");
  TestPrint<Observation>("observation-example-f203-bicarbonate");
  TestPrint<Observation>("observation-example-f204-creatinine");
  TestPrint<Observation>("observation-example-f205-egfr");
  TestPrint<Observation>("observation-example-f206-staphylococcus");
  TestPrint<Observation>("observation-example-genetics-1");
  TestPrint<Observation>("observation-example-genetics-2");
  TestPrint<Observation>("observation-example-genetics-3");
  TestPrint<Observation>("observation-example-genetics-4");
  TestPrint<Observation>("observation-example-genetics-5");
  TestPrint<Observation>("observation-example-glasgow");
  TestPrint<Observation>("observation-example-glasgow-qa");
  TestPrint<Observation>("observation-example-haplotype1");
  TestPrint<Observation>("observation-example-haplotype2");
  TestPrint<Observation>("observation-example-head-circumference");
  TestPrint<Observation>("observation-example-heart-rate");
  TestPrint<Observation>("observation-example-mbp");
  TestPrint<Observation>("observation-example-phenotype");
  TestPrint<Observation>("observation-example-respiratory-rate");
  TestPrint<Observation>("observation-example-sample-data");
  TestPrint<Observation>("observation-example-satO2");
  TestPrint<Observation>("observation-example-TPMT-diplotype");
  TestPrint<Observation>("observation-example-TPMT-haplotype-one");
  TestPrint<Observation>("observation-example-TPMT-haplotype-two");
  TestPrint<Observation>("observation-example-unsat");
  TestPrint<Observation>("observation-example-vitals-panel");
}

/** Test printing of the OperationDefinition FHIR resource. */
TEST(JsonFormatTest, OperationDefinitionPrint) {
  TestPrint<OperationDefinition>("operationdefinition-example");
}

/** Test printing of the OperationOutcome FHIR resource. */
TEST(JsonFormatTest, OperationOutcomePrint) {
  TestPrint<OperationOutcome>("operationoutcome-example");
  TestPrint<OperationOutcome>("operationoutcome-example-allok");
  TestPrint<OperationOutcome>("operationoutcome-example-break-the-glass");
  TestPrint<OperationOutcome>("operationoutcome-example-exception");
  TestPrint<OperationOutcome>("operationoutcome-example-searchfail");
  TestPrint<OperationOutcome>("operationoutcome-example-validationfail");
}

/** Test printing of the Organization FHIR resource. */
TEST(JsonFormatTest, OrganizationPrint) {
  TestPrint<Organization>("organization-example");
  TestPrint<Organization>("organization-example-f001-burgers");
  TestPrint<Organization>("organization-example-f002-burgers-card");
  TestPrint<Organization>("organization-example-f003-burgers-ENT");
  TestPrint<Organization>("organization-example-f201-aumc");
  TestPrint<Organization>("organization-example-f203-bumc");
  TestPrint<Organization>("organization-example-gastro");
  TestPrint<Organization>("organization-example-good-health-care");
  TestPrint<Organization>("organization-example-insurer");
  TestPrint<Organization>("organization-example-lab");
  TestPrint<Organization>("organization-example-mmanu");
}

/** Test printing of the Parameters FHIR resource. */
TEST(JsonFormatTest, ParametersPrint) {
  TestPrint<Parameters>("parameters-example");
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
  TestPrint<PaymentNotice>("paymentnotice-example");
}

/** Test printing of the PaymentReconciliation FHIR resource. */
TEST(JsonFormatTest, PaymentReconciliationPrint) {
  TestPrint<PaymentReconciliation>("paymentreconciliation-example");
}

/** Test printing of the Person FHIR resource. */
TEST(JsonFormatTest, PersonPrint) {
  TestPrint<Person>("person-example");
  TestPrint<Person>("person-example-f002-ariadne");
}

/** Test printing of the PlanDefinition FHIR resource. */
TEST(JsonFormatTest, PlanDefinitionPrint) {
  TestPrint<PlanDefinition>("plandefinition-example");
  TestPrint<PlanDefinition>("plandefinition-example-kdn5-simplified");
  TestPrint<PlanDefinition>("plandefinition-options-example");
  TestPrint<PlanDefinition>("plandefinition-predecessor-example");
  TestPrint<PlanDefinition>("plandefinition-protocol-example");
}

/** Test printing of the Practitioner FHIR resource. */
TEST(JsonFormatTest, PractitionerPrint) {
  TestPrint<Practitioner>("practitioner-example");
  TestPrint<Practitioner>("practitioner-example-f001-evdb");
  TestPrint<Practitioner>("practitioner-example-f002-pv");
  TestPrint<Practitioner>("practitioner-example-f003-mv");
  TestPrint<Practitioner>("practitioner-example-f004-rb");
  TestPrint<Practitioner>("practitioner-example-f005-al");
  TestPrint<Practitioner>("practitioner-example-f006-rvdb");
  TestPrint<Practitioner>("practitioner-example-f007-sh");
  TestPrint<Practitioner>("practitioner-example-f201-ab");
  TestPrint<Practitioner>("practitioner-example-f202-lm");
  TestPrint<Practitioner>("practitioner-example-f203-jvg");
  TestPrint<Practitioner>("practitioner-example-f204-ce");
  TestPrint<Practitioner>("practitioner-example-xcda1");
  TestPrint<Practitioner>("practitioner-example-xcda-author");
}

/** Test printing of the PractitionerRole FHIR resource. */
TEST(JsonFormatTest, PractitionerRolePrint) {
  TestPrint<PractitionerRole>("practitionerrole-example");
}

/** Test printing of the Procedure FHIR resource. */
TEST(JsonFormatTest, ProcedurePrint) {
  TestPrint<Procedure>("procedure-example");
  TestPrint<Procedure>("procedure-example-ambulation");
  TestPrint<Procedure>("procedure-example-appendectomy-narrative");
  TestPrint<Procedure>("procedure-example-biopsy");
  TestPrint<Procedure>("procedure-example-colon-biopsy");
  TestPrint<Procedure>("procedure-example-colonoscopy");
  TestPrint<Procedure>("procedure-example-education");
  TestPrint<Procedure>("procedure-example-f001-heart");
  TestPrint<Procedure>("procedure-example-f002-lung");
  TestPrint<Procedure>("procedure-example-f003-abscess");
  TestPrint<Procedure>("procedure-example-f004-tracheotomy");
  TestPrint<Procedure>("procedure-example-f201-tpf");
  TestPrint<Procedure>("procedure-example-implant");
  TestPrint<Procedure>("procedure-example-ob");
  TestPrint<Procedure>("procedure-example-physical-therapy");
}

/** Test printing of the ProcedureRequest FHIR resource. */
TEST(JsonFormatTest, ProcedureRequestPrint) {
  TestPrint<ProcedureRequest>("procedurerequest-example");
  TestPrint<ProcedureRequest>("procedurerequest-example2");
  TestPrint<ProcedureRequest>("procedurerequest-example3");
  TestPrint<ProcedureRequest>("procedurerequest-example4");
  TestPrint<ProcedureRequest>("procedurerequest-example-ambulation");
  TestPrint<ProcedureRequest>("procedurerequest-example-appendectomy");
  TestPrint<ProcedureRequest>("procedurerequest-example-colonoscopy");
  TestPrint<ProcedureRequest>("procedurerequest-example-colonoscopy-bx");
  TestPrint<ProcedureRequest>("procedurerequest-example-di");
  TestPrint<ProcedureRequest>("procedurerequest-example-edu");
  TestPrint<ProcedureRequest>("procedurerequest-example-ft4");
  TestPrint<ProcedureRequest>("procedurerequest-example-implant");
  TestPrint<ProcedureRequest>("procedurerequest-example-lipid");
  TestPrint<ProcedureRequest>("procedurerequest-example-ob");
  TestPrint<ProcedureRequest>("procedurerequest-example-pgx");
  TestPrint<ProcedureRequest>("procedurerequest-example-pt");
  TestPrint<ProcedureRequest>("procedurerequest-example-subrequest");
  TestPrint<ProcedureRequest>("procedurerequest-genetics-example-1");
}

/** Test printing of the ProcessRequest FHIR resource. */
TEST(JsonFormatTest, ProcessRequestPrint) {
  TestPrint<ProcessRequest>("processrequest-example");
  TestPrint<ProcessRequest>("processrequest-example-poll-eob");
  TestPrint<ProcessRequest>("processrequest-example-poll-exclusive");
  TestPrint<ProcessRequest>("processrequest-example-poll-inclusive");
  TestPrint<ProcessRequest>("processrequest-example-poll-payrec");
  TestPrint<ProcessRequest>("processrequest-example-poll-specific");
  TestPrint<ProcessRequest>("processrequest-example-reprocess");
  TestPrint<ProcessRequest>("processrequest-example-reverse");
  TestPrint<ProcessRequest>("processrequest-example-status");
}

/** Test printing of the ProcessResponse FHIR resource. */
TEST(JsonFormatTest, ProcessResponsePrint) {
  TestPrint<ProcessResponse>("processresponse-example");
  TestPrint<ProcessResponse>("processresponse-example-error");
  TestPrint<ProcessResponse>("processresponse-example-pended");
}

/** Test printing of the Provenance FHIR resource. */
TEST(JsonFormatTest, ProvenancePrint) {
  TestPrint<Provenance>("provenance-example");
  TestPrint<Provenance>("provenance-example-biocompute-object");
  TestPrint<Provenance>("provenance-example-cwl");
  TestPrint<Provenance>("provenance-example-sig");
}

/** Test printing of the Questionnaire FHIR resource. */
TEST(JsonFormatTest, QuestionnairePrint) {
  TestPrint<Questionnaire>("questionnaire-example");
  TestPrint<Questionnaire>("questionnaire-example-bluebook");
  TestPrint<Questionnaire>("questionnaire-example-f201-lifelines");
  TestPrint<Questionnaire>("questionnaire-example-gcs");
}

/** Test printing of the QuestionnaireResponse FHIR resource. */
TEST(JsonFormatTest, QuestionnaireResponsePrint) {
  TestPrint<QuestionnaireResponse>("questionnaireresponse-example");
  TestPrint<QuestionnaireResponse>("questionnaireresponse-example-bluebook");
  TestPrint<QuestionnaireResponse>(
      "questionnaireresponse-example-f201-lifelines");
  TestPrint<QuestionnaireResponse>("questionnaireresponse-example-gcs");
  TestPrint<QuestionnaireResponse>(
      "questionnaireresponse-example-ussg-fht-answers");
}

/** Test printing of the ReferralRequest FHIR resource. */
TEST(JsonFormatTest, ReferralRequestPrint) {
  TestPrint<ReferralRequest>("referralrequest-example");
}

/** Test printing of the RelatedPerson FHIR resource. */
TEST(JsonFormatTest, RelatedPersonPrint) {
  TestPrint<RelatedPerson>("relatedperson-example");
  TestPrint<RelatedPerson>("relatedperson-example-f001-sarah");
  TestPrint<RelatedPerson>("relatedperson-example-f002-ariadne");
  TestPrint<RelatedPerson>("relatedperson-example-peter");
}

/** Test printing of the RequestGroup FHIR resource. */
TEST(JsonFormatTest, RequestGroupPrint) {
  TestPrint<RequestGroup>("requestgroup-example");
  TestPrint<RequestGroup>("requestgroup-kdn5-example");
}

/** Test printing of the ResearchStudy FHIR resource. */
TEST(JsonFormatTest, ResearchStudyPrint) {
  TestPrint<ResearchStudy>("researchstudy-example");
}

/** Test printing of the ResearchSubject FHIR resource. */
TEST(JsonFormatTest, ResearchSubjectPrint) {
  TestPrint<ResearchSubject>("researchsubject-example");
}

/** Test printing of the RiskAssessment FHIR resource. */
TEST(JsonFormatTest, RiskAssessmentPrint) {
  TestPrint<RiskAssessment>("riskassessment-example");
  TestPrint<RiskAssessment>("riskassessment-example-cardiac");
  TestPrint<RiskAssessment>("riskassessment-example-population");
  TestPrint<RiskAssessment>("riskassessment-example-prognosis");
}

/** Test printing of the Schedule FHIR resource. */
TEST(JsonFormatTest, SchedulePrint) {
  TestPrint<Schedule>("schedule-example");
  TestPrint<Schedule>("schedule-provider-location1-example");
  TestPrint<Schedule>("schedule-provider-location2-example");
}

/** Test printing of the SearchParameter FHIR resource. */
TEST(JsonFormatTest, SearchParameterPrint) {
  TestPrint<SearchParameter>("searchparameter-example");
  TestPrint<SearchParameter>("searchparameter-example-extension");
  TestPrint<SearchParameter>("searchparameter-example-reference");
}

/** Test printing of the Sequence FHIR resource. */
TEST(JsonFormatTest, SequencePrint) {
  TestPrint<Sequence>("coord-0base-example");
  TestPrint<Sequence>("coord-1base-example");
  TestPrint<Sequence>("sequence-example");
  TestPrint<Sequence>("sequence-example-fda");
  TestPrint<Sequence>("sequence-example-fda-comparisons");
  TestPrint<Sequence>("sequence-example-fda-vcfeval");
  TestPrint<Sequence>("sequence-example-pgx-1");
  TestPrint<Sequence>("sequence-example-pgx-2");
  TestPrint<Sequence>("sequence-example-TPMT-one");
  TestPrint<Sequence>("sequence-example-TPMT-two");
  TestPrint<Sequence>("sequence-graphic-example-1");
  TestPrint<Sequence>("sequence-graphic-example-2");
  TestPrint<Sequence>("sequence-graphic-example-3");
  TestPrint<Sequence>("sequence-graphic-example-4");
  TestPrint<Sequence>("sequence-graphic-example-5");
}

/** Test printing of the ServiceDefinition FHIR resource. */
TEST(JsonFormatTest, ServiceDefinitionPrint) {
  TestPrint<ServiceDefinition>("servicedefinition-example");
}

/** Test printing of the Slot FHIR resource. */
TEST(JsonFormatTest, SlotPrint) {
  TestPrint<Slot>("slot-example");
  TestPrint<Slot>("slot-example-busy");
  TestPrint<Slot>("slot-example-tentative");
  TestPrint<Slot>("slot-example-unavailable");
}

/** Test printing of the Specimen FHIR resource. */
TEST(JsonFormatTest, SpecimenPrint) {
  TestPrint<Specimen>("specimen-example");
  TestPrint<Specimen>("specimen-example-isolate");
  TestPrint<Specimen>("specimen-example-serum");
  TestPrint<Specimen>("specimen-example-urine");
}

/** Test printing of the StructureDefinition FHIR resource. */
TEST(JsonFormatTest, StructureDefinitionPrint) {
  TestPrint<StructureDefinition>("structuredefinition-example");
}

/** Test printing of the StructureMap FHIR resource. */
TEST(JsonFormatTest, StructureMapPrint) {
  TestPrint<StructureMap>("structuremap-example");
}

/** Test printing of the Subscription FHIR resource. */
TEST(JsonFormatTest, SubscriptionPrint) {
  TestPrint<Subscription>("subscription-example");
  TestPrint<Subscription>("subscription-example-error");
}

/** Test printing of the Substance FHIR resource. */
TEST(JsonFormatTest, SubstancePrint) {
  TestPrint<Substance>("substance-example");
  TestPrint<Substance>("substance-example-amoxicillin-clavulanate");
  TestPrint<Substance>("substance-example-f201-dust");
  TestPrint<Substance>("substance-example-f202-staphylococcus");
  TestPrint<Substance>("substance-example-f203-potassium");
  TestPrint<Substance>("substance-example-silver-nitrate-product");
}

/** Test printing of the SupplyDelivery FHIR resource. */
TEST(JsonFormatTest, SupplyDeliveryPrint) {
  TestPrint<SupplyDelivery>("supplydelivery-example");
  TestPrint<SupplyDelivery>("supplydelivery-example-pumpdelivery");
}

/** Test printing of the SupplyRequest FHIR resource. */
TEST(JsonFormatTest, SupplyRequestPrint) {
  TestPrint<SupplyRequest>("supplyrequest-example-simpleorder");
}

/** Test printing of the Task FHIR resource. */
TEST(JsonFormatTest, TaskPrint) {
  TestPrint<Task>("task-example1");
  TestPrint<Task>("task-example2");
  TestPrint<Task>("task-example3");
  TestPrint<Task>("task-example4");
  TestPrint<Task>("task-example5");
  TestPrint<Task>("task-example6");
}

/** Test printing of the TestReport FHIR resource. */
TEST(JsonFormatTest, TestReportPrint) {
  TestPrint<TestReport>("testreport-example");
}

/** Test printing of the TestScript FHIR resource. */
TEST(JsonFormatTest, TestScriptPrint) {
  TestPrint<TestScript>("testscript-example");
  TestPrint<TestScript>("testscript-example-history");
  TestPrint<TestScript>("testscript-example-multisystem");
  TestPrint<TestScript>("testscript-example-readtest");
  TestPrint<TestScript>("testscript-example-rule");
  TestPrint<TestScript>("testscript-example-search");
  TestPrint<TestScript>("testscript-example-update");
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
  TestPrint<VisionPrescription>("visionprescription-example");
  TestPrint<VisionPrescription>("visionprescription-example-1");
}

}  // namespace

}  // namespace stu3
}  // namespace fhir
}  // namespace google
