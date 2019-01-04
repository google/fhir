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

import static com.google.common.truth.Truth.assertWithMessage;

import com.google.common.base.CaseFormat;
import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Iterators;
import com.google.common.collect.PeekingIterator;
import com.google.common.io.Files;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.common.FhirVersion;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.stu3.proto.Annotations;
import com.google.fhir.stu3.proto.ContactDetail;
import com.google.fhir.stu3.proto.ContainedResource;
import com.google.fhir.stu3.proto.Extension;
import com.google.fhir.stu3.proto.StructureDefinition;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.SortedMap;
import java.util.TreeMap;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link ProtoFilePrinter}. */
@RunWith(JUnit4.class)
public final class ProtoFilePrinterTest {

  private JsonFormat.Parser jsonParser;
  private ProtoGenerator protoGenerator;
  private ProtoFilePrinter protoPrinter;
  private Runfiles runfiles;

  private static Map<StructureDefinition, String> knownStructDefs = null;

  /** Read and parse the specified StructureDefinition. */
  private StructureDefinition readStructureDefinition(String resourceName) throws IOException {
    File file =
        new File(
            runfiles.rlocation(
                "com_google_fhir/spec/hl7.fhir.core/3.0.1/modified/StructureDefinition-"
                    + resourceName
                    + ".json"));
    if (!file.exists()) {
      file =
          new File(
              runfiles.rlocation(
                  "com_google_fhir/spec/hl7.fhir.core/3.0.1/package/StructureDefinition-"
                      + resourceName
                      + ".json"));
    }
    if (!file.exists()) {
      String lowerCased = resourceName.substring(0, 1).toLowerCase() + resourceName.substring(1);
      file =
          new File(
              runfiles.rlocation(
                  "com_google_fhir/spec/hl7.fhir.core/3.0.1/package/StructureDefinition-"
                      + lowerCased
                      + ".json"));
    }
    String json = Files.asCharSource(file, StandardCharsets.UTF_8).read();
    StructureDefinition.Builder builder = StructureDefinition.newBuilder();
    jsonParser.merge(json, builder);
    return builder.build();
  }

  public Map<StructureDefinition, String> getKnownStructDefs() throws IOException {
    if (knownStructDefs != null) {
      return knownStructDefs;
    }
    // Note: consentdirective is malformed.
    FilenameFilter jsonFilter =
        (dir, name) ->
            name.startsWith("StructureDefinition-")
                && name.endsWith(".json")
                && !name.endsWith("consentdirective.profile.json");
    List<File> structDefs = new ArrayList<>();
    Collections.addAll(
        structDefs,
        new File(runfiles.rlocation("com_google_fhir/spec/hl7.fhir.core/3.0.1/package/"))
            .listFiles(jsonFilter));
    knownStructDefs = new HashMap<>();
    for (File file : structDefs) {
      String json = Files.asCharSource(file, StandardCharsets.UTF_8).read();
      StructureDefinition.Builder builder = StructureDefinition.newBuilder();
      jsonParser.merge(json, builder);
      knownStructDefs.put(builder.build(), "google.fhir.stu3.proto");
    }
    return knownStructDefs;
  }

  /**
   * Read the expected golden output for a specific message, either from the .proto file, or from a
   * file in the testdata directory.
   */
  private String readGolden(String messageName) throws IOException {
    String filename = CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, messageName);

