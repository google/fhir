//    Copyright 2021 Google Inc.
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

import com.google.protobuf.Descriptors.GenericDescriptor;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.Function;

/**
 * Memoization object keyed by Descriptors, allowing for a consistent caching strategy. Note that
 * the implementation actually caches on descriptor name, to avoid a memory leak if the JVM moves
 * around descriptors (causing new cache entries to get added). This templates on {@code
 * GenericDescriptor} type so as to preserve the most specific type information possible by the
 * compute function.
 */
// TODO: Try caching on descriptor object, with a size limit and "least recently used"
// eviction strategy.  This will avoid string comparisons.
public final class DescriptorMemosMap<D extends GenericDescriptor, T> {
  private final ConcurrentHashMap<String, T> memos = new ConcurrentHashMap<>();

  public T computeIfAbsent(D descriptor, Function<D, T> computeFunction) {
    return memos.computeIfAbsent(
        descriptor.getFullName(), fullName -> computeFunction.apply(descriptor));
  }
}
