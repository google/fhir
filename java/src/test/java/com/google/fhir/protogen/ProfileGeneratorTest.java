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
import com.google.fhir.r4.core.Bundle;
import com.google.fhir.r4.core.StructureDefinition;
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
  private static Extensions loadExtensionsProto(String filename) throws IOException {
    Extensions.Builder extensionsBuilder = Extensions.newBuilder();
    TextFormat.merge(
        Files.asCharSource(new File(filename), StandardCharsets.UTF_8).read(), extensionsBuilder);
    return extensionsBuilder.build();
  }

  private static Profiles loadProfilesProto(String filename) throws IOException {
    Profiles.Builder profilesBuilder = Profiles.newBuilder();
    TextFormat.merge(
        Files.asCharSource(new File(filename), StandardCharsets.UTF_8).read(), profilesBuilder);
    return profilesBuilder.build();
  }

  private static PackageInfo loadPackageInfoProto(String filename) throws IOException {
    PackageInfo.Builder projectInfo = PackageInfo.newBuilder();
    TextFormat.merge(
        Files.asCharSource(new File(filename), StandardCharsets.UTF_8).read(), projectInfo);
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
  private static LocalDate getCreationDate(String jsonFile) throws IOException {
    String json = loadGoldenJson(jsonFile);
    Matcher matcher = DATE_PATTERN.matcher(json);
    Assert.assertTrue(matcher.find());
    return LocalDate.of(
        Integer.parseInt(matcher.group(1)),
        Integer.parseInt(matcher.group(2)),
        Integer.parseInt(matcher.group(3)));
  }

  private ProfileGenerator makeStu3Generator() throws IOException {
    List<StructureDefinition> knownTypes = new ArrayList<>();
    knownTypes.add(loadStu3FhirStructureDefinition("StructureDefinition-Coding.json"));
    knownTypes.add(loadStu3FhirStructureDefinition("StructureDefinition-CodeableConcept.json"));
    knownTypes.add(loadStu3FhirStructureDefinition("StructureDefinition-Element.json"));
    knownTypes.add(loadStu3FhirStructureDefinition("StructureDefinition-Extension.json"));
    knownTypes.add(loadStu3FhirStructureDefinition("StructureDefinition-Observation.json"));
    knownTypes.add(loadStu3FhirStructureDefinition("StructureDefinition-Patient.json"));
    knownTypes.add(loadStu3FhirStructureDefinition("StructureDefinition-Bundle.json"));
    LocalDate creationDate = getCreationDate(STU3_TESTDATA_DIR + "test.json");
    return new ProfileGenerator(
        loadPackageInfoProto(STU3_TESTDATA_DIR + "test_package_info.prototxt"),
        loadProfilesProto(STU3_TESTDATA_DIR + "test_profiles.prototxt"),
        loadExtensionsProto(STU3_TESTDATA_DIR + "test_extensions.prototxt"),
        knownTypes,
        creationDate);
  }

  @Test
  public void testGenerateExtensionsStu3() throws Exception {
    Bundle extensions = makeStu3Generator().generateExtensions();
    String goldenDefinition = loadGoldenJson(STU3_TESTDATA_DIR + "test_extensions.json").trim();
    String output = jsonPrinter.print(extensions).trim();
    assertThat(output).isEqualTo(goldenDefinition);
  }

  @Test
  public void testGenerateProfilesStu3() throws Exception {
    Bundle profiles = makeStu3Generator().generateProfiles();
    String goldenDefinition = loadGoldenJson(STU3_TESTDATA_DIR + "test.json").trim();
    String output = jsonPrinter.print(profiles).trim();
    assertThat(output).isEqualTo(goldenDefinition);
  }

  private ProfileGenerator makeR4Generator() throws IOException {
    List<StructureDefinition> knownTypes = new ArrayList<>();
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-Coding.json"));
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-CodeableConcept.json"));
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-Element.json"));
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-Extension.json"));
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-Observation.json"));
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-Patient.json"));
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-Bundle.json"));
    knownTypes.add(loadR4FhirStructureDefinition("StructureDefinition-Encounter.json"));

    LocalDate creationDate = getCreationDate(R4_TESTDATA_DIR + "test.json");

    return new ProfileGenerator(
        loadPackageInfoProto(R4_TESTDATA_DIR + "test_package_info.prototxt"),
        loadProfilesProto(R4_TESTDATA_DIR + "test_profiles.prototxt"),
        loadExtensionsProto(R4_TESTDATA_DIR + "test_extensions.prototxt"),
        knownTypes,
        creationDate);
  }

  @Test
  public void testGenerateExtensionsR4() throws Exception {
    Bundle extensions = makeR4Generator().generateExtensions();
    String goldenDefinition = loadGoldenJson(R4_TESTDATA_DIR + "test_extensions.json").trim();
    String output = jsonPrinter.print(extensions).trim();
    assertThat(output).isEqualTo(goldenDefinition);
  }

  @Test
  public void testGenerateProfilesR4() throws Exception {
    Bundle profiles = makeR4Generator().generateProfiles();
    String goldenDefinition = loadGoldenJson(R4_TESTDATA_DIR + "test.json").trim();
    String output = jsonPrinter.print(profiles).trim();
    assertThat(output).isEqualTo(goldenDefinition);
  }
}
