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

import com.google.common.io.Files;
import com.google.fhir.stu3.proto.StructureDefinition;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

/** Utilities related to loading FHIR data from files. */
public final class FileUtils {

  private FileUtils() {}

  public static StructureDefinition loadStructureDefinition(String fullFilename)
      throws IOException {
    return loadStructureDefinition(new File(fullFilename));
  }

  public static StructureDefinition loadStructureDefinition(File file) throws IOException {
    String structDefString = Files.toString(file, StandardCharsets.UTF_8);
    StructureDefinition.Builder structDefBuilder = StructureDefinition.newBuilder();
    JsonFormat.Parser.newBuilder().build().merge(structDefString, structDefBuilder);
    return structDefBuilder.build();
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
}
