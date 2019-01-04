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

import com.beust.jcommander.IStringConverter;
import com.beust.jcommander.JCommander;
import com.beust.jcommander.Parameter;
import com.beust.jcommander.ParameterException;
import com.google.common.base.Splitter;
import com.google.common.io.Files;
import com.google.fhir.common.FhirVersion;
import com.google.fhir.dstu2.StructureDefinitionTransformer;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.stu3.AnnotationUtils;
import com.google.fhir.stu3.FileUtils;
import com.google.fhir.stu3.JsonFormat;
import com.google.fhir.stu3.ProtoFilePrinter;
import com.google.fhir.stu3.ProtoGenerator;
import com.google.fhir.stu3.proto.Bundle;
import com.google.fhir.stu3.proto.ContainedResource;
import com.google.fhir.stu3.proto.Extension;
import com.google.fhir.stu3.proto.StructureDefinition;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.TextFormat;
import java.io.BufferedWriter;
import java.io.File;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

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
  private static final Map<String, String> typeToSourceFileBaseName = new HashMap<>();

  private static final String EXTENSION_STRUCTURE_DEFINITION_URL =
      AnnotationUtils.getStructureDefinitionUrl(Extension.getDescriptor());

  /** Class that implements string flag to FHIR version enum. */
  public static class FhirVersionConverter implements IStringConverter<FhirVersion> {

    @Override
    public FhirVersion convert(String value) {
      FhirVersion convertedValue = FhirVersion.fromString(value);
      if (convertedValue == null) {
        throw new ParameterException(String.format("failed to convert %s to FHIR version", value));
      }
      return convertedValue;
    }
  }

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
        names = {"--descriptor_output_directory"},
        description = "Directory where generated descriptor output will be saved")
    private String descriptorOutputDirectory = ".";

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
        names = {"--struct_def_dep_pkg"},
        description =
            "For StructureDefinitions that are dependencies of the types being "
                + "generated, this is a pipe-delimited tuple of target|PackageInfo. "
                + "The target can be either a directory or a zip archive, and the PackageInfo "
                + "should be a path to a prototxt file containing a google.fhir.proto.PackageInfo "
                + "proto.")
    private List<String> structDefDepPkgList = new ArrayList<>();

    @Parameter(
        names = {"--additional_import"},
        description = "Non-core fhir dependencies to add.")
    private List<String> additionalImports = new ArrayList<>();

    @Parameter(
        names = {"--add_apache_license"},
        description = "Adds Apache License to proto files.")
    private Boolean addApacheLicense = false;

    @Parameter(
        names = {"--separate_extensions"},
        description =
            "If true, will separate all extensions into a ${output_name}_extensions.proto file.")
    private boolean separateExtensions = false;

    @Parameter(
        names = {"--output_name"},
        description =
            "Name for output proto files.  If separate_extensions is true, will "
                + "output ${output_name}.proto and ${output_name}_extensions.proto.  "
                + "Otherwise, just outputs ${output_name}.proto.")
    private String outputName = "output";

    @Parameter(
        names = {"--input_bundle"},
        description = "Input Bundle of StructureDefinitions")
    private String inputBundleFile = null;

    @Parameter(
        names = {"--input_zip"},
        description = "Input Zip of StructureDefinitions")
    private String inputZipFile = null;

    @Parameter(
        names = {"--fhir_version"},
        description = "FHIR version of the StructureDefinitions, DSTU2 or STU3",
        converter = FhirVersionConverter.class)
    private FhirVersion fhirVersion = FhirVersion.STU3;

    @Parameter(description = "List of input StructureDefinitions")
    private List<String> inputFiles = new ArrayList<>();

    /**
     * For StructureDefinitions that are dependencies of the input StructureDefinitions, returns a
     * map from directory of StructureDefinitions to the proto package they were (previously)
     * generated in.
     */
    private Map<String, String> getDependencyPackagesMap() {
      Splitter keyValueSplitter = Splitter.on("|");
      Map<String, String> packages = new HashMap<>();
      for (String structDefDepPkg : structDefDepPkgList) {
        List<String> keyValuePair = keyValueSplitter.splitToList(structDefDepPkg);
        if (keyValuePair.size() != 2) {
          throw new IllegalArgumentException(
              "Invalid struct_def_dep_pkg entry ["
                  + structDefDepPkg
                  + "].  Should be of the form: your/directory|proto.package.name");
        }
        packages.put(keyValuePair.get(0), keyValuePair.get(1));
      }
      return packages;
    }
  }

  private static StructureDefinition loadStructureDefinition(
      String fullFilename, FhirVersion fhirVersion) throws IOException {
    if (FhirVersion.STU3.equals(fhirVersion)) {
      return FileUtils.loadStructureDefinition(new File(fullFilename));
    }
    String json = Files.asCharSource(new File(fullFilename), StandardCharsets.UTF_8).read();
    String transformed = StructureDefinitionTransformer.transform(json);
    StructureDefinition.Builder structDefBuilder = StructureDefinition.newBuilder();
    JsonFormat.Parser.newBuilder().build().merge(transformed, structDefBuilder);
    return structDefBuilder.build();
  }

  ProtoGeneratorMain(PrintWriter writer, ProtoFilePrinter protoPrinter) {
    this.writer = checkNotNull(writer);
    this.printer = checkNotNull(protoPrinter);
  }

  void run(Args args) throws IOException {
    // Read all structure definitions that should be inlined into the
    // output protos.  This will not generate proto definitions for these extensions -
    // that must be done separately.
    // Generate a Pair of {StructureDefinition, proto_package} for each.
    Map<StructureDefinition, String> knownTypes = new HashMap<>();
    for (Map.Entry<String, String> dirPackagePair : args.getDependencyPackagesMap().entrySet()) {
      List<StructureDefinition> structDefs =
          new File(dirPackagePair.getKey()).isDirectory()
              ? FileUtils.loadStructureDefinitionsInDir(dirPackagePair.getKey())
              : FileUtils.loadStructureDefinitionsInZip(dirPackagePair.getKey());
      PackageInfo depPackageInfo =
          FileUtils.mergeText(new File(dirPackagePair.getValue()), PackageInfo.newBuilder())
              .build();
      if (depPackageInfo.getProtoPackage().isEmpty()) {
        throw new IllegalArgumentException(
            "PackageInfo has no proto_package: " + dirPackagePair.getValue());
      }
      for (StructureDefinition structDef : structDefs) {
        knownTypes.put(structDef, depPackageInfo.getProtoPackage());
      }
    }

    PackageInfo packageInfo =
        FileUtils.mergeText(new File(args.packageInfo), PackageInfo.newBuilder()).build();
    if (packageInfo.getProtoPackage().isEmpty()) {
      throw new IllegalArgumentException("package_info must contain at least a proto_package.");
    }

    // Read the inputs in sequence.
    ArrayList<StructureDefinition> definitions = new ArrayList<>();
    for (String filename : args.inputFiles) {
      StructureDefinition definition = loadStructureDefinition(filename, args.fhirVersion);
      definitions.add(definition);

      // Keep a mapping from Message name that will be generated to file name that it came from.
      // This allows us to generate parallel file names between input and output files.

      // File base name is the last token, stripped of any extension
      // e.g., my-oddly_namedFile from foo/bar/my-oddly_namedFile.profile.json
      String fileBaseName = Splitter.on('.').splitToList(new File(filename).getName()).get(0);
      typeToSourceFileBaseName.put(ProtoGenerator.getTypeName(definition), fileBaseName);

      // Add in anything currently being generated as a known type.
      knownTypes.put(definition, packageInfo.getProtoPackage());
    }

    if (args.inputBundleFile != null) {
      Bundle bundle = (Bundle) FileUtils.loadFhir(args.inputBundleFile, Bundle.newBuilder());
      for (Bundle.Entry entry : bundle.getEntryList()) {
        StructureDefinition structDef = entry.getResource().getStructureDefinition();
        definitions.add(structDef);
        typeToSourceFileBaseName.put(
            ProtoGenerator.getTypeName(structDef), structDef.getId().getValue());
        knownTypes.put(structDef, packageInfo.getProtoPackage());
      }
    }

    if (args.inputZipFile != null) {
      for (StructureDefinition structDef :
          FileUtils.loadStructureDefinitionsInZip(args.inputZipFile)) {
        definitions.add(structDef);
        typeToSourceFileBaseName.put(
            ProtoGenerator.getTypeName(structDef), structDef.getId().getValue());
        knownTypes.put(structDef, packageInfo.getProtoPackage());
      }
    }

    // Generate the proto file.
    writer.println("Generating proto descriptors...");
    writer.flush();

    ProtoGenerator generator =
        new ProtoGenerator(packageInfo, args.fhirProtoRoot, args.fhirVersion, knownTypes);

    if (args.separateExtensions) {
      List<StructureDefinition> extensions = new ArrayList<>();
      List<StructureDefinition> profiles = new ArrayList<>();
      for (StructureDefinition structDef : definitions) {
        if (structDef.getBaseDefinition().getValue().equals(EXTENSION_STRUCTURE_DEFINITION_URL)) {
          extensions.add(structDef);
        } else {
          profiles.add(structDef);
        }
      }
      writeProto(
          generator.generateFileDescriptor(extensions),
          args.outputName + "_extensions.proto",
          args,
          false);
      writeProto(
          generator.generateFileDescriptor(profiles), args.outputName + ".proto", args, true);
    } else {
      FileDescriptorProto proto = generator.generateFileDescriptor(definitions);
      if (args.includeContainedResource) {
        proto = generator.addContainedResource(proto);
      }
      writeProto(proto, args.outputName + ".proto", args, true);
    }
  }

  private void writeProto(
      FileDescriptorProto proto, String protoFileName, Args args, boolean includeAdditionalImports)
      throws IOException {
    if (includeAdditionalImports) {
      for (String additionalImport : args.additionalImports) {
        proto = proto.toBuilder().addDependency(new File(additionalImport).toString()).build();
      }
    }
    String protoFileContents = printer.print(proto);

    if (args.emitProto) {
      // Save the result as a .proto file
      writer.println("Writing " + protoFileName + "...");
      writer.flush();
      File outputFile = new File(args.outputDirectory, protoFileName);
      Files.asCharSink(outputFile, UTF_8).write(protoFileContents);
    }

    if (args.emitDescriptors) {
      // Save the result as individual .descriptor.prototxt files
      writer.println("Writing individual descriptors to " + args.descriptorOutputDirectory + "...");
      writer.flush();
      for (DescriptorProto descriptor : proto.getMessageTypeList()) {
        if (descriptor.getName().equals(ContainedResource.getDescriptor().getName())) {
          continue;
        }
        String fileBaseName = typeToSourceFileBaseName.get(descriptor.getName());
        if (fileBaseName == null) {
          throw new IllegalArgumentException(
              "No file basename associated with type: "
                  + descriptor.getName()
                  + "\n"
                  + typeToSourceFileBaseName);
        }
        File outputFile =
            new File(args.descriptorOutputDirectory, fileBaseName + ".descriptor.prototxt");
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
    ProtoFilePrinter printer =
        args.addApacheLicense ? new ProtoFilePrinter().withApacheLicense() : new ProtoFilePrinter();
    new ProtoGeneratorMain(
            new PrintWriter(new BufferedWriter(new OutputStreamWriter(System.out, UTF_8))), printer)
        .run(args);
  }
}
