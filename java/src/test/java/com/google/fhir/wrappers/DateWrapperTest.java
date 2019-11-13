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

import com.google.fhir.r4.core.Date;
import java.time.ZoneId;
import java.time.ZoneOffset;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link DateWrapper}. */
@RunWith(JUnit4.class)
public final class DateWrapperTest {

  @Test
  public void parseYear() {
    Date parsed = new DateWrapper("1971", ZoneOffset.UTC).getWrapped();
    Date expected =
        Date.newBuilder()
            .setValueUs(31536000000000L)
            .setPrecision(Date.Precision.YEAR)
            .setTimezone("Z")
            .build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new DateWrapper("1971", ZoneId.of("Australia/Sydney")).getWrapped();
    expected =
        Date.newBuilder()
            .setValueUs(31500000000000L)
            .setPrecision(Date.Precision.YEAR)
            .setTimezone("Australia/Sydney")
            .build();
    assertThat(parsed).isEqualTo(expected);
  }

  @Test
  public void parseYearMonth() {
    Date parsed = new DateWrapper("1970-02", ZoneOffset.UTC).getWrapped();
    Date expected =
        Date.newBuilder()
            .setValueUs(2678400000000L)
            .setPrecision(Date.Precision.MONTH)
            .setTimezone("Z")
            .build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new DateWrapper("1970-02", ZoneId.of("Australia/Sydney")).getWrapped();
    expected =
        Date.newBuilder()
            .setValueUs(2642400000000L)
            .setPrecision(Date.Precision.MONTH)
            .setTimezone("Australia/Sydney")
            .build();
    assertThat(parsed).isEqualTo(expected);
  }

  @Test
  public void parseYearMonthDay() {
    Date parsed = new DateWrapper("1970-01-01", ZoneOffset.UTC).getWrapped();
    Date expected =
        Date.newBuilder().setValueUs(0).setPrecision(Date.Precision.DAY).setTimezone("Z").build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new DateWrapper("1970-01-01", ZoneId.of("Australia/Sydney")).getWrapped();
    expected =
        Date.newBuilder()
            .setValueUs(-36000000000L)
            .setPrecision(Date.Precision.DAY)
            .setTimezone("Australia/Sydney")
            .build();
    assertThat(parsed).isEqualTo(expected);
  }

  @Test
  public void parseYearMonthDayWithOffset() {
    Date parsed = new DateWrapper("1970-01-01", ZoneOffset.UTC).getWrapped();
    Date expected =
        Date.newBuilder().setValueUs(0).setPrecision(Date.Precision.DAY).setTimezone("Z").build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new DateWrapper("1970-01-01", ZoneId.of("-05:00")).getWrapped();
    expected =
        Date.newBuilder()
            .setValueUs(18000000000L)
            .setPrecision(Date.Precision.DAY)
            .setTimezone("-05:00")
            .build();
    assertThat(parsed).isEqualTo(expected);
  }

  @Test
  public void printYear() {
    Date input =
        Date.newBuilder()
            .setValueUs(0)
            .setPrecision(Date.Precision.YEAR)
            .setTimezone("Z")
            .build();
    assertThat(new DateWrapper(input).toString()).isEqualTo("1970");
    assertThat(new DateWrapper(input, ZoneId.of("Australia/Sydney")).toString()).isEqualTo("1970");

    input = input.toBuilder().setValueUs(-36000000000L).setTimezone("Australia/Sydney").build();
    assertThat(new DateWrapper(input).toString()).isEqualTo("1970");
    assertThat(new DateWrapper(input, ZoneId.of("Australia/Sydney")).toString()).isEqualTo("1970");
  }

  @Test
  public void printYearMonth() {
    Date input =
        Date.newBuilder()
            .setValueUs(0)
            .setPrecision(Date.Precision.MONTH)
            .setTimezone("Z")
            .build();
    assertThat(new DateWrapper(input).toString()).isEqualTo("1970-01");
    assertThat(new DateWrapper(input, ZoneId.of("Australia/Sydney")).toString())
        .isEqualTo("1970-01");

    input = input.toBuilder().setValueUs(-36000000000L).setTimezone("Australia/Sydney").build();
    assertThat(new DateWrapper(input).toString()).isEqualTo("1970-01");
    assertThat(new DateWrapper(input, ZoneId.of("Australia/Sydney")).toString())
        .isEqualTo("1970-01");
  }

  @Test
  public void printYearMonthDay() {
    Date input =
        Date.newBuilder().setValueUs(0).setPrecision(Date.Precision.DAY).setTimezone("Z").build();
    assertThat(new DateWrapper(input).toString()).isEqualTo("1970-01-01");
    assertThat(new DateWrapper(input, ZoneId.of("Australia/Sydney")).toString())
        .isEqualTo("1970-01-01");

    input = input.toBuilder().setValueUs(-36000000000L).setTimezone("Australia/Sydney").build();
    assertThat(new DateWrapper(input).toString()).isEqualTo("1970-01-01");
    assertThat(new DateWrapper(input, ZoneId.of("Australia/Sydney")).toString())
        .isEqualTo("1970-01-01");
  }
}
