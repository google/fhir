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
import com.google.fhir.common.ProtoUtils;
import com.google.fhir.r4.core.Instant;
import com.google.protobuf.MessageOrBuilder;
import java.time.OffsetDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeParseException;
import java.util.regex.Pattern;

/** A wrapper around the Instant FHIR primitive type. */
public class InstantWrapper extends PrimitiveWrapper<Instant> {

  private static final Pattern INSTANT_PATTERN =
      Pattern.compile(
          "-?[0-9]{4}-(0[1-9]|1[0-2])-(0[0-9]|[1-2][0-9]|3[0-1])T([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9](\\.[0-9]+)?(Z|(\\+|-)((0[0-9]|1[0-3]):[0-5][0-9]|14:00))");
  private static final Instant NULL_INSTANT =
      Instant.newBuilder().addExtension(getNoValueExtension()).build();

  private static final DateTimeFormatter SECOND_WITH_TZ =
      DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ssXXX");
  private static final ImmutableMap<Instant.Precision, DateTimeFormatter> FORMATTERS =
      ImmutableMap.of(
          Instant.Precision.SECOND,
          SECOND_WITH_TZ,
          Instant.Precision.MILLISECOND,
          DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ss.SSSXXX"));

  /** Create an InstantWrapper from an Instant. */
  public InstantWrapper(Instant instant) {
    super(instant);
  }

  public InstantWrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, Instant.newBuilder()).build());
  }

  /** Create an InstantWrapper from a java String. */
  public InstantWrapper(String input) {
    super(input == null ? NULL_INSTANT : parseAndValidate(input));
  }

  private static Instant parseAndValidate(String input) {
    validateUsingPattern(INSTANT_PATTERN, input);
    try {
      OffsetDateTime offsetDateTime = OffsetDateTime.parse(input, SECOND_WITH_TZ);
      String timezone = extractFhirTimezone(input, offsetDateTime);
      return buildInstant(
          offsetDateTime.toInstant().toEpochMilli() * 1000L, timezone, Instant.Precision.SECOND);
    } catch (DateTimeParseException e) {
      // Fall through.
    }
    try {
      // The default parser for OffsetDateTime handles fractional seconds.
      OffsetDateTime offsetDateTime = OffsetDateTime.parse(input);
      String timezone = extractFhirTimezone(input, offsetDateTime);
      return buildInstant(
          offsetDateTime.toInstant().toEpochMilli() * 1000L,
          timezone,
          Instant.Precision.MILLISECOND);
    } catch (DateTimeParseException e) {
      // Fall through.
    }
    throw new IllegalArgumentException("Invalid Instant: " + input);
  }

  private static Instant buildInstant(long valueUs, String timezone, Instant.Precision precision) {
    return Instant.newBuilder()
        .setValueUs(valueUs)
        .setPrecision(precision)
        .setTimezone(timezone)
        .build();
  }

  @Override
  protected String printValue() {
    DateTimeFormatter formatter = FORMATTERS.get(getWrapped().getPrecision());
    if (formatter == null) {
      throw new IllegalArgumentException("Invalid precision: " + getWrapped().getPrecision());
    }
    return withOriginalTimezone(
        java.time.Instant.ofEpochMilli(getWrapped().getValueUs() / 1000L)
            .atZone(ZoneId.of(getWrapped().getTimezone()))
            .format(formatter),
        getWrapped().getTimezone());
  }
}
