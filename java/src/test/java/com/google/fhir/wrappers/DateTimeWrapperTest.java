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

import com.google.fhir.r4.core.DateTime;
import java.time.ZoneId;
import java.time.ZoneOffset;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link DateTimeWrapper}. */
@RunWith(JUnit4.class)
public final class DateTimeWrapperTest {

  @Test
  public void parseYear() {
    DateTime parsed = new DateTimeWrapper("1971", ZoneOffset.UTC).getWrapped();
    DateTime expected =
        DateTime.newBuilder()
            .setValueUs(31536000000000L)
            .setPrecision(DateTime.Precision.YEAR)
            .setTimezone("Z")
            .build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new DateTimeWrapper("1971", ZoneId.of("Australia/Sydney")).getWrapped();
    expected =
        DateTime.newBuilder()
            .setValueUs(31500000000000L)
            .setPrecision(DateTime.Precision.YEAR)
            .setTimezone("Australia/Sydney")
            .build();
    assertThat(parsed).isEqualTo(expected);
  }

  @Test
  public void parseYearMonth() {
    DateTime parsed = new DateTimeWrapper("1970-02", ZoneOffset.UTC).getWrapped();
    DateTime expected =
        DateTime.newBuilder()
            .setValueUs(2678400000000L)
            .setPrecision(DateTime.Precision.MONTH)
            .setTimezone("Z")
            .build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new DateTimeWrapper("1970-02", ZoneId.of("Australia/Sydney")).getWrapped();
    expected =
        DateTime.newBuilder()
            .setValueUs(2642400000000L)
            .setPrecision(DateTime.Precision.MONTH)
            .setTimezone("Australia/Sydney")
            .build();
    assertThat(parsed).isEqualTo(expected);
  }

  @Test
  public void parseYearMonthDay() {
    DateTime parsed = new DateTimeWrapper("1970-01-01", ZoneOffset.UTC).getWrapped();
    DateTime expected =
        DateTime.newBuilder()
            .setValueUs(0)
            .setPrecision(DateTime.Precision.DAY)
            .setTimezone("Z")
            .build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new DateTimeWrapper("1970-01-01", ZoneId.of("Australia/Sydney")).getWrapped();
    expected =
        DateTime.newBuilder()
            .setValueUs(-36000000000L)
            .setPrecision(DateTime.Precision.DAY)
            .setTimezone("Australia/Sydney")
            .build();
    assertThat(parsed).isEqualTo(expected);
  }

  @Test
  public void parseDateTime() {
    DateTime parsed =
        new DateTimeWrapper("1970-01-01T12:00:00.123Z", ZoneId.of("Australia/Sydney")).getWrapped();
    DateTime expected =
        DateTime.newBuilder()
            .setValueUs(43200123000L)
            .setPrecision(DateTime.Precision.MILLISECOND)
            .setTimezone("Z")
            .build();
    assertThat(parsed).isEqualTo(expected);

    parsed =
        new DateTimeWrapper("1970-01-01T12:00:00.123+00:00", ZoneId.of("Australia/Sydney"))
            .getWrapped();
    expected =
        DateTime.newBuilder()
            .setValueUs(43200123000L)
            .setPrecision(DateTime.Precision.MILLISECOND)
            .setTimezone("+00:00")
            .build();
    assertThat(parsed).isEqualTo(expected);

    parsed =
        new DateTimeWrapper("1970-01-01T12:00:00.123-00:00", ZoneId.of("Australia/Sydney"))
            .getWrapped();
    expected =
        DateTime.newBuilder()
            .setValueUs(43200123000L)
            .setPrecision(DateTime.Precision.MILLISECOND)
            .setTimezone("-00:00")
            .build();
    assertThat(parsed).isEqualTo(expected);

    parsed =
        new DateTimeWrapper("2014-10-09T14:58:00+11:00", ZoneId.of("Australia/Sydney"))
            .getWrapped();
    expected =
        DateTime.newBuilder()
            .setValueUs(1412827080000000L)
            .setPrecision(DateTime.Precision.SECOND)
            .setTimezone("+11:00")
            .build();
    assertThat(parsed).isEqualTo(expected);
  }

