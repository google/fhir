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

import static com.google.common.collect.ImmutableList.toImmutableList;
import static com.google.common.truth.Truth.assertThat;
import static java.util.stream.Collectors.toList;
import static org.junit.Assert.assertThrows;

import com.google.common.collect.ImmutableList;
import com.google.common.io.Files;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.proto.Annotations.FhirVersion;
import com.google.fhir.proto.CodeableConceptSlice;
import com.google.fhir.proto.CodeableConceptSlice.CodingSlice;
import com.google.fhir.proto.ElementData;
import com.google.fhir.proto.ExtensionSlice;
import com.google.fhir.proto.Extensions;
import com.google.fhir.proto.FieldRestriction;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.proto.Profile;
import com.google.fhir.proto.Profiles;
import com.google.fhir.proto.ReferenceRestriction;
import com.google.fhir.proto.SimpleExtension;
import com.google.fhir.proto.Terminologies;
import com.google.fhir.proto.ValueSetBinding;
import com.google.fhir.r4.core.BindingStrengthCode;
import com.google.fhir.r4.core.Bundle;
import com.google.fhir.r4.core.ConstraintSeverityCode;
import com.google.fhir.r4.core.ElementDefinition;
import com.google.fhir.r4.core.Id;
import com.google.fhir.r4.core.Markdown;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.UnsignedInt;
import com.google.fhir.r4.core.Uri;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.time.LocalDate;
import java.time.ZoneId;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for ProfileGenerator */
@RunWith(JUnit4.class)
public final class ProfileGeneratorTest {
  private final JsonFormat.Printer jsonPrinter = JsonFormat.getPrinter();

  private static final String STU3_TESTDATA_DIR = "testdata/stu3/profiles/";
  private static final String R4_TESTDATA_DIR = "testdata/r4/profiles/";

  // TODO: consolidate these proto loading functions across test files.
  private static void loadProto(String filename, Message.Builder builder) throws IOException {
    TextFormat.merge(
        Files.asCharSource(new File(filename), StandardCharsets.UTF_8).read(), builder);
  }

  private static Extensions loadExtensionsProto(String filename) throws IOException {
    Extensions.Builder extensionsBuilder = Extensions.newBuilder();
    loadProto(filename, extensionsBuilder);
    return extensionsBuilder.build();
  }

  private static Profiles loadProfilesProto(String filename) throws IOException {
    Profiles.Builder profilesBuilder = Profiles.newBuilder();
    loadProto(filename, profilesBuilder);
    return profilesBuilder.build();
  }

  private static Terminologies loadTerminologiesProto(String filename) throws IOException {
    Terminologies.Builder terminologiesBuilder = Terminologies.newBuilder();
    loadProto(filename, terminologiesBuilder);
    return terminologiesBuilder.build();
  }

  private static PackageInfo loadPackageInfoProto(String filename) throws IOException {
    PackageInfo.Builder projectInfo = PackageInfo.newBuilder();
    loadProto(filename, projectInfo);
    return projectInfo.build();
  }

  private static StructureDefinition loadStructureDefinition(
      String fullFilename, FhirVersion version) throws IOException, InvalidFhirException {
    String structDefString =
        Files.asCharSource(new File(fullFilename), StandardCharsets.UTF_8).read();
    StructureDefinition.Builder structDefBuilder = StructureDefinition.newBuilder();
    JsonFormat.getSpecParser(version).merge(structDefString, structDefBuilder);
    return structDefBuilder.build();
  }

  private static String loadGoldenJson(String filename) throws IOException {
    return Files.asCharSource(new File(filename), StandardCharsets.UTF_8).read();
  }

  private static StructureDefinition loadStu3FhirStructureDefinition(String filename)
      throws IOException, InvalidFhirException {
    return loadStructureDefinition(
        "spec/hl7.fhir.core/3.0.1/package/" + filename, FhirVersion.STU3);
  }

