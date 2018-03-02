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

package com.google.fhir.stu3;

import com.google.common.collect.ImmutableMap;
import com.google.fhir.stu3.proto.DateTime;
import java.time.Instant;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.OffsetDateTime;
import java.time.Year;
import java.time.YearMonth;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeParseException;
import java.util.regex.Pattern;

/** A wrapper around the DateTime FHIR primitive type. */
public class DateTimeWrapper extends PrimitiveWrapper<DateTime> {

  private static final Pattern DATE_TIME_PATTERN =
      Pattern.compile(
          "-?[0-9]{4}(-(0[1-9]|1[0-2])(-(0[0-9]|[1-2][0-9]|3[0-1])(T([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9](\\.[0-9]+)?(Z|(\\+|-)((0[0-9]|1[0-3]):[0-5][0-9]|14:00)))?)?)?");
  private static final DateTime NULL_DATE_TIME =
      DateTime.newBuilder().addExtension(getNoValueExtension()).build();

  private static final DateTimeFormatter SECOND =
      DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ss");
  private static final DateTimeFormatter SECOND_WITH_TZ =
      DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ssXXX");

  private static final ImmutableMap<DateTime.Precision, DateTimeFormatter> FORMATTERS =
      ImmutableMap.of(
          DateTime.Precision.YEAR, DateTimeFormatter.ofPattern("yyyy"),
          DateTime.Precision.MONTH, DateTimeFormatter.ofPattern("yyyy-MM"),
          DateTime.Precision.DAY, DateTimeFormatter.ofPattern("yyyy-MM-dd"),
          DateTime.Precision.SECOND, SECOND,
          DateTime.Precision.MILLISECOND, DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ss.SSS"));
  private static final ImmutableMap<DateTime.Precision, DateTimeFormatter> TZ_FORMATTERS =
      ImmutableMap.of(
          DateTime.Precision.YEAR,
          DateTimeFormatter.ofPattern("yyyy"),
          DateTime.Precision.MONTH,
          DateTimeFormatter.ofPattern("yyyy-MM"),
          DateTime.Precision.DAY,
          DateTimeFormatter.ofPattern("yyyy-MM-dd"),
          DateTime.Precision.SECOND,
          SECOND_WITH_TZ,
          DateTime.Precision.MILLISECOND,
          DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ss.SSSXXX"));

  private ZoneId defaultTimeZone;

  /** Create a DateTimeWrapper from a DateTime. */
  public DateTimeWrapper(DateTime dateTime) {
    super(dateTime);
    this.defaultTimeZone = null;
  }

  /**
   * Create a DateTimeWrapper from a DateTime and a default timezone. The default timezone is used
   * when printing the DateTime; if the timezone of dateTime is defaultTimeZone, it is considered to
   * be local, and no timezone information will be printed. Note that explicit offsets such as
   * "+11:00" or "UTC" are considered different from "Australia/Sydney" or "Europe/London", even
   * when they happen to resolve to the same offset.
   */
  public DateTimeWrapper(DateTime dateTime, ZoneId defaultTimeZone) {
    super(dateTime);
    this.defaultTimeZone = defaultTimeZone;
  }

  /** Create a DateTimeWrapper from a java String and a default timezone. */
  public DateTimeWrapper(String input, ZoneId defaultTimeZone) {
    super(input == null ? NULL_DATE_TIME : parseAndValidate(input, defaultTimeZone));
  }

  private static DateTime parseAndValidate(String input, ZoneId defaultTimeZone) {
    validateUsingPattern(DATE_TIME_PATTERN, input);

    // Dates, no provided timezone.
    try {
      return buildDateTime(
          Year.parse(input).atDay(1).atStartOfDay(), defaultTimeZone, DateTime.Precision.YEAR);
    } catch (DateTimeParseException e) {
      // Fall through.
    }
    try {
      return buildDateTime(
          YearMonth.parse(input).atDay(1).atStartOfDay(),
          defaultTimeZone,
          DateTime.Precision.MONTH);
    } catch (DateTimeParseException e) {
      // Fall through.
    }
    try {
      return buildDateTime(
          LocalDate.parse(input).atStartOfDay(), defaultTimeZone, DateTime.Precision.DAY);
    } catch (DateTimeParseException e) {
      // Fall through.
    }

    // DateTime, without timezone offset.
    try {
      return buildDateTime(
          LocalDateTime.parse(input, SECOND), defaultTimeZone, DateTime.Precision.SECOND);
    } catch (DateTimeParseException e) {
      // Fall through.
    }
    try {
      return buildDateTime(
          LocalDateTime.parse(input), defaultTimeZone, DateTime.Precision.MILLISECOND);
    } catch (DateTimeParseException e) {
      // Fall through.
    }

    // DateTime, with timezone offset.
    try {
      return buildDateTime(OffsetDateTime.parse(input, SECOND_WITH_TZ), DateTime.Precision.SECOND);
    } catch (DateTimeParseException e) {
      // Fall through.
    }
    try {
      return buildDateTime(OffsetDateTime.parse(input), DateTime.Precision.MILLISECOND);
    } catch (DateTimeParseException e) {
      // Fall through.
    }
    throw new IllegalArgumentException("Invalid DateTime: " + input);
  }

  private static DateTime buildDateTime(
      LocalDateTime dateTime, ZoneId defaultTimeZone, DateTime.Precision precision) {
    String timezone = defaultTimeZone.toString();
    return DateTime.newBuilder()
        .setValueUs(dateTime.atZone(defaultTimeZone).toInstant().toEpochMilli() * 1000L)
        .setTimezone("Z".equals(timezone) ? "UTC" : timezone) // Prefer "UTC" over "Z" for GMT
        .setPrecision(precision)
        .build();
  }

  private static DateTime buildDateTime(OffsetDateTime dateTime, DateTime.Precision precision) {
    String timezone = dateTime.getOffset().toString();
    return DateTime.newBuilder()
        .setValueUs(dateTime.toInstant().toEpochMilli() * 1000L)
        .setPrecision(precision)
        .setTimezone("Z".equals(timezone) ? "UTC" : timezone) // Prefer "UTC" over "Z" for GMT
        .build();
  }

  @Override
  protected String printValue() {
    DateTime dateTime = getWrapped();
    ZoneId zoneId;
    DateTimeFormatter formatter;
    if (dateTime.getTimezone().isEmpty()) {
      throw new IllegalArgumentException("DateTime missing timezone");
    }
    zoneId = ZoneId.of(dateTime.getTimezone());
    if (zoneId.equals(defaultTimeZone)) {
      formatter = FORMATTERS.get(dateTime.getPrecision());
    } else {
      formatter = TZ_FORMATTERS.get(dateTime.getPrecision());
    }
    if (formatter == null) {
      throw new IllegalArgumentException("Invalid precision: " + dateTime.getPrecision());
    }
    return Instant.ofEpochMilli(dateTime.getValueUs() / 1000L).atZone(zoneId).format(formatter);
  }
}