  @Test
  public void printYear() {
    DateTime input =
        DateTime.newBuilder()
            .setValueUs(0)
            .setPrecision(DateTime.Precision.YEAR)
            .setTimezone("Z")
            .build();
    assertThat(new DateTimeWrapper(input).toString()).isEqualTo("1970");
    assertThat(new DateTimeWrapper(input, ZoneId.of("Australia/Sydney")).toString())
        .isEqualTo("1970");

    input = input.toBuilder().setValueUs(-36000000000L).setTimezone("Australia/Sydney").build();
    assertThat(new DateTimeWrapper(input).toString()).isEqualTo("1970");
    assertThat(new DateTimeWrapper(input, ZoneId.of("Australia/Sydney")).toString())
        .isEqualTo("1970");
  }

  @Test
  public void printYearMonth() {
    DateTime input =
        DateTime.newBuilder()
            .setValueUs(0)
            .setPrecision(DateTime.Precision.MONTH)
            .setTimezone("Z")
            .build();
    assertThat(new DateTimeWrapper(input).toString()).isEqualTo("1970-01");
    assertThat(new DateTimeWrapper(input, ZoneId.of("Australia/Sydney")).toString())
        .isEqualTo("1970-01");

    input = input.toBuilder().setValueUs(-36000000000L).setTimezone("Australia/Sydney").build();
    assertThat(new DateTimeWrapper(input).toString()).isEqualTo("1970-01");
    assertThat(new DateTimeWrapper(input, ZoneId.of("Australia/Sydney")).toString())
        .isEqualTo("1970-01");
  }

  @Test
  public void printYearMonthDay() {
    DateTime input =
        DateTime.newBuilder()
            .setValueUs(0)
            .setPrecision(DateTime.Precision.DAY)
            .setTimezone("Z")
            .build();
    assertThat(new DateTimeWrapper(input).toString()).isEqualTo("1970-01-01");
    assertThat(new DateTimeWrapper(input, ZoneId.of("Australia/Sydney")).toString())
        .isEqualTo("1970-01-01");

    input = input.toBuilder().setValueUs(-36000000000L).setTimezone("Australia/Sydney").build();
    assertThat(new DateTimeWrapper(input).toString()).isEqualTo("1970-01-01");
    assertThat(new DateTimeWrapper(input, ZoneId.of("Australia/Sydney")).toString())
        .isEqualTo("1970-01-01");
  }

  @Test
  public void printDateTime() {
    DateTime input =
        DateTime.newBuilder()
            .setValueUs(43200123000L)
            .setPrecision(DateTime.Precision.MILLISECOND)
            .setTimezone("Z")
            .build();
    assertThat(new DateTimeWrapper(input, ZoneId.of("Europe/London")).toString())
        .isEqualTo("1970-01-01T12:00:00.123Z");

    input =
        DateTime.newBuilder()
            .setValueUs(43200123000L)
            .setPrecision(DateTime.Precision.MILLISECOND)
            .setTimezone("UTC")
            .build();
    assertThat(new DateTimeWrapper(input, ZoneId.of("Europe/London")).toString())
        .isEqualTo("1970-01-01T12:00:00.123Z");

    input =
        DateTime.newBuilder()
            .setValueUs(1412827080000000L)
            .setPrecision(DateTime.Precision.SECOND)
            .setTimezone("+11:00")
            .build();
    assertThat(new DateTimeWrapper(input, ZoneId.of("Europe/London")).toString())
        .isEqualTo("2014-10-09T14:58:00+11:00");

    input =
        DateTime.newBuilder()
            .setValueUs(1412827080000000L)
            .setPrecision(DateTime.Precision.SECOND)
            .setTimezone("Australia/Sydney")
            .build();
    assertThat(new DateTimeWrapper(input, ZoneId.of("Australia/Sydney")).toString())
        .isEqualTo("2014-10-09T14:58:00+11:00");

    input =
        DateTime.newBuilder()
            .setValueUs(43200123000L)
            .setPrecision(DateTime.Precision.MILLISECOND)
            .setTimezone("+00:00")
            .build();
    assertThat(new DateTimeWrapper(input, ZoneId.of("Europe/London")).toString())
        .isEqualTo("1970-01-01T12:00:00.123+00:00");

    input =
        DateTime.newBuilder()
            .setValueUs(43200123000L)
            .setPrecision(DateTime.Precision.MILLISECOND)
            .setTimezone("-00:00")
            .build();
    assertThat(new DateTimeWrapper(input, ZoneId.of("Europe/London")).toString())
        .isEqualTo("1970-01-01T12:00:00.123-00:00");
  }
}
