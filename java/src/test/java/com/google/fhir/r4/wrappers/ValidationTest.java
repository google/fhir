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

package com.google.fhir.r4.wrappers;

import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.r4.core.Boolean;
import com.google.fhir.r4.google.PrimitiveHasNoValue;
import com.google.fhir.r4.uscore.UsCoreDirectEmail;
import com.google.fhir.testing.ValidationTestBase;
import com.google.protobuf.TextFormat;
import java.io.IOException;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Validation tests for primitive types. For each tested type, we load two ndjson file */
@RunWith(JUnit4.class)
public final class ValidationTest extends ValidationTestBase {

  public ValidationTest() {
    super(
        "r4",
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
    testJsonValidation(com.google.fhir.r4.core.Base64Binary.newBuilder());
  }

  @Test
  public void testBoolean() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Boolean.newBuilder());
  }

  @Test
  public void testCanonical() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Canonical.newBuilder());
  }

  @Test
  public void testCode() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Code.newBuilder());
  }

  @Test
  public void testDate() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Date.newBuilder());
  }

  @Test
  public void testDateTime() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.DateTime.newBuilder());
  }

  @Test
  public void testDecimal() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Decimal.newBuilder());
  }

  @Test
  public void testId() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Id.newBuilder());
  }

  @Test
  public void testInstant() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Instant.newBuilder());
  }

  @Test
  public void testInteger() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Integer.newBuilder());
  }

  @Test
  public void testMarkdown() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Markdown.newBuilder());
  }

  @Test
  public void testOid() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Oid.newBuilder());
  }

  @Test
  public void testPositiveInt() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.PositiveInt.newBuilder());
  }

  @Test
  public void testReference() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Reference.newBuilder());
  }

  @Test
  public void testString() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.String.newBuilder());
  }

  @Test
  public void testTime() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Time.newBuilder());
  }

  @Test
  public void testUnsignedInt() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.UnsignedInt.newBuilder());
  }

  @Test
  public void testUri() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Uri.newBuilder());
  }

  @Test
  public void testUrl() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Url.newBuilder());
  }

  @Test
  public void testXhtml() throws IOException {
    testJsonValidation(com.google.fhir.r4.core.Xhtml.newBuilder());
  }

  @Test
  public void testValidatePrimitiveBase64Binary() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.Base64Binary.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveBoolean() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.Boolean.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveCanonical() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.Canonical.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveCode() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.Code.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveDate() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.Date.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveDateTime() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.DateTime.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveDecimal() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.Decimal.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveId() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.Id.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveInstant() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.Instant.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveInteger() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.Integer.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveMarkdown() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.Markdown.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveOid() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.Oid.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitivePositiveInt() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.PositiveInt.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveString() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.String.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveTime() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.Time.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveUnsignedInt() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.UnsignedInt.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveUri() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.Uri.getDefaultInstance());
  }

  @Test
  public void testValidatePrimitiveUrl() throws IOException {
    testProtoValidation(com.google.fhir.r4.core.Url.getDefaultInstance());
  }
}
