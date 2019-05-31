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
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.r4.proto.Bundle;
import com.google.fhir.stu3.FileUtils;
import com.google.fhir.stu3.ProtoFilePrinter;
import com.google.fhir.stu3.ValueSetGenerator;
import java.io.File;
import java.io.IOException;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/** Main class for using the ValueSetGenerator to generate valueset protos. */
final class ValueSetGeneratorMain {

  private static class Args {

    @Parameter(
        names = {"--valueset_bundle"},
        description = "Bundle containing CodeSystem and ValueSet definitions.",
        required = true)
    private List<String> valueSetBundleFiles = null;

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
        names = {"--include_codes_in_datatypes"},
        description = "Whether or not to include types already found in datatypes")
    private boolean includeCodesInDatatypes = false;
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

    Set<Bundle> valuesetBundles = new HashSet<>();
    for (String filename : args.valueSetBundleFiles) {
      valuesetBundles.add((Bundle) FileUtils.loadFhir(filename, Bundle.newBuilder()));
    }

    ValueSetGenerator generator =
        new ValueSetGenerator(
            packageInfo, args.fhirProtoRoot, valuesetBundles, args.includeCodesInDatatypes);

    System.out.println(packageInfo);
    ProtoFilePrinter printer = new ProtoFilePrinter();

    System.out.println("Writing " + args.outputName + "...");
    File outputFile = new File(args.outputName);
    Files.asCharSink(outputFile, UTF_8).write(printer.print(generator.generateValueSetFile()));
  }
}
