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
  public void add_invalidJsonObject_throws(@TestParameter InvalidJsonToAddTestCase testCase) {
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    IllegalArgumentException thrown =
        assertThrows(IllegalArgumentException.class, () -> resourceCollection.add(testCase.json));

    assertThat(thrown)
        .hasMessageThat()
        .isEqualTo("Must have a non-empty `url` to add resource to collection.");
  }

  @Test
  public void add_duplicateUrlInUnparsedResources_throws() {
    JsonElement json = buildJsonResource("http://foo.com");
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    resourceCollection.add(json);

    IllegalArgumentException thrown =
        assertThrows(IllegalArgumentException.class, () -> resourceCollection.add(json));

    assertThat(thrown)
        .hasMessageThat()
        .isEqualTo("Resource collection already contains a resource with `url`: http://foo.com");
  }

  @Test
  public void add_duplicateUrlInParsedResources_throws() throws Exception {
    JsonElement json = buildJsonResource("http://foo.com");
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    resourceCollection.add(json);
    // Retrieve the resource, moving it to the parsed collection.
    resourceCollection.get("http://foo.com");

    IllegalArgumentException thrown =
        assertThrows(IllegalArgumentException.class, () -> resourceCollection.add(json));

    assertThat(thrown)
        .hasMessageThat()
        .isEqualTo("Resource collection already contains a resource with `url`: http://foo.com");
  }

  @Test
  public void add_thenGetResource_succeeds() throws Exception {
    JsonElement json = buildJsonResource("http://foo.com");
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    resourceCollection.add(json);

    StructureDefinition expected =
        StructureDefinition.newBuilder()
            .setUrl(Uri.newBuilder().setValue("http://foo.com"))
            .build();
    assertThat(resourceCollection.get("http://foo.com")).isEqualTo(expected);
    // `get` it twice since on the second invocation, the `ResourceCollection` is reading from its
    // parsed resources.
    assertThat(resourceCollection.get("http://foo.com")).isEqualTo(expected);
  }

  @Test
  public void get_nonExistantResource_throws() {
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    NoSuchElementException thrown =
        assertThrows(NoSuchElementException.class, () -> resourceCollection.get("http://foo.com"));

    assertThat(thrown).hasMessageThat().isEqualTo("No resource found for URI: http://foo.com");
  }

  @Test
  public void get_unparsableResource_throws() {
    JsonObject jsonObject = new JsonObject();
    jsonObject.add("url", new JsonPrimitive("http://foo.com"));
    jsonObject.add("notASupportedField", new JsonPrimitive("bar"));
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    resourceCollection.add(jsonObject);

    InvalidFhirException thrown =
        assertThrows(InvalidFhirException.class, () -> resourceCollection.get("http://foo.com"));

    assertThat(thrown).hasMessageThat().startsWith("Unknown field notASupportedField");
  }

  @Test
  public void iterator_emptyCollection_succeeds() {
    ResourceCollection<StructureDefinition> r =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    assertThat(r).isEmpty();
    assertThrows(NoSuchElementException.class, () -> r.iterator().next());
  }

  @Test
  public void iterator_mixedCollectionOfParsedAndUnparsed_succeedsUntilDone() throws Exception {
    JsonElement json0 = buildJsonResource("http://foo.com");
    JsonElement json1 = buildJsonResource("http://bar.com");
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    resourceCollection.add(json0);
    resourceCollection.add(json1);
    // Create a mix of parsed and unparsed resources by retrieving them for the first time.
    resourceCollection.get("http://foo.com");

    assertThat(resourceCollection).hasSize(2);
  }

  @Test
  public void filter_nullPredicate_noop() throws Exception {
    JsonElement json = buildJsonResource("http://foo.com");
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    resourceCollection.add(json);
    assertThat(resourceCollection).hasSize(1);

    resourceCollection.setFilter(/* filter= */ null);

    assertThat(resourceCollection).hasSize(1);
    assertThat(resourceCollection.get("http://foo.com"))
        .isEqualTo(
            StructureDefinition.newBuilder()
                .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                .build());
  }

  @Test
  public void filter_alwaysMatchingPredicate_noop() throws Exception {
    JsonElement json = buildJsonResource("http://foo.com");
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    resourceCollection.add(json);
    assertThat(resourceCollection).hasSize(1);

    resourceCollection.setFilter(s -> true);

    assertThat(resourceCollection).hasSize(1);
    assertThat(resourceCollection.get("http://foo.com"))
        .isEqualTo(
            StructureDefinition.newBuilder()
                .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                .build());
  }

  @Test
  public void filter_matchingPredicate_filtersNonMatchingResources() throws Exception {
    JsonElement json0 = buildJsonResource("http://foo.com");
    JsonElement json1 = buildJsonResource("http://bar.com");
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    resourceCollection.add(json0);
    resourceCollection.add(json1);

    resourceCollection.setFilter(s -> s.getUrl().getValue().equals("http://bar.com"));

    assertThat(resourceCollection).hasSize(1);
    // Issue the `hasSize` assertion a second time which will call for the `iterator` and iterate
    // over already-parsed resources which read the resources from cache.
    assertThat(resourceCollection).hasSize(1);
    assertThat(resourceCollection.get("http://bar.com"))
        .isEqualTo(
            StructureDefinition.newBuilder()
                .setUrl(Uri.newBuilder().setValue("http://bar.com"))
                .build());
    assertThat(
            assertThrows(
                NoSuchElementException.class, () -> resourceCollection.get("http://foo.com")))
        .hasMessageThat()
        .isEqualTo(
            "No resource found for URI: 'http://foo.com'. Does the resource match the set filter?");
  }
}
