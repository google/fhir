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

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableSet;
import com.google.common.io.Files;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.Annotations.FhirVersion;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.proto.ProtoGeneratorAnnotations;
import com.google.fhir.r4.core.SlicingRulesCode;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.MessageOptions;
import com.google.protobuf.DescriptorProtos.OneofDescriptorProto;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.HashSet;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

// TODO(b/244184211): consider adding more specialized test structure definitions that isolate
// individual functionality from ProtoGenerator

/** Unit tests for {@link ProtoGenerator}. */
@RunWith(JUnit4.class)
public class ProtoGeneratorTest {

  private JsonFormat.Parser jsonParser;
  private Runfiles runfiles;
  private final ProtoGenerator r4ProtoGenerator;

  private static ProtoGenerator makeProtoGenerator(
      String packageLocation,
      String codesProtoImport,
      String coreDep,
      ImmutableSet<String> dependencyLocations)
      throws IOException, InvalidFhirException {
    FhirPackage packageToGenerate = FhirPackage.load(packageLocation);
    PackageInfo packageInfo = packageToGenerate.packageInfo;

    Set<FhirPackage> packages = new HashSet<>();
    packages.add(packageToGenerate);
    for (String location : dependencyLocations) {
      packages.add(FhirPackage.load(location));
    }

    packages.add(FhirPackage.load(coreDep));

    return new ProtoGenerator(
        packageInfo,
        codesProtoImport,
        ImmutableSet.copyOf(packages),
        new ValueSetGenerator(packageInfo, packages));
  }

  public ProtoGeneratorTest() throws Exception {
    r4ProtoGenerator =
        makeProtoGenerator(
            "spec/fhir_r4_package.zip",
            "codes.proto",
            "spec/fhir_r4_package.zip",
            ImmutableSet.of() /* no dependencies */);
  }

