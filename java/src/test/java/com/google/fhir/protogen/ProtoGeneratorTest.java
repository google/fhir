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

package com.google.fhir.protogen;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.fail;

import com.google.common.collect.ImmutableSet;
import com.google.common.io.Files;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.Annotations.FhirVersion;
import com.google.fhir.proto.ProtoGeneratorAnnotations;
import com.google.fhir.r4.core.StructureDefinition;
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

// TODO: consider adding more specialized test structure definitions that isolate
// individual functionality from ProtoGenerator

/** Unit tests for {@link ProtoGenerator}. */
@RunWith(JUnit4.class)
public class ProtoGeneratorTest {

  private JsonFormat.Parser jsonParser;
  private TextFormat.Parser textParser;
  private ExtensionRegistry registry;
  private Runfiles runfiles;

  /** Read the specifed file from the testdata directory into a String. */
  private String loadFile(String relativePath) throws IOException {
    File file = new File(runfiles.rlocation(relativePath));
    return Files.asCharSource(file, StandardCharsets.UTF_8).read();
  }

  /** Read and parse the specified StructureDefinition. */
  private StructureDefinition readStructureDefinition(String resourceName, FhirVersion version)
      throws IOException {
    String pathPrefix;
    switch (version) {
      case STU3:
        pathPrefix =
            "com_google_fhir/spec/hl7.fhir.core/3.0.1/package/StructureDefinition-";
        break;
      case R4:
        pathPrefix =
            "com_google_fhir/spec/hl7.fhir.core/4.0.0/package/StructureDefinition-";
        break;
      default:
        throw new IllegalArgumentException("unrecognized FHIR version " + version);
    }
    String json = loadFile(pathPrefix + resourceName + ".json");
    StructureDefinition.Builder builder = StructureDefinition.newBuilder();
    jsonParser.merge(json, builder);
    return builder.build();
  }

  // /** Read and parse the specified StructureDefinition. */
  // private StructureDefinition readModifiedStructureDefinition(String resourceName)
  //     throws IOException {
  //   String json =
  //       loadFile(
  //           "com_google_fhir/spec/hl7.fhir.core/3.0.1/modified/StructureDefinition-"
  //               + resourceName
  //               + ".json");
  //   StructureDefinition.Builder builder = StructureDefinition.newBuilder();
  //   jsonParser.merge(json, builder);
  //   return builder.build();
  // }

  /** Read and parse the specified DescriptorProto. */
  private DescriptorProto readDescriptorProto(String resourceName, FhirVersion version)
      throws IOException {
    String pathPrefix;
    switch (version) {
      case STU3:
        pathPrefix = "com_google_fhir/testdata/stu3/descriptors/StructureDefinition-";
        break;
      case R4:
        pathPrefix = "com_google_fhir/testdata/r4/descriptors/";
        break;
      default:
        throw new IllegalArgumentException("unrecognized FHIR version " + version);
    }
    String text = loadFile(pathPrefix + resourceName + ".descriptor.prototxt");
    DescriptorProto.Builder builder = DescriptorProto.newBuilder();
    textParser.merge(text, registry, builder);
    return builder.build();
  }

  // STU3 temporarily frozen.
  //   private void testGeneratedSTU3Proto(String resourceName) throws IOException {
  //     StructureDefinition resource = readStructureDefinition(resourceName, FhirVersion.STU3);
  //     DescriptorProto generatedProto = protoGenerator.generateProto(resource);
  //     DescriptorProto golden = readDescriptorProto(resourceName, FhirVersion.STU3);
  //     assertThat(generatedProto).isEqualTo(golden);
  //   }
  //
  //   private void testModifiedGeneratedSTU3Proto(String resourceName) throws IOException {
  //     StructureDefinition resource = readModifiedStructureDefinition(resourceName);
  //     DescriptorProto generatedProto = protoGenerator.generateProto(resource);
  //     DescriptorProto golden = readDescriptorProto(resourceName, FhirVersion.STU3);
  //     assertThat(generatedProto).isEqualTo(golden);
  //   }
  //
  //   private void testExtension(String resourceName) throws IOException {
  //     StructureDefinition resource = readStructureDefinition(resourceName, FhirVersion.STU3);
  //     DescriptorProto generatedProto = protoGenerator.generateProto(resource);
  //     DescriptorProto golden = readDescriptorProto(resourceName, FhirVersion.STU3);
  //     assertThat(generatedProto).isEqualTo(golden);
  //   }
  //
  //   private void verifyCompiledDescriptor(Descriptor descriptor) throws IOException {
  //     DescriptorProto golden = readDescriptorProto(descriptor.getName(), FhirVersion.STU3);
  //     assertThat(descriptor.toProto()).isEqualTo(golden);
  //   }

  @Before
  public void setUp() throws IOException {
    jsonParser = JsonFormat.getEarlyVersionGeneratorParser();
    textParser = TextFormat.getParser();
    runfiles = Runfiles.create();

    registry = ExtensionRegistry.newInstance();
    registry.add(Annotations.fhirCodeSystemUrl);
    registry.add(Annotations.fhirInlinedCodingCode);
    registry.add(Annotations.fhirInlinedCodingSystem);
    registry.add(Annotations.fhirInlinedExtensionUrl);
    registry.add(Annotations.fhirOneofIsOptional);
    registry.add(Annotations.fhirPathConstraint);
    registry.add(Annotations.fhirPathMessageConstraint);
    registry.add(Annotations.fhirProfileBase);
    registry.add(Annotations.fhirReferenceType);
    registry.add(Annotations.sourceCodeSystem);
    registry.add(Annotations.fhirStructureDefinitionUrl);
    registry.add(Annotations.fhirOriginalCode);
    registry.add(Annotations.fhirValuesetUrl);
    registry.add(Annotations.fhirVersion);
    registry.add(Annotations.isAbstractType);
    registry.add(Annotations.isChoiceType);
    registry.add(Annotations.referencedFhirType);
    registry.add(Annotations.structureDefinitionKind);
    registry.add(Annotations.validReferenceType);
    registry.add(Annotations.validationRequirement);
    registry.add(Annotations.valueRegex);
    registry.add(ProtoGeneratorAnnotations.fieldDescription);
    registry.add(ProtoGeneratorAnnotations.messageDescription);
    registry.add(ProtoGeneratorAnnotations.reservedReason);
  }