  private static StructureDefinition loadR4FhirStructureDefinition(String filename)
      throws IOException, InvalidFhirException {
    return loadStructureDefinition(
        "spec/hl7.fhir.core/4.0.1/package/" + filename, FhirVersion.R4);
  }

  private static final Pattern DATE_PATTERN =
      Pattern.compile("\"date\"\\s*:\\s*\"([0-9]{4})-([0-9]{2})-([0-9]{2})\"");

  /**
   * Extract the creation date that was used in the Golden json. This makes the testdata immune to
   * regenerations with new dates, by guaranteeing the ProfileGenerator will use the correct date.
   */
  private static LocalDate getCreationDate(String json) throws IOException {
    Matcher matcher = DATE_PATTERN.matcher(json);
    Assert.assertTrue(matcher.find());
    return LocalDate.of(
        Integer.parseInt(matcher.group(1)),
        Integer.parseInt(matcher.group(2)),
        Integer.parseInt(matcher.group(3)));
  }

  private static ProfileGenerator makeStu3Generator(String goldenJson) throws Exception {
    List<StructureDefinition> knownTypes = new ArrayList<>();
    knownTypes.add(loadStu3FhirStructureDefinition("StructureDefinition-Coding.json"));
    knownTypes.add(loadStu3FhirStructureDefinition("StructureDefinition-CodeableConcept.json"));
    knownTypes.add(loadStu3FhirStructureDefinition("StructureDefinition-Element.json"));
    knownTypes.add(loadStu3FhirStructureDefinition("StructureDefinition-Extension.json"));
    knownTypes.add(loadStu3FhirStructureDefinition("StructureDefinition-Observation.json"));
    knownTypes.add(loadStu3FhirStructureDefinition("StructureDefinition-Patient.json"));
    knownTypes.add(loadStu3FhirStructureDefinition("StructureDefinition-Bundle.json"));

    LocalDate creationDate = getCreationDate(goldenJson);

    return new ProfileGenerator(
        loadPackageInfoProto(STU3_TESTDATA_DIR + "test_package_info.prototxt"),
        knownTypes,
        creationDate);
  }

  @Test
  public void testGenerateExtensionsStu3() throws Exception {
    String goldenDefinition = loadGoldenJson(STU3_TESTDATA_DIR + "test_extensions.json").trim();
    Bundle extensions =
        makeStu3Generator(goldenDefinition)
            .generateExtensions(
                loadExtensionsProto(STU3_TESTDATA_DIR + "test_extensions.prototxt"));
    String output = jsonPrinter.print(extensions).trim();
    assertThat(output).isEqualTo(goldenDefinition);
  }

  @Test
  public void testGenerateProfilesStu3() throws Exception {
    String goldenDefinition = loadGoldenJson(STU3_TESTDATA_DIR + "test.json").trim();
    Bundle profiles =
        makeStu3Generator(goldenDefinition)
            .generateProfiles(loadProfilesProto(STU3_TESTDATA_DIR + "test_profiles.prototxt"));
    String output = jsonPrinter.print(profiles).trim();
    assertThat(output).isEqualTo(goldenDefinition);
  }

  @Test
  public void testGenerateTerminologiesStu3() throws Exception {
    String goldenDefinition = loadGoldenJson(STU3_TESTDATA_DIR + "test_terminologies.json").trim();
    Bundle terminologies =
        makeStu3Generator(goldenDefinition)
            .generateTerminologies(
                loadTerminologiesProto(STU3_TESTDATA_DIR + "test_terminologies.prototxt"));
    String output = jsonPrinter.print(terminologies).trim();
    assertThat(output).isEqualTo(goldenDefinition);
  }

  private static ProfileGenerator makeR4Generator(String goldenJson) throws Exception {
    List<StructureDefinition> knownTypes = new ArrayList<>();
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-Coding.json"));
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-CodeableConcept.json"));
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-Element.json"));
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-Extension.json"));
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-Observation.json"));
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-Patient.json"));
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-Bundle.json"));
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-Encounter.json"));

    LocalDate creationDate = getCreationDate(goldenJson);

    return new ProfileGenerator(
        loadPackageInfoProto(R4_TESTDATA_DIR + "test_package_info.prototxt"),
        knownTypes,
        creationDate);
  }

