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

package com.google.fhir.wrappers;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.fail;

import com.google.common.base.CaseFormat;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.common.JsonFormat;
import com.google.protobuf.Message.Builder;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Validation tests for primitive types. For each tested type, we load two ndjson file */
@RunWith(JUnit4.class)
public final class ValidationTest {

  private JsonFormat.Parser jsonParser;
  private Runfiles runfiles;

  /** Parse the given line, expecting it to be valid. */
  private void expectValid(java.lang.String line, Builder builder) throws IOException {
    jsonParser.merge(line, builder);
  }

  /** Parse the given line, expecting it to be invalid. */
  private void expectInvalid(java.lang.String line, Builder builder) throws IOException {
    try {
      jsonParser.merge(line, builder);
      fail("Unexpected parse success for input: '" + line + "', result is " + builder.toString());
    } catch (IllegalArgumentException exception) {
      assertThat(exception).hasMessageThat().containsMatch("Invalid|Unknown|Error");
    }
  }

  /** Read the specified ndjson file from the testdata directory as a List<String>. */
  private java.util.List<java.lang.String> readLines(Builder type, boolean valid)
      throws IOException {
    Path path =
        Paths.get(
            runfiles.rlocation("com_google_fhir/testdata/stu3/validation/"),
            CaseFormat.UPPER_CAMEL.to(
                    CaseFormat.LOWER_UNDERSCORE, type.getDescriptorForType().getName())
                + (valid ? ".valid.ndjson" : ".invalid.ndjson"));
    return Files.readAllLines(path, StandardCharsets.UTF_8);
  }

  /** Test parsing a set of valid and invalid inputs for the given type. */
  private void testValidation(Builder type) throws IOException {
    for (java.lang.String line : readLines(type.clone(), true)) {
      expectValid(line, type.clone());
    }
    for (java.lang.String line : readLines(type.clone(), false)) {
      expectInvalid(line, type.clone());
    }
  }

  @Before
  public void setUp() throws IOException {
    jsonParser = JsonFormat.getParser();
    runfiles = Runfiles.create();
  }

  /* Test validation of primitive types. */

  @Test
  public void testBase64Binary() throws IOException {
    testValidation(com.google.fhir.stu3.proto.Base64Binary.newBuilder());
  }

  @Test
  public void testBoolean() throws IOException {
    testValidation(com.google.fhir.stu3.proto.Boolean.newBuilder());
  }

  @Test
  public void testCode() throws IOException {
    testValidation(com.google.fhir.stu3.proto.Code.newBuilder());
  }

  @Test
  public void testDate() throws IOException {
    testValidation(com.google.fhir.stu3.proto.Date.newBuilder());
  }

  @Test
  public void testDateTime() throws IOException {
    testValidation(com.google.fhir.stu3.proto.DateTime.newBuilder());
  }

  @Test
  public void testDecimal() throws IOException {
    testValidation(com.google.fhir.stu3.proto.Decimal.newBuilder());
  }

  @Test
  public void testId() throws IOException {
    testValidation(com.google.fhir.stu3.proto.Id.newBuilder());
  }

  @Test
  public void testInstant() throws IOException {
    testValidation(com.google.fhir.stu3.proto.Instant.newBuilder());
  }

  @Test
  public void testInteger() throws IOException {
    testValidation(com.google.fhir.stu3.proto.Integer.newBuilder());
  }

  @Test
  public void testMarkdown() throws IOException {
    testValidation(com.google.fhir.stu3.proto.Markdown.newBuilder());
  }

  @Test
  public void testOid() throws IOException {
    testValidation(com.google.fhir.stu3.proto.Oid.newBuilder());
  }

  @Test
  public void testPositiveInt() throws IOException {
    testValidation(com.google.fhir.stu3.proto.PositiveInt.newBuilder());
  }

  @Test
  public void testReference() throws IOException {
    testValidation(com.google.fhir.stu3.proto.Reference.newBuilder());
  }

  @Test
  public void testString() throws IOException {
    testValidation(com.google.fhir.stu3.proto.String.newBuilder());
  }

  @Test
  public void testTime() throws IOException {
    testValidation(com.google.fhir.stu3.proto.Time.newBuilder());
  }

  @Test
  public void testUnsignedInt() throws IOException {
    testValidation(com.google.fhir.stu3.proto.UnsignedInt.newBuilder());
  }

  @Test
  public void testUri() throws IOException {
    testValidation(com.google.fhir.stu3.proto.Uri.newBuilder());
  }

  @Test
  public void testXhtml() throws IOException {
    testValidation(com.google.fhir.stu3.proto.Xhtml.newBuilder());
  }
}
