//    Copyright 2018 Google Inc.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        https://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

package com.google.fhir.stu3;

import static com.google.common.truth.Truth.assertThat;

import com.google.fhir.common.JsonFormatTestBase;
import com.google.fhir.stu3.proto.Account;
import com.google.fhir.stu3.proto.ActivityDefinition;
import com.google.fhir.stu3.proto.AdverseEvent;
import com.google.fhir.stu3.proto.AllergyIntolerance;
import com.google.fhir.stu3.proto.Appointment;
import com.google.fhir.stu3.proto.AppointmentResponse;
import com.google.fhir.stu3.proto.AuditEvent;
import com.google.fhir.stu3.proto.Basic;
import com.google.fhir.stu3.proto.Binary;
import com.google.fhir.stu3.proto.BodySite;
import com.google.fhir.stu3.proto.Bundle;
import com.google.fhir.stu3.proto.CapabilityStatement;
import com.google.fhir.stu3.proto.CarePlan;
import com.google.fhir.stu3.proto.CareTeam;
import com.google.fhir.stu3.proto.ChargeItem;
import com.google.fhir.stu3.proto.Claim;
import com.google.fhir.stu3.proto.ClaimResponse;
import com.google.fhir.stu3.proto.ClinicalImpression;
import com.google.fhir.stu3.proto.CodeSystem;
import com.google.fhir.stu3.proto.Communication;
import com.google.fhir.stu3.proto.CommunicationRequest;
import com.google.fhir.stu3.proto.CompartmentDefinition;
import com.google.fhir.stu3.proto.Composition;
import com.google.fhir.stu3.proto.ConceptMap;
import com.google.fhir.stu3.proto.Condition;
import com.google.fhir.stu3.proto.Consent;
import com.google.fhir.stu3.proto.Contract;
import com.google.fhir.stu3.proto.Coverage;
import com.google.fhir.stu3.proto.DataElement;
import com.google.fhir.stu3.proto.DetectedIssue;
import com.google.fhir.stu3.proto.Device;
import com.google.fhir.stu3.proto.DeviceComponent;
import com.google.fhir.stu3.proto.DeviceMetric;
import com.google.fhir.stu3.proto.DeviceRequest;
import com.google.fhir.stu3.proto.DeviceUseStatement;
import com.google.fhir.stu3.proto.DiagnosticReport;
import com.google.fhir.stu3.proto.DocumentManifest;
import com.google.fhir.stu3.proto.DocumentReference;
import com.google.fhir.stu3.proto.EligibilityRequest;
import com.google.fhir.stu3.proto.EligibilityResponse;
import com.google.fhir.stu3.proto.Encounter;
import com.google.fhir.stu3.proto.Endpoint;
import com.google.fhir.stu3.proto.EnrollmentRequest;
import com.google.fhir.stu3.proto.EnrollmentResponse;
import com.google.fhir.stu3.proto.EpisodeOfCare;
import com.google.fhir.stu3.proto.ExpansionProfile;
import com.google.fhir.stu3.proto.ExplanationOfBenefit;
import com.google.fhir.stu3.proto.FamilyMemberHistory;
import com.google.fhir.stu3.proto.Flag;
import com.google.fhir.stu3.proto.Goal;
import com.google.fhir.stu3.proto.GraphDefinition;
import com.google.fhir.stu3.proto.Group;
import com.google.fhir.stu3.proto.GuidanceResponse;
import com.google.fhir.stu3.proto.HealthcareService;
import com.google.fhir.stu3.proto.ImagingManifest;
import com.google.fhir.stu3.proto.ImagingStudy;
import com.google.fhir.stu3.proto.Immunization;
import com.google.fhir.stu3.proto.ImmunizationRecommendation;
import com.google.fhir.stu3.proto.ImplementationGuide;
import com.google.fhir.stu3.proto.Library;
import com.google.fhir.stu3.proto.Linkage;
import com.google.fhir.stu3.proto.List;
import com.google.fhir.stu3.proto.Location;
import com.google.fhir.stu3.proto.Measure;
import com.google.fhir.stu3.proto.MeasureReport;
import com.google.fhir.stu3.proto.Media;
import com.google.fhir.stu3.proto.Medication;
import com.google.fhir.stu3.proto.MedicationAdministration;
import com.google.fhir.stu3.proto.MedicationDispense;
import com.google.fhir.stu3.proto.MedicationRequest;
import com.google.fhir.stu3.proto.MedicationStatement;
import com.google.fhir.stu3.proto.MessageDefinition;
import com.google.fhir.stu3.proto.MessageHeader;
import com.google.fhir.stu3.proto.NamingSystem;
import com.google.fhir.stu3.proto.NutritionOrder;
import com.google.fhir.stu3.proto.Observation;
import com.google.fhir.stu3.proto.OperationDefinition;
import com.google.fhir.stu3.proto.OperationOutcome;
import com.google.fhir.stu3.proto.Organization;
import com.google.fhir.stu3.proto.Parameters;
import com.google.fhir.stu3.proto.Patient;
import com.google.fhir.stu3.proto.PaymentNotice;
import com.google.fhir.stu3.proto.PaymentReconciliation;
import com.google.fhir.stu3.proto.Person;
import com.google.fhir.stu3.proto.PlanDefinition;
import com.google.fhir.stu3.proto.Practitioner;
import com.google.fhir.stu3.proto.PractitionerRole;
import com.google.fhir.stu3.proto.Procedure;
import com.google.fhir.stu3.proto.ProcedureRequest;
import com.google.fhir.stu3.proto.ProcessRequest;
import com.google.fhir.stu3.proto.ProcessResponse;
import com.google.fhir.stu3.proto.Provenance;
import com.google.fhir.stu3.proto.Questionnaire;
import com.google.fhir.stu3.proto.QuestionnaireResponse;
import com.google.fhir.stu3.proto.ReferralRequest;
import com.google.fhir.stu3.proto.RelatedPerson;
import com.google.fhir.stu3.proto.RequestGroup;
import com.google.fhir.stu3.proto.ResearchStudy;
import com.google.fhir.stu3.proto.ResearchSubject;
import com.google.fhir.stu3.proto.RiskAssessment;
import com.google.fhir.stu3.proto.Schedule;
import com.google.fhir.stu3.proto.SearchParameter;
import com.google.fhir.stu3.proto.Sequence;
import com.google.fhir.stu3.proto.ServiceDefinition;
import com.google.fhir.stu3.proto.Slot;
import com.google.fhir.stu3.proto.Specimen;
import com.google.fhir.stu3.proto.StructureDefinition;
import com.google.fhir.stu3.proto.StructureMap;
import com.google.fhir.stu3.proto.Subscription;
import com.google.fhir.stu3.proto.Substance;
import com.google.fhir.stu3.proto.SupplyDelivery;
import com.google.fhir.stu3.proto.SupplyRequest;
import com.google.fhir.stu3.proto.Task;
import com.google.fhir.stu3.proto.TestReport;
import com.google.fhir.stu3.proto.TestScript;
import com.google.fhir.stu3.proto.ValueSet;
import com.google.fhir.stu3.proto.VisionPrescription;
import java.io.IOException;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link JsonFormat}. */
@RunWith(JUnit4.class)
public class JsonFormatTest extends JsonFormatTestBase {

  public JsonFormatTest() {
    super("stu3", "3.0.1");
  }

  @Before
  public void setUp() throws IOException {
    setUpParser();
  }

  /** Test parsing JSON edge cases. */
  @Test
  public void parseEdgeCases() throws Exception {
    testParse("Patient-null", Patient.newBuilder());
  }

  /**
   * Test printing JSON edge cases. Since this json file is not sorted in any particular way, we
   * sort the json objects directly and compare them instead of comparing the raw strings.
   */
  @Test
  public void printEdgeCases() throws Exception {
    String jsonGolden = loadJson("spec/hl7.fhir.core/3.0.1/package/Patient-null.json");
    Patient.Builder patient = Patient.newBuilder();
    mergeText("examples/Patient-null.prototxt", patient);
    String jsonTest = jsonPrinter.print(patient);
    assertThat(canonicalizeJson(jsonTest)).isEqualTo(canonicalizeJson(jsonGolden));
  }

  /** Test the analytics output format. */
  @Test
  public void convertForAnalytics() throws Exception {
    testConvertForAnalytics("Composition-example", Composition.newBuilder());
    testConvertForAnalytics("Encounter-home", Encounter.newBuilder());
    testConvertForAnalytics("Observation-example-genetics-1", Observation.newBuilder());
    testConvertForAnalytics("patient-example", Patient.newBuilder());
  }

  /** Test parsing to a profile */
  // TODO: Profiles aren't supported yet in Java.
  //   @Test
  //   public void testParseSingleArrayElementIntoSingularField() throws Exception {
  //     TestPatient.Builder testPatient = TestPatient.newBuilder();
  //     jsonParser.merge(loadJson("testdata/stu3/profiles/test_patient.json"), testPatient);
  //
  //     // Parse the proto text version of the input.
  //     TestPatient.Builder textBuilder = TestPatient.newBuilder();
  //     mergeText("profiles/test_patient-profiled-testpatient.prototxt", textBuilder);
  //     assertThat(testPatient.build().toString()).isEqualTo(textBuilder.build().toString());
  //
  //     // This test is only meaningful because Patient.name is singular in the
  //     // profile, unlike in the unprofiled resource.  This means it has array
  //     // syntax in json, but singular syntax in proto.
  //     // Access the proto field directly, so it will be a compile-time failure if
  //     // that changes.
  //     assertThat(testPatient.getName().getGiven(0).getValue()).isEqualTo("Duck");
  //   }

  /* Resource tests start here. */

  /** Test parsing of the Account FHIR resource. */
  @Test
  public void parseAccount() throws Exception {
    testParse("Account-example", Account.newBuilder());
    testParse("Account-ewg", Account.newBuilder());
  }

  /** Test printing of the Account FHIR resource. */
  @Test
  public void printAccount() throws Exception {
    testPrint("Account-example", Account.newBuilder());
    testPrint("Account-ewg", Account.newBuilder());
  }

  /** Test parsing of the ActivityDefinition FHIR resource. */
  @Test
  public void parseActivityDefinition() throws Exception {
    testParse(
        "ActivityDefinition-referralPrimaryCareMentalHealth", ActivityDefinition.newBuilder());
    testParse("ActivityDefinition-citalopramPrescription", ActivityDefinition.newBuilder());
    testParse(
        "ActivityDefinition-referralPrimaryCareMentalHealth-initial",
        ActivityDefinition.newBuilder());
    testParse("ActivityDefinition-heart-valve-replacement", ActivityDefinition.newBuilder());
    testParse("ActivityDefinition-blood-tubes-supply", ActivityDefinition.newBuilder());
  }

  /** Test printing of the ActivityDefinition FHIR resource. */
  @Test
  public void printActivityDefinition() throws Exception {
    testPrint(
        "ActivityDefinition-referralPrimaryCareMentalHealth", ActivityDefinition.newBuilder());
    testPrint("ActivityDefinition-citalopramPrescription", ActivityDefinition.newBuilder());
    testPrint(
        "ActivityDefinition-referralPrimaryCareMentalHealth-initial",
        ActivityDefinition.newBuilder());
    testPrint("ActivityDefinition-heart-valve-replacement", ActivityDefinition.newBuilder());
    testPrint("ActivityDefinition-blood-tubes-supply", ActivityDefinition.newBuilder());
  }

  /** Test parsing of the AdverseEvent FHIR resource. */
  @Test
  public void parseAdverseEvent() throws Exception {
    testParse("AdverseEvent-example", AdverseEvent.newBuilder());
  }

  /** Test printing of the AdverseEvent FHIR resource. */
  @Test
  public void printAdverseEvent() throws Exception {
    testPrint("AdverseEvent-example", AdverseEvent.newBuilder());
  }

  /** Test parsing of the AllergyIntolerance FHIR resource. */
  @Test
  public void parseAllergyIntolerance() throws Exception {
    testParse("AllergyIntolerance-example", AllergyIntolerance.newBuilder());
  }

  /** Test printing of the AllergyIntolerance FHIR resource. */
  @Test
  public void printAllergyIntolerance() throws Exception {
    testPrint("AllergyIntolerance-example", AllergyIntolerance.newBuilder());
  }

  /** Test parsing of the Appointment FHIR resource. */
  @Test
  public void parseAppointment() throws Exception {
    testParse("Appointment-example", Appointment.newBuilder());
    testParse("Appointment-2docs", Appointment.newBuilder());
    testParse("Appointment-examplereq", Appointment.newBuilder());
  }

  /** Test printing of the Appointment FHIR resource. */
  @Test
  public void printAppointment() throws Exception {
    testPrint("Appointment-example", Appointment.newBuilder());
    testPrint("Appointment-2docs", Appointment.newBuilder());
    testPrint("Appointment-examplereq", Appointment.newBuilder());
  }

  /** Test parsing of the AppointmentResponse FHIR resource. */
  @Test
  public void parseAppointmentResponse() throws Exception {
    testParse("AppointmentResponse-example", AppointmentResponse.newBuilder());
    testParse("AppointmentResponse-exampleresp", AppointmentResponse.newBuilder());
  }

  /** Test printing of the AppointmentResponse FHIR resource. */
  @Test
  public void printAppointmentResponse() throws Exception {
    testPrint("AppointmentResponse-example", AppointmentResponse.newBuilder());
    testPrint("AppointmentResponse-exampleresp", AppointmentResponse.newBuilder());
  }

  /** Test parsing of the AuditEvent FHIR resource. */
  @Test
  public void parseAuditEvent() throws Exception {
    testParse("AuditEvent-example", AuditEvent.newBuilder());
    testParse("AuditEvent-example-disclosure", AuditEvent.newBuilder());
    testParse("AuditEvent-example-login", AuditEvent.newBuilder());
    testParse("AuditEvent-example-logout", AuditEvent.newBuilder());
    testParse("AuditEvent-example-media", AuditEvent.newBuilder());
    testParse("AuditEvent-example-pixQuery", AuditEvent.newBuilder());
    testParse("AuditEvent-example-search", AuditEvent.newBuilder());
    testParse("AuditEvent-example-rest", AuditEvent.newBuilder());
  }