  @Test
  public void testGenerateExtensionsR4() throws Exception {
    String goldenDefinition = loadGoldenJson(R4_TESTDATA_DIR + "test_extensions.json").trim();
    Bundle extensions =
        makeR4Generator(goldenDefinition)
            .generateExtensions(loadExtensionsProto(R4_TESTDATA_DIR + "test_extensions.prototxt"));
    String output = jsonPrinter.print(extensions).trim();
    assertThat(output).isEqualTo(goldenDefinition);
  }

  @Test
  public void testGenerateProfilesR4() throws Exception {
    String goldenDefinition = loadGoldenJson(R4_TESTDATA_DIR + "test.json").trim();
    Bundle profiles =
        makeR4Generator(goldenDefinition)
            .generateProfiles(loadProfilesProto(R4_TESTDATA_DIR + "test_profiles.prototxt"));
    String output = jsonPrinter.print(profiles).trim();
    assertThat(output).isEqualTo(goldenDefinition);
  }

  @Test
  public void testGenerateTerminologiesR4() throws Exception {
    String goldenDefinition = loadGoldenJson(R4_TESTDATA_DIR + "test_terminologies.json").trim();
    Bundle terminologies =
        makeR4Generator(goldenDefinition)
            .generateTerminologies(
                loadTerminologiesProto(R4_TESTDATA_DIR + "test_terminologies.prototxt"));
    String output = jsonPrinter.print(terminologies).trim();
    assertThat(output).isEqualTo(goldenDefinition);
  }

  @Test
  public void testReferenceRestriction_referenceBeforeDefine_validType() throws Exception {
    Profiles profiles =
        Profiles.newBuilder()
            .addProfile(
                Profile.newBuilder()
                    .setBaseUrl("http://hl7.org/fhir/StructureDefinition/Observation")
                    .addRestriction(
                        FieldRestriction.newBuilder()
                            .setFieldId("Observation.subject")
                            .setReferenceRestriction(
                                ReferenceRestriction.newBuilder()
                                    .addAllowed("http://test_patient_url"))))
            .addProfile(
                Profile.newBuilder()
                    .setBaseUrl("http://hl7.org/fhir/StructureDefinition/Patient")
                    .setElementData(
                        ElementData.newBuilder().setUrlOverride("http://test_patient_url")))
            .build();

    ProfileGenerator generator =
        new ProfileGenerator(
            loadPackageInfoProto(R4_TESTDATA_DIR + "test_package_info.prototxt"),
            ImmutableList.of(
                loadR4FhirStructureDefinition("StructureDefinition-Observation.json"),
                loadR4FhirStructureDefinition("StructureDefinition-Patient.json")),
            LocalDate.now(ZoneId.of("UTC")));
    Bundle bundle = generator.generateProfiles(profiles);

    StructureDefinition observationDefinition =
        bundle.getEntry(0).getResource().getStructureDefinition();

    List<ElementDefinition> observationSubject =
        observationDefinition.getSnapshot().getElementList().stream()
            .filter(element -> element.getId().getValue().equals("Observation.subject"))
            .collect(toList());

    assertThat(observationSubject).hasSize(1);
    assertThat(observationSubject.get(0).getType(0).getTargetProfile(0).getValue())
        .isEqualTo("http://test_patient_url");
  }

