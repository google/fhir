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

package com.google.medical.records.fhir;

import static com.google.common.truth.Truth.assertThat;

import com.google.common.io.Files;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.stu3.JsonFormat;
import com.google.fhir.stu3.proto.Bundle;
import com.google.fhir.stu3.proto.StructureDefinition;
import com.google.medical.records.fhir.proto.Extensions;
import com.google.medical.records.fhir.proto.Profiles;
import com.google.protobuf.TextFormat;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for ProfileGenerator */
@RunWith(JUnit4.class)
public final class ProfileGeneratorTest {

  private final JsonFormat.Parser jsonParser = JsonFormat.Parser.newBuilder().build();
  private final JsonFormat.Printer jsonPrinter = JsonFormat.getPrinter();

  private ProfileGenerator generator;
  private List<StructureDefinition> knownTypes;
  private final LocalDate creationDate = LocalDate.of(2018, 9, 22);

  // TODO: consolidate these proto loading functions across test files.
  private static Extensions loadExtensionsProto(String filename) throws IOException {
    Extensions.Builder extensionsBuilder = Extensions.newBuilder();
    TextFormat.merge(
        Files.asCharSource(
                new File(
                    "javatests/com/google/medical/records/fhir/testdata/" + filename + ".prototxt"),
                StandardCharsets.UTF_8)
            .read(),
        extensionsBuilder);
    return extensionsBuilder.build();
  }

  private static Profiles loadProfilesProto(String filename) throws IOException {
    Profiles.Builder profilesBuilder = Profiles.newBuilder();
    TextFormat.merge(
        Files.asCharSource(
                new File(
                    "javatests/com/google/medical/records/fhir/testdata/" + filename + ".prototxt"),
                StandardCharsets.UTF_8)
            .read(),
        profilesBuilder);
    return profilesBuilder.build();
  }

  private static PackageInfo loadPackageInfoProto(String filename) throws IOException {
    PackageInfo.Builder projectInfo = PackageInfo.newBuilder();
    TextFormat.merge(
        Files.asCharSource(
                new File(
                    "javatests/com/google/medical/records/fhir/testdata/" + filename + ".prototxt"),
                StandardCharsets.UTF_8)
            .read(),
        projectInfo);
    return projectInfo.build();
  }

  private StructureDefinition loadStructureDefinition(String fullFilename) throws IOException {
    String structDefString =
        Files.asCharSource(new File(fullFilename), StandardCharsets.UTF_8).read();
    StructureDefinition.Builder structDefBuilder = StructureDefinition.newBuilder();
    jsonParser.merge(structDefString, structDefBuilder);
    return structDefBuilder.build();
  }

  private String loadTestStructureDefinitionJson(String filename) throws IOException {
    return Files.asCharSource(
            new File("javatests/com/google/medical/records/fhir/testdata/" + filename),
            StandardCharsets.UTF_8)
        .read();
  }

  private StructureDefinition loadFhirStructureDefinition(String filename) throws IOException {
    return loadStructureDefinition(
        "testdata/stu3/structure_definitions/" + filename + ".profile.json");
  }

  @Before
  public void setUp() throws IOException {
    knownTypes = new ArrayList<>();
    knownTypes.add(loadFhirStructureDefinition("coding"));
    knownTypes.add(loadFhirStructureDefinition("codeableconcept"));
    knownTypes.add(loadFhirStructureDefinition("element"));
    knownTypes.add(loadFhirStructureDefinition("extension"));
    knownTypes.add(loadFhirStructureDefinition("observation"));
    knownTypes.add(loadFhirStructureDefinition("patient"));
    generator =
        new ProfileGenerator(
            loadPackageInfoProto("test_package_info"),
            loadProfilesProto("profiles"),
            loadExtensionsProto("extensions"),
            knownTypes,
            creationDate);
  }

  @Test
  public void testGenerateExtensions() throws Exception {
    Bundle extensions = generator.generateExtensions();
    for (Bundle.Entry entry : extensions.getEntryList()) {
      StructureDefinition structDef = entry.getResource().getStructureDefinition();
      String testDefinition =
          loadTestStructureDefinitionJson(structDef.getId().getValue() + ".extension.json");
      assertThat(jsonPrinter.print(structDef).trim()).isEqualTo(testDefinition.trim());
    }
  }

  @Test
  public void testGenerateProfiles() throws Exception {
    Bundle profiles = generator.generateProfiles();
    for (Bundle.Entry entry : profiles.getEntryList()) {
      StructureDefinition structDef = entry.getResource().getStructureDefinition();
      String testDefinition =
          loadTestStructureDefinitionJson(structDef.getId().getValue() + ".profile.json");
      assertThat(jsonPrinter.print(structDef).trim()).isEqualTo(testDefinition.trim());
    }
  }
}
