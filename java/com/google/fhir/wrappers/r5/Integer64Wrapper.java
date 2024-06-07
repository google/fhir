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
import com.google.fhir.r5.core.Integer64;
import com.google.protobuf.MessageOrBuilder;
import java.util.regex.Pattern;

/** A wrapper around the Integer64 FHIR primitive type. */
final class Integer64Wrapper extends PrimitiveWrapper<Integer64> {

  private static final Pattern REGEX_PATTERN =
      Pattern.compile(
          AnnotationUtils.getValueRegexForPrimitiveType(Integer64.getDefaultInstance()));

  @Override
  protected Pattern getPattern() {
    return REGEX_PATTERN;
  }

  private static final Integer64 NULL_INTEGER_64 =
      Integer64.newBuilder().addExtension(getNoValueExtension()).build();

  /** Create an Integer64Wrapper from an Integer64. */
  public Integer64Wrapper(Integer64 integer64) {
    super(integer64);
  }

  public Integer64Wrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, Integer64.newBuilder()).build());
  }

  /** Create an Integer64Wrapper from a java String. */
  public Integer64Wrapper(String input) {
    super(input == null ? NULL_INTEGER_64 : parseAndValidate(input));
  }

  private static Integer64 parseAndValidate(String input) {
    validateUsingPattern(REGEX_PATTERN, input);
    return Integer64.newBuilder().setValue(Long.parseLong(input)).build();
  }

  @Override
  protected String printValue() {
    return Long.toString(getWrapped().getValue());
  }
}