    // Use the actual .proto file as golden.
    File file =
        new File(runfiles.rlocation("com_google_fhir/proto/stu3/" + filename + ".proto"));
    return Files.asCharSource(file, StandardCharsets.UTF_8).read();
  }

  /** Collapse comments spread across multiple lines into single lines. */
  private SortedMap<java.lang.Integer, String> collapseComments(
      SortedMap<java.lang.Integer, String> input) {
    TreeMap<java.lang.Integer, String> result = new TreeMap<>();
    PeekingIterator<Map.Entry<java.lang.Integer, String>> iter =
        Iterators.peekingIterator(input.entrySet().iterator());
    while (iter.hasNext()) {
      Map.Entry<java.lang.Integer, String> current = iter.next();
      result.put(current.getKey(), current.getValue().trim());
      if (current.getValue().trim().startsWith("//")) {
        while (iter.hasNext() && iter.peek().getValue().trim().startsWith("//")) {
          // Merge.
          result.put(
              current.getKey(),
              result.get(current.getKey()) + iter.next().getValue().trim().substring(2));
        }
      }
    }
    return result;
  }

  /**
   * Collapse statements spread across multiple lines into single lines. These statements are
   * typically field definitions, along with annotations, and may be well over the maximum line
   * length allowed by the style guide.
   */
  private SortedMap<java.lang.Integer, String> collapseStatements(
      SortedMap<java.lang.Integer, String> input) {
    TreeMap<java.lang.Integer, String> result = new TreeMap<>();
    Iterator<Map.Entry<java.lang.Integer, String>> iter = input.entrySet().iterator();
    while (iter.hasNext()) {
      Map.Entry<java.lang.Integer, String> current = iter.next();
      String value = current.getValue();
      if (!value.trim().isEmpty() && !value.trim().startsWith("//")) {
        // Merge until we see the closing ';'
        while (!value.endsWith(";") && iter.hasNext()) {
          String next = iter.next().getValue().trim();
          if (!next.isEmpty()) {
            value = value + (value.endsWith("[") || next.startsWith("]") ? "" : " ") + next;
          }
        }
      }
      if (value.contains("End of auto-generated messages.")) {
        return result;
      }
      value = value.replaceAll("\\s\\s+", " ");
      if (!value.isEmpty()) {
        result.put(current.getKey(), value);
      }
    }
    return result;
  }

  private SortedMap<java.lang.Integer, String> splitIntoLines(String text) {
    TreeMap<java.lang.Integer, String> result = new TreeMap<>();
    for (String line : Splitter.on('\n').split(text)) {
      result.put(result.size() + 1, line);
    }
    return result;
  }

  /**
   * Compare two .proto files, line by line, ignoring differences that may have been caused by
   * clang-format.
   */
  private void assertEqualsIgnoreClangFormat(String golden, String test) {
    Iterator<Map.Entry<java.lang.Integer, String>> goldenIter =
        collapseStatements(collapseComments(splitIntoLines(golden))).entrySet().iterator();
    Iterator<Map.Entry<java.lang.Integer, String>> testIter =
        collapseStatements(collapseComments(splitIntoLines(test))).entrySet().iterator();
    while (goldenIter.hasNext() && testIter.hasNext()) {
      Map.Entry<java.lang.Integer, String> goldenEntry = goldenIter.next();
      Map.Entry<java.lang.Integer, String> testEntry = testIter.next();
      assertWithMessage(
              "Test line "
                  + testEntry.getKey()
                  + " does not match golden line "
                  + goldenEntry.getKey())
          .that(testEntry.getValue())
          .isEqualTo(goldenEntry.getValue());
    }
  }

  private static final ImmutableSet<String> TYPES_TO_IGNORE =
      ImmutableSet.of(
          "Extension", "Reference", "ReferenceId", "CodingWithFixedCode", "CodingWithFixedSystem");

  private List<StructureDefinition> getResourcesInFile(FileDescriptor compiled) throws IOException {
    List<StructureDefinition> resourceDefinitions = new ArrayList<>();
    for (Descriptor message : compiled.getMessageTypes()) {
      if (!TYPES_TO_IGNORE.contains(message.getName())
          && !message.getOptions().hasExtension(Annotations.fhirValuesetUrl)) {
        resourceDefinitions.add(readStructureDefinition(message.getName()));
      }
    }
    return resourceDefinitions;
  }

  @Before
  public void setUp() throws IOException {
    String packageName = "google.fhir.stu3.proto";
    jsonParser = JsonFormat.getParser();
    runfiles = Runfiles.create();
    protoGenerator =
        new ProtoGenerator(
            PackageInfo.newBuilder()
                .setProtoPackage(packageName)
                .setJavaProtoPackage("com.google.fhir.stu3.proto")
                .build(),
            "proto/stu3",
            FhirVersion.STU3,
            getKnownStructDefs());
    protoPrinter = new ProtoFilePrinter().withApacheLicense();
  }

  // TODO: Test the FHIR code types.

  /** Test generating datatypes.proto. */
  @Test
  public void generateDataTypes() throws Exception {
    List<StructureDefinition> resourceDefinitions =
        getResourcesInFile(Extension.getDescriptor().getFile());
    FileDescriptorProto descriptor = protoGenerator.generateFileDescriptor(resourceDefinitions);
    String generated = protoPrinter.print(descriptor);
    String golden = readGolden("datatypes");
    assertEqualsIgnoreClangFormat(golden, generated);
  }

  /** Test generating metadatatypes.proto. */
  @Test
  public void generateMetadataTypes() throws Exception {
    List<StructureDefinition> resourceDefinitions =
        getResourcesInFile(ContactDetail.getDescriptor().getFile());
    FileDescriptorProto descriptor = protoGenerator.generateFileDescriptor(resourceDefinitions);
    String generated = protoPrinter.print(descriptor);
    String golden = readGolden("metadatatypes");
    assertEqualsIgnoreClangFormat(golden, generated);
  }

  /** Test generating resources.proto. */
  @Test
  public void generateResources() throws Exception {
    List<StructureDefinition> resourceDefinitions = new ArrayList<>();
    for (FieldDescriptor resource : ContainedResource.getDescriptor().getFields()) {
      resourceDefinitions.add(readStructureDefinition(resource.getMessageType().getName()));
    }
    // DomainResource is not a contained resource.
    resourceDefinitions.add(readStructureDefinition("DomainResource"));
    FileDescriptorProto descriptor = protoGenerator.generateFileDescriptor(resourceDefinitions);
    descriptor = protoGenerator.addContainedResource(descriptor);
    String generated = protoPrinter.print(descriptor);
    String golden = readGolden("resources");
    assertEqualsIgnoreClangFormat(golden, generated);
  }

  /** Test generating extensions.proto. */
  @Test
  public void generateElementDefinitionExtensions() throws Exception {
    List<StructureDefinition> extensionDefinitions = new ArrayList<>();
    for (String extensionName : EXTENSIONS) {
      extensionDefinitions.add(readStructureDefinition(extensionName));
    }
    FileDescriptorProto descriptor = protoGenerator.generateFileDescriptor(extensionDefinitions);
    String generated = protoPrinter.print(descriptor);
    String golden = readGolden("extensions");
    assertEqualsIgnoreClangFormat(golden, generated);
  }

  // Extension structure definitions are named in the spec via use of an Ouija board, so rather than
  // trying to deduce, we list them explictly.
  private static final String[] EXTENSIONS = {
    "11179-de-administrative-status",
    "11179-de-change-description",
    "11179-de-classification-or-context",
    "11179-de-contact-address",
    "11179-de-document-reference",
    "11179-de-effective-period",
    "11179-de-is-data-element-concept",
    "11179-de-registry-org",
    "11179-de-submitter-org",
    "11179-objectClass",
    "11179-objectClassProperty",
    "11179-permitted-value-conceptmap",
    "11179-permitted-value-valueset",
    "allergyintolerance-certainty",
    "allergyintolerance-duration",
    "allergyintolerance-reasonRefuted",
    "allergyintolerance-resolutionAge",
    "allergyintolerance-substanceExposureRisk",
    "auditevent-Accession",
    "auditevent-Anonymized",
    "auditevent-Encrypted",
    "auditevent-Instance",
    "auditevent-MPPS",
    "auditevent-NumberOfInstances",
    "auditevent-ParticipantObjectContainsStudy",
    "auditevent-SOPClass",
    "birthPlace",
    "body-site-instance",
    "capabilitystatement-expectation",
    "capabilitystatement-prohibited",
    "capabilitystatement-search-parameter-combination",
    "capabilitystatement-supported-system",
    "capabilitystatement-websocket",
    "careplan-activity-title",
    "codesystem-author",
    "codesystem-comment",
    "codesystem-comments",
    "codesystem-conceptOrder",
    "codesystem-deprecated",
    "codesystem-effectiveDate",
    "codesystem-expirationDate",
    "codesystem-history",
    "codesystem-keyWord",
    "codesystem-label",
    "codesystem-map",
    "codesystem-ordinalValue",
    "codesystem-otherName",
    "codesystem-reference",
    "codesystem-replacedby",
    "codesystem-sourceReference",
    "codesystem-subsumes",
    "codesystem-trusted-expansion",
    "codesystem-usage",
    "codesystem-warning",
    "codesystem-workflowStatus",
    "coding-sctdescid",
    "communication-media",
    "communication-reasonNotPerformed",
    "communicationrequest-definition",
    "communicationrequest-orderedBy",
    "communicationrequest-reasonRejected",
    "communicationrequest-relevantHistory",
    "communicationrequest-supportingInfo",
    "composition-clindoc-otherConfidentiality",
    "concept-bidirectional",
    "condition-basedOn",
    "condition-criticality",
    "condition-definition",
    "condition-dueTo",
    "condition-occurredFollowing",
    "condition-outcome",
    "condition-partOf",
    "condition-ruledOut",
    "condition-targetBodySite",
    "consent-location",
    "consent-NotificationEndpoint",
    "consent-Witness",
    "cqif-calculatedValue",
    "cqif-citation",
    "cqif-condition",
    "cqif-cqlExpression",
    "cqif-fhirPathExpression",
    "cqif-guidanceencounterClass",
    "cqif-guidanceencounterType",
    "cqif-guidanceinitiatingOrganization",
    "cqif-guidanceinitiatingPerson",
    "cqif-guidancereceivingOrganization",
    "cqif-guidancereceivingPerson",
    "cqif-guidancerecipientLanguage",
    "cqif-guidancerecipientType",
    "cqif-guidancesystemUserLanguage",
    "cqif-guidancesystemUserTaskContext",
    "cqif-guidancesystemUserType",
    "cqif-initialValue",
    "cqif-library",
    "cqif-measureInfo",
    "cqif-optionCode",
    "cqif-qualityOfEvidence",
    "cqif-sourceValueSet",
    "cqif-strengthOfRecommendation",
    "data-absent-reason",
    "datadictionary",
    "device-din",
    "device-implant-status",
    "devicerequest-patientInstruction",
    "devicerequest-reasonRejected",
    "diagnosticReport-addendumOf",
    "diagnosticReport-extends",
    "DiagnosticReport-geneticsAnalysis",
    "DiagnosticReport-geneticsAssessedCondition",
    "DiagnosticReport-geneticsFamilyMemberHistory",
    "diagnosticReport-locationPerformed",
    "diagnosticReport-replaces",
    "diagnosticReport-summaryOf",
    "elementdefinition-allowedUnits",
    "elementdefinition-bestpractice",
    "elementdefinition-bindingName",
    "elementdefinition-equivalence",
    "elementdefinition-identifier",
    "elementdefinition-inheritedExtensibleValueSet",
    "elementdefinition-isCommonBinding",
    "elementdefinition-maxValueSet",
    "elementdefinition-minValueSet",
    "elementdefinition-namespace",
    "elementdefinition-question",
    "elementdefinition-selector",
    "elementdefinition-translatable",
    "encounter-associatedEncounter",
    "encounter-modeOfArrival",
    "encounter-primaryDiagnosis",
    "encounter-reasonCancelled",
    "entryFormat",
    "event-definition",
    "event-notDone",
    "event-OnBehalfOf",
    "event-partOf",
    "event-performerRole",
    "event-reasonCode",
    "event-reasonReference",
    "family-member-history-genetics-observation",
    "family-member-history-genetics-parent",
    "family-member-history-genetics-sibling",
    "familymemberhistory-abatement",
    "familymemberhistory-patient-record",
    "familymemberhistory-severity",
    "familymemberhistory-type",
    "flag-detail",
    "flag-priority",
    "geolocation",
    "goal-acceptance",
    "goal-pertainsToGoal",
    "goal-reasonRejected",
    "goal-relationship",
    "hla-genotyping-results-allele-database",
    "hla-genotyping-results-glstring",
    "hla-genotyping-results-haploid",
    "hla-genotyping-results-method",
    "http-response-header",
    "humanname-assembly-order",
    "humanname-fathers-family",
    "humanname-mothers-family",
    "humanname-own-name",
    "humanname-own-prefix",
    "humanname-partner-name",
    "humanname-partner-prefix",
    "identifier-validDate",
    "implementationguide-page",
    "iso21090-AD-use",
    "iso21090-ADXP-additionalLocator",
    "iso21090-ADXP-buildingNumberSuffix",
    "iso21090-ADXP-careOf",
    "iso21090-ADXP-censusTract",
    "iso21090-ADXP-delimiter",
    "iso21090-ADXP-deliveryAddressLine",
    "iso21090-ADXP-deliveryInstallationArea",
    "iso21090-ADXP-deliveryInstallationQualifier",
    "iso21090-ADXP-deliveryInstallationType",
    "iso21090-ADXP-deliveryMode",
    "iso21090-ADXP-deliveryModeIdentifier",
    "iso21090-ADXP-direction",
    "iso21090-ADXP-houseNumber",
    "iso21090-ADXP-houseNumberNumeric",
    "iso21090-ADXP-postBox",
    "iso21090-ADXP-precinct",
    "iso21090-ADXP-streetAddressLine",
    "iso21090-ADXP-streetName",
    "iso21090-ADXP-streetNameBase",
    "iso21090-ADXP-streetNameType",
    "iso21090-ADXP-unitID",
    "iso21090-ADXP-unitType",
    "iso21090-CO-value",
    "iso21090-EN-qualifier",
    "iso21090-EN-representation",
    "iso21090-nullFlavor",
    "iso21090-preferred",
    "iso21090-SC-coding",
    "iso21090-TEL-address",
    "iso21090-uncertainty",
    "iso21090-uncertaintyType",
    "iso21090-verification",
    "location-alias",
    "location-distance",
    "mapSourcePublisher",
    "match-grade",
    "maxDecimalPlaces",
    "maxSize",
    "maxValue",
    "medication-usualRoute",
    "medicationdispense-validityPeriod",
    "medicationstatement-Prescriber",
    "messageheader-response-request",
    "mimeType",
    "minLength",
    "minValue",
    "observation-bodyPosition",
    "observation-delta",
    "observation-eventTiming",
    "observation-focal-subject",
    "observation-geneticsAlleleName",
    "observation-geneticsAllelicFrequency",
    "observation-geneticsAllelicState",
    "observation-geneticsAminoAcidChangeName",
    "observation-geneticsAminoAcidChangeType",
    "observation-geneticsCopyNumberEvent",
    "observation-geneticsDNARegionName",
    "observation-geneticsDNASequenceVariantName",
    "observation-geneticsDNASequenceVariantType",
    "observation-geneticsDNAVariantId",
    "observation-geneticsGene",
    "observation-geneticsGenomicSourceClass",
    "observation-geneticsInterpretation",
    "observation-geneticsPhaseSet",
    "observation-geneticsSequence",
    "observation-time-offset",
    "openEHR-administration",
    "openEHR-careplan",
    "openEHR-exposureDate",
    "openEHR-exposureDescription",
    "openEHR-exposureDuration",
    "openEHR-location",
    "openEHR-management",
    "openEHR-test",
    "operationoutcome-authority",
    "operationoutcome-detectedIssue",
    "operationoutcome-issue-source",
    "organization-alias",
    "organization-period",
    "organization-preferredContact",
    "patient-adoptionInfo",
    "patient-birthTime",
    "patient-cadavericDonor",
    "patient-citizenship",
    "patient-clinicalTrial",
    "patient-congregation",
    "patient-disability",
    "patient-importance",
    "patient-interpreterRequired",
    "patient-mothersMaidenName",
    "patient-nationality",
    "patient-religion",
    "pharmacy-core-doseType",
    "pharmacy-core-infuseOver",
    "pharmacy-core-maxDeliveryRate",
    "pharmacy-core-maxDeliveryVolume",
    "pharmacy-core-minDosePerPeriod",
    "pharmacy-core-rateGoal",
    "pharmacy-core-rateIncrement",
    "pharmacy-core-rateIncrementInterval",
    "pharmacy-core-refillsRemaining",
    "practitioner-animalSpecies",
    "practitioner-classification",
    "practitionerrole-primaryInd",
    "procedure-approachBodySite",
    "procedure-causedBy",
    "procedure-incisionDateTime",
    "procedure-method",
    "procedure-progressStatus",
    "procedure-schedule",
    "procedure-targetBodySite",
    "procedurerequest-approachBodySite",
    "procedurerequest-authorizedBy",
    "procedurerequest-geneticsItem",
    "procedurerequest-precondition",
    "procedurerequest-questionnaireRequest",
    "procedurerequest-reasonRefused",
    "procedurerequest-reasonRejected",
    "procedurerequest-targetBodySite",
    "questionnaire-allowedProfile",
    "questionnaire-allowedResource",
    "questionnaire-baseType",
    "questionnaire-choiceOrientation",
    "questionnaire-deMap",
    "questionnaire-displayCategory",
    "questionnaire-fhirType",
    "questionnaire-hidden",
    "questionnaire-itemControl",
    "questionnaire-lookupQuestionnaire",
    "questionnaire-maxOccurs",
    "questionnaire-minOccurs",
    "questionnaire-optionExclusive",
    "questionnaire-optionPrefix",
    "questionnaire-ordinalValue",
    "questionnaire-referenceFilter",
    "questionnaire-sourceStructureMap",
    "questionnaire-studyprotocolIdentifier",
    "questionnaire-supportLink",
    "questionnaire-targetStructureMap",
    "questionnaire-unit",
    "questionnaire-usageMode",
    "questionnaireresponse-author",
    "questionnaireresponse-note",
    "questionnaireresponse-reason",
    "questionnaireresponse-reviewer",
    "referralrequest-reasonRefused",
    "regex",
    "rendered-value",
    "rendering-markdown",
    "rendering-style",
    "rendering-styleSensitive",
    "rendering-xhtml",
    "resource-approvalDate",
    "resource-effectivePeriod",
    "resource-lastReviewDate",
    "specimen-collectionPriority",
    "specimen-isDryWeight",
    "specimen-processingTime",
    "specimen-sequenceNumber",
    "specimen-specialHandling",
    "structuredefinition-ancestor",
    "structuredefinition-annotation",
    "structuredefinition-ballot-status",
    "structuredefinition-category",
    "structuredefinition-display-hint",
    "structuredefinition-explicit-type-name",
    "structuredefinition-fmm-no-warnings",
    "structuredefinition-fmm",
    "structuredefinition-json-type",
    "structuredefinition-rdf-type",
    "structuredefinition-regex",
    "structuredefinition-summary",
    "structuredefinition-table-name",
    "structuredefinition-template-status",
    "structuredefinition-wg",
    "structuredefinition-xml-type",
    "task-candidateList",
    "task-replaces",
    "timing-daysOfCycle",
    "timing-exact",
    "translation",
    "usagecontext-group",
    "valueset-author",
    "valueset-caseSensitive",
    "valueset-comment",
    "valueset-comments",
    "valueset-conceptOrder",
    "valueset-definition",
    "valueset-effectiveDate",
    "valueset-expansionSource",
    "valueset-expirationDate",
    "valueset-history",
    "valueset-keyWord",
    "valueset-label",
    "valueset-map",
    "valueset-ordinalValue",
    "valueset-otherName",
    "valueset-reference",
    "valueset-sourceReference",
    "valueset-system",
    "valueset-systemName",
    "valueset-systemRef",
    "valueset-toocostly",
    "valueset-trusted-expansion",
    "valueset-unclosed",
    "valueset-usage",
    "valueset-warning",
    "valueset-workflowStatus"
  };
}
