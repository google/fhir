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
// bazel build //java:StructureDefinitionTransformer
// bazel-bin/java/StructureDefinitionTransformer \
//     --input_directory spec/hl7.fhir.core/1.0.2/package/ \
//     --output_directory /tmp/ \
//     StructureDefinition-base64Binary.json

package com.google.fhir.examples;

import com.beust.jcommander.JCommander;
import com.beust.jcommander.Parameter;
import com.beust.jcommander.ParameterException;
import com.google.common.io.Files;
import com.google.fhir.dstu2.StructureDefinitionTransformer;
import java.io.BufferedWriter;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/** A class that transforms DSTU2 structure definitions to STU3 to prepare for proto generation. */
class StructureDefinitionTransformerMain {

  private static class Args {

    @Parameter(
        names = {"--input_directory"},
        description = "Directory where input files will be read")
    private String inputDirectory = "";

    @Parameter(
        names = {"--output_directory"},
        description = "Directory where transformed output will be saved")
    private String outputDirectory = "";

    @Parameter(description = "List of input StructureDefinitions")
    private List<String> inputFiles = new ArrayList<>();
  }

  StructureDefinitionTransformerMain() {}

  private static void transform(String inputFilename, String outputFilename) throws IOException {
    String input = Files.asCharSource(new File(inputFilename), StandardCharsets.UTF_8).read();
    String output = StructureDefinitionTransformer.transform(input);
    try (BufferedWriter writer =
        Files.newWriter(new File(outputFilename), StandardCharsets.UTF_8)) {
      writer.write(output);
      writer.close();
    }
  }

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

    if (args.inputDirectory.equals(args.outputDirectory)) {
      System.err.println("Input directory and output directory cannot be the same");
      System.exit(1);
    }

    for (String filename : args.inputFiles) {
      transform(args.inputDirectory + filename, args.outputDirectory + filename);
    }
  }
}
