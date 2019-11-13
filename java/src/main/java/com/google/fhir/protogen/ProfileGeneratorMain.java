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

package com.google.fhir.protogen;

import com.beust.jcommander.JCommander;
import com.beust.jcommander.Parameter;
import com.beust.jcommander.ParameterException;
import com.google.common.io.Files;
import com.google.fhir.common.FileUtils;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.proto.Annotations.FhirVersion;
import com.google.fhir.proto.Extensions;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.proto.Profiles;
import com.google.fhir.r4.core.Bundle;
import com.google.fhir.r4.core.StructureDefinition;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.List;

/**
 * Entry point for ProfileGenerator that takes Extensions and Profiles protos and generates a proto
 * package.
 */
public class ProfileGeneratorMain {

  private static final JsonFormat.Printer jsonPrinter = JsonFormat.getPrinter();

  private static class Args {

    @Parameter(
        names = {"--name"},
        description = "Name prefix for the generated files.")
    private String name = null;

    @Parameter(
        names = {"--output_directory"},
        description = "Directory where generated output will be saved")
    private String outputDirectory = ".";

    @Parameter(
        names = {"--package_info"},
        description = "Prototxt file containing a PackageInfo proto.")
    private String packageInfo = null;

    @Parameter(
        names = {"--extensions"},
        description = "Prototxt file containing an Extensions proto.")
    private List<String> extensions = new ArrayList<>();

    @Parameter(
        names = {"--profiles"},
        description = "Prototxt file containing a Profiles proto.")
    private List<String> profiles = new ArrayList<>();

    @Parameter(
        names = {"--struct_def_dep_zip"},
        description = "Zip file containing structure definitions that profiles might depend on.")
    private List<String> structDefDepZips = new ArrayList<>();

    @Parameter(
        names = {"--stu3_struct_def_zip"},
        description = "Zip file containing core STU3 Structure Definitions.")
    private String stu3StructDefZip = null;

    @Parameter(
        names = {"--r4_struct_def_zip"},
        description = "Zip file containing core R4 Structure Definitions.")
    private String r4StructDefZip = null;
  }

  private ProfileGeneratorMain() {}

  private static Profiles readProfiles(String filename) throws IOException {
    return FileUtils.mergeText(new File(filename), Profiles.newBuilder()).build();
  }

  private static Extensions readExtensions(String filename) throws IOException {
    return FileUtils.mergeText(new File(filename), Extensions.newBuilder()).build();
  }

  private static PackageInfo readPackageInfo(String filename) throws IOException {
    return FileUtils.mergeText(new File(filename), PackageInfo.newBuilder()).build();
  }

  public static void main(String[] argv) throws IOException {
    Args args = new Args();
    JCommander jcommander = new JCommander(args);

    try {
      jcommander.parse(argv);
    } catch (ParameterException exception) {
      System.err.println("Invalid usage: " + exception.getMessage());
      System.exit(1);
    }

    Profiles.Builder combinedProfilesBuilder = Profiles.newBuilder();
    for (String profilesFile : args.profiles) {
      combinedProfilesBuilder.addAllProfile(readProfiles(profilesFile).getProfileList());
    }

    Extensions.Builder combinedExtensionsBuilder = Extensions.newBuilder();
    for (String extensionsFile : args.extensions) {
      Extensions extensions = readExtensions(extensionsFile);
      combinedExtensionsBuilder.addAllSimpleExtension(extensions.getSimpleExtensionList());
      combinedExtensionsBuilder.addAllComplexExtension(extensions.getComplexExtensionList());
    }

    PackageInfo packageInfo = readPackageInfo(args.packageInfo);

    List<StructureDefinition> baseStructDefPool = new ArrayList<>();
    for (String zip : args.structDefDepZips) {
      baseStructDefPool.addAll(
          FhirPackage.load(zip, packageInfo.getFhirVersion()).structureDefinitions);
    }

    switch (packageInfo.getFhirVersion()) {
      case STU3:
        if (args.stu3StructDefZip == null) {
          throw new IllegalArgumentException(
              "Profile is for STU3, but --stu3_struct_def_zip is not specified.");
        }
        baseStructDefPool.addAll(
            FhirPackage.load(args.stu3StructDefZip, FhirVersion.STU3).structureDefinitions);
        break;
      case R4:
        if (args.r4StructDefZip == null) {
          throw new IllegalArgumentException(
              "Profile is for R4, but --r4_struct_def_zip is not specified.");
        }
        baseStructDefPool.addAll(FhirPackage.load(args.r4StructDefZip).structureDefinitions);
        break;
      default:
        throw new IllegalArgumentException(
            "FHIR version not supported by ProfileGenerator: " + packageInfo.getFhirVersion());
    }

    ProfileGenerator profileGenerator =
        new ProfileGenerator(
            packageInfo,
            combinedProfilesBuilder.build(),
            combinedExtensionsBuilder.build(),
            baseStructDefPool,
            LocalDate.now());

    writeBundle(profileGenerator.generateProfiles(), args.outputDirectory, args.name);
    writeBundle(
        profileGenerator.generateExtensions(), args.outputDirectory, args.name + "_extensions");
  }

  private static void writeBundle(Bundle bundle, String dir, String name) throws IOException {
    String filename = dir + "/" + name + ".json";
    Files.asCharSink(new File(filename), StandardCharsets.UTF_8).write(jsonPrinter.print(bundle));
  }
}
