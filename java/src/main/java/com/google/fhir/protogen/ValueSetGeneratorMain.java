//    Copyright 2019 Google LLC.
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
import com.google.common.io.Files;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.proto.Annotations.FhirVersion;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.r4.core.Bundle;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/** Main class for using the ValueSetGenerator to generate valueset protos. */
final class ValueSetGeneratorMain {

  private static class Args {

    @Parameter(
        names = {"--output_filename"},
        description = "Name for output ValueSet + CodeSystem proto file.")
    private String outputName = null;

    @Parameter(
        names = {"--output_codesystem_filename"},
        description = "Name for output CodeSystem proto file.")
    private String outputCodeSystemName = null;

    @Parameter(
        names = {"--output_valueset_filename"},
        description = "Name for output ValueSet proto file.")
    private String outputValueSetName = null;

    @Parameter(
        names = {"--fhir_definition_dep"},
        description =
            "FhirPackage zips that are dependencies of the valuesets being generated.")
    private Set<String> fhirDefinitionDepList = new HashSet<>();

    @Parameter(
        names = {"--for_codes_in"},
        description = "Generate codes for types used in these bundles.")
    private Set<String> codeUsers = new HashSet<>();

    @Parameter(
        names = {"--eager_mode"},
        description =
            "Include all referenced code systems in --for_codes_in, even if they're not directly"
                + " inlined (e.g. suggested or preferred bindings).")
    private boolean eagerMode = false;

    @Parameter(
        names = {"--input_package"},
        description = "FhirPackage zip to generate for codes and valuesets for.",
        required = true)
    private String inputPackage = null;

    private Set<FhirPackage> getDependencies() throws IOException {
      Set<FhirPackage> packages = new HashSet<>();
      for (String fhirDefinitionDep : fhirDefinitionDepList) {
        packages.add(FhirPackage.load(fhirDefinitionDep));
      }
      if (inputPackage != null) {
        packages.add(FhirPackage.load(inputPackage));
      }
      return packages;
    }
  }

  private ValueSetGeneratorMain() {}

  public static void main(String[] argv) throws IOException {
    // Each non-flag argument is assumed to be an input file.
    Args args = new Args();
    JCommander jcommander = new JCommander(args);
    try {
      jcommander.parse(argv);
    } catch (ParameterException exception) {
      System.err.printf("Invalid usage: %s\n", exception.getMessage());
      System.exit(1);
    }

    FhirPackage inputPackage = FhirPackage.load(args.inputPackage);
    PackageInfo packageInfo = inputPackage.packageInfo;
    FhirVersion fhirVersion = packageInfo.getFhirVersion();

    ValueSetGenerator generator = new ValueSetGenerator(packageInfo, args.getDependencies());

    FileDescriptorProto codesFileDescriptor =
        args.codeUsers.isEmpty()
            ? generator.generateCodeSystemFile(inputPackage)
            : generator.forCodesUsedIn(loadBundles(args.codeUsers, fhirVersion), args.eagerMode);
    FileDescriptorProto valueSetsFileDescriptor =
        args.codeUsers.isEmpty()
            ? generator.generateValueSetFile(inputPackage)
            : generator.forValueSetsUsedIn(loadBundles(args.codeUsers, fhirVersion));

    ProtoFilePrinter printer = new ProtoFilePrinter(packageInfo);

    if (args.outputName != null) {
      System.out.println("Writing " + args.outputName + "...");
      FileDescriptorProto jointFile =
          codesFileDescriptor.toBuilder()
              .addAllMessageType(valueSetsFileDescriptor.getMessageTypeList())
              .build();
      Files.asCharSink(new File(args.outputName), UTF_8).write(printer.print(jointFile));
    }

    if (args.outputCodeSystemName != null) {
      System.out.println("Writing " + args.outputCodeSystemName + "...");
      Files.asCharSink(new File(args.outputCodeSystemName), UTF_8)
          .write(printer.print(codesFileDescriptor));
    }

    if (args.outputValueSetName != null) {
      System.out.println("Writing " + args.outputValueSetName + "...");
      Files.asCharSink(new File(args.outputValueSetName), UTF_8)
          .write(printer.print(valueSetsFileDescriptor));
    }
  }

  private static JsonFormat.Parser getParser(FhirVersion fhirVersion) {
    switch (fhirVersion) {
      case STU3:
        return JsonFormat.getEarlyVersionGeneratorParser();
      case R4:
        return JsonFormat.getParser();
      default:
        throw new IllegalArgumentException(
            "Fhir version unsupported by ValueSetGenerator: " + fhirVersion);
    }
  }

  private static List<Bundle> loadBundles(Set<String> filenames, FhirVersion fhirVersion)
      throws IOException {
    JsonFormat.Parser parser = getParser(fhirVersion);
    List<Bundle> bundles = new ArrayList<>();
    for (String filename : filenames) {
      Bundle.Builder builder = Bundle.newBuilder();
      if (filename.endsWith(".json")) {
        String json = Files.asCharSource(new File(filename), StandardCharsets.UTF_8).read();
        parser.merge(json, builder);
      } else if (filename.endsWith(".zip")) {
        FhirPackage fhirPackage = FhirPackage.load(filename);
        for (StructureDefinition structDef : fhirPackage.structureDefinitions) {
          builder.addEntryBuilder().getResourceBuilder().setStructureDefinition(structDef);
        }
      } else {
        throw new IllegalArgumentException(
            "Filename must be either a .json bundle or .zip archive");
      }
      bundles.add(builder.build());
    }
    return bundles;
  }
}
