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

import com.google.fhir.common.AnnotationUtils;
import com.google.fhir.common.ProtoUtils;
import com.google.fhir.r4.core.UnsignedInt;
import com.google.protobuf.MessageOrBuilder;
import java.util.regex.Pattern;

/** A wrapper around the UnsignedInt FHIR primitive type. */
public class UnsignedIntWrapper extends NumericTypeWrapper<UnsignedInt> {

  private static final Pattern REGEX_PATTERN =
      Pattern.compile(
          AnnotationUtils.getValueRegexForPrimitiveType(UnsignedInt.getDefaultInstance()));

  @Override
  protected Pattern getPattern() {
    return REGEX_PATTERN;
  }

  private static final UnsignedInt NULL_UNSIGNED_INT =
      UnsignedInt.newBuilder().addExtension(getNoValueExtension()).build();

  /** Create an UnsignedIntWrapper from an UnsignedInt. */
  public UnsignedIntWrapper(UnsignedInt unsignedInt) {
    super(unsignedInt);
  }

  public UnsignedIntWrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, UnsignedInt.newBuilder()).build());
  }

  /** Create an UnsignedIntWrapper from a java String. */
  public UnsignedIntWrapper(String input) {
    super(input == null ? NULL_UNSIGNED_INT : parseAndValidate(input));
  }

  @Override
  protected String printValue() {
    return Integer.toString(getWrapped().getValue());
  }

  private static UnsignedInt parseAndValidate(String input) {
    validateUsingPattern(REGEX_PATTERN, input);
    return UnsignedInt.newBuilder().setValue(Integer.parseInt(input)).build();
  }
}
