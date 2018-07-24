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

package com.google.fhir.examples;

import static com.google.common.base.Preconditions.checkNotNull;
import static java.nio.charset.StandardCharsets.UTF_8;

import com.beust.jcommander.JCommander;
import com.beust.jcommander.Parameter;
import com.beust.jcommander.ParameterException;
import com.google.common.io.Files;
import com.google.fhir.stu3.JsonFormat;
import com.google.fhir.stu3.ProtoFilePrinter;
import com.google.fhir.stu3.ProtoGenerator;
import com.google.fhir.stu3.proto.StructureDefinition;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.TextFormat;
import java.io.BufferedWriter;
import java.io.File;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

/**
 * A class that runs ProtoGenerator on the specified inputs, turning FHIR StructureDefinition files
 * into proto descriptors. Depending on settings, either the descriptors, the .proto file, or both
 * will be emitted.
 */
class ProtoGeneratorMain {

  private final ProtoGenerator generator;
  private final ProtoFilePrinter printer;
  private final PrintWriter writer;

  private static class Args {
    @Parameter(
      names = {"--emit_descriptors"},
      description = "Emit individual descriptor files"
    )
    private Boolean emitDescriptors = false;

    @Parameter(
      names = {"--output_directory"},
      description = "Directory where generated output will be saved"
    )
    private String outputDirectory = ".";

    @Parameter(
      names = {"--emit_proto"},
      description = "Emit a .proto file generated from the input"
    )
    private Boolean emitProto = false;

    @Parameter(
      names = {"--include_contained_resource"},
      description =
          "Include a ContainedResource message, containing an entry for each message type present "
              + " in the proto"
    )
    private Boolean includeContainedResource = false;

    @Parameter(
      names = {"--output_filename"},
      description = "File name of the output .proto file, relative to --output_directory"
    )
    private String outputFilename = "output.proto";

    @Parameter(
      names = {"--proto_package"},
      description = "Generated proto package name"
    )
    private String protoPackage = "google.fhir.stu3.proto";

    @Parameter(
      names = {"--proto_root"},
      description = "Generated proto import root path"
    )
    private String protoRoot = "proto/stu3";

    @Parameter(description = "List of input files")
    private List<String> inputFiles = new ArrayList<>();
  }

  ProtoGeneratorMain(ProtoGenerator generator, PrintWriter writer, ProtoFilePrinter protoPrinter) {
    this.generator = checkNotNull(generator);
    this.writer = checkNotNull(writer);
    this.printer = checkNotNull(protoPrinter);
  }

  void run(Args args) throws IOException {
    JsonFormat.Parser jsonParser = JsonFormat.getParser();

    // Read the inputs in sequence.
    ArrayList<StructureDefinition> definitions = new ArrayList<>();
    for (String filename : args.inputFiles) {
      writer.println("Reading " + filename + "...");

      File file = new File(filename);
      String json = Files.asCharSource(file, UTF_8).read();
      StructureDefinition.Builder builder = StructureDefinition.newBuilder();
      jsonParser.merge(json, builder);
      definitions.add(builder.build());
    }

    // Generate the proto file.
    writer.println("Generating proto descriptors...");
    writer.flush();
    FileDescriptorProto proto;
    proto = generator.generateFileDescriptor(definitions);
    if (args.includeContainedResource) {
      proto = generator.addContainedResource(proto);
    }
    String protoFileContents = printer.print(proto);

    if (args.emitProto) {
      // Save the result as a .proto file
      writer.println("Writing " + args.outputFilename + "...");
      writer.flush();
      File outputFile = new File(args.outputDirectory, args.outputFilename);
      Files.asCharSink(outputFile, UTF_8).write(protoFileContents);
    }

    if (args.emitDescriptors) {
      // Save the result as individual .descriptor.prototxt files
      writer.println("Writing individual descriptors to " + args.outputDirectory + "...");
      writer.flush();
      for (DescriptorProto descriptor : proto.getMessageTypeList()) {
        File outputFile =
            new File(
                args.outputDirectory, descriptor.getName().toLowerCase() + ".descriptor.prototxt");
        Files.asCharSink(outputFile, UTF_8).write(TextFormat.printToString(descriptor));
      }
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

    new ProtoGeneratorMain(
            new ProtoGenerator(args.protoPackage, args.protoRoot),
            new PrintWriter(new BufferedWriter(new OutputStreamWriter(System.out, UTF_8))),
            new ProtoFilePrinter().withApacheLicense())
        .run(args);
  }
}
