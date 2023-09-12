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

import com.google.fhir.common.JsonFormat;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.Uri;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import com.google.testing.junit.testparameterinjector.TestParameter;
import com.google.testing.junit.testparameterinjector.TestParameterInjector;
import java.util.HashSet;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.Set;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(TestParameterInjector.class)
public final class ResourceCollectionTest {

  private static JsonElement buildJsonResource(String url) {
    JsonObject jsonObject = new JsonObject();
    jsonObject.add("url", new JsonPrimitive(url));
    return jsonObject;
  }

  private enum InvalidJsonToPutTestCase {
    NOT_JSON_OBJECT(new JsonArray()),
    NO_URL_FIELD(new JsonObject()),
    EMPTY_URL(buildJsonResource(""));

    final JsonElement json;

    InvalidJsonToPutTestCase(JsonElement json) {
      this.json = json;
    }
  }

  @Test
  public void put_invalidJsonObject_throws(@TestParameter InvalidJsonToPutTestCase testCase) {
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    IllegalArgumentException thrown =
        assertThrows(IllegalArgumentException.class, () -> resourceCollection.put(testCase.json));

    assertThat(thrown)
        .hasMessageThat()
        .isEqualTo("Must have a non-empty `url` to put the resource in the collection.");
  }

  @Test
  public void put_duplicateUrlInUnparsedResources_replaces() throws Exception {
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    JsonObject json = new JsonObject();
    json.add("url", new JsonPrimitive("http://foo.com"));
    json.add("title", new JsonPrimitive("Before"));
    resourceCollection.put(json);

    json.remove("title");
    json.add("title", new JsonPrimitive("After"));
    resourceCollection.put(json);

    assertThat(resourceCollection.get("http://foo.com"))
        .isEqualTo(
            Optional.of(
                StructureDefinition.newBuilder()
                    .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                    .setTitle(com.google.fhir.r4.core.String.newBuilder().setValue("After"))
                    .build()));
  }

  @Test
  public void put_duplicateUrlInParsedResources_replaces() throws Exception {
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    JsonObject json = new JsonObject();
    json.add("url", new JsonPrimitive("http://foo.com"));
    json.add("title", new JsonPrimitive("Before"));
    resourceCollection.put(json);
    // Retrieve the resource, moving it to the parsed collection.
    assertThat(resourceCollection.get("http://foo.com"))
        .isEqualTo(
            Optional.of(
                StructureDefinition.newBuilder()
                    .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                    .setTitle(com.google.fhir.r4.core.String.newBuilder().setValue("Before"))
                    .build()));

    json.remove("title");
    json.add("title", new JsonPrimitive("After"));
    resourceCollection.put(json);

    assertThat(resourceCollection.get("http://foo.com"))
        .isEqualTo(
            Optional.of(
                StructureDefinition.newBuilder()
                    .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                    .setTitle(com.google.fhir.r4.core.String.newBuilder().setValue("After"))
                    .build()));
  }

  @Test
  public void put_thenGetResource_succeeds() throws Exception {
    JsonElement json = buildJsonResource("http://foo.com");
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    resourceCollection.put(json);

    Optional<StructureDefinition> expected =
        Optional.of(
            StructureDefinition.newBuilder()
                .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                .build());
    assertThat(resourceCollection.get("http://foo.com")).isEqualTo(expected);
    // `get` it twice since on the second invocation, the `ResourceCollection` is reading from its
    // parsed resources.
    assertThat(resourceCollection.get("http://foo.com")).isEqualTo(expected);
  }

  @Test
  public void put_thenGetResource_persistsCopies() throws Exception {
    JsonElement json = buildJsonResource("http://foo.com");
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    resourceCollection.put(json);
    // Modify the `json` so we can assert the `ResourceCollection` took a copy.
    json.getAsJsonObject().remove("url");
    json.getAsJsonObject().add("bar", new JsonPrimitive("baz"));

    Optional<StructureDefinition> expected =
        Optional.of(
            StructureDefinition.newBuilder()
                .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                .build());
    assertThat(resourceCollection.get("http://foo.com")).isEqualTo(expected);
  }

  @Test
  public void get_nonExistantResource_returnsEmpty() throws Exception {
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    assertThat(resourceCollection.get("http://foo.com")).isEqualTo(Optional.empty());
  }

  @Test
  public void get_unparsableResource_returnsEmpty() {
    JsonObject jsonObject = new JsonObject();
    jsonObject.add("url", new JsonPrimitive("http://foo.com"));
    jsonObject.add("notASupportedField", new JsonPrimitive("bar"));
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    resourceCollection.put(jsonObject);

    assertThat(resourceCollection.get("http://foo.com")).isEqualTo(Optional.empty());
  }

