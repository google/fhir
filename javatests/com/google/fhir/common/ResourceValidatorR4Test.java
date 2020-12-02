// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.fhir.common;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.google.common.io.Files;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.r4.core.Bundle;
import com.google.fhir.r4.core.Encounter;
import com.google.fhir.r4.core.Observation;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for ResourceValidator using R4 data. */
@RunWith(JUnit4.class)
public final class ResourceValidatorR4Test {

  private Runfiles runfiles;
  private ResourceValidator validator;
  protected TextFormat.Parser textParser;

  @Before
  public void setUp() throws IOException {
    validator = new ResourceValidator();
    runfiles = Runfiles.create();
    textParser = TextFormat.getParser();
  }

  private String loadError(String name) throws IOException {
    File file =
        new File(
            runfiles.rlocation(
                "com_google_fhir/testdata/r4/validation/" + name + ".result.txt"));
    return Files.asCharSource(file, StandardCharsets.UTF_8).read().trim();
  }

  private Message.Builder parseProto(String name, Message.Builder builder) throws IOException {
    File file =
        new File(
            runfiles.rlocation(
                "com_google_fhir/testdata/r4/validation/" + name + ".prototxt"));
    textParser.merge(Files.asCharSource(file, StandardCharsets.UTF_8).read(), builder);
    return builder;
  }

  private void validTest(String name, Message.Builder builder)
      throws InvalidFhirException, IOException {
    validator.validateResource(parseProto(name, builder));
  }

  private void invalidTest(String name, Message.Builder builder) throws IOException {
    String errorMsg = loadError(name);
    InvalidFhirException e =
        assertThrows(
            InvalidFhirException.class,
            () -> validator.validateResource(parseProto(name, builder)));
    assertThat(e).hasMessageThat().isEqualTo(errorMsg);
  }

  @Test
  public void testMissingRequiredField() throws Exception {
    invalidTest("observation_invalid_missing_required", Observation.newBuilder());
  }

  @Test
  public void testInvalidPrimitiveField() throws Exception {
    invalidTest("observation_invalid_primitive", Observation.newBuilder());
  }

  @Test
  public void testValidReference() throws Exception {
    validTest("observation_valid_reference", Observation.newBuilder());
  }

  @Test
  public void testInvalidReference() throws Exception {
    invalidTest("observation_invalid_reference", Observation.newBuilder());
  }

  @Test
  public void testRepeatedReferenceValid() throws Exception {
    validTest("encounter_valid_repeated_reference", Encounter.newBuilder());
  }

  @Test
  public void testRepeatedReferenceInvalid() throws Exception {
    invalidTest("encounter_invalid_repeated_reference", Encounter.newBuilder());
  }

  @Test
  public void testEmptyOneof() throws Exception {
    invalidTest("observation_invalid_empty_oneof", Observation.newBuilder());
  }

  @Test
  public void testValidBundle() throws Exception {
    validTest("bundle_valid", Bundle.newBuilder());
  }

  @Test
  public void testStartLaterThanEnd() throws Exception {
    invalidTest("encounter_invalid_start_later_than_end", Encounter.newBuilder());
  }

  @Test
  public void testStartLaterThanEndButEndHasDayPrecision() throws Exception {
    validTest("encounter_valid_start_later_than_end_day_precision", Encounter.newBuilder());
  }

  @Test
  public void testValidEncounter() throws Exception {
    validTest("encounter_valid", Encounter.newBuilder());
  }
}
