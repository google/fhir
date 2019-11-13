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

import com.google.fhir.common.ProtoUtils;
import com.google.fhir.r4.core.Boolean;
import com.google.gson.JsonPrimitive;
import com.google.protobuf.MessageOrBuilder;
import java.util.regex.Pattern;

/** A wrapper around the Boolean FHIR primitive type. */
public class BooleanWrapper extends PrimitiveWrapper<Boolean> {

  private static final Pattern BOOLEAN_PATTERN = Pattern.compile("true|false");
  private static final Boolean NULL_BOOLEAN =
      Boolean.newBuilder().addExtension(getNoValueExtension()).build();

  /** Create an BooleanWrapper from a Boolean. */
  public BooleanWrapper(Boolean bool) {
    super(bool);
  }

  public BooleanWrapper(MessageOrBuilder bool) {
    super(ProtoUtils.fieldWiseCopy(bool, Boolean.newBuilder()).build());
  }

  /** Create an BooleanWrapper from a java String. */
  public BooleanWrapper(String input) {
    super(input == null ? NULL_BOOLEAN : parseAndValidate(input));
  }

  private static Boolean parseAndValidate(String input) {
    validateUsingPattern(BOOLEAN_PATTERN, input);
    return Boolean.newBuilder().setValue(java.lang.Boolean.parseBoolean(input)).build();
  }

  @Override
  protected String printValue() {
    return java.lang.Boolean.toString(getWrapped().getValue());
  }

  @Override
  public JsonPrimitive toJson() {
    return new JsonPrimitive(getWrapped().getValue());
  }
}
