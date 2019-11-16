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
import com.google.fhir.r4.core.Date;
import com.google.protobuf.MessageOrBuilder;
import java.time.Instant;
import java.time.LocalDate;
import java.time.Year;
import java.time.YearMonth;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeParseException;
import java.util.regex.Pattern;

/** A wrapper around the Date FHIR primitive type. */
public class DateWrapper extends PrimitiveWrapper<Date> {

  private static final Pattern REGEX_PATTERN =
      Pattern.compile(AnnotationUtils.getValueRegexForPrimitiveType(Date.getDefaultInstance()));

  @Override
  protected Pattern getPattern() {
    return REGEX_PATTERN;
  }

  private static final Date NULL_DATE =
      Date.newBuilder().addExtension(getNoValueExtension()).build();

  private static final ImmutableMap<Date.Precision, DateTimeFormatter> FORMATTERS =
      ImmutableMap.of(
          Date.Precision.YEAR, DateTimeFormatter.ofPattern("yyyy"),
          Date.Precision.MONTH, DateTimeFormatter.ofPattern("yyyy-MM"),
          Date.Precision.DAY, DateTimeFormatter.ofPattern("yyyy-MM-dd"));

  /** Create an DateWrapper from a Date. */
  public DateWrapper(Date date) {
    super(date);
  }

  public DateWrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, Date.newBuilder()).build());
  }

  /**
   * Create a DateWrapper from a Date and a default timezone. The default timezone is currently
   * unused, since FHIR dates do not include timezones. Once there are finalized timezone extension,
   * this class will use those to emit timezones when they differ from the default.
   */
  public DateWrapper(Date date, ZoneId defaultTimeZone) {
    super(date);
  }

  public DateWrapper(MessageOrBuilder message, ZoneId defaultTimeZone) {
    super(ProtoUtils.fieldWiseCopy(message, Date.newBuilder()).build());
  }

  /** Create a DateWrapper from a java String and a default timezone. */
  public DateWrapper(String input, ZoneId defaultTimeZone) {
    super(input == null ? NULL_DATE : parseAndValidate(input, defaultTimeZone));
  }

  private static Date parseAndValidate(String input, ZoneId defaultTimeZone) {
    validateUsingPattern(REGEX_PATTERN, input);
    try {
      return buildDate(Year.parse(input).atDay(1), defaultTimeZone, Date.Precision.YEAR);
    } catch (DateTimeParseException e) {
      // Fall through.
    }
    try {
      return buildDate(YearMonth.parse(input).atDay(1), defaultTimeZone, Date.Precision.MONTH);
    } catch (DateTimeParseException e) {
      // Fall through.
    }
    try {
      return buildDate(LocalDate.parse(input), defaultTimeZone, Date.Precision.DAY);
    } catch (DateTimeParseException e) {
      // Fall through.
    }
    throw new IllegalArgumentException("Invalid Date: " + input);
  }

  private static Date buildDate(LocalDate date, ZoneId defaultTimeZone, Date.Precision precision) {
    String timezone = defaultTimeZone.toString();
    return Date.newBuilder()
        .setValueUs(date.atStartOfDay().atZone(defaultTimeZone).toInstant().toEpochMilli() * 1000L)
        .setPrecision(precision)
        .setTimezone(timezone)
        .build();
  }

  @Override
  protected String printValue() {
    Date date = getWrapped();
    if (date.getTimezone().isEmpty()) {
      throw new IllegalArgumentException("Date missing timezone");
    }
    ZoneId zoneId = ZoneId.of(date.getTimezone());
    DateTimeFormatter formatter = FORMATTERS.get(date.getPrecision());
    if (formatter == null) {
      throw new IllegalArgumentException("Invalid precision: " + date.getPrecision());
    }
    return Instant.ofEpochMilli(date.getValueUs() / 1000L).atZone(zoneId).format(formatter);
  }
}