  /** Test printing of the AuditEvent FHIR resource. */
  @Test
  public void printAuditEvent() throws Exception {
    testPrint("AuditEvent-example", AuditEvent.newBuilder());
    testPrint("AuditEvent-example-disclosure", AuditEvent.newBuilder());
    testPrint("AuditEvent-example-login", AuditEvent.newBuilder());
    testPrint("AuditEvent-example-logout", AuditEvent.newBuilder());
    testPrint("AuditEvent-example-media", AuditEvent.newBuilder());
    testPrint("AuditEvent-example-pixQuery", AuditEvent.newBuilder());
    testPrint("AuditEvent-example-search", AuditEvent.newBuilder());
    testPrint("AuditEvent-example-rest", AuditEvent.newBuilder());
  }

  /** Test parsing of the Basic FHIR resource. */
  @Test
  public void parseBasic() throws Exception {
    testParse("Basic-referral", Basic.newBuilder());
    testParse("Basic-classModel", Basic.newBuilder());
    testParse("Basic-basic-example-narrative", Basic.newBuilder());
  }

  /** Test printing of the Basic FHIR resource. */
  @Test
  public void printBasic() throws Exception {
    testPrint("Basic-referral", Basic.newBuilder());
    testPrint("Basic-classModel", Basic.newBuilder());
    testPrint("Basic-basic-example-narrative", Basic.newBuilder());
  }

  /** Test parsing of the Binary FHIR resource. */
  @Test
  public void parseBinary() throws Exception {
    testParse("Binary-example", Binary.newBuilder());
  }

  /** Test printing of the Binary FHIR resource. */
  @Test
  public void printBinary() throws Exception {
    testPrint("Binary-example", Binary.newBuilder());
  }

  /** Test parsing of the BodySite FHIR resource. */
  @Test
  public void parseBodySite() throws Exception {
    testParse("BodySite-fetus", BodySite.newBuilder());
    testParse("BodySite-skin-patch", BodySite.newBuilder());
    testParse("BodySite-tumor", BodySite.newBuilder());
  }

  /** Test printing of the BodySite FHIR resource. */
  @Test
  public void printBodySite() throws Exception {
    testPrint("BodySite-fetus", BodySite.newBuilder());
    testPrint("BodySite-skin-patch", BodySite.newBuilder());
    testPrint("BodySite-tumor", BodySite.newBuilder());
  }

  /** Test parsing of the Bundle FHIR resource. */
  @Test
  public void parseBundle() throws Exception {
    testParse("Bundle-bundle-example", Bundle.newBuilder());
    testParse("Bundle-72ac8493-52ac-41bd-8d5d-7258c289b5ea", Bundle.newBuilder());
    testParse("Bundle-hla-1", Bundle.newBuilder());
    testParse("Bundle-father", Bundle.newBuilder());
    testParse("Bundle-b0a5e4277-83c4-4adb-87e2-e3efe3369b6f", Bundle.newBuilder());
    testParse("Bundle-3ad0687e-f477-468c-afd5-fcc2bf897819", Bundle.newBuilder());
    testParse("patient-examples-cypress-template", Bundle.newBuilder());
    testParse("Bundle-b248b1b2-1686-4b94-9936-37d7a5f94b51", Bundle.newBuilder());
    testParse("Bundle-3ad0687e-f477-468c-afd5-fcc2bf897809", Bundle.newBuilder());
    testParse("Bundle-3ad0687e-f477-468c-afd5-fcc2bf897808", Bundle.newBuilder());
    testParse("Bundle-ussg-fht", Bundle.newBuilder());
    testParse("Bundle-xds", Bundle.newBuilder());
  }

  /** Test printing of the Bundle FHIR resource. */
  @Test
  public void printBundle() throws Exception {
    testPrint("Bundle-bundle-example", Bundle.newBuilder());
    testPrint("Bundle-72ac8493-52ac-41bd-8d5d-7258c289b5ea", Bundle.newBuilder());
    testPrint("Bundle-hla-1", Bundle.newBuilder());
    testPrint("Bundle-father", Bundle.newBuilder());
    testPrint("Bundle-b0a5e4277-83c4-4adb-87e2-e3efe3369b6f", Bundle.newBuilder());
    testPrint("Bundle-3ad0687e-f477-468c-afd5-fcc2bf897819", Bundle.newBuilder());
    testPrint("patient-examples-cypress-template", Bundle.newBuilder());
    testPrint("Bundle-b248b1b2-1686-4b94-9936-37d7a5f94b51", Bundle.newBuilder());
    testPrint("Bundle-3ad0687e-f477-468c-afd5-fcc2bf897809", Bundle.newBuilder());
    testPrint("Bundle-3ad0687e-f477-468c-afd5-fcc2bf897808", Bundle.newBuilder());
    testPrint("Bundle-ussg-fht", Bundle.newBuilder());
    testPrint("Bundle-xds", Bundle.newBuilder());
  }

  /** Test parsing of the CapabilityStatement FHIR resource. */
  @Test
  public void parseCapabilityStatement() throws Exception {
    testParse("CapabilityStatement-example", CapabilityStatement.newBuilder());
    testParse("CapabilityStatement-phr", CapabilityStatement.newBuilder());
  }

  /** Test printing of the CapabilityStatement FHIR resource. */
  @Test
  public void printCapabilityStatement() throws Exception {
    testPrint("CapabilityStatement-example", CapabilityStatement.newBuilder());
    testPrint("CapabilityStatement-phr", CapabilityStatement.newBuilder());
  }

  /** Test parsing of the CarePlan FHIR resource. */
  @Test
  public void parseCarePlan() throws Exception {
    testParse("CarePlan-example", CarePlan.newBuilder());
    testParse("CarePlan-f001", CarePlan.newBuilder());
    testParse("CarePlan-f002", CarePlan.newBuilder());
    testParse("CarePlan-f003", CarePlan.newBuilder());
    testParse("CarePlan-f201", CarePlan.newBuilder());
    testParse("CarePlan-f202", CarePlan.newBuilder());
    testParse("CarePlan-f203", CarePlan.newBuilder());
    testParse("CarePlan-gpvisit", CarePlan.newBuilder());
    testParse("CarePlan-integrate", CarePlan.newBuilder());
    testParse("CarePlan-obesity-narrative", CarePlan.newBuilder());
    testParse("CarePlan-preg", CarePlan.newBuilder());
  }

  /** Test printing of the CarePlan FHIR resource. */
  @Test
  public void printCarePlan() throws Exception {
    testPrint("CarePlan-example", CarePlan.newBuilder());
    testPrint("CarePlan-f001", CarePlan.newBuilder());
    testPrint("CarePlan-f002", CarePlan.newBuilder());
    testPrint("CarePlan-f003", CarePlan.newBuilder());
    testPrint("CarePlan-f201", CarePlan.newBuilder());
    testPrint("CarePlan-f202", CarePlan.newBuilder());
    testPrint("CarePlan-f203", CarePlan.newBuilder());
    testPrint("CarePlan-gpvisit", CarePlan.newBuilder());
    testPrint("CarePlan-integrate", CarePlan.newBuilder());
    testPrint("CarePlan-obesity-narrative", CarePlan.newBuilder());
    testPrint("CarePlan-preg", CarePlan.newBuilder());
  }

  /** Test parsing of the CareTeam FHIR resource. */
  @Test
  public void parseCareTeam() throws Exception {
    testParse("CareTeam-example", CareTeam.newBuilder());
  }

  /** Test printing of the CareTeam FHIR resource. */
  @Test
  public void printCareTeam() throws Exception {
    testPrint("CareTeam-example", CareTeam.newBuilder());
  }

  /** Test parsing of the ChargeItem FHIR resource. */
  @Test
  public void parseChargeItem() throws Exception {
    testParse("ChargeItem-example", ChargeItem.newBuilder());
  }

  /** Test printing of the ChargeItem FHIR resource. */
  @Test
  public void printChargeItem() throws Exception {
    testPrint("ChargeItem-example", ChargeItem.newBuilder());
  }

  /** Test parsing of the Claim FHIR resource. */
  @Test
  public void parseClaim() throws Exception {
    testParse("Claim-100150", Claim.newBuilder());
    testParse("Claim-960150", Claim.newBuilder());
    testParse("Claim-960151", Claim.newBuilder());
    testParse("Claim-100151", Claim.newBuilder());
    testParse("Claim-100156", Claim.newBuilder());
    testParse("Claim-100152", Claim.newBuilder());
    testParse("Claim-100155", Claim.newBuilder());
    testParse("Claim-100154", Claim.newBuilder());
    testParse("Claim-100153", Claim.newBuilder());
    testParse("Claim-760150", Claim.newBuilder());
    testParse("Claim-760152", Claim.newBuilder());
    testParse("Claim-760151", Claim.newBuilder());
    testParse("Claim-860150", Claim.newBuilder());
    testParse("Claim-660150", Claim.newBuilder());
    testParse("Claim-660151", Claim.newBuilder());
    testParse("Claim-660152", Claim.newBuilder());
  }

  /** Test printing of the Claim FHIR resource. */
  @Test
  public void printClaim() throws Exception {
    testPrint("Claim-100150", Claim.newBuilder());
    testPrint("Claim-960150", Claim.newBuilder());
    testPrint("Claim-960151", Claim.newBuilder());
    testPrint("Claim-100151", Claim.newBuilder());
    testPrint("Claim-100156", Claim.newBuilder());
    testPrint("Claim-100152", Claim.newBuilder());
    testPrint("Claim-100155", Claim.newBuilder());
    testPrint("Claim-100154", Claim.newBuilder());
    testPrint("Claim-100153", Claim.newBuilder());
    testPrint("Claim-760150", Claim.newBuilder());
    testPrint("Claim-760152", Claim.newBuilder());
    testPrint("Claim-760151", Claim.newBuilder());
    testPrint("Claim-860150", Claim.newBuilder());
    testPrint("Claim-660150", Claim.newBuilder());
    testPrint("Claim-660151", Claim.newBuilder());
    testPrint("Claim-660152", Claim.newBuilder());
  }

  /** Test parsing of the ClaimResponse FHIR resource. */
  @Test
  public void parseClaimResponse() throws Exception {
    testParse("ClaimResponse-R3500", ClaimResponse.newBuilder());
  }

  /** Test printing of the ClaimResponse FHIR resource. */
  @Test
  public void printClaimResponse() throws Exception {
    testPrint("ClaimResponse-R3500", ClaimResponse.newBuilder());
  }

  /** Test parsing of the ClinicalImpression FHIR resource. */
  @Test
  public void parseClinicalImpression() throws Exception {
    testParse("ClinicalImpression-example", ClinicalImpression.newBuilder());
  }

  /** Test printing of the ClinicalImpression FHIR resource. */
  @Test
  public void printClinicalImpression() throws Exception {
    testPrint("ClinicalImpression-example", ClinicalImpression.newBuilder());
  }

  /** Test parsing of the CodeSystem FHIR resource. */
  @Test
  public void parseCodeSystem() throws Exception {
    testParse("codesystem-example", CodeSystem.newBuilder());
    testParse("codesystem-example-summary", CodeSystem.newBuilder());
    testParse("codesystem-list-example-codes", CodeSystem.newBuilder());
  }

  /** Test printing of the CodeSystem FHIR resource. */
  @Test
  public void printCodeSystem() throws Exception {
    testPrint("codesystem-example", CodeSystem.newBuilder());
    testPrint("codesystem-example-summary", CodeSystem.newBuilder());
    testPrint("codesystem-list-example-codes", CodeSystem.newBuilder());
  }

  /** Test parsing of the Communication FHIR resource. */
  @Test
  public void parseCommunication() throws Exception {
    testParse("Communication-example", Communication.newBuilder());
    testParse("Communication-fm-attachment", Communication.newBuilder());
    testParse("Communication-fm-solicited", Communication.newBuilder());
  }

  /** Test printing of the Communication FHIR resource. */
  @Test
  public void printCommunication() throws Exception {
    testPrint("Communication-example", Communication.newBuilder());
    testPrint("Communication-fm-attachment", Communication.newBuilder());
    testPrint("Communication-fm-solicited", Communication.newBuilder());
  }

  /** Test parsing of the CommunicationRequest FHIR resource. */
  @Test
  public void parseCommunicationRequest() throws Exception {
    testParse("CommunicationRequest-example", CommunicationRequest.newBuilder());
    testParse("CommunicationRequest-fm-solicit", CommunicationRequest.newBuilder());
  }

  /** Test printing of the CommunicationRequest FHIR resource. */
  @Test
  public void printCommunicationRequest() throws Exception {
    testPrint("CommunicationRequest-example", CommunicationRequest.newBuilder());
    testPrint("CommunicationRequest-fm-solicit", CommunicationRequest.newBuilder());
  }

  /** Test parsing of the CompartmentDefinition FHIR resource. */
  @Test
  public void parseCompartmentDefinition() throws Exception {
    testParse("CompartmentDefinition-example", CompartmentDefinition.newBuilder());
  }

  /** Test printing of the CompartmentDefinition FHIR resource. */
  @Test
  public void printCompartmentDefinition() throws Exception {
    testPrint("CompartmentDefinition-example", CompartmentDefinition.newBuilder());
  }

  /** Test parsing of the Composition FHIR resource. */
  @Test
  public void parseComposition() throws Exception {
    testParse("Composition-example", Composition.newBuilder());
  }

  /** Test printing of the Composition FHIR resource. */
  @Test
  public void printComposition() throws Exception {
    testPrint("Composition-example", Composition.newBuilder());
  }

  /** Test parsing of the ConceptMap FHIR resource. */
  @Test
  public void parseConceptMap() throws Exception {
    testParse("conceptmap-example", ConceptMap.newBuilder());
    testParse("conceptmap-example-2", ConceptMap.newBuilder());
    testParse("conceptmap-example-specimen-type", ConceptMap.newBuilder());
  }

