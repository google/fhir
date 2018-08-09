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

import static com.google.common.truth.Truth.assertThat;

import com.google.api.client.json.gson.GsonFactory;
import com.google.api.services.bigquery.model.TableFieldSchema;
import com.google.api.services.bigquery.model.TableSchema;
import com.google.common.collect.ImmutableList;
import com.google.common.io.Files;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.stu3.proto.Base64Binary;
import com.google.fhir.stu3.proto.Composition;
import com.google.fhir.stu3.proto.DateTime;
import com.google.fhir.stu3.proto.Integer;
import com.google.fhir.stu3.proto.Patient;
import com.google.protobuf.Message;
import com.google.protobuf.Message.Builder;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.time.ZoneId;
import java.util.ArrayList;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class BigQuerySchemaTest {

  private JsonFormat.Parser jsonParser;
  private GsonFactory gsonFactory;
  private Runfiles runfiles;

  @Before
  public void setUp() throws IOException {
    jsonParser =
        JsonFormat.Parser.newBuilder().withDefaultTimeZone(ZoneId.of("Australia/Sydney")).build();
    gsonFactory = new GsonFactory();
    runfiles = Runfiles.create();
  }

  /** Read the specifed json file from the testdata directory as a String. */
  private Message loadMessage(String name, Builder builder) throws IOException {
    File file =
        new File(
            runfiles.rlocation(
                "com_google_fhir/testdata/stu3/examples/" + name + ".json"));
    String json = Files.asCharSource(file, StandardCharsets.UTF_8).read();
    Builder jsonBuilder = builder.clone();
    jsonParser.merge(json, jsonBuilder);
    return jsonBuilder.build();
  }

  /** Read the specifed json schema file from the testdata directory and parse it. */
  @SuppressWarnings("unchecked")
  private TableSchema readSchema(String filename) throws IOException {
    Runfiles runfiles = Runfiles.create();
    File file =
        new File(runfiles.rlocation("com_google_fhir/testdata/stu3/bigquery/" + filename));
    ArrayList<TableFieldSchema> fields =
        (ArrayList<TableFieldSchema>)
            gsonFactory.fromString(
                Files.asCharSource(file, StandardCharsets.UTF_8).read(), ArrayList.class);
    return new TableSchema().setFields(fields);
  }

  private void testSchema(String name, Builder builder) throws IOException {
    // Parse the json version of the input.
    Message input = loadMessage(name, builder);
    TableSchema schema = BigQuerySchema.fromMessage(input);

    // Parse the json-schema version of the input.
    TableSchema expected = readSchema(name + ".schema.json");

    assertThat(schema).isEqualTo(expected);
  }

  @Test
  public void testDateTime() throws Exception {
    DateTime input =
        DateTime.newBuilder()
            .setValueUs(31536000000000L)
            .setPrecision(DateTime.Precision.YEAR)
            .setTimezone("UTC")
            .build();
    TableSchema expected =
        new TableSchema()
            .setFields(
                ImmutableList.of(
                    new TableFieldSchema()
                        .setName("value_us")
                        .setType("TIMESTAMP")
                        .setMode("NULLABLE"),
                    new TableFieldSchema()
                        .setName("timezone")
                        .setType("STRING")
                        .setMode("NULLABLE"),
                    new TableFieldSchema()
                        .setName("precision")
                        .setType("STRING")
                        .setMode("NULLABLE")));
    TableSchema schema = BigQuerySchema.fromMessage(input);
    assertThat(schema).isEqualTo(expected);
  }

  @Test
  public void testEmptyBase64Binary() throws Exception {
    Base64Binary input = Base64Binary.newBuilder().build();
    TableSchema expected =
        new TableSchema()
            .setFields(
                ImmutableList.of(
                    new TableFieldSchema().setName("value").setType("BYTES").setMode("NULLABLE")));
    TableSchema schema = BigQuerySchema.fromMessage(input);
    assertThat(schema).isEqualTo(expected);
  }

  @Test
  public void testInteger() throws Exception {
    Integer input = Integer.newBuilder().setValue(3).build();
    TableSchema expected =
        new TableSchema()
            .setFields(
                ImmutableList.of(
                    new TableFieldSchema()
                        .setName("value")
                        .setType("INTEGER")
                        .setMode("NULLABLE")));
    TableSchema schema = BigQuerySchema.fromMessage(input);
    assertThat(schema).isEqualTo(expected);
  }

  @Test
  public void testPatient() throws Exception {
    testSchema("patient-example", Patient.newBuilder());
  }

  @Test
  public void testComposition() throws Exception {
    testSchema("composition-example", Composition.newBuilder());
  }
}
