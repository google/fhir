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

import com.google.common.io.Files;
import com.google.devtools.build.runfiles.Runfiles;
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
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.stream.JsonReader;
import com.google.protobuf.Message;
import com.google.protobuf.Message.Builder;
import com.google.protobuf.TextFormat;
import java.io.File;
import java.io.IOException;
import java.io.StringReader;
import java.nio.charset.StandardCharsets;
import java.time.ZoneId;
import java.util.Map;
import java.util.TreeSet;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link JsonFormat}. */
@RunWith(JUnit4.class)
public class JsonFormatTest {
  private JsonFormat.Parser jsonParser;
  private JsonFormat.Printer jsonPrinter;
  private TextFormat.Parser textParser;
  private Runfiles runfiles;

  /** Read the specifed json file from the testdata directory as a String. */
  private String loadJson(String filename) throws IOException {
    File file =
        new File(runfiles.rlocation("com_google_fhir/testdata/stu3/examples/" + filename));
    return Files.asCharSource(file, StandardCharsets.UTF_8).read();
  }

  /** Read the specifed prototxt file from the testdata directory and parse it. */
  private void mergeText(String filename, Message.Builder builder) throws IOException {
    File file =
        new File(runfiles.rlocation("com_google_fhir/testdata/stu3/examples/" + filename));
    textParser.merge(Files.asCharSource(file, StandardCharsets.UTF_8).read(), builder);
  }

  private void testParse(String name, Builder builder) throws IOException {
    // Parse the json version of the input.
    Builder jsonBuilder = builder.clone();
    jsonParser.merge(loadJson(name + ".json"), jsonBuilder);
    // Parse the proto text version of the input.
    Builder textBuilder = builder.clone();
    mergeText(name + ".prototxt", textBuilder);

    assertThat(jsonBuilder.build().toString()).isEqualTo(textBuilder.build().toString());
  }

  private JsonElement canonicalize(JsonElement element) {
    if (element.isJsonObject()) {
      JsonObject object = element.getAsJsonObject();
      JsonObject sorted = new JsonObject();
      TreeSet<String> keys = new TreeSet<>();
      for (Map.Entry<String, JsonElement> entry : object.entrySet()) {
        keys.add(entry.getKey());
      }
      for (String key : keys) {
        sorted.add(key, canonicalize(object.get(key)));
      }
      return sorted;
    }
    if (element.isJsonArray()) {
      JsonArray sorted = new JsonArray();
      for (JsonElement e : element.getAsJsonArray()) {
        sorted.add(canonicalize(e));
      }
      return sorted;
    }
    return element;
  }

  private String canonicalizeJson(String json) {
    com.google.gson.JsonParser gsonParser = new com.google.gson.JsonParser();
    JsonElement testJson = canonicalize(gsonParser.parse(new JsonReader(new StringReader(json))));
    return testJson.toString();
  }

  private void testPrint(String name, Builder builder) throws IOException {
    // Parse the proto text version of the input.
    Builder textBuilder = builder.clone();
    mergeText(name + ".prototxt", textBuilder);
    // Load the json version of the input as a String.
    String jsonGolden = loadJson(name + ".json");
    // Print the proto as json and compare.
    String jsonTest = jsonPrinter.print(textBuilder);
    assertThat(jsonTest).isEqualTo(jsonGolden);
  }

  @Before
  public void setUp() throws IOException {
    jsonParser =
        JsonFormat.Parser.newBuilder().withDefaultTimeZone(ZoneId.of("Australia/Sydney")).build();
    jsonPrinter = JsonFormat.getPrinter().withDefaultTimeZone(ZoneId.of("Australia/Sydney"));
    textParser = TextFormat.getParser();
    runfiles = Runfiles.create();
  }

  /** Test parsing JSON edge cases. */
  @Test
  public void parseEdgeCases() throws Exception {
    testParse("json-edge-cases", Patient.newBuilder());
  }

  /**
   * Test printing JSON edge cases. Since this json file is not sorted in any particular way, we
   * sort the json objects directly and compare them instead of comparing the raw strings.
   */
  @Test
  public void printEdgeCases() throws Exception {
    String jsonGolden = loadJson("json-edge-cases.json");
    Patient.Builder patient = Patient.newBuilder();
    mergeText("json-edge-cases.prototxt", patient);
    String jsonTest = jsonPrinter.print(patient);
    assertThat(canonicalizeJson(jsonTest)).isEqualTo(canonicalizeJson(jsonGolden));
  }

  /* Resource tests start here. */

  /** Test parsing of the Account FHIR resource. */
  @Test
  public void parseAccount() throws Exception {
    testParse("account-example", Account.newBuilder());
    testParse("account-example-with-guarantor", Account.newBuilder());
  }

  /** Test printing of the Account FHIR resource. */
  @Test
  public void printAccount() throws Exception {
    testPrint("account-example", Account.newBuilder());
    testPrint("account-example-with-guarantor", Account.newBuilder());
  }

  /** Test parsing of the ActivityDefinition FHIR resource. */
  @Test
  public void parseActivityDefinition() throws Exception {
    testParse("activitydefinition-example", ActivityDefinition.newBuilder());
    testParse("activitydefinition-medicationorder-example", ActivityDefinition.newBuilder());
    testParse("activitydefinition-predecessor-example", ActivityDefinition.newBuilder());
    testParse("activitydefinition-procedurerequest-example", ActivityDefinition.newBuilder());
    testParse("activitydefinition-supplyrequest-example", ActivityDefinition.newBuilder());
  }

  /** Test printing of the ActivityDefinition FHIR resource. */
  @Test
  public void printActivityDefinition() throws Exception {
    testPrint("activitydefinition-example", ActivityDefinition.newBuilder());
    testPrint("activitydefinition-medicationorder-example", ActivityDefinition.newBuilder());
    testPrint("activitydefinition-predecessor-example", ActivityDefinition.newBuilder());
    testPrint("activitydefinition-procedurerequest-example", ActivityDefinition.newBuilder());
    testPrint("activitydefinition-supplyrequest-example", ActivityDefinition.newBuilder());
  }

  /** Test parsing of the AdverseEvent FHIR resource. */
  @Test
  public void parseAdverseEvent() throws Exception {
    testParse("adverseevent-example", AdverseEvent.newBuilder());
  }

  /** Test printing of the AdverseEvent FHIR resource. */
  @Test
  public void printAdverseEvent() throws Exception {
    testPrint("adverseevent-example", AdverseEvent.newBuilder());
  }

  /** Test parsing of the AllergyIntolerance FHIR resource. */
  @Test
  public void parseAllergyIntolerance() throws Exception {
    testParse("allergyintolerance-example", AllergyIntolerance.newBuilder());
  }

  /** Test printing of the AllergyIntolerance FHIR resource. */
  @Test
  public void printAllergyIntolerance() throws Exception {
    testPrint("allergyintolerance-example", AllergyIntolerance.newBuilder());
  }

  /** Test parsing of the Appointment FHIR resource. */
  @Test
  public void parseAppointment() throws Exception {
    testParse("appointment-example", Appointment.newBuilder());
    testParse("appointment-example2doctors", Appointment.newBuilder());
    testParse("appointment-example-request", Appointment.newBuilder());
  }

  /** Test printing of the Appointment FHIR resource. */
  @Test
  public void printAppointment() throws Exception {
    testPrint("appointment-example", Appointment.newBuilder());
    testPrint("appointment-example2doctors", Appointment.newBuilder());
    testPrint("appointment-example-request", Appointment.newBuilder());
  }

  /** Test parsing of the AppointmentResponse FHIR resource. */
  @Test
  public void parseAppointmentResponse() throws Exception {
    testParse("appointmentresponse-example", AppointmentResponse.newBuilder());
    testParse("appointmentresponse-example-req", AppointmentResponse.newBuilder());
  }

  /** Test printing of the AppointmentResponse FHIR resource. */
  @Test
  public void printAppointmentResponse() throws Exception {
    testPrint("appointmentresponse-example", AppointmentResponse.newBuilder());
    testPrint("appointmentresponse-example-req", AppointmentResponse.newBuilder());
  }

  /** Test parsing of the AuditEvent FHIR resource. */
  @Test
  public void parseAuditEvent() throws Exception {
    testParse("auditevent-example", AuditEvent.newBuilder());
    testParse("auditevent-example-disclosure", AuditEvent.newBuilder());
    testParse("audit-event-example-login", AuditEvent.newBuilder());
    testParse("audit-event-example-logout", AuditEvent.newBuilder());
    testParse("audit-event-example-media", AuditEvent.newBuilder());
    testParse("audit-event-example-pixQuery", AuditEvent.newBuilder());
    testParse("audit-event-example-search", AuditEvent.newBuilder());
    testParse("audit-event-example-vread", AuditEvent.newBuilder());
  }

  /** Test printing of the AuditEvent FHIR resource. */
  @Test
  public void printAuditEvent() throws Exception {
    testPrint("auditevent-example", AuditEvent.newBuilder());
    testPrint("auditevent-example-disclosure", AuditEvent.newBuilder());
    testPrint("audit-event-example-login", AuditEvent.newBuilder());
    testPrint("audit-event-example-logout", AuditEvent.newBuilder());
    testPrint("audit-event-example-media", AuditEvent.newBuilder());
    testPrint("audit-event-example-pixQuery", AuditEvent.newBuilder());
    testPrint("audit-event-example-search", AuditEvent.newBuilder());
    testPrint("audit-event-example-vread", AuditEvent.newBuilder());
  }

  /** Test parsing of the Basic FHIR resource. */
  @Test
  public void parseBasic() throws Exception {
    testParse("basic-example", Basic.newBuilder());
    testParse("basic-example2", Basic.newBuilder());
    testParse("basic-example-narrative", Basic.newBuilder());
  }

  /** Test printing of the Basic FHIR resource. */
  @Test
  public void printBasic() throws Exception {
    testPrint("basic-example", Basic.newBuilder());
    testPrint("basic-example2", Basic.newBuilder());
    testPrint("basic-example-narrative", Basic.newBuilder());
  }

  /** Test parsing of the Binary FHIR resource. */
  @Test
  public void parseBinary() throws Exception {
    testParse("binary-example", Binary.newBuilder());
  }

  /** Test printing of the Binary FHIR resource. */
  @Test
  public void printBinary() throws Exception {
    testPrint("binary-example", Binary.newBuilder());
  }

  /** Test parsing of the BodySite FHIR resource. */
  @Test
  public void parseBodySite() throws Exception {
    testParse("bodysite-example-fetus", BodySite.newBuilder());
    testParse("bodysite-example-skin-patch", BodySite.newBuilder());
    testParse("bodysite-example-tumor", BodySite.newBuilder());
  }

  /** Test printing of the BodySite FHIR resource. */
  @Test
  public void printBodySite() throws Exception {
    testPrint("bodysite-example-fetus", BodySite.newBuilder());
    testPrint("bodysite-example-skin-patch", BodySite.newBuilder());
    testPrint("bodysite-example-tumor", BodySite.newBuilder());
  }

  /** Test parsing of the Bundle FHIR resource. */
  @Test
  public void parseBundle() throws Exception {
    testParse("bundle-example", Bundle.newBuilder());
    testParse("diagnosticreport-examples-general", Bundle.newBuilder());
    testParse("diagnosticreport-hla-genetics-results-example", Bundle.newBuilder());
    testParse("document-example-dischargesummary", Bundle.newBuilder());
    testParse("endpoint-examples-general-template", Bundle.newBuilder());
    testParse("location-examples-general", Bundle.newBuilder());
    testParse("patient-examples-cypress-template", Bundle.newBuilder());
    testParse("patient-examples-general", Bundle.newBuilder());
    testParse("practitioner-examples-general", Bundle.newBuilder());
    testParse("practitionerrole-examples-general", Bundle.newBuilder());
    testParse("questionnaire-profile-example-ussg-fht", Bundle.newBuilder());
    testParse("xds-example", Bundle.newBuilder());
  }

  /** Test printing of the Bundle FHIR resource. */
  @Test
  public void printBundle() throws Exception {
    testPrint("bundle-example", Bundle.newBuilder());
    testPrint("diagnosticreport-examples-general", Bundle.newBuilder());
    testPrint("diagnosticreport-hla-genetics-results-example", Bundle.newBuilder());
    testPrint("document-example-dischargesummary", Bundle.newBuilder());
    testPrint("endpoint-examples-general-template", Bundle.newBuilder());
    testPrint("location-examples-general", Bundle.newBuilder());
    testPrint("patient-examples-cypress-template", Bundle.newBuilder());
    testPrint("patient-examples-general", Bundle.newBuilder());
    testPrint("practitioner-examples-general", Bundle.newBuilder());
    testPrint("practitionerrole-examples-general", Bundle.newBuilder());
    testPrint("questionnaire-profile-example-ussg-fht", Bundle.newBuilder());
    testPrint("xds-example", Bundle.newBuilder());
  }

