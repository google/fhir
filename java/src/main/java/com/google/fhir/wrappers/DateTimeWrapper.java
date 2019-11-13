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

import com.google.common.collect.ImmutableMap;
import com.google.fhir.common.AnnotationUtils;
import com.google.fhir.common.ProtoUtils;
import com.google.fhir.r4.core.DateTime;
import com.google.protobuf.MessageOrBuilder;
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
      Pattern.compile(AnnotationUtils.getValueRegexForPrimitiveType(DateTime.getDefaultInstance()));
  private static final DateTime NULL_DATE_TIME =
      DateTime.newBuilder().addExtension(getNoValueExtension()).build();

  private static final DateTimeFormatter SECOND_WITH_TZ =
      DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ssXXX");
  private static final DateTimeFormatter MILLISECOND_WITH_TZ =
      DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ss.SSSXXX");

  private static final ImmutableMap<DateTime.Precision, DateTimeFormatter> FORMATTERS =
      ImmutableMap.of(
          DateTime.Precision.YEAR, DateTimeFormatter.ofPattern("yyyy"),
          DateTime.Precision.MONTH, DateTimeFormatter.ofPattern("yyyy-MM"),
          DateTime.Precision.DAY, DateTimeFormatter.ofPattern("yyyy-MM-dd"),
          DateTime.Precision.SECOND, SECOND_WITH_TZ,
          DateTime.Precision.MILLISECOND, MILLISECOND_WITH_TZ);

  /** Create a DateTimeWrapper from a DateTime. */
  public DateTimeWrapper(DateTime dateTime) {
    super(dateTime);
  }

  public DateTimeWrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, DateTime.newBuilder()).build());
  }

  /**
   * Create a DateTimeWrapper from a DateTime and a default timezone. The default timezone is
   * currently unused. Once there are finalized timezone extension, this class will use those to
   * emit timezones when they differ from the default.
   */
  public DateTimeWrapper(DateTime dateTime, ZoneId defaultTimeZone) {
    super(dateTime);
  }

  public DateTimeWrapper(MessageOrBuilder message, ZoneId defaultTimeZone) {
    super(ProtoUtils.fieldWiseCopy(message, DateTime.newBuilder()).build());
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

    // DateTime, with timezone offset.
    try {
      OffsetDateTime offsetDateTime = OffsetDateTime.parse(input, SECOND_WITH_TZ);
      String timezone = extractFhirTimezone(input, offsetDateTime);
      return buildDateTime(
          offsetDateTime.toInstant().toEpochMilli() * 1000L, timezone, DateTime.Precision.SECOND);
    } catch (DateTimeParseException e) {
      // Fall through.
    }
    try {
      OffsetDateTime offsetDateTime = OffsetDateTime.parse(input);
      String timezone = extractFhirTimezone(input, offsetDateTime);
      return buildDateTime(
          offsetDateTime.toInstant().toEpochMilli() * 1000L,
          timezone,
          DateTime.Precision.MILLISECOND);
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
        .setTimezone(timezone)
        .setPrecision(precision)
        .build();
  }

  private static DateTime buildDateTime(
      long valueUs, String timezone, DateTime.Precision precision) {
    return DateTime.newBuilder()
        .setValueUs(valueUs)
        .setPrecision(precision)
        .setTimezone(timezone)
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
    formatter = FORMATTERS.get(dateTime.getPrecision());
    if (formatter == null) {
      throw new IllegalArgumentException("Invalid precision: " + dateTime.getPrecision());
    }
    return withOriginalTimezone(
        Instant.ofEpochMilli(dateTime.getValueUs() / 1000L).atZone(zoneId).format(formatter),
        dateTime.getTimezone());
  }
}
