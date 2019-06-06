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

package com.google.fhir.examples;

import static java.nio.charset.StandardCharsets.UTF_8;

import com.beust.jcommander.JCommander;
import com.beust.jcommander.Parameter;
import com.beust.jcommander.ParameterException;
import com.google.common.io.Files;
import com.google.fhir.proto.Annotations.FhirVersion;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.r4.proto.Bundle;
import com.google.fhir.r4.proto.CodeSystem;
import com.google.fhir.r4.proto.ContainedResource;
import com.google.fhir.r4.proto.ValueSet;
import com.google.fhir.stu3.FileUtils;
import com.google.fhir.stu3.JsonFormat;
import com.google.fhir.stu3.ProtoFilePrinter;
import com.google.fhir.stu3.ValueSetGenerator;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.HashSet;
import java.util.Set;

/** Main class for using the ValueSetGenerator to generate valueset protos. */
final class ValueSetGeneratorMain {

  private static class Args {

    @Parameter(
        names = {"--valueset_bundle"},
        description = "Bundle containing CodeSystem and ValueSet definitions.")
    private Set<String> valueSetBundleFiles = new HashSet<>();

    @Parameter(
        names = {"--codesystem_file"},
        description = "File containing CodeSystem definition.")
    private Set<String> codeSystemFiles = new HashSet<>();

    @Parameter(
        names = {"--valueset_file"},
        description = "File containing ValueSet definition.")
    private Set<String> valueSetFiles = new HashSet<>();

    @Parameter(
        names = {"--output_filename"},
        description = "Name for output Value Set proto file.",
        required = true)
    private String outputName = null;

    @Parameter(
        names = {"--package_info"},
        description = "Prototxt containing google.fhir.proto.PackageInfo",
        required = true)
    private String packageInfo = null;

    @Parameter(
        names = {"--fhir_proto_root"},
        description = "Generated proto import root path",
        required = true)
    private String fhirProtoRoot = null;

    @Parameter(
        names = {"--for_codes_in"},
        description = "Generate codes for types used in these bundles.")
    private Set<String> codeUsers = new HashSet<>();

    @Parameter(
        names = {"--exclude_codes_in"},
        description = "Exclude codes for types used in these bundles")
    private Set<String> excludeCodeUsers = new HashSet<>();
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

    PackageInfo packageInfo =
        FileUtils.mergeText(new File(args.packageInfo), PackageInfo.newBuilder()).build();
    FhirVersion fhirVersion = packageInfo.getFhirVersion();

    Set<Bundle> valueSetBundles = loadBundles(args.valueSetBundleFiles, fhirVersion);
    valueSetBundles.add(makeBundle(args.codeSystemFiles, args.valueSetFiles, fhirVersion));

    ValueSetGenerator generator =
        new ValueSetGenerator(packageInfo, args.fhirProtoRoot, valueSetBundles);

    ProtoFilePrinter printer = new ProtoFilePrinter();

    System.out.println("Writing " + args.outputName + "...");
    File outputFile = new File(args.outputName);
    FileDescriptorProto fileDescriptor =
        args.codeUsers.isEmpty()
            ? generator.generateValueSetFile()
            : generator.forCodesUsedIn(
                loadBundles(args.codeUsers, fhirVersion),
                loadBundles(args.excludeCodeUsers, fhirVersion));
    Files.asCharSink(outputFile, UTF_8).write(printer.print(fileDescriptor));
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

  private static Bundle makeBundle(
      Set<String> codeSystemFiles, Set<String> valueSetFiles, FhirVersion fhirVersion)
      throws IOException {
    JsonFormat.Parser parser = getParser(fhirVersion);
    Bundle.Builder bundle = Bundle.newBuilder();
    for (String filename : codeSystemFiles) {
      CodeSystem.Builder codeSystem = CodeSystem.newBuilder();
      String json = Files.asCharSource(new File(filename), StandardCharsets.UTF_8).read();
      parser.merge(json, codeSystem);
      bundle.addEntry(
          Bundle.Entry.newBuilder()
              .setResource(ContainedResource.newBuilder().setCodeSystem(codeSystem)));
    }
    for (String filename : valueSetFiles) {
      ValueSet.Builder valueSet = ValueSet.newBuilder();
      String json = Files.asCharSource(new File(filename), StandardCharsets.UTF_8).read();
      parser.merge(json, valueSet);
      bundle.addEntry(
          Bundle.Entry.newBuilder()
              .setResource(ContainedResource.newBuilder().setValueSet(valueSet)));
    }
    return bundle.build();
  }

  private static Set<Bundle> loadBundles(Set<String> filenames, FhirVersion fhirVersion)
      throws IOException {
    JsonFormat.Parser parser = getParser(fhirVersion);
    Set<Bundle> bundles = new HashSet<>();
    for (String filename : filenames) {
      Bundle.Builder builder = Bundle.newBuilder();
      String json = Files.asCharSource(new File(filename), StandardCharsets.UTF_8).read();
      parser.merge(json, builder);
      bundles.add(builder.build());
    }
    return bundles;
  }
}