  /** Test parsing of the CapabilityStatement FHIR resource. */
  @Test
  public void parseCapabilityStatement() throws Exception {
    testParse("capabilitystatement-example", CapabilityStatement.newBuilder());
    testParse("capabilitystatement-phr-example", CapabilityStatement.newBuilder());
  }

  /** Test printing of the CapabilityStatement FHIR resource. */
  @Test
  public void printCapabilityStatement() throws Exception {
    testPrint("capabilitystatement-example", CapabilityStatement.newBuilder());
    testPrint("capabilitystatement-phr-example", CapabilityStatement.newBuilder());
  }

  /** Test parsing of the CarePlan FHIR resource. */
  @Test
  public void parseCarePlan() throws Exception {
    testParse("careplan-example", CarePlan.newBuilder());
    testParse("careplan-example-f001-heart", CarePlan.newBuilder());
    testParse("careplan-example-f002-lung", CarePlan.newBuilder());
    testParse("careplan-example-f003-pharynx", CarePlan.newBuilder());
    testParse("careplan-example-f201-renal", CarePlan.newBuilder());
    testParse("careplan-example-f202-malignancy", CarePlan.newBuilder());
    testParse("careplan-example-f203-sepsis", CarePlan.newBuilder());
    testParse("careplan-example-GPVisit", CarePlan.newBuilder());
    testParse("careplan-example-integrated", CarePlan.newBuilder());
    testParse("careplan-example-obesity-narrative", CarePlan.newBuilder());
    testParse("careplan-example-pregnancy", CarePlan.newBuilder());
  }

  /** Test printing of the CarePlan FHIR resource. */
  @Test
  public void printCarePlan() throws Exception {
    testPrint("careplan-example", CarePlan.newBuilder());
    testPrint("careplan-example-f001-heart", CarePlan.newBuilder());
    testPrint("careplan-example-f002-lung", CarePlan.newBuilder());
    testPrint("careplan-example-f003-pharynx", CarePlan.newBuilder());
    testPrint("careplan-example-f201-renal", CarePlan.newBuilder());
    testPrint("careplan-example-f202-malignancy", CarePlan.newBuilder());
    testPrint("careplan-example-f203-sepsis", CarePlan.newBuilder());
    testPrint("careplan-example-GPVisit", CarePlan.newBuilder());
    testPrint("careplan-example-integrated", CarePlan.newBuilder());
    testPrint("careplan-example-obesity-narrative", CarePlan.newBuilder());
    testPrint("careplan-example-pregnancy", CarePlan.newBuilder());
  }

  /** Test parsing of the CareTeam FHIR resource. */
  @Test
  public void parseCareTeam() throws Exception {
    testParse("careteam-example", CareTeam.newBuilder());
  }

  /** Test printing of the CareTeam FHIR resource. */
  @Test
  public void printCareTeam() throws Exception {
    testPrint("careteam-example", CareTeam.newBuilder());
  }

  /** Test parsing of the ChargeItem FHIR resource. */
  @Test
  public void parseChargeItem() throws Exception {
    testParse("chargeitem-example", ChargeItem.newBuilder());
  }

  /** Test printing of the ChargeItem FHIR resource. */
  @Test
  public void printChargeItem() throws Exception {
    testPrint("chargeitem-example", ChargeItem.newBuilder());
  }

  /** Test parsing of the Claim FHIR resource. */
  @Test
  public void parseClaim() throws Exception {
    testParse("claim-example", Claim.newBuilder());
    testParse("claim-example-institutional", Claim.newBuilder());
    testParse("claim-example-institutional-rich", Claim.newBuilder());
    testParse("claim-example-oral-average", Claim.newBuilder());
    testParse("claim-example-oral-bridge", Claim.newBuilder());
    testParse("claim-example-oral-contained", Claim.newBuilder());
    testParse("claim-example-oral-contained-identifier", Claim.newBuilder());
    testParse("claim-example-oral-identifier", Claim.newBuilder());
    testParse("claim-example-oral-orthoplan", Claim.newBuilder());
    testParse("claim-example-pharmacy", Claim.newBuilder());
    testParse("claim-example-pharmacy-compound", Claim.newBuilder());
    testParse("claim-example-pharmacy-medication", Claim.newBuilder());
    testParse("claim-example-professional", Claim.newBuilder());
    testParse("claim-example-vision", Claim.newBuilder());
    testParse("claim-example-vision-glasses", Claim.newBuilder());
    testParse("claim-example-vision-glasses-3tier", Claim.newBuilder());
  }

  /** Test printing of the Claim FHIR resource. */
  @Test
  public void printClaim() throws Exception {
    testPrint("claim-example", Claim.newBuilder());
    testPrint("claim-example-institutional", Claim.newBuilder());
    testPrint("claim-example-institutional-rich", Claim.newBuilder());
    testPrint("claim-example-oral-average", Claim.newBuilder());
    testPrint("claim-example-oral-bridge", Claim.newBuilder());
    testPrint("claim-example-oral-contained", Claim.newBuilder());
    testPrint("claim-example-oral-contained-identifier", Claim.newBuilder());
    testPrint("claim-example-oral-identifier", Claim.newBuilder());
    testPrint("claim-example-oral-orthoplan", Claim.newBuilder());
    testPrint("claim-example-pharmacy", Claim.newBuilder());
    testPrint("claim-example-pharmacy-compound", Claim.newBuilder());
    testPrint("claim-example-pharmacy-medication", Claim.newBuilder());
    testPrint("claim-example-professional", Claim.newBuilder());
    testPrint("claim-example-vision", Claim.newBuilder());
    testPrint("claim-example-vision-glasses", Claim.newBuilder());
    testPrint("claim-example-vision-glasses-3tier", Claim.newBuilder());
  }

  /** Test parsing of the ClaimResponse FHIR resource. */
  @Test
  public void parseClaimResponse() throws Exception {
    testParse("claimresponse-example", ClaimResponse.newBuilder());
  }

  /** Test printing of the ClaimResponse FHIR resource. */
  @Test
  public void printClaimResponse() throws Exception {
    testPrint("claimresponse-example", ClaimResponse.newBuilder());
  }

  /** Test parsing of the ClinicalImpression FHIR resource. */
  @Test
  public void parseClinicalImpression() throws Exception {
    testParse("clinicalimpression-example", ClinicalImpression.newBuilder());
  }

  /** Test printing of the ClinicalImpression FHIR resource. */
  @Test
  public void printClinicalImpression() throws Exception {
    testPrint("clinicalimpression-example", ClinicalImpression.newBuilder());
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
    testParse("communication-example", Communication.newBuilder());
    testParse("communication-example-fm-attachment", Communication.newBuilder());
    testParse("communication-example-fm-solicited-attachment", Communication.newBuilder());
  }

  /** Test printing of the Communication FHIR resource. */
  @Test
  public void printCommunication() throws Exception {
    testPrint("communication-example", Communication.newBuilder());
    testPrint("communication-example-fm-attachment", Communication.newBuilder());
    testPrint("communication-example-fm-solicited-attachment", Communication.newBuilder());
  }

  /** Test parsing of the CommunicationRequest FHIR resource. */
  @Test
  public void parseCommunicationRequest() throws Exception {
    testParse("communicationrequest-example", CommunicationRequest.newBuilder());
    testParse(
        "communicationrequest-example-fm-solicit-attachment", CommunicationRequest.newBuilder());
  }

  /** Test printing of the CommunicationRequest FHIR resource. */
  @Test
  public void printCommunicationRequest() throws Exception {
    testPrint("communicationrequest-example", CommunicationRequest.newBuilder());
    testPrint(
        "communicationrequest-example-fm-solicit-attachment", CommunicationRequest.newBuilder());
  }

  /** Test parsing of the CompartmentDefinition FHIR resource. */
  @Test
  public void parseCompartmentDefinition() throws Exception {
    testParse("compartmentdefinition-example", CompartmentDefinition.newBuilder());
  }

  /** Test printing of the CompartmentDefinition FHIR resource. */
  @Test
  public void printCompartmentDefinition() throws Exception {
    testPrint("compartmentdefinition-example", CompartmentDefinition.newBuilder());
  }

  /** Test parsing of the Composition FHIR resource. */
  @Test
  public void parseComposition() throws Exception {
    testParse("composition-example", Composition.newBuilder());
  }

  /** Test printing of the Composition FHIR resource. */
  @Test
  public void printComposition() throws Exception {
    testPrint("composition-example", Composition.newBuilder());
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
    testParse("condition-example", Condition.newBuilder());
    testParse("condition-example2", Condition.newBuilder());
    testParse("condition-example-f001-heart", Condition.newBuilder());
    testParse("condition-example-f002-lung", Condition.newBuilder());
    testParse("condition-example-f003-abscess", Condition.newBuilder());
    testParse("condition-example-f201-fever", Condition.newBuilder());
    testParse("condition-example-f202-malignancy", Condition.newBuilder());
    testParse("condition-example-f203-sepsis", Condition.newBuilder());
    testParse("condition-example-f204-renal", Condition.newBuilder());
    testParse("condition-example-f205-infection", Condition.newBuilder());
    testParse("condition-example-family-history", Condition.newBuilder());
    testParse("condition-example-stroke", Condition.newBuilder());
  }

  /** Test printing of the Condition FHIR resource. */
  @Test
  public void printCondition() throws Exception {
    testPrint("condition-example", Condition.newBuilder());
    testPrint("condition-example2", Condition.newBuilder());
    testPrint("condition-example-f001-heart", Condition.newBuilder());
    testPrint("condition-example-f002-lung", Condition.newBuilder());
    testPrint("condition-example-f003-abscess", Condition.newBuilder());
    testPrint("condition-example-f201-fever", Condition.newBuilder());
    testPrint("condition-example-f202-malignancy", Condition.newBuilder());
    testPrint("condition-example-f203-sepsis", Condition.newBuilder());
    testPrint("condition-example-f204-renal", Condition.newBuilder());
    testPrint("condition-example-f205-infection", Condition.newBuilder());
    testPrint("condition-example-family-history", Condition.newBuilder());
    testPrint("condition-example-stroke", Condition.newBuilder());
  }

  /** Test parsing of the Consent FHIR resource. */
  @Test
  public void parseConsent() throws Exception {
    testParse("consent-example", Consent.newBuilder());
    testParse("consent-example-Emergency", Consent.newBuilder());
    testParse("consent-example-grantor", Consent.newBuilder());
    testParse("consent-example-notAuthor", Consent.newBuilder());
    testParse("consent-example-notOrg", Consent.newBuilder());
    testParse("consent-example-notThem", Consent.newBuilder());
    testParse("consent-example-notThis", Consent.newBuilder());
    testParse("consent-example-notTime", Consent.newBuilder());
    testParse("consent-example-Out", Consent.newBuilder());
    testParse("consent-example-pkb", Consent.newBuilder());
    testParse("consent-example-signature", Consent.newBuilder());
    testParse("consent-example-smartonfhir", Consent.newBuilder());
  }

  /** Test printing of the Consent FHIR resource. */
  @Test
  public void printConsent() throws Exception {
    testPrint("consent-example", Consent.newBuilder());
    testPrint("consent-example-Emergency", Consent.newBuilder());
    testPrint("consent-example-grantor", Consent.newBuilder());
    testPrint("consent-example-notAuthor", Consent.newBuilder());
    testPrint("consent-example-notOrg", Consent.newBuilder());
    testPrint("consent-example-notThem", Consent.newBuilder());
    testPrint("consent-example-notThis", Consent.newBuilder());
    testPrint("consent-example-notTime", Consent.newBuilder());
    testPrint("consent-example-Out", Consent.newBuilder());
    testPrint("consent-example-pkb", Consent.newBuilder());
    testPrint("consent-example-signature", Consent.newBuilder());
    testPrint("consent-example-smartonfhir", Consent.newBuilder());
  }

  /** Test parsing of the Contract FHIR resource. */
  @Test
  public void parseContract() throws Exception {
    testParse("contract-example", Contract.newBuilder());
    testParse("contract-example-42cfr-part2", Contract.newBuilder());
    testParse("pcd-example-notAuthor", Contract.newBuilder());
    testParse("pcd-example-notLabs", Contract.newBuilder());
    testParse("pcd-example-notOrg", Contract.newBuilder());
    testParse("pcd-example-notThem", Contract.newBuilder());
    testParse("pcd-example-notThis", Contract.newBuilder());
  }