  @Test
  public void testRestriction_valueSetBindingAnnotation() throws Exception {
    Profiles profiles =
        Profiles.newBuilder()
            .addProfile(
                Profile.newBuilder()
                    .setBaseUrl("http://hl7.org/fhir/StructureDefinition/Observation")
                    .addRestriction(
                        FieldRestriction.newBuilder()
                            .setFieldId("Observation.code")
                            .setValueSetBinding(
                                ValueSetBinding.newBuilder()
                                    .setSystem("http://test_system/Foo")
                                    .setDescription("MUST have a Foo code.")))
                    .addRestriction(
                        FieldRestriction.newBuilder()
                            .setFieldId("Observation.category")
                            .setValueSetBinding(
                                ValueSetBinding.newBuilder()
                                    .setSystem("http://test_system/Bar")
                                    .setBindingStrength(BindingStrengthCode.Value.PREFERRED)
                                    .setDescription("SHOULD have a Bar code."))))
            .build();

    ProfileGenerator generator =
        new ProfileGenerator(
            loadPackageInfoProto(R4_TESTDATA_DIR + "test_package_info.prototxt"),
            ImmutableList.of(loadR4FhirStructureDefinition("StructureDefinition-Observation.json")),
            LocalDate.now(ZoneId.of("UTC")));
    Bundle bundle = generator.generateProfiles(profiles);

    StructureDefinition observationDefinition =
        bundle.getEntry(0).getResource().getStructureDefinition();

    ElementDefinition observationCode =
        observationDefinition.getSnapshot().getElementList().stream()
            .filter(element -> element.getId().getValue().equals("Observation.code"))
            .collect(toList())
            .get(0);
    assertThat(observationCode.getBinding().getStrength().getValue())
        .isEqualTo(BindingStrengthCode.Value.REQUIRED);
    assertThat(observationCode.getBinding().getValueSet().getValue())
        .isEqualTo("http://test_system/Foo");
    assertThat(observationCode.getBinding().getDescription().getValue())
        .isEqualTo("MUST have a Foo code.");

    ElementDefinition observationCategory =
        observationDefinition.getSnapshot().getElementList().stream()
            .filter(element -> element.getId().getValue().equals("Observation.category"))
            .collect(toList())
            .get(0);
    assertThat(observationCategory.getBinding().getStrength().getValue())
        .isEqualTo(BindingStrengthCode.Value.PREFERRED);
    assertThat(observationCategory.getBinding().getValueSet().getValue())
        .isEqualTo("http://test_system/Bar");
    assertThat(observationCategory.getBinding().getDescription().getValue())
        .isEqualTo("SHOULD have a Bar code.");
  }

  @Test
  public void testExtension_withValueSetBinding() throws Exception {
    Extensions extensions =
        Extensions.newBuilder()
            .addSimpleExtension(
                SimpleExtension.newBuilder()
                    .setElementData(ElementData.newBuilder().setName("MyExtension"))
                    .addType("CodeableConcept")
                    .setCodeType(
                        ValueSetBinding.newBuilder()
                            .setSystem("http://test_system/Foo")
                            .setDescription("MUST have a Foo code.")))
            .build();

    ProfileGenerator generator =
        new ProfileGenerator(
            loadPackageInfoProto(R4_TESTDATA_DIR + "test_package_info.prototxt"),
            ImmutableList.of(
                loadR4FhirStructureDefinition("StructureDefinition-Extension.json"),
                loadR4FhirStructureDefinition("StructureDefinition-Element.json")),
            LocalDate.now(ZoneId.of("UTC")));
    Bundle bundle = generator.generateExtensions(extensions);

    StructureDefinition extension = bundle.getEntry(0).getResource().getStructureDefinition();

    ElementDefinition valueElement =
        extension.getSnapshot().getElementList().stream()
            .filter(element -> element.getId().getValue().equals("Extension.value[x]"))
            .collect(toList())
            .get(0);
    assertThat(valueElement.getBinding().getStrength().getValue())
        .isEqualTo(BindingStrengthCode.Value.REQUIRED);
    assertThat(valueElement.getBinding().getValueSet().getValue())
        .isEqualTo("http://test_system/Foo");
    assertThat(valueElement.getBinding().getDescription().getValue())
        .isEqualTo("MUST have a Foo code.");
  }

