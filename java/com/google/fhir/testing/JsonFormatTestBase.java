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

package com.google.fhir.testing;

import static com.google.common.truth.Truth.assertThat;
import static java.lang.Math.min;
import static java.nio.charset.StandardCharsets.UTF_8;
import static org.junit.Assert.fail;

import com.google.common.io.Files;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.common.JsonFormat;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.stream.JsonReader;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import java.io.File;
import java.io.IOException;
import java.io.StringReader;
import java.time.ZoneId;
import java.util.Map;
import java.util.TreeSet;

/** Unit tests for {@link JsonFormat}. */
public abstract class JsonFormatTestBase {
  protected JsonFormat.Parser jsonParser;
  protected JsonFormat.Printer jsonPrinter;
  protected JsonFormat.Printer ndjsonPrinter;
  protected TextFormat.Parser textParser;
  protected Runfiles runfiles;

  private final String versionName;
  private final String examplesDir;

  protected JsonFormatTestBase(String versionName, String examplesDir) {
    this.versionName = versionName;
    this.examplesDir = examplesDir;
  }

  /** Read the specifed json file from the testdata directory as a String. */
  protected String loadJson(String filename) throws IOException {
    File file = new File(runfiles.rlocation("com_google_fhir/" + filename));
    return Files.asCharSource(file, UTF_8).read();
  }

  /** Read the specifed prototxt file from the testdata directory and parse it. */
  protected void mergeText(String filename, Message.Builder builder) throws IOException {
    File file =
        new File(
            runfiles.rlocation(
                "com_google_fhir/testdata/" + versionName + "/" + filename));
    textParser.merge(Files.asCharSource(file, UTF_8).read(), builder);
  }

  protected void parseToProto(String name, Message.Builder builder)
      throws IOException, InvalidFhirException {
    String filename = "spec/" + examplesDir + "/package/" + name + ".json";

    jsonParser.merge(loadJson(filename), builder);
  }

  public void testPair(String name, Message.Builder builder)
      throws IOException, InvalidFhirException {
    try {
      // Load golden JSON
      String goldenJson = loadJson("spec/" + examplesDir + "/package/" + name + ".json");

      // Load golden proto
      Message.Builder goldenProto = builder.clone();
      mergeText("examples/" + name + ".prototxt", goldenProto);

      // Test Parser
      Message.Builder testProto = builder.clone();
      jsonParser.merge(goldenJson, testProto);
      if (!testProto.build().toString().equals(goldenProto.build().toString())) {
        System.err.println("Failed Parsing on: " + name);
        assertThat(testProto.build().toString()).isEqualTo(goldenProto.build().toString());
      }

      // Test printer
      boolean goldenIsNdJson = !goldenJson.contains("\n");
      String testJson = (goldenIsNdJson ? ndjsonPrinter : jsonPrinter).print(goldenProto);
      if (!testJson.equals(goldenJson)) {
        // They're not exactly equal - try testing canonical before failing.
        String canonicalGolden = canonicalizeJson(goldenJson);
        String canonicalTest = canonicalizeJson(testJson);
        if (!canonicalGolden.equals(canonicalTest)) {
          System.err.println("Failed Printing on: " + name);
          if (testJson.length() < 10000000) {
            assertThat(testJson).isEqualTo(goldenJson);
          } else {
            // Full output is too big to diff - try diffing in chunks
            for (int i = 0; i < min(goldenJson.length(), testJson.length()); i += 1000) {
              assertThat(testJson.substring(i, i + 1000))
                  .isEqualTo(goldenJson.substring(i, i + 1000));
            }
            // fall back to just printing everything.
            System.out.println("Expected:\n" + goldenJson);
            System.out.println("But was:\n " + testJson);
            fail();
          }
        }
      }
    } catch (Exception e) {
      System.out.println("Failed with Exception on " + name);
      throw e;
    }
  }

  protected void testParse(String name, Message.Builder builder)
      throws IOException, InvalidFhirException {
    // Parse the json version of the input.
    Message.Builder jsonBuilder = builder.clone();
    parseToProto(name, jsonBuilder);
    // Parse the proto text version of the input.
    Message.Builder textBuilder = builder.clone();
    mergeText("examples/" + name + ".prototxt", textBuilder);

    if (!jsonBuilder.build().toString().equals(textBuilder.build().toString())) {
      System.out.println("Failed Parsing on: " + name);
      assertThat(jsonBuilder.build().toString()).isEqualTo(textBuilder.build().toString());
    }
  }

  protected JsonElement canonicalize(JsonElement element) {
    if (element.isJsonObject()) {
      JsonObject object = element.getAsJsonObject();
      JsonObject sorted = new JsonObject();
      TreeSet<String> keys = new TreeSet<>();
      for (Map.Entry<String, JsonElement> entry : object.entrySet()) {
        keys.add(entry.getKey());
      }
      for (String key : keys) {
        sorted.add(key, canonicalize(object.get(key)));
      }
      return sorted;
    }
    if (element.isJsonArray()) {
      JsonArray sorted = new JsonArray();
      for (JsonElement e : element.getAsJsonArray()) {
        sorted.add(canonicalize(e));
      }
      return sorted;
    }
    return element;
  }

  protected String canonicalizeJson(String json) {
    JsonElement testJson =
        canonicalize(JsonParser.parseReader(new JsonReader(new StringReader(json))));
    return testJson.toString();
  }

  protected void testPrint(String name, Message.Builder builder)
      throws IOException, InvalidFhirException {
    // Parse the proto text version of the input.
    Message.Builder textBuilder = builder.clone();
    mergeText("examples/" + name + ".prototxt", textBuilder);
    // Load the json version of the input as a String.
    String jsonGolden = loadJson("spec/" + examplesDir + "/package/" + name + ".json");
    // Print the proto as json and compare.
    boolean goldenIsNdJson = !jsonGolden.contains("\n");
    String testJson = (goldenIsNdJson ? ndjsonPrinter : jsonPrinter).print(textBuilder);

    if (!testJson.equals(jsonGolden)) {
      System.out.println("Failed Printing on: " + name);
      assertThat(testJson).isEqualTo(jsonGolden);
    }
  }

  protected void testConvertForAnalytics(String name, Message.Builder builder)
      throws IOException, InvalidFhirException {
    // Parse the json version of the input.
    Message.Builder jsonBuilder = builder.clone();
    jsonParser.merge(loadJson("spec/" + examplesDir + "/package/" + name + ".json"), jsonBuilder);
    // Load the analytics version of the input as a String.
    String analyticsGolden = loadJson("testdata/" + versionName + "/bigquery/" + name + ".json");
    // Print and compare.
    String analyticsTest = jsonPrinter.forAnalytics().print(jsonBuilder);
    assertThat(analyticsTest.trim()).isEqualTo(analyticsGolden.trim());
  }

  public void setUpParser() throws IOException {
    jsonParser = JsonFormat.Parser.withDefaultTimeZone(ZoneId.of("Australia/Sydney"));
    jsonPrinter = JsonFormat.getPrinter();
    ndjsonPrinter = JsonFormat.getPrinter().omittingInsignificantWhitespace();
    textParser = TextFormat.getParser();
    runfiles = Runfiles.create();
  }
}