  /** Test printing of the Contract FHIR resource. */
  @Test
  public void printContract() throws Exception {
    testPrint("contract-example", Contract.newBuilder());
    testPrint("contract-example-42cfr-part2", Contract.newBuilder());
    testPrint("pcd-example-notAuthor", Contract.newBuilder());
    testPrint("pcd-example-notLabs", Contract.newBuilder());
    testPrint("pcd-example-notOrg", Contract.newBuilder());
    testPrint("pcd-example-notThem", Contract.newBuilder());
    testPrint("pcd-example-notThis", Contract.newBuilder());
  }

  /** Test parsing of the Coverage FHIR resource. */
  @Test
  public void parseCoverage() throws Exception {
    testParse("coverage-example", Coverage.newBuilder());
    testParse("coverage-example-2", Coverage.newBuilder());
    testParse("coverage-example-ehic", Coverage.newBuilder());
    testParse("coverage-example-selfpay", Coverage.newBuilder());
  }

  /** Test printing of the Coverage FHIR resource. */
  @Test
  public void printCoverage() throws Exception {
    testPrint("coverage-example", Coverage.newBuilder());
    testPrint("coverage-example-2", Coverage.newBuilder());
    testPrint("coverage-example-ehic", Coverage.newBuilder());
    testPrint("coverage-example-selfpay", Coverage.newBuilder());
  }

  /** Test parsing of the DataElement FHIR resource. */
  @Test
  public void parseDataElement() throws Exception {
    testParse("dataelement-example", DataElement.newBuilder());
    testParse("dataelement-labtestmaster-example", DataElement.newBuilder());
  }

  /** Test printing of the DataElement FHIR resource. */
  @Test
  public void printDataElement() throws Exception {
    testPrint("dataelement-example", DataElement.newBuilder());
    testPrint("dataelement-labtestmaster-example", DataElement.newBuilder());
  }

  /** Test parsing of the DetectedIssue FHIR resource. */
  @Test
  public void parseDetectedIssue() throws Exception {
    testParse("detectedissue-example", DetectedIssue.newBuilder());
    testParse("detectedissue-example-allergy", DetectedIssue.newBuilder());
    testParse("detectedissue-example-dup", DetectedIssue.newBuilder());
    testParse("detectedissue-example-lab", DetectedIssue.newBuilder());
  }

  /** Test printing of the DetectedIssue FHIR resource. */
  @Test
  public void printDetectedIssue() throws Exception {
    testPrint("detectedissue-example", DetectedIssue.newBuilder());
    testPrint("detectedissue-example-allergy", DetectedIssue.newBuilder());
    testPrint("detectedissue-example-dup", DetectedIssue.newBuilder());
    testPrint("detectedissue-example-lab", DetectedIssue.newBuilder());
  }

  /** Test parsing of the Device FHIR resource. */
  @Test
  public void parseDevice() throws Exception {
    testParse("device-example", Device.newBuilder());
    testParse("device-example-f001-feedingtube", Device.newBuilder());
    testParse("device-example-ihe-pcd", Device.newBuilder());
    testParse("device-example-pacemaker", Device.newBuilder());
    testParse("device-example-software", Device.newBuilder());
    testParse("device-example-udi1", Device.newBuilder());
    testParse("device-example-udi2", Device.newBuilder());
    testParse("device-example-udi3", Device.newBuilder());
    testParse("device-example-udi4", Device.newBuilder());
  }

  /** Test printing of the Device FHIR resource. */
  @Test
  public void printDevice() throws Exception {
    testPrint("device-example", Device.newBuilder());
    testPrint("device-example-f001-feedingtube", Device.newBuilder());
    testPrint("device-example-ihe-pcd", Device.newBuilder());
    testPrint("device-example-pacemaker", Device.newBuilder());
    testPrint("device-example-software", Device.newBuilder());
    testPrint("device-example-udi1", Device.newBuilder());
    testPrint("device-example-udi2", Device.newBuilder());
    testPrint("device-example-udi3", Device.newBuilder());
    testPrint("device-example-udi4", Device.newBuilder());
  }

  /** Test parsing of the DeviceComponent FHIR resource. */
  @Test
  public void parseDeviceComponent() throws Exception {
    testParse("devicecomponent-example", DeviceComponent.newBuilder());
    testParse("devicecomponent-example-prodspec", DeviceComponent.newBuilder());
  }

  /** Test printing of the DeviceComponent FHIR resource. */
  @Test
  public void printDeviceComponent() throws Exception {
    testPrint("devicecomponent-example", DeviceComponent.newBuilder());
    testPrint("devicecomponent-example-prodspec", DeviceComponent.newBuilder());
  }

  /** Test parsing of the DeviceMetric FHIR resource. */
  @Test
  public void parseDeviceMetric() throws Exception {
    testParse("devicemetric-example", DeviceMetric.newBuilder());
  }

  /** Test printing of the DeviceMetric FHIR resource. */
  @Test
  public void printDeviceMetric() throws Exception {
    testPrint("devicemetric-example", DeviceMetric.newBuilder());
  }

  /** Test parsing of the DeviceRequest FHIR resource. */
  @Test
  public void parseDeviceRequest() throws Exception {
    testParse("devicerequest-example", DeviceRequest.newBuilder());
    testParse("devicerequest-example-insulinpump", DeviceRequest.newBuilder());
  }

  /** Test printing of the DeviceRequest FHIR resource. */
  @Test
  public void printDeviceRequest() throws Exception {
    testPrint("devicerequest-example", DeviceRequest.newBuilder());
    testPrint("devicerequest-example-insulinpump", DeviceRequest.newBuilder());
  }

  /** Test parsing of the DeviceUseStatement FHIR resource. */
  @Test
  public void parseDeviceUseStatement() throws Exception {
    testParse("deviceusestatement-example", DeviceUseStatement.newBuilder());
  }

  /** Test printing of the DeviceUseStatement FHIR resource. */
  @Test
  public void printDeviceUseStatement() throws Exception {
    testPrint("deviceusestatement-example", DeviceUseStatement.newBuilder());
  }

  /** Test parsing of the DiagnosticReport FHIR resource. */
  @Test
  public void parseDiagnosticReport() throws Exception {
    testParse("diagnosticreport-example", DiagnosticReport.newBuilder());
    testParse("diagnosticreport-example-dxa", DiagnosticReport.newBuilder());
    testParse("diagnosticreport-example-f001-bloodexam", DiagnosticReport.newBuilder());
    testParse("diagnosticreport-example-f201-brainct", DiagnosticReport.newBuilder());
    testParse("diagnosticreport-example-f202-bloodculture", DiagnosticReport.newBuilder());
    testParse("diagnosticreport-example-ghp", DiagnosticReport.newBuilder());
    testParse("diagnosticreport-example-gingival-mass", DiagnosticReport.newBuilder());
    testParse("diagnosticreport-example-lipids", DiagnosticReport.newBuilder());
    testParse("diagnosticreport-example-papsmear", DiagnosticReport.newBuilder());
    testParse("diagnosticreport-example-pgx", DiagnosticReport.newBuilder());
    testParse("diagnosticreport-example-ultrasound", DiagnosticReport.newBuilder());
    testParse("diagnosticreport-genetics-example-2-familyhistory", DiagnosticReport.newBuilder());
  }

  /** Test printing of the DiagnosticReport FHIR resource. */
  @Test
  public void printDiagnosticReport() throws Exception {
    testPrint("diagnosticreport-example", DiagnosticReport.newBuilder());
    testPrint("diagnosticreport-example-dxa", DiagnosticReport.newBuilder());
    testPrint("diagnosticreport-example-f001-bloodexam", DiagnosticReport.newBuilder());
    testPrint("diagnosticreport-example-f201-brainct", DiagnosticReport.newBuilder());
    testPrint("diagnosticreport-example-f202-bloodculture", DiagnosticReport.newBuilder());
    testPrint("diagnosticreport-example-ghp", DiagnosticReport.newBuilder());
    testPrint("diagnosticreport-example-gingival-mass", DiagnosticReport.newBuilder());
    testPrint("diagnosticreport-example-lipids", DiagnosticReport.newBuilder());
    testPrint("diagnosticreport-example-papsmear", DiagnosticReport.newBuilder());
    testPrint("diagnosticreport-example-pgx", DiagnosticReport.newBuilder());
    testPrint("diagnosticreport-example-ultrasound", DiagnosticReport.newBuilder());
    testPrint("diagnosticreport-genetics-example-2-familyhistory", DiagnosticReport.newBuilder());
  }

  /** Test parsing of the DocumentManifest FHIR resource. */
  @Test
  public void parseDocumentManifest() throws Exception {
    testParse("documentmanifest-example", DocumentManifest.newBuilder());
  }

  /** Test printing of the DocumentManifest FHIR resource. */
  @Test
  public void printDocumentManifest() throws Exception {
    testPrint("documentmanifest-example", DocumentManifest.newBuilder());
  }

  /** Test parsing of the DocumentReference FHIR resource. */
  @Test
  public void parseDocumentReference() throws Exception {
    testParse("documentreference-example", DocumentReference.newBuilder());
  }

  /** Test printing of the DocumentReference FHIR resource. */
  @Test
  public void printDocumentReference() throws Exception {
    testPrint("documentreference-example", DocumentReference.newBuilder());
  }

  /** Test parsing of the EligibilityRequest FHIR resource. */
  @Test
  public void parseEligibilityRequest() throws Exception {
    testParse("eligibilityrequest-example", EligibilityRequest.newBuilder());
    testParse("eligibilityrequest-example-2", EligibilityRequest.newBuilder());
  }

  /** Test printing of the EligibilityRequest FHIR resource. */
  @Test
  public void printEligibilityRequest() throws Exception {
    testPrint("eligibilityrequest-example", EligibilityRequest.newBuilder());
    testPrint("eligibilityrequest-example-2", EligibilityRequest.newBuilder());
  }

  /** Test parsing of the EligibilityResponse FHIR resource. */
  @Test
  public void parseEligibilityResponse() throws Exception {
    testParse("eligibilityresponse-example", EligibilityResponse.newBuilder());
    testParse("eligibilityresponse-example-benefits", EligibilityResponse.newBuilder());
    testParse("eligibilityresponse-example-benefits-2", EligibilityResponse.newBuilder());
    testParse("eligibilityresponse-example-error", EligibilityResponse.newBuilder());
  }

  /** Test printing of the EligibilityResponse FHIR resource. */
  @Test
  public void printEligibilityResponse() throws Exception {
    testPrint("eligibilityresponse-example", EligibilityResponse.newBuilder());
    testPrint("eligibilityresponse-example-benefits", EligibilityResponse.newBuilder());
    testPrint("eligibilityresponse-example-benefits-2", EligibilityResponse.newBuilder());
    testPrint("eligibilityresponse-example-error", EligibilityResponse.newBuilder());
  }

  /** Test parsing of the Encounter FHIR resource. */
  @Test
  public void parseEncounter() throws Exception {
    testParse("encounter-example", Encounter.newBuilder());
    testParse("encounter-example-emerg", Encounter.newBuilder());
    testParse("encounter-example-f001-heart", Encounter.newBuilder());
    testParse("encounter-example-f002-lung", Encounter.newBuilder());
    testParse("encounter-example-f003-abscess", Encounter.newBuilder());
    testParse("encounter-example-f201-20130404", Encounter.newBuilder());
    testParse("encounter-example-f202-20130128", Encounter.newBuilder());
    testParse("encounter-example-f203-20130311", Encounter.newBuilder());
    testParse("encounter-example-home", Encounter.newBuilder());
    testParse("encounter-example-xcda", Encounter.newBuilder());
  }

  /** Test printing of the Encounter FHIR resource. */
  @Test
  public void printEncounter() throws Exception {
    testPrint("encounter-example", Encounter.newBuilder());
    testPrint("encounter-example-emerg", Encounter.newBuilder());
    testPrint("encounter-example-f001-heart", Encounter.newBuilder());
    testPrint("encounter-example-f002-lung", Encounter.newBuilder());
    testPrint("encounter-example-f003-abscess", Encounter.newBuilder());
    testPrint("encounter-example-f201-20130404", Encounter.newBuilder());
    testPrint("encounter-example-f202-20130128", Encounter.newBuilder());
    testPrint("encounter-example-f203-20130311", Encounter.newBuilder());
    testPrint("encounter-example-home", Encounter.newBuilder());
    testPrint("encounter-example-xcda", Encounter.newBuilder());
  }

  /** Test parsing of the Endpoint FHIR resource. */
  @Test
  public void parseEndpoint() throws Exception {
    testParse("endpoint-example", Endpoint.newBuilder());
    testParse("endpoint-example-iid", Endpoint.newBuilder());
    testParse("endpoint-example-wadors", Endpoint.newBuilder());
  }

