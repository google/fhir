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

import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.common.JsonFormat;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.protobuf.Internal;
import com.google.protobuf.Message;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.function.Predicate;

/**
 * A collection of FHIR resources of a given type, T.
 *
 * <p>Resources in their JSON format can be put in the collection. They are only ever parsed when
 * retrieved (either by `get` or through iterating the collection).
 *
 * <p>TODO(b/240605161): Consider aligning approaches to loading FHIR packages from the archive
 * file.
 */
class ResourceCollection<T extends Message> implements Iterable<T> {

  private final JsonFormat.Parser jsonParser;
  private final Map<String, JsonElement> jsonResourcesByUri = new HashMap<>();
  private final Map<String, T> parsedResourcesByUri = new HashMap<>();
  private final Class<T> protoClass;
  private Predicate<T> filter = null;

  ResourceCollection(JsonFormat.Parser jsonParser, Class<T> protoClass) {
    this.jsonParser = jsonParser;
    this.protoClass = protoClass;
  }

  /**
   * Sets a filter for the collection.
   *
   * <p>Only resources matching the filter will be returned by iterators or the `get` method.
   *
   * <p>Invalidates iterators.
   */
  public void setFilter(Predicate<T> filter) {
    this.filter = filter;
  }

  @Override
  public Iterator<T> iterator() {
    return new Iterator<T>() {
      private Optional<T> next = Optional.empty();
      private final Iterator<Map.Entry<String, T>> parsedIterator =
          parsedResourcesByUri.entrySet().iterator();

      @Override
      public boolean hasNext() {
        if (next.isPresent()) {
          return true;
        }
        Optional<T> unfilteredNext = Optional.empty();
        // First iterate the parsed resources, then the unparsed ones.
        if (parsedIterator.hasNext()) {
          unfilteredNext = Optional.of(parsedIterator.next().getValue());
        } else if (!jsonResourcesByUri.isEmpty()) {
          // Calling `get` parses the resource, removing it from the `jsonResourceByUri`
          // collection. We can simply read from the front of that entry set each iteration.
          Map.Entry<String, JsonElement> unparsed = jsonResourcesByUri.entrySet().iterator().next();

          unfilteredNext = get(unparsed.getKey());
          if (!unfilteredNext.isPresent()) {
            return hasNext();
          }
        } else {
          return false;
        }

        if (unfilteredNext != null && (filter == null || filter.test(unfilteredNext.get()))) {
          next = unfilteredNext;
          return true;
        }

        // The filter was set and the next unfiltered resource didn't match it.
        return hasNext();
      }

      @Override
      public T next() {
        if (!hasNext()) {
          throw new NoSuchElementException();
        }
        T next = this.next.get();
        this.next = Optional.empty();
        return next;
      }
    };
  }

  /**
   * Associates the provided resource as `json` with this collection, using the `url` field as the
   * unique identifier. If the collection previously registered a resource using the `url` in the
   * `json`, the old resource is replaced by the specified value.
   *
   * <p>Invalidates iterators.
   */
  public void put(JsonElement json) {
    JsonObject jsonObject;
    String url;
    if (!json.isJsonObject()
        || !(jsonObject = json.getAsJsonObject()).has("url")
        || (url = jsonObject.get("url").getAsString()).isEmpty()) {
      throw new IllegalArgumentException(
          "Must have a non-empty `url` to put the resource in the collection.");
    }
    // Remove the parsed resource if it exists since `put` should override.
    if (parsedResourcesByUri.containsKey(url)) {
      parsedResourcesByUri.remove(url);
    }

    // If the map previously contained a mapping for the key, the old value is replaced by the
    // specified value.
    // A copy of the `json` is made to ensure mutations don't affect underlying storage.
    jsonResourcesByUri.put(url, json.deepCopy());
  }

  /**
   * Returns the resource to which the specified `uri` is registered with, or an empty `Optional` if
   * this collection does not contain a resource for the `uri` or if the resource is invalid.
   *
   * <p>If a filter is set, the retrieved resource will only be returned if it matches.
   */
  @SuppressWarnings("unchecked") // newBuilderForType().build() produces a T.
  public Optional<T> get(String uri) {
    T resource = parsedResourcesByUri.get(uri);
    if (resource == null) {
      JsonElement jsonResource = jsonResourcesByUri.get(uri);
      if (jsonResource == null) {
        return Optional.empty();
      }

      Message.Builder builder = Internal.getDefaultInstance(protoClass).newBuilderForType();

      try {
        resource = (T) jsonParser.merge(jsonResource.toString(), builder).build();
        parsedResourcesByUri.put(uri, resource);
      } catch (InvalidFhirException e) {
        System.out.println(
            "Warning - Skipping " + uri + ", which failed to parse:\n" + e.getMessage());
      }

      // Now that the resource is parsed, remove it from the unparsed JSON resources.
      jsonResourcesByUri.remove(uri);
    }

    if (resource == null || (filter != null && !filter.test(resource))) {
      return Optional.empty();
    }

    return Optional.of(resource);
  }
}
