//    Copyright 2023 Google Inc.
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
import static com.google.common.truth.extensions.proto.ProtoTruth.assertThat;
import static com.google.fhir.protogen.ProtoGeneratorTestUtils.cleaned;
import static com.google.fhir.protogen.ProtoGeneratorTestUtils.sorted;
import static java.util.stream.Collectors.toList;

import com.google.common.collect.ImmutableList;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.proto.ProtogenConfig;
import com.google.fhir.r4.core.Bundle;
import com.google.fhir.r4.core.ContainedResource;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.StructureDefinitionKindCode;
import com.google.fhir.r4.core.TypeDerivationRuleCode;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.testing.junit.testparameterinjector.TestParameter;
import com.google.testing.junit.testparameterinjector.TestParameterInjector;
import com.google.testing.junit.testparameterinjector.TestParameterValuesProvider;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(TestParameterInjector.class)
public final class ProtoGeneratorV2Test {
  private static FhirPackage r4Package = null;
  private static FhirPackage r5Package = null;

  /**
   * Gets the R4 package. Lazy loaded to avoid questions of class/test/parameter initialization
   * order.
   */
  private static FhirPackage getR4Package() throws IOException, InvalidFhirException {
    if (r4Package == null) {
      r4Package =
          FhirPackage.load(
              "external/hl7.fhir.r4.core_4.0.1/file/hl7.fhir.r4.core@4.0.1.tgz",
              /* no packageInfo proto */ null,
              /* ignoreUnrecognizedFieldsAndCodes= */ true);
    }
    return r4Package;
  }

  /**
   * Gets the R5 package. Lazy loaded to avoid questions of class/test/parameter initialization
   * order.
   */
  private static FhirPackage getR5Package() throws IOException, InvalidFhirException {
    if (r5Package == null) {
      r5Package =
          FhirPackage.load(
              "external/hl7.fhir.r5.core_5.0.0/file/hl7.fhir.r5.core@5.0.0.tgz",
              /* no packageInfo proto */ null,
              /* ignoreUnrecognizedFieldsAndCodes= */ true);
    }
    return r5Package;
  }

  public static ProtoGeneratorV2 makeR4ProtoGenerator() throws Exception {
    ProtogenConfig config =
        ProtogenConfig.newBuilder()
            .setProtoPackage("google.fhir.r4.core")
            .setJavaProtoPackage("com.google.fhir.r4.core")
            .setLicenseDate("1995")
            .setSourceDirectory("proto/google/fhir/proto/r4/core")
            .build();

    FhirPackage r4Package = getR4Package();
    return new ProtoGeneratorV2(r4Package, config);
  }

  public static ProtoGeneratorV2 makeR5ProtoGenerator() throws Exception {
    ProtogenConfig config =
        ProtogenConfig.newBuilder()
            .setProtoPackage("google.fhir.r5.core")
            .setJavaProtoPackage("com.google.fhir.r5.core")
            .setLicenseDate("1995")
            .setSourceDirectory("proto/google/fhir/proto/r5/core")
            .setLegacyRetagging(true)
            .build();
    FhirPackage r5Package = getR5Package();
    return new ProtoGeneratorV2(r5Package, config);
  }

  /** Cleans up some datatypes that are hardcoded rather than generated. */
  FileDescriptorProto filtered(FileDescriptorProto file) {
    FileDescriptorProto.Builder builder = file.toBuilder().clearMessageType();
    for (DescriptorProto message : file.getMessageTypeList()) {
      // Some datatypes are still hardcoded and added in the generation script, rather than by
      // the protogenerator.
      if (!message.getName().equals("CodingWithFixedCode")
          && !message.getName().equals("Extension")) {
        builder.addMessageType(message);
      }
    }
    return builder.build();
  }

  private static boolean isCoreResource(StructureDefinition def) {
    return def.getKind().getValue() == StructureDefinitionKindCode.Value.RESOURCE
        && def.getDerivation().getValue() == TypeDerivationRuleCode.Value.SPECIALIZATION
        && !def.getAbstract().getValue();
  }

