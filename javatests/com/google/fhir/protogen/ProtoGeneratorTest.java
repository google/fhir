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
import static java.nio.charset.StandardCharsets.UTF_8;
import static org.junit.Assert.fail;

import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.google.common.io.Files;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.common.Codes;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.proto.Annotations.FhirVersion;
import com.google.fhir.r4.core.ResourceTypeCode;
import com.google.fhir.r4.core.SearchParameter;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.StructureDefinitionKindCode;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.ExtensionRegistry;
import com.google.protobuf.TextFormat;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
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
  private FhirPackage fhirPackage;
  private Map<ResourceTypeCode.Value, List<SearchParameter>> searchParameterMap;

  /** Read the specifed file from the testdata directory into a String. */
  private String loadFile(String relativePath) throws IOException {
    File file = new File(runfiles.rlocation(relativePath));
    return Files.asCharSource(file, UTF_8).read();
  }

  /** Read and parse the specified StructureDefinition. */
  private StructureDefinition readStructureDefinition(String resourceName, FhirVersion version)
      throws IOException, InvalidFhirException {
    String pathPrefix;
    switch (version) {
      case STU3:
        pathPrefix =
            "com_google_fhir/spec/hl7.fhir.core/3.0.1/package/StructureDefinition-";
        break;
      case R4:
        pathPrefix =
            "com_google_fhir/spec/hl7.fhir.core/4.0.1/package/StructureDefinition-";
        break;
      default:
        throw new IllegalArgumentException("unrecognized FHIR version " + version);
    }
    String json = loadFile(pathPrefix + resourceName + ".json");
    StructureDefinition.Builder builder = StructureDefinition.newBuilder();
    jsonParser.merge(json, builder);
    return builder.build();
  }

  @Before
  public void setUp() throws IOException, InvalidFhirException {
    jsonParser = JsonFormat.getParser();
    textParser = TextFormat.getParser();
    runfiles = Runfiles.create();
    fhirPackage = FhirPackage.load("spec/fhir_r4_package.zip");
    searchParameterMap = GeneratorUtils.getSearchParameterMap(fhirPackage.searchParameters);

    registry = ExtensionRegistry.newInstance();
    ProtoGeneratorTestUtils.initializeRegistry(registry);
  }

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

  private void testGeneratedR4Proto(ProtoGenerator protoGenerator, String resourceName)
      throws Exception {
    StructureDefinition resource = readStructureDefinition(resourceName, FhirVersion.R4);
    List<SearchParameter> searchParameters;
    if (resource.getKind().getValue() == StructureDefinitionKindCode.Value.RESOURCE) {
      String resourceTypeId = resource.getSnapshot().getElementList().get(0).getId().getValue();
      // Get the string representation of the enum value for the resource type.
      EnumValueDescriptor enumValueDescriptor =
          Codes.codeStringToEnumValue(ResourceTypeCode.Value.getDescriptor(), resourceTypeId);
      searchParameters =
          searchParameterMap.getOrDefault(
              ResourceTypeCode.Value.forNumber(enumValueDescriptor.getNumber()),
              new ArrayList<>());
    } else {
      // Not a resource - no search parameters to add.
      searchParameters = new ArrayList<>();
    }
    DescriptorProto generatedProto = protoGenerator.generateProto(resource, searchParameters);
    DescriptorProto golden = readDescriptorProto(resourceName, FhirVersion.R4);
    if (!generatedProto.equals(golden)) {
      System.out.println("Failed on: " + resourceName);
      assertThat(generatedProto).isEqualTo(golden);
    }
  }

  private static final int EXPECTED_R4_COUNT = 644;

  /** Test generating R4 proto files. */
  @Test
  public void generateR4() throws Exception {
    ProtoGenerator protoGenerator =
        ProtoGeneratorTestUtils.makeProtoGenerator(
            "spec/fhir_r4_package.zip",
            "codes.proto",
            ImmutableMap.of("R4", "spec/fhir_r4_package.zip"),
            ImmutableSet.of() /* no dependencies */);
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
