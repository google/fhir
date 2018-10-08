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
import com.google.common.base.Splitter;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Multimap;
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
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;

/**
 * A class that runs ProtoGenerator on the specified inputs, turning FHIR StructureDefinition files
 * into proto descriptors. Depending on settings, either the descriptors, the .proto file, or both
 * will be emitted.
 */
class ProtoGeneratorMain {

  private final ProtoFilePrinter printer;
  private final PrintWriter writer;

  // The convention is to name profiles as the lowercased version of the element they define,
  // but this is not guaranteed by the spec, so we don't rely on it.
  // This mapping lets us keep track of source filenames for generated types.
  private final Map<String, String> typeToSourceFileBaseName = new HashMap<>();

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
        description = "Generated proto package name",
        required = true)
    private String protoPackage = null;

    @Parameter(
        names = {"--java_proto_package"},
        description = "Generated java proto package name")
    private String javaProtoPackage = null;

    @Parameter(
        names = {"--go_proto_package"},
        description = "Generated go proto package name")
    private String goProtoPackage = null;

    @Parameter(
        names = {"--fhir_proto_root"},
        description = "Generated proto import root path",
        required = true)
    private String fhirProtoRoot = null;

    @Parameter(
        names = {"--known_types"},
        description = "List of known StructureDefinitions, for inlining types.")
    private List<String> knownTypes = new ArrayList<>();

    // TODO: figure out a smarter way to handle dependencies
    @Parameter(
        names = {"--include_resources"},
        description = "Includes a dependency on resources.proto")
    private boolean includeResources = false;

    @Parameter(
        names = {"--include_extensions"},
        description = "Includes a dependency on extensions.proto")
    private boolean includeExtensions = false;

    @Parameter(
        names = {"--include_metadatatypes"},
        description = "Includes a dependency on metadatatypes.proto")
    private boolean includeMetadatatypes = false;

    @Parameter(
        names = {"--additional_import"},
        description = "Non-core fhir dependencies to add.")
    private List<String> additionalImports = new ArrayList<>();

    @Parameter(
        names = {"--add_apache_license"},
        description = "Adds Apache License to proto files.")
    private Boolean addApacheLicense = false;

    @Parameter(description = "List of input files")
    private List<String> inputFiles = new ArrayList<>();

    /** Returns a map from known StructureDefinition type to the package it is located in. */
    private Map<String, String> getKnownTypesMap() {
      Splitter keyValueSplitter = Splitter.on(":");
      Splitter fileSplitter = Splitter.on(";");
      // Convert to a multimap from package -> list of files in that package.
      Multimap<String, String> packages = HashMultimap.create();
      for (String knownTypesEntry : knownTypes) {
        if (knownTypesEntry.endsWith(";")) {
          knownTypesEntry = knownTypesEntry.substring(0, knownTypesEntry.length() - 1);
        }
        List<String> keyValuePair = keyValueSplitter.splitToList(knownTypesEntry);
        if (keyValuePair.size() != 2) {
          throw new IllegalArgumentException(
              "Invalid knownTypes Entry ["
                  + knownTypesEntry
                  + "].  Should be of the form: [your.package:file_1;file_2;file_3...].");
        }
        packages.putAll(keyValuePair.get(0), fileSplitter.split(keyValuePair.get(1)));
      }
      // Invert multimap into a map from file -> package it is located in.
      return packages.entries().stream()
          .collect(Collectors.toMap(Map.Entry::getValue, Map.Entry::getKey));
    }
  }

  ProtoGeneratorMain(PrintWriter writer, ProtoFilePrinter protoPrinter) {
    this.writer = checkNotNull(writer);
    this.printer = checkNotNull(protoPrinter);
  }

  void run(Args args) throws IOException {
    JsonFormat.Parser jsonParser = JsonFormat.getParser();

    // Read all structure definitions that should be inlined into the
    // output protos.  This will not generate proto definitions for these extensions -
    // that must be done separately.
    // Generate a Pair of {StructureDefinition, proto_package} for each.
    Map<StructureDefinition, String> knownTypes = new HashMap<>();
    for (Map.Entry<String, String> typePackagePair : args.getKnownTypesMap().entrySet()) {
      knownTypes.put(
          readStructureDefinition(typePackagePair.getKey(), jsonParser),
          typePackagePair.getValue());
    }

    // Read the inputs in sequence.
    ArrayList<StructureDefinition> definitions = new ArrayList<>();
    for (String filename : args.inputFiles) {
      StructureDefinition definition = readStructureDefinition(filename, jsonParser);
      // TODO: We could skip over simple extensions here (since they'll get inlined as
      // primitives, but that would break usages of things like ExtensionWrapper.fromExtensionsIn.
      // Think about this a bit more.
      definitions.add(definition);

      // Keep a mapping from Message name that will be generated to file name that it came from.
      // This allows us to generate parallel file names between input and output files.

      // File base name is the last token, stripped of any extension
      // e.g., my-oddly_namedFile from foo/bar/my-oddly_namedFile.profile.json
      String fileBaseName = Splitter.on('.').splitToList(new File(filename).getName()).get(0);
      typeToSourceFileBaseName.put(ProtoGenerator.getTypeName(definition), fileBaseName);
    }

    // Generate the proto file.
    writer.println("Generating proto descriptors...");
    writer.flush();
    FileDescriptorProto proto;
    ProtoGenerator generator =
        new ProtoGenerator(
            args.protoPackage,
            Optional.ofNullable(args.javaProtoPackage),
            Optional.ofNullable(args.goProtoPackage),
            args.fhirProtoRoot,
            knownTypes);
    proto = generator.generateFileDescriptor(definitions);
    if (args.includeContainedResource) {
      proto = generator.addContainedResource(proto);
    }
    // TODO: deduce these automatically
    if (args.includeResources) {
      proto =
          proto
              .toBuilder()
              .addDependency(new File(args.fhirProtoRoot, "resources.proto").toString())
              .build();
    }
    if (args.includeExtensions) {
      proto =
          proto
              .toBuilder()
              .addDependency(new File(args.fhirProtoRoot, "extensions.proto").toString())
              .build();
    }
    if (args.includeMetadatatypes) {
      proto =
          proto
              .toBuilder()
              .addDependency(new File(args.fhirProtoRoot, "metadatatypes.proto").toString())
              .build();
    }
    for (String additionalImport : args.additionalImports) {
      proto = proto.toBuilder().addDependency(new File(additionalImport).toString()).build();
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
        String fileBaseName = typeToSourceFileBaseName.get(descriptor.getName());
        if (fileBaseName == null) {
          throw new IllegalArgumentException(
              "No file basename associated with type: "
                  + descriptor.getName()
                  + "\n"
                  + typeToSourceFileBaseName);
        }
        File outputFile = new File(args.outputDirectory, fileBaseName + ".descriptor.prototxt");
        Files.asCharSink(outputFile, UTF_8).write(TextFormat.printToString(descriptor));
      }
    }
  }

  private StructureDefinition readStructureDefinition(String filename, JsonFormat.Parser jsonParser)
      throws IOException {
    File file = new File(filename);
    String json = Files.asCharSource(file, UTF_8).read();
    StructureDefinition.Builder builder = StructureDefinition.newBuilder();
    jsonParser.merge(json, builder);
    return builder.build();
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
    ProtoFilePrinter printer =
        args.addApacheLicense ? new ProtoFilePrinter().withApacheLicense() : new ProtoFilePrinter();
    new ProtoGeneratorMain(
            new PrintWriter(new BufferedWriter(new OutputStreamWriter(System.out, UTF_8))), printer)
        .run(args);
  }
}
