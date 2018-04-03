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
import com.google.fhir.stu3.proto.Annotations;
import com.google.fhir.stu3.proto.StructureDefinition;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.ExtensionRegistry;
import com.google.protobuf.TextFormat;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link ProtoGenerator}. */
@RunWith(JUnit4.class)
public class ProtoGeneratorTest {

  private JsonFormat.Parser jsonParser;
  private TextFormat.Parser textParser;
  private ExtensionRegistry registry;
  private ProtoGenerator protoGenerator;

  /** Read the specifed file from the testdata directory into a String. */
  private String loadFile(String filename) throws IOException {
    File file = new File("testdata/stu3/structure_definitions/" + filename);
    return Files.asCharSource(file, StandardCharsets.UTF_8).read();
  }

  /** Read and parse the specified StructureDefinition. */
  private StructureDefinition readStructureDefinition(String messageName) throws IOException {
    String json = loadFile(messageName.toLowerCase() + ".profile.json");
    StructureDefinition.Builder builder = StructureDefinition.newBuilder();
    jsonParser.merge(json, builder);
    return builder.build();
  }

  /** Read and parse the specified DescriptorProto. */
  private DescriptorProto readDescriptorProto(String messageName) throws IOException {
    String text = loadFile(messageName.toLowerCase() + ".descriptor.prototxt");
    DescriptorProto.Builder builder = DescriptorProto.newBuilder();
    textParser.merge(text, registry, builder);
    return builder.build();
  }

  private void testGeneratedProto(String resourceName) throws IOException {
    StructureDefinition resource = readStructureDefinition(resourceName);
    DescriptorProto generatedProto = protoGenerator.generateProto(resource);
    DescriptorProto golden = readDescriptorProto(resourceName);
    assertThat(golden).isEqualTo(generatedProto);
  }

  @Before
  public void setUp() {
    jsonParser = JsonFormat.getParser();
    textParser = TextFormat.getParser();
    protoGenerator = new ProtoGenerator("google.fhir.stu3.proto", "proto/stu3");
    registry = ExtensionRegistry.newInstance();
    registry.add(Annotations.structureDefinitionKind);
    registry.add(Annotations.validationRequirement);
    registry.add(Annotations.isChoiceType);
    registry.add(Annotations.oneofValidationRequirement);
    registry.add(Annotations.fieldDescription);
    registry.add(Annotations.messageDescription);
  }

  // TODO(sundberg): Test the basic FHIR data types.

  // Test the FHIR resource types individually.

  /** Test generating the Account FHIR resource. */
  @Test
  public void generateAccount() throws Exception {
    testGeneratedProto("Account");
  }

  /** Test generating the ActivityDefinition FHIR resource. */
  @Test
  public void generateActivityDefinition() throws Exception {
    testGeneratedProto("ActivityDefinition");
  }

  /** Test generating the AdverseEvent FHIR resource. */
  @Test
  public void generateAdverseEvent() throws Exception {
    testGeneratedProto("AdverseEvent");
  }

  /** Test generating the AllergyIntolerance FHIR resource. */
  @Test
  public void generateAllergyIntolerance() throws Exception {
    testGeneratedProto("AllergyIntolerance");
  }

  /** Test generating the Appointment FHIR resource. */
  @Test
  public void generateAppointment() throws Exception {
    testGeneratedProto("Appointment");
  }

  /** Test generating the AppointmentResponse FHIR resource. */
  @Test
  public void generateAppointmentResponse() throws Exception {
    testGeneratedProto("AppointmentResponse");
  }

  /** Test generating the AuditEvent FHIR resource. */
  @Test
  public void generateAuditEvent() throws Exception {
    testGeneratedProto("AuditEvent");
  }

  /** Test generating the Basic FHIR resource. */
  @Test
  public void generateBasic() throws Exception {
    testGeneratedProto("Basic");
  }

  /** Test generating the Binary FHIR resource. */
  @Test
  public void generateBinary() throws Exception {
    testGeneratedProto("Binary");
  }

  /** Test generating the BodySite FHIR resource. */
  @Test
  public void generateBodySite() throws Exception {
    testGeneratedProto("BodySite");
  }

  /** Test generating the Bundle FHIR resource. */
  @Test
  public void generateBundle() throws Exception {
    testGeneratedProto("Bundle");
  }

  /** Test generating the CapabilityStatement FHIR resource. */
  @Test
  public void generateCapabilityStatement() throws Exception {
    testGeneratedProto("CapabilityStatement");
  }