  /** Test printing of the Endpoint FHIR resource. */
  @Test
  public void printEndpoint() throws Exception {
    testPrint("endpoint-example", Endpoint.newBuilder());
    testPrint("endpoint-example-iid", Endpoint.newBuilder());
    testPrint("endpoint-example-wadors", Endpoint.newBuilder());
  }

  /** Test parsing of the EnrollmentRequest FHIR resource. */
  @Test
  public void parseEnrollmentRequest() throws Exception {
    testParse("enrollmentrequest-example", EnrollmentRequest.newBuilder());
  }

  /** Test printing of the EnrollmentRequest FHIR resource. */
  @Test
  public void printEnrollmentRequest() throws Exception {
    testPrint("enrollmentrequest-example", EnrollmentRequest.newBuilder());
  }

  /** Test parsing of the EnrollmentResponse FHIR resource. */
  @Test
  public void parseEnrollmentResponse() throws Exception {
    testParse("enrollmentresponse-example", EnrollmentResponse.newBuilder());
  }

  /** Test printing of the EnrollmentResponse FHIR resource. */
  @Test
  public void printEnrollmentResponse() throws Exception {
    testPrint("enrollmentresponse-example", EnrollmentResponse.newBuilder());
  }

  /** Test parsing of the EpisodeOfCare FHIR resource. */
  @Test
  public void parseEpisodeOfCare() throws Exception {
    testParse("episodeofcare-example", EpisodeOfCare.newBuilder());
  }

  /** Test printing of the EpisodeOfCare FHIR resource. */
  @Test
  public void printEpisodeOfCare() throws Exception {
    testPrint("episodeofcare-example", EpisodeOfCare.newBuilder());
  }

  /** Test parsing of the ExpansionProfile FHIR resource. */
  @Test
  public void parseExpansionProfile() throws Exception {
    testParse("expansionprofile-example", ExpansionProfile.newBuilder());
  }

  /** Test printing of the ExpansionProfile FHIR resource. */
  @Test
  public void printExpansionProfile() throws Exception {
    testPrint("expansionprofile-example", ExpansionProfile.newBuilder());
  }

  /** Test parsing of the ExplanationOfBenefit FHIR resource. */
  @Test
  public void parseExplanationOfBenefit() throws Exception {
    testParse("explanationofbenefit-example", ExplanationOfBenefit.newBuilder());
  }

  /** Test printing of the ExplanationOfBenefit FHIR resource. */
  @Test
  public void printExplanationOfBenefit() throws Exception {
    testPrint("explanationofbenefit-example", ExplanationOfBenefit.newBuilder());
  }

  /** Test parsing of the FamilyMemberHistory FHIR resource. */
  @Test
  public void parseFamilyMemberHistory() throws Exception {
    testParse("familymemberhistory-example", FamilyMemberHistory.newBuilder());
    testParse("familymemberhistory-example-mother", FamilyMemberHistory.newBuilder());
  }

  /** Test printing of the FamilyMemberHistory FHIR resource. */
  @Test
  public void printFamilyMemberHistory() throws Exception {
    testPrint("familymemberhistory-example", FamilyMemberHistory.newBuilder());
    testPrint("familymemberhistory-example-mother", FamilyMemberHistory.newBuilder());
  }

  /** Test parsing of the Flag FHIR resource. */
  @Test
  public void parseFlag() throws Exception {
    testParse("flag-example", Flag.newBuilder());
    testParse("flag-example-encounter", Flag.newBuilder());
  }

  /** Test printing of the Flag FHIR resource. */
  @Test
  public void printFlag() throws Exception {
    testPrint("flag-example", Flag.newBuilder());
    testPrint("flag-example-encounter", Flag.newBuilder());
  }

  /** Test parsing of the Goal FHIR resource. */
  @Test
  public void parseGoal() throws Exception {
    testParse("goal-example", Goal.newBuilder());
    testParse("goal-example-stop-smoking", Goal.newBuilder());
  }

  /** Test printing of the Goal FHIR resource. */
  @Test
  public void printGoal() throws Exception {
    testPrint("goal-example", Goal.newBuilder());
    testPrint("goal-example-stop-smoking", Goal.newBuilder());
  }

  /** Test parsing of the GraphDefinition FHIR resource. */
  @Test
  public void parseGraphDefinition() throws Exception {
    testParse("graphdefinition-example", GraphDefinition.newBuilder());
  }

  /** Test printing of the GraphDefinition FHIR resource. */
  @Test
  public void printGraphDefinition() throws Exception {
    testPrint("graphdefinition-example", GraphDefinition.newBuilder());
  }

  /** Test parsing of the Group FHIR resource. */
  @Test
  public void parseGroup() throws Exception {
    testParse("group-example", Group.newBuilder());
    testParse("group-example-member", Group.newBuilder());
  }

  /** Test printing of the Group FHIR resource. */
  @Test
  public void printGroup() throws Exception {
    testPrint("group-example", Group.newBuilder());
    testPrint("group-example-member", Group.newBuilder());
  }

  /** Test parsing of the GuidanceResponse FHIR resource. */
  @Test
  public void parseGuidanceResponse() throws Exception {
    testParse("guidanceresponse-example", GuidanceResponse.newBuilder());
  }

  /** Test printing of the GuidanceResponse FHIR resource. */
  @Test
  public void printGuidanceResponse() throws Exception {
    testPrint("guidanceresponse-example", GuidanceResponse.newBuilder());
  }

  /** Test parsing of the HealthcareService FHIR resource. */
  @Test
  public void parseHealthcareService() throws Exception {
    testParse("healthcareservice-example", HealthcareService.newBuilder());
  }

  /** Test printing of the HealthcareService FHIR resource. */
  @Test
  public void printHealthcareService() throws Exception {
    testPrint("healthcareservice-example", HealthcareService.newBuilder());
  }

  /** Test parsing of the ImagingManifest FHIR resource. */
  @Test
  public void parseImagingManifest() throws Exception {
    testParse("imagingmanifest-example", ImagingManifest.newBuilder());
  }

  /** Test printing of the ImagingManifest FHIR resource. */
  @Test
  public void printImagingManifest() throws Exception {
    testPrint("imagingmanifest-example", ImagingManifest.newBuilder());
  }

  /** Test parsing of the ImagingStudy FHIR resource. */
  @Test
  public void parseImagingStudy() throws Exception {
    testParse("imagingstudy-example", ImagingStudy.newBuilder());
    testParse("imagingstudy-example-xr", ImagingStudy.newBuilder());
  }

  /** Test printing of the ImagingStudy FHIR resource. */
  @Test
  public void printImagingStudy() throws Exception {
    testPrint("imagingstudy-example", ImagingStudy.newBuilder());
    testPrint("imagingstudy-example-xr", ImagingStudy.newBuilder());
  }

  /** Test parsing of the Immunization FHIR resource. */
  @Test
  public void parseImmunization() throws Exception {
    testParse("immunization-example", Immunization.newBuilder());
    testParse("immunization-example-historical", Immunization.newBuilder());
    testParse("immunization-example-refused", Immunization.newBuilder());
  }

  /** Test printing of the Immunization FHIR resource. */
  @Test
  public void printImmunization() throws Exception {
    testPrint("immunization-example", Immunization.newBuilder());
    testPrint("immunization-example-historical", Immunization.newBuilder());
    testPrint("immunization-example-refused", Immunization.newBuilder());
  }

  /** Test parsing of the ImmunizationRecommendation FHIR resource. */
  @Test
  public void parseImmunizationRecommendation() throws Exception {
    testParse("immunizationrecommendation-example", ImmunizationRecommendation.newBuilder());
    testParse(
        "immunizationrecommendation-target-disease-example",
        ImmunizationRecommendation.newBuilder());
  }

  /** Test printing of the ImmunizationRecommendation FHIR resource. */
  @Test
  public void printImmunizationRecommendation() throws Exception {
    testPrint("immunizationrecommendation-example", ImmunizationRecommendation.newBuilder());
    testPrint(
        "immunizationrecommendation-target-disease-example",
        ImmunizationRecommendation.newBuilder());
  }

  /** Test parsing of the ImplementationGuide FHIR resource. */
  @Test
  public void parseImplementationGuide() throws Exception {
    testParse("implementationguide-example", ImplementationGuide.newBuilder());
  }

  /** Test printing of the ImplementationGuide FHIR resource. */
  @Test
  public void printImplementationGuide() throws Exception {
    testPrint("implementationguide-example", ImplementationGuide.newBuilder());
  }

  /** Test parsing of the Library FHIR resource. */
  @Test
  public void parseLibrary() throws Exception {
    testParse("library-cms146-example", Library.newBuilder());
    testParse("library-composition-example", Library.newBuilder());
    testParse("library-example", Library.newBuilder());
    testParse("library-predecessor-example", Library.newBuilder());
  }

  /** Test printing of the Library FHIR resource. */
  @Test
  public void printLibrary() throws Exception {
    testPrint("library-cms146-example", Library.newBuilder());
    testPrint("library-composition-example", Library.newBuilder());
    testPrint("library-example", Library.newBuilder());
    testPrint("library-predecessor-example", Library.newBuilder());
  }

  /** Test parsing of the Linkage FHIR resource. */
  @Test
  public void parseLinkage() throws Exception {
    testParse("linkage-example", Linkage.newBuilder());
  }

  /** Test printing of the Linkage FHIR resource. */
  @Test
  public void printLinkage() throws Exception {
    testPrint("linkage-example", Linkage.newBuilder());
  }

  /** Test parsing of the List FHIR resource. */
  @Test
  public void parseList() throws Exception {
    testParse("list-example", List.newBuilder());
    testParse("list-example-allergies", List.newBuilder());
    testParse("list-example-double-cousin-relationship-pedigree", List.newBuilder());
    testParse("list-example-empty", List.newBuilder());
    testParse("list-example-familyhistory-f201-roel", List.newBuilder());
    testParse("list-example-familyhistory-genetics-profile", List.newBuilder());
    testParse("list-example-familyhistory-genetics-profile-annie", List.newBuilder());
    testParse("list-example-medlist", List.newBuilder());
    testParse("list-example-simple-empty", List.newBuilder());
  }

  /** Test printing of the List FHIR resource. */
  @Test
  public void printList() throws Exception {
    testPrint("list-example", List.newBuilder());
    testPrint("list-example-allergies", List.newBuilder());
    testPrint("list-example-double-cousin-relationship-pedigree", List.newBuilder());
    testPrint("list-example-empty", List.newBuilder());
    testPrint("list-example-familyhistory-f201-roel", List.newBuilder());
    testPrint("list-example-familyhistory-genetics-profile", List.newBuilder());
    testPrint("list-example-familyhistory-genetics-profile-annie", List.newBuilder());
    testPrint("list-example-medlist", List.newBuilder());
    testPrint("list-example-simple-empty", List.newBuilder());
  }

  /** Test parsing of the Location FHIR resource. */
  @Test
  public void parseLocation() throws Exception {
    testParse("location-example", Location.newBuilder());
    testParse("location-example-ambulance", Location.newBuilder());
    testParse("location-example-hl7hq", Location.newBuilder());
    testParse("location-example-patients-home", Location.newBuilder());
    testParse("location-example-room", Location.newBuilder());
    testParse("location-example-ukpharmacy", Location.newBuilder());
  }

  /** Test printing of the Location FHIR resource. */
  @Test
  public void printLocation() throws Exception {
    testPrint("location-example", Location.newBuilder());
    testPrint("location-example-ambulance", Location.newBuilder());
    testPrint("location-example-hl7hq", Location.newBuilder());
    testPrint("location-example-patients-home", Location.newBuilder());
    testPrint("location-example-room", Location.newBuilder());
    testPrint("location-example-ukpharmacy", Location.newBuilder());
  }

  /** Test parsing of the Measure FHIR resource. */
  @Test
  public void parseMeasure() throws Exception {
    testParse("measure-cms146-example", Measure.newBuilder());
    testParse("measure-component-a-example", Measure.newBuilder());
    testParse("measure-component-b-example", Measure.newBuilder());
    testParse("measure-composite-example", Measure.newBuilder());
    testParse("measure-predecessor-example", Measure.newBuilder());
  }

  /** Test printing of the Measure FHIR resource. */
  @Test
  public void printMeasure() throws Exception {
    testPrint("measure-cms146-example", Measure.newBuilder());
    testPrint("measure-component-a-example", Measure.newBuilder());
    testPrint("measure-component-b-example", Measure.newBuilder());
    testPrint("measure-composite-example", Measure.newBuilder());
    testPrint("measure-predecessor-example", Measure.newBuilder());
  }

