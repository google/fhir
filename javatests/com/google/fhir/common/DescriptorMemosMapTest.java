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

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.google.protobuf.Descriptors.Descriptor;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class DescriptorMemosMapTest {

  private static class MutableInt {
    private int value = 0;

    private void increment() {
      value++;
    }
  }

  @Test
  public void testComputeIfAbsent() {
    DescriptorMemosMap<Descriptor, String> memos = new DescriptorMemosMap<>();

    MutableInt r4RunCount = new MutableInt();
    MutableInt stu3RunCount = new MutableInt();

    for (int i = 0; i < 5; i++) {
      assertThat(
              memos.computeIfAbsent(
                  com.google.fhir.r4.core.String.getDescriptor(),
                  param -> {
                    r4RunCount.increment();
                    return "r4";
                  }))
          .isEqualTo("r4");
      assertThat(
              memos.computeIfAbsent(
                  com.google.fhir.stu3.proto.String.getDescriptor(),
                  param -> {
                    stu3RunCount.increment();
                    return "stu3";
                  }))
          .isEqualTo("stu3");

      assertThat(r4RunCount.value).isEqualTo(1);
      assertThat(stu3RunCount.value).isEqualTo(1);
    }
  }

  @Test
  public void testComputeOrThrowIfAbsent_noException() throws Exception {
    DescriptorMemosMap<Descriptor, String> memos = new DescriptorMemosMap<>();

    MutableInt r4RunCount = new MutableInt();
    MutableInt stu3RunCount = new MutableInt();

    for (int i = 0; i < 5; i++) {
      assertThat(
              memos.computeOrThrowIfAbsent(
                  com.google.fhir.r4.core.String.getDescriptor(),
                  param -> {
                    r4RunCount.increment();
                    return "r4";
                  }))
          .isEqualTo("r4");
      assertThat(
              memos.computeOrThrowIfAbsent(
                  com.google.fhir.stu3.proto.String.getDescriptor(),
                  param -> {
                    stu3RunCount.increment();
                    return "stu3";
                  }))
          .isEqualTo("stu3");

      assertThat(r4RunCount.value).isEqualTo(1);
      assertThat(stu3RunCount.value).isEqualTo(1);
    }
  }

  @Test
  public void testComputeOrThrowIfAbsent_throwsException() throws Exception {
    DescriptorMemosMap<Descriptor, String> memos = new DescriptorMemosMap<>();

    assertThrows(
        InvalidFhirException.class,
        () ->
            memos.computeOrThrowIfAbsent(
                com.google.fhir.r4.core.String.getDescriptor(),
                param -> {
                  throw new InvalidFhirException("failure");
                }));
    // Test twice with the same descriptor to ensure nothing is being improperly memoized.
    assertThrows(
        IllegalStateException.class,
        () ->
            memos.computeOrThrowIfAbsent(
                com.google.fhir.r4.core.String.getDescriptor(),
                param -> {
                  throw new IllegalStateException("failure");
                }));
  }
}