  @Test
  public void testCodeableConceptSlice_withFixedValue() throws Exception {
    Profiles profiles =
        Profiles.newBuilder()
            .addProfile(
                Profile.newBuilder()
                    .setBaseUrl("http://hl7.org/fhir/StructureDefinition/Observation")
                    .addCodeableConceptSlice(
                        CodeableConceptSlice.newBuilder()
                            .setFieldId("Observation.code")
                            .addCodingSlice(
                                CodingSlice.newBuilder()
                                    .setElementData(ElementData.newBuilder().setName("myslice"))
                                    .setCodeData(
                                        ValueSetBinding.newBuilder()
                                            .setSystem("sys")
                                            .setFixedValue("val")))))
            .build();

    ProfileGenerator generator =
        new ProfileGenerator(
            loadPackageInfoProto(R4_TESTDATA_DIR + "test_package_info.prototxt"),
            ImmutableList.of(
                loadR4FhirStructureDefinition("StructureDefinition-Observation.json"),
                loadR4FhirStructureDefinition("StructureDefinition-Coding.json"),
                loadR4FhirStructureDefinition("StructureDefinition-Element.json"),
                loadR4FhirStructureDefinition("StructureDefinition-CodeableConcept.json")),
            LocalDate.now(ZoneId.of("UTC")));
    Bundle bundle = generator.generateProfiles(profiles);

    StructureDefinition extension = bundle.getEntry(0).getResource().getStructureDefinition();

    ElementDefinition codingSlice =
        extension.getSnapshot().getElementList().stream()
            .filter(element -> element.getId().getValue().equals("Observation.code.coding:myslice"))
            .collect(toImmutableList())
            .get(0);
    assertThat(codingSlice.getTypeCount()).isEqualTo(1);
    assertThat(codingSlice.getType(0).getCode().getValue()).isEqualTo("Coding");

    ElementDefinition sliceCodeElement =
        extension.getSnapshot().getElementList().stream()
            .filter(
                element ->
                    element.getId().getValue().equals("Observation.code.coding:myslice.code"))
            .collect(toList())
            .get(0);
    assertThat(sliceCodeElement.getFixed().getCode().getValue()).isEqualTo("val");
    assertThat(sliceCodeElement.hasBinding()).isFalse();

    ElementDefinition sliceSystemElement =
        extension.getSnapshot().getElementList().stream()
            .filter(
                element ->
                    element.getId().getValue().equals("Observation.code.coding:myslice.system"))
            .collect(toList())
            .get(0);
    assertThat(sliceSystemElement.getFixed().getUri().getValue()).isEqualTo("sys");
  }

  @Test
  public void testReferenceRestriction_referenceBeforeDefine_invalidType() throws Exception {
    Profiles profiles =
        Profiles.newBuilder()
            .addProfile(
                Profile.newBuilder()
                    .setBaseUrl("http://hl7.org/fhir/StructureDefinition/Observation")
                    .addRestriction(
                        FieldRestriction.newBuilder()
                            .setFieldId("Observation.subject")
                            .setReferenceRestriction(
                                ReferenceRestriction.newBuilder()
                                    .addAllowed("http://test_medication_url"))))
            .addProfile(
                Profile.newBuilder()
                    .setBaseUrl("http://hl7.org/fhir/StructureDefinition/Medication")
                    .setElementData(
                        ElementData.newBuilder().setUrlOverride("http://test_medication_url")))
            .build();

    ProfileGenerator generator =
        new ProfileGenerator(
            loadPackageInfoProto(R4_TESTDATA_DIR + "test_package_info.prototxt"),
            ImmutableList.of(
                loadR4FhirStructureDefinition("StructureDefinition-Observation.json"),
                loadR4FhirStructureDefinition("StructureDefinition-Medication.json")),
            LocalDate.now(ZoneId.of("UTC")));

    // Observation.subject cannot refer to a Medication, which http://test_medication_url is a
    // profile of.
    IllegalArgumentException e =
        assertThrows(IllegalArgumentException.class, () -> generator.generateProfiles(profiles));

    assertThat(e).hasMessageThat().contains("Invalid ReferenceRestriction");
  }