  /** Test parsing of the MeasureReport FHIR resource. */
  @Test
  public void parseMeasureReport() throws Exception {
    testParse("measurereport-cms146-cat1-example", MeasureReport.newBuilder());
    testParse("measurereport-cms146-cat2-example", MeasureReport.newBuilder());
    testParse("measurereport-cms146-cat3-example", MeasureReport.newBuilder());
  }

  /** Test printing of the MeasureReport FHIR resource. */
  @Test
  public void printMeasureReport() throws Exception {
    testPrint("measurereport-cms146-cat1-example", MeasureReport.newBuilder());
    testPrint("measurereport-cms146-cat2-example", MeasureReport.newBuilder());
    testPrint("measurereport-cms146-cat3-example", MeasureReport.newBuilder());
  }

  /** Test parsing of the Media FHIR resource. */
  @Test
  public void parseMedia() throws Exception {
    testParse("media-example", Media.newBuilder());
    testParse("media-example-dicom", Media.newBuilder());
    testParse("media-example-sound", Media.newBuilder());
    testParse("media-example-xray", Media.newBuilder());
  }

  /** Test printing of the Media FHIR resource. */
  @Test
  public void printMedia() throws Exception {
    testPrint("media-example", Media.newBuilder());
    testPrint("media-example-dicom", Media.newBuilder());
    testPrint("media-example-sound", Media.newBuilder());
    testPrint("media-example-xray", Media.newBuilder());
  }

  /** Test parsing of the Medication FHIR resource. */
  @Test
  public void parseMedication() throws Exception {
    testParse("medicationexample0301", Medication.newBuilder());
    testParse("medicationexample0302", Medication.newBuilder());
    testParse("medicationexample0303", Medication.newBuilder());
    testParse("medicationexample0304", Medication.newBuilder());
    testParse("medicationexample0305", Medication.newBuilder());
    testParse("medicationexample0306", Medication.newBuilder());
    testParse("medicationexample0307", Medication.newBuilder());
    testParse("medicationexample0308", Medication.newBuilder());
    testParse("medicationexample0309", Medication.newBuilder());
    testParse("medicationexample0310", Medication.newBuilder());
    testParse("medicationexample0311", Medication.newBuilder());
    testParse("medicationexample0312", Medication.newBuilder());
    testParse("medicationexample0313", Medication.newBuilder());
    testParse("medicationexample0314", Medication.newBuilder());
    testParse("medicationexample0315", Medication.newBuilder());
    testParse("medicationexample0316", Medication.newBuilder());
    testParse("medicationexample0317", Medication.newBuilder());
    testParse("medicationexample0318", Medication.newBuilder());
    testParse("medicationexample0319", Medication.newBuilder());
    testParse("medicationexample0320", Medication.newBuilder());
    testParse("medicationexample0321", Medication.newBuilder());
    testParse("medicationexample1", Medication.newBuilder());
    testParse("medicationexample15", Medication.newBuilder());
  }

  /** Test printing of the Medication FHIR resource. */
  @Test
  public void printMedication() throws Exception {
    testPrint("medicationexample0301", Medication.newBuilder());
    testPrint("medicationexample0302", Medication.newBuilder());
    testPrint("medicationexample0303", Medication.newBuilder());
    testPrint("medicationexample0304", Medication.newBuilder());
    testPrint("medicationexample0305", Medication.newBuilder());
    testPrint("medicationexample0306", Medication.newBuilder());
    testPrint("medicationexample0307", Medication.newBuilder());
    testPrint("medicationexample0308", Medication.newBuilder());
    testPrint("medicationexample0309", Medication.newBuilder());
    testPrint("medicationexample0310", Medication.newBuilder());
    testPrint("medicationexample0311", Medication.newBuilder());
    testPrint("medicationexample0312", Medication.newBuilder());
    testPrint("medicationexample0313", Medication.newBuilder());
    testPrint("medicationexample0314", Medication.newBuilder());
    testPrint("medicationexample0315", Medication.newBuilder());
    testPrint("medicationexample0316", Medication.newBuilder());
    testPrint("medicationexample0317", Medication.newBuilder());
    testPrint("medicationexample0318", Medication.newBuilder());
    testPrint("medicationexample0319", Medication.newBuilder());
    testPrint("medicationexample0320", Medication.newBuilder());
    testPrint("medicationexample0321", Medication.newBuilder());
    testPrint("medicationexample1", Medication.newBuilder());
    testPrint("medicationexample15", Medication.newBuilder());
  }

  /** Test parsing of the MedicationAdministration FHIR resource. */
  @Test
  public void parseMedicationAdministration() throws Exception {
    testParse("medicationadministrationexample3", MedicationAdministration.newBuilder());
  }

  /** Test printing of the MedicationAdministration FHIR resource. */
  @Test
  public void printMedicationAdministration() throws Exception {
    testPrint("medicationadministrationexample3", MedicationAdministration.newBuilder());
  }

  /** Test parsing of the MedicationDispense FHIR resource. */
  @Test
  public void parseMedicationDispense() throws Exception {
    testParse("medicationdispenseexample8", MedicationDispense.newBuilder());
  }

  /** Test printing of the MedicationDispense FHIR resource. */
  @Test
  public void printMedicationDispense() throws Exception {
    testPrint("medicationdispenseexample8", MedicationDispense.newBuilder());
  }

  /** Test parsing of the MedicationRequest FHIR resource. */
  @Test
  public void parseMedicationRequest() throws Exception {
    testParse("medicationrequestexample1", MedicationRequest.newBuilder());
    testParse("medicationrequestexample2", MedicationRequest.newBuilder());
  }

  /** Test printing of the MedicationRequest FHIR resource. */
  @Test
  public void printMedicationRequest() throws Exception {
    testPrint("medicationrequestexample1", MedicationRequest.newBuilder());
    testPrint("medicationrequestexample2", MedicationRequest.newBuilder());
  }

  /** Test parsing of the MedicationStatement FHIR resource. */
  @Test
  public void parseMedicationStatement() throws Exception {
    testParse("medicationstatementexample1", MedicationStatement.newBuilder());
    testParse("medicationstatementexample2", MedicationStatement.newBuilder());
    testParse("medicationstatementexample3", MedicationStatement.newBuilder());
    testParse("medicationstatementexample4", MedicationStatement.newBuilder());
    testParse("medicationstatementexample5", MedicationStatement.newBuilder());
    testParse("medicationstatementexample6", MedicationStatement.newBuilder());
    testParse("medicationstatementexample7", MedicationStatement.newBuilder());
  }

  /** Test printing of the MedicationStatement FHIR resource. */
  @Test
  public void printMedicationStatement() throws Exception {
    testPrint("medicationstatementexample1", MedicationStatement.newBuilder());
    testPrint("medicationstatementexample2", MedicationStatement.newBuilder());
    testPrint("medicationstatementexample3", MedicationStatement.newBuilder());
    testPrint("medicationstatementexample4", MedicationStatement.newBuilder());
    testPrint("medicationstatementexample5", MedicationStatement.newBuilder());
    testPrint("medicationstatementexample6", MedicationStatement.newBuilder());
    testPrint("medicationstatementexample7", MedicationStatement.newBuilder());
  }

  /** Test parsing of the MessageDefinition FHIR resource. */
  @Test
  public void parseMessageDefinition() throws Exception {
    testParse("messagedefinition-example", MessageDefinition.newBuilder());
  }

  /** Test printing of the MessageDefinition FHIR resource. */
  @Test
  public void printMessageDefinition() throws Exception {
    testPrint("messagedefinition-example", MessageDefinition.newBuilder());
  }

  /** Test parsing of the MessageHeader FHIR resource. */
  @Test
  public void parseMessageHeader() throws Exception {
    testParse("messageheader-example", MessageHeader.newBuilder());
  }

  /** Test printing of the MessageHeader FHIR resource. */
  @Test
  public void printMessageHeader() throws Exception {
    testPrint("messageheader-example", MessageHeader.newBuilder());
  }

  /** Test parsing of the NamingSystem FHIR resource. */
  @Test
  public void parseNamingSystem() throws Exception {
    testParse("namingsystem-example", NamingSystem.newBuilder());
    testParse("namingsystem-example-id", NamingSystem.newBuilder());
    testParse("namingsystem-example-replaced", NamingSystem.newBuilder());
  }

  /** Test printing of the NamingSystem FHIR resource. */
  @Test
  public void printNamingSystem() throws Exception {
    testPrint("namingsystem-example", NamingSystem.newBuilder());
    testPrint("namingsystem-example-id", NamingSystem.newBuilder());
    testPrint("namingsystem-example-replaced", NamingSystem.newBuilder());
  }

  /** Test parsing of the NutritionOrder FHIR resource. */
  @Test
  public void parseNutritionOrder() throws Exception {
    testParse("nutritionorder-example-cardiacdiet", NutritionOrder.newBuilder());
    testParse("nutritionorder-example-diabeticdiet", NutritionOrder.newBuilder());
    testParse("nutritionorder-example-diabeticsupplement", NutritionOrder.newBuilder());
    testParse("nutritionorder-example-energysupplement", NutritionOrder.newBuilder());
    testParse("nutritionorder-example-enteralbolus", NutritionOrder.newBuilder());
    testParse("nutritionorder-example-enteralcontinuous", NutritionOrder.newBuilder());
    testParse("nutritionorder-example-fiberrestricteddiet", NutritionOrder.newBuilder());
    testParse("nutritionorder-example-infantenteral", NutritionOrder.newBuilder());
    testParse("nutritionorder-example-proteinsupplement", NutritionOrder.newBuilder());
    testParse("nutritionorder-example-pureeddiet", NutritionOrder.newBuilder());
    testParse("nutritionorder-example-pureeddiet-simple", NutritionOrder.newBuilder());
    testParse("nutritionorder-example-renaldiet", NutritionOrder.newBuilder());
    testParse("nutritionorder-example-texture-modified", NutritionOrder.newBuilder());
  }

  /** Test printing of the NutritionOrder FHIR resource. */
  @Test
  public void printNutritionOrder() throws Exception {
    testPrint("nutritionorder-example-cardiacdiet", NutritionOrder.newBuilder());
    testPrint("nutritionorder-example-diabeticdiet", NutritionOrder.newBuilder());
    testPrint("nutritionorder-example-diabeticsupplement", NutritionOrder.newBuilder());
    testPrint("nutritionorder-example-energysupplement", NutritionOrder.newBuilder());
    testPrint("nutritionorder-example-enteralbolus", NutritionOrder.newBuilder());
    testPrint("nutritionorder-example-enteralcontinuous", NutritionOrder.newBuilder());
    testPrint("nutritionorder-example-fiberrestricteddiet", NutritionOrder.newBuilder());
    testPrint("nutritionorder-example-infantenteral", NutritionOrder.newBuilder());
    testPrint("nutritionorder-example-proteinsupplement", NutritionOrder.newBuilder());
    testPrint("nutritionorder-example-pureeddiet", NutritionOrder.newBuilder());
    testPrint("nutritionorder-example-pureeddiet-simple", NutritionOrder.newBuilder());
    testPrint("nutritionorder-example-renaldiet", NutritionOrder.newBuilder());
    testPrint("nutritionorder-example-texture-modified", NutritionOrder.newBuilder());
  }

