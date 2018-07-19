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

import com.google.fhir.stu3.proto.PositiveInt;
import java.util.regex.Pattern;

/** A wrapper around the PositiveInt FHIR primitive type. */
public class PositiveIntWrapper extends NumericTypeWrapper<PositiveInt> {

  private static final Pattern POSITIVE_INT_PATTERN =
      Pattern.compile(
          AnnotationUtils.getValueRegexForPrimitiveType(PositiveInt.getDefaultInstance()));
  private static final PositiveInt NULL_POSITIVE_INT =
      PositiveInt.newBuilder().addExtension(getNoValueExtension()).build();

  /** Create an PositiveIntWrapper from a PositiveInt. */
  public PositiveIntWrapper(PositiveInt positiveInt) {
    super(positiveInt);
  }

  /** Create an PositiveIntWrapper from a java String. */
  public PositiveIntWrapper(String input) {
    super(input == null ? NULL_POSITIVE_INT : parseAndValidate(input));
  }

  private static PositiveInt parseAndValidate(String input) {
    validateUsingPattern(POSITIVE_INT_PATTERN, input);
    return PositiveInt.newBuilder().setValue(Integer.parseInt(input)).build();
  }

  @Override
  protected String printValue() {
    return Integer.toString(getWrapped().getValue());
  }
}