  /** Test generating the CarePlan FHIR resource. */
  @Test
  public void generateCarePlan() throws Exception {
    testGeneratedProto("CarePlan");
  }

  /** Test generating the CareTeam FHIR resource. */
  @Test
  public void generateCareTeam() throws Exception {
    testGeneratedProto("CareTeam");
  }

  /** Test generating the ChargeItem FHIR resource. */
  @Test
  public void generateChargeItem() throws Exception {
    testGeneratedProto("ChargeItem");
  }

  /** Test generating the Claim FHIR resource. */
  @Test
  public void generateClaim() throws Exception {
    testGeneratedProto("Claim");
  }

  /** Test generating the ClaimResponse FHIR resource. */
  @Test
  public void generateClaimResponse() throws Exception {
    testGeneratedProto("ClaimResponse");
  }

  /** Test generating the ClinicalImpression FHIR resource. */
  @Test
  public void generateClinicalImpression() throws Exception {
    testGeneratedProto("ClinicalImpression");
  }

  /** Test generating the CodeSystem FHIR resource. */
  @Test
  public void generateCodeSystem() throws Exception {
    testGeneratedProto("CodeSystem");
  }

  /** Test generating the Communication FHIR resource. */
  @Test
  public void generateCommunication() throws Exception {
    testGeneratedProto("Communication");
  }

  /** Test generating the CommunicationRequest FHIR resource. */
  @Test
  public void generateCommunicationRequest() throws Exception {
    testGeneratedProto("CommunicationRequest");
  }

  /** Test generating the CompartmentDefinition FHIR resource. */
  @Test
  public void generateCompartmentDefinition() throws Exception {
    testGeneratedProto("CompartmentDefinition");
  }

  /** Test generating the Composition FHIR resource. */
  @Test
  public void generateComposition() throws Exception {
    testGeneratedProto("Composition");
  }

  /** Test generating the ConceptMap FHIR resource. */
  @Test
  public void generateConceptMap() throws Exception {
    testGeneratedProto("ConceptMap");
  }

  /** Test generating the Condition FHIR resource. */
  @Test
  public void generateCondition() throws Exception {
    testGeneratedProto("Condition");
  }

  /** Test generating the Consent FHIR resource. */
  @Test
  public void generateConsent() throws Exception {
    testGeneratedProto("Consent");
  }

  /** Test generating the Contract FHIR resource. */
  @Test
  public void generateContract() throws Exception {
    testGeneratedProto("Contract");
  }

  /** Test generating the Coverage FHIR resource. */
  @Test
  public void generateCoverage() throws Exception {
    testGeneratedProto("Coverage");
  }

  /** Test generating the DataElement FHIR resource. */
  @Test
  public void generateDataElement() throws Exception {
    testGeneratedProto("DataElement");
  }

  /** Test generating the DetectedIssue FHIR resource. */
  @Test
  public void generateDetectedIssue() throws Exception {
    testGeneratedProto("DetectedIssue");
  }

  /** Test generating the Device FHIR resource. */
  @Test
  public void generateDevice() throws Exception {
    testGeneratedProto("Device");
  }

  /** Test generating the DeviceComponent FHIR resource. */
  @Test
  public void generateDeviceComponent() throws Exception {
    testGeneratedProto("DeviceComponent");
  }

  /** Test generating the DeviceMetric FHIR resource. */
  @Test
  public void generateDeviceMetric() throws Exception {
    testGeneratedProto("DeviceMetric");
  }

  /** Test generating the DeviceRequest FHIR resource. */
  @Test
  public void generateDeviceRequest() throws Exception {
    testGeneratedProto("DeviceRequest");
  }

  /** Test generating the DeviceUseStatement FHIR resource. */
  @Test
  public void generateDeviceUseStatement() throws Exception {
    testGeneratedProto("DeviceUseStatement");
  }

  /** Test generating the DiagnosticReport FHIR resource. */
  @Test
  public void generateDiagnosticReport() throws Exception {
    testGeneratedProto("DiagnosticReport");
  }

  /** Test generating the DocumentManifest FHIR resource. */
  @Test
  public void generateDocumentManifest() throws Exception {
    testGeneratedProto("DocumentManifest");
  }

  /** Test generating the DocumentReference FHIR resource. */
  @Test
  public void generateDocumentReference() throws Exception {
    testGeneratedProto("DocumentReference");
  }