  /** Test parsing of the Observation FHIR resource. */
  @Test
  public void parseObservation() throws Exception {
    testParse("observation-example", Observation.newBuilder());
    testParse("observation-example-10minute-apgar-score", Observation.newBuilder());
    testParse("observation-example-1minute-apgar-score", Observation.newBuilder());
    testParse("observation-example-20minute-apgar-score", Observation.newBuilder());
    testParse("observation-example-2minute-apgar-score", Observation.newBuilder());
    testParse("observation-example-5minute-apgar-score", Observation.newBuilder());
    testParse("observation-example-bloodpressure", Observation.newBuilder());
    testParse("observation-example-bloodpressure-cancel", Observation.newBuilder());
    testParse("observation-example-bloodpressure-dar", Observation.newBuilder());
    testParse("observation-example-bmd", Observation.newBuilder());
    testParse("observation-example-bmi", Observation.newBuilder());
    testParse("observation-example-body-height", Observation.newBuilder());
    testParse("observation-example-body-length", Observation.newBuilder());
    testParse("observation-example-body-temperature", Observation.newBuilder());
    testParse("observation-example-date-lastmp", Observation.newBuilder());
    testParse("observation-example-diplotype1", Observation.newBuilder());
    testParse("observation-example-eye-color", Observation.newBuilder());
    testParse("observation-example-f001-glucose", Observation.newBuilder());
    testParse("observation-example-f002-excess", Observation.newBuilder());
    testParse("observation-example-f003-co2", Observation.newBuilder());
    testParse("observation-example-f004-erythrocyte", Observation.newBuilder());
    testParse("observation-example-f005-hemoglobin", Observation.newBuilder());
    testParse("observation-example-f202-temperature", Observation.newBuilder());
    testParse("observation-example-f203-bicarbonate", Observation.newBuilder());
    testParse("observation-example-f204-creatinine", Observation.newBuilder());
    testParse("observation-example-f205-egfr", Observation.newBuilder());
    testParse("observation-example-f206-staphylococcus", Observation.newBuilder());
    testParse("observation-example-genetics-1", Observation.newBuilder());
    testParse("observation-example-genetics-2", Observation.newBuilder());
    testParse("observation-example-genetics-3", Observation.newBuilder());
    testParse("observation-example-genetics-4", Observation.newBuilder());
    testParse("observation-example-genetics-5", Observation.newBuilder());
    testParse("observation-example-glasgow", Observation.newBuilder());
    testParse("observation-example-glasgow-qa", Observation.newBuilder());
    testParse("observation-example-haplotype1", Observation.newBuilder());
    testParse("observation-example-haplotype2", Observation.newBuilder());
    testParse("observation-example-head-circumference", Observation.newBuilder());
    testParse("observation-example-heart-rate", Observation.newBuilder());
    testParse("observation-example-mbp", Observation.newBuilder());
    testParse("observation-example-phenotype", Observation.newBuilder());
    testParse("observation-example-respiratory-rate", Observation.newBuilder());
    testParse("observation-example-sample-data", Observation.newBuilder());
    testParse("observation-example-satO2", Observation.newBuilder());
    testParse("observation-example-TPMT-diplotype", Observation.newBuilder());
    testParse("observation-example-TPMT-haplotype-one", Observation.newBuilder());
    testParse("observation-example-TPMT-haplotype-two", Observation.newBuilder());
    testParse("observation-example-unsat", Observation.newBuilder());
    testParse("observation-example-vitals-panel", Observation.newBuilder());
  }

  /** Test printing of the Observation FHIR resource. */
  @Test
  public void printObservation() throws Exception {
    testPrint("observation-example", Observation.newBuilder());
    testPrint("observation-example-10minute-apgar-score", Observation.newBuilder());
    testPrint("observation-example-1minute-apgar-score", Observation.newBuilder());
    testPrint("observation-example-20minute-apgar-score", Observation.newBuilder());
    testPrint("observation-example-2minute-apgar-score", Observation.newBuilder());
    testPrint("observation-example-5minute-apgar-score", Observation.newBuilder());
    testPrint("observation-example-bloodpressure", Observation.newBuilder());
    testPrint("observation-example-bloodpressure-cancel", Observation.newBuilder());
    testPrint("observation-example-bloodpressure-dar", Observation.newBuilder());
    testPrint("observation-example-bmd", Observation.newBuilder());
    testPrint("observation-example-bmi", Observation.newBuilder());
    testPrint("observation-example-body-height", Observation.newBuilder());
    testPrint("observation-example-body-length", Observation.newBuilder());
    testPrint("observation-example-body-temperature", Observation.newBuilder());
    testPrint("observation-example-date-lastmp", Observation.newBuilder());
    testPrint("observation-example-diplotype1", Observation.newBuilder());
    testPrint("observation-example-eye-color", Observation.newBuilder());
    testPrint("observation-example-f001-glucose", Observation.newBuilder());
    testPrint("observation-example-f002-excess", Observation.newBuilder());
    testPrint("observation-example-f003-co2", Observation.newBuilder());
    testPrint("observation-example-f004-erythrocyte", Observation.newBuilder());
    testPrint("observation-example-f005-hemoglobin", Observation.newBuilder());
    testPrint("observation-example-f202-temperature", Observation.newBuilder());
    testPrint("observation-example-f203-bicarbonate", Observation.newBuilder());
    testPrint("observation-example-f204-creatinine", Observation.newBuilder());
    testPrint("observation-example-f205-egfr", Observation.newBuilder());
    testPrint("observation-example-f206-staphylococcus", Observation.newBuilder());
    testPrint("observation-example-genetics-1", Observation.newBuilder());
    testPrint("observation-example-genetics-2", Observation.newBuilder());
    testPrint("observation-example-genetics-3", Observation.newBuilder());
    testPrint("observation-example-genetics-4", Observation.newBuilder());
    testPrint("observation-example-genetics-5", Observation.newBuilder());
    testPrint("observation-example-glasgow", Observation.newBuilder());
    testPrint("observation-example-glasgow-qa", Observation.newBuilder());
    testPrint("observation-example-haplotype1", Observation.newBuilder());
    testPrint("observation-example-haplotype2", Observation.newBuilder());
    testPrint("observation-example-head-circumference", Observation.newBuilder());
    testPrint("observation-example-heart-rate", Observation.newBuilder());
    testPrint("observation-example-mbp", Observation.newBuilder());
    testPrint("observation-example-phenotype", Observation.newBuilder());
    testPrint("observation-example-respiratory-rate", Observation.newBuilder());
    testPrint("observation-example-sample-data", Observation.newBuilder());
    testPrint("observation-example-satO2", Observation.newBuilder());
    testPrint("observation-example-TPMT-diplotype", Observation.newBuilder());
    testPrint("observation-example-TPMT-haplotype-one", Observation.newBuilder());
    testPrint("observation-example-TPMT-haplotype-two", Observation.newBuilder());
    testPrint("observation-example-unsat", Observation.newBuilder());
    testPrint("observation-example-vitals-panel", Observation.newBuilder());
  }

  /** Test parsing of the OperationDefinition FHIR resource. */
  @Test
  public void parseOperationDefinition() throws Exception {
    testParse("operationdefinition-example", OperationDefinition.newBuilder());
  }

  /** Test printing of the OperationDefinition FHIR resource. */
  @Test
  public void printOperationDefinition() throws Exception {
    testPrint("operationdefinition-example", OperationDefinition.newBuilder());
  }

  /** Test parsing of the OperationOutcome FHIR resource. */
  @Test
  public void parseOperationOutcome() throws Exception {
    testParse("operationoutcome-example", OperationOutcome.newBuilder());
    testParse("operationoutcome-example-allok", OperationOutcome.newBuilder());
    testParse("operationoutcome-example-break-the-glass", OperationOutcome.newBuilder());
    testParse("operationoutcome-example-exception", OperationOutcome.newBuilder());
    testParse("operationoutcome-example-searchfail", OperationOutcome.newBuilder());
    testParse("operationoutcome-example-validationfail", OperationOutcome.newBuilder());
  }

  /** Test printing of the OperationOutcome FHIR resource. */
  @Test
  public void printOperationOutcome() throws Exception {
    testPrint("operationoutcome-example", OperationOutcome.newBuilder());
    testPrint("operationoutcome-example-allok", OperationOutcome.newBuilder());
    testPrint("operationoutcome-example-break-the-glass", OperationOutcome.newBuilder());
    testPrint("operationoutcome-example-exception", OperationOutcome.newBuilder());
    testPrint("operationoutcome-example-searchfail", OperationOutcome.newBuilder());
    testPrint("operationoutcome-example-validationfail", OperationOutcome.newBuilder());
  }

  /** Test parsing of the Organization FHIR resource. */
  @Test
  public void parseOrganization() throws Exception {
    testParse("organization-example", Organization.newBuilder());
    testParse("organization-example-f001-burgers", Organization.newBuilder());
    testParse("organization-example-f002-burgers-card", Organization.newBuilder());
    testParse("organization-example-f003-burgers-ENT", Organization.newBuilder());
    testParse("organization-example-f201-aumc", Organization.newBuilder());
    testParse("organization-example-f203-bumc", Organization.newBuilder());
    testParse("organization-example-gastro", Organization.newBuilder());
    testParse("organization-example-good-health-care", Organization.newBuilder());
    testParse("organization-example-insurer", Organization.newBuilder());
    testParse("organization-example-lab", Organization.newBuilder());
    testParse("organization-example-mmanu", Organization.newBuilder());
  }

  /** Test printing of the Organization FHIR resource. */
  @Test
  public void printOrganization() throws Exception {
    testPrint("organization-example", Organization.newBuilder());
    testPrint("organization-example-f001-burgers", Organization.newBuilder());
    testPrint("organization-example-f002-burgers-card", Organization.newBuilder());
    testPrint("organization-example-f003-burgers-ENT", Organization.newBuilder());
    testPrint("organization-example-f201-aumc", Organization.newBuilder());
    testPrint("organization-example-f203-bumc", Organization.newBuilder());
    testPrint("organization-example-gastro", Organization.newBuilder());
    testPrint("organization-example-good-health-care", Organization.newBuilder());
    testPrint("organization-example-insurer", Organization.newBuilder());
    testPrint("organization-example-lab", Organization.newBuilder());
    testPrint("organization-example-mmanu", Organization.newBuilder());
  }

  /** Test parsing of the Parameters FHIR resource. */
  @Test
  public void parseParameters() throws Exception {
    testParse("parameters-example", Parameters.newBuilder());
  }

  /** Test printing of the Parameters FHIR resource. */
  @Test
  public void printParameters() throws Exception {
    testPrint("parameters-example", Parameters.newBuilder());
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
    testParse("paymentnotice-example", PaymentNotice.newBuilder());
  }

  /** Test printing of the PaymentNotice FHIR resource. */
  @Test
  public void printPaymentNotice() throws Exception {
    testPrint("paymentnotice-example", PaymentNotice.newBuilder());
  }

  /** Test parsing of the PaymentReconciliation FHIR resource. */
  @Test
  public void parsePaymentReconciliation() throws Exception {
    testParse("paymentreconciliation-example", PaymentReconciliation.newBuilder());
  }

  /** Test printing of the PaymentReconciliation FHIR resource. */
  @Test
  public void printPaymentReconciliation() throws Exception {
    testPrint("paymentreconciliation-example", PaymentReconciliation.newBuilder());
  }

  /** Test parsing of the Person FHIR resource. */
  @Test
  public void parsePerson() throws Exception {
    testParse("person-example", Person.newBuilder());
    testParse("person-example-f002-ariadne", Person.newBuilder());
  }

  /** Test printing of the Person FHIR resource. */
  @Test
  public void printPerson() throws Exception {
    testPrint("person-example", Person.newBuilder());
    testPrint("person-example-f002-ariadne", Person.newBuilder());
  }

  /** Test parsing of the PlanDefinition FHIR resource. */
  @Test
  public void parsePlanDefinition() throws Exception {
    testParse("plandefinition-example", PlanDefinition.newBuilder());
    testParse("plandefinition-example-kdn5-simplified", PlanDefinition.newBuilder());
    testParse("plandefinition-options-example", PlanDefinition.newBuilder());
    testParse("plandefinition-predecessor-example", PlanDefinition.newBuilder());
    testParse("plandefinition-protocol-example", PlanDefinition.newBuilder());
  }

  /** Test printing of the PlanDefinition FHIR resource. */
  @Test
  public void printPlanDefinition() throws Exception {
    testPrint("plandefinition-example", PlanDefinition.newBuilder());
    testPrint("plandefinition-example-kdn5-simplified", PlanDefinition.newBuilder());
    testPrint("plandefinition-options-example", PlanDefinition.newBuilder());
    testPrint("plandefinition-predecessor-example", PlanDefinition.newBuilder());
    testPrint("plandefinition-protocol-example", PlanDefinition.newBuilder());
  }

  /** Test parsing of the Practitioner FHIR resource. */
  @Test
  public void parsePractitioner() throws Exception {
    testParse("practitioner-example", Practitioner.newBuilder());
    testParse("practitioner-example-f001-evdb", Practitioner.newBuilder());
    testParse("practitioner-example-f002-pv", Practitioner.newBuilder());
    testParse("practitioner-example-f003-mv", Practitioner.newBuilder());
    testParse("practitioner-example-f004-rb", Practitioner.newBuilder());
    testParse("practitioner-example-f005-al", Practitioner.newBuilder());
    testParse("practitioner-example-f006-rvdb", Practitioner.newBuilder());
    testParse("practitioner-example-f007-sh", Practitioner.newBuilder());
    testParse("practitioner-example-f201-ab", Practitioner.newBuilder());
    testParse("practitioner-example-f202-lm", Practitioner.newBuilder());
    testParse("practitioner-example-f203-jvg", Practitioner.newBuilder());
    testParse("practitioner-example-f204-ce", Practitioner.newBuilder());
    testParse("practitioner-example-xcda1", Practitioner.newBuilder());
    testParse("practitioner-example-xcda-author", Practitioner.newBuilder());
  }

