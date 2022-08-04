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
import static java.util.stream.Collectors.toList;
import static org.junit.Assert.fail;

import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Iterables;
import com.google.common.io.Files;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.Annotations.FhirVersion;
import com.google.fhir.proto.ProtoGeneratorAnnotations;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.DescriptorProto.ReservedRange;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldOptions;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.MessageOptions;
import com.google.protobuf.DescriptorProtos.OneofDescriptorProto;
import com.google.protobuf.ExtensionRegistry;
import com.google.protobuf.Message;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
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
  private Runfiles runfiles;
  private final ProtoGenerator r4ProtoGenerator;

  public ProtoGeneratorTest() throws Exception {
    r4ProtoGenerator =
        ProtoGeneratorTestUtils.makeProtoGenerator(
            "spec/fhir_r4_package.zip",
            "codes.proto",
            ImmutableMap.of("R4", "spec/fhir_r4_package.zip"),
            ImmutableSet.of() /* no dependencies */);
  }

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
    runfiles = Runfiles.create();

    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    ProtoGeneratorTestUtils.initializeRegistry(registry);
  }

  // Clears ProtoGeneratorAnnotation extensions from generated descriptor protos.
  // These are directives to the {@link ProtoFilePrinter}, and do no appear in the printed protos.
  // Also replaces annotated reserved fields with ReservedRanges, as they appear in the printed
  // proto.
  private DescriptorProto.Builder clearProtogenAnnotations(DescriptorProto.Builder original) {
    original.getOptionsBuilder().clearExtension(ProtoGeneratorAnnotations.messageDescription);
    if (original.getOptions().equals(MessageOptions.getDefaultInstance())) {
      original.clearOptions();
    }

    for (DescriptorProto.Builder submessage : original.getNestedTypeBuilderList()) {
      clearProtogenAnnotations(submessage);
    }

    Set<Integer> fieldsToRemove = new HashSet<>();
    for (FieldDescriptorProto.Builder field : original.getFieldBuilderList()) {
      FieldOptions.Builder fieldOptionsBuilder = field.getOptionsBuilder();
      if (fieldOptionsBuilder.hasExtension(ProtoGeneratorAnnotations.reservedReason)) {
        original.addReservedRange(
            ReservedRange.newBuilder()
                .setStart(field.getNumber())
                .setEnd(field.getNumber() + 1)
                .build());
        fieldsToRemove.add(field.getNumber());
      }
      fieldOptionsBuilder.clearExtension(ProtoGeneratorAnnotations.fieldDescription);
      if (field.getOptions().equals(FieldOptions.getDefaultInstance())) {
        field.clearOptions();
      }
    }
    List<FieldDescriptorProto> finalFields =
        original.getFieldList().stream()
            .filter(field -> !fieldsToRemove.contains(field.getNumber()))
            .collect(toList());
    return original.clearField().addAllField(finalFields);
  }

  private static final ImmutableSet<String> RESOURCES_TO_SKIP =
      ImmutableSet.of(
          "ContainedResource", // Contained resource isn't generated from struct def.
          "Extension", // Extension type is hard coded.
          "Reference", // Reference type is hard coded.
          "ReferenceId", // ReferenceId type is hard coded.
          "CodingWithFixedCode", // CodingWithFixedCode is a custom data struct for profiling.
          "CodingWithFixedSystem"); // CodingWithFixedSystem is a custom data struct for profiling.

  // Builds a `DescriptorProto` as a StructureDefinition resource.
  private DescriptorProto buildResourceDescriptor(String resourceName) {
    return DescriptorProto.newBuilder()
        .setName(resourceName)
        .setOptions(
            MessageOptions.newBuilder()
                .setExtension(
                    Annotations.structureDefinitionKind,
                    Annotations.StructureDefinitionKindValue.KIND_RESOURCE))
        .build();
  }

  private void testGeneratedR4Proto(ProtoGenerator protoGenerator, String resourceName)
      throws IOException, ReflectiveOperationException, InvalidFhirException {
    DescriptorProto golden =
        ((Message)
                Class.forName("com.google.fhir.r4.core." + resourceName)
                    .getMethod("getDefaultInstance")
                    .invoke(null))
            .getDescriptorForType()
            .toProto();
    String structDefName =
        Iterables.getLast(
            Splitter.on("/")
                .splitToList(
                    golden.getOptions().getExtension(Annotations.fhirStructureDefinitionUrl)));
    StructureDefinition resource = readStructureDefinition(structDefName, FhirVersion.R4);
    DescriptorProto generatedProto =
        clearProtogenAnnotations(protoGenerator.generateProto(resource).toBuilder()).build();

    if (!generatedProto.equals(golden)) {
      System.out.println("Failed on: " + resourceName);
      assertThat(generatedProto).isEqualTo(golden);
    }
  }

  private static final Pattern MESSAGE_PATTERN =
      Pattern.compile("^message ([A-Za-z][A-Za-z0-9_]*) \\{", Pattern.MULTILINE);

  private int testDirectoryOfProtoFiles(String testdir)
      throws IOException, ReflectiveOperationException, InvalidFhirException {
    String suffix = ".proto";
    int count = 0;
    for (File file :
        new File(runfiles.rlocation(testdir)).listFiles((listDir, name) -> name.endsWith(suffix))) {
      count += testProtoFile(file);
    }
    return count;
  }

  // Given a .proto file, finds all messages along with corresponding structure definitions, and
  // generates protos from the structure definitions, and asserts that they match messages in the
  // file.
  // Returns the number of messages verified.
  private int testProtoFile(File file)
      throws IOException, ReflectiveOperationException, InvalidFhirException {
    int count = 0;

    String protoFileContents = Files.asCharSource(file, UTF_8).read();
    Matcher nameMatcher = MESSAGE_PATTERN.matcher(protoFileContents);
    List<String> resourceNames = new ArrayList<>();
    while (nameMatcher.find()) {
      resourceNames.add(nameMatcher.group(1));
      if (nameMatcher.group(1).isEmpty()) {
        throw new IllegalArgumentException();
      }
    }
    if (resourceNames.isEmpty()) {
      fail("Unable to locate message name in " + file.getAbsolutePath());
    }
    for (String resourceName : resourceNames) {
      if (RESOURCES_TO_SKIP.contains(resourceName)) {
        continue;
      }
      testGeneratedR4Proto(r4ProtoGenerator, resourceName);
      count++;
    }
    return count;
  }

  private static final int EXPECTED_R4_RESOURCE_COUNT = 149;
  private static final int EXPECTED_R4_PROFILE_COUNT = 42;
  private static final int EXPECTED_R4_EXTENSION_COUNT = 393;
  private static final int EXPECTED_R4_DATATYPE_COUNT = 61;

  /** Test generating R4 core profile files. */
  @Test
  public void testGenerateR4Datatypes() throws Exception {
    File file =
        new File(runfiles.rlocation("com_google_fhir/proto/google/fhir/proto/r4/core/datatypes.proto"));
    int resourceCount = testProtoFile(file);
    if (resourceCount != EXPECTED_R4_DATATYPE_COUNT) {
      fail(
          "Expected "
              + EXPECTED_R4_DATATYPE_COUNT
              + " R4 descriptors to test, but found "
              + resourceCount);
    }
  }

  /** Test generating R4 resource files. */
  @Test
  public void testGenerateR4Resources() throws Exception {
    int resourceCount =
        testDirectoryOfProtoFiles("com_google_fhir/proto/google/fhir/proto/r4/core/resources/");
    if (resourceCount != EXPECTED_R4_RESOURCE_COUNT) {
      fail(
          "Expected "
              + EXPECTED_R4_RESOURCE_COUNT
              + " R4 descriptors to test, but found "
              + resourceCount);
    }
  }

  /** Test generating R4 core profile files. */
  @Test
  public void testGenerateR4Extensions() throws Exception {
    File file =
        new File(runfiles.rlocation("com_google_fhir/proto/google/fhir/proto/r4/core/extensions.proto"));
    int resourceCount = testProtoFile(file);
    if (resourceCount != EXPECTED_R4_EXTENSION_COUNT) {
      fail(
          "Expected "
              + EXPECTED_R4_EXTENSION_COUNT
              + " R4 descriptors to test, but found "
              + resourceCount);
    }
  }

  /** Test generating R4 core profile files. */
  @Test
  public void testGenerateR4Profiles() throws Exception {
    int resourceCount =
        testDirectoryOfProtoFiles("com_google_fhir/proto/google/fhir/proto/r4/core/profiles/");
    if (resourceCount != EXPECTED_R4_PROFILE_COUNT) {
      fail(
          "Expected "
              + EXPECTED_R4_PROFILE_COUNT
              + " R4 descriptors to test, but found "
              + resourceCount);
    }
  }

  @Test
  public void addContainedResource_noResourceTypes_emptyFileDescriptor() {
    FileDescriptorProto res =
        r4ProtoGenerator.addContainedResource(
            FileDescriptorProto.getDefaultInstance(), /*resourceTypes=*/ ImmutableList.of());

    assertThat(res)
        .isEqualTo(
            FileDescriptorProto.newBuilder()
                .addMessageType(
                    DescriptorProto.newBuilder()
                        .setName("ContainedResource")
                        .addOneofDecl(OneofDescriptorProto.newBuilder().setName("oneof_resource")))
                .build());
  }

  @Test
  public void addContainedResource_duplicateResourceTypes_consolidates() {
    ImmutableList<DescriptorProto> resourceTypes =
        ImmutableList.of(buildResourceDescriptor("Foo"), buildResourceDescriptor("Foo"));

    FileDescriptorProto res =
        r4ProtoGenerator.addContainedResource(
            FileDescriptorProto.getDefaultInstance(), resourceTypes);

    assertThat(res)
        .isEqualTo(
            FileDescriptorProto.newBuilder()
                .addMessageType(
                    DescriptorProto.newBuilder()
                        .setName("ContainedResource")
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("foo")
                                .setNumber(1)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName("Foo")
                                .setOneofIndex(0))
                        .addOneofDecl(OneofDescriptorProto.newBuilder().setName("oneof_resource")))
                .build());
  }

  @Test
  public void addContainedResource_existingFileDescriptor_preservesExistingMessageType() {
    FileDescriptorProto fileDescriptor =
        FileDescriptorProto.newBuilder()
            .addMessageType(DescriptorProto.newBuilder().setName("ExistingMessageType"))
            .build();
    ImmutableList<DescriptorProto> resourceTypes = ImmutableList.of(buildResourceDescriptor("Foo"));

    FileDescriptorProto res = r4ProtoGenerator.addContainedResource(fileDescriptor, resourceTypes);

    assertThat(res)
        .isEqualTo(
            FileDescriptorProto.newBuilder()
                .addMessageType(DescriptorProto.newBuilder().setName("ExistingMessageType"))
                .addMessageType(
                    DescriptorProto.newBuilder()
                        .setName("ContainedResource")
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("foo")
                                .setNumber(1)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName("Foo")
                                .setOneofIndex(0))
                        .addOneofDecl(OneofDescriptorProto.newBuilder().setName("oneof_resource")))
                .build());
  }

  @Test
  public void addContainedResource_nonResourceType_excluded() {
    ImmutableList<DescriptorProto> resourceTypes =
        ImmutableList.of(
            buildResourceDescriptor("Foo"),
            DescriptorProto.newBuilder()
                .setName("Bar")
                .setOptions(
                    MessageOptions.newBuilder()
                        .setExtension(
                            Annotations.structureDefinitionKind,
                            Annotations.StructureDefinitionKindValue.KIND_PRIMITIVE_TYPE))
                .build());

    FileDescriptorProto res =
        r4ProtoGenerator.addContainedResource(
            FileDescriptorProto.getDefaultInstance(), resourceTypes);

    assertThat(res)
        .isEqualTo(
            FileDescriptorProto.newBuilder()
                .addMessageType(
                    DescriptorProto.newBuilder()
                        .setName("ContainedResource")
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("foo")
                                .setNumber(1)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName("Foo")
                                .setOneofIndex(0))
                        .addOneofDecl(OneofDescriptorProto.newBuilder().setName("oneof_resource")))
                .build());
  }

  @Test
  public void addContainedResource_abstractType_excluded() {
    ImmutableList<DescriptorProto> resourceTypes =
        ImmutableList.of(
            buildResourceDescriptor("Foo"),
            DescriptorProto.newBuilder()
                .setName("Bar")
                .setOptions(
                    MessageOptions.newBuilder()
                        .setExtension(
                            Annotations.structureDefinitionKind,
                            Annotations.StructureDefinitionKindValue.KIND_RESOURCE)
                        .setExtension(Annotations.isAbstractType, true))
                .build());

    FileDescriptorProto res =
        r4ProtoGenerator.addContainedResource(
            FileDescriptorProto.getDefaultInstance(), resourceTypes);

    assertThat(res)
        .isEqualTo(
            FileDescriptorProto.newBuilder()
                .addMessageType(
                    DescriptorProto.newBuilder()
                        .setName("ContainedResource")
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("foo")
                                .setNumber(1)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName("Foo")
                                .setOneofIndex(0))
                        .addOneofDecl(OneofDescriptorProto.newBuilder().setName("oneof_resource")))
                .build());
  }

  @Test
  public void addCoreContainedResource_nonAlphabetizedResourceTypes_sorted() {
    ImmutableList<DescriptorProto> resourceTypes =
        ImmutableList.of(
            buildResourceDescriptor("Foo"),
            buildResourceDescriptor("Bar"),
            buildResourceDescriptor("Baz"));

    FileDescriptorProto res =
        r4ProtoGenerator.addContainedResource(
            FileDescriptorProto.getDefaultInstance(), resourceTypes);

    assertThat(res)
        .isEqualTo(
            FileDescriptorProto.newBuilder()
                .addMessageType(
                    DescriptorProto.newBuilder()
                        .setName("ContainedResource")
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("bar")
                                .setNumber(1)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName("Bar")
                                .setOneofIndex(0))
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("baz")
                                .setNumber(2)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName("Baz")
                                .setOneofIndex(0))
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("foo")
                                .setNumber(3)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName("Foo")
                                .setOneofIndex(0))
                        .addOneofDecl(OneofDescriptorProto.newBuilder().setName("oneof_resource")))
                .build());
  }

  // Creates a temporary non-core proto package as a ZIP file and returns its absolute path.
  private String createNonCorePackage(String protoPackage) throws IOException {
    File f = File.createTempFile("non_core_package", ".zip");
    try (ZipOutputStream out = new ZipOutputStream(new FileOutputStream(f))) {
      ZipEntry e = new ZipEntry("foo_package_info.prototxt");
      out.putNextEntry(e);
      // The "google.foo" proto package makes this a non-core package.
      byte[] data =
          ("proto_package: \""
                  + protoPackage
                  + "\""
                  + "\njava_proto_package: \"com."
                  + protoPackage
                  + "\""
                  + "\nfhir_version: R4"
                  + "\nlicense: APACHE"
                  + "\nlicense_date: \"2019\""
                  + "\nlocal_contained_resource: true"
                  + "\nfile_splitting_behavior: SPLIT_RESOURCES")
              .getBytes(Charset.forName(UTF_8.name()));
      out.write(data, 0, data.length);
      out.closeEntry();
    }

    return f.getAbsolutePath();
  }

  @Test
  public void addContainedResource_derivedResources_unsortedFileDescriptor()
      throws IOException, InvalidFhirException {
    ProtoGenerator generator =
        ProtoGeneratorTestUtils.makeProtoGenerator(
            createNonCorePackage("google.foo"),
            "codes.proto",
            /*coreDepMap=*/ ImmutableMap.of("R4", "spec/fhir_r4_package.zip"),
            /*dependencyLocations=*/ ImmutableSet.of());

    ImmutableList<DescriptorProto> resourceTypes =
        ImmutableList.of(
            buildResourceDescriptor("Foo"),
            buildResourceDescriptor("Bar"),
            buildResourceDescriptor("Baz"),
            // For derived contained resources, make sure to keep the tag numbers from the base file
            // for resources that keep the same name.
            // In other words, "Patient" should keep the tag number (103) of the field in the base
            // contained resources for Patient.
            buildResourceDescriptor("Patient"));

    FileDescriptorProto res =
        generator.addContainedResource(FileDescriptorProto.getDefaultInstance(), resourceTypes);

    assertThat(res)
        .isEqualTo(
            FileDescriptorProto.newBuilder()
                .addMessageType(
                    DescriptorProto.newBuilder()
                        .setName("ContainedResource")
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("patient")
                                .setNumber(103)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName(".google.foo.Patient")
                                .setOneofIndex(0))
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("foo")
                                .setNumber(147)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName(".google.foo.Foo")
                                .setOneofIndex(0))
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("bar")
                                .setNumber(148)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName(".google.foo.Bar")
                                .setOneofIndex(0))
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("baz")
                                .setNumber(149)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName(".google.foo.Baz")
                                .setOneofIndex(0))
                        .addOneofDecl(OneofDescriptorProto.newBuilder().setName("oneof_resource")))
                .build());
  }

  @Test
  public void addContainedResource_derivedResourcesWithBaseResources_maintainBaseResourceNumbers()
      throws IOException, InvalidFhirException {
    ProtoGenerator generator =
        ProtoGeneratorTestUtils.makeProtoGenerator(
            createNonCorePackage("google.foo"),
            "codes.proto",
            /*coreDepMap=*/ ImmutableMap.of("R4", "spec/fhir_r4_package.zip"),
            /*dependencyLocations=*/ ImmutableSet.of());

    ImmutableList<DescriptorProto> resourceTypes =
        ImmutableList.of(
            buildResourceDescriptor("Foo"),
            // For derived contained resources, make sure to keep the tag numbers from the base file
            // for resources that keep the same name.
            // In other words, "Patient" should keep the tag number (103) of the field in the base
            // contained resources for Patient.
            buildResourceDescriptor("Patient"));

    FileDescriptorProto res =
        generator.addContainedResource(FileDescriptorProto.getDefaultInstance(), resourceTypes);

    assertThat(res)
        .isEqualTo(
            FileDescriptorProto.newBuilder()
                .addMessageType(
                    DescriptorProto.newBuilder()
                        .setName("ContainedResource")
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("patient")
                                .setNumber(103)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName(".google.foo.Patient")
                                .setOneofIndex(0))
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("foo")
                                .setNumber(147)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName(".google.foo.Foo")
                                .setOneofIndex(0))
                        .addOneofDecl(OneofDescriptorProto.newBuilder().setName("oneof_resource")))
                .build());
  }
}