  @Test
  public void iterator_emptyCollection_isEmptyAndCannotIterate() {
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);

    assertThat(resourceCollection).isEmpty();
    assertThrows(NoSuchElementException.class, () -> resourceCollection.iterator().next());
  }

  @Test
  public void iterator_skipsUnparseableResources() throws Exception {
    JsonElement jsonValid0 = buildJsonResource("http://foo.com");

    JsonObject jsonInvalid1 = new JsonObject();
    jsonInvalid1.add("url", new JsonPrimitive("http://bar.com"));
    jsonInvalid1.add("notASupportedField", new JsonPrimitive("bar"));

    JsonElement jsonValid2 = buildJsonResource("http://baz.com");

    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    resourceCollection.put(jsonValid0);
    resourceCollection.put(jsonInvalid1);
    resourceCollection.put(jsonValid2);

    assertThat(resourceCollection).hasSize(2);

    {
      // Test with all json unparsed
      Set<String> resultUrls = new HashSet<>();
      var it = resourceCollection.iterator();
      resultUrls.add(it.next().getUrl().getValue());
      resultUrls.add(it.next().getUrl().getValue());
      assertThat(it.hasNext()).isFalse();
      assertThat(resultUrls).containsExactly("http://foo.com", "http://baz.com");
    }

    {
      // Repeat tests, now that resources are parsed.
      Set<String> resultUrls = new HashSet<>();
      var it = resourceCollection.iterator();
      resultUrls.add(it.next().getUrl().getValue());
      resultUrls.add(it.next().getUrl().getValue());
      assertThat(it.hasNext()).isFalse();
      assertThat(resultUrls).containsExactly("http://foo.com", "http://baz.com");
    }
  }

  @Test
  public void iterator_mixedCollectionOfParsedAndUnparsed_succeedsUntilDone() throws Exception {
    JsonElement json0 = buildJsonResource("http://foo.com");
    JsonElement json1 = buildJsonResource("http://bar.com");
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    resourceCollection.put(json0);
    resourceCollection.put(json1);
    // Create a mix of parsed and unparsed resources by retrieving them for the first time.
    resourceCollection.get("http://foo.com");

    assertThat(resourceCollection).hasSize(2);
  }

  @Test
  public void filter_nullPredicate_noop() throws Exception {
    JsonElement json = buildJsonResource("http://foo.com");
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    resourceCollection.put(json);
    assertThat(resourceCollection).hasSize(1);

    resourceCollection.setFilter(/* filter= */ null);

    assertThat(resourceCollection).hasSize(1);
    assertThat(resourceCollection.get("http://foo.com"))
        .isEqualTo(
            Optional.of(
                StructureDefinition.newBuilder()
                    .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                    .build()));
  }

  @Test
  public void filter_alwaysMatchingPredicate_noop() throws Exception {
    JsonElement json = buildJsonResource("http://foo.com");
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    resourceCollection.put(json);
    assertThat(resourceCollection).hasSize(1);

    resourceCollection.setFilter(s -> true);

    assertThat(resourceCollection).hasSize(1);
    assertThat(resourceCollection.get("http://foo.com"))
        .isEqualTo(
            Optional.of(
                StructureDefinition.newBuilder()
                    .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                    .build()));
  }

  @Test
  public void filter_matchingPredicate_filtersNonMatchingResources() throws Exception {
    JsonElement json0 = buildJsonResource("http://foo.com");
    JsonElement json1 = buildJsonResource("http://bar.com");
    ResourceCollection<StructureDefinition> resourceCollection =
        new ResourceCollection<>(JsonFormat.getParser(), StructureDefinition.class);
    resourceCollection.put(json0);
    resourceCollection.put(json1);

    resourceCollection.setFilter(s -> s.getUrl().getValue().equals("http://bar.com"));

    assertThat(resourceCollection).hasSize(1);
    // Issue the `hasSize` assertion a second time which will call for the `iterator` and iterate
    // over already-parsed resources which read the resources from cache.
    assertThat(resourceCollection).hasSize(1);
    assertThat(resourceCollection.get("http://bar.com"))
        .isEqualTo(
            Optional.of(
                StructureDefinition.newBuilder()
                    .setUrl(Uri.newBuilder().setValue("http://bar.com"))
                    .build()));
    assertThat(resourceCollection.get("http://foo.com")).isEqualTo(Optional.empty());
  }
}
