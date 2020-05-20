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

import static com.google.common.collect.ImmutableSet.toImmutableSet;
import static com.google.common.truth.Truth.assertWithMessage;
import static java.util.stream.Collectors.toList;

import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.google.fhir.common.ProtoUtils;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.proto.PackageInfo.FileSplittingBehavior;
import com.google.fhir.proto.ProtoGeneratorAnnotations;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldOptions;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.MessageOptions;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.ExtensionRegistry;
import java.io.File;
import java.io.IOException;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/** Utilities for tests involving the ProtoGenerator. */
public final class ProtoGeneratorTestUtils {

  private ProtoGeneratorTestUtils() {}

  public static void initializeRegistry(ExtensionRegistry registry) throws IOException {
    Annotations.registerAllExtensions(registry);
    ProtoGeneratorAnnotations.registerAllExtensions(registry);
  }

  public static ProtoGenerator makeProtoGenerator(
      String packageLocation,
      ImmutableMap<String, String> coreDepMap,
      ImmutableSet<String> dependencyLocations)
      throws IOException {
    FhirPackage packageToGenerate = FhirPackage.load(packageLocation);
    PackageInfo packageInfo = packageToGenerate.packageInfo;

    Set<FhirPackage> packages = new HashSet<>();
    packages.add(packageToGenerate);
    for (String location : dependencyLocations) {
      packages.add(FhirPackage.load(location));
    }

    String coreDep = coreDepMap.get(packageInfo.getFhirVersion().toString());
    if (coreDep == null) {
      throw new IllegalArgumentException(
          "Unable to load core dep for fhir version: " + packageInfo.getFhirVersion());
    }
    packages.add(FhirPackage.load(coreDep));

    return new ProtoGenerator(
        packageInfo, ImmutableSet.copyOf(packages), new ValueSetGenerator(packageInfo, packages));
  }

  public static void testGeneratedProto(
      String packageLocation,
      String ruleName,
      ImmutableMap<String, String> coreDepMap,
      ImmutableSet<String> dependencyLocations,
      ImmutableSet<String> additionalImports)
      throws Exception {
    FhirPackage inputPackage = FhirPackage.load(packageLocation);

    // TODO: Enable once protogeneration is working for STU3
    if (inputPackage.packageInfo.getFhirVersion()
        == com.google.fhir.proto.Annotations.FhirVersion.STU3) {
      return;
    }

    ProtoGenerator generator = makeProtoGenerator(packageLocation, coreDepMap, dependencyLocations);

    // TODO: Also test generated extension files when split.
    boolean hasSplitExtensionFile =
        inputPackage.packageInfo.getFileSplittingBehavior()
            == FileSplittingBehavior.SEPARATE_EXTENSIONS;

    List<StructureDefinition> typesToGenerate =
        hasSplitExtensionFile
            ? inputPackage.structureDefinitions.stream()
                .filter(def -> !GeneratorUtils.isExtensionProfile(def))
                .collect(toList())
            : inputPackage.structureDefinitions;
    FileDescriptorProto generatedFile = generator.generateFileDescriptor(typesToGenerate);

    if (hasSplitExtensionFile) {
      generatedFile =
          generatedFile.toBuilder()
              .addDependency(
                  new File(packageLocation).getParent() + "/" + ruleName + "_extensions.proto")
              .build();
    }

    if (inputPackage.packageInfo.getLocalContainedResource()) {
      generatedFile =
          generator.addContainedResource(generatedFile, generatedFile.getMessageTypeList());
    }

    String sampleMessageName =
        inputPackage.packageInfo.getJavaProtoPackage()
            + "."
            + GeneratorUtils.getTypeName(
                typesToGenerate.get(0), inputPackage.packageInfo.getFhirVersion());

    FileDescriptorProto fileInSource =
        ((Descriptor)
                Class.forName(sampleMessageName)
                    .getMethod("getDescriptor")
                    .invoke(null /* static method, no instance*/))
            .getFile()
            .toProto();

    generatedFile =
        withSortedMessages(
            withAdditionalImports(
                withoutProtoGeneratorAnnotations(generatedFile), additionalImports));
    fileInSource = withSortedMessages(withoutProtoGeneratorAnnotations(fileInSource));

    assertWithMessage(
            "\n***\nGeneratedProtoTest failed for profile set `"
                + ruleName
                + "` from "
                + packageLocation
                + ".\n"
                + "If you modified either that package or the ProtoGenerator, make sure to"
                + " regenerate this target.  For any questions, contact fhir-team@google.com\n"
                + "***\n")
        .that(generatedFile)
        .isEqualTo(fileInSource);
  }

