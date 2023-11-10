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

#include "google/fhir/dstu2/json_format.h"

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>

#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/fhir/dstu2/primitive_handler.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/json/json_sax_handler.h"
#include "google/fhir/json/test_matchers.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "proto/google/fhir/proto/dstu2/codes.pb.h"
#include "proto/google/fhir/proto/dstu2/datatypes.pb.h"
#include "proto/google/fhir/proto/dstu2/resources.pb.h"

namespace google {
namespace fhir {
namespace dstu2 {

namespace {

using namespace google::fhir::dstu2::proto;  // NOLINT
using std::string;

static const char* const kTimeZoneString = "Australia/Sydney";

template <typename R>
absl::StatusOr<R> ParseJsonToProto(absl::string_view json_path) {
  const string json = ReadFile(json_path);
  absl::TimeZone tz;
  absl::LoadTimeZone(kTimeZoneString, &tz);
  return JsonFhirStringToProto<R>(json, tz);
}

template <typename R>
void TestParseWithFilepaths(absl::string_view proto_path,
                            absl::string_view json_path) {
  absl::StatusOr<R> from_json_status = ParseJsonToProto<R>(json_path);
  ASSERT_TRUE(from_json_status.ok()) << "Failed parsing: " << json_path << "\n"
                                     << from_json_status.status();
  R from_json = from_json_status.value();
  R from_disk = ReadProto<R>(proto_path);

  ::google::protobuf::util::MessageDifferencer differencer;
  string differences;
  differencer.ReportDifferencesToString(&differences);
  EXPECT_TRUE(differencer.Compare(from_disk, from_json)) << differences;
}

template <typename R>
void TestPrintWithFilepaths(absl::string_view proto_path,
                            absl::string_view json_path) {
  const R proto = ReadProto<R>(proto_path);
  absl::StatusOr<string> from_proto_status = PrettyPrintFhirToJsonString(proto);
  EXPECT_TRUE(from_proto_status.ok())
      << "Failed Printing on: " << proto_path << ": "
      << from_proto_status.status().message();

  internal::FhirJson from_proto, from_json;
  ASSERT_TRUE(
      internal::ParseJsonValue(from_proto_status.value(), from_proto).ok());
  ASSERT_TRUE(internal::ParseJsonValue(ReadFile(json_path), from_json).ok());
  EXPECT_THAT(from_proto, JsonEq(&from_json));
}

template <typename R>
void TestPairWithFilePaths(absl::string_view proto_path,
                           absl::string_view json_path) {
  TestParseWithFilepaths<R>(proto_path, json_path);
  TestPrintWithFilepaths<R>(proto_path, json_path);
}

template <typename R>
void TestPair(absl::string_view name) {
  const string proto_path =
      absl::StrCat("testdata/dstu2/examples/", name, ".prototxt");
  const string json_path =
      absl::StrCat("spec/hl7.fhir.core/1.0.2/package/", name, ".json");
  TestPairWithFilePaths<R>(proto_path, json_path);
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

TEST(JsonFormatR4Test, Ndjson) {
  TestPairWithFilePaths<Location>(
      "testdata/jsonformat/location_ndjson.prototxt",
      "testdata/jsonformat/location_ndjson.json");
}

TEST(JsonFormatDSTU2Test, NdjsonLocation) {
  const string proto_path = "testdata/jsonformat/location_ndjson.prototxt";
  const string json_path = "testdata/jsonformat/location_ndjson.json";
  TestPairWithFilePaths<Location>(proto_path, json_path);
}

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define JSON_FORMAT_TEST(type, filename)                    \
  TEST(JsonFormatDSTU2Test, CONCAT(type##_, __COUNTER__)) { \
    TestPair<type>(#filename);                              \
  }

// clang-format off
JSON_FORMAT_TEST(Account, Account-example);
JSON_FORMAT_TEST(AllergyIntolerance, AllergyIntolerance-example);
JSON_FORMAT_TEST(Appointment, Appointment-example);
JSON_FORMAT_TEST(Appointment, Appointment-examplereq);
JSON_FORMAT_TEST(AppointmentResponse, AppointmentResponse-example);
JSON_FORMAT_TEST(AppointmentResponse, AppointmentResponse-exampleresp);
JSON_FORMAT_TEST(AuditEvent, AuditEvent-example);
JSON_FORMAT_TEST(AuditEvent, AuditEvent-example-disclosure);
JSON_FORMAT_TEST(AuditEvent, AuditEvent-example-login);
JSON_FORMAT_TEST(AuditEvent, AuditEvent-example-logout);
JSON_FORMAT_TEST(AuditEvent, AuditEvent-example-media);
JSON_FORMAT_TEST(AuditEvent, AuditEvent-example-pixQuery);
JSON_FORMAT_TEST(AuditEvent, AuditEvent-example-rest);
JSON_FORMAT_TEST(AuditEvent, AuditEvent-example-search);
JSON_FORMAT_TEST(Basic, Basic-basic-example-narrative);
JSON_FORMAT_TEST(Binary, Binary-example);
JSON_FORMAT_TEST(BodySite, BodySite-example);
JSON_FORMAT_TEST(Bundle, Bundle-bundle-example);
JSON_FORMAT_TEST(CarePlan, CarePlan-example);
JSON_FORMAT_TEST(Claim, Claim-100150);
JSON_FORMAT_TEST(Claim, Claim-100151);
JSON_FORMAT_TEST(ClaimResponse, ClaimResponse-R3500);
JSON_FORMAT_TEST(ClinicalImpression, ClinicalImpression-example);
JSON_FORMAT_TEST(Communication, Communication-example);
JSON_FORMAT_TEST(CommunicationRequest, CommunicationRequest-example);
JSON_FORMAT_TEST(Composition, Composition-example);
JSON_FORMAT_TEST(ConceptMap, ConceptMap-v2-address-use);
JSON_FORMAT_TEST(ConceptMap, ConceptMap-v2-administrative-gender);
JSON_FORMAT_TEST(Condition, Condition-example);
JSON_FORMAT_TEST(Condition, Condition-example2);
JSON_FORMAT_TEST(Conformance, Conformance-example);
JSON_FORMAT_TEST(Contract, Contract-C-123);
JSON_FORMAT_TEST(Coverage, Coverage-7546D);
JSON_FORMAT_TEST(DataElement, DataElement-ElementDefinition.exampleX);
JSON_FORMAT_TEST(DataElement,
                 DataElement-ImplementationGuide.package.resource.exampleFor);
JSON_FORMAT_TEST(DetectedIssue, DetectedIssue-allergy);
JSON_FORMAT_TEST(DeviceComponent, DeviceComponent-example);
JSON_FORMAT_TEST(DeviceComponent, DeviceComponent-example-prodspec);
JSON_FORMAT_TEST(Device, Device-example);
JSON_FORMAT_TEST(Device, Device-example-pacemaker);
JSON_FORMAT_TEST(DeviceMetric, DeviceMetric-example);
JSON_FORMAT_TEST(DeviceUseRequest, DeviceUseRequest-example);
JSON_FORMAT_TEST(DeviceUseStatement, DeviceUseStatement-example);
JSON_FORMAT_TEST(DiagnosticOrder, DiagnosticOrder-example);
JSON_FORMAT_TEST(DiagnosticReport, DiagnosticReport-101);
JSON_FORMAT_TEST(DiagnosticReport, DiagnosticReport-102);
JSON_FORMAT_TEST(DiagnosticReport, DiagnosticReport-f001);
JSON_FORMAT_TEST(DocumentManifest, DocumentManifest-example);
JSON_FORMAT_TEST(DocumentReference, DocumentReference-example);
JSON_FORMAT_TEST(EligibilityRequest, EligibilityRequest-52345);
JSON_FORMAT_TEST(EligibilityResponse, EligibilityResponse-E2500);
JSON_FORMAT_TEST(Encounter, Encounter-example);
JSON_FORMAT_TEST(EnrollmentRequest, EnrollmentRequest-22345);
JSON_FORMAT_TEST(EnrollmentResponse, EnrollmentResponse-ER2500);
JSON_FORMAT_TEST(EpisodeOfCare, EpisodeOfCare-example);
JSON_FORMAT_TEST(ExplanationOfBenefit, ExplanationOfBenefit-R3500);
JSON_FORMAT_TEST(FamilyMemberHistory, FamilyMemberHistory-father);
JSON_FORMAT_TEST(Flag, Flag-example);
JSON_FORMAT_TEST(Flag, Flag-example-encounter);
JSON_FORMAT_TEST(Goal, Goal-example);
JSON_FORMAT_TEST(Group, Group-101);
JSON_FORMAT_TEST(HealthcareService, HealthcareService-example);
JSON_FORMAT_TEST(ImagingObjectSelection, ImagingObjectSelection-example);
JSON_FORMAT_TEST(ImagingStudy, ImagingStudy-example);
JSON_FORMAT_TEST(Immunization, Immunization-example);
JSON_FORMAT_TEST(ImmunizationRecommendation,
                 ImmunizationRecommendation-example);
JSON_FORMAT_TEST(ImplementationGuide, ImplementationGuide-example);
JSON_FORMAT_TEST(List, List-example);
JSON_FORMAT_TEST(List, List-example-empty);
JSON_FORMAT_TEST(Location, Location-1);
JSON_FORMAT_TEST(Media, Media-example);
JSON_FORMAT_TEST(MedicationAdministration,
                 MedicationAdministration-medadminexample01);
JSON_FORMAT_TEST(MedicationAdministration,
                 MedicationAdministration-medadminexample02);
JSON_FORMAT_TEST(MedicationAdministration,
                 MedicationAdministration-medadminexample03);
JSON_FORMAT_TEST(MedicationDispense, MedicationDispense-meddisp001);
JSON_FORMAT_TEST(MedicationOrder, MedicationOrder-f001);
JSON_FORMAT_TEST(MedicationOrder, MedicationOrder-f002);
JSON_FORMAT_TEST(MedicationOrder, MedicationOrder-f003);
JSON_FORMAT_TEST(Medication, Medication-medexample001);
JSON_FORMAT_TEST(Medication, Medication-medexample002);
JSON_FORMAT_TEST(Medication, Medication-medexample003);
JSON_FORMAT_TEST(MedicationStatement, MedicationStatement-example001);
JSON_FORMAT_TEST(MedicationStatement, MedicationStatement-example002);
JSON_FORMAT_TEST(MessageHeader,
                 MessageHeader-1cbdfb97-5859-48a4-8301-d54eab818d68);
JSON_FORMAT_TEST(NamingSystem, NamingSystem-example);
JSON_FORMAT_TEST(NamingSystem, NamingSystem-example-id);
JSON_FORMAT_TEST(NamingSystem, NamingSystem-example-replaced);
JSON_FORMAT_TEST(NutritionOrder, NutritionOrder-cardiacdiet);
JSON_FORMAT_TEST(NutritionOrder, NutritionOrder-diabeticdiet);
JSON_FORMAT_TEST(Observation, Observation-example);
JSON_FORMAT_TEST(Observation, Observation-genetics-example1-somatic);
JSON_FORMAT_TEST(Observation, Observation-genetics-example2-germline);
JSON_FORMAT_TEST(Observation, Observation-genetics-example3-mutationlist-1);
JSON_FORMAT_TEST(Observation, Observation-genetics-example3-mutationlist-2);
JSON_FORMAT_TEST(Observation, Observation-genetics-example3-mutationlist-3);
JSON_FORMAT_TEST(Observation, Observation-genetics-example3-mutationlist-4);
JSON_FORMAT_TEST(OperationDefinition, OperationDefinition-example);
JSON_FORMAT_TEST(OperationOutcome, OperationOutcome-101);
JSON_FORMAT_TEST(OperationOutcome, OperationOutcome-allok);
JSON_FORMAT_TEST(Order, Order-example);
JSON_FORMAT_TEST(OrderResponse, OrderResponse-example);
JSON_FORMAT_TEST(Organization, Organization-1);
JSON_FORMAT_TEST(Organization,
                 Organization-1832473e-2fe0-452d-abe9-3cdb9879522f);
JSON_FORMAT_TEST(Parameters, Parameters-example);
JSON_FORMAT_TEST(Patient, Patient-example);
JSON_FORMAT_TEST(PaymentNotice, PaymentNotice-77654);
JSON_FORMAT_TEST(PaymentReconciliation, PaymentReconciliation-ER2500);
JSON_FORMAT_TEST(Person, Person-example);
JSON_FORMAT_TEST(Practitioner, Practitioner-example);
JSON_FORMAT_TEST(Procedure, Procedure-example);
JSON_FORMAT_TEST(Procedure, Procedure-example-implant);
JSON_FORMAT_TEST(ProcedureRequest, ProcedureRequest-example);
JSON_FORMAT_TEST(ProcessRequest, ProcessRequest-1110);
JSON_FORMAT_TEST(ProcessRequest, ProcessRequest-1111);
JSON_FORMAT_TEST(ProcessRequest, ProcessRequest-1112);
JSON_FORMAT_TEST(ProcessResponse, ProcessResponse-SR2500);
JSON_FORMAT_TEST(Provenance, Provenance-example);
JSON_FORMAT_TEST(Questionnaire, Questionnaire-3141);
JSON_FORMAT_TEST(Questionnaire, Questionnaire-bb);
JSON_FORMAT_TEST(QuestionnaireResponse, QuestionnaireResponse-3141);
JSON_FORMAT_TEST(QuestionnaireResponse, QuestionnaireResponse-bb);
JSON_FORMAT_TEST(ReferralRequest, ReferralRequest-example);
JSON_FORMAT_TEST(RelatedPerson, RelatedPerson-benedicte);
JSON_FORMAT_TEST(RelatedPerson, RelatedPerson-f001);
JSON_FORMAT_TEST(RiskAssessment, RiskAssessment-cardiac);
JSON_FORMAT_TEST(RiskAssessment, RiskAssessment-genetic);
JSON_FORMAT_TEST(Schedule, Schedule-example);
JSON_FORMAT_TEST(SearchParameter, SearchParameter-example);
JSON_FORMAT_TEST(SearchParameter, SearchParameter-example-extension);
JSON_FORMAT_TEST(Slot, Slot-example);
JSON_FORMAT_TEST(Specimen, Specimen-101);
JSON_FORMAT_TEST(StructureDefinition, StructureDefinition-example);
JSON_FORMAT_TEST(Subscription, Subscription-example);
JSON_FORMAT_TEST(Subscription, Subscription-example-error);
JSON_FORMAT_TEST(Substance, Substance-example);
JSON_FORMAT_TEST(SupplyDelivery, SupplyDelivery-example);
JSON_FORMAT_TEST(SupplyRequest, SupplyRequest-example);
JSON_FORMAT_TEST(TestScript, TestScript-example);
JSON_FORMAT_TEST(ValueSet, ValueSet-list-example-codes);
JSON_FORMAT_TEST(ValueSet, ValueSet-valueset-sdc-profile-example);
JSON_FORMAT_TEST(VisionPrescription, VisionPrescription-33123);
// clang-format on

}  // namespace

}  // namespace dstu2
}  // namespace fhir
}  // namespace google