  // STU3 temporarily frozen.
  //
  //   // Test the primitive FHIR data types individually. */
  //
  //   /** Test generating the Base64Binary FHIR primitive type. */
  //   @Test
  //   public void generateBase64Binary() throws Exception {
  //     testGeneratedSTU3Proto("base64Binary");
  //   }
  //
  //   /** Test generating the Boolean FHIR primitive type. */
  //   @Test
  //   public void generateBoolean() throws Exception {
  //     testGeneratedSTU3Proto("boolean");
  //   }
  //
  //   /** Test generating the Code FHIR primitive type. */
  //   @Test
  //   public void generateCode() throws Exception {
  //     testGeneratedSTU3Proto("code");
  //   }
  //
  //   /** Test generating the Date FHIR primitive type. */
  //   @Test
  //   public void generateDate() throws Exception {
  //     testGeneratedSTU3Proto("date");
  //   }
  //
  //   /** Test generating the DateTime FHIR primitive type. */
  //   @Test
  //   public void generateDateTime() throws Exception {
  //     testGeneratedSTU3Proto("dateTime");
  //   }
  //
  //   /** Test generating the Decimal FHIR primitive type. */
  //   @Test
  //   public void generateDecimal() throws Exception {
  //     testGeneratedSTU3Proto("decimal");
  //   }
  //
  //   /** Test generating the Id FHIR primitive type. */
  //   @Test
  //   public void generateId() throws Exception {
  //     testGeneratedSTU3Proto("id");
  //   }
  //
  //   /** Test generating the Instant FHIR primitive type. */
  //   @Test
  //   public void generateInstant() throws Exception {
  //     testGeneratedSTU3Proto("instant");
  //   }
  //
  //   /** Test generating the Integer FHIR primitive type. */
  //   @Test
  //   public void generateInteger() throws Exception {
  //     testGeneratedSTU3Proto("integer");
  //   }
  //
  //   /** Test generating the Markdown FHIR primitive type. */
  //   @Test
  //   public void generateMarkdown() throws Exception {
  //     testGeneratedSTU3Proto("markdown");
  //   }
  //
  //   /** Test generating the Oid FHIR primitive type. */
  //   @Test
  //   public void generateOid() throws Exception {
  //     testGeneratedSTU3Proto("oid");
  //   }
  //
  //   /** Test generating the PositiveInt FHIR primitive type. */
  //   @Test
  //   public void generatePositiveInt() throws Exception {
  //     testGeneratedSTU3Proto("positiveInt");
  //   }
  //
  //   /** Test generating the String FHIR primitive type. */
  //   @Test
  //   public void generateString() throws Exception {
  //     testGeneratedSTU3Proto("string");
  //   }
  //
  //   /** Test generating the Time FHIR primitive type. */
  //   @Test
  //   public void generateTime() throws Exception {
  //     testGeneratedSTU3Proto("time");
  //   }
  //
  //   /** Test generating the UnsignedInt FHIR primitive type. */
  //   @Test
  //   public void generateUnsignedInt() throws Exception {
  //     testGeneratedSTU3Proto("unsignedInt");
  //   }
  //
  //   /** Test generating the Uri FHIR primitive type. */
  //   @Test
  //   public void generateUri() throws Exception {
  //     testGeneratedSTU3Proto("uri");
  //   }
  //
  //   // Test the complex FHIR data types individually. */
  //
  //   /** Test generating the Address FHIR complex type. */
  //   @Test
  //   public void generateAddress() throws Exception {
  //     testGeneratedSTU3Proto("Address");
  //   }
  //
  //   /** Test generating the Age FHIR complex type. */
  //   @Test
  //   public void generateAge() throws Exception {
  //     testGeneratedSTU3Proto("Age");
  //   }
  //
  //   /** Test generating the Annotation FHIR complex type. */
  //   @Test
  //   public void generateAnnotation() throws Exception {
  //     testGeneratedSTU3Proto("Annotation");
  //   }
  //
  //   /** Test generating the Attachment FHIR complex type. */
  //   @Test
  //   public void generateAttachment() throws Exception {
  //     testGeneratedSTU3Proto("Attachment");
  //   }
  //
  //   /** Test generating the CodeableConcept FHIR complex type. */
  //   @Test
  //   public void generateCodeableConcept() throws Exception {
  //     testGeneratedSTU3Proto("CodeableConcept");
  //   }
  //
  //   /** Test generating the Coding FHIR complex type. */
  //   @Test
  //   public void generateCoding() throws Exception {
  //     testGeneratedSTU3Proto("Coding");
  //   }
  //
  //   /** Test generating the ContactPoint FHIR complex type. */
  //   @Test
  //   public void generateContactPoint() throws Exception {
  //     testGeneratedSTU3Proto("ContactPoint");
  //   }
  //
  //   /** Test generating the Count FHIR complex type. */
  //   @Test
  //   public void generateCount() throws Exception {
  //     testGeneratedSTU3Proto("Count");
  //   }
  //
  //   /** Test generating the Distance FHIR complex type. */
  //   @Test
  //   public void generateDistance() throws Exception {
  //     testGeneratedSTU3Proto("Distance");
  //   }
  //
  //   /** Test generating the Duration FHIR complex type. */
  //   @Test
  //   public void generateDuration() throws Exception {
  //     testGeneratedSTU3Proto("Duration");
  //   }
  //
  //   /** Test generating the HumanName FHIR complex type. */
  //   @Test
  //   public void generateHumanName() throws Exception {
  //     testGeneratedSTU3Proto("HumanName");
  //   }
  //
  //   /** Test generating the Identifier FHIR complex type. */
  //   @Test
  //   public void generateIdentifier() throws Exception {
  //     testGeneratedSTU3Proto("Identifier");
  //   }
  //
  //   /** Test generating the Money FHIR complex type. */
  //   @Test
  //   public void generateMoney() throws Exception {
  //     testGeneratedSTU3Proto("Money");
  //   }
  //
  //   /** Test generating the Period FHIR complex type. */
  //   @Test
  //   public void generatePeriod() throws Exception {
  //     testGeneratedSTU3Proto("Period");
  //   }
  //
  //   /** Test generating the Quantity FHIR complex type. */
  //   @Test
  //   public void generateQuantity() throws Exception {
  //     testGeneratedSTU3Proto("Quantity");
  //   }
  //
  //   /** Test generating the Range FHIR complex type. */
  //   @Test
  //   public void generateRange() throws Exception {
  //     testGeneratedSTU3Proto("Range");
  //   }
  //
  //   /** Test generating the Ratio FHIR complex type. */
  //   @Test
  //   public void generateRatio() throws Exception {
  //     testGeneratedSTU3Proto("Ratio");
  //   }
  //
  //   /** Test generating the SampledData FHIR complex type. */
  //   @Test
  //   public void generateSampledData() throws Exception {
  //     testGeneratedSTU3Proto("SampledData");
  //   }
  //
  //   /** Test generating the Signature FHIR complex type. */
  //   @Test
  //   public void generateSignature() throws Exception {
  //     testGeneratedSTU3Proto("Signature");
  //   }
  //
  //   /** Test generating the Timing FHIR complex type. */
  //   @Test
  //   public void generateTiming() throws Exception {
  //     testGeneratedSTU3Proto("Timing");
  //   }
  //
  //   // Test the FHIR metadata types individually.
  //
  //   /** Test generating the ContactDetail FHIR metadata type. */
  //   @Test
  //   public void generateContactDetail() throws Exception {
  //     testGeneratedSTU3Proto("ContactDetail");
  //   }
  //
  //   /** Test generating the Contributor FHIR metadata type. */
  //   @Test
  //   public void generateContributor() throws Exception {
  //     testGeneratedSTU3Proto("Contributor");
  //   }
  //
  //   /** Test generating the DataRequirement FHIR metadata type. */
  //   @Test
  //   public void generateDataRequirement() throws Exception {
  //     testGeneratedSTU3Proto("DataRequirement");
  //   }
  //
  //   /** Test generating the ParameterDefinition FHIR metadata type. */
  //   @Test
  //   public void generateParameterDefinition() throws Exception {
  //     testGeneratedSTU3Proto("ParameterDefinition");
  //   }
  //
  //   /** Test generating the RelatedArtifact FHIR metadata type. */
  //   @Test
  //   public void generateRelatedArtifact() throws Exception {
  //     testGeneratedSTU3Proto("RelatedArtifact");
  //   }
  //
  //   /** Test generating the TriggerDefinition FHIR metadata type. */
  //   @Test
  //   public void generateTriggerDefinition() throws Exception {
  //     testGeneratedSTU3Proto("TriggerDefinition");
  //   }
  //
  //   /** Test generating the UsageContext FHIR metadata type. */
  //   @Test
  //   public void generateUsageContext() throws Exception {
  //     testGeneratedSTU3Proto("UsageContext");
  //   }
  //
  //   // Test the FHIR special-purpose data types individually.
  //
  //   /** Test generating the BackboneElement FHIR special-purpose type. */
  //   @Test
  //   public void generateBackboneElement() throws Exception {
  //     testGeneratedSTU3Proto("BackboneElement");
  //   }
  //
  //   /** Test generating the Dosage FHIR special-purpose type. */
  //   @Test
  //   public void generateDosage() throws Exception {
  //     testGeneratedSTU3Proto("Dosage");
  //   }
  //
  //   /** Test generating the Element FHIR special-purpose type. */
  //   @Test
  //   public void generateElement() throws Exception {
  //     testGeneratedSTU3Proto("Element");
  //   }
  //
  //   /** Test generating the ElementDefinition FHIR special-purpose type. */
  //   @Test
  //   public void generateElementDefinition() throws Exception {
  //     testGeneratedSTU3Proto("ElementDefinition");
  //   }
  //
  //   /** Verify the Extension FHIR special-purpose type. */
  //   @Test
  //   public void verifyExtension() throws Exception {
  //     verifyCompiledDescriptor(com.google.fhir.stu3.proto.Extension.getDescriptor());
  //   }
  //
  //   /** Test generating the Meta FHIR special-purpose type. */
  //   @Test
  //   public void generateMeta() throws Exception {
  //     testGeneratedSTU3Proto("Meta");
  //   }
  //
  //   /** Test generating the MetadataResource special-purpose type. */
  //   @Test
  //   public void generateMetadataResource() throws Exception {
  //     testGeneratedSTU3Proto("MetadataResource");
  //   }
  //
  //   /** Test generating the Narrative FHIR special-purpose type. */
  //   @Test
  //   public void generateNarrative() throws Exception {
  //     testGeneratedSTU3Proto("Narrative");
  //   }
  //
  //   /** Verify the Reference FHIR special-purpose type. */
  //   @Test
  //   public void verifyReference() throws Exception {
  //     verifyCompiledDescriptor(com.google.fhir.stu3.proto.Reference.getDescriptor());
  //     verifyCompiledDescriptor(com.google.fhir.stu3.proto.ReferenceId.getDescriptor());
  //   }
  //
  //   /** Test generating the Resource FHIR special-purpose type. */
  //   @Test
  //   public void generateResource() throws Exception {
  //     testGeneratedSTU3Proto("Resource");
  //   }
  //
  //   // Test the FHIR resource types individually.
  //
  //   /** Test generating the Account FHIR resource. */
  //   @Test
  //   public void generateAccount() throws Exception {
  //     testGeneratedSTU3Proto("Account");
  //   }
  //
  //   /** Test generating the ActivityDefinition FHIR resource. */
  //   @Test
  //   public void generateActivityDefinition() throws Exception {
  //     testGeneratedSTU3Proto("ActivityDefinition");
  //   }
  //
  //   /** Test generating the AdverseEvent FHIR resource. */
  //   @Test
  //   public void generateAdverseEvent() throws Exception {
  //     testGeneratedSTU3Proto("AdverseEvent");
  //   }
  //
  //   /** Test generating the AllergyIntolerance FHIR resource. */
  //   @Test
  //   public void generateAllergyIntolerance() throws Exception {
  //     testGeneratedSTU3Proto("AllergyIntolerance");
  //   }
  //
  //   /** Test generating the Appointment FHIR resource. */
  //   @Test
  //   public void generateAppointment() throws Exception {
  //     testGeneratedSTU3Proto("Appointment");
  //   }
  //
  //   /** Test generating the AppointmentResponse FHIR resource. */
  //   @Test
  //   public void generateAppointmentResponse() throws Exception {
  //     testGeneratedSTU3Proto("AppointmentResponse");
  //   }
  //
  //   /** Test generating the AuditEvent FHIR resource. */
  //   @Test
  //   public void generateAuditEvent() throws Exception {
  //     testGeneratedSTU3Proto("AuditEvent");
  //   }
  //
  //   /** Test generating the Basic FHIR resource. */
  //   @Test
  //   public void generateBasic() throws Exception {
  //     testGeneratedSTU3Proto("Basic");
  //   }
  //
  //   /** Test generating the Binary FHIR resource. */
  //   @Test
  //   public void generateBinary() throws Exception {
  //     testGeneratedSTU3Proto("Binary");
  //   }
  //
  //   /** Test generating the BodySite FHIR resource. */
  //   @Test
  //   public void generateBodySite() throws Exception {
  //     testGeneratedSTU3Proto("BodySite");
  //   }
  //
  //   /** Test generating the Bundle FHIR resource. */
  //   @Test
  //   public void generateBundle() throws Exception {
  //     testGeneratedSTU3Proto("Bundle");
  //   }
  //
  //   /** Test generating the CapabilityStatement FHIR resource. */
  //   @Test
  //   public void generateCapabilityStatement() throws Exception {
  //     testGeneratedSTU3Proto("CapabilityStatement");
  //   }
  //
  //   /** Test generating the CarePlan FHIR resource. */
  //   @Test
  //   public void generateCarePlan() throws Exception {
  //     testGeneratedSTU3Proto("CarePlan");
  //   }
  //
  //   /** Test generating the CareTeam FHIR resource. */
  //   @Test
  //   public void generateCareTeam() throws Exception {
  //     testGeneratedSTU3Proto("CareTeam");
  //   }
  //
  //   /** Test generating the ChargeItem FHIR resource. */
  //   @Test
  //   public void generateChargeItem() throws Exception {
  //     testGeneratedSTU3Proto("ChargeItem");
  //   }
  //
  //   /** Test generating the Claim FHIR resource. */
  //   @Test
  //   public void generateClaim() throws Exception {
  //     testGeneratedSTU3Proto("Claim");
  //   }
  //
  //   /** Test generating the ClaimResponse FHIR resource. */
  //   @Test
  //   public void generateClaimResponse() throws Exception {
  //     testGeneratedSTU3Proto("ClaimResponse");
  //   }
  //
  //   /** Test generating the ClinicalImpression FHIR resource. */
  //   @Test
  //   public void generateClinicalImpression() throws Exception {
  //     testGeneratedSTU3Proto("ClinicalImpression");
  //   }
  //
  //   /** Test generating the CodeSystem FHIR resource. */
  //   @Test
  //   public void generateCodeSystem() throws Exception {
  //     testGeneratedSTU3Proto("CodeSystem");
  //   }
  //
  //   /** Test generating the Communication FHIR resource. */
  //   @Test
  //   public void generateCommunication() throws Exception {
  //     testGeneratedSTU3Proto("Communication");
  //   }
  //
  //   /** Test generating the CommunicationRequest FHIR resource. */
  //   @Test
  //   public void generateCommunicationRequest() throws Exception {
  //     testGeneratedSTU3Proto("CommunicationRequest");
  //   }
  //
  //   /** Test generating the CompartmentDefinition FHIR resource. */
  //   @Test
  //   public void generateCompartmentDefinition() throws Exception {
  //     testGeneratedSTU3Proto("CompartmentDefinition");
  //   }
  //
  //   /** Test generating the Composition FHIR resource. */
  //   @Test
  //   public void generateComposition() throws Exception {
  //     testGeneratedSTU3Proto("Composition");
  //   }
  //
  //   /** Test generating the ConceptMap FHIR resource. */
  //   @Test
  //   public void generateConceptMap() throws Exception {
  //     testGeneratedSTU3Proto("ConceptMap");
  //   }
  //
  //   /** Test generating the Condition FHIR resource. */
  //   @Test
  //   public void generateCondition() throws Exception {
  //     testGeneratedSTU3Proto("Condition");
  //   }
  //
  //   /** Test generating the Consent FHIR resource. */
  //   @Test
  //   public void generateConsent() throws Exception {
  //     testGeneratedSTU3Proto("Consent");
  //   }
  //
  //   /** Test generating the Contract FHIR resource. */
  //   @Test
  //   public void generateContract() throws Exception {
  //     testGeneratedSTU3Proto("Contract");
  //   }
  //
  //   /** Test generating the Coverage FHIR resource. */
  //   @Test
  //   public void generateCoverage() throws Exception {
  //     testGeneratedSTU3Proto("Coverage");
  //   }
  //
  //   /** Test generating the DataElement FHIR resource. */
  //   @Test
  //   public void generateDataElement() throws Exception {
  //     testGeneratedSTU3Proto("DataElement");
  //   }
  //
  //   /** Test generating the DetectedIssue FHIR resource. */
  //   @Test
  //   public void generateDetectedIssue() throws Exception {
  //     testGeneratedSTU3Proto("DetectedIssue");
  //   }
  //
  //   /** Test generating the Device FHIR resource. */
  //   @Test
  //   public void generateDevice() throws Exception {
  //     testGeneratedSTU3Proto("Device");
  //   }
  //
  //   /** Test generating the DeviceComponent FHIR resource. */
  //   @Test
  //   public void generateDeviceComponent() throws Exception {
  //     testGeneratedSTU3Proto("DeviceComponent");
  //   }
  //
  //   /** Test generating the DeviceMetric FHIR resource. */
  //   @Test
  //   public void generateDeviceMetric() throws Exception {
  //     testGeneratedSTU3Proto("DeviceMetric");
  //   }
  //
  //   /** Test generating the DeviceRequest FHIR resource. */
  //   @Test
  //   public void generateDeviceRequest() throws Exception {
  //     testGeneratedSTU3Proto("DeviceRequest");
  //   }
  //
  //   /** Test generating the DeviceUseStatement FHIR resource. */
  //   @Test
  //   public void generateDeviceUseStatement() throws Exception {
  //     testGeneratedSTU3Proto("DeviceUseStatement");
  //   }
  //
  //   /** Test generating the DiagnosticReport FHIR resource. */
  //   @Test
  //   public void generateDiagnosticReport() throws Exception {
  //     testGeneratedSTU3Proto("DiagnosticReport");
  //   }
  //
  //   /** Test generating the DocumentManifest FHIR resource. */
  //   @Test
  //   public void generateDocumentManifest() throws Exception {
  //     testGeneratedSTU3Proto("DocumentManifest");
  //   }
  //
  //   /** Test generating the DocumentReference FHIR resource. */
  //   @Test
  //   public void generateDocumentReference() throws Exception {
  //     testGeneratedSTU3Proto("DocumentReference");
  //   }
  //
  //   /** Test generating the EligibilityRequest FHIR resource. */
  //   @Test
  //   public void generateEligibilityRequest() throws Exception {
  //     testGeneratedSTU3Proto("EligibilityRequest");
  //   }
  //
  //   /** Test generating the EligibilityResponse FHIR resource. */
  //   @Test
  //   public void generateEligibilityResponse() throws Exception {
  //     testGeneratedSTU3Proto("EligibilityResponse");
  //   }
  //
  //   /** Test generating the Encounter FHIR resource. */
  //   @Test
  //   public void generateEncounter() throws Exception {
  //     testGeneratedSTU3Proto("Encounter");
  //   }
  //
  //   /** Test generating the Endpoint FHIR resource. */
  //   @Test
  //   public void generateEndpoint() throws Exception {
  //     testGeneratedSTU3Proto("Endpoint");
  //   }
  //
  //   /** Test generating the EnrollmentRequest FHIR resource. */
  //   @Test
  //   public void generateEnrollmentRequest() throws Exception {
  //     testGeneratedSTU3Proto("EnrollmentRequest");
  //   }
  //
  //   /** Test generating the EnrollmentResponse FHIR resource. */
  //   @Test
  //   public void generateEnrollmentResponse() throws Exception {
  //     testGeneratedSTU3Proto("EnrollmentResponse");
  //   }
  //
  //   /** Test generating the EpisodeOfCare FHIR resource. */
  //   @Test
  //   public void generateEpisodeOfCare() throws Exception {
  //     testGeneratedSTU3Proto("EpisodeOfCare");
  //   }
  //
  //   /** Test generating the ExpansionProfile FHIR resource. */
  //   @Test
  //   public void generateExpansionProfile() throws Exception {
  //     testGeneratedSTU3Proto("ExpansionProfile");
  //   }
  //
  //   /** Test generating the ExplanationOfBenefit FHIR resource. */
  //   @Test
  //   public void generateExplanationOfBenefit() throws Exception {
  //     testGeneratedSTU3Proto("ExplanationOfBenefit");
  //   }
  //
  //   /** Test generating the FamilyMemberHistory FHIR resource. */
  //   @Test
  //   public void generateFamilyMemberHistory() throws Exception {
  //     testGeneratedSTU3Proto("FamilyMemberHistory");
  //   }
  //
  //   /** Test generating the Flag FHIR resource. */
  //   @Test
  //   public void generateFlag() throws Exception {
  //     testGeneratedSTU3Proto("Flag");
  //   }
  //
  //   /** Test generating the Goal FHIR resource. */
  //   @Test
  //   public void generateGoal() throws Exception {
  //     testGeneratedSTU3Proto("Goal");
  //   }
  //
  //   /** Test generating the GraphDefinition FHIR resource. */
  //   @Test
  //   public void generateGraphDefinition() throws Exception {
  //     testGeneratedSTU3Proto("GraphDefinition");
  //   }
  //
  //   /** Test generating the Group FHIR resource. */
  //   @Test
  //   public void generateGroup() throws Exception {
  //     testGeneratedSTU3Proto("Group");
  //   }
  //
  //   /** Test generating the GuidanceResponse FHIR resource. */
  //   @Test
  //   public void generateGuidanceResponse() throws Exception {
  //     testGeneratedSTU3Proto("GuidanceResponse");
  //   }
  //
  //   /** Test generating the HealthcareService FHIR resource. */
  //   @Test
  //   public void generateHealthcareService() throws Exception {
  //     testGeneratedSTU3Proto("HealthcareService");
  //   }
  //
  //   /** Test generating the ImagingManifest FHIR resource. */
  //   @Test
  //   public void generateImagingManifest() throws Exception {
  //     testGeneratedSTU3Proto("ImagingManifest");
  //   }
  //
  //   /** Test generating the ImagingStudy FHIR resource. */
  //   @Test
  //   public void generateImagingStudy() throws Exception {
  //     testGeneratedSTU3Proto("ImagingStudy");
  //   }
  //
  //   /** Test generating the Immunization FHIR resource. */
  //   @Test
  //   public void generateImmunization() throws Exception {
  //     testGeneratedSTU3Proto("Immunization");
  //   }
  //
  //   /** Test generating the ImmunizationRecommendation FHIR resource. */
  //   @Test
  //   public void generateImmunizationRecommendation() throws Exception {
  //     testGeneratedSTU3Proto("ImmunizationRecommendation");
  //   }
  //
  //   /** Test generating the ImplementationGuide FHIR resource. */
  //   @Test
  //   public void generateImplementationGuide() throws Exception {
  //     testGeneratedSTU3Proto("ImplementationGuide");
  //   }
  //
  //   /** Test generating the Library FHIR resource. */
  //   @Test
  //   public void generateLibrary() throws Exception {
  //     testGeneratedSTU3Proto("Library");
  //   }
  //
  //   /** Test generating the Linkage FHIR resource. */
  //   @Test
  //   public void generateLinkage() throws Exception {
  //     testGeneratedSTU3Proto("Linkage");
  //   }
  //
  //   /** Test generating the List FHIR resource. */
  //   @Test
  //   public void generateList() throws Exception {
  //     testGeneratedSTU3Proto("List");
  //   }
  //
  //   /** Test generating the Location FHIR resource. */
  //   @Test
  //   public void generateLocation() throws Exception {
  //     testGeneratedSTU3Proto("Location");
  //   }
  //
  //   /** Test generating the Measure FHIR resource. */
  //   @Test
  //   public void generateMeasure() throws Exception {
  //     testGeneratedSTU3Proto("Measure");
  //   }
  //
  //   /** Test generating the MeasureReport FHIR resource. */
  //   @Test
  //   public void generateMeasureReport() throws Exception {
  //     testGeneratedSTU3Proto("MeasureReport");
  //   }
  //
  //   /** Test generating the Media FHIR resource. */
  //   @Test
  //   public void generateMedia() throws Exception {
  //     testGeneratedSTU3Proto("Media");
  //   }
  //
  //   /** Test generating the Medication FHIR resource. */
  //   @Test
  //   public void generateMedication() throws Exception {
  //     testGeneratedSTU3Proto("Medication");
  //   }
  //
  //   /** Test generating the MedicationAdministration FHIR resource. */
  //   @Test
  //   public void generateMedicationAdministration() throws Exception {
  //     testGeneratedSTU3Proto("MedicationAdministration");
  //   }
  //
  //   /** Test generating the MedicationDispense FHIR resource. */
  //   @Test
  //   public void generateMedicationDispense() throws Exception {
  //     testGeneratedSTU3Proto("MedicationDispense");
  //   }
  //
  //   /** Test generating the MedicationRequest FHIR resource. */
  //   @Test
  //   public void generateMedicationRequest() throws Exception {
  //     testGeneratedSTU3Proto("MedicationRequest");
  //   }
  //
  //   /** Test generating the MedicationStatement FHIR resource. */
  //   @Test
  //   public void generateMedicationStatement() throws Exception {
  //     testGeneratedSTU3Proto("MedicationStatement");
  //   }
  //
  //   /** Test generating the MessageDefinition FHIR resource. */
  //   @Test
  //   public void generateMessageDefinition() throws Exception {
  //     testGeneratedSTU3Proto("MessageDefinition");
  //   }
  //
  //   /** Test generating the MessageHeader FHIR resource. */
  //   @Test
  //   public void generateMessageHeader() throws Exception {
  //     testGeneratedSTU3Proto("MessageHeader");
  //   }
  //
  //   /** Test generating the NamingSystem FHIR resource. */
  //   @Test
  //   public void generateNamingSystem() throws Exception {
  //     testGeneratedSTU3Proto("NamingSystem");
  //   }
  //
  //   /** Test generating the NutritionOrder FHIR resource. */
  //   @Test
  //   public void generateNutritionOrder() throws Exception {
  //     testGeneratedSTU3Proto("NutritionOrder");
  //   }
  //
  //   /** Test generating the Observation FHIR resource. */
  //   @Test
  //   public void generateObservation() throws Exception {
  //     testGeneratedSTU3Proto("Observation");
  //   }
  //
  //   /** Test generating the OperationDefinition FHIR resource. */
  //   @Test
  //   public void generateOperationDefinition() throws Exception {
  //     testGeneratedSTU3Proto("OperationDefinition");
  //   }
  //
  //   /** Test generating the OperationOutcome FHIR resource. */
  //   @Test
  //   public void generateOperationOutcome() throws Exception {
  //     testGeneratedSTU3Proto("OperationOutcome");
  //   }
  //
  //   /** Test generating the Organization FHIR resource. */
  //   @Test
  //   public void generateOrganization() throws Exception {
  //     testGeneratedSTU3Proto("Organization");
  //   }
  //
  //   /** Test generating the Parameters FHIR resource. */
  //   @Test
  //   public void generateParameters() throws Exception {
  //     testGeneratedSTU3Proto("Parameters");
  //   }
  //
  //   /** Test generating the Patient FHIR resource. */
  //   @Test
  //   public void generatePatient() throws Exception {
  //     testGeneratedSTU3Proto("Patient");
  //   }
  //
  //   /** Test generating the PaymentNotice FHIR resource. */
  //   @Test
  //   public void generatePaymentNotice() throws Exception {
  //     testGeneratedSTU3Proto("PaymentNotice");
  //   }
  //
  //   /** Test generating the PaymentReconciliation FHIR resource. */
  //   @Test
  //   public void generatePaymentReconciliation() throws Exception {
  //     testGeneratedSTU3Proto("PaymentReconciliation");
  //   }
  //
  //   /** Test generating the Person FHIR resource. */
  //   @Test
  //   public void generatePerson() throws Exception {
  //     testGeneratedSTU3Proto("Person");
  //   }
  //
  //   /** Test generating the PlanDefinition FHIR resource. */
  //   @Test
  //   public void generatePlanDefinition() throws Exception {
  //     testGeneratedSTU3Proto("PlanDefinition");
  //   }
  //
  //   /** Test generating the Practitioner FHIR resource. */
  //   @Test
  //   public void generatePractitioner() throws Exception {
  //     testGeneratedSTU3Proto("Practitioner");
  //   }
  //
  //   /** Test generating the PractitionerRole FHIR resource. */
  //   @Test
  //   public void generatePractitionerRole() throws Exception {
  //     testGeneratedSTU3Proto("PractitionerRole");
  //   }
  //
  //   /** Test generating the Procedure FHIR resource. */
  //   @Test
  //   public void generateProcedure() throws Exception {
  //     testGeneratedSTU3Proto("Procedure");
  //   }
  //
  //   /** Test generating the ProcedureRequest FHIR resource. */
  //   @Test
  //   public void generateProcedureRequest() throws Exception {
  //     testGeneratedSTU3Proto("ProcedureRequest");
  //   }
  //
  //   /** Test generating the ProcessRequest FHIR resource. */
  //   @Test
  //   public void generateProcessRequest() throws Exception {
  //     testGeneratedSTU3Proto("ProcessRequest");
  //   }
  //
  //   /** Test generating the ProcessResponse FHIR resource. */
  //   @Test
  //   public void generateProcessResponse() throws Exception {
  //     testGeneratedSTU3Proto("ProcessResponse");
  //   }
  //
  //   /** Test generating the Provenance FHIR resource. */
  //   @Test
  //   public void generateProvenance() throws Exception {
  //     testGeneratedSTU3Proto("Provenance");
  //   }
  //
  //   /** Test generating the Questionnaire FHIR resource. */
  //   @Test
  //   public void generateQuestionnaire() throws Exception {
  //     testGeneratedSTU3Proto("Questionnaire");
  //   }
  //
  //   /** Test generating the QuestionnaireResponse FHIR resource. */
  //   @Test
  //   public void generateQuestionnaireResponse() throws Exception {
  //     testGeneratedSTU3Proto("QuestionnaireResponse");
  //   }
  //
  //   /** Test generating the ReferralRequest FHIR resource. */
  //   @Test
  //   public void generateReferralRequest() throws Exception {
  //     testGeneratedSTU3Proto("ReferralRequest");
  //   }
  //
  //   /** Test generating the RelatedPerson FHIR resource. */
  //   @Test
  //   public void generateRelatedPerson() throws Exception {
  //     testGeneratedSTU3Proto("RelatedPerson");
  //   }
  //
  //   /** Test generating the RequestGroup FHIR resource. */
  //   @Test
  //   public void generateRequestGroup() throws Exception {
  //     testGeneratedSTU3Proto("RequestGroup");
  //   }
  //
  //   /** Test generating the ResearchStudy FHIR resource. */
  //   @Test
  //   public void generateResearchStudy() throws Exception {
  //     testGeneratedSTU3Proto("ResearchStudy");
  //   }
  //
  //   /** Test generating the ResearchSubject FHIR resource. */
  //   @Test
  //   public void generateResearchSubject() throws Exception {
  //     testGeneratedSTU3Proto("ResearchSubject");
  //   }
  //
  //   /** Test generating the RiskAssessment FHIR resource. */
  //   @Test
  //   public void generateRiskAssessment() throws Exception {
  //     testGeneratedSTU3Proto("RiskAssessment");
  //   }
  //
  //   /** Test generating the Schedule FHIR resource. */
  //   @Test
  //   public void generateSchedule() throws Exception {
  //     testGeneratedSTU3Proto("Schedule");
  //   }
  //
  //   /** Test generating the SearchParameter FHIR resource. */
  //   @Test
  //   public void generateSearchParameter() throws Exception {
  //     testGeneratedSTU3Proto("SearchParameter");
  //   }
  //
  //   /** Test generating the Sequence FHIR resource. */
  //   @Test
  //   public void generateSequence() throws Exception {
  //     testGeneratedSTU3Proto("Sequence");
  //   }
  //
  //   /** Test generating the ServiceDefinition FHIR resource. */
  //   @Test
  //   public void generateServiceDefinition() throws Exception {
  //     testGeneratedSTU3Proto("ServiceDefinition");
  //   }
  //
  //   /** Test generating the Slot FHIR resource. */
  //   @Test
  //   public void generateSlot() throws Exception {
  //     testGeneratedSTU3Proto("Slot");
  //   }
  //
  //   /** Test generating the Specimen FHIR resource. */
  //   @Test
  //   public void generateSpecimen() throws Exception {
  //     testGeneratedSTU3Proto("Specimen");
  //   }
  //
  //   /** Test generating the StructureDefinition FHIR resource. */
  //   @Test
  //   public void generateStructureDefinition() throws Exception {
  //     testModifiedGeneratedSTU3Proto("StructureDefinition");
  //   }
  //
  //   /** Test generating the StructureMap FHIR resource. */
  //   @Test
  //   public void generateStructureMap() throws Exception {
  //     testGeneratedSTU3Proto("StructureMap");
  //   }
  //
  //   /** Test generating the Subscription FHIR resource. */
  //   @Test
  //   public void generateSubscription() throws Exception {
  //     testGeneratedSTU3Proto("Subscription");
  //   }
  //
  //   /** Test generating the Substance FHIR resource. */
  //   @Test
  //   public void generateSubstance() throws Exception {
  //     testGeneratedSTU3Proto("Substance");
  //   }
  //
  //   /** Test generating the SupplyDelivery FHIR resource. */
  //   @Test
  //   public void generateSupplyDelivery() throws Exception {
  //     testGeneratedSTU3Proto("SupplyDelivery");
  //   }
  //
  //   /** Test generating the SupplyRequest FHIR resource. */
  //   @Test
  //   public void generateSupplyRequest() throws Exception {
  //     testGeneratedSTU3Proto("SupplyRequest");
  //   }
  //
  //   /** Test generating the Task FHIR resource. */
  //   @Test
  //   public void generateTask() throws Exception {
  //     testGeneratedSTU3Proto("Task");
  //   }
  //
  //   /** Test generating the TestReport FHIR resource. */
  //   @Test
  //   public void generateTestReport() throws Exception {
  //     testGeneratedSTU3Proto("TestReport");
  //   }
  //
  //   /** Test generating the TestScript FHIR resource. */
  //   @Test
  //   public void generateTestScript() throws Exception {
  //     testGeneratedSTU3Proto("TestScript");
  //   }
  //
  //   /** Test generating the ValueSet FHIR resource. */
  //   @Test
  //   public void generateValueSet() throws Exception {
  //     testGeneratedSTU3Proto("ValueSet");
  //   }
  //
  //   /** Test generating the VisionPrescription FHIR resource. */
  //   @Test
  //   public void generateVisionPrescription() throws Exception {
  //     testGeneratedSTU3Proto("VisionPrescription");
  //   }
  //
  //   // Test generating profiles.
  //
  //   /** Test generating the bmi profile. */
  //   @Test
  //   public void generateBmi() throws Exception {
  //     testGeneratedSTU3Proto("bmi");
  //   }
  //
  //   /** Test generating the bodyheight profile. */
  //   @Test
  //   public void generateBodyheight() throws Exception {
  //     testGeneratedSTU3Proto("bodyheight");
  //   }
  //
  //   /** Test generating the bodylength profile. */
  //   @Test
  //   public void generateBodylength() throws Exception {
  //     testGeneratedSTU3Proto("bodylength");
  //   }
  //
  //   /** Test generating the bodytemp profile. */
  //   @Test
  //   public void generateBodytemp() throws Exception {
  //     testGeneratedSTU3Proto("bodytemp");
  //   }
  //
  //   /** Test generating the bodyweight profile. */
  //   @Test
  //   public void generateBodyweight() throws Exception {
  //     testGeneratedSTU3Proto("bodyweight");
  //   }
  //
  //   /** Test generating the bp profile. */
  //   @Test
  //   public void generateBp() throws Exception {
  //     testGeneratedSTU3Proto("bp");
  //   }
  //
  //   /** Test generating the cholesterol profile. */
  //   @Test
  //   public void generateCholesterol() throws Exception {
  //     testGeneratedSTU3Proto("cholesterol");
  //   }
  //
  //   /** Test generating the clinicaldocument profile. */
  //   @Test
  //   public void generateClinicaldocument() throws Exception {
  //     testGeneratedSTU3Proto("clinicaldocument");
  //   }
  //
  //   /** Test generating the consentdirective profile. */
  //   // Note: consentdirective is malformed.
  //   // TODO: reenable.
  //   // @Test
  //   // public void generateConsentdirective() throws Exception {
  //   //   testGeneratedSTU3Proto("consentdirective");
  //   // }
  //
  //   /** Test generating the devicemetricobservation profile. */
  //   @Test
  //   public void generateDevicemetricobservation() throws Exception {
  //     testGeneratedSTU3Proto("devicemetricobservation");
  //   }
  //
  //   /** Test generating the diagnosticreport-genetics profile. */
  //   @Test
  //   public void generateDiagnosticreportGenetics() throws Exception {
  //     testGeneratedSTU3Proto("diagnosticreport-genetics");
  //   }
  //
  //   /** Test generating the elementdefinition-de profile. */
  //   @Test
  //   public void generateElementdefinitionDe() throws Exception {
  //     testGeneratedSTU3Proto("elementdefinition-de");
  //   }
  //
  //   /** Test generating the hdlcholesterol profile. */
  //   @Test
  //   public void generateHdlcholesterol() throws Exception {
  //     testGeneratedSTU3Proto("hdlcholesterol");
  //   }
  //
  //   /** Test generating the headcircum profile. */
  //   @Test
  //   public void generateHeadcircum() throws Exception {
  //     testGeneratedSTU3Proto("headcircum");
  //   }
  //
  //   /** Test generating the heartrate profile. */
  //   @Test
  //   public void generateHeartrate() throws Exception {
  //     testGeneratedSTU3Proto("heartrate");
  //   }
  //
  //   /** Test generating the hlaresult profile. */
  //   @Test
  //   public void generateHlaresult() throws Exception {
  //     testGeneratedSTU3Proto("hlaresult");
  //   }
  //
  //   /** Test generating the ldlcholesterol profile. */
  //   @Test
  //   public void generateLdlcholesterol() throws Exception {
  //     testGeneratedSTU3Proto("ldlcholesterol");
  //   }
  //
  //   /** Test generating the lipidprofile profile. */
  //   @Test
  //   public void generateLipidprofile() throws Exception {
  //     testGeneratedSTU3Proto("lipidprofile");
  //   }
  //
  //   /** Test generating the observation-genetics profile. */
  //   @Test
  //   public void generateObservationGenetics() throws Exception {
  //     testGeneratedSTU3Proto("observation-genetics");
  //   }
  //
  //   /** Test generating the oxygensat profile. */
  //   @Test
  //   public void generateOxygensat() throws Exception {
  //     testGeneratedSTU3Proto("oxygensat");
  //   }
  //
  //   /** Test generating the procedurerequest-genetics profile. */
  //   @Test
  //   public void generateProcedurerequestGenetics() throws Exception {
  //     testGeneratedSTU3Proto("procedurerequest-genetics");
  //   }
  //
  //   /** Test generating the resprate profile. */
  //   @Test
  //   public void generateResprate() throws Exception {
  //     testGeneratedSTU3Proto("resprate");
  //   }
  //
  //   /** Test generating the shareablecodesystem profile. */
  //   @Test
  //   public void generateShareablecodesystem() throws Exception {
  //     testGeneratedSTU3Proto("shareablecodesystem");
  //   }
  //
  //   /** Test generating the shareablevalueset profile. */
  //   @Test
  //   public void generateShareablevalueset() throws Exception {
  //     testGeneratedSTU3Proto("shareablevalueset");
  //   }
  //
  //   /** Test generating the simplequantity profile. */
  //   @Test
  //   public void generateSimplequantity() throws Exception {
  //     testGeneratedSTU3Proto("SimpleQuantity");
  //   }
  //
  //   /** Test generating the triglyceride profile. */
  //   @Test
  //   public void generateTriglyceride() throws Exception {
  //     testGeneratedSTU3Proto("triglyceride");
  //   }
  //
  //   /** Test generating the uuid profile. */
  //   @Test
  //   public void generateUuid() throws Exception {
  //     testGeneratedSTU3Proto("uuid");
  //   }
  //
  //   /** Test generating the vitalsigns profile. */
  //   @Test
  //   public void generateVitalsigns() throws Exception {
  //     testGeneratedSTU3Proto("vitalsigns");
  //   }
  //
  //   /** Test generating the vitalspanel profile. */
  //   @Test
  //   public void generateVitalspanel() throws Exception {
  //     testGeneratedSTU3Proto("vitalspanel");
  //   }
  //
  //   /** Test generating the xhtml profile. */
  //   @Test
  //   public void generateXhtml() throws Exception {
  //     testGeneratedSTU3Proto("xhtml");
  //   }
  //
  //   // Test generating extensions.
  //
  //   /** Test generating the elementdefinition-bindingname extension. */
  //   @Test
  //   public void generateElementDefinitionBindingName() throws Exception {
  //     testExtension("elementdefinition-bindingName");
  //   }
  //
  //   /** Test generating the structuredefinition-explicit-type-name extension. */
  //   @Test
  //   public void generateElementDefinitionExplicitTypeName() throws Exception {
  //     testExtension("structuredefinition-explicit-type-name");
  //   }
  //
  //   /** Test generating the structuredefinition-regex extension. */
  //   @Test
  //   public void generateElementDefinitionRegex() throws Exception {
  //     testExtension("structuredefinition-regex");
  //   }
  //
  //   /** Test generating the patient-clinicaltrial extension. */
  //   @Test
  //   public void generatePatientClinicalTrial() throws Exception {
  //     testExtension("patient-clinicalTrial");
  //   }
  //
  //   /** Test generating the elementdefinition-allowedunits extension. */
  //   @Test
  //   public void generateElementDefinitionAllowedUnits() throws Exception {
  //     testExtension("elementdefinition-allowedUnits");
  //   }
  //
  //   /** Test generating the codesystem-history extension. */
  //   @Test
  //   public void generateCodesystemHistory() throws Exception {
  //     testExtension("codesystem-history");
  //   }
  //
  //   /** Test generating the timing-daysofcycle extension. */
  //   @Test
  //   public void generateTimingDaysofcycle() throws Exception {
  //     testExtension("timing-daysOfCycle");
  //   }
  //
  //   /** Test generating the timing-daysofcycle extension. */
  //   @Test
  //   public void generateTranslate() throws Exception {
  //     testExtension("translation");
  //   }