  /** Test printing of the ConceptMap FHIR resource. */
  @Test
  public void printConceptMap() throws Exception {
    testPrint("conceptmap-example", ConceptMap.newBuilder());
    testPrint("conceptmap-example-2", ConceptMap.newBuilder());
    testPrint("conceptmap-example-specimen-type", ConceptMap.newBuilder());
  }

  /** Test parsing of the Condition FHIR resource. */
  @Test
  public void parseCondition() throws Exception {
    testParse("Condition-example", Condition.newBuilder());
    testParse("Condition-example2", Condition.newBuilder());
    testParse("Condition-f001", Condition.newBuilder());
    testParse("Condition-f002", Condition.newBuilder());
    testParse("Condition-f003", Condition.newBuilder());
    testParse("Condition-f201", Condition.newBuilder());
    testParse("Condition-f202", Condition.newBuilder());
    testParse("Condition-f203", Condition.newBuilder());
    testParse("Condition-f204", Condition.newBuilder());
    testParse("Condition-f205", Condition.newBuilder());
    testParse("Condition-family-history", Condition.newBuilder());
    testParse("Condition-stroke", Condition.newBuilder());
  }

  /** Test printing of the Condition FHIR resource. */
  @Test
  public void printCondition() throws Exception {
    testPrint("Condition-example", Condition.newBuilder());
    testPrint("Condition-example2", Condition.newBuilder());
    testPrint("Condition-f001", Condition.newBuilder());
    testPrint("Condition-f002", Condition.newBuilder());
    testPrint("Condition-f003", Condition.newBuilder());
    testPrint("Condition-f201", Condition.newBuilder());
    testPrint("Condition-f202", Condition.newBuilder());
    testPrint("Condition-f203", Condition.newBuilder());
    testPrint("Condition-f204", Condition.newBuilder());
    testPrint("Condition-f205", Condition.newBuilder());
    testPrint("Condition-family-history", Condition.newBuilder());
    testPrint("Condition-stroke", Condition.newBuilder());
  }

  /** Test parsing of the Consent FHIR resource. */
  @Test
  public void parseConsent() throws Exception {
    testParse("Consent-consent-example-basic", Consent.newBuilder());
    testParse("Consent-consent-example-Emergency", Consent.newBuilder());
    testParse("Consent-consent-example-grantor", Consent.newBuilder());
    testParse("Consent-consent-example-notAuthor", Consent.newBuilder());
    testParse("Consent-consent-example-notOrg", Consent.newBuilder());
    testParse("Consent-consent-example-notThem", Consent.newBuilder());
    testParse("Consent-consent-example-notThis", Consent.newBuilder());
    testParse("Consent-consent-example-notTime", Consent.newBuilder());
    testParse("Consent-consent-example-Out", Consent.newBuilder());
    testParse("Consent-consent-example-pkb", Consent.newBuilder());
    testParse("Consent-consent-example-signature", Consent.newBuilder());
    testParse("Consent-consent-example-smartonfhir", Consent.newBuilder());
  }

  /** Test printing of the Consent FHIR resource. */
  @Test
  public void printConsent() throws Exception {
    testPrint("Consent-consent-example-basic", Consent.newBuilder());
    testPrint("Consent-consent-example-Emergency", Consent.newBuilder());
    testPrint("Consent-consent-example-grantor", Consent.newBuilder());
    testPrint("Consent-consent-example-notAuthor", Consent.newBuilder());
    testPrint("Consent-consent-example-notOrg", Consent.newBuilder());
    testPrint("Consent-consent-example-notThem", Consent.newBuilder());
    testPrint("Consent-consent-example-notThis", Consent.newBuilder());
    testPrint("Consent-consent-example-notTime", Consent.newBuilder());
    testPrint("Consent-consent-example-Out", Consent.newBuilder());
    testPrint("Consent-consent-example-pkb", Consent.newBuilder());
    testPrint("Consent-consent-example-signature", Consent.newBuilder());
    testPrint("Consent-consent-example-smartonfhir", Consent.newBuilder());
  }

  /** Test parsing of the Contract FHIR resource. */
  @Test
  public void parseContract() throws Exception {
    testParse("Contract-C-123", Contract.newBuilder());
    testParse("Contract-C-2121", Contract.newBuilder());
    testParse("Contract-pcd-example-notAuthor", Contract.newBuilder());
    testParse("Contract-pcd-example-notLabs", Contract.newBuilder());
    testParse("Contract-pcd-example-notOrg", Contract.newBuilder());
    testParse("Contract-pcd-example-notThem", Contract.newBuilder());
    testParse("Contract-pcd-example-notThis", Contract.newBuilder());
  }

  /** Test printing of the Contract FHIR resource. */
  @Test
  public void printContract() throws Exception {
    testPrint("Contract-C-123", Contract.newBuilder());
    testPrint("Contract-C-2121", Contract.newBuilder());
    testPrint("Contract-pcd-example-notAuthor", Contract.newBuilder());
    testPrint("Contract-pcd-example-notLabs", Contract.newBuilder());
    testPrint("Contract-pcd-example-notOrg", Contract.newBuilder());
    testPrint("Contract-pcd-example-notThem", Contract.newBuilder());
    testPrint("Contract-pcd-example-notThis", Contract.newBuilder());
  }

  /** Test parsing of the Coverage FHIR resource. */
  @Test
  public void parseCoverage() throws Exception {
    testParse("Coverage-9876B1", Coverage.newBuilder());
    testParse("Coverage-7546D", Coverage.newBuilder());
    testParse("Coverage-7547E", Coverage.newBuilder());
    testParse("Coverage-SP1234", Coverage.newBuilder());
  }

  /** Test printing of the Coverage FHIR resource. */
  @Test
  public void printCoverage() throws Exception {
    testPrint("Coverage-9876B1", Coverage.newBuilder());
    testPrint("Coverage-7546D", Coverage.newBuilder());
    testPrint("Coverage-7547E", Coverage.newBuilder());
    testPrint("Coverage-SP1234", Coverage.newBuilder());
  }

  /** Test parsing of the DataElement FHIR resource. */
  @Test
  public void parseDataElement() throws Exception {
    testParse("DataElement-gender", DataElement.newBuilder());
    testParse("DataElement-prothrombin", DataElement.newBuilder());
  }

  /** Test printing of the DataElement FHIR resource. */
  @Test
  public void printDataElement() throws Exception {
    testPrint("DataElement-gender", DataElement.newBuilder());
    testPrint("DataElement-prothrombin", DataElement.newBuilder());
  }

  /** Test parsing of the DetectedIssue FHIR resource. */
  @Test
  public void parseDetectedIssue() throws Exception {
    testParse("DetectedIssue-ddi", DetectedIssue.newBuilder());
    testParse("DetectedIssue-allergy", DetectedIssue.newBuilder());
    testParse("DetectedIssue-duplicate", DetectedIssue.newBuilder());
    testParse("DetectedIssue-lab", DetectedIssue.newBuilder());
  }

  /** Test printing of the DetectedIssue FHIR resource. */
  @Test
  public void printDetectedIssue() throws Exception {
    testPrint("DetectedIssue-ddi", DetectedIssue.newBuilder());
    testPrint("DetectedIssue-allergy", DetectedIssue.newBuilder());
    testPrint("DetectedIssue-duplicate", DetectedIssue.newBuilder());
    testPrint("DetectedIssue-lab", DetectedIssue.newBuilder());
  }

  /** Test parsing of the Device FHIR resource. */
  @Test
  public void parseDevice() throws Exception {
    testParse("Device-example", Device.newBuilder());
    testParse("Device-f001", Device.newBuilder());
    testParse("Device-ihe-pcd", Device.newBuilder());
    testParse("Device-example-pacemaker", Device.newBuilder());
    testParse("Device-software", Device.newBuilder());
    testParse("Device-example-udi1", Device.newBuilder());
    testParse("Device-example-udi2", Device.newBuilder());
    testParse("Device-example-udi3", Device.newBuilder());
    testParse("Device-example-udi4", Device.newBuilder());
  }

  /** Test printing of the Device FHIR resource. */
  @Test
  public void printDevice() throws Exception {
    testPrint("Device-example", Device.newBuilder());
    testPrint("Device-f001", Device.newBuilder());
    testPrint("Device-ihe-pcd", Device.newBuilder());
    testPrint("Device-example-pacemaker", Device.newBuilder());
    testPrint("Device-software", Device.newBuilder());
    testPrint("Device-example-udi1", Device.newBuilder());
    testPrint("Device-example-udi2", Device.newBuilder());
    testPrint("Device-example-udi3", Device.newBuilder());
    testPrint("Device-example-udi4", Device.newBuilder());
  }

  /** Test parsing of the DeviceComponent FHIR resource. */
  @Test
  public void parseDeviceComponent() throws Exception {
    testParse("DeviceComponent-example", DeviceComponent.newBuilder());
    testParse("DeviceComponent-example-prodspec", DeviceComponent.newBuilder());
  }

  /** Test printing of the DeviceComponent FHIR resource. */
  @Test
  public void printDeviceComponent() throws Exception {
    testPrint("DeviceComponent-example", DeviceComponent.newBuilder());
    testPrint("DeviceComponent-example-prodspec", DeviceComponent.newBuilder());
  }

  /** Test parsing of the DeviceMetric FHIR resource. */
  @Test
  public void parseDeviceMetric() throws Exception {
    testParse("DeviceMetric-example", DeviceMetric.newBuilder());
  }

  /** Test printing of the DeviceMetric FHIR resource. */
  @Test
  public void printDeviceMetric() throws Exception {
    testPrint("DeviceMetric-example", DeviceMetric.newBuilder());
  }

  /** Test parsing of the DeviceRequest FHIR resource. */
  @Test
  public void parseDeviceRequest() throws Exception {
    testParse("DeviceRequest-example", DeviceRequest.newBuilder());
    testParse("DeviceRequest-insulinpump", DeviceRequest.newBuilder());
  }

  /** Test printing of the DeviceRequest FHIR resource. */
  @Test
  public void printDeviceRequest() throws Exception {
    testPrint("DeviceRequest-example", DeviceRequest.newBuilder());
    testPrint("DeviceRequest-insulinpump", DeviceRequest.newBuilder());
  }

  /** Test parsing of the DeviceUseStatement FHIR resource. */
  @Test
  public void parseDeviceUseStatement() throws Exception {
    testParse("DeviceUseStatement-example", DeviceUseStatement.newBuilder());
  }

  /** Test printing of the DeviceUseStatement FHIR resource. */
  @Test
  public void printDeviceUseStatement() throws Exception {
    testPrint("DeviceUseStatement-example", DeviceUseStatement.newBuilder());
  }

  /** Test parsing of the DiagnosticReport FHIR resource. */
  @Test
  public void parseDiagnosticReport() throws Exception {
    testParse("DiagnosticReport-101", DiagnosticReport.newBuilder());
    testParse("DiagnosticReport-102", DiagnosticReport.newBuilder());
    testParse("DiagnosticReport-f001", DiagnosticReport.newBuilder());
    testParse("DiagnosticReport-f201", DiagnosticReport.newBuilder());
    testParse("DiagnosticReport-f202", DiagnosticReport.newBuilder());
    testParse("DiagnosticReport-ghp", DiagnosticReport.newBuilder());
    testParse("DiagnosticReport-gingival-mass", DiagnosticReport.newBuilder());
    testParse("DiagnosticReport-lipids", DiagnosticReport.newBuilder());
    testParse("DiagnosticReport-pap", DiagnosticReport.newBuilder());
    testParse("DiagnosticReport-example-pgx", DiagnosticReport.newBuilder());
    testParse("DiagnosticReport-ultrasound", DiagnosticReport.newBuilder());
    testParse("DiagnosticReport-dg2", DiagnosticReport.newBuilder());
  }

  /** Test printing of the DiagnosticReport FHIR resource. */
  @Test
  public void printDiagnosticReport() throws Exception {
    testPrint("DiagnosticReport-101", DiagnosticReport.newBuilder());
    testPrint("DiagnosticReport-102", DiagnosticReport.newBuilder());
    testPrint("DiagnosticReport-f001", DiagnosticReport.newBuilder());
    testPrint("DiagnosticReport-f201", DiagnosticReport.newBuilder());
    testPrint("DiagnosticReport-f202", DiagnosticReport.newBuilder());
    testPrint("DiagnosticReport-ghp", DiagnosticReport.newBuilder());
    testPrint("DiagnosticReport-gingival-mass", DiagnosticReport.newBuilder());
    testPrint("DiagnosticReport-lipids", DiagnosticReport.newBuilder());
    testPrint("DiagnosticReport-pap", DiagnosticReport.newBuilder());
    testPrint("DiagnosticReport-example-pgx", DiagnosticReport.newBuilder());
    testPrint("DiagnosticReport-ultrasound", DiagnosticReport.newBuilder());
    testPrint("DiagnosticReport-dg2", DiagnosticReport.newBuilder());
  }

  /** Test parsing of the DocumentManifest FHIR resource. */
  @Test
  public void parseDocumentManifest() throws Exception {
    testParse("DocumentManifest-example", DocumentManifest.newBuilder());
  }

  /** Test printing of the DocumentManifest FHIR resource. */
  @Test
  public void printDocumentManifest() throws Exception {
    testPrint("DocumentManifest-example", DocumentManifest.newBuilder());
  }

  /** Test parsing of the DocumentReference FHIR resource. */
  @Test
  public void parseDocumentReference() throws Exception {
    testParse("DocumentReference-example", DocumentReference.newBuilder());
  }

  /** Test printing of the DocumentReference FHIR resource. */
  @Test
  public void printDocumentReference() throws Exception {
    testPrint("DocumentReference-example", DocumentReference.newBuilder());
  }

  /** Test parsing of the EligibilityRequest FHIR resource. */
  @Test
  public void parseEligibilityRequest() throws Exception {
    testParse("EligibilityRequest-52345", EligibilityRequest.newBuilder());
    testParse("EligibilityRequest-52346", EligibilityRequest.newBuilder());
  }

