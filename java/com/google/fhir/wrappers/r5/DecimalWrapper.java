//    Copyright 2024 Google Inc.
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

package com.google.fhir.wrappers.r5;

import com.google.fhir.common.AnnotationUtils;
import com.google.fhir.common.ProtoUtils;
import com.google.fhir.r5.core.Decimal;
import com.google.protobuf.MessageOrBuilder;
import java.util.regex.Pattern;

/** A wrapper around the Decimal FHIR primitive type. */
public class DecimalWrapper extends NumericTypeWrapper<Decimal> {

  private static final Pattern REGEX_PATTERN =
      Pattern.compile(AnnotationUtils.getValueRegexForPrimitiveType(Decimal.getDefaultInstance()));

  @Override
  protected Pattern getPattern() {
    return REGEX_PATTERN;
  }

  private static final Decimal NULL_DECIMAL =
      Decimal.newBuilder().addExtension(getNoValueExtension()).build();

  /** Create an DecimalWrapper from a Decimal. */
  public DecimalWrapper(Decimal decimal) {
    super(decimal);
  }

  public DecimalWrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, Decimal.newBuilder()).build());
  }

  /** Create an DecimalWrapper from a java String. */
  public DecimalWrapper(String input) {
    super(input == null ? NULL_DECIMAL : parseAndValidate(input));
  }

  private static Decimal parseAndValidate(String input) {
    validateUsingPattern(REGEX_PATTERN, input);
    return Decimal.newBuilder().setValue(input).build();
  }

  @Override
  protected String printValue() {
    return getWrapped().getValue();
  }
}
