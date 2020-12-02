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

package com.google.fhir.common;

import static com.google.common.truth.Truth.assertThat;

import com.google.api.client.json.gson.GsonFactory;
import com.google.api.services.bigquery.model.TableFieldSchema;
import com.google.api.services.bigquery.model.TableSchema;
import com.google.common.io.Files;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.stu3.proto.Composition;
import com.google.fhir.stu3.proto.Encounter;
import com.google.fhir.stu3.proto.Observation;
import com.google.fhir.stu3.proto.Patient;
import com.google.protobuf.Descriptors.Descriptor;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class BigQuerySchemaTest {

  private static final Boolean GENERATE_GOLDEN = false;
  private static final String GOLDEN_OUTPUT_DIRECTORY = "/tmp/schema";
  private GsonFactory gsonFactory;

  @Before
  public void setUp() throws IOException {
    gsonFactory = new GsonFactory();
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

  private void testSchema(Descriptor descriptor) throws IOException {
    TableSchema schema = BigQuerySchema.fromDescriptor(descriptor);
    String name = descriptor.getName();

    if (GENERATE_GOLDEN) {
      // Not actually testing, just generating test data.
      String filename = GOLDEN_OUTPUT_DIRECTORY + "/" + name + ".schema.json";
      System.out.println("Writing " + filename + "...");
      File file = new File(filename);
      Files.asCharSink(file, StandardCharsets.UTF_8)
          .write(gsonFactory.toPrettyString(schema.getFields()));
    } else {
      // Testing.
      // Parse the json-schema version of the input.
      TableSchema expected = readSchema(name + ".schema.json");
      assertThat(schema).isEqualTo(expected);
    }
  }

  @Test
  public void testPatient() throws Exception {
    testSchema(Patient.getDescriptor());
  }

  @Test
  public void testComposition() throws Exception {
    testSchema(Composition.getDescriptor());
  }

  @Test
  public void testObservation() throws Exception {
    testSchema(Observation.getDescriptor());
  }

  @Test
  public void testEncounter() throws Exception {
    testSchema(Encounter.getDescriptor());
  }
}