  /** Test printing of the EligibilityRequest FHIR resource. */
  @Test
  public void printEligibilityRequest() throws Exception {
    testPrint("EligibilityRequest-52345", EligibilityRequest.newBuilder());
    testPrint("EligibilityRequest-52346", EligibilityRequest.newBuilder());
  }

  /** Test parsing of the EligibilityResponse FHIR resource. */
  @Test
  public void parseEligibilityResponse() throws Exception {
    testParse("EligibilityResponse-E2500", EligibilityResponse.newBuilder());
    testParse("EligibilityResponse-E2501", EligibilityResponse.newBuilder());
    testParse("EligibilityResponse-E2502", EligibilityResponse.newBuilder());
    testParse("EligibilityResponse-E2503", EligibilityResponse.newBuilder());
  }

  /** Test printing of the EligibilityResponse FHIR resource. */
  @Test
  public void printEligibilityResponse() throws Exception {
    testPrint("EligibilityResponse-E2500", EligibilityResponse.newBuilder());
    testPrint("EligibilityResponse-E2501", EligibilityResponse.newBuilder());
    testPrint("EligibilityResponse-E2502", EligibilityResponse.newBuilder());
    testPrint("EligibilityResponse-E2503", EligibilityResponse.newBuilder());
  }

  /** Test parsing of the Encounter FHIR resource. */
  @Test
  public void parseEncounter() throws Exception {
    testParse("Encounter-example", Encounter.newBuilder());
    testParse("Encounter-emerg", Encounter.newBuilder());
    testParse("Encounter-f001", Encounter.newBuilder());
    testParse("Encounter-f002", Encounter.newBuilder());
    testParse("Encounter-f003", Encounter.newBuilder());
    testParse("Encounter-f201", Encounter.newBuilder());
    testParse("Encounter-f202", Encounter.newBuilder());
    testParse("Encounter-f203", Encounter.newBuilder());
    testParse("Encounter-home", Encounter.newBuilder());
    testParse("Encounter-xcda", Encounter.newBuilder());
  }

  /** Test printing of the Encounter FHIR resource. */
  @Test
  public void printEncounter() throws Exception {
    testPrint("Encounter-example", Encounter.newBuilder());
    testPrint("Encounter-emerg", Encounter.newBuilder());
    testPrint("Encounter-f001", Encounter.newBuilder());
    testPrint("Encounter-f002", Encounter.newBuilder());
    testPrint("Encounter-f003", Encounter.newBuilder());
    testPrint("Encounter-f201", Encounter.newBuilder());
    testPrint("Encounter-f202", Encounter.newBuilder());
    testPrint("Encounter-f203", Encounter.newBuilder());
    testPrint("Encounter-home", Encounter.newBuilder());
    testPrint("Encounter-xcda", Encounter.newBuilder());
  }

  /** Test parsing of the Endpoint FHIR resource. */
  @Test
  public void parseEndpoint() throws Exception {
    testParse("Endpoint-example", Endpoint.newBuilder());
    testParse("Endpoint-example-iid", Endpoint.newBuilder());
    testParse("Endpoint-example-wadors", Endpoint.newBuilder());
  }

  /** Test printing of the Endpoint FHIR resource. */
  @Test
  public void printEndpoint() throws Exception {
    testPrint("Endpoint-example", Endpoint.newBuilder());
    testPrint("Endpoint-example-iid", Endpoint.newBuilder());
    testPrint("Endpoint-example-wadors", Endpoint.newBuilder());
  }

  /** Test parsing of the EnrollmentRequest FHIR resource. */
  @Test
  public void parseEnrollmentRequest() throws Exception {
    testParse("EnrollmentRequest-22345", EnrollmentRequest.newBuilder());
  }

  /** Test printing of the EnrollmentRequest FHIR resource. */
  @Test
  public void printEnrollmentRequest() throws Exception {
    testPrint("EnrollmentRequest-22345", EnrollmentRequest.newBuilder());
  }

  /** Test parsing of the EnrollmentResponse FHIR resource. */
  @Test
  public void parseEnrollmentResponse() throws Exception {
    testParse("EnrollmentResponse-ER2500", EnrollmentResponse.newBuilder());
  }

  /** Test printing of the EnrollmentResponse FHIR resource. */
  @Test
  public void printEnrollmentResponse() throws Exception {
    testPrint("EnrollmentResponse-ER2500", EnrollmentResponse.newBuilder());
  }

  /** Test parsing of the EpisodeOfCare FHIR resource. */
  @Test
  public void parseEpisodeOfCare() throws Exception {
    testParse("EpisodeOfCare-example", EpisodeOfCare.newBuilder());
  }

  /** Test printing of the EpisodeOfCare FHIR resource. */
  @Test
  public void printEpisodeOfCare() throws Exception {
    testPrint("EpisodeOfCare-example", EpisodeOfCare.newBuilder());
  }

  /** Test parsing of the ExpansionProfile FHIR resource. */
  @Test
  public void parseExpansionProfile() throws Exception {
    testParse("ExpansionProfile-example", ExpansionProfile.newBuilder());
  }

  /** Test printing of the ExpansionProfile FHIR resource. */
  @Test
  public void printExpansionProfile() throws Exception {
    testPrint("ExpansionProfile-example", ExpansionProfile.newBuilder());
  }

  /** Test parsing of the ExplanationOfBenefit FHIR resource. */
  @Test
  public void parseExplanationOfBenefit() throws Exception {
    testParse("ExplanationOfBenefit-EB3500", ExplanationOfBenefit.newBuilder());
  }

  /** Test printing of the ExplanationOfBenefit FHIR resource. */
  @Test
  public void printExplanationOfBenefit() throws Exception {
    testPrint("ExplanationOfBenefit-EB3500", ExplanationOfBenefit.newBuilder());
  }

  /** Test parsing of the FamilyMemberHistory FHIR resource. */
  @Test
  public void parseFamilyMemberHistory() throws Exception {
    testParse("FamilyMemberHistory-father", FamilyMemberHistory.newBuilder());
    testParse("FamilyMemberHistory-mother", FamilyMemberHistory.newBuilder());
  }

  /** Test printing of the FamilyMemberHistory FHIR resource. */
  @Test
  public void printFamilyMemberHistory() throws Exception {
    testPrint("FamilyMemberHistory-father", FamilyMemberHistory.newBuilder());
    testPrint("FamilyMemberHistory-mother", FamilyMemberHistory.newBuilder());
  }

  /** Test parsing of the Flag FHIR resource. */
  @Test
  public void parseFlag() throws Exception {
    testParse("Flag-example", Flag.newBuilder());
    testParse("Flag-example-encounter", Flag.newBuilder());
  }

  /** Test printing of the Flag FHIR resource. */
  @Test
  public void printFlag() throws Exception {
    testPrint("Flag-example", Flag.newBuilder());
    testPrint("Flag-example-encounter", Flag.newBuilder());
  }

  /** Test parsing of the Goal FHIR resource. */
  @Test
  public void parseGoal() throws Exception {
    testParse("Goal-example", Goal.newBuilder());
    testParse("Goal-stop-smoking", Goal.newBuilder());
  }

  /** Test printing of the Goal FHIR resource. */
  @Test
  public void printGoal() throws Exception {
    testPrint("Goal-example", Goal.newBuilder());
    testPrint("Goal-stop-smoking", Goal.newBuilder());
  }

  /** Test parsing of the GraphDefinition FHIR resource. */
  @Test
  public void parseGraphDefinition() throws Exception {
    testParse("GraphDefinition-example", GraphDefinition.newBuilder());
  }

  /** Test printing of the GraphDefinition FHIR resource. */
  @Test
  public void printGraphDefinition() throws Exception {
    testPrint("GraphDefinition-example", GraphDefinition.newBuilder());
  }

  /** Test parsing of the Group FHIR resource. */
  @Test
  public void parseGroup() throws Exception {
    testParse("Group-101", Group.newBuilder());
    testParse("Group-102", Group.newBuilder());
  }

  /** Test printing of the Group FHIR resource. */
  @Test
  public void printGroup() throws Exception {
    testPrint("Group-101", Group.newBuilder());
    testPrint("Group-102", Group.newBuilder());
  }

  /** Test parsing of the GuidanceResponse FHIR resource. */
  @Test
  public void parseGuidanceResponse() throws Exception {
    testParse("GuidanceResponse-example", GuidanceResponse.newBuilder());
  }

  /** Test printing of the GuidanceResponse FHIR resource. */
  @Test
  public void printGuidanceResponse() throws Exception {
    testPrint("GuidanceResponse-example", GuidanceResponse.newBuilder());
  }

  /** Test parsing of the HealthcareService FHIR resource. */
  @Test
  public void parseHealthcareService() throws Exception {
    testParse("HealthcareService-example", HealthcareService.newBuilder());
  }

  /** Test printing of the HealthcareService FHIR resource. */
  @Test
  public void printHealthcareService() throws Exception {
    testPrint("HealthcareService-example", HealthcareService.newBuilder());
  }

  /** Test parsing of the ImagingManifest FHIR resource. */
  @Test
  public void parseImagingManifest() throws Exception {
    testParse("ImagingManifest-example", ImagingManifest.newBuilder());
  }

  /** Test printing of the ImagingManifest FHIR resource. */
  @Test
  public void printImagingManifest() throws Exception {
    testPrint("ImagingManifest-example", ImagingManifest.newBuilder());
  }

  /** Test parsing of the ImagingStudy FHIR resource. */
  @Test
  public void parseImagingStudy() throws Exception {
    testParse("ImagingStudy-example", ImagingStudy.newBuilder());
    testParse("ImagingStudy-example-xr", ImagingStudy.newBuilder());
  }

  /** Test printing of the ImagingStudy FHIR resource. */
  @Test
  public void printImagingStudy() throws Exception {
    testPrint("ImagingStudy-example", ImagingStudy.newBuilder());
    testPrint("ImagingStudy-example-xr", ImagingStudy.newBuilder());
  }

  /** Test parsing of the Immunization FHIR resource. */
  @Test
  public void parseImmunization() throws Exception {
    testParse("Immunization-example", Immunization.newBuilder());
    testParse("Immunization-historical", Immunization.newBuilder());
    testParse("Immunization-notGiven", Immunization.newBuilder());
  }

  /** Test printing of the Immunization FHIR resource. */
  @Test
  public void printImmunization() throws Exception {
    testPrint("Immunization-example", Immunization.newBuilder());
    testPrint("Immunization-historical", Immunization.newBuilder());
    testPrint("Immunization-notGiven", Immunization.newBuilder());
  }

  /** Test parsing of the ImmunizationRecommendation FHIR resource. */
  @Test
  public void parseImmunizationRecommendation() throws Exception {
    testParse("ImmunizationRecommendation-example", ImmunizationRecommendation.newBuilder());
    testParse(
        "immunizationrecommendation-target-disease-example",
        ImmunizationRecommendation.newBuilder());
  }

  /** Test printing of the ImmunizationRecommendation FHIR resource. */
  @Test
  public void printImmunizationRecommendation() throws Exception {
    testPrint("ImmunizationRecommendation-example", ImmunizationRecommendation.newBuilder());
    testPrint("ImmunizationRecommendation-example", ImmunizationRecommendation.newBuilder());
  }

  /** Test parsing of the ImplementationGuide FHIR resource. */
  @Test
  public void parseImplementationGuide() throws Exception {
    testParse("ImplementationGuide-example", ImplementationGuide.newBuilder());
  }

  /** Test printing of the ImplementationGuide FHIR resource. */
  @Test
  public void printImplementationGuide() throws Exception {
    testPrint("ImplementationGuide-example", ImplementationGuide.newBuilder());
  }

  /** Test parsing of the Library FHIR resource. */
  @Test
  public void parseLibrary() throws Exception {
    testParse("Library-library-cms146-example", Library.newBuilder());
    testParse("Library-composition-example", Library.newBuilder());
    testParse("Library-example", Library.newBuilder());
    testParse("Library-library-fhir-helpers-predecessor", Library.newBuilder());
  }

  /** Test printing of the Library FHIR resource. */
  @Test
  public void printLibrary() throws Exception {
    testPrint("Library-library-cms146-example", Library.newBuilder());
    testPrint("Library-composition-example", Library.newBuilder());
    testPrint("Library-example", Library.newBuilder());
    testPrint("Library-library-fhir-helpers-predecessor", Library.newBuilder());
  }

  /** Test parsing of the Linkage FHIR resource. */
  @Test
  public void parseLinkage() throws Exception {
    testParse("Linkage-example", Linkage.newBuilder());
  }

  /** Test printing of the Linkage FHIR resource. */
  @Test
  public void printLinkage() throws Exception {
    testPrint("Linkage-example", Linkage.newBuilder());
  }

  /** Test parsing of the List FHIR resource. */
  @Test
  public void parseList() throws Exception {
    testParse("List-example", List.newBuilder());
    testParse("List-current-allergies", List.newBuilder());
    testParse("List-example-double-cousin-relationship", List.newBuilder());
    testParse("List-example-empty", List.newBuilder());
    testParse("List-f201", List.newBuilder());
    testParse("List-genetic", List.newBuilder());
    testParse("List-prognosis", List.newBuilder());
    testParse("List-med-list", List.newBuilder());
    testParse("List-example-simple-empty", List.newBuilder());
  }

  /** Test printing of the List FHIR resource. */
  @Test
  public void printList() throws Exception {
    testPrint("List-example", List.newBuilder());
    testPrint("List-current-allergies", List.newBuilder());
    testPrint("List-example-double-cousin-relationship", List.newBuilder());
    testPrint("List-example-empty", List.newBuilder());
    testPrint("List-f201", List.newBuilder());
    testPrint("List-genetic", List.newBuilder());
    testPrint("List-prognosis", List.newBuilder());
    testPrint("List-med-list", List.newBuilder());
    testPrint("List-example-simple-empty", List.newBuilder());
  }

