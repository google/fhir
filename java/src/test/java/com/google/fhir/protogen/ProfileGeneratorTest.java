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

import com.google.common.io.Files;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.proto.Extensions;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.proto.Profiles;
import com.google.fhir.proto.Terminologies;
import com.google.fhir.r4.core.Bundle;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.time.LocalDate;
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

  private final JsonFormat.Parser jsonParser = JsonFormat.getEarlyVersionGeneratorParser();
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

  private StructureDefinition loadStructureDefinition(String fullFilename) throws IOException {
    String structDefString =
        Files.asCharSource(new File(fullFilename), StandardCharsets.UTF_8).read();
    StructureDefinition.Builder structDefBuilder = StructureDefinition.newBuilder();
    jsonParser.merge(structDefString, structDefBuilder);
    return structDefBuilder.build();
  }

  private static String loadGoldenJson(String filename) throws IOException {
    return Files.asCharSource(new File(filename), StandardCharsets.UTF_8).read();
  }

  private StructureDefinition loadStu3FhirStructureDefinition(String filename) throws IOException {
    return loadStructureDefinition("spec/hl7.fhir.core/3.0.1/package/" + filename);
  }

  private StructureDefinition loadR4FhirStructureDefinition(String filename) throws IOException {
    return loadStructureDefinition("spec/hl7.fhir.core/4.0.1/package/" + filename);
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

  private ProfileGenerator makeStu3Generator(String goldenJson) throws IOException {
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
            .generateCodeSystems(
                loadTerminologiesProto(STU3_TESTDATA_DIR + "test_terminologies.prototxt"));
    String output = jsonPrinter.print(terminologies).trim();
    assertThat(output).isEqualTo(goldenDefinition);
  }

  private ProfileGenerator makeR4Generator(String goldenJson) throws IOException {
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
            .generateCodeSystems(
                loadTerminologiesProto(R4_TESTDATA_DIR + "test_terminologies.prototxt"));
    String output = jsonPrinter.print(terminologies).trim();
    assertThat(output).isEqualTo(goldenDefinition);
  }
}
