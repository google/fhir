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
import com.google.fhir.r4.core.String;
import com.google.protobuf.MessageOrBuilder;
import java.util.regex.Pattern;

/** A wrapper around the String FHIR primitive type. */
public class StringWrapper extends PrimitiveWrapper<String> {
  private static final Pattern REGEX_PATTERN =
      Pattern.compile(AnnotationUtils.getValueRegexForPrimitiveType(String.getDefaultInstance()));

  @Override
  protected Pattern getPattern() {
    return REGEX_PATTERN;
  }

  private static final String NULL_STRING =
      String.newBuilder().addExtension(getNoValueExtension()).build();

  /** Create an StringWrapper from a String. */
  public StringWrapper(String string) {
    super(string);
  }

  public StringWrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, String.newBuilder()).build());
  }

  /** Create an StringWrapper from a java String. */
  public StringWrapper(java.lang.String input) {
    super(input == null ? NULL_STRING : parseAndValidate(input));
  }

  private static String parseAndValidate(java.lang.String input) {
    validateUsingPattern(REGEX_PATTERN, input);
    return String.newBuilder().setValue(input).build();
  }

  @Override
  protected java.lang.String printValue() {
    return getWrapped().getValue();
  }
}