  /** Test parsing of the Location FHIR resource. */
  @Test
  public void parseLocation() throws Exception {
    testParse("Location-1", Location.newBuilder());
    testParse("Location-amb", Location.newBuilder());
    testParse("Location-hl7", Location.newBuilder());
    testParse("Location-ph", Location.newBuilder());
    testParse("Location-2", Location.newBuilder());
    testParse("Location-ukp", Location.newBuilder());
  }

  /** Test printing of the Location FHIR resource. */
  @Test
  public void printLocation() throws Exception {
    testPrint("Location-1", Location.newBuilder());
    testPrint("Location-amb", Location.newBuilder());
    testPrint("Location-hl7", Location.newBuilder());
    testPrint("Location-ph", Location.newBuilder());
    testPrint("Location-2", Location.newBuilder());
    testPrint("Location-ukp", Location.newBuilder());
  }

  /** Test parsing of the Measure FHIR resource. */
  @Test
  public void parseMeasure() throws Exception {
    testParse("Measure-measure-cms146-example", Measure.newBuilder());
    testParse("Measure-component-a-example", Measure.newBuilder());
    testParse("Measure-component-b-example", Measure.newBuilder());
    testParse("Measure-composite-example", Measure.newBuilder());
    testParse("Measure-measure-predecessor-example", Measure.newBuilder());
  }

  /** Test printing of the Measure FHIR resource. */
  @Test
  public void printMeasure() throws Exception {
    testPrint("Measure-measure-cms146-example", Measure.newBuilder());
    testPrint("Measure-component-a-example", Measure.newBuilder());
    testPrint("Measure-component-b-example", Measure.newBuilder());
    testPrint("Measure-composite-example", Measure.newBuilder());
    testPrint("Measure-measure-predecessor-example", Measure.newBuilder());
  }

  /** Test parsing of the MeasureReport FHIR resource. */
  @Test
  public void parseMeasureReport() throws Exception {
    testParse("MeasureReport-measurereport-cms146-cat1-example", MeasureReport.newBuilder());
    testParse("MeasureReport-measurereport-cms146-cat2-example", MeasureReport.newBuilder());
    testParse("MeasureReport-measurereport-cms146-cat3-example", MeasureReport.newBuilder());
  }

  /** Test printing of the MeasureReport FHIR resource. */
  @Test
  public void printMeasureReport() throws Exception {
    testPrint("MeasureReport-measurereport-cms146-cat1-example", MeasureReport.newBuilder());
    testPrint("MeasureReport-measurereport-cms146-cat2-example", MeasureReport.newBuilder());
    testPrint("MeasureReport-measurereport-cms146-cat3-example", MeasureReport.newBuilder());
  }

  /** Test parsing of the Media FHIR resource. */
  @Test
  public void parseMedia() throws Exception {
    testParse("Media-example", Media.newBuilder());
    testParse("Media-1.2.840.11361907579238403408700.3.0.14.19970327150033", Media.newBuilder());
    testParse("Media-sound", Media.newBuilder());
    testParse("Media-xray", Media.newBuilder());
  }

  /** Test printing of the Media FHIR resource. */
  @Test
  public void printMedia() throws Exception {
    testPrint("Media-example", Media.newBuilder());
    testPrint("Media-1.2.840.11361907579238403408700.3.0.14.19970327150033", Media.newBuilder());
    testPrint("Media-sound", Media.newBuilder());
    testPrint("Media-xray", Media.newBuilder());
  }

  /** Test parsing of the Medication FHIR resource. */
  @Test
  public void parseMedication() throws Exception {
    testParse("Medication-med0301", Medication.newBuilder());
    testParse("Medication-med0302", Medication.newBuilder());
    testParse("Medication-med0303", Medication.newBuilder());
    testParse("Medication-med0304", Medication.newBuilder());
    testParse("Medication-med0305", Medication.newBuilder());
    testParse("Medication-med0306", Medication.newBuilder());
    testParse("Medication-med0307", Medication.newBuilder());
    testParse("Medication-med0308", Medication.newBuilder());
    testParse("Medication-med0309", Medication.newBuilder());
    testParse("Medication-med0310", Medication.newBuilder());
    testParse("Medication-med0311", Medication.newBuilder());
    testParse("Medication-med0312", Medication.newBuilder());
    testParse("Medication-med0313", Medication.newBuilder());
    testParse("Medication-med0314", Medication.newBuilder());
    testParse("Medication-med0315", Medication.newBuilder());
    testParse("Medication-med0316", Medication.newBuilder());
    testParse("Medication-med0317", Medication.newBuilder());
    testParse("Medication-med0318", Medication.newBuilder());
    testParse("Medication-med0319", Medication.newBuilder());
    testParse("Medication-med0320", Medication.newBuilder());
    testParse("Medication-med0321", Medication.newBuilder());
    testParse("Medication-medicationexample1", Medication.newBuilder());
    testParse("Medication-medexample015", Medication.newBuilder());
  }

  /** Test printing of the Medication FHIR resource. */
  @Test
  public void printMedication() throws Exception {
    testPrint("Medication-med0301", Medication.newBuilder());
    testPrint("Medication-med0302", Medication.newBuilder());
    testPrint("Medication-med0303", Medication.newBuilder());
    testPrint("Medication-med0304", Medication.newBuilder());
    testPrint("Medication-med0305", Medication.newBuilder());
    testPrint("Medication-med0306", Medication.newBuilder());
    testPrint("Medication-med0307", Medication.newBuilder());
    testPrint("Medication-med0308", Medication.newBuilder());
    testPrint("Medication-med0309", Medication.newBuilder());
    testPrint("Medication-med0310", Medication.newBuilder());
    testPrint("Medication-med0311", Medication.newBuilder());
    testPrint("Medication-med0312", Medication.newBuilder());
    testPrint("Medication-med0313", Medication.newBuilder());
    testPrint("Medication-med0314", Medication.newBuilder());
    testPrint("Medication-med0315", Medication.newBuilder());
    testPrint("Medication-med0316", Medication.newBuilder());
    testPrint("Medication-med0317", Medication.newBuilder());
    testPrint("Medication-med0318", Medication.newBuilder());
    testPrint("Medication-med0319", Medication.newBuilder());
    testPrint("Medication-med0320", Medication.newBuilder());
    testPrint("Medication-med0321", Medication.newBuilder());
    testPrint("Medication-medicationexample1", Medication.newBuilder());
    testPrint("Medication-medexample015", Medication.newBuilder());
  }

  /** Test parsing of the MedicationAdministration FHIR resource. */
  @Test
  public void parseMedicationAdministration() throws Exception {
    testParse("MedicationAdministration-medadminexample03", MedicationAdministration.newBuilder());
  }

  /** Test printing of the MedicationAdministration FHIR resource. */
  @Test
  public void printMedicationAdministration() throws Exception {
    testPrint("MedicationAdministration-medadminexample03", MedicationAdministration.newBuilder());
  }

  /** Test parsing of the MedicationDispense FHIR resource. */
  @Test
  public void parseMedicationDispense() throws Exception {
    testParse("MedicationDispense-meddisp008", MedicationDispense.newBuilder());
  }

  /** Test printing of the MedicationDispense FHIR resource. */
  @Test
  public void printMedicationDispense() throws Exception {
    testPrint("MedicationDispense-meddisp008", MedicationDispense.newBuilder());
  }

  /** Test parsing of the MedicationRequest FHIR resource. */
  @Test
  public void parseMedicationRequest() throws Exception {
    testParse("MedicationRequest-medrx0311", MedicationRequest.newBuilder());
    testParse("MedicationRequest-medrx002", MedicationRequest.newBuilder());
  }

  /** Test printing of the MedicationRequest FHIR resource. */
  @Test
  public void printMedicationRequest() throws Exception {
    testPrint("MedicationRequest-medrx0311", MedicationRequest.newBuilder());
    testPrint("MedicationRequest-medrx002", MedicationRequest.newBuilder());
  }

  /** Test parsing of the MedicationStatement FHIR resource. */
  @Test
  public void parseMedicationStatement() throws Exception {
    testParse("MedicationStatement-example001", MedicationStatement.newBuilder());
    testParse("MedicationStatement-example002", MedicationStatement.newBuilder());
    testParse("MedicationStatement-example003", MedicationStatement.newBuilder());
    testParse("MedicationStatement-example004", MedicationStatement.newBuilder());
    testParse("MedicationStatement-example005", MedicationStatement.newBuilder());
    testParse("MedicationStatement-example006", MedicationStatement.newBuilder());
    testParse("MedicationStatement-example007", MedicationStatement.newBuilder());
  }

  /** Test printing of the MedicationStatement FHIR resource. */
  @Test
  public void printMedicationStatement() throws Exception {
    testPrint("MedicationStatement-example001", MedicationStatement.newBuilder());
    testPrint("MedicationStatement-example002", MedicationStatement.newBuilder());
    testPrint("MedicationStatement-example003", MedicationStatement.newBuilder());
    testPrint("MedicationStatement-example004", MedicationStatement.newBuilder());
    testPrint("MedicationStatement-example005", MedicationStatement.newBuilder());
    testPrint("MedicationStatement-example006", MedicationStatement.newBuilder());
    testPrint("MedicationStatement-example007", MedicationStatement.newBuilder());
  }

  /** Test parsing of the MessageDefinition FHIR resource. */
  @Test
  public void parseMessageDefinition() throws Exception {
    testParse("MessageDefinition-example", MessageDefinition.newBuilder());
  }

  /** Test printing of the MessageDefinition FHIR resource. */
  @Test
  public void printMessageDefinition() throws Exception {
    testPrint("MessageDefinition-example", MessageDefinition.newBuilder());
  }

  /** Test parsing of the MessageHeader FHIR resource. */
  @Test
  public void parseMessageHeader() throws Exception {
    testParse("MessageHeader-1cbdfb97-5859-48a4-8301-d54eab818d68", MessageHeader.newBuilder());
  }

  /** Test printing of the MessageHeader FHIR resource. */
  @Test
  public void printMessageHeader() throws Exception {
    testPrint("MessageHeader-1cbdfb97-5859-48a4-8301-d54eab818d68", MessageHeader.newBuilder());
  }

  /** Test parsing of the NamingSystem FHIR resource. */
  @Test
  public void parseNamingSystem() throws Exception {
    testParse("NamingSystem-example", NamingSystem.newBuilder());
    testParse("NamingSystem-example-id", NamingSystem.newBuilder());
    testParse("NamingSystem-example-replaced", NamingSystem.newBuilder());
  }

  /** Test printing of the NamingSystem FHIR resource. */
  @Test
  public void printNamingSystem() throws Exception {
    testPrint("NamingSystem-example", NamingSystem.newBuilder());
    testPrint("NamingSystem-example-id", NamingSystem.newBuilder());
    testPrint("NamingSystem-example-replaced", NamingSystem.newBuilder());
  }

  /** Test parsing of the NutritionOrder FHIR resource. */
  @Test
  public void parseNutritionOrder() throws Exception {
    testParse("NutritionOrder-cardiacdiet", NutritionOrder.newBuilder());
    testParse("NutritionOrder-diabeticdiet", NutritionOrder.newBuilder());
    testParse("NutritionOrder-diabeticsupplement", NutritionOrder.newBuilder());
    testParse("NutritionOrder-energysupplement", NutritionOrder.newBuilder());
    testParse("NutritionOrder-enteralbolus", NutritionOrder.newBuilder());
    testParse("NutritionOrder-enteralcontinuous", NutritionOrder.newBuilder());
    testParse("NutritionOrder-fiberrestricteddiet", NutritionOrder.newBuilder());
    testParse("NutritionOrder-infantenteral", NutritionOrder.newBuilder());
    testParse("NutritionOrder-proteinsupplement", NutritionOrder.newBuilder());
    testParse("NutritionOrder-pureeddiet", NutritionOrder.newBuilder());
    testParse("NutritionOrder-pureeddiet-simple", NutritionOrder.newBuilder());
    testParse("NutritionOrder-renaldiet", NutritionOrder.newBuilder());
    testParse("NutritionOrder-texturemodified", NutritionOrder.newBuilder());
  }

  /** Test printing of the NutritionOrder FHIR resource. */
  @Test
  public void printNutritionOrder() throws Exception {
    testPrint("NutritionOrder-cardiacdiet", NutritionOrder.newBuilder());
    testPrint("NutritionOrder-diabeticdiet", NutritionOrder.newBuilder());
    testPrint("NutritionOrder-diabeticsupplement", NutritionOrder.newBuilder());
    testPrint("NutritionOrder-energysupplement", NutritionOrder.newBuilder());
    testPrint("NutritionOrder-enteralbolus", NutritionOrder.newBuilder());
    testPrint("NutritionOrder-enteralcontinuous", NutritionOrder.newBuilder());
    testPrint("NutritionOrder-fiberrestricteddiet", NutritionOrder.newBuilder());
    testPrint("NutritionOrder-infantenteral", NutritionOrder.newBuilder());
    testPrint("NutritionOrder-proteinsupplement", NutritionOrder.newBuilder());
    testPrint("NutritionOrder-pureeddiet", NutritionOrder.newBuilder());
    testPrint("NutritionOrder-pureeddiet-simple", NutritionOrder.newBuilder());
    testPrint("NutritionOrder-renaldiet", NutritionOrder.newBuilder());
    testPrint("NutritionOrder-texturemodified", NutritionOrder.newBuilder());
  }