  /** Test printing of the Practitioner FHIR resource. */
  @Test
  public void printPractitioner() throws Exception {
    testPrint("practitioner-example", Practitioner.newBuilder());
    testPrint("practitioner-example-f001-evdb", Practitioner.newBuilder());
    testPrint("practitioner-example-f002-pv", Practitioner.newBuilder());
    testPrint("practitioner-example-f003-mv", Practitioner.newBuilder());
    testPrint("practitioner-example-f004-rb", Practitioner.newBuilder());
    testPrint("practitioner-example-f005-al", Practitioner.newBuilder());
    testPrint("practitioner-example-f006-rvdb", Practitioner.newBuilder());
    testPrint("practitioner-example-f007-sh", Practitioner.newBuilder());
    testPrint("practitioner-example-f201-ab", Practitioner.newBuilder());
    testPrint("practitioner-example-f202-lm", Practitioner.newBuilder());
    testPrint("practitioner-example-f203-jvg", Practitioner.newBuilder());
    testPrint("practitioner-example-f204-ce", Practitioner.newBuilder());
    testPrint("practitioner-example-xcda1", Practitioner.newBuilder());
    testPrint("practitioner-example-xcda-author", Practitioner.newBuilder());
  }

  /** Test parsing of the PractitionerRole FHIR resource. */
  @Test
  public void parsePractitionerRole() throws Exception {
    testParse("practitionerrole-example", PractitionerRole.newBuilder());
  }

  /** Test printing of the PractitionerRole FHIR resource. */
  @Test
  public void printPractitionerRole() throws Exception {
    testPrint("practitionerrole-example", PractitionerRole.newBuilder());
  }

  /** Test parsing of the Procedure FHIR resource. */
  @Test
  public void parseProcedure() throws Exception {
    testParse("procedure-example", Procedure.newBuilder());
    testParse("procedure-example-ambulation", Procedure.newBuilder());
    testParse("procedure-example-appendectomy-narrative", Procedure.newBuilder());
    testParse("procedure-example-biopsy", Procedure.newBuilder());
    testParse("procedure-example-colon-biopsy", Procedure.newBuilder());
    testParse("procedure-example-colonoscopy", Procedure.newBuilder());
    testParse("procedure-example-education", Procedure.newBuilder());
    testParse("procedure-example-f001-heart", Procedure.newBuilder());
    testParse("procedure-example-f002-lung", Procedure.newBuilder());
    testParse("procedure-example-f003-abscess", Procedure.newBuilder());
    testParse("procedure-example-f004-tracheotomy", Procedure.newBuilder());
    testParse("procedure-example-f201-tpf", Procedure.newBuilder());
    testParse("procedure-example-implant", Procedure.newBuilder());
    testParse("procedure-example-ob", Procedure.newBuilder());
    testParse("procedure-example-physical-therapy", Procedure.newBuilder());
  }

  /** Test printing of the Procedure FHIR resource. */
  @Test
  public void printProcedure() throws Exception {
    testPrint("procedure-example", Procedure.newBuilder());
    testPrint("procedure-example-ambulation", Procedure.newBuilder());
    testPrint("procedure-example-appendectomy-narrative", Procedure.newBuilder());
    testPrint("procedure-example-biopsy", Procedure.newBuilder());
    testPrint("procedure-example-colon-biopsy", Procedure.newBuilder());
    testPrint("procedure-example-colonoscopy", Procedure.newBuilder());
    testPrint("procedure-example-education", Procedure.newBuilder());
    testPrint("procedure-example-f001-heart", Procedure.newBuilder());
    testPrint("procedure-example-f002-lung", Procedure.newBuilder());
    testPrint("procedure-example-f003-abscess", Procedure.newBuilder());
    testPrint("procedure-example-f004-tracheotomy", Procedure.newBuilder());
    testPrint("procedure-example-f201-tpf", Procedure.newBuilder());
    testPrint("procedure-example-implant", Procedure.newBuilder());
    testPrint("procedure-example-ob", Procedure.newBuilder());
    testPrint("procedure-example-physical-therapy", Procedure.newBuilder());
  }

