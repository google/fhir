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

import static java.nio.charset.StandardCharsets.UTF_8;

import com.beust.jcommander.JCommander;
import com.beust.jcommander.Parameter;
import com.beust.jcommander.ParameterException;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.proto.Annotations.FhirVersion;
import com.google.fhir.proto.ProtogenConfig;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.StructureDefinitionKindCode;
import com.google.fhir.r4.core.TypeDerivationRuleCode;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/**
 * A class that runs ProtoGenerator on the specified inputs, turning FHIR StructureDefinition files
 * into proto descriptors. Depending on settings, either the descriptors, the .proto file, or both
 * will be emitted.
 */
class ProtoGeneratorMainV2 {

  private final Args args;

  private static final String BUNDLE_STRUCTURE_DEFINITION_URL =
      "http://hl7.org/fhir/StructureDefinition/Bundle";

  private static class Args {
    @Parameter(
        names = {"--output_directory"},
        description =
            "The directory in the source tree that the proto files will be located. "
                + "This allows for intra-package imports, like codes and extensions.")
    private String outputDirectory = null;

    @Parameter(
        names = {"--input_package"},
        description = "Input FHIR package",
        required = true)
    private String inputPackageLocation = null;

    @Parameter(
        names = {"--proto_package"},
        description = "Proto package for generated messages",
        required = true)
    private String protoPackage = null;

    @Parameter(
        names = {"--java_proto_package"},
        description = "Java proto package for generated messages",
        required = true)
    private String javaProtoPackage = null;

    @Parameter(
        names = {"--license_date"},
        description = "Date to use for Apache License",
        required = true)
    private String licenseDate = null;

    @Parameter(
        names = {"--fhir_version"},
        description = "FHIR version annotation to add to the generated file.",
        required = true)
    // TODO(b/292116008): Remove reliance on precompiled enums.
    private FhirVersion fhirVersion = null;

    @Parameter(
        names = {"--contained_resource_offset"},
        description =
            "Field number offset for ContainedResources.  This is used to ensure that"
                + " ContainedResources from different versions of FHIR don't use overlapping"
                + " numbers, so they can eventually be combined.  See: go/StableFhirProtos")
    private int containedResourceOffset = 0;

    @Parameter(
        names = {"--add_search_parameters"},
        description = "If true, add search parameter annotations.  Defaults to false.")
    private boolean addSearchParameters = false;

    @Parameter(
        names = {"--legacy_datatype_generation"},
        description =
            "If true, skips generating Element, Reference, and Extension, which are hardcoded in "
                + "legacy behavior.  Defaults to false.")
    private boolean legacyDatatypeGeneration = false;
  }

  ProtoGeneratorMainV2(Args args) {
    this.args = args;
  }

  void run() throws IOException, InvalidFhirException {
    FhirPackage inputPackage = FhirPackage.load(args.inputPackageLocation);
    ProtogenConfig config =
        ProtogenConfig.newBuilder()
            .setProtoPackage(args.protoPackage)
            .setJavaProtoPackage(args.javaProtoPackage)
            .setLicenseDate(args.licenseDate)
            .setSourceDirectory(args.outputDirectory)
            .setFhirVersion(args.fhirVersion)
            .build();

    // Generate the proto file.
    System.out.println("Generating proto descriptors...");

    ValueSetGeneratorV2 valueSetGenerator = new ValueSetGeneratorV2(config, inputPackage);
    FileDescriptorProto codesFileDescriptor = valueSetGenerator.forCodesUsedIn(inputPackage);
    FileDescriptorProto valueSetsFileDescriptor =
        valueSetGenerator.forValueSetsUsedIn(inputPackage);

    ProtoGeneratorV2 generator =
        new ProtoGeneratorV2(
            config,
            inputPackage,
            valueSetGenerator.getBoundCodeGenerator(codesFileDescriptor, valueSetsFileDescriptor));

    if (args.addSearchParameters) {
      generator.addSearchParameters();
    }

    ProtoFilePrinter printer = new ProtoFilePrinter(config);

    try (ZipOutputStream zipOutputStream =
        new ZipOutputStream(new FileOutputStream(new File(args.outputDirectory, "output.zip")))) {
      try {
        addEntry(zipOutputStream, printer, config, codesFileDescriptor, "codes.proto");
        addEntry(zipOutputStream, printer, config, valueSetsFileDescriptor, "valuesets.proto");

        List<String> resourceNames = new ArrayList<>();
        StructureDefinition bundleDefinition = null;
        // Iterate over all non-bundle Resources, and generate a single file per resource.
        // Aggregate resource names for use in generating a "Bundle and ContainedResource" file,
        // as well as generating a typed reference datatype.
        for (StructureDefinition structDef : inputPackage.structureDefinitions()) {
          if (structDef.getKind().getValue() == StructureDefinitionKindCode.Value.RESOURCE
              && structDef.getDerivation().getValue() == TypeDerivationRuleCode.Value.SPECIALIZATION
              && !structDef.getAbstract().getValue()) {
            String resourceName = GeneratorUtils.getTypeName(structDef);
            resourceNames.add(resourceName);
            if (structDef.getUrl().getValue().equals(BUNDLE_STRUCTURE_DEFINITION_URL)) {
              // Don't make a bundle resource - it will be generated with the ContainedResource.
              bundleDefinition = structDef;
            } else {
              addEntry(
                  zipOutputStream,
                  printer,
                  config,
                  generator.generateResourceFileDescriptor(structDef),
                  "resources/" + GeneratorUtils.resourceNameToFileName(resourceName));
            }
          }
        }

        // Generate the "Bundle and Contained Resource" file.
        addEntry(
            zipOutputStream,
            printer,
            config,
            generator.generateBundleAndContainedResource(
                bundleDefinition, resourceNames, args.containedResourceOffset),
            "resources/bundle_and_contained_resource.proto");

        // Generate the Datatypes file.  Pass all resource names, for use in generating the
        // Reference datatype.
        addEntry(
            zipOutputStream,
            printer,
            config,
            args.legacyDatatypeGeneration
                ? generator.generateLegacyDatatypesFileDescriptor(resourceNames)
                : generator.generateDatatypesFileDescriptor(resourceNames),
            "datatypes.proto");
      } finally {
        zipOutputStream.closeEntry();
      }
    }
  }

  private static void addEntry(
      ZipOutputStream zipOutputStream,
      ProtoFilePrinter printer,
      ProtogenConfig config,
      FileDescriptorProto fileDescriptor,
      String name)
      throws IOException {
    fileDescriptor = GeneratorUtils.setGoPackage(fileDescriptor, config.getSourceDirectory(), name);
    zipOutputStream.putNextEntry(new ZipEntry(name));
    byte[] entryBytes = printer.print(fileDescriptor).getBytes(UTF_8);
    zipOutputStream.write(entryBytes, 0, entryBytes.length);
  }

  public static void main(String[] argv) throws IOException, InvalidFhirException {
    // Each non-flag argument is assumed to be an input file.
    Args args = new Args();
    JCommander jcommander = new JCommander(args);
    try {
      jcommander.parse(argv);
    } catch (ParameterException exception) {
      System.err.printf("Invalid usage: %s\n", exception.getMessage());
      System.exit(1);
    }
    new ProtoGeneratorMainV2(args).run();
  }
}
