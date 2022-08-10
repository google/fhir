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
 * <p>Resources in their JSON format can be added to the collection. They are only ever parsed when
 * retrieved (either by `get` or through iterating the collection).
 *
 * <p>TODO: Consider aligning approaches to loading FHIR packages from the archive
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
        T unfilteredNext = null;
        // First iterate the parsed resources, then the unparsed ones.
        if (parsedIterator.hasNext()) {
          unfilteredNext = parsedIterator.next().getValue();
        } else if (!jsonResourcesByUri.isEmpty()) {
          // Calling `get` parses the resource, removing it from the `jsonResourceByUri`
          // collection. We can simply read from the front of that entry set each iteration.
          Map.Entry<String, JsonElement> unparsed = jsonResourcesByUri.entrySet().iterator().next();
          try {
            unfilteredNext = get(unparsed.getKey());
          } catch (NoSuchElementException e) {
            // While the resource exists, it does not match the filter.
            return hasNext();
          } catch (InvalidFhirException e) {
            throw new IllegalStateException(e);
          }
        } else {
          return false;
        }

        if (filter == null || filter.test(unfilteredNext)) {
          next = Optional.ofNullable(unfilteredNext);
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
   * Adds a resource to the collection.
   *
   * <p>Invalidates iterators.
   */
  public void add(JsonElement json) {
    JsonObject jsonObject;
    String url;
    if (!json.isJsonObject()
        || !(jsonObject = json.getAsJsonObject()).has("url")
        || (url = jsonObject.get("url").getAsString()).isEmpty()) {
      throw new IllegalArgumentException(
          "Must have a non-empty `url` to add resource to collection.");
    }
    if (jsonResourcesByUri.containsKey(url) || parsedResourcesByUri.containsKey(url)) {
      throw new IllegalArgumentException(
          "Resource collection already contains a resource with `url`: " + url);
    }

    jsonResourcesByUri.put(url, json);
  }

  /**
   * Returns the resource matching the URI.
   *
   * <p>If a filter is set, the retrieved resource will only be returned if it matches.
   */
  @SuppressWarnings("unchecked") // newBuilderForType().build() produces a T.
  public T get(String uri) throws InvalidFhirException {
    T resource = parsedResourcesByUri.get(uri);
    if (resource == null) {
      JsonElement jsonResource = jsonResourcesByUri.get(uri);
      if (jsonResource == null) {
        throw new NoSuchElementException("No resource found for URI: " + uri);
      }

      Message.Builder builder = Internal.getDefaultInstance(protoClass).newBuilderForType();
      resource = (T) jsonParser.merge(jsonResource.toString(), builder).build();
      parsedResourcesByUri.put(uri, resource);

      // Now that the resource is parsed, remove it from the unparsed JSON resources.
      jsonResourcesByUri.remove(uri);
    }

    if (filter != null && !filter.test(resource)) {
      throw new NoSuchElementException(
          "No resource found for URI: '" + uri + "'. Does the resource match the set filter?");
    }

    return resource;
  }
}
