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
import com.google.protobuf.Descriptors.Descriptor;
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

  private void verifyCompiledDescriptor(Descriptor descriptor) throws IOException {
    DescriptorProto golden = readDescriptorProto(descriptor.getName());
    assertThat(golden).isEqualTo(descriptor.toProto());
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
    registry.add(Annotations.fhirReferenceType);
  }

  // We don't yet generate the FHIR primitive type descriptors. As a result, here we just verify
  // that the descriptors in testdata/ correspond to the compiled protos in the test binary.

  /** Verify the Base64Binary FHIR primitive type. */
  @Test
  public void verifyBase64Binary() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.Base64Binary.getDescriptor());
  }

  /** Verify the Boolean FHIR primitive type. */
  @Test
  public void verifyBoolean() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.Boolean.getDescriptor());
  }

  /** Verify the Code FHIR primitive type. */
  @Test
  public void verifyCode() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.Code.getDescriptor());
  }

  /** Verify the Date FHIR primitive type. */
  @Test
  public void verifyDate() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.Date.getDescriptor());
  }

  /** Verify the DateTime FHIR primitive type. */
  @Test
  public void verifyDateTime() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.DateTime.getDescriptor());
  }

  /** Verify the Decimal FHIR primitive type. */
  @Test
  public void verifyDecimal() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.Decimal.getDescriptor());
  }

  /** Verify the Id FHIR primitive type. */
  @Test
  public void verifyId() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.Id.getDescriptor());
  }

  /** Verify the Instant FHIR primitive type. */
  @Test
  public void verifyInstant() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.Instant.getDescriptor());
  }

  /** Verify the Integer FHIR primitive type. */
  @Test
  public void verifyInteger() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.Integer.getDescriptor());
  }

  /** Verify the Markdown FHIR primitive type. */
  @Test
  public void verifyMarkdown() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.Markdown.getDescriptor());
  }

  /** Verify the Oid FHIR primitive type. */
  @Test
  public void verifyOid() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.Oid.getDescriptor());
  }

  /** Verify the PositiveInt FHIR primitive type. */
  @Test
  public void verifyPositiveInt() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.PositiveInt.getDescriptor());
  }

  /** Verify the String FHIR primitive type. */
  @Test
  public void verifyString() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.String.getDescriptor());
  }

  /** Verify the Time FHIR primitive type. */
  @Test
  public void verifyTime() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.Time.getDescriptor());
  }

  /** Verify the UnsignedInt FHIR primitive type. */
  @Test
  public void verifyUnsignedInt() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.UnsignedInt.getDescriptor());
  }

  /** Verify the Uri FHIR primitive type. */
  @Test
  public void verifyUri() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.Uri.getDescriptor());
  }

  // Test the complex FHIR data types individually. */

  /** Test generating the Address FHIR complex type. */
  @Test
  public void generateAddress() throws Exception {
    testGeneratedProto("Address");
  }

  /** Test generating the Age FHIR complex type. */
  @Test
  public void generateAge() throws Exception {
    testGeneratedProto("Age");
  }

  /** Test generating the Annotation FHIR complex type. */
  @Test
  public void generateAnnotation() throws Exception {
    testGeneratedProto("Annotation");
  }

  /** Test generating the Attachment FHIR complex type. */
  @Test
  public void generateAttachment() throws Exception {
    testGeneratedProto("Attachment");
  }

  /** Test generating the CodeableConcept FHIR complex type. */
  @Test
  public void generateCodeableConcept() throws Exception {
    testGeneratedProto("CodeableConcept");
  }

  /** Test generating the Coding FHIR complex type. */
  @Test
  public void generateCoding() throws Exception {
    testGeneratedProto("Coding");
  }

  /** Test generating the ContactPoint FHIR complex type. */
  @Test
  public void generateContactPoint() throws Exception {
    testGeneratedProto("ContactPoint");
  }

  /** Test generating the Count FHIR complex type. */
  @Test
  public void generateCount() throws Exception {
    testGeneratedProto("Count");
  }

  /** Test generating the Distance FHIR complex type. */
  @Test
  public void generateDistance() throws Exception {
    testGeneratedProto("Distance");
  }

  /** Test generating the Duration FHIR complex type. */
  @Test
  public void generateDuration() throws Exception {
    testGeneratedProto("Duration");
  }

  /** Test generating the HumanName FHIR complex type. */
  @Test
  public void generateHumanName() throws Exception {
    testGeneratedProto("HumanName");
  }

  /** Test generating the Identifier FHIR complex type. */
  @Test
  public void generateIdentifier() throws Exception {
    testGeneratedProto("Identifier");
  }

  /** Test generating the Money FHIR complex type. */
  @Test
  public void generateMoney() throws Exception {
    testGeneratedProto("Money");
  }

  /** Test generating the Period FHIR complex type. */
  @Test
  public void generatePeriod() throws Exception {
    testGeneratedProto("Period");
  }

  /** Test generating the Quantity FHIR complex type. */
  @Test
  public void generateQuantity() throws Exception {
    testGeneratedProto("Quantity");
  }

  /** Test generating the Range FHIR complex type. */
  @Test
  public void generateRange() throws Exception {
    testGeneratedProto("Range");
  }

  /** Test generating the Ratio FHIR complex type. */
  @Test
  public void generateRatio() throws Exception {
    testGeneratedProto("Ratio");
  }

  /** Test generating the SampledData FHIR complex type. */
  @Test
  public void generateSampledData() throws Exception {
    testGeneratedProto("SampledData");
  }

  /** Test generating the Signature FHIR complex type. */
  @Test
  public void generateSignature() throws Exception {
    testGeneratedProto("Signature");
  }

  /** Test generating the Timing FHIR complex type. */
  @Test
  public void generateTiming() throws Exception {
    testGeneratedProto("Timing");
  }

  // Test the FHIR metadata types individually.

  /** Test generating the ContactDetail FHIR metadata type. */
  @Test
  public void generateContactDetail() throws Exception {
    testGeneratedProto("ContactDetail");
  }

  /** Test generating the Contributor FHIR metadata type. */
  @Test
  public void generateContributor() throws Exception {
    testGeneratedProto("Contributor");
  }

  /** Test generating the DataRequirement FHIR metadata type. */
  @Test
  public void generateDataRequirement() throws Exception {
    testGeneratedProto("DataRequirement");
  }

  /** Test generating the ParameterDefinition FHIR metadata type. */
  @Test
  public void generateParameterDefinition() throws Exception {
    testGeneratedProto("ParameterDefinition");
  }

  /** Test generating the RelatedArtifact FHIR metadata type. */
  @Test
  public void generateRelatedArtifact() throws Exception {
    testGeneratedProto("RelatedArtifact");
  }

  /** Test generating the TriggerDefinition FHIR metadata type. */
  @Test
  public void generateTriggerDefinition() throws Exception {
    testGeneratedProto("TriggerDefinition");
  }

  /** Test generating the UsageContext FHIR metadata type. */
  @Test
  public void generateUsageContext() throws Exception {
    testGeneratedProto("UsageContext");
  }

  // Test the FHIR special-purpose data types individually.

  /** Test generating the BackboneElement FHIR special-purpose type. */
  @Test
  public void generateBackboneElement() throws Exception {
    testGeneratedProto("BackboneElement");
  }

  /** Test generating the DomainResource FHIR special-purpose type. */
  @Test
  public void generateDomainResource() throws Exception {
    testGeneratedProto("DomainResource");
  }

  /** Test generating the Dosage FHIR special-purpose type. */
  @Test
  public void generateDosage() throws Exception {
    testGeneratedProto("Dosage");
  }

  /** Test generating the Element FHIR special-purpose type. */
  @Test
  public void generateElement() throws Exception {
    testGeneratedProto("Element");
  }

  /** Test generating the ElementDefinition FHIR special-purpose type. */
  @Test
  public void generateElementDefinition() throws Exception {
    testGeneratedProto("ElementDefinition");
  }

  /** Verify the Extension FHIR special-purpose type. */
  @Test
  public void verifyExtension() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.Extension.getDescriptor());
  }

  /** Test generating the Meta FHIR special-purpose type. */
  @Test
  public void generateMeta() throws Exception {
    testGeneratedProto("Meta");
  }

  /** Test generating the MetadataResource special-purpose type. */
  @Test
  public void generateMetadataResource() throws Exception {
    testGeneratedProto("MetadataResource");
  }

  /** Test generating the Narrative FHIR special-purpose type. */
  @Test
  public void generateNarrative() throws Exception {
    testGeneratedProto("Narrative");
  }

  /** Verify the Reference FHIR special-purpose type. */
  @Test
  public void verifyReference() throws Exception {
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.Reference.getDescriptor());
    verifyCompiledDescriptor(com.google.fhir.stu3.proto.ReferenceId.getDescriptor());
  }

  /** Test generating the Resource FHIR special-purpose type. */
  @Test
  public void generateResource() throws Exception {
    testGeneratedProto("Resource");
  }

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

  // Test generating profiles.

  /** Test generating the bmi profile. */
  @Test
  public void generateBmi() throws Exception {
    testGeneratedProto("bmi");
  }

  /** Test generating the bodyheight profile. */
  @Test
  public void generateBodyheight() throws Exception {
    testGeneratedProto("bodyheight");
  }

  /** Test generating the bodylength profile. */
  @Test
  public void generateBodylength() throws Exception {
    testGeneratedProto("bodylength");
  }

  /** Test generating the bodytemp profile. */
  @Test
  public void generateBodytemp() throws Exception {
    testGeneratedProto("bodytemp");
  }

  /** Test generating the bodyweight profile. */
  @Test
  public void generateBodyweight() throws Exception {
    testGeneratedProto("bodyweight");
  }

  /** Test generating the bp profile. */
  @Test
  public void generateBp() throws Exception {
    testGeneratedProto("bp");
  }

  /** Test generating the cholesterol profile. */
  @Test
  public void generateCholesterol() throws Exception {
    testGeneratedProto("cholesterol");
  }

  /** Test generating the clinicaldocument profile. */
  @Test
  public void generateClinicaldocument() throws Exception {
    testGeneratedProto("clinicaldocument");
  }

  /** Test generating the consentdirective profile. */
  @Test
  public void generateConsentdirective() throws Exception {
    testGeneratedProto("consentdirective");
  }

  /** Test generating the devicemetricobservation profile. */
  @Test
  public void generateDevicemetricobservation() throws Exception {
    testGeneratedProto("devicemetricobservation");
  }

  /** Test generating the diagnosticreport-genetics profile. */
  @Test
  public void generateDiagnosticreportGenetics() throws Exception {
    testGeneratedProto("diagnosticreport-genetics");
  }

  /** Test generating the elementdefinition-de profile. */
  @Test
  public void generateElementdefinitionDe() throws Exception {
    testGeneratedProto("elementdefinition-de");
  }

  /** Test generating the familymemberhistory-genetic profile. */
  @Test
  public void generateFamilymemberhistoryGenetic() throws Exception {
    testGeneratedProto("familymemberhistory-genetic");
  }

  /** Test generating the hdlcholesterol profile. */
  @Test
  public void generateHdlcholesterol() throws Exception {
    testGeneratedProto("hdlcholesterol");
  }

  /** Test generating the headcircum profile. */
  @Test
  public void generateHeadcircum() throws Exception {
    testGeneratedProto("headcircum");
  }

  /** Test generating the heartrate profile. */
  @Test
  public void generateHeartrate() throws Exception {
    testGeneratedProto("heartrate");
  }

  /** Test generating the hlaresult profile. */
  @Test
  public void generateHlaresult() throws Exception {
    testGeneratedProto("hlaresult");
  }

  /** Test generating the ldlcholesterol profile. */
  @Test
  public void generateLdlcholesterol() throws Exception {
    testGeneratedProto("ldlcholesterol");
  }

  /** Test generating the lipidprofile profile. */
  @Test
  public void generateLipidprofile() throws Exception {
    testGeneratedProto("lipidprofile");
  }

  /** Test generating the observation-genetics profile. */
  @Test
  public void generateObservationGenetics() throws Exception {
    testGeneratedProto("observation-genetics");
  }

  /** Test generating the oxygensat profile. */
  @Test
  public void generateOxygensat() throws Exception {
    testGeneratedProto("oxygensat");
  }

  /** Test generating the procedurerequest-genetics profile. */
  @Test
  public void generateProcedurerequestGenetics() throws Exception {
    testGeneratedProto("procedurerequest-genetics");
  }

  /** Test generating the resprate profile. */
  @Test
  public void generateResprate() throws Exception {
    testGeneratedProto("resprate");
  }

  /** Test generating the shareablecodesystem profile. */
  @Test
  public void generateShareablecodesystem() throws Exception {
    testGeneratedProto("shareablecodesystem");
  }

  /** Test generating the shareablevalueset profile. */
  @Test
  public void generateShareablevalueset() throws Exception {
    testGeneratedProto("shareablevalueset");
  }

  /** Test generating the simplequantity profile. */
  @Test
  public void generateSimplequantity() throws Exception {
    testGeneratedProto("simplequantity");
  }

  /** Test generating the triglyceride profile. */
  @Test
  public void generateTriglyceride() throws Exception {
    testGeneratedProto("triglyceride");
  }

  /** Test generating the uuid profile. */
  @Test
  public void generateUuid() throws Exception {
    testGeneratedProto("uuid");
  }

  /** Test generating the vitalsigns profile. */
  @Test
  public void generateVitalsigns() throws Exception {
    testGeneratedProto("vitalsigns");
  }

  /** Test generating the vitalspanel profile. */
  @Test
  public void generateVitalspanel() throws Exception {
    testGeneratedProto("vitalspanel");
  }

  /** Test generating the xhtml profile. */
  @Test
  public void generateXhtml() throws Exception {
    testGeneratedProto("xhtml");
  }
}