  /** Test generating the EligibilityRequest FHIR resource. */
  @Test
  public void generateEligibilityRequest() throws Exception {
    testGeneratedProto("EligibilityRequest");
  }

  /** Test generating the EligibilityResponse FHIR resource. */
  @Test
  public void generateEligibilityResponse() throws Exception {
    testGeneratedProto("EligibilityResponse");
  }

  /** Test generating the Encounter FHIR resource. */
  @Test
  public void generateEncounter() throws Exception {
    testGeneratedProto("Encounter");
  }

  /** Test generating the Endpoint FHIR resource. */
  @Test
  public void generateEndpoint() throws Exception {
    testGeneratedProto("Endpoint");
  }

  /** Test generating the EnrollmentRequest FHIR resource. */
  @Test
  public void generateEnrollmentRequest() throws Exception {
    testGeneratedProto("EnrollmentRequest");
  }

  /** Test generating the EnrollmentResponse FHIR resource. */
  @Test
  public void generateEnrollmentResponse() throws Exception {
    testGeneratedProto("EnrollmentResponse");
  }

  /** Test generating the EpisodeOfCare FHIR resource. */
  @Test
  public void generateEpisodeOfCare() throws Exception {
    testGeneratedProto("EpisodeOfCare");
  }

  /** Test generating the ExpansionProfile FHIR resource. */
  @Test
  public void generateExpansionProfile() throws Exception {
    testGeneratedProto("ExpansionProfile");
  }

  /** Test generating the ExplanationOfBenefit FHIR resource. */
  @Test
  public void generateExplanationOfBenefit() throws Exception {
    testGeneratedProto("ExplanationOfBenefit");
  }

  /** Test generating the FamilyMemberHistory FHIR resource. */
  @Test
  public void generateFamilyMemberHistory() throws Exception {
    testGeneratedProto("FamilyMemberHistory");
  }

  /** Test generating the Flag FHIR resource. */
  @Test
  public void generateFlag() throws Exception {
    testGeneratedProto("Flag");
  }

  /** Test generating the Goal FHIR resource. */
  @Test
  public void generateGoal() throws Exception {
    testGeneratedProto("Goal");
  }

  /** Test generating the GraphDefinition FHIR resource. */
  @Test
  public void generateGraphDefinition() throws Exception {
    testGeneratedProto("GraphDefinition");
  }

  /** Test generating the Group FHIR resource. */
  @Test
  public void generateGroup() throws Exception {
    testGeneratedProto("Group");
  }

  /** Test generating the GuidanceResponse FHIR resource. */
  @Test
  public void generateGuidanceResponse() throws Exception {
    testGeneratedProto("GuidanceResponse");
  }

  /** Test generating the HealthcareService FHIR resource. */
  @Test
  public void generateHealthcareService() throws Exception {
    testGeneratedProto("HealthcareService");
  }

  /** Test generating the ImagingManifest FHIR resource. */
  @Test
  public void generateImagingManifest() throws Exception {
    testGeneratedProto("ImagingManifest");
  }

  /** Test generating the ImagingStudy FHIR resource. */
  @Test
  public void generateImagingStudy() throws Exception {
    testGeneratedProto("ImagingStudy");
  }

  /** Test generating the Immunization FHIR resource. */
  @Test
  public void generateImmunization() throws Exception {
    testGeneratedProto("Immunization");
  }

  /** Test generating the ImmunizationRecommendation FHIR resource. */
  @Test
  public void generateImmunizationRecommendation() throws Exception {
    testGeneratedProto("ImmunizationRecommendation");
  }

  /** Test generating the ImplementationGuide FHIR resource. */
  @Test
  public void generateImplementationGuide() throws Exception {
    testGeneratedProto("ImplementationGuide");
  }

  /** Test generating the Library FHIR resource. */
  @Test
  public void generateLibrary() throws Exception {
    testGeneratedProto("Library");
  }

  /** Test generating the Linkage FHIR resource. */
  @Test
  public void generateLinkage() throws Exception {
    testGeneratedProto("Linkage");
  }

  /** Test generating the List FHIR resource. */
  @Test
  public void generateList() throws Exception {
    testGeneratedProto("List");
  }

  /** Test generating the Location FHIR resource. */
  @Test
  public void generateLocation() throws Exception {
    testGeneratedProto("Location");
  }

  /** Test generating the Measure FHIR resource. */
  @Test
  public void generateMeasure() throws Exception {
    testGeneratedProto("Measure");
  }