  /** Test parsing of the Observation FHIR resource. */
  @Test
  public void parseObservation() throws Exception {
    testParse("Observation-example", Observation.newBuilder());
    testParse("Observation-10minute-apgar-score", Observation.newBuilder());
    testParse("Observation-1minute-apgar-score", Observation.newBuilder());
    testParse("Observation-20minute-apgar-score", Observation.newBuilder());
    testParse("Observation-2minute-apgar-score", Observation.newBuilder());
    testParse("Observation-5minute-apgar-score", Observation.newBuilder());
    testParse("Observation-blood-pressure", Observation.newBuilder());
    testParse("Observation-blood-pressure-cancel", Observation.newBuilder());
    testParse("Observation-blood-pressure-dar", Observation.newBuilder());
    testParse("Observation-bmd", Observation.newBuilder());
    testParse("Observation-bmi", Observation.newBuilder());
    testParse("Observation-body-height", Observation.newBuilder());
    testParse("Observation-body-length", Observation.newBuilder());
    testParse("Observation-body-temperature", Observation.newBuilder());
    testParse("Observation-date-lastmp", Observation.newBuilder());
    testParse("Observation-example-diplotype1", Observation.newBuilder());
    testParse("Observation-eye-color", Observation.newBuilder());
    testParse("Observation-f001", Observation.newBuilder());
    testParse("Observation-f002", Observation.newBuilder());
    testParse("Observation-f003", Observation.newBuilder());
    testParse("Observation-f004", Observation.newBuilder());
    testParse("Observation-f005", Observation.newBuilder());
    testParse("Observation-f202", Observation.newBuilder());
    testParse("Observation-f203", Observation.newBuilder());
    testParse("Observation-f204", Observation.newBuilder());
    testParse("Observation-f205", Observation.newBuilder());
    testParse("Observation-f206", Observation.newBuilder());
    testParse("Observation-example-genetics-1", Observation.newBuilder());
    testParse("Observation-example-genetics-2", Observation.newBuilder());
    testParse("Observation-example-genetics-3", Observation.newBuilder());
    testParse("Observation-example-genetics-4", Observation.newBuilder());
    testParse("Observation-example-genetics-5", Observation.newBuilder());
    testParse("Observation-glasgow", Observation.newBuilder());
    testParse("Observation-gcs-qa", Observation.newBuilder());
    testParse("Observation-example-haplotype1", Observation.newBuilder());
    testParse("Observation-example-haplotype2", Observation.newBuilder());
    testParse("Observation-head-circumference", Observation.newBuilder());
    testParse("Observation-heart-rate", Observation.newBuilder());
    testParse("Observation-mbp", Observation.newBuilder());
    testParse("Observation-example-phenotype", Observation.newBuilder());
    testParse("Observation-respiratory-rate", Observation.newBuilder());
    testParse("Observation-ekg", Observation.newBuilder());
    testParse("Observation-satO2", Observation.newBuilder());
    testParse("Observation-example-TPMT-diplotype", Observation.newBuilder());
    testParse("Observation-example-TPMT-haplotype-one", Observation.newBuilder());
    testParse("Observation-example-TPMT-haplotype-two", Observation.newBuilder());
    testParse("Observation-unsat", Observation.newBuilder());
    testParse("Observation-vitals-panel", Observation.newBuilder());
  }

  /** Test printing of the Observation FHIR resource. */
  @Test
  public void printObservation() throws Exception {
    testPrint("Observation-example", Observation.newBuilder());
    testPrint("Observation-10minute-apgar-score", Observation.newBuilder());
    testPrint("Observation-1minute-apgar-score", Observation.newBuilder());
    testPrint("Observation-20minute-apgar-score", Observation.newBuilder());
    testPrint("Observation-2minute-apgar-score", Observation.newBuilder());
    testPrint("Observation-5minute-apgar-score", Observation.newBuilder());
    testPrint("Observation-blood-pressure", Observation.newBuilder());
    testPrint("Observation-blood-pressure-cancel", Observation.newBuilder());
    testPrint("Observation-blood-pressure-dar", Observation.newBuilder());
    testPrint("Observation-bmd", Observation.newBuilder());
    testPrint("Observation-bmi", Observation.newBuilder());
    testPrint("Observation-body-height", Observation.newBuilder());
    testPrint("Observation-body-length", Observation.newBuilder());
    testPrint("Observation-body-temperature", Observation.newBuilder());
    testPrint("Observation-date-lastmp", Observation.newBuilder());
    testPrint("Observation-example-diplotype1", Observation.newBuilder());
    testPrint("Observation-eye-color", Observation.newBuilder());
    testPrint("Observation-f001", Observation.newBuilder());
    testPrint("Observation-f002", Observation.newBuilder());
    testPrint("Observation-f003", Observation.newBuilder());
    testPrint("Observation-f004", Observation.newBuilder());
    testPrint("Observation-f005", Observation.newBuilder());
    testPrint("Observation-f202", Observation.newBuilder());
    testPrint("Observation-f203", Observation.newBuilder());
    testPrint("Observation-f204", Observation.newBuilder());
    testPrint("Observation-f205", Observation.newBuilder());
    testPrint("Observation-f206", Observation.newBuilder());
    testPrint("Observation-example-genetics-1", Observation.newBuilder());
    testPrint("Observation-example-genetics-2", Observation.newBuilder());
    testPrint("Observation-example-genetics-3", Observation.newBuilder());
    testPrint("Observation-example-genetics-4", Observation.newBuilder());
    testPrint("Observation-example-genetics-5", Observation.newBuilder());
    testPrint("Observation-glasgow", Observation.newBuilder());
    testPrint("Observation-gcs-qa", Observation.newBuilder());
    testPrint("Observation-example-haplotype1", Observation.newBuilder());
    testPrint("Observation-example-haplotype2", Observation.newBuilder());
    testPrint("Observation-head-circumference", Observation.newBuilder());
    testPrint("Observation-heart-rate", Observation.newBuilder());
    testPrint("Observation-mbp", Observation.newBuilder());
    testPrint("Observation-example-phenotype", Observation.newBuilder());
    testPrint("Observation-respiratory-rate", Observation.newBuilder());
    testPrint("Observation-ekg", Observation.newBuilder());
    testPrint("Observation-satO2", Observation.newBuilder());
    testPrint("Observation-example-TPMT-diplotype", Observation.newBuilder());
    testPrint("Observation-example-TPMT-haplotype-one", Observation.newBuilder());
    testPrint("Observation-example-TPMT-haplotype-two", Observation.newBuilder());
    testPrint("Observation-unsat", Observation.newBuilder());
    testPrint("Observation-vitals-panel", Observation.newBuilder());
  }

  /** Test parsing of the OperationDefinition FHIR resource. */
  @Test
  public void parseOperationDefinition() throws Exception {
    testParse("OperationDefinition-example", OperationDefinition.newBuilder());
  }

  /** Test printing of the OperationDefinition FHIR resource. */
  @Test
  public void printOperationDefinition() throws Exception {
    testPrint("OperationDefinition-example", OperationDefinition.newBuilder());
  }

  /** Test parsing of the OperationOutcome FHIR resource. */
  @Test
  public void parseOperationOutcome() throws Exception {
    testParse("OperationOutcome-101", OperationOutcome.newBuilder());
    testParse("OperationOutcome-allok", OperationOutcome.newBuilder());
    testParse("OperationOutcome-break-the-glass", OperationOutcome.newBuilder());
    testParse("OperationOutcome-exception", OperationOutcome.newBuilder());
    testParse("OperationOutcome-searchfail", OperationOutcome.newBuilder());
    testParse("OperationOutcome-validationfail", OperationOutcome.newBuilder());
  }

  /** Test printing of the OperationOutcome FHIR resource. */
  @Test
  public void printOperationOutcome() throws Exception {
    testPrint("OperationOutcome-101", OperationOutcome.newBuilder());
    testPrint("OperationOutcome-allok", OperationOutcome.newBuilder());
    testPrint("OperationOutcome-break-the-glass", OperationOutcome.newBuilder());
    testPrint("OperationOutcome-exception", OperationOutcome.newBuilder());
    testPrint("OperationOutcome-searchfail", OperationOutcome.newBuilder());
    testPrint("OperationOutcome-validationfail", OperationOutcome.newBuilder());
  }

  /** Test parsing of the Organization FHIR resource. */
  @Test
  public void parseOrganization() throws Exception {
    testParse("Organization-hl7", Organization.newBuilder());
    testParse("Organization-f001", Organization.newBuilder());
    testParse("Organization-f002", Organization.newBuilder());
    testParse("Organization-f003", Organization.newBuilder());
    testParse("Organization-f201", Organization.newBuilder());
    testParse("Organization-f203", Organization.newBuilder());
    testParse("Organization-1", Organization.newBuilder());
    testParse("Organization-2.16.840.1.113883.19.5", Organization.newBuilder());
    testParse("Organization-2", Organization.newBuilder());
    testParse("Organization-1832473e-2fe0-452d-abe9-3cdb9879522f", Organization.newBuilder());
    testParse("Organization-mmanu", Organization.newBuilder());
  }

  /** Test printing of the Organization FHIR resource. */
  @Test
  public void printOrganization() throws Exception {
    testPrint("Organization-hl7", Organization.newBuilder());
    testPrint("Organization-f001", Organization.newBuilder());
    testPrint("Organization-f002", Organization.newBuilder());
    testPrint("Organization-f003", Organization.newBuilder());
    testPrint("Organization-f201", Organization.newBuilder());
    testPrint("Organization-f203", Organization.newBuilder());
    testPrint("Organization-1", Organization.newBuilder());
    testPrint("Organization-2.16.840.1.113883.19.5", Organization.newBuilder());
    testPrint("Organization-2", Organization.newBuilder());
    testPrint("Organization-1832473e-2fe0-452d-abe9-3cdb9879522f", Organization.newBuilder());
    testPrint("Organization-mmanu", Organization.newBuilder());
  }

  /** Test parsing of the Parameters FHIR resource. */
  @Test
  public void parseParameters() throws Exception {
    testParse("Parameters-example", Parameters.newBuilder());
  }

  /** Test printing of the Parameters FHIR resource. */
  @Test
  public void printParameters() throws Exception {
    testPrint("Parameters-example", Parameters.newBuilder());
  }

  /** Test parsing of the Patient FHIR resource. */
  @Test
  public void parsePatient() throws Exception {
    testParse("patient-example", Patient.newBuilder());
    testParse("patient-example-a", Patient.newBuilder());
    testParse("patient-example-animal", Patient.newBuilder());
    testParse("patient-example-b", Patient.newBuilder());
    testParse("patient-example-c", Patient.newBuilder());
    testParse("patient-example-chinese", Patient.newBuilder());
    testParse("patient-example-d", Patient.newBuilder());
    testParse("patient-example-dicom", Patient.newBuilder());
    testParse("patient-example-f001-pieter", Patient.newBuilder());
    testParse("patient-example-f201-roel", Patient.newBuilder());
    testParse("patient-example-ihe-pcd", Patient.newBuilder());
    testParse("patient-example-proband", Patient.newBuilder());
    testParse("patient-example-xcda", Patient.newBuilder());
    testParse("patient-example-xds", Patient.newBuilder());
    testParse("patient-genetics-example1", Patient.newBuilder());
    testParse("patient-glossy-example", Patient.newBuilder());
  }

  /** Test printing of the Patient FHIR resource. */
  @Test
  public void printPatient() throws Exception {
    testPrint("patient-example", Patient.newBuilder());
    testPrint("patient-example-a", Patient.newBuilder());
    testPrint("patient-example-animal", Patient.newBuilder());
    testPrint("patient-example-b", Patient.newBuilder());
    testPrint("patient-example-c", Patient.newBuilder());
    testPrint("patient-example-chinese", Patient.newBuilder());
    testPrint("patient-example-d", Patient.newBuilder());
    testPrint("patient-example-dicom", Patient.newBuilder());
    testPrint("patient-example-f001-pieter", Patient.newBuilder());
    testPrint("patient-example-f201-roel", Patient.newBuilder());
    testPrint("patient-example-ihe-pcd", Patient.newBuilder());
    testPrint("patient-example-proband", Patient.newBuilder());
    testPrint("patient-example-xcda", Patient.newBuilder());
    testPrint("patient-example-xds", Patient.newBuilder());
    testPrint("patient-genetics-example1", Patient.newBuilder());
    testPrint("patient-glossy-example", Patient.newBuilder());
  }

  /** Test parsing of the PaymentNotice FHIR resource. */
  @Test
  public void parsePaymentNotice() throws Exception {
    testParse("PaymentNotice-77654", PaymentNotice.newBuilder());
  }

  /** Test printing of the PaymentNotice FHIR resource. */
  @Test
  public void printPaymentNotice() throws Exception {
    testPrint("PaymentNotice-77654", PaymentNotice.newBuilder());
  }

  /** Test parsing of the PaymentReconciliation FHIR resource. */
  @Test
  public void parsePaymentReconciliation() throws Exception {
    testParse("PaymentReconciliation-ER2500", PaymentReconciliation.newBuilder());
  }

  /** Test printing of the PaymentReconciliation FHIR resource. */
  @Test
  public void printPaymentReconciliation() throws Exception {
    testPrint("PaymentReconciliation-ER2500", PaymentReconciliation.newBuilder());
  }

  /** Test parsing of the Person FHIR resource. */
  @Test
  public void parsePerson() throws Exception {
    testParse("Person-example", Person.newBuilder());
    testParse("Person-f002", Person.newBuilder());
  }

  /** Test printing of the Person FHIR resource. */
  @Test
  public void printPerson() throws Exception {
    testPrint("Person-example", Person.newBuilder());
    testPrint("Person-f002", Person.newBuilder());
  }

  /** Test parsing of the PlanDefinition FHIR resource. */
  @Test
  public void parsePlanDefinition() throws Exception {
    testParse("PlanDefinition-low-suicide-risk-order-set", PlanDefinition.newBuilder());
    testParse("PlanDefinition-KDN5", PlanDefinition.newBuilder());
    testParse("PlanDefinition-options-example", PlanDefinition.newBuilder());
    testParse("PlanDefinition-zika-virus-intervention-initial", PlanDefinition.newBuilder());
    testParse("PlanDefinition-protocol-example", PlanDefinition.newBuilder());
  }

  /** Test printing of the PlanDefinition FHIR resource. */
  @Test
  public void printPlanDefinition() throws Exception {
    testPrint("PlanDefinition-low-suicide-risk-order-set", PlanDefinition.newBuilder());
    testPrint("PlanDefinition-KDN5", PlanDefinition.newBuilder());
    testPrint("PlanDefinition-options-example", PlanDefinition.newBuilder());
    testPrint("PlanDefinition-zika-virus-intervention-initial", PlanDefinition.newBuilder());
    testPrint("PlanDefinition-protocol-example", PlanDefinition.newBuilder());
  }

