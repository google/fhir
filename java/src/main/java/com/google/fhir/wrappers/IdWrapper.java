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
import com.google.fhir.r4.core.Id;
import com.google.protobuf.MessageOrBuilder;
import java.util.regex.Pattern;

/** A wrapper around the Id FHIR primitive type. */
public class IdWrapper extends PrimitiveWrapper<Id> {

  private static final Pattern ID_PATTERN =
      Pattern.compile(AnnotationUtils.getValueRegexForPrimitiveType(Id.getDefaultInstance()));
  private static final Id NULL_ID = Id.newBuilder().addExtension(getNoValueExtension()).build();

  /** Create an IdWrapper from an Id. */
  public IdWrapper(Id id) {
    super(id);
  }

  public IdWrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, Id.newBuilder()).build());
  }

  /** Create an IdWrapper from a java String. */
  public IdWrapper(String input) {
    super(input == null ? NULL_ID : parseAndValidate(input));
  }

  private static Id parseAndValidate(String input) {
    validateUsingPattern(ID_PATTERN, input);
    return Id.newBuilder().setValue(input).build();
  }

  @Override
  protected String printValue() {
    return getWrapped().getValue();
  }
}
