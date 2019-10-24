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
import com.google.common.base.CaseFormat;
import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableMap;
import com.google.common.io.Files;
import com.google.fhir.dstu2.StructureDefinitionTransformer;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.Annotations.FhirVersion;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.r4.core.Bundle;
import com.google.fhir.r4.core.CodeSystem;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.StructureDefinitionKindCode;
import com.google.fhir.r4.core.ValueSet;
import com.google.fhir.stu3.FileUtils;
import com.google.fhir.stu3.GeneratorUtils;
import com.google.fhir.stu3.JsonFormat;
import com.google.fhir.stu3.ProtoFilePrinter;
import com.google.fhir.stu3.ProtoGenerator;
import com.google.fhir.stu3.ValueSetGenerator;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import java.io.BufferedWriter;
import java.io.File;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * A class that runs ProtoGenerator on the specified inputs, turning FHIR StructureDefinition files
 * into proto descriptors. Depending on settings, either the descriptors, the .proto file, or both
 * will be emitted.
 */
class ProtoGeneratorMain {

  private final PrintWriter writer;

  // The convention is to name profiles as the lowercased version of the element they define,
  // but this is not guaranteed by the spec, so we don't rely on it.
  // This mapping lets us keep track of source filenames for generated types.
  private static final Map<String, String> typeToSourceFileBaseName = new HashMap<>();

