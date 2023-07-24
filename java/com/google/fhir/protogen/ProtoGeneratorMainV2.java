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
import com.google.common.base.CaseFormat;
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

    ProtoFilePrinter printer = new ProtoFilePrinter(config);

    try (ZipOutputStream zipOutputStream =
        new ZipOutputStream(new FileOutputStream(new File(args.outputDirectory, "output.zip")))) {
      try {
        addEntry(zipOutputStream, printer, codesFileDescriptor, "codes.proto");
        addEntry(zipOutputStream, printer, valueSetsFileDescriptor, "valuesets.proto");
        addEntry(
            zipOutputStream,
            printer,
            generator.generateDatatypesFileDescriptor(),
            "datatypes.proto");

        for (StructureDefinition structDef : inputPackage.structureDefinitions()) {
          if (structDef.getKind().getValue() == StructureDefinitionKindCode.Value.RESOURCE
              && structDef.getDerivation().getValue() == TypeDerivationRuleCode.Value.SPECIALIZATION
              && !structDef.getUrl().getValue().equals(BUNDLE_STRUCTURE_DEFINITION_URL)) {
            addEntry(
                zipOutputStream,
                printer,
                generator.generateResourceFileDescriptor(structDef),
                "resources/" + resourceNameToFileName(GeneratorUtils.getTypeName(structDef)));
          }
        }

      } finally {
        zipOutputStream.closeEntry();
      }
    }
  }

  private static void addEntry(
      ZipOutputStream zipOutputStream,
      ProtoFilePrinter printer,
      FileDescriptorProto fileDescriptor,
      String name)
      throws IOException {
    zipOutputStream.putNextEntry(new ZipEntry(name));
    byte[] entryBytes = printer.print(fileDescriptor).getBytes(UTF_8);
    zipOutputStream.write(entryBytes, 0, entryBytes.length);
  }

  private String resourceNameToFileName(String resourceName) {
    return CaseFormat.UPPER_CAMEL.to(
            CaseFormat.LOWER_UNDERSCORE,
            GeneratorUtils.resolveAcronyms(GeneratorUtils.toFieldTypeCase(resourceName)))
        + ".proto";
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
