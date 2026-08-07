//    Copyright 2020 Google Inc.
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
import static java.nio.charset.StandardCharsets.UTF_8;
import static org.junit.Assert.fail;

import com.google.common.io.ByteStreams;
import com.google.common.io.Files;
import java.io.File;
import java.io.InputStream;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Generic test case for ensuring generated protos are up to date. This rule is automatically added
 * to every profile set by the generation rules. Note: this file should be kept clear of
 * dependencies on the core fhir package other than ProtoGeneratorTestUtils, so that it works both
 * inside and outside of the core lib.
 */
@RunWith(JUnit4.class)
public final class GeneratedProtoTest {

  @Test
  public void testGeneratedProto() throws Exception {
    Map<String, String> generatedContentsByFilename = new HashMap<>();
    ZipFile zipFile = new ZipFile(new File(System.getProperty("generated_zip")));
    Enumeration<? extends ZipEntry> entries = zipFile.entries();
    while (entries.hasMoreElements()) {
      ZipEntry entry = entries.nextElement();
      InputStream stream = zipFile.getInputStream(entry);
      generatedContentsByFilename.put(
          entry.getName(), new String(ByteStreams.toByteArray(stream), UTF_8));
    }

    Map<String, String> goldenContentsByFilename = new HashMap<>();
    File dir = new File(System.getProperty("golden_dir"));
    File[] directoryListing = dir.listFiles();
    if (directoryListing == null) {
      fail("Unable to load files from directory: " + System.getProperty("golden_dir"));
    }

    for (File file : directoryListing) {
      if (!file.isDirectory()) {
        goldenContentsByFilename.put(file.getName(), Files.asCharSource(file, UTF_8).read());
      }
    }

    for (String filename : generatedContentsByFilename.keySet()) {
      if (filename.startsWith(".")) {
        continue;
      }
      assertThat(goldenContentsByFilename).containsKey(filename);
      assertThat(cleanProtoFile(generatedContentsByFilename.get(filename)))
          .isEqualTo(cleanProtoFile(goldenContentsByFilename.get(filename)));
    }
  }

  // Removes discrepancies from generated code introduced by formatting tools. Includes:
  // * Removing comment lines.  This is because they can be reformatted into multiline comments,
  //   with double-slashes added.
  // * Removes import statements.  This is because sometimes the protogenerator will include an
  //   extra dep (such as extensions or codes imports) that can be pruned by a clean-up tool.
  //   TODO(b/185161283): be smarter about which imports we include per resource.
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