  /** Test parsing of the Practitioner FHIR resource. */
  @Test
  public void parsePractitioner() throws Exception {
    testParse("Practitioner-example", Practitioner.newBuilder());
    testParse("Practitioner-f001", Practitioner.newBuilder());
    testParse("Practitioner-f002", Practitioner.newBuilder());
    testParse("Practitioner-f003", Practitioner.newBuilder());
    testParse("Practitioner-f004", Practitioner.newBuilder());
    testParse("Practitioner-f005", Practitioner.newBuilder());
    testParse("Practitioner-f006", Practitioner.newBuilder());
    testParse("Practitioner-f007", Practitioner.newBuilder());
    testParse("Practitioner-f201", Practitioner.newBuilder());
    testParse("Practitioner-f202", Practitioner.newBuilder());
    testParse("Practitioner-f203", Practitioner.newBuilder());
    testParse("Practitioner-f204", Practitioner.newBuilder());
    testParse("Practitioner-xcda1", Practitioner.newBuilder());
    testParse("Practitioner-xcda-author", Practitioner.newBuilder());
  }

  /** Test printing of the Practitioner FHIR resource. */
  @Test
  public void printPractitioner() throws Exception {
    testPrint("Practitioner-example", Practitioner.newBuilder());
    testPrint("Practitioner-f001", Practitioner.newBuilder());
    testPrint("Practitioner-f002", Practitioner.newBuilder());
    testPrint("Practitioner-f003", Practitioner.newBuilder());
    testPrint("Practitioner-f004", Practitioner.newBuilder());
    testPrint("Practitioner-f005", Practitioner.newBuilder());
    testPrint("Practitioner-f006", Practitioner.newBuilder());
    testPrint("Practitioner-f007", Practitioner.newBuilder());
    testPrint("Practitioner-f201", Practitioner.newBuilder());
    testPrint("Practitioner-f202", Practitioner.newBuilder());
    testPrint("Practitioner-f203", Practitioner.newBuilder());
    testPrint("Practitioner-f204", Practitioner.newBuilder());
    testPrint("Practitioner-xcda1", Practitioner.newBuilder());
    testPrint("Practitioner-xcda-author", Practitioner.newBuilder());
  }

  /** Test parsing of the PractitionerRole FHIR resource. */
  @Test
  public void parsePractitionerRole() throws Exception {
    testParse("PractitionerRole-example", PractitionerRole.newBuilder());
  }

  /** Test printing of the PractitionerRole FHIR resource. */
  @Test
  public void printPractitionerRole() throws Exception {
    testPrint("PractitionerRole-example", PractitionerRole.newBuilder());
  }

  /** Test parsing of the Procedure FHIR resource. */
  @Test
  public void parseProcedure() throws Exception {
    testParse("Procedure-example", Procedure.newBuilder());
    testParse("Procedure-ambulation", Procedure.newBuilder());
    testParse("Procedure-appendectomy-narrative", Procedure.newBuilder());
    testParse("Procedure-biopsy", Procedure.newBuilder());
    testParse("Procedure-colon-biopsy", Procedure.newBuilder());
    testParse("Procedure-colonoscopy", Procedure.newBuilder());
    testParse("Procedure-education", Procedure.newBuilder());
    testParse("Procedure-f001", Procedure.newBuilder());
    testParse("Procedure-f002", Procedure.newBuilder());
    testParse("Procedure-f003", Procedure.newBuilder());
    testParse("Procedure-f004", Procedure.newBuilder());
    testParse("Procedure-f201", Procedure.newBuilder());
    testParse("Procedure-example-implant", Procedure.newBuilder());
    testParse("Procedure-ob", Procedure.newBuilder());
    testParse("Procedure-physical-therapy", Procedure.newBuilder());
  }

  /** Test printing of the Procedure FHIR resource. */
  @Test
  public void printProcedure() throws Exception {
    testPrint("Procedure-example", Procedure.newBuilder());
    testPrint("Procedure-ambulation", Procedure.newBuilder());
    testPrint("Procedure-appendectomy-narrative", Procedure.newBuilder());
    testPrint("Procedure-biopsy", Procedure.newBuilder());
    testPrint("Procedure-colon-biopsy", Procedure.newBuilder());
    testPrint("Procedure-colonoscopy", Procedure.newBuilder());
    testPrint("Procedure-education", Procedure.newBuilder());
    testPrint("Procedure-f001", Procedure.newBuilder());
    testPrint("Procedure-f002", Procedure.newBuilder());
    testPrint("Procedure-f003", Procedure.newBuilder());
    testPrint("Procedure-f004", Procedure.newBuilder());
    testPrint("Procedure-f201", Procedure.newBuilder());
    testPrint("Procedure-example-implant", Procedure.newBuilder());
    testPrint("Procedure-ob", Procedure.newBuilder());
    testPrint("Procedure-physical-therapy", Procedure.newBuilder());
  }

  /** Test parsing of the ProcedureRequest FHIR resource. */
  @Test
  public void parseProcedureRequest() throws Exception {
    testParse("ProcedureRequest-example", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-physiotherapy", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-do-not-turn", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-benchpress", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-ambulation", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-appendectomy-narrative", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-colonoscopy", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-colon-biopsy", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-di", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-education", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-ft4", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-example-implant", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-lipid", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-ob", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-example-pgx", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-physical-therapy", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-subrequest", ProcedureRequest.newBuilder());
    testParse("ProcedureRequest-og-example1", ProcedureRequest.newBuilder());
  }

  /** Test printing of the ProcedureRequest FHIR resource. */
  @Test
  public void printProcedureRequest() throws Exception {
    testPrint("ProcedureRequest-example", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-physiotherapy", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-do-not-turn", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-benchpress", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-ambulation", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-appendectomy-narrative", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-colonoscopy", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-colon-biopsy", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-di", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-education", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-ft4", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-example-implant", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-lipid", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-ob", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-example-pgx", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-physical-therapy", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-subrequest", ProcedureRequest.newBuilder());
    testPrint("ProcedureRequest-og-example1", ProcedureRequest.newBuilder());
  }

  /** Test parsing of the ProcessRequest FHIR resource. */
  @Test
  public void parseProcessRequest() throws Exception {
    testParse("ProcessRequest-1110", ProcessRequest.newBuilder());
    testParse("ProcessRequest-1115", ProcessRequest.newBuilder());
    testParse("ProcessRequest-1113", ProcessRequest.newBuilder());
    testParse("ProcessRequest-1112", ProcessRequest.newBuilder());
    testParse("ProcessRequest-1114", ProcessRequest.newBuilder());
    testParse("ProcessRequest-1111", ProcessRequest.newBuilder());
    testParse("ProcessRequest-44654", ProcessRequest.newBuilder());
    testParse("ProcessRequest-87654", ProcessRequest.newBuilder());
    testParse("ProcessRequest-87655", ProcessRequest.newBuilder());
  }

  /** Test printing of the ProcessRequest FHIR resource. */
  @Test
  public void printProcessRequest() throws Exception {
    testPrint("ProcessRequest-1110", ProcessRequest.newBuilder());
    testPrint("ProcessRequest-1115", ProcessRequest.newBuilder());
    testPrint("ProcessRequest-1113", ProcessRequest.newBuilder());
    testPrint("ProcessRequest-1112", ProcessRequest.newBuilder());
    testPrint("ProcessRequest-1114", ProcessRequest.newBuilder());
    testPrint("ProcessRequest-1111", ProcessRequest.newBuilder());
    testPrint("ProcessRequest-44654", ProcessRequest.newBuilder());
    testPrint("ProcessRequest-87654", ProcessRequest.newBuilder());
    testPrint("ProcessRequest-87655", ProcessRequest.newBuilder());
  }

  /** Test parsing of the ProcessResponse FHIR resource. */
  @Test
  public void parseProcessResponse() throws Exception {
    testParse("ProcessResponse-SR2500", ProcessResponse.newBuilder());
    testParse("ProcessResponse-SR2349", ProcessResponse.newBuilder());
    testParse("ProcessResponse-SR2499", ProcessResponse.newBuilder());
  }

  /** Test printing of the ProcessResponse FHIR resource. */
  @Test
  public void printProcessResponse() throws Exception {
    testPrint("ProcessResponse-SR2500", ProcessResponse.newBuilder());
    testPrint("ProcessResponse-SR2349", ProcessResponse.newBuilder());
    testPrint("ProcessResponse-SR2499", ProcessResponse.newBuilder());
  }

  /** Test parsing of the Provenance FHIR resource. */
  @Test
  public void parseProvenance() throws Exception {
    testParse("Provenance-example", Provenance.newBuilder());
    testParse("Provenance-example-biocompute-object", Provenance.newBuilder());
    testParse("Provenance-example-cwl", Provenance.newBuilder());
    testParse("Provenance-signature", Provenance.newBuilder());
  }

  /** Test printing of the Provenance FHIR resource. */
  @Test
  public void printProvenance() throws Exception {
    testPrint("Provenance-example", Provenance.newBuilder());
    testPrint("Provenance-example-biocompute-object", Provenance.newBuilder());
    testPrint("Provenance-example-cwl", Provenance.newBuilder());
    testPrint("Provenance-signature", Provenance.newBuilder());
  }

  /** Test parsing of the Questionnaire FHIR resource. */
  @Test
  public void parseQuestionnaire() throws Exception {
    testParse("Questionnaire-3141", Questionnaire.newBuilder());
    testParse("Questionnaire-bb", Questionnaire.newBuilder());
    testParse("Questionnaire-f201", Questionnaire.newBuilder());
    testParse("Questionnaire-gcs", Questionnaire.newBuilder());
  }

  /** Test printing of the Questionnaire FHIR resource. */
  @Test
  public void printQuestionnaire() throws Exception {
    testPrint("Questionnaire-3141", Questionnaire.newBuilder());
    testPrint("Questionnaire-bb", Questionnaire.newBuilder());
    testPrint("Questionnaire-f201", Questionnaire.newBuilder());
    testPrint("Questionnaire-gcs", Questionnaire.newBuilder());
  }

  /** Test parsing of the QuestionnaireResponse FHIR resource. */
  @Test
  public void parseQuestionnaireResponse() throws Exception {
    testParse("QuestionnaireResponse-3141", QuestionnaireResponse.newBuilder());
    testParse("QuestionnaireResponse-bb", QuestionnaireResponse.newBuilder());
    testParse("QuestionnaireResponse-f201", QuestionnaireResponse.newBuilder());
    testParse("QuestionnaireResponse-gcs", QuestionnaireResponse.newBuilder());
    testParse("QuestionnaireResponse-ussg-fht-answers", QuestionnaireResponse.newBuilder());
  }

  /** Test printing of the QuestionnaireResponse FHIR resource. */
  @Test
  public void printQuestionnaireResponse() throws Exception {
    testPrint("QuestionnaireResponse-3141", QuestionnaireResponse.newBuilder());
    testPrint("QuestionnaireResponse-bb", QuestionnaireResponse.newBuilder());
    testPrint("QuestionnaireResponse-f201", QuestionnaireResponse.newBuilder());
    testPrint("QuestionnaireResponse-gcs", QuestionnaireResponse.newBuilder());
    testPrint("QuestionnaireResponse-ussg-fht-answers", QuestionnaireResponse.newBuilder());
  }

  /** Test parsing of the ReferralRequest FHIR resource. */
  @Test
  public void parseReferralRequest() throws Exception {
    testParse("ReferralRequest-example", ReferralRequest.newBuilder());
  }

  /** Test printing of the ReferralRequest FHIR resource. */
  @Test
  public void printReferralRequest() throws Exception {
    testPrint("ReferralRequest-example", ReferralRequest.newBuilder());
  }

  /** Test parsing of the RelatedPerson FHIR resource. */
  @Test
  public void parseRelatedPerson() throws Exception {
    testParse("RelatedPerson-benedicte", RelatedPerson.newBuilder());
    testParse("RelatedPerson-f001", RelatedPerson.newBuilder());
    testParse("RelatedPerson-f002", RelatedPerson.newBuilder());
    testParse("RelatedPerson-peter", RelatedPerson.newBuilder());
  }

  /** Test printing of the RelatedPerson FHIR resource. */
  @Test
  public void printRelatedPerson() throws Exception {
    testPrint("RelatedPerson-benedicte", RelatedPerson.newBuilder());
    testPrint("RelatedPerson-f001", RelatedPerson.newBuilder());
    testPrint("RelatedPerson-f002", RelatedPerson.newBuilder());
    testPrint("RelatedPerson-peter", RelatedPerson.newBuilder());
  }

  /** Test parsing of the RequestGroup FHIR resource. */
  @Test
  public void parseRequestGroup() throws Exception {
    testParse("RequestGroup-example", RequestGroup.newBuilder());
    testParse("RequestGroup-kdn5-example", RequestGroup.newBuilder());
  }

  /** Test printing of the RequestGroup FHIR resource. */
  @Test
  public void printRequestGroup() throws Exception {
    testPrint("RequestGroup-example", RequestGroup.newBuilder());
    testPrint("RequestGroup-kdn5-example", RequestGroup.newBuilder());
  }

  /** Test parsing of the ResearchStudy FHIR resource. */
  @Test
  public void parseResearchStudy() throws Exception {
    testParse("ResearchStudy-example", ResearchStudy.newBuilder());
  }

  /** Test printing of the ResearchStudy FHIR resource. */
  @Test
  public void printResearchStudy() throws Exception {
    testPrint("ResearchStudy-example", ResearchStudy.newBuilder());
  }

  /** Test parsing of the ResearchSubject FHIR resource. */
  @Test
  public void parseResearchSubject() throws Exception {
    testParse("ResearchSubject-example", ResearchSubject.newBuilder());
  }

  /** Test printing of the ResearchSubject FHIR resource. */
  @Test
  public void printResearchSubject() throws Exception {
    testPrint("ResearchSubject-example", ResearchSubject.newBuilder());
  }

