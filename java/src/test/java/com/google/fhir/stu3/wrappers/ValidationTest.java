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

package com.google.fhir.stu3.wrappers;

import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.stu3.google.PrimitiveHasNoValue;
import com.google.fhir.stu3.proto.Boolean;
import com.google.fhir.stu3.uscore.UsCoreDirectEmail;
import com.google.fhir.testing.ValidationTestBase;
import com.google.protobuf.TextFormat;
import java.io.IOException;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Validation tests for STU3 primitive types. */
@RunWith(JUnit4.class)
public final class ValidationTest extends ValidationTestBase {

  public ValidationTest() {
    super(
        "stu3",
        PrimitiveHasNoValue.newBuilder()
            .setValueBoolean(Boolean.newBuilder().setValue(true))
            .build(),
        UsCoreDirectEmail.newBuilder()
            .setValueBoolean(Boolean.newBuilder().setValue(true))
            .build());
  }

  @Before
  public void setUp() throws IOException {
    jsonParser = JsonFormat.getParser();
    textParser = TextFormat.getParser();
    runfiles = Runfiles.create();
  }

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