  /**
   * Tests that generating the R4 Datatypes file using generateDatatypesFileDescriptor generates
   * descriptors that match the currently-checked in R4 datatypes. This serves as both a whole lot
   * of unit tests, and as a regression test to guard against checking in a change that would alter
   * the R4 Core protos.
   */
  @Test
  public void r4RegressionTest_generateLegacyDatatypesFileDescriptor() throws Exception {
    List<String> resourceNames =
        ContainedResource.getDescriptor().getFields().stream()
            .map(field -> field.getMessageType().getName())
            .collect(toList());

    // Old R4 had a few reference types to non-concrete resources.  Include these to be backwards
    // compatible during transition.
    // TODO(b/299644315): Consider dropping these fields and reserving the field numbers instead.
    resourceNames.add("DomainResource");
    resourceNames.add("MetadataResource");

    FileDescriptorProto descriptor =
        makeR4ProtoGenerator().generateDatatypesFileDescriptor(resourceNames);
    assertThat(sorted(cleaned(descriptor)))
        .ignoringRepeatedFieldOrder()
        .isEqualTo(
            sorted(
                cleaned(
                    filtered(com.google.fhir.r4.core.String.getDescriptor().getFile().toProto()))));
  }

  /**
   * Parameter provider for getting all resources that have a file generated for a given package.
   */
  private abstract static class ResourceProvider extends TestParameterValuesProvider {
    /** Returns the expected number of resources from the package, as a sanity check. */
    protected abstract int getNumberOfExpectedResources();

    /** Gets the package under test. */
    protected abstract FhirPackage getPackage() throws IOException, InvalidFhirException;

    @Override
    public List<StructureDefinition> provideValues(Context context) {
      FhirPackage fhirPackage;
      try {
        fhirPackage = getPackage();
      } catch (IOException | InvalidFhirException e) {
        throw new AssertionError(e);
      }

      List<StructureDefinition> structDefs = new ArrayList<>();
      for (StructureDefinition structDef : fhirPackage.structureDefinitions()) {
        if (isCoreResource(structDef)
            // Skip bundle since it is part of bundle_and_contained_resource
            && !structDef.getName().getValue().equals("Bundle")) {
          structDefs.add(structDef);
        }
      }
      // Sanity check that we're actually testing all the resources.
      int expectedResourcesCount = getNumberOfExpectedResources();
      if (structDefs.size() != expectedResourcesCount) {
        throw new AssertionError(
            "Expected " + expectedResourcesCount + " resources, got " + structDefs.size());
      }
      return structDefs;
    }
  }

  private static final class R4ResourceProvider extends ResourceProvider {
    @Override
    protected int getNumberOfExpectedResources() {
      return 145;
    }

    @Override
    protected FhirPackage getPackage() throws IOException, InvalidFhirException {
      return getR4Package();
    }
  }

  /**
   * Tests that generating the R4 Resource files using generateResourceFileDescriptor generates
   * descriptors that match the currently-checked in R4 Resource files. This serves as both a whole
   * lot of unit tests, and as a regression test to guard against checking in a change that would
   * alter the R4 Core protos.
   *
   * <p>This test is parameterized on resource StructureDefinition, as provided by
   * `ResourceProvider`, which provides one resource StructureDefinition per test.
   */
  @Test
  public void r4RegressionTest_generateResourceFileDescriptor(
      @TestParameter(valuesProvider = R4ResourceProvider.class) StructureDefinition resource)
      throws Exception {
    ProtoGeneratorV2 protoGenerator = makeR4ProtoGenerator();

    FileDescriptorProto file =
        protoGenerator.generateResourceFileDescriptor(
            resource, getR4Package().getSemanticVersion());

    // Get the checked-in FileDescriptor for the resource.
    String name = file.getOptions().getJavaPackage() + "." + file.getMessageType(0).getName();
    FileDescriptorProto descriptor =
        ((Descriptor) Class.forName(name).getMethod("getDescriptor").invoke(null))
            .getFile()
            .toProto();

    assertThat(cleaned(file)).ignoringRepeatedFieldOrder().isEqualTo(cleaned(descriptor));
  }

  /**
   * Tests that generating the R4 bundle_and_contained_resource file using
   * generateBundleAndContainedResource generates file descriptors that match the currently-checked
   * in R4 bundle_and_contained_resources file. This serves as both a unit tests, and as a
   * regression test to guard against checking in a change that would alter the R4 Core protos.
   */
  @Test
  public void r4RegressionTest_generateBundleAndContainedResource() throws Exception {
    ProtoGeneratorV2 protoGenerator = makeR4ProtoGenerator();

    List<String> resourceNames = new ArrayList<>();
    StructureDefinition bundle = null;
    for (StructureDefinition structDef : r4Package.structureDefinitions()) {
      if (structDef.getName().getValue().equals("Bundle")) {
        bundle = structDef;
      }
      if (isCoreResource(structDef)) {
        resourceNames.add(structDef.getName().getValue());
      }
    }

    if (bundle == null) {
      throw new AssertionError("No Bundle found");
    }

    assertThat(
            cleaned(
                protoGenerator.generateBundleAndContainedResource(
                    bundle, getR4Package().getSemanticVersion(), resourceNames, 0)))
        .ignoringRepeatedFieldOrder()
        .isEqualTo(cleaned(Bundle.getDescriptor().getFile().toProto()));
  }