  private static final String EXTENSION_STRUCTURE_DEFINITION_URL =
      "http://hl7.org/fhir/StructureDefinition/Extension";

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
        names = {"--package_info"},
        description = "Prototxt containing google.fhir.proto.PackageInfo",
        required = true)
    private String packageInfo = null;

    @Parameter(
        names = {"--dstu2_struct_def_zip"},
        description = "Zip file containing core DSTU2 Structure Definitions.")
    private String dstu2StructDefZip = null;

    @Parameter(
        names = {"--stu3_definitions_zip"},
        description = "Zip file containing core STU3 Definitions.")
    private String stu3DefinitionsZip = null;

    @Parameter(
        names = {"--r4_definitions_zip"},
        description = "Zip file containing core R4 Definitions.")
    private String r4DefinitionsZip = null;

    @Parameter(
        names = {"--fhir_definition_dep"},
        description =
            "For StructureDefinitions that are non-core dependencies of the types being "
                + "generated, this is a pipe-delimited tuple of zip|PackageInfo. "
                + "The PackageInfo "
                + "should be a path to a prototxt file containing a google.fhir.proto.PackageInfo "
                + "proto.")
    private List<String> fhirDefinitionDepList = new ArrayList<>();

    @Parameter(
        names = {"--additional_import"},
        description = "Non-core fhir dependencies to add.")
    private List<String> additionalImports = new ArrayList<>();

    @Parameter(
        names = {"--output_name"},
        description =
            "Name for output proto files.  If separating extensions, will "
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
        names = {"--exclude"},
        description = "Ids of input StructureDefinitions to ignore.")
    private List<String> excludeIds = new ArrayList<>();

    @Parameter(description = "List of input StructureDefinitions")
    private List<String> inputFiles = new ArrayList<>();

    /**
     * For StructureDefinitions that are dependencies of the input StructureDefinitions, returns a
     * map from zip archive of StructureDefinitions to proto package those types were generated in.
     * E.g., { "path/to/stu3/uscore_structuredefinitions.zip" -> "google.fhir.stu3.uscore" } This
     * allows inlining an externally-defined proto based on its structure definition url.
     */
    private Map<String, String> getDependencyPackagesMap() throws IOException {
      Splitter keyValueSplitter = Splitter.on("|");
      Map<String, String> packages = new HashMap<>();
      for (String fhirDefinitionDep : fhirDefinitionDepList) {
        List<String> keyValuePair = keyValueSplitter.splitToList(fhirDefinitionDep);
        if (keyValuePair.size() != 2) {
          throw new IllegalArgumentException(
              "Invalid fhir_definition_dep entry ["
                  + fhirDefinitionDep
                  + "].  Should be of the form: your/archive.zip|your/package_info.prototxt");
        }
        PackageInfo depPackageInfo =
            FileUtils.mergeText(new File(keyValuePair.get(1)), PackageInfo.newBuilder()).build();
        if (depPackageInfo.getProtoPackage().isEmpty()) {
          throw new IllegalArgumentException(
              "Missing proto_package from PackageInfo: " + keyValuePair.get(1));
        }
        packages.put(keyValuePair.get(0), depPackageInfo.getProtoPackage());
      }
      return packages;
    }
  }

  private static StructureDefinition loadStructureDefinition(
      String fullFilename, FhirVersion fhirVersion) throws IOException {
    String json = Files.asCharSource(new File(fullFilename), StandardCharsets.UTF_8).read();
    StructureDefinition.Builder structDefBuilder = StructureDefinition.newBuilder();
    switch (fhirVersion) {
      case STU3:
        return JsonFormat.getEarlyVersionGeneratorParser().merge(json, structDefBuilder).build();
      case R4:
        return JsonFormat.getParser().merge(json, structDefBuilder).build();
      default:
        throw new IllegalArgumentException("Unrecognized FHIR version: " + fhirVersion);
    }
  }

  ProtoGeneratorMain(PrintWriter writer) {
    this.writer = checkNotNull(writer);
  }

  void run(Args args) throws IOException {
    PackageInfo packageInfo =
        FileUtils.mergeText(new File(args.packageInfo), PackageInfo.newBuilder()).build();

    if (packageInfo.getProtoPackage().isEmpty()
        || packageInfo.getFhirVersion() == FhirVersion.FHIR_VERSION_UNKNOWN) {
      throw new IllegalArgumentException(
          "package_info must contain at least a proto_package and fhir_version.");
    }

    // Map representing all packages that are dependencies of this package.
    // This map is of the form
    // {location of zip archive of definitions -> proto package to inlined those types as}
    Map<String, String> dependencyPackagesMap = args.getDependencyPackagesMap();

    // Add in core FHIR types (e.g., datatypes and unprofiled resources)
    switch (packageInfo.getFhirVersion()) {
      case DSTU2:
        if (args.dstu2StructDefZip == null) {
          throw new IllegalArgumentException(
              "Package is for DSTU2, but --dstu2_struct_def_zip is not specified.");
        }
        dependencyPackagesMap.put(
            args.dstu2StructDefZip, com.google.fhir.common.FhirVersion.R4.coreProtoPackage);
        break;
      case STU3:
        if (args.stu3DefinitionsZip == null) {
          throw new IllegalArgumentException(
              "Package is for STU3, but --stu3_definitions_zip is not specified.");
        }
        dependencyPackagesMap.put(
            args.stu3DefinitionsZip, com.google.fhir.common.FhirVersion.STU3.coreProtoPackage);
        break;
      case R4:
        if (args.r4DefinitionsZip == null) {
          throw new IllegalArgumentException(
              "Package is for R4, but --r4_definitions_zip is not specified.");
        }
        dependencyPackagesMap.put(
            args.r4DefinitionsZip, com.google.fhir.common.FhirVersion.R4.coreProtoPackage);
        break;
      default:
        throw new IllegalArgumentException(
            "FHIR version not supported by ProfileGenerator: " + packageInfo.getFhirVersion());
    }

    // Read all structure definitions that should be inlined into the
    // output protos.  This will not generate proto definitions for these extensions -
    // that must be done separately.
    // Generate a Pair of {StructureDefinition, proto package string} for each.
    Map<StructureDefinition, String> knownTypes = new HashMap<>();
    for (Map.Entry<String, String> zipPackagePair : dependencyPackagesMap.entrySet()) {
      List<StructureDefinition> structDefs =
          FileUtils.loadTypesInZip(
              StructureDefinition.getDefaultInstance(),
              zipPackagePair.getKey(),
              packageInfo.getFhirVersion());
      for (StructureDefinition structDef : structDefs) {
        knownTypes.put(structDef, zipPackagePair.getValue());
      }
    }

    // Read the inputs in sequence.
    List<StructureDefinition> definitions = new ArrayList<>();
    for (String filename : args.inputFiles) {
      StructureDefinition definition =
          loadStructureDefinition(filename, packageInfo.getFhirVersion());
      definitions.add(definition);

      // Keep a mapping from Message name that will be generated to file name that it came from.
      // This allows us to generate parallel file names between input and output files.

      // File base name is the last token, stripped of any extension
      // e.g., my-oddly_namedFile from foo/bar/my-oddly_namedFile.profile.json
      String fileBaseName = Splitter.on('.').splitToList(new File(filename).getName()).get(0);
      typeToSourceFileBaseName.put(
          ProtoGenerator.getTypeName(definition, packageInfo.getFhirVersion()), fileBaseName);

      // Add in anything currently being generated as a known type.
      knownTypes.put(definition, packageInfo.getProtoPackage());
    }

    if (args.inputBundleFile != null) {
      Bundle bundle = (Bundle) FileUtils.loadFhir(args.inputBundleFile, Bundle.newBuilder());
      for (Bundle.Entry entry : bundle.getEntryList()) {
        if (entry.getResource().hasStructureDefinition()) {
          StructureDefinition structDef = entry.getResource().getStructureDefinition();
          definitions.add(structDef);
          typeToSourceFileBaseName.put(
              ProtoGenerator.getTypeName(structDef, packageInfo.getFhirVersion()),
              structDef.getId().getValue());
          knownTypes.put(structDef, packageInfo.getProtoPackage());
        }
      }
    }

    if (args.inputZipFile != null) {
      for (StructureDefinition structDef :
          FileUtils.loadStructureDefinitionsInZip(args.inputZipFile)) {
        definitions.add(structDef);
        typeToSourceFileBaseName.put(
            ProtoGenerator.getTypeName(structDef, packageInfo.getFhirVersion()),
            structDef.getId().getValue());
        knownTypes.put(structDef, packageInfo.getProtoPackage());
      }
    }

    definitions =
        definitions.stream()
            .filter(def -> !args.excludeIds.contains(def.getId().getValue()))
            .collect(Collectors.toList());

    // Generate the proto file.
    writer.println("Generating proto descriptors...");
    writer.flush();

    ProtoGenerator generator =
        packageInfo.getFhirVersion() != FhirVersion.R4
            ? new ProtoGenerator(packageInfo, ImmutableMap.copyOf(knownTypes))
            : new ProtoGenerator(
                packageInfo,
                ImmutableMap.copyOf(knownTypes),
                makeR4ValueSetGenerator(packageInfo, dependencyPackagesMap.keySet()));
    ProtoFilePrinter printer = new ProtoFilePrinter(packageInfo);

    switch (packageInfo.getFileSplittingBehavior()) {
      case DEFAULT_SPLITTING_BEHAVIOR:
      case NO_SPLITTING:
        {
          FileDescriptorProto proto = generator.generateFileDescriptor(definitions);
          if (packageInfo.getLocalContainedResource()) {
            proto = generator.addContainedResource(proto, proto.getMessageTypeList());
          }
          writeProto(proto, args.outputName + ".proto", args, true, printer);
        }
        break;
      case SEPARATE_EXTENSIONS:
        writeWithSeparateExtensionsFile(definitions, generator, printer, packageInfo, args);
        break;
      case SPLIT_RESOURCES:
        writeSplitResources(definitions, generator, printer, packageInfo, args);
        break;
      case UNRECOGNIZED:
        throw new IllegalArgumentException(
            "Unrecognized file splitting behavior: " + packageInfo.getFileSplittingBehavior());
    }
  }

  void writeWithSeparateExtensionsFile(
      List<StructureDefinition> definitions,
      ProtoGenerator generator,
      ProtoFilePrinter printer,
      PackageInfo packageInfo,
      Args args)
      throws IOException {
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
        false,
        printer);
    FileDescriptorProto mainFileProto = generator.generateFileDescriptor(profiles);
    if (packageInfo.getLocalContainedResource()) {
      mainFileProto =
          generator.addContainedResource(mainFileProto, mainFileProto.getMessageTypeList());
    }
    writeProto(mainFileProto, args.outputName + ".proto", args, true, printer);
  }

  void writeSplitResources(
      List<StructureDefinition> definitions,
      ProtoGenerator generator,
      ProtoFilePrinter printer,
      PackageInfo packageInfo,
      Args args)
      throws IOException {
    // Divide into three categories.
    // Extensions and datatypes will be printed into a single aggregate file each,
    // while resources will be printed into one file per resource.
    // Note primititives are include in datatypes here.
    List<StructureDefinition> extensions = new ArrayList<>();
    List<StructureDefinition> datatypes = new ArrayList<>();
    List<StructureDefinition> resources = new ArrayList<>();

    for (StructureDefinition structDef : definitions) {
      StructureDefinitionKindCode.Value kind = structDef.getKind().getValue();
      if (structDef.getBaseDefinition().getValue().equals(EXTENSION_STRUCTURE_DEFINITION_URL)) {
        extensions.add(structDef);
      } else if (kind == StructureDefinitionKindCode.Value.RESOURCE) {
        resources.add(structDef);
      } else if (kind == StructureDefinitionKindCode.Value.PRIMITIVE_TYPE
          || kind == StructureDefinitionKindCode.Value.COMPLEX_TYPE) {
        datatypes.add(structDef);
      }
    }

    if (!extensions.isEmpty()) {
      writeProto(
          generator.generateFileDescriptor(extensions), "extensions.proto", args, false, printer);
    }

    if (!datatypes.isEmpty()) {
      writeProto(
          generator.generateFileDescriptor(datatypes), "datatypes.proto", args, false, printer);
    }

    // TODO: Move Contained Resource logic into ProtoGenerator.java
    if (!resources.isEmpty()) {
      List<DescriptorProto> containedTypes = new ArrayList<>();
      // Note that in the case where there is a contained resource that is local to a proto set,
      // (the usual case), we need to define the ContainedResource proto in the same file as the
      // Bundle proto to avoid a circular dependency.  Since we need to define all other resources
      // before we can define ContainedResource, we defer printing the Bundle file until after
      // all other resources are generated, and after we've added in ContainedResource.
      FileDescriptorProto deferredBundleFile = null;
      for (StructureDefinition structDef : definitions) {
        List<StructureDefinition> oneResource = new ArrayList<>();
        oneResource.add(structDef);
        FileDescriptorProto fileProto = generator.generateFileDescriptor(oneResource);
        DescriptorProto type = fileProto.getMessageType(0);
        String filename = resourceNameToFileName(generator.getTypeName(structDef), generator);
        if (type.getName().equals("Bundle")) {
          deferredBundleFile = fileProto;
        } else {
          writeProto(fileProto, filename, args, true, printer);
        }
        if (!type.getOptions().getExtension(Annotations.isAbstractType)) {
          containedTypes.add(type);
        }
      }
      if (deferredBundleFile != null) {
        if (packageInfo.getLocalContainedResource()) {
          FileDescriptorProto.Builder fileBuilder =
              generator.addContainedResource(deferredBundleFile, containedTypes).toBuilder();
          String importRoot = args.outputDirectory;
          while (importRoot.contains("/../")) {
            // resolve foo/bar/baz/../../quux into foo/quux
            importRoot = importRoot.replaceAll("/[^/]*/\\.\\./", "/");
          }
          for (DescriptorProto type : containedTypes) {
            if (!type.getName().equals("Bundle")) {
              fileBuilder.addDependency(
                  new File(importRoot, resourceNameToFileName(type.getName(), generator))
                      .toString());
            }
          }
          writeProto(
              fileBuilder.build(), "bundle_and_contained_resource.proto", args, true, printer);
        } else {
          writeProto(deferredBundleFile, "bundle.proto", args, true, printer);
        }
      }
    }
  }

  String resourceNameToFileName(String resourceName, ProtoGenerator generator) {
    return CaseFormat.UPPER_CAMEL.to(
            CaseFormat.LOWER_UNDERSCORE,
            GeneratorUtils.resolveAcronyms(GeneratorUtils.toFieldTypeCase(resourceName)))
        + ".proto";
  }

  private void writeProto(
      FileDescriptorProto proto,
      String protoFileName,
      Args args,
      boolean includeAdditionalImports,
      ProtoFilePrinter printer)
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
        if (descriptor.getName().equals("ContainedResource")) {
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
        Files.asCharSink(outputFile, UTF_8).write(descriptor.toString());
      }
    }
  }

  private static ValueSetGenerator makeR4ValueSetGenerator(
      PackageInfo packageInfo, Set<String> definitionZips) throws IOException {
    Set<ValueSet> valueSets = new HashSet<>();
    Set<CodeSystem> codeSystems = new HashSet<>();
    for (String definitionZip : definitionZips) {
      valueSets.addAll(
          FileUtils.loadTypesInZip(ValueSet.getDefaultInstance(), definitionZip, FhirVersion.R4));
      codeSystems.addAll(
          FileUtils.loadTypesInZip(CodeSystem.getDefaultInstance(), definitionZip, FhirVersion.R4));
    }

    return new ValueSetGenerator(packageInfo, valueSets, codeSystems);
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
            new PrintWriter(new BufferedWriter(new OutputStreamWriter(System.out, UTF_8))))
        .run(args);
  }
}
