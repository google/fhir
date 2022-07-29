//    Copyright 2022 Google LLC
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
import static org.junit.Assert.assertThrows;

import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.Uri;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import com.google.testing.junit.testparameterinjector.TestParameter;
import com.google.testing.junit.testparameterinjector.TestParameterInjector;
import java.util.NoSuchElementException;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(TestParameterInjector.class)
public final class ResourceCollectionTest {

  private static JsonElement buildJsonResource(String url) {
    JsonObject jsonObject = new JsonObject();
    jsonObject.add("url", new JsonPrimitive(url));
    return jsonObject;
  }

  private enum InvalidJsonToAddTestCase {
    NOT_JSON_OBJECT(new JsonArray()),
    NO_URL_FIELD(new JsonObject()),
    EMPTY_URL(buildJsonResource(""));

    final JsonElement json;

    InvalidJsonToAddTestCase(JsonElement json) {
      this.json = json;
    }
  }

  @Test
  public void addInvalidJsonObject_throws(@TestParameter InvalidJsonToAddTestCase testCase) {
    ResourceCollection<StructureDefinition> r =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    IllegalArgumentException thrown =
        assertThrows(IllegalArgumentException.class, () -> r.add(testCase.json));

    assertThat(thrown)
        .hasMessageThat()
        .isEqualTo("Must have a non-empty `url` to add resource to collection.");
  }

  @Test
  public void addDuplicateUrlInUnparsedResources_throws() {
    JsonElement json = buildJsonResource("http://foo.com");
    ResourceCollection<StructureDefinition> r =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    r.add(json);

    IllegalArgumentException thrown =
        assertThrows(IllegalArgumentException.class, () -> r.add(json));

    assertThat(thrown)
        .hasMessageThat()
        .isEqualTo("Resource collection already contains a resource with `url`: http://foo.com");
  }

  @Test
  public void addDuplicateUrlInParsedResources_throws() throws InvalidFhirException {
    JsonElement json = buildJsonResource("http://foo.com");
    ResourceCollection<StructureDefinition> r =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    r.add(json);
    // Retrieve the resource, moving it to the parsed collection.
    r.get("http://foo.com");

    IllegalArgumentException thrown =
        assertThrows(IllegalArgumentException.class, () -> r.add(json));

    assertThat(thrown)
        .hasMessageThat()
        .isEqualTo("Resource collection already contains a resource with `url`: http://foo.com");
  }

  @Test
  public void addThenGetResource_succeeds() throws InvalidFhirException {
    JsonElement json = buildJsonResource("http://foo.com");
    ResourceCollection<StructureDefinition> r =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    r.add(json);

    StructureDefinition expected =
        StructureDefinition.newBuilder()
            .setUrl(Uri.newBuilder().setValue("http://foo.com"))
            .build();
    assertThat(r.get("http://foo.com")).isEqualTo(expected);
    // `get` it twice since on the second invocation, the `ResourceCollection` is reading from its
    // parsed resources.
    assertThat(r.get("http://foo.com")).isEqualTo(expected);
  }

  @Test
  public void getNonExistantResource_throws() {
    ResourceCollection<StructureDefinition> r =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    NoSuchElementException thrown =
        assertThrows(NoSuchElementException.class, () -> r.get("http://foo.com"));

    assertThat(thrown).hasMessageThat().isEqualTo("No resource found for URI: http://foo.com");
  }

  @Test
  public void getUnparsableResource_throws() {
    JsonObject jsonObject = new JsonObject();
    jsonObject.add("url", new JsonPrimitive("http://foo.com"));
    jsonObject.add("notASupportedField", new JsonPrimitive("bar"));
    ResourceCollection<StructureDefinition> r =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    r.add(jsonObject);

    InvalidFhirException thrown =
        assertThrows(InvalidFhirException.class, () -> r.get("http://foo.com"));

    assertThat(thrown).hasMessageThat().startsWith("Unknown field notASupportedField");
  }

  @Test
  public void iteratesEmptyCollection_succeeds() {
    ResourceCollection<StructureDefinition> r =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    assertThat(r).isEmpty();
    assertThrows(NoSuchElementException.class, () -> r.iterator().next());
  }

  @Test
  public void iteratesMixedCollectionOfParsedAndUnparsed_succeedsUntilDone()
      throws InvalidFhirException {
    JsonElement json0 = buildJsonResource("http://foo.com");
    JsonElement json1 = buildJsonResource("http://bar.com");
    ResourceCollection<StructureDefinition> r =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    r.add(json0);
    r.add(json1);
    // Create a mix of parsed and unparsed resources by retrieving them for the first time.
    r.get("http://foo.com");

    assertThat(r).hasSize(2);
  }
}