  private void testGeneratedR4Proto(ProtoGenerator protoGenerator, String resourceName)
      throws IOException {
    StructureDefinition resource = readStructureDefinition(resourceName, FhirVersion.R4);
    DescriptorProto generatedProto = protoGenerator.generateProto(resource);
    DescriptorProto golden = readDescriptorProto(resourceName, FhirVersion.R4);
    if (!generatedProto.equals(golden)) {
      System.out.println("Failed on: " + resourceName);
      assertThat(generatedProto).isEqualTo(golden);
    }
  }

  static ProtoGenerator makeR4ProtoGenerator(String definitionZip) throws IOException {
    FhirPackage fhirPackage = FhirPackage.load(definitionZip);

    return new ProtoGenerator(
        fhirPackage.packageInfo,
        ImmutableSet.of(fhirPackage),
        new ValueSetGenerator(fhirPackage.packageInfo, ImmutableSet.of(fhirPackage)));
  }

  private static final int EXPECTED_R4_COUNT = 647;

  /** Test generating R4 proto files. */
  @Test
  public void generateR4() throws Exception {
    ProtoGenerator protoGenerator =
        makeR4ProtoGenerator("spec/fhir_r4_package.zip");
    String suffix = ".descriptor.prototxt";
    int fileCount = 0;
    for (File file :
        new File(runfiles.rlocation("com_google_fhir/testdata/r4/descriptors/"))
            .listFiles((listDir, name) -> name.endsWith(suffix))) {
      String resourceName = file.getName().substring(0, file.getName().indexOf(suffix));
      testGeneratedR4Proto(protoGenerator, resourceName);
      fileCount++;
    }
    if (fileCount != EXPECTED_R4_COUNT) {
      fail("Expected " + EXPECTED_R4_COUNT + " R4 descriptors to test, but found " + fileCount);
    }
  }
}
