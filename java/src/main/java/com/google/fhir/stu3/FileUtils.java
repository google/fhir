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

package com.google.fhir.stu3;

import static java.nio.charset.StandardCharsets.UTF_8;

import com.google.common.io.ByteStreams;
import com.google.common.io.Files;
import com.google.fhir.dstu2.StructureDefinitionTransformer;
import com.google.fhir.proto.Annotations.FhirVersion;
import com.google.fhir.r4.core.Bundle;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/** Utilities related to loading FHIR data from files. */
public final class FileUtils {

  private FileUtils() {}

  public static StructureDefinition loadStructureDefinition(String fullFilename)
      throws IOException {
    return loadStructureDefinition(new File(fullFilename));
  }

  public static StructureDefinition loadStructureDefinition(File file) throws IOException {
    return (StructureDefinition) loadFhir(file, StructureDefinition.newBuilder());
  }

  public static List<StructureDefinition> loadStructureDefinitionsInZip(String zip)
      throws IOException {
    return loadTypesInZip(StructureDefinition.newBuilder().build(), zip, FhirVersion.STU3);
  }

  // Given a zip and a FHIR resource type, returns all the resources of that type
  // found in the zip.  This will get all loose resources of that type in the zip,
  // AND all resources of that type located in bundles found in the zip.
  @SuppressWarnings("unchecked")
  public static <T extends Message> List<T> loadTypesInZip(
      T type, String zip, FhirVersion fhirVersion) throws IOException {
    JsonFormat.Parser parser =
        fhirVersion == FhirVersion.DSTU2 || fhirVersion == fhirVersion.STU3
            ? JsonFormat.getEarlyVersionGeneratorParser()
            : JsonFormat.getParser();
    List<T> extracted = new ArrayList<>();
    ZipFile zipFile = new ZipFile(new File(zip));
    Enumeration<? extends ZipEntry> entries = zipFile.entries();
    while (entries.hasMoreElements()) {
      String json =
          new String(ByteStreams.toByteArray(zipFile.getInputStream(entries.nextElement())), UTF_8);
      if (fhirVersion == FhirVersion.DSTU2) {
        json = StructureDefinitionTransformer.transformDstu2ToStu3(json);
      }
      try {
        Message.Builder typeBuilder = type.newBuilderForType();
        parser.merge(json, typeBuilder);
        extracted.add((T) (typeBuilder.build()));
      } catch (IllegalArgumentException e) {
        // Couldn't parse as requested.
        // Try to parse as a bundle, and if that works, pull out all instances of the requested
        // type.
        try {
          Bundle.Builder bundleBuilder = Bundle.newBuilder();
          parser.merge(json, bundleBuilder);
          for (Bundle.Entry entry : bundleBuilder.getEntryList()) {
            Message contained = ResourceUtils.getContainedResource(entry.getResource());
            if (contained
                .getDescriptorForType()
                .getFullName()
                .equals(type.getDescriptorForType().getFullName())) {
              extracted.add((T) (contained));
            }
          }
        } catch (IllegalArgumentException e2) {
          // This is neither a structure definition, or a bundle.  Ignore.
        }
      }
    }
    return extracted;
  }

  public static Message loadFhir(String filename, Message.Builder builder) throws IOException {
    return loadFhir(new File(filename), builder);
  }

  public static Message loadFhir(File file, Message.Builder builder) throws IOException {
    String json = Files.asCharSource(file, StandardCharsets.UTF_8).read();
    JsonFormat.Parser.newBuilder().build().merge(json, builder);
    return builder.build();
  }

  /** Read the specifed prototxt file and parse it. */
  public static <T extends Message.Builder> T mergeText(File file, T builder) throws IOException {
    TextFormat.getParser().merge(Files.asCharSource(file, StandardCharsets.UTF_8).read(), builder);
    return builder;
  }
}