  /** Read the specified file from the testdata directory into a String. */
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
  }

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

  @Test
  public void addContainedResource_noResourceTypes_emptyFileDescriptor() {
    FileDescriptorProto res =
        r4ProtoGenerator.addContainedResource(
            FileDescriptorProto.getDefaultInstance(), /* resourceTypes= */ ImmutableList.of());

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
  public void addContainedResource_derivedResources_sorted()
      throws IOException, InvalidFhirException {
    ProtoGenerator generator =
        makeProtoGenerator(
            createNonCorePackage("google.foo"),
            "codes.proto",
            "spec/fhir_r4_package.zip",
            /* dependencyLocations= */ ImmutableSet.of());

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
                                .setName("bar")
                                .setNumber(147)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName(".google.foo.Bar")
                                .setOneofIndex(0))
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("baz")
                                .setNumber(148)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName(".google.foo.Baz")
                                .setOneofIndex(0))
                        .addField(
                            FieldDescriptorProto.newBuilder()
                                .setName("foo")
                                .setNumber(149)
                                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                                .setTypeName(".google.foo.Foo")
                                .setOneofIndex(0))
                        .addOneofDecl(OneofDescriptorProto.newBuilder().setName("oneof_resource")))
                .build());
  }

  @Test
  public void addContainedResource_derivedResourcesWithBaseResources_maintainBaseResourceNumbers()
      throws IOException, InvalidFhirException {
    ProtoGenerator generator =
        makeProtoGenerator(
            createNonCorePackage("google.foo"),
            "codes.proto",
            "spec/fhir_r4_package.zip",
            /* dependencyLocations= */ ImmutableSet.of());

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

  @Test
  public void generateProto_genericFieldAddedForOpenExtension()
      throws IOException, InvalidFhirException {
    ProtoGenerator generator =
        makeProtoGenerator(
            createNonCorePackage("google.foo"),
            "codes.proto",
            "spec/fhir_r4_package.zip",
            /* dependencyLocations= */ ImmutableSet.of());

    StructureDefinition citizenshipExtension =
        readStructureDefinition("patient-citizenship", FhirVersion.R4);

    // By default, the patient citizenship has open slicing on sub-extensions.
    assertThat(
            GeneratorUtils.getElementById(
                    "Extension.extension", citizenshipExtension.getSnapshot().getElementList())
                .getSlicing()
                .getRules()
                .getValue())
        .isEqualTo(SlicingRulesCode.Value.OPEN);

    DescriptorProto citizenshipMessage = generator.generateProto(citizenshipExtension);
    assertThat(
            citizenshipMessage.getFieldList().stream()
                .filter(field -> field.getName().equals("extension"))
                .count())
        .isEqualTo(1);
  }

  @Test
  public void generateProto_noGenericFieldAddedForClosedExtension()
      throws IOException, InvalidFhirException {
    ProtoGenerator generator =
        makeProtoGenerator(
            createNonCorePackage("google.foo"),
            "codes.proto",
            "spec/fhir_r4_package.zip",
            /* dependencyLocations= */ ImmutableSet.of());

    // Make a version of citizenship extension that is closed on extension slicing
    StructureDefinition.Builder citizenshipExtension =
        readStructureDefinition("patient-citizenship", FhirVersion.R4).toBuilder();
    citizenshipExtension.getSnapshotBuilder().getElementBuilderList().stream()
        .filter(elem -> elem.getId().getValue().equals("Extension.extension"))
        .findFirst()
        .get()
        .getSlicingBuilder()
        .getRulesBuilder()
        .setValue(SlicingRulesCode.Value.CLOSED);

    DescriptorProto citizenshipMessage = generator.generateProto(citizenshipExtension.build());
    assertThat(
            citizenshipMessage.getFieldList().stream()
                .filter(field -> field.getName().equals("extension"))
                .findAny()
                .isPresent())
        .isFalse();

    System.out.println(citizenshipMessage);

    assertThat(
            citizenshipMessage.getFieldList().stream()
                .filter(
                    field ->
                        field.getOptions().hasExtension(ProtoGeneratorAnnotations.reservedReason)
                            && field.getNumber() == 2)
                .count())
        .isEqualTo(1);
  }

  @Test
  public void generateProto_genericFieldAddedForOpenCoding()
      throws IOException, InvalidFhirException {
    ProtoGenerator generator =
        makeProtoGenerator(
            createNonCorePackage("google.foo"),
            "codes.proto",
            "spec/fhir_r4_package.zip",
            /* dependencyLocations= */ ImmutableSet.of());

    StructureDefinition bloodPressureProfile = readStructureDefinition("bp", FhirVersion.R4);

    // By default, the Blood Pressure has open slicing on Observation.code.coding.
    assertThat(
            GeneratorUtils.getElementById(
                    "Observation.code.coding", bloodPressureProfile.getSnapshot().getElementList())
                .getSlicing()
                .getRules()
                .getValue())
        .isEqualTo(SlicingRulesCode.Value.OPEN);

    DescriptorProto bpMessage = generator.generateProto(bloodPressureProfile);
    assertThat(
            bpMessage.getNestedType(1).getFieldList().stream()
                .filter(field -> field.getName().equals("coding"))
                .count())
        .isEqualTo(1);
  }

  @Test
  public void generateProto_genericFieldNotAddedForClosedCoding()
      throws IOException, InvalidFhirException {
    ProtoGenerator generator =
        makeProtoGenerator(
            createNonCorePackage("google.foo"),
            "codes.proto",
            "spec/fhir_r4_package.zip",
            /* dependencyLocations= */ ImmutableSet.of());

    // Make a version of Blood Pressure that has closed slicing on Observation.code.coding.
    StructureDefinition.Builder bloodPressureProfile =
        readStructureDefinition("bp", FhirVersion.R4).toBuilder();
    bloodPressureProfile.getSnapshotBuilder().getElementBuilderList().stream()
        .filter(elem -> elem.getId().getValue().equals("Observation.code.coding"))
        .findFirst()
        .get()
        .getSlicingBuilder()
        .getRulesBuilder()
        .setValue(SlicingRulesCode.Value.CLOSED);

    DescriptorProto bpMessage = generator.generateProto(bloodPressureProfile.build());
    assertThat(
            bpMessage.getNestedType(1).getFieldList().stream()
                .filter(field -> field.getName().equals("coding"))
                .findAny()
                .isPresent())
        .isFalse();
    assertThat(
            bpMessage.getNestedType(1).getFieldList().stream()
                .filter(
                    field ->
                        field.getOptions().hasExtension(ProtoGeneratorAnnotations.reservedReason)
                            && field.getNumber() == 3)
                .count())
        .isEqualTo(1);
  }
}