  /** Test generating the MeasureReport FHIR resource. */
  @Test
  public void generateMeasureReport() throws Exception {
    testGeneratedProto("MeasureReport");
  }

  /** Test generating the Media FHIR resource. */
  @Test
  public void generateMedia() throws Exception {
    testGeneratedProto("Media");
  }

  /** Test generating the Medication FHIR resource. */
  @Test
  public void generateMedication() throws Exception {
    testGeneratedProto("Medication");
  }

  /** Test generating the MedicationAdministration FHIR resource. */
  @Test
  public void generateMedicationAdministration() throws Exception {
    testGeneratedProto("MedicationAdministration");
  }

  /** Test generating the MedicationDispense FHIR resource. */
  @Test
  public void generateMedicationDispense() throws Exception {
    testGeneratedProto("MedicationDispense");
  }

  /** Test generating the MedicationRequest FHIR resource. */
  @Test
  public void generateMedicationRequest() throws Exception {
    testGeneratedProto("MedicationRequest");
  }

  /** Test generating the MedicationStatement FHIR resource. */
  @Test
  public void generateMedicationStatement() throws Exception {
    testGeneratedProto("MedicationStatement");
  }

  /** Test generating the MessageDefinition FHIR resource. */
  @Test
  public void generateMessageDefinition() throws Exception {
    testGeneratedProto("MessageDefinition");
  }

  /** Test generating the MessageHeader FHIR resource. */
  @Test
  public void generateMessageHeader() throws Exception {
    testGeneratedProto("MessageHeader");
  }

  /** Test generating the NamingSystem FHIR resource. */
  @Test
  public void generateNamingSystem() throws Exception {
    testGeneratedProto("NamingSystem");
  }

  /** Test generating the NutritionOrder FHIR resource. */
  @Test
  public void generateNutritionOrder() throws Exception {
    testGeneratedProto("NutritionOrder");
  }

  /** Test generating the Observation FHIR resource. */
  @Test
  public void generateObservation() throws Exception {
    testGeneratedProto("Observation");
  }

  /** Test generating the OperationDefinition FHIR resource. */
  @Test
  public void generateOperationDefinition() throws Exception {
    testGeneratedProto("OperationDefinition");
  }

  /** Test generating the OperationOutcome FHIR resource. */
  @Test
  public void generateOperationOutcome() throws Exception {
    testGeneratedProto("OperationOutcome");
  }

  /** Test generating the Organization FHIR resource. */
  @Test
  public void generateOrganization() throws Exception {
    testGeneratedProto("Organization");
  }

  /** Test generating the Parameters FHIR resource. */
  @Test
  public void generateParameters() throws Exception {
    testGeneratedProto("Parameters");
  }

  /** Test generating the Patient FHIR resource. */
  @Test
  public void generatePatient() throws Exception {
    testGeneratedProto("Patient");
  }

  /** Test generating the PaymentNotice FHIR resource. */
  @Test
  public void generatePaymentNotice() throws Exception {
    testGeneratedProto("PaymentNotice");
  }

  /** Test generating the PaymentReconciliation FHIR resource. */
  @Test
  public void generatePaymentReconciliation() throws Exception {
    testGeneratedProto("PaymentReconciliation");
  }

  /** Test generating the Person FHIR resource. */
  @Test
  public void generatePerson() throws Exception {
    testGeneratedProto("Person");
  }

  /** Test generating the PlanDefinition FHIR resource. */
  @Test
  public void generatePlanDefinition() throws Exception {
    testGeneratedProto("PlanDefinition");
  }

  /** Test generating the Practitioner FHIR resource. */
  @Test
  public void generatePractitioner() throws Exception {
    testGeneratedProto("Practitioner");
  }

  /** Test generating the PractitionerRole FHIR resource. */
  @Test
  public void generatePractitionerRole() throws Exception {
    testGeneratedProto("PractitionerRole");
  }

  /** Test generating the Procedure FHIR resource. */
  @Test
  public void generateProcedure() throws Exception {
    testGeneratedProto("Procedure");
  }

  /** Test generating the ProcedureRequest FHIR resource. */
  @Test
  public void generateProcedureRequest() throws Exception {
    testGeneratedProto("ProcedureRequest");
  }

  /** Test generating the ProcessRequest FHIR resource. */
  @Test
  public void generateProcessRequest() throws Exception {
    testGeneratedProto("ProcessRequest");
  }

  /** Test generating the ProcessResponse FHIR resource. */
  @Test
  public void generateProcessResponse() throws Exception {
    testGeneratedProto("ProcessResponse");
  }

