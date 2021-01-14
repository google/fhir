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

package com.google.fhir.testing;

import static com.google.common.truth.Truth.assertThat;
import static java.nio.charset.StandardCharsets.UTF_8;
import static org.junit.Assert.assertThrows;

import com.google.common.base.CaseFormat;
import com.google.common.base.Splitter;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.wrappers.ExtensionWrapper;
import com.google.fhir.wrappers.PrimitiveWrappers;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.List;
import org.junit.Assert;

/** Unit tests for primitive wrapper validation. */
public abstract class ValidationTestBase {
  private static final String TESTDATA_BASEDIR = "com_google_fhir/testdata/";
  private static final String PROTO_DELIMITER = "\n---\n";

  protected JsonFormat.Parser jsonParser;
  protected TextFormat.Parser textParser;
  protected Runfiles runfiles;

  private final String validationDir;
  private final Message primitiveHasNoValue;
  private final Message arbitraryExtension;

  protected ValidationTestBase(
      String versionName, Message primitiveHasNoValue, Message arbitraryExtension) {
    this.validationDir = Paths.get(TESTDATA_BASEDIR, versionName, "validation").toString();
    this.primitiveHasNoValue = primitiveHasNoValue;
    this.arbitraryExtension = arbitraryExtension;
  }

  /** Test parsing a set of valid and invalid inputs for the given type. */
  protected void testJsonValidation(Message.Builder type) throws IOException, InvalidFhirException {
    for (java.lang.String line : readLines(type.clone(), true)) {
      expectValid(line, type.clone());
    }
    for (java.lang.String line : readLines(type.clone(), false)) {
      expectInvalid(line, type.clone());
    }
  }

  /** Test reading and parsing a set of .prototxt definitions for the provided message. */
  protected void testProtoValidation(Message message) throws IOException {
    String messageName = message.getDescriptorForType().getFullName();
    // Test cases that are common to all primitives

    // It's ok to have no value if there's another extension present.
    Message.Builder onlyExtensions = message.newBuilderForType();
    ExtensionWrapper.of()
        .add(primitiveHasNoValue)
        .add(arbitraryExtension)
        .addToMessage(onlyExtensions);
    try {
      PrimitiveWrappers.validatePrimitive(onlyExtensions);
    } catch (Exception e) {
      Assert.fail(messageName + " with only extensions should pass, got: " + e.getMessage());
    }

    // But it's not okay to JUST have the no value extension (and no other extensions).
    Message.Builder justNoValue = message.newBuilderForType();
    ExtensionWrapper.of().add(primitiveHasNoValue).addToMessage(justNoValue);

    InvalidFhirException primitiveException =
        assertThrows(
            InvalidFhirException.class, () -> PrimitiveWrappers.validatePrimitive(justNoValue));
    assertThat(primitiveException).hasMessageThat().contains("PrimitiveHasNoValue");

    // Run individual cases from testdata files.
    Path fileBase =
        Paths.get(validationDir, camelCaseToSnakeCase(message.getDescriptorForType().getName()));

    // Check the valid file
    Iterable<String> validProtoStrings =
        Splitter.on(PROTO_DELIMITER).split(loadText(fileBase + ".valid.prototxt"));
    for (String validProtoString : validProtoStrings) {
      Message.Builder testBuilder = message.newBuilderForType();
      textParser.merge(validProtoString, testBuilder);
      try {
        PrimitiveWrappers.validatePrimitive(testBuilder);
      } catch (Exception e) {
        Assert.fail(
            messageName + " expected valid " + validProtoString + "\nGot: " + e.getMessage());
      }
    }

    // Check the invalid file if present.
    String[] invalidProtoStrings = null;
    try {
      invalidProtoStrings = loadText(fileBase + ".invalid.prototxt").split(PROTO_DELIMITER);
    } catch (IOException e) {
      // Not all primitives have invalid files - e.g., it's impossible to have a Boolean with an
      // invalid value.
    }
    if (invalidProtoStrings != null) {
      for (String invalidProtoString : invalidProtoStrings) {
        Message.Builder testBuilder = message.newBuilderForType();
        textParser.merge(invalidProtoString, testBuilder);
        assertThrows(Exception.class, () -> PrimitiveWrappers.validatePrimitive(testBuilder));
      }
    }
  }

  /** Read the specified ndjson file from the testdata directory as a List of Strings. */
  private List<String> readLines(Message.Builder type, boolean valid) throws IOException {
    String fileExtension = valid ? ".valid.ndjson" : ".invalid.ndjson";
    Path path =
        Paths.get(
            runfiles.rlocation(validationDir),
            camelCaseToSnakeCase(type.getDescriptorForType().getName()) + fileExtension);
    return Files.readAllLines(path, UTF_8);
  }

  /** Read the specifed text file from the testdata directory as a String. */
  private String loadText(String filepath) throws IOException {
    File file = new File(runfiles.rlocation(filepath));
    return com.google.common.io.Files.asCharSource(file, UTF_8).read();
  }

  /** Parse the given line, expecting it to be valid. */
  private void expectValid(String line, Message.Builder builder)
      throws IOException, InvalidFhirException {
    jsonParser.merge(line, builder);
  }

  /** Parse the given line, expecting it to be invalid. */
  private void expectInvalid(String line, Message.Builder builder) throws IOException {
    InvalidFhirException exception =
        assertThrows(InvalidFhirException.class, () -> jsonParser.merge(line, builder));
    assertThat(exception).hasMessageThat().containsMatch("Invalid|Unknown|Error");
  }

  private static String camelCaseToSnakeCase(String name) {
    return CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, name);
  }
}