  @Test
  public void testElementDefinitionsOverridesBase() throws Exception {
    Profiles profiles =
        Profiles.newBuilder()
            .addProfile(
                Profile.newBuilder()
                    .setBaseUrl("http://hl7.org/fhir/StructureDefinition/Observation")
                    .addElementDefinition(
                        ElementDefinition.newBuilder()
                            .setId(
                                com.google.fhir.r4.core.String.newBuilder()
                                    .setValue("Observation.component"))
                            .setMin(UnsignedInt.newBuilder().setValue(1))
                            .setMax(com.google.fhir.r4.core.String.newBuilder().setValue("1"))
                            .setComment(Markdown.newBuilder().setValue("test comment"))))
            .build();

    ProfileGenerator generator =
        new ProfileGenerator(
            loadPackageInfoProto(R4_TESTDATA_DIR + "test_package_info.prototxt"),
            ImmutableList.of(loadR4FhirStructureDefinition("StructureDefinition-Observation.json")),
            LocalDate.now(ZoneId.of("UTC")));
    Bundle bundle = generator.generateProfiles(profiles);

    StructureDefinition observationDefinition =
        bundle.getEntry(0).getResource().getStructureDefinition();

    List<ElementDefinition> observationComponent =
        observationDefinition.getSnapshot().getElementList().stream()
            .filter(element -> element.getId().getValue().equals("Observation.component"))
            .collect(toList());

    assertThat(observationComponent).hasSize(1);
    assertThat(observationComponent.get(0).getMin().getValue()).isEqualTo(1);
    assertThat(observationComponent.get(0).getMax().getValue()).isEqualTo("1");
    assertThat(observationComponent.get(0).getComment().getValue()).isEqualTo("test comment");
  }

  @Test
  public void testElementDefinitions_addingConstraint() throws Exception {
    Profiles profiles =
        Profiles.newBuilder()
            .addProfile(
                Profile.newBuilder()
                    .setBaseUrl("http://hl7.org/fhir/StructureDefinition/Patient")
                    .addElementDefinition(
                        ElementDefinition.newBuilder()
                            .setId(com.google.fhir.r4.core.String.newBuilder().setValue("Patient"))
                            .addConstraint(
                                ElementDefinition.Constraint.newBuilder()
                                    .setKey(Id.newBuilder().setValue("some-unique-key"))
                                    .setSeverity(
                                        ElementDefinition.Constraint.SeverityCode.newBuilder()
                                            .setValue(ConstraintSeverityCode.Value.ERROR))
                                    .setExpression(
                                        com.google.fhir.r4.core.String.newBuilder()
                                            .setValue("some-expression"))
                                    .setHuman(
                                        com.google.fhir.r4.core.String.newBuilder()
                                            .setValue("human")))))
            .build();

    ProfileGenerator generator =
        new ProfileGenerator(
            loadPackageInfoProto(R4_TESTDATA_DIR + "test_package_info.prototxt"),
            ImmutableList.of(loadR4FhirStructureDefinition("StructureDefinition-Patient.json")),
            LocalDate.now(ZoneId.of("UTC")));
    Bundle bundle = generator.generateProfiles(profiles);

    StructureDefinition patient = bundle.getEntry(0).getResource().getStructureDefinition();

    List<ElementDefinition> root =
        patient.getSnapshot().getElementList().stream()
            .filter(element -> element.getId().getValue().equals("Patient"))
            .collect(toList());

    // The Root element should have 6 constraints - 5 from the base Patient, one that we've added
    assertThat(root).hasSize(1);
    assertThat(root.get(0).getConstraintCount()).isEqualTo(6);
    assertThat(root.get(0).getConstraint(5).getKey().getValue()).isEqualTo("some-unique-key");
  }