  /** Test parsing of the RiskAssessment FHIR resource. */
  @Test
  public void parseRiskAssessment() throws Exception {
    testParse("RiskAssessment-genetic", RiskAssessment.newBuilder());
    testParse("RiskAssessment-cardiac", RiskAssessment.newBuilder());
    testParse("RiskAssessment-population", RiskAssessment.newBuilder());
    testParse("RiskAssessment-prognosis", RiskAssessment.newBuilder());
  }

  /** Test printing of the RiskAssessment FHIR resource. */
  @Test
  public void printRiskAssessment() throws Exception {
    testPrint("RiskAssessment-genetic", RiskAssessment.newBuilder());
    testPrint("RiskAssessment-cardiac", RiskAssessment.newBuilder());
    testPrint("RiskAssessment-population", RiskAssessment.newBuilder());
    testPrint("RiskAssessment-prognosis", RiskAssessment.newBuilder());
  }

  /** Test parsing of the Schedule FHIR resource. */
  @Test
  public void parseSchedule() throws Exception {
    testParse("Schedule-example", Schedule.newBuilder());
    testParse("Schedule-exampleloc1", Schedule.newBuilder());
    testParse("Schedule-exampleloc2", Schedule.newBuilder());
  }

  /** Test printing of the Schedule FHIR resource. */
  @Test
  public void printSchedule() throws Exception {
    testPrint("Schedule-example", Schedule.newBuilder());
    testPrint("Schedule-exampleloc1", Schedule.newBuilder());
    testPrint("Schedule-exampleloc2", Schedule.newBuilder());
  }

  /** Test parsing of the SearchParameter FHIR resource. */
  @Test
  public void parseSearchParameter() throws Exception {
    testParse("SearchParameter-example", SearchParameter.newBuilder());
    testParse("SearchParameter-example-extension", SearchParameter.newBuilder());
    testParse("SearchParameter-example-reference", SearchParameter.newBuilder());
  }

  /** Test printing of the SearchParameter FHIR resource. */
  @Test
  public void printSearchParameter() throws Exception {
    testPrint("SearchParameter-example", SearchParameter.newBuilder());
    testPrint("SearchParameter-example-extension", SearchParameter.newBuilder());
    testPrint("SearchParameter-example-reference", SearchParameter.newBuilder());
  }

  /** Test parsing of the Sequence FHIR resource. */
  @Test
  public void parseSequence() throws Exception {
    testParse("Sequence-coord-0-base", Sequence.newBuilder());
    testParse("Sequence-coord-1-base", Sequence.newBuilder());
    testParse("Sequence-example", Sequence.newBuilder());
    testParse("Sequence-fda-example", Sequence.newBuilder());
    testParse("Sequence-fda-vcf-comparison", Sequence.newBuilder());
    testParse("Sequence-fda-vcfeval-comparison", Sequence.newBuilder());
    testParse("Sequence-example-pgx-1", Sequence.newBuilder());
    testParse("Sequence-example-pgx-2", Sequence.newBuilder());
    testParse("Sequence-example-TPMT-one", Sequence.newBuilder());
    testParse("Sequence-example-TPMT-two", Sequence.newBuilder());
    testParse("Sequence-graphic-example-1", Sequence.newBuilder());
    testParse("Sequence-graphic-example-2", Sequence.newBuilder());
    testParse("Sequence-graphic-example-3", Sequence.newBuilder());
    testParse("Sequence-graphic-example-4", Sequence.newBuilder());
    testParse("Sequence-graphic-example-5", Sequence.newBuilder());
  }

  /** Test printing of the Sequence FHIR resource. */
  @Test
  public void printSequence() throws Exception {
    testPrint("Sequence-coord-0-base", Sequence.newBuilder());
    testPrint("Sequence-coord-1-base", Sequence.newBuilder());
    testPrint("Sequence-example", Sequence.newBuilder());
    testPrint("Sequence-fda-example", Sequence.newBuilder());
    testPrint("Sequence-fda-vcf-comparison", Sequence.newBuilder());
    testPrint("Sequence-fda-vcfeval-comparison", Sequence.newBuilder());
    testPrint("Sequence-example-pgx-1", Sequence.newBuilder());
    testPrint("Sequence-example-pgx-2", Sequence.newBuilder());
    testPrint("Sequence-example-TPMT-one", Sequence.newBuilder());
    testPrint("Sequence-example-TPMT-two", Sequence.newBuilder());
    testPrint("Sequence-graphic-example-1", Sequence.newBuilder());
    testPrint("Sequence-graphic-example-2", Sequence.newBuilder());
    testPrint("Sequence-graphic-example-3", Sequence.newBuilder());
    testPrint("Sequence-graphic-example-4", Sequence.newBuilder());
    testPrint("Sequence-graphic-example-5", Sequence.newBuilder());
  }

  /** Test parsing of the ServiceDefinition FHIR resource. */
  @Test
  public void parseServiceDefinition() throws Exception {
    testParse("ServiceDefinition-example", ServiceDefinition.newBuilder());
  }

  /** Test printing of the ServiceDefinition FHIR resource. */
  @Test
  public void printServiceDefinition() throws Exception {
    testPrint("ServiceDefinition-example", ServiceDefinition.newBuilder());
  }

  /** Test parsing of the Slot FHIR resource. */
  @Test
  public void parseSlot() throws Exception {
    testParse("Slot-example", Slot.newBuilder());
    testParse("Slot-1", Slot.newBuilder());
    testParse("Slot-2", Slot.newBuilder());
    testParse("Slot-3", Slot.newBuilder());
  }

  /** Test printing of the Slot FHIR resource. */
  @Test
  public void printSlot() throws Exception {
    testPrint("Slot-example", Slot.newBuilder());
    testPrint("Slot-1", Slot.newBuilder());
    testPrint("Slot-2", Slot.newBuilder());
    testPrint("Slot-3", Slot.newBuilder());
  }

  /** Test parsing of the Specimen FHIR resource. */
  @Test
  public void parseSpecimen() throws Exception {
    testParse("Specimen-101", Specimen.newBuilder());
    testParse("Specimen-isolate", Specimen.newBuilder());
    testParse("Specimen-sst", Specimen.newBuilder());
    testParse("Specimen-vma-urine", Specimen.newBuilder());
  }

  /** Test printing of the Specimen FHIR resource. */
  @Test
  public void printSpecimen() throws Exception {
    testPrint("Specimen-101", Specimen.newBuilder());
    testPrint("Specimen-isolate", Specimen.newBuilder());
    testPrint("Specimen-sst", Specimen.newBuilder());
    testPrint("Specimen-vma-urine", Specimen.newBuilder());
  }

  /** Test parsing of the StructureDefinition FHIR resource. */
  @Test
  public void parseStructureDefinition() throws Exception {
    testParse("StructureDefinition-example", StructureDefinition.newBuilder());
  }

  /** Test printing of the StructureDefinition FHIR resource. */
  @Test
  public void printStructureDefinition() throws Exception {
    testPrint("StructureDefinition-example", StructureDefinition.newBuilder());
  }

  /** Test parsing of the StructureMap FHIR resource. */
  @Test
  public void parseStructureMap() throws Exception {
    testParse("StructureMap-example", StructureMap.newBuilder());
  }

  /** Test printing of the StructureMap FHIR resource. */
  @Test
  public void printStructureMap() throws Exception {
    testPrint("StructureMap-example", StructureMap.newBuilder());
  }

  /** Test parsing of the Subscription FHIR resource. */
  @Test
  public void parseSubscription() throws Exception {
    testParse("Subscription-example", Subscription.newBuilder());
    testParse("Subscription-example-error", Subscription.newBuilder());
  }

  /** Test printing of the Subscription FHIR resource. */
  @Test
  public void printSubscription() throws Exception {
    testPrint("Subscription-example", Subscription.newBuilder());
    testPrint("Subscription-example-error", Subscription.newBuilder());
  }

  /** Test parsing of the Substance FHIR resource. */
  @Test
  public void parseSubstance() throws Exception {
    testParse("Substance-example", Substance.newBuilder());
    testParse("Substance-f205", Substance.newBuilder());
    testParse("Substance-f201", Substance.newBuilder());
    testParse("Substance-f202", Substance.newBuilder());
    testParse("Substance-f203", Substance.newBuilder());
    testParse("Substance-f204", Substance.newBuilder());
  }

  /** Test printing of the Substance FHIR resource. */
  @Test
  public void printSubstance() throws Exception {
    testPrint("Substance-example", Substance.newBuilder());
    testPrint("Substance-f205", Substance.newBuilder());
    testPrint("Substance-f201", Substance.newBuilder());
    testPrint("Substance-f202", Substance.newBuilder());
    testPrint("Substance-f203", Substance.newBuilder());
    testPrint("Substance-f204", Substance.newBuilder());
  }

  /** Test parsing of the SupplyDelivery FHIR resource. */
  @Test
  public void parseSupplyDelivery() throws Exception {
    testParse("SupplyDelivery-simpledelivery", SupplyDelivery.newBuilder());
    testParse("SupplyDelivery-pumpdelivery", SupplyDelivery.newBuilder());
  }

  /** Test printing of the SupplyDelivery FHIR resource. */
  @Test
  public void printSupplyDelivery() throws Exception {
    testPrint("SupplyDelivery-simpledelivery", SupplyDelivery.newBuilder());
    testPrint("SupplyDelivery-pumpdelivery", SupplyDelivery.newBuilder());
  }

  /** Test parsing of the SupplyRequest FHIR resource. */
  @Test
  public void parseSupplyRequest() throws Exception {
    testParse("SupplyRequest-simpleorder", SupplyRequest.newBuilder());
  }

  /** Test printing of the SupplyRequest FHIR resource. */
  @Test
  public void printSupplyRequest() throws Exception {
    testPrint("SupplyRequest-simpleorder", SupplyRequest.newBuilder());
  }

  /** Test parsing of the Task FHIR resource. */
  @Test
  public void parseTask() throws Exception {
    testParse("Task-example1", Task.newBuilder());
    testParse("Task-example2", Task.newBuilder());
    testParse("Task-example3", Task.newBuilder());
    testParse("Task-example4", Task.newBuilder());
    testParse("Task-example5", Task.newBuilder());
    testParse("Task-example6", Task.newBuilder());
  }

  /** Test printing of the Task FHIR resource. */
  @Test
  public void printTask() throws Exception {
    testPrint("Task-example1", Task.newBuilder());
    testPrint("Task-example2", Task.newBuilder());
    testPrint("Task-example3", Task.newBuilder());
    testPrint("Task-example4", Task.newBuilder());
    testPrint("Task-example5", Task.newBuilder());
    testPrint("Task-example6", Task.newBuilder());
  }

  /** Test parsing of the TestReport FHIR resource. */
  @Test
  public void parseTestReport() throws Exception {
    testParse("TestReport-testreport-example", TestReport.newBuilder());
  }

  /** Test printing of the TestReport FHIR resource. */
  @Test
  public void printTestReport() throws Exception {
    testPrint("TestReport-testreport-example", TestReport.newBuilder());
  }

  /** Test parsing of the TestScript FHIR resource. */
  @Test
  public void parseTestScript() throws Exception {
    testParse("TestScript-testscript-example", TestScript.newBuilder());
    testParse("TestScript-testscript-example-history", TestScript.newBuilder());
    testParse("TestScript-testscript-example-multisystem", TestScript.newBuilder());
    testParse("TestScript-testscript-example-readtest", TestScript.newBuilder());
    testParse("TestScript-testscript-example-rule", TestScript.newBuilder());
    testParse("TestScript-testscript-example-search", TestScript.newBuilder());
    testParse("TestScript-testscript-example-update", TestScript.newBuilder());
  }

  /** Test printing of the TestScript FHIR resource. */
  @Test
  public void printTestScript() throws Exception {
    testPrint("TestScript-testscript-example", TestScript.newBuilder());
    testPrint("TestScript-testscript-example-history", TestScript.newBuilder());
    testPrint("TestScript-testscript-example-multisystem", TestScript.newBuilder());
    testPrint("TestScript-testscript-example-readtest", TestScript.newBuilder());
    testPrint("TestScript-testscript-example-rule", TestScript.newBuilder());
    testPrint("TestScript-testscript-example-search", TestScript.newBuilder());
    testPrint("TestScript-testscript-example-update", TestScript.newBuilder());
  }

  /** Test parsing of the ValueSet FHIR resource. */
  @Test
  public void parseValueSet() throws Exception {
    testParse("valueset-example", ValueSet.newBuilder());
    testParse("valueset-example-expansion", ValueSet.newBuilder());
    testParse("valueset-example-inactive", ValueSet.newBuilder());
    testParse("valueset-example-intensional", ValueSet.newBuilder());
    testParse("valueset-example-yesnodontknow", ValueSet.newBuilder());
    testParse("valueset-list-example-codes", ValueSet.newBuilder());
  }

  /** Test printing of the ValueSet FHIR resource. */
  @Test
  public void printValueSet() throws Exception {
    testPrint("valueset-example", ValueSet.newBuilder());
    testPrint("valueset-example-expansion", ValueSet.newBuilder());
    testPrint("valueset-example-inactive", ValueSet.newBuilder());
    testPrint("valueset-example-intensional", ValueSet.newBuilder());
    testPrint("valueset-example-yesnodontknow", ValueSet.newBuilder());
    testPrint("valueset-list-example-codes", ValueSet.newBuilder());
  }

  /** Test parsing of the VisionPrescription FHIR resource. */
  @Test
  public void parseVisionPrescription() throws Exception {
    testParse("VisionPrescription-33123", VisionPrescription.newBuilder());
    testParse("VisionPrescription-33124", VisionPrescription.newBuilder());
  }

  /** Test printing of the VisionPrescription FHIR resource. */
  @Test
  public void printVisionPrescription() throws Exception {
    testPrint("VisionPrescription-33123", VisionPrescription.newBuilder());
    testPrint("VisionPrescription-33124", VisionPrescription.newBuilder());
  }
}
