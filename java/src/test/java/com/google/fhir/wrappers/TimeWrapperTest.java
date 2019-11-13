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

package com.google.fhir.wrappers;

import static com.google.common.truth.Truth.assertThat;

import com.google.fhir.r4.core.Time;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link TimeWrapper}. */
@RunWith(JUnit4.class)
public final class TimeWrapperTest {

  @Test
  public void parseTime() {
    Time parsed;
    Time expected;

    parsed = new TimeWrapper("12:00:00").getWrapped();
    expected =
        Time.newBuilder().setValueUs(43200000000L).setPrecision(Time.Precision.SECOND).build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new TimeWrapper("12:00:00.123").getWrapped();
    expected =
        Time.newBuilder().setValueUs(43200123000L).setPrecision(Time.Precision.MILLISECOND).build();
    assertThat(parsed).isEqualTo(expected);
  }

  @Test
  public void printTime() {
    Time input;

    input = Time.newBuilder().setValueUs(43200000000L).setPrecision(Time.Precision.SECOND).build();
    assertThat(new TimeWrapper(input).toString()).isEqualTo("12:00:00");
    input =
        Time.newBuilder().setValueUs(43200123000L).setPrecision(Time.Precision.MILLISECOND).build();
    assertThat(new TimeWrapper(input).toString()).isEqualTo("12:00:00.123");
  }
}