  @Test
  public void testElementDefinitions_newTypesOverwritesOldList() throws Exception {
    Profiles profiles =
        Profiles.newBuilder()
            .addProfile(
                Profile.newBuilder()
                    .setBaseUrl("http://hl7.org/fhir/StructureDefinition/Patient")
                    .addElementDefinition(
                        ElementDefinition.newBuilder()
                            .setId(
                                com.google.fhir.r4.core.String.newBuilder()
                                    .setValue("Patient.deceased[x]"))
                            .addType(
                                ElementDefinition.TypeRef.newBuilder()
                                    .setCode(Uri.newBuilder().setValue("boolean")))))
            .build();

    ProfileGenerator generator =
        new ProfileGenerator(
            loadPackageInfoProto(R4_TESTDATA_DIR + "test_package_info.prototxt"),
            ImmutableList.of(loadR4FhirStructureDefinition("StructureDefinition-Patient.json")),
            LocalDate.now(ZoneId.of("UTC")));
    Bundle bundle = generator.generateProfiles(profiles);

    StructureDefinition patient = bundle.getEntry(0).getResource().getStructureDefinition();

    List<ElementDefinition> typeElement =
        patient.getSnapshot().getElementList().stream()
            .filter(element -> element.getId().getValue().equals("Patient.deceased[x]"))
            .collect(toList());

    assertThat(typeElement).hasSize(1);
    assertThat(typeElement.get(0).getTypeCount()).isEqualTo(1);
    assertThat(typeElement.get(0).getType(0).getCode().getValue()).isEqualTo("boolean");
  }

  @Test
  public void testElementDefinitions_invalidPath() throws Exception {
    Profiles profiles =
        Profiles.newBuilder()
            .addProfile(
                Profile.newBuilder()
                    .setBaseUrl("http://hl7.org/fhir/StructureDefinition/Observation")
                    .addElementDefinition(
                        ElementDefinition.newBuilder()
                            .setId(
                                com.google.fhir.r4.core.String.newBuilder()
                                    .setValue("Observation.garbage"))
                            .setMin(UnsignedInt.newBuilder().setValue(1))
                            .setMax(com.google.fhir.r4.core.String.newBuilder().setValue("1"))
                            .setComment(Markdown.newBuilder().setValue("test comment"))))
            .build();

    ProfileGenerator generator =
        new ProfileGenerator(
            loadPackageInfoProto(R4_TESTDATA_DIR + "test_package_info.prototxt"),
            ImmutableList.of(loadR4FhirStructureDefinition("StructureDefinition-Observation.json")),
            LocalDate.now(ZoneId.of("UTC")));

    assertThrows(IllegalArgumentException.class, () -> generator.generateProfiles(profiles));
  }

  @Test
  public void testExtensionSlice_mustSupport() throws Exception {
    Profiles profiles =
        Profiles.newBuilder()
            .addProfile(
                Profile.newBuilder()
                    .setBaseUrl("http://hl7.org/fhir/StructureDefinition/Patient")
                    .addExtensionSlice(
                        ExtensionSlice.newBuilder()
                            .setFieldId("Patient.birthDate")
                            .setElementData(ElementData.newBuilder().setName("birthTime"))
                            .setUrl("http://hl7.org/fhir/StructureDefinition/patient-birthTime")
                            .setMustSupport(true)))
            .build();

    ProfileGenerator generator =
        new ProfileGenerator(
            loadPackageInfoProto(R4_TESTDATA_DIR + "test_package_info.prototxt"),
            ImmutableList.of(
                loadR4FhirStructureDefinition("StructureDefinition-Patient.json"),
                loadR4FhirStructureDefinition("StructureDefinition-Extension.json")),
            LocalDate.now(ZoneId.of("UTC")));

    Bundle bundle = generator.generateProfiles(profiles);
    StructureDefinition observationDefinition =
        bundle.getEntry(0).getResource().getStructureDefinition();

    List<ElementDefinition> birthTimeExtension =
        observationDefinition.getSnapshot().getElementList().stream()
            .filter(
                element ->
                    element.getId().getValue().equals("Patient.birthDate.extension:birthTime"))
            .collect(toList());
    assertThat(birthTimeExtension).hasSize(1);
    assertThat(birthTimeExtension.get(0).getMustSupport().getValue()).isTrue();
  }
}