  public static FileDescriptorProto withAdditionalImports(
      FileDescriptorProto file, Set<String> imports) {
    FileDescriptorProto.Builder builder = file.toBuilder();
    for (String additionalImport : imports) {
      builder.addDependency(new File(additionalImport).toString());
    }
    return builder.build();
  }

  public static FileDescriptorProto withSortedMessages(FileDescriptorProto file) {
    return file.toBuilder()
        .clearMessageType()
        .addAllMessageType(
            file.getMessageTypeList().stream()
                .sorted(Comparator.comparing(DescriptorProto::getName))
                .collect(toList()))
        .clearDependency()
        .addAllDependency(file.getDependencyList().stream().sorted().collect(toList()))
        .build();
  }

  private static FileDescriptorProto withoutProtoGeneratorAnnotations(FileDescriptorProto file) {
    FileDescriptorProto.Builder builder = file.toBuilder().clearName();
    // TODO: remove this once GoPackage is respected by ProtoFilePrinter
    builder.getOptionsBuilder().clearGoPackage();
    builder
        .getMessageTypeBuilderList()
        .forEach(ProtoGeneratorTestUtils::stripProtoGeneratorAnnotations);
    return builder.build();
  }

  private static final ImmutableSet<FieldDescriptor> MESSAGE_ANNOTATIONS =
      ProtoGeneratorAnnotations.getDescriptor().getExtensions().stream()
          .filter(
              annotation ->
                  ProtoUtils.areSameMessageType(
                      annotation.getContainingType(), MessageOptions.getDescriptor()))
          .collect(toImmutableSet());

  private static final ImmutableSet<FieldDescriptor> FIELD_ANNOTATIONS =
      ProtoGeneratorAnnotations.getDescriptor().getExtensions().stream()
          .filter(
              annotation ->
                  ProtoUtils.areSameMessageType(
                      annotation.getContainingType(), FieldOptions.getDescriptor()))
          .collect(toImmutableSet());

  private static void stripProtoGeneratorAnnotations(DescriptorProto.Builder builder) {
    MESSAGE_ANNOTATIONS.forEach(ext -> builder.getOptionsBuilder().clearField(ext));
    if (builder.getOptions().equals(MessageOptions.getDefaultInstance())) {
      builder.clearOptions();
    }

    // TODO: Currently the native proto representation of reserved fields is different
    // from what the proto file printer expects, so we strip both.  These should be reconciled.
    builder.clearReservedRange();
    List<FieldDescriptorProto> allFields = builder.build().getFieldList();
    builder.clearField();
    allFields.forEach(
        field -> {
          if (!field.getOptions().hasExtension(ProtoGeneratorAnnotations.reservedReason)) {
            FieldDescriptorProto.Builder fieldBuilder = field.toBuilder();
            FIELD_ANNOTATIONS.forEach(
                ext -> {
                  fieldBuilder.getOptionsBuilder().clearField(ext);
                });
            if (fieldBuilder.getOptions().equals(FieldOptions.getDefaultInstance())) {
              fieldBuilder.clearOptions();
            }
            builder.addField(fieldBuilder);
          }
        });

    builder
        .getNestedTypeBuilderList()
        .forEach(ProtoGeneratorTestUtils::stripProtoGeneratorAnnotations);
  }
}
