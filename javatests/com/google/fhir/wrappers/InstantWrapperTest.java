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

import com.google.fhir.r4.core.Instant;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link InstantWrapper}. */
@RunWith(JUnit4.class)
public final class InstantWrapperTest {

  @Test
  public void parseInstant() {
    Instant parsed;
    Instant expected;

    parsed = new InstantWrapper("1970-01-01T00:00:00+00:00").getWrapped();
    expected =
        Instant.newBuilder()
            .setValueUs(0)
            .setPrecision(Instant.Precision.SECOND)
            .setTimezone("+00:00")
            .build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new InstantWrapper("1970-01-01T00:00:00Z").getWrapped();
    expected =
        Instant.newBuilder()
            .setValueUs(0)
            .setPrecision(Instant.Precision.SECOND)
            .setTimezone("Z")
            .build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new InstantWrapper("1970-01-01T00:00:00-00:00").getWrapped();
    expected =
        Instant.newBuilder()
            .setValueUs(0)
            .setPrecision(Instant.Precision.SECOND)
            .setTimezone("-00:00")
            .build();
    assertThat(parsed).isEqualTo(expected);
  }

  @Test
  public void printInstant() {
    Instant input;

    input =
        Instant.newBuilder()
            .setValueUs(0)
            .setPrecision(Instant.Precision.SECOND)
            .setTimezone("Z")
            .build();
    assertThat(new InstantWrapper(input).toString()).isEqualTo("1970-01-01T00:00:00Z");

    input =
        Instant.newBuilder()
            .setValueUs(0)
            .setPrecision(Instant.Precision.SECOND)
            .setTimezone("UTC")
            .build();
    assertThat(new InstantWrapper(input).toString()).isEqualTo("1970-01-01T00:00:00Z");

    input =
        Instant.newBuilder()
            .setValueUs(0)
            .setPrecision(Instant.Precision.SECOND)
            .setTimezone("+00:00")
            .build();
    assertThat(new InstantWrapper(input).toString()).isEqualTo("1970-01-01T00:00:00+00:00");

    input =
        Instant.newBuilder()
            .setValueUs(0)
            .setPrecision(Instant.Precision.SECOND)
            .setTimezone("-00:00")
            .build();
    assertThat(new InstantWrapper(input).toString()).isEqualTo("1970-01-01T00:00:00-00:00");

    input =
        Instant.newBuilder()
            .setValueUs(1412827080000000L)
            .setPrecision(Instant.Precision.SECOND)
            .setTimezone("Australia/Sydney")
            .build();
    assertThat(new InstantWrapper(input).toString()).isEqualTo("2014-10-09T14:58:00+11:00");
  }
}