  /** Test generating the Provenance FHIR resource. */
  @Test
  public void generateProvenance() throws Exception {
    testGeneratedProto("Provenance");
  }

  /** Test generating the Questionnaire FHIR resource. */
  @Test
  public void generateQuestionnaire() throws Exception {
    testGeneratedProto("Questionnaire");
  }

  /** Test generating the QuestionnaireResponse FHIR resource. */
  @Test
  public void generateQuestionnaireResponse() throws Exception {
    testGeneratedProto("QuestionnaireResponse");
  }

  /** Test generating the ReferralRequest FHIR resource. */
  @Test
  public void generateReferralRequest() throws Exception {
    testGeneratedProto("ReferralRequest");
  }

  /** Test generating the RelatedPerson FHIR resource. */
  @Test
  public void generateRelatedPerson() throws Exception {
    testGeneratedProto("RelatedPerson");
  }

  /** Test generating the RequestGroup FHIR resource. */
  @Test
  public void generateRequestGroup() throws Exception {
    testGeneratedProto("RequestGroup");
  }

  /** Test generating the ResearchStudy FHIR resource. */
  @Test
  public void generateResearchStudy() throws Exception {
    testGeneratedProto("ResearchStudy");
  }

  /** Test generating the ResearchSubject FHIR resource. */
  @Test
  public void generateResearchSubject() throws Exception {
    testGeneratedProto("ResearchSubject");
  }

  /** Test generating the RiskAssessment FHIR resource. */
  @Test
  public void generateRiskAssessment() throws Exception {
    testGeneratedProto("RiskAssessment");
  }

  /** Test generating the Schedule FHIR resource. */
  @Test
  public void generateSchedule() throws Exception {
    testGeneratedProto("Schedule");
  }

  /** Test generating the SearchParameter FHIR resource. */
  @Test
  public void generateSearchParameter() throws Exception {
    testGeneratedProto("SearchParameter");
  }

  /** Test generating the Sequence FHIR resource. */
  @Test
  public void generateSequence() throws Exception {
    testGeneratedProto("Sequence");
  }

  /** Test generating the ServiceDefinition FHIR resource. */
  @Test
  public void generateServiceDefinition() throws Exception {
    testGeneratedProto("ServiceDefinition");
  }

  /** Test generating the Slot FHIR resource. */
  @Test
  public void generateSlot() throws Exception {
    testGeneratedProto("Slot");
  }

  /** Test generating the Specimen FHIR resource. */
  @Test
  public void generateSpecimen() throws Exception {
    testGeneratedProto("Specimen");
  }

  /** Test generating the StructureDefinition FHIR resource. */
  @Test
  public void generateStructureDefinition() throws Exception {
    testGeneratedProto("StructureDefinition");
  }

  /** Test generating the StructureMap FHIR resource. */
  @Test
  public void generateStructureMap() throws Exception {
    testGeneratedProto("StructureMap");
  }

  /** Test generating the Subscription FHIR resource. */
  @Test
  public void generateSubscription() throws Exception {
    testGeneratedProto("Subscription");
  }

  /** Test generating the Substance FHIR resource. */
  @Test
  public void generateSubstance() throws Exception {
    testGeneratedProto("Substance");
  }

  /** Test generating the SupplyDelivery FHIR resource. */
  @Test
  public void generateSupplyDelivery() throws Exception {
    testGeneratedProto("SupplyDelivery");
  }

  /** Test generating the SupplyRequest FHIR resource. */
  @Test
  public void generateSupplyRequest() throws Exception {
    testGeneratedProto("SupplyRequest");
  }

  /** Test generating the Task FHIR resource. */
  @Test
  public void generateTask() throws Exception {
    testGeneratedProto("Task");
  }

  /** Test generating the TestReport FHIR resource. */
  @Test
  public void generateTestReport() throws Exception {
    testGeneratedProto("TestReport");
  }

  /** Test generating the TestScript FHIR resource. */
  @Test
  public void generateTestScript() throws Exception {
    testGeneratedProto("TestScript");
  }

  /** Test generating the ValueSet FHIR resource. */
  @Test
  public void generateValueSet() throws Exception {
    testGeneratedProto("ValueSet");
  }

  /** Test generating the VisionPrescription FHIR resource. */
  @Test
  public void generateVisionPrescription() throws Exception {
    testGeneratedProto("VisionPrescription");
  }
}
