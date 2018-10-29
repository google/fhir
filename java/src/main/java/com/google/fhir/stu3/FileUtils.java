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
import com.google.fhir.stu3.proto.Bundle;
import com.google.fhir.stu3.proto.StructureDefinition;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.List;
import java.util.stream.Collectors;
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

  public static List<StructureDefinition> loadStructureDefinitionsInDir(String dir)
      throws IOException {
    List<File> files =
        Arrays.stream(new File(dir).listFiles())
            .filter(file -> file.getName().endsWith(".json"))
            .sorted()
            .collect(Collectors.toList());

    List<StructureDefinition> structDefs = new ArrayList<>(files.size());
    for (File file : files) {
      structDefs.add(loadStructureDefinition(file));
    }
    return structDefs;
  }

  public static List<StructureDefinition> loadStructureDefinitionsInZip(String zip)
      throws IOException {
    JsonFormat.Parser parser = JsonFormat.Parser.newBuilder().build();
    List<StructureDefinition> structDefs = new ArrayList<>();
    ZipFile zipFile = new ZipFile(new File(zip));
    Enumeration<? extends ZipEntry> entries = zipFile.entries();
    while (entries.hasMoreElements()) {
      String json =
          new String(ByteStreams.toByteArray(zipFile.getInputStream(entries.nextElement())), UTF_8);
      try {
        StructureDefinition.Builder structDefBuilder = StructureDefinition.newBuilder();
        parser.merge(json, structDefBuilder);
        structDefs.add(structDefBuilder.build());
      } catch (IllegalArgumentException e) {
        // Couldn't parse as structure definitions.  Check if it's a bundle of structure definitions
        Bundle.Builder bundleBuilder = Bundle.newBuilder();
        parser.merge(json, bundleBuilder);
        for (Bundle.Entry entry : bundleBuilder.getEntryList()) {
          if (!entry.getResource().hasStructureDefinition()) {
            throw new IllegalArgumentException(
                "Zip "
                    + zip
                    + " must be a zip of either"
                    + "structure definitions, or bundles of structure definitions.",
                e);
          }
          structDefs.add(entry.getResource().getStructureDefinition());
        }
      }
    }
    return structDefs;
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