  /**
   * Tests that generating the R5 Datatypes file using generateDatatypesFileDescriptor generates
   * descriptors that match the currently-checked in R5 datatypes. This serves as both a whole lot
   * of unit tests, and as a regression test to guard against checking in a change that would alter
   * the R5 Core protos.
   */
  @Test
  public void r5regressionTest_generateDatatypesFileDescriptor() throws Exception {
    ImmutableList<String> resourceNames =
        com.google.fhir.r5.core.ContainedResource.getDescriptor().getFields().stream()
            .map(field -> field.getMessageType().getName())
            .collect(toImmutableList());
    FileDescriptorProto descriptor =
        makeR5ProtoGenerator().generateDatatypesFileDescriptor(resourceNames);
    assertThat(sorted(cleaned(descriptor)))
        .ignoringRepeatedFieldOrder()
        .isEqualTo(
            sorted(
                cleaned(
                    filtered(com.google.fhir.r5.core.String.getDescriptor().getFile().toProto()))));
  }

  private static final class R5ResourceProvider extends ResourceProvider {
    @Override
    protected int getNumberOfExpectedResources() {
      return 157;
    }

    @Override
    protected FhirPackage getPackage() throws IOException, InvalidFhirException {
      return getR5Package();
    }
  }

  /**
   * Tests that generating the R5 Resource files using generateResourceFileDescriptor generates
   * descriptors that match the currently-checked in R5 Resource files. This serves as both a whole
   * lot of unit tests, and as a regression test to guard against checking in a change that would
   * alter the R5 Core protos.
   *
   * <p>This test is parameterized on resource StructureDefinition, as provided by
   * `ResourceProvider`, which provides one resource StructureDefinition per test.
   */
  @Test
  public void r5RegressionTest_generateResourceFileDescriptor(
      @TestParameter(valuesProvider = R5ResourceProvider.class) StructureDefinition resource)
      throws Exception {
    ProtoGeneratorV2 protoGenerator = makeR5ProtoGenerator();

    FileDescriptorProto file =
        protoGenerator.generateResourceFileDescriptor(
            resource, getR5Package().getSemanticVersion());

    // Get the checked-in FileDescriptor for the resource.
    String name = file.getOptions().getJavaPackage() + "." + file.getMessageType(0).getName();
    FileDescriptorProto descriptor =
        ((Descriptor) Class.forName(name).getMethod("getDescriptor").invoke(null))
            .getFile()
            .toProto();

    assertThat(cleaned(file)).ignoringRepeatedFieldOrder().isEqualTo(cleaned(descriptor));
  }

  /**
   * Tests that generating the R5 bundle_and_contained_resource file using
   * generateBundleAndContainedResource generates file descriptors that match the currently-checked
   * in R5 bundle_and_contained_resources file. This serves as both a unit tests, and as a
   * regression test to guard against checking in a change that would alter the R5 Core protos.
   */
  @Test
  public void r5RegressionTest_generateBundleAndContainedResource() throws Exception {
    ProtoGeneratorV2 protoGenerator = makeR5ProtoGenerator();

    List<String> resourceNames = new ArrayList<>();
    StructureDefinition bundle = null;
    for (StructureDefinition structDef : r5Package.structureDefinitions()) {
      if (structDef.getName().getValue().equals("Bundle")) {
        bundle = structDef;
      }
      if (isCoreResource(structDef)) {
        resourceNames.add(structDef.getName().getValue());
      }
    }

    if (bundle == null) {
      throw new AssertionError("No Bundle found");
    }

    assertThat(
            cleaned(
                protoGenerator.generateBundleAndContainedResource(
                    bundle, getR5Package().getSemanticVersion(), resourceNames, 5000)))
        .ignoringRepeatedFieldOrder()
        .isEqualTo(cleaned(com.google.fhir.r5.core.Bundle.getDescriptor().getFile().toProto()));
  }
}
