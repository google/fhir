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
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import com.google.common.base.CaseFormat;
import com.google.common.base.Splitter;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.stu3.google.PrimitiveHasNoValue;
import com.google.fhir.stu3.proto.Boolean;
import com.google.fhir.stu3.proto.Extension;
import com.google.fhir.stu3.proto.Uri;
import com.google.protobuf.Message;
import com.google.protobuf.Message.Builder;
import com.google.protobuf.TextFormat;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Validation tests for primitive types. For each tested type, we load two ndjson file */
// TODO: Update this test (and corresponding c++ test) to R4
@RunWith(JUnit4.class)
public final class ValidationTest {

  private JsonFormat.Parser jsonParser;
  private TextFormat.Parser textParser;
  private Runfiles runfiles;

  /** Parse the given line, expecting it to be valid. */
  private void expectValid(java.lang.String line, Builder builder) throws IOException {
    jsonParser.merge(line, builder);
  }

  /** Parse the given line, expecting it to be invalid. */
  private void expectInvalid(java.lang.String line, Builder builder) throws IOException {
    IllegalArgumentException exception =
        assertThrows(IllegalArgumentException.class, () -> jsonParser.merge(line, builder));
    assertThat(exception).hasMessageThat().containsMatch("Invalid|Unknown|Error");
  }

  /** Read the specified ndjson file from the testdata directory as a List of Strings. */
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

  /** Read the specifed text file from the testdata directory as a String. */
  private String loadText(String filename) throws IOException {
    File file = new File(runfiles.rlocation("com_google_fhir/" + filename));
    return com.google.common.io.Files.asCharSource(file, StandardCharsets.UTF_8).read();
  }

  /** Test parsing a set of valid and invalid inputs for the given type. */
  private void testJsonValidation(Builder type) throws IOException {
    for (java.lang.String line : readLines(type.clone(), true)) {
      expectValid(line, type.clone());
    }
    for (java.lang.String line : readLines(type.clone(), false)) {
      expectInvalid(line, type.clone());
    }
  }

  private static final PrimitiveHasNoValue PRIMITIVE_HAS_NO_VALUE =
      PrimitiveHasNoValue.newBuilder().setValueBoolean(Boolean.newBuilder().setValue(true)).build();
  private static final Extension ARBITRARY_EXTENSION =
      Extension.newBuilder()
          .setUrl(Uri.newBuilder().setValue("abcd"))
          .setValue(Extension.Value.newBuilder().setBoolean(Boolean.newBuilder().setValue(true)))
          .build();

  private void testProtoValidation(Message message) throws IOException {
    String messageName = message.getDescriptorForType().getFullName();
    // Test cases that are common to all primitives

    // It's ok to have no value if there's another extension present.
    Message.Builder onlyExtensions = message.newBuilderForType();
    ExtensionWrapper.of()
        .add(PRIMITIVE_HAS_NO_VALUE)
        .add(ARBITRARY_EXTENSION)
        .addToMessage(onlyExtensions);
    try {
      PrimitiveWrappers.validatePrimitive(onlyExtensions);
    } catch (Exception e) {
      Assert.fail(messageName + " with only extensions should pass, got: " + e.getMessage());
    }

    // But it's not okay to JUST have the no value extension (and no other extensions).
    Message.Builder justNoValue = message.newBuilderForType();
    ExtensionWrapper.of().add(PRIMITIVE_HAS_NO_VALUE).addToMessage(justNoValue);

    IllegalArgumentException primitiveException = assertThrows(IllegalArgumentException.class,
        () -> PrimitiveWrappers.validatePrimitive(justNoValue));
    assertTrue(primitiveException.getMessage().contains("PrimitiveHasNoValue"));

    // Run individual cases from testdata files.
    String fileBase =
        "testdata/stu3/validation/"
            + CaseFormat.UPPER_CAMEL.to(
                CaseFormat.LOWER_UNDERSCORE, message.getDescriptorForType().getName());

    // Check the valid file
    Iterable<String> validProtoStrings =
        Splitter.on("\n---\n").split(loadText(fileBase + ".valid.prototxt"));
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
      invalidProtoStrings = loadText(fileBase + ".invalid.prototxt").split("\n---\n");
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

  @Before
  public void setUp() throws IOException {
    jsonParser = JsonFormat.getParser();
    textParser = TextFormat.getParser();
    runfiles = Runfiles.create();
  }

  /* Test validation of primitive types. */

  @Test
  public void testBase64Binary() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.Base64Binary.newBuilder());
  }

  @Test
  public void testBoolean() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.Boolean.newBuilder());
  }

  @Test
  public void testCode() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.Code.newBuilder());
  }

  @Test
  public void testDate() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.Date.newBuilder());
  }

  @Test
  public void testDateTime() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.DateTime.newBuilder());
  }

  @Test
  public void testDecimal() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.Decimal.newBuilder());
  }

  @Test
  public void testId() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.Id.newBuilder());
  }

  @Test
  public void testInstant() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.Instant.newBuilder());
  }

  @Test
  public void testInteger() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.Integer.newBuilder());
  }

  @Test
  public void testMarkdown() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.Markdown.newBuilder());
  }

  @Test
  public void testOid() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.Oid.newBuilder());
  }

  @Test
  public void testPositiveInt() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.PositiveInt.newBuilder());
  }

  @Test
  public void testReference() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.Reference.newBuilder());
  }

  @Test
  public void testString() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.String.newBuilder());
  }

  @Test
  public void testTime() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.Time.newBuilder());
  }

  @Test
  public void testUnsignedInt() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.UnsignedInt.newBuilder());
  }

  @Test
  public void testUri() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.Uri.newBuilder());
  }

  @Test
  public void testXhtml() throws IOException {
    testJsonValidation(com.google.fhir.stu3.proto.Xhtml.newBuilder());
  }

  @Test
  public void testValidatePrimitiveBase64Binary() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.Base64Binary.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveBoolean() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.Boolean.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveCode() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.Code.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveDate() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.Date.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveDateTime() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.DateTime.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveDecimal() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.Decimal.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveId() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.Id.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveInstant() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.Instant.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveInteger() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.Integer.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveMarkdown() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.Markdown.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveOid() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.Oid.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitivePositiveInt() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.PositiveInt.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveString() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.String.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveTime() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.Time.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveUnsignedInt() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.UnsignedInt.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveUri() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.Uri.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveTypedCode() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.AdministrativeGenderCode.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveStringCode() throws IOException {
    testProtoValidation(com.google.fhir.stu3.proto.MimeTypeCode.getDefaultInstance());
  }
}
