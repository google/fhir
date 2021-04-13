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

import static com.google.common.truth.Truth.assertThat;

import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.proto.ProtoGeneratorAnnotations;
import com.google.protobuf.ExtensionRegistry;
import java.io.IOException;
import java.util.HashSet;
import java.util.Set;

/** Utilities for tests involving the ProtoGenerator. */
public final class ProtoGeneratorTestUtils {

  private ProtoGeneratorTestUtils() {}

  public static void initializeRegistry(ExtensionRegistry registry) throws IOException {
    Annotations.registerAllExtensions(registry);
    ProtoGeneratorAnnotations.registerAllExtensions(registry);
  }

  public static ProtoGenerator makeProtoGenerator(
      String packageLocation,
      String codesProtoImport,
      ImmutableMap<String, String> coreDepMap,
      ImmutableSet<String> dependencyLocations)
      throws IOException, InvalidFhirException {
    FhirPackage packageToGenerate = FhirPackage.load(packageLocation);
    PackageInfo packageInfo = packageToGenerate.packageInfo;

    Set<FhirPackage> packages = new HashSet<>();
    packages.add(packageToGenerate);
    for (String location : dependencyLocations) {
      packages.add(FhirPackage.load(location));
    }

    String coreDep = coreDepMap.get(packageInfo.getFhirVersion().toString());
    if (coreDep == null) {
      throw new IllegalArgumentException(
          "Unable to load core dep for fhir version: " + packageInfo.getFhirVersion());
    }
    packages.add(FhirPackage.load(coreDep));

    return new ProtoGenerator(
        packageInfo,
        codesProtoImport,
        ImmutableSet.copyOf(packages),
        new ValueSetGenerator(packageInfo, packages));
  }

  public static void testProtoFiles(String generatedFile, String goldenFile) {
    assertThat(cleanProtoFile(goldenFile)).isEqualTo(cleanProtoFile(generatedFile));
  }

  // Removes discrepancies from generated code introduced by formatting tools. Includes:
  // * Removing comment lines.  This is because they can be reformatted into multiline comments,
  //   with double-slashes added.
  // * Removes import statements.  This is because sometimes the protogenerator will include an
  //   extra dep (such as extensions or codes imports) that can be pruned by a clean-up tool.
  //   TODO: be smarter about which imports we include per resource.
  // * Replaces any repeated whitespace with a single white space
  // * Removes insignificant whitespace between control symbols.
  private static String cleanProtoFile(String protoFile) {
    return protoFile
        .replaceAll("(?m)^\\s*//.*$", "")
        .replaceAll("(?m)^import \".*\";$", "")
        .replaceAll("\\s+", " ")
        .replace("[ (", "[(")
        .replace("\" ]", "\"]");
  }
}
