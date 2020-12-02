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
import com.google.fhir.r4.core.Time;
import com.google.protobuf.MessageOrBuilder;
import java.time.LocalTime;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeParseException;
import java.util.regex.Pattern;

/** A wrapper around the Time FHIR primitive type. */
public class TimeWrapper extends PrimitiveWrapper<Time> {

  private static final Pattern REGEX_PATTERN =
      Pattern.compile(AnnotationUtils.getValueRegexForPrimitiveType(Time.getDefaultInstance()));

  @Override
  protected Pattern getPattern() {
    return REGEX_PATTERN;
  }

  private static final Time NULL_TIME =
      Time.newBuilder().addExtension(getNoValueExtension()).build();

  private static final DateTimeFormatter SECOND = DateTimeFormatter.ofPattern("HH:mm:ss");
  private static final ImmutableMap<Time.Precision, DateTimeFormatter> FORMATTERS =
      ImmutableMap.of(
          Time.Precision.SECOND,
          SECOND,
          Time.Precision.MILLISECOND,
          DateTimeFormatter.ofPattern("HH:mm:ss.SSS"));

  /** Create a TimeWrapper from a Time. */
  public TimeWrapper(Time time) {
    super(time);
  }

  public TimeWrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, Time.newBuilder()).build());
  }

  /** Create a TimeWrapper from a java String. */
  public TimeWrapper(String input) {
    super(input == null ? NULL_TIME : parseAndValidate(input));
  }

  private static Time parseAndValidate(String input) {
    validateUsingPattern(REGEX_PATTERN, input);
    try {
      return buildTime(LocalTime.parse(input, SECOND), Time.Precision.SECOND);
    } catch (DateTimeParseException e) {
      // Fall through.
    }
    try {
      return buildTime(LocalTime.parse(input), Time.Precision.MILLISECOND);
    } catch (DateTimeParseException e) {
      // Fall through.
    }
    throw new IllegalArgumentException("Invalid Time: " + input);
  }

  private static Time buildTime(LocalTime time, Time.Precision precision) {
    return Time.newBuilder().setValueUs(time.toNanoOfDay() / 1000L).setPrecision(precision).build();
  }

  @Override
  protected String printValue() {
    DateTimeFormatter formatter = FORMATTERS.get(getWrapped().getPrecision());
    if (formatter == null) {
      throw new IllegalArgumentException("Invalid precision: " + getWrapped().getPrecision());
    }
    return LocalTime.ofNanoOfDay(getWrapped().getValueUs() * 1000L).format(formatter);
  }
}