  /** Test parsing of the ProcedureRequest FHIR resource. */
  @Test
  public void parseProcedureRequest() throws Exception {
    testParse("procedurerequest-example", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example2", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example3", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example4", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example-ambulation", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example-appendectomy", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example-colonoscopy", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example-colonoscopy-bx", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example-di", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example-edu", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example-ft4", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example-implant", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example-lipid", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example-ob", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example-pgx", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example-pt", ProcedureRequest.newBuilder());
    testParse("procedurerequest-example-subrequest", ProcedureRequest.newBuilder());
    testParse("procedurerequest-genetics-example-1", ProcedureRequest.newBuilder());
  }

  /** Test printing of the ProcedureRequest FHIR resource. */
  @Test
  public void printProcedureRequest() throws Exception {
    testPrint("procedurerequest-example", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example2", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example3", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example4", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example-ambulation", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example-appendectomy", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example-colonoscopy", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example-colonoscopy-bx", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example-di", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example-edu", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example-ft4", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example-implant", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example-lipid", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example-ob", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example-pgx", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example-pt", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-example-subrequest", ProcedureRequest.newBuilder());
    testPrint("procedurerequest-genetics-example-1", ProcedureRequest.newBuilder());
  }

  /** Test parsing of the ProcessRequest FHIR resource. */
  @Test
  public void parseProcessRequest() throws Exception {
    testParse("processrequest-example", ProcessRequest.newBuilder());
    testParse("processrequest-example-poll-eob", ProcessRequest.newBuilder());
    testParse("processrequest-example-poll-exclusive", ProcessRequest.newBuilder());
    testParse("processrequest-example-poll-inclusive", ProcessRequest.newBuilder());
    testParse("processrequest-example-poll-payrec", ProcessRequest.newBuilder());
    testParse("processrequest-example-poll-specific", ProcessRequest.newBuilder());
    testParse("processrequest-example-reprocess", ProcessRequest.newBuilder());
    testParse("processrequest-example-reverse", ProcessRequest.newBuilder());
    testParse("processrequest-example-status", ProcessRequest.newBuilder());
  }

  /** Test printing of the ProcessRequest FHIR resource. */
  @Test
  public void printProcessRequest() throws Exception {
    testPrint("processrequest-example", ProcessRequest.newBuilder());
    testPrint("processrequest-example-poll-eob", ProcessRequest.newBuilder());
    testPrint("processrequest-example-poll-exclusive", ProcessRequest.newBuilder());
    testPrint("processrequest-example-poll-inclusive", ProcessRequest.newBuilder());
    testPrint("processrequest-example-poll-payrec", ProcessRequest.newBuilder());
    testPrint("processrequest-example-poll-specific", ProcessRequest.newBuilder());
    testPrint("processrequest-example-reprocess", ProcessRequest.newBuilder());
    testPrint("processrequest-example-reverse", ProcessRequest.newBuilder());
    testPrint("processrequest-example-status", ProcessRequest.newBuilder());
  }

  /** Test parsing of the ProcessResponse FHIR resource. */
  @Test
  public void parseProcessResponse() throws Exception {
    testParse("processresponse-example", ProcessResponse.newBuilder());
    testParse("processresponse-example-error", ProcessResponse.newBuilder());
    testParse("processresponse-example-pended", ProcessResponse.newBuilder());
  }

  /** Test printing of the ProcessResponse FHIR resource. */
  @Test
  public void printProcessResponse() throws Exception {
    testPrint("processresponse-example", ProcessResponse.newBuilder());
    testPrint("processresponse-example-error", ProcessResponse.newBuilder());
    testPrint("processresponse-example-pended", ProcessResponse.newBuilder());
  }

  /** Test parsing of the Provenance FHIR resource. */
  @Test
  public void parseProvenance() throws Exception {
    testParse("provenance-example", Provenance.newBuilder());
    testParse("provenance-example-biocompute-object", Provenance.newBuilder());
    testParse("provenance-example-cwl", Provenance.newBuilder());
    testParse("provenance-example-sig", Provenance.newBuilder());
  }

  /** Test printing of the Provenance FHIR resource. */
  @Test
  public void printProvenance() throws Exception {
    testPrint("provenance-example", Provenance.newBuilder());
    testPrint("provenance-example-biocompute-object", Provenance.newBuilder());
    testPrint("provenance-example-cwl", Provenance.newBuilder());
    testPrint("provenance-example-sig", Provenance.newBuilder());
  }

  /** Test parsing of the Questionnaire FHIR resource. */
  @Test
  public void parseQuestionnaire() throws Exception {
    testParse("questionnaire-example", Questionnaire.newBuilder());
    testParse("questionnaire-example-bluebook", Questionnaire.newBuilder());
    testParse("questionnaire-example-f201-lifelines", Questionnaire.newBuilder());
    testParse("questionnaire-example-gcs", Questionnaire.newBuilder());
  }

  /** Test printing of the Questionnaire FHIR resource. */
  @Test
  public void printQuestionnaire() throws Exception {
    testPrint("questionnaire-example", Questionnaire.newBuilder());
    testPrint("questionnaire-example-bluebook", Questionnaire.newBuilder());
    testPrint("questionnaire-example-f201-lifelines", Questionnaire.newBuilder());
    testPrint("questionnaire-example-gcs", Questionnaire.newBuilder());
  }

  /** Test parsing of the QuestionnaireResponse FHIR resource. */
  @Test
  public void parseQuestionnaireResponse() throws Exception {
    testParse("questionnaireresponse-example", QuestionnaireResponse.newBuilder());
    testParse("questionnaireresponse-example-bluebook", QuestionnaireResponse.newBuilder());
    testParse("questionnaireresponse-example-f201-lifelines", QuestionnaireResponse.newBuilder());
    testParse("questionnaireresponse-example-gcs", QuestionnaireResponse.newBuilder());
    testParse("questionnaireresponse-example-ussg-fht-answers", QuestionnaireResponse.newBuilder());
  }

  /** Test printing of the QuestionnaireResponse FHIR resource. */
  @Test
  public void printQuestionnaireResponse() throws Exception {
    testPrint("questionnaireresponse-example", QuestionnaireResponse.newBuilder());
    testPrint("questionnaireresponse-example-bluebook", QuestionnaireResponse.newBuilder());
    testPrint("questionnaireresponse-example-f201-lifelines", QuestionnaireResponse.newBuilder());
    testPrint("questionnaireresponse-example-gcs", QuestionnaireResponse.newBuilder());
    testPrint("questionnaireresponse-example-ussg-fht-answers", QuestionnaireResponse.newBuilder());
  }

  /** Test parsing of the ReferralRequest FHIR resource. */
  @Test
  public void parseReferralRequest() throws Exception {
    testParse("referralrequest-example", ReferralRequest.newBuilder());
  }

  /** Test printing of the ReferralRequest FHIR resource. */
  @Test
  public void printReferralRequest() throws Exception {
    testPrint("referralrequest-example", ReferralRequest.newBuilder());
  }

  /** Test parsing of the RelatedPerson FHIR resource. */
  @Test
  public void parseRelatedPerson() throws Exception {
    testParse("relatedperson-example", RelatedPerson.newBuilder());
    testParse("relatedperson-example-f001-sarah", RelatedPerson.newBuilder());
    testParse("relatedperson-example-f002-ariadne", RelatedPerson.newBuilder());
    testParse("relatedperson-example-peter", RelatedPerson.newBuilder());
  }

  /** Test printing of the RelatedPerson FHIR resource. */
  @Test
  public void printRelatedPerson() throws Exception {
    testPrint("relatedperson-example", RelatedPerson.newBuilder());
    testPrint("relatedperson-example-f001-sarah", RelatedPerson.newBuilder());
    testPrint("relatedperson-example-f002-ariadne", RelatedPerson.newBuilder());
    testPrint("relatedperson-example-peter", RelatedPerson.newBuilder());
  }

  /** Test parsing of the RequestGroup FHIR resource. */
  @Test
  public void parseRequestGroup() throws Exception {
    testParse("requestgroup-example", RequestGroup.newBuilder());
    testParse("requestgroup-kdn5-example", RequestGroup.newBuilder());
  }

  /** Test printing of the RequestGroup FHIR resource. */
  @Test
  public void printRequestGroup() throws Exception {
    testPrint("requestgroup-example", RequestGroup.newBuilder());
    testPrint("requestgroup-kdn5-example", RequestGroup.newBuilder());
  }

  /** Test parsing of the ResearchStudy FHIR resource. */
  @Test
  public void parseResearchStudy() throws Exception {
    testParse("researchstudy-example", ResearchStudy.newBuilder());
  }

  /** Test printing of the ResearchStudy FHIR resource. */
  @Test
  public void printResearchStudy() throws Exception {
    testPrint("researchstudy-example", ResearchStudy.newBuilder());
  }

  /** Test parsing of the ResearchSubject FHIR resource. */
  @Test
  public void parseResearchSubject() throws Exception {
    testParse("researchsubject-example", ResearchSubject.newBuilder());
  }

  /** Test printing of the ResearchSubject FHIR resource. */
  @Test
  public void printResearchSubject() throws Exception {
    testPrint("researchsubject-example", ResearchSubject.newBuilder());
  }

  /** Test parsing of the RiskAssessment FHIR resource. */
  @Test
  public void parseRiskAssessment() throws Exception {
    testParse("riskassessment-example", RiskAssessment.newBuilder());
    testParse("riskassessment-example-cardiac", RiskAssessment.newBuilder());
    testParse("riskassessment-example-population", RiskAssessment.newBuilder());
    testParse("riskassessment-example-prognosis", RiskAssessment.newBuilder());
  }

  /** Test printing of the RiskAssessment FHIR resource. */
  @Test
  public void printRiskAssessment() throws Exception {
    testPrint("riskassessment-example", RiskAssessment.newBuilder());
    testPrint("riskassessment-example-cardiac", RiskAssessment.newBuilder());
    testPrint("riskassessment-example-population", RiskAssessment.newBuilder());
    testPrint("riskassessment-example-prognosis", RiskAssessment.newBuilder());
  }

  /** Test parsing of the Schedule FHIR resource. */
  @Test
  public void parseSchedule() throws Exception {
    testParse("schedule-example", Schedule.newBuilder());
    testParse("schedule-provider-location1-example", Schedule.newBuilder());
    testParse("schedule-provider-location2-example", Schedule.newBuilder());
  }

  /** Test printing of the Schedule FHIR resource. */
  @Test
  public void printSchedule() throws Exception {
    testPrint("schedule-example", Schedule.newBuilder());
    testPrint("schedule-provider-location1-example", Schedule.newBuilder());
    testPrint("schedule-provider-location2-example", Schedule.newBuilder());
  }

  /** Test parsing of the SearchParameter FHIR resource. */
  @Test
  public void parseSearchParameter() throws Exception {
    testParse("searchparameter-example", SearchParameter.newBuilder());
    testParse("searchparameter-example-extension", SearchParameter.newBuilder());
    testParse("searchparameter-example-reference", SearchParameter.newBuilder());
  }

  /** Test printing of the SearchParameter FHIR resource. */
  @Test
  public void printSearchParameter() throws Exception {
    testPrint("searchparameter-example", SearchParameter.newBuilder());
    testPrint("searchparameter-example-extension", SearchParameter.newBuilder());
    testPrint("searchparameter-example-reference", SearchParameter.newBuilder());
  }

  /** Test parsing of the Sequence FHIR resource. */
  @Test
  public void parseSequence() throws Exception {
    testParse("coord-0base-example", Sequence.newBuilder());
    testParse("coord-1base-example", Sequence.newBuilder());
    testParse("sequence-example", Sequence.newBuilder());
    testParse("sequence-example-fda", Sequence.newBuilder());
    testParse("sequence-example-fda-comparisons", Sequence.newBuilder());
    testParse("sequence-example-fda-vcfeval", Sequence.newBuilder());
    testParse("sequence-example-pgx-1", Sequence.newBuilder());
    testParse("sequence-example-pgx-2", Sequence.newBuilder());
    testParse("sequence-example-TPMT-one", Sequence.newBuilder());
    testParse("sequence-example-TPMT-two", Sequence.newBuilder());
    testParse("sequence-graphic-example-1", Sequence.newBuilder());
    testParse("sequence-graphic-example-2", Sequence.newBuilder());
    testParse("sequence-graphic-example-3", Sequence.newBuilder());
    testParse("sequence-graphic-example-4", Sequence.newBuilder());
    testParse("sequence-graphic-example-5", Sequence.newBuilder());
  }

  /** Test printing of the Sequence FHIR resource. */
  @Test
  public void printSequence() throws Exception {
    testPrint("coord-0base-example", Sequence.newBuilder());
    testPrint("coord-1base-example", Sequence.newBuilder());
    testPrint("sequence-example", Sequence.newBuilder());
    testPrint("sequence-example-fda", Sequence.newBuilder());
    testPrint("sequence-example-fda-comparisons", Sequence.newBuilder());
    testPrint("sequence-example-fda-vcfeval", Sequence.newBuilder());
    testPrint("sequence-example-pgx-1", Sequence.newBuilder());
    testPrint("sequence-example-pgx-2", Sequence.newBuilder());
    testPrint("sequence-example-TPMT-one", Sequence.newBuilder());
    testPrint("sequence-example-TPMT-two", Sequence.newBuilder());
    testPrint("sequence-graphic-example-1", Sequence.newBuilder());
    testPrint("sequence-graphic-example-2", Sequence.newBuilder());
    testPrint("sequence-graphic-example-3", Sequence.newBuilder());
    testPrint("sequence-graphic-example-4", Sequence.newBuilder());
    testPrint("sequence-graphic-example-5", Sequence.newBuilder());
  }

  /** Test parsing of the ServiceDefinition FHIR resource. */
  @Test
  public void parseServiceDefinition() throws Exception {
    testParse("servicedefinition-example", ServiceDefinition.newBuilder());
  }

  /** Test printing of the ServiceDefinition FHIR resource. */
  @Test
  public void printServiceDefinition() throws Exception {
    testPrint("servicedefinition-example", ServiceDefinition.newBuilder());
  }

  /** Test parsing of the Slot FHIR resource. */
  @Test
  public void parseSlot() throws Exception {
    testParse("slot-example", Slot.newBuilder());
    testParse("slot-example-busy", Slot.newBuilder());
    testParse("slot-example-tentative", Slot.newBuilder());
    testParse("slot-example-unavailable", Slot.newBuilder());
  }

  /** Test printing of the Slot FHIR resource. */
  @Test
  public void printSlot() throws Exception {
    testPrint("slot-example", Slot.newBuilder());
    testPrint("slot-example-busy", Slot.newBuilder());
    testPrint("slot-example-tentative", Slot.newBuilder());
    testPrint("slot-example-unavailable", Slot.newBuilder());
  }

  /** Test parsing of the Specimen FHIR resource. */
  @Test
  public void parseSpecimen() throws Exception {
    testParse("specimen-example", Specimen.newBuilder());
    testParse("specimen-example-isolate", Specimen.newBuilder());
    testParse("specimen-example-serum", Specimen.newBuilder());
    testParse("specimen-example-urine", Specimen.newBuilder());
  }

  /** Test printing of the Specimen FHIR resource. */
  @Test
  public void printSpecimen() throws Exception {
    testPrint("specimen-example", Specimen.newBuilder());
    testPrint("specimen-example-isolate", Specimen.newBuilder());
    testPrint("specimen-example-serum", Specimen.newBuilder());
    testPrint("specimen-example-urine", Specimen.newBuilder());
  }

  /** Test parsing of the StructureDefinition FHIR resource. */
  @Test
  public void parseStructureDefinition() throws Exception {
    testParse("structuredefinition-example", StructureDefinition.newBuilder());
  }

  /** Test printing of the StructureDefinition FHIR resource. */
  @Test
  public void printStructureDefinition() throws Exception {
    testPrint("structuredefinition-example", StructureDefinition.newBuilder());
  }

  /** Test parsing of the StructureMap FHIR resource. */
  @Test
  public void parseStructureMap() throws Exception {
    testParse("structuremap-example", StructureMap.newBuilder());
  }

  /** Test printing of the StructureMap FHIR resource. */
  @Test
  public void printStructureMap() throws Exception {
    testPrint("structuremap-example", StructureMap.newBuilder());
  }

  /** Test parsing of the Subscription FHIR resource. */
  @Test
  public void parseSubscription() throws Exception {
    testParse("subscription-example", Subscription.newBuilder());
    testParse("subscription-example-error", Subscription.newBuilder());
  }

  /** Test printing of the Subscription FHIR resource. */
  @Test
  public void printSubscription() throws Exception {
    testPrint("subscription-example", Subscription.newBuilder());
    testPrint("subscription-example-error", Subscription.newBuilder());
  }

  /** Test parsing of the Substance FHIR resource. */
  @Test
  public void parseSubstance() throws Exception {
    testParse("substance-example", Substance.newBuilder());
    testParse("substance-example-amoxicillin-clavulanate", Substance.newBuilder());
    testParse("substance-example-f201-dust", Substance.newBuilder());
    testParse("substance-example-f202-staphylococcus", Substance.newBuilder());
    testParse("substance-example-f203-potassium", Substance.newBuilder());
    testParse("substance-example-silver-nitrate-product", Substance.newBuilder());
  }

  /** Test printing of the Substance FHIR resource. */
  @Test
  public void printSubstance() throws Exception {
    testPrint("substance-example", Substance.newBuilder());
    testPrint("substance-example-amoxicillin-clavulanate", Substance.newBuilder());
    testPrint("substance-example-f201-dust", Substance.newBuilder());
    testPrint("substance-example-f202-staphylococcus", Substance.newBuilder());
    testPrint("substance-example-f203-potassium", Substance.newBuilder());
    testPrint("substance-example-silver-nitrate-product", Substance.newBuilder());
  }

  /** Test parsing of the SupplyDelivery FHIR resource. */
  @Test
  public void parseSupplyDelivery() throws Exception {
    testParse("supplydelivery-example", SupplyDelivery.newBuilder());
    testParse("supplydelivery-example-pumpdelivery", SupplyDelivery.newBuilder());
  }

  /** Test printing of the SupplyDelivery FHIR resource. */
  @Test
  public void printSupplyDelivery() throws Exception {
    testPrint("supplydelivery-example", SupplyDelivery.newBuilder());
    testPrint("supplydelivery-example-pumpdelivery", SupplyDelivery.newBuilder());
  }

  /** Test parsing of the SupplyRequest FHIR resource. */
  @Test
  public void parseSupplyRequest() throws Exception {
    testParse("supplyrequest-example-simpleorder", SupplyRequest.newBuilder());
  }

  /** Test printing of the SupplyRequest FHIR resource. */
  @Test
  public void printSupplyRequest() throws Exception {
    testPrint("supplyrequest-example-simpleorder", SupplyRequest.newBuilder());
  }

  /** Test parsing of the Task FHIR resource. */
  @Test
  public void parseTask() throws Exception {
    testParse("task-example1", Task.newBuilder());
    testParse("task-example2", Task.newBuilder());
    testParse("task-example3", Task.newBuilder());
    testParse("task-example4", Task.newBuilder());
    testParse("task-example5", Task.newBuilder());
    testParse("task-example6", Task.newBuilder());
  }

  /** Test printing of the Task FHIR resource. */
  @Test
  public void printTask() throws Exception {
    testPrint("task-example1", Task.newBuilder());
    testPrint("task-example2", Task.newBuilder());
    testPrint("task-example3", Task.newBuilder());
    testPrint("task-example4", Task.newBuilder());
    testPrint("task-example5", Task.newBuilder());
    testPrint("task-example6", Task.newBuilder());
  }

  /** Test parsing of the TestReport FHIR resource. */
  @Test
  public void parseTestReport() throws Exception {
    testParse("testreport-example", TestReport.newBuilder());
  }

  /** Test printing of the TestReport FHIR resource. */
  @Test
  public void printTestReport() throws Exception {
    testPrint("testreport-example", TestReport.newBuilder());
  }

  /** Test parsing of the TestScript FHIR resource. */
  @Test
  public void parseTestScript() throws Exception {
    testParse("testscript-example", TestScript.newBuilder());
    testParse("testscript-example-history", TestScript.newBuilder());
    testParse("testscript-example-multisystem", TestScript.newBuilder());
    testParse("testscript-example-readtest", TestScript.newBuilder());
    testParse("testscript-example-rule", TestScript.newBuilder());
    testParse("testscript-example-search", TestScript.newBuilder());
    testParse("testscript-example-update", TestScript.newBuilder());
  }

  /** Test printing of the TestScript FHIR resource. */
  @Test
  public void printTestScript() throws Exception {
    testPrint("testscript-example", TestScript.newBuilder());
    testPrint("testscript-example-history", TestScript.newBuilder());
    testPrint("testscript-example-multisystem", TestScript.newBuilder());
    testPrint("testscript-example-readtest", TestScript.newBuilder());
    testPrint("testscript-example-rule", TestScript.newBuilder());
    testPrint("testscript-example-search", TestScript.newBuilder());
    testPrint("testscript-example-update", TestScript.newBuilder());
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
    testParse("visionprescription-example", VisionPrescription.newBuilder());
    testParse("visionprescription-example-1", VisionPrescription.newBuilder());
  }

  /** Test printing of the VisionPrescription FHIR resource. */
  @Test
  public void printVisionPrescription() throws Exception {
    testPrint("visionprescription-example", VisionPrescription.newBuilder());
    testPrint("visionprescription-example-1", VisionPrescription.newBuilder());
  }
}
