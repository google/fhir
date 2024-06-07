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
import com.google.fhir.r5.core.Extension;
import com.google.fhir.r5.core.Uuid;
import com.google.protobuf.MessageOrBuilder;
import java.util.regex.Pattern;

/** A wrapper around the Uuid FHIR primitive type. */
public class UuidWrapper extends PrimitiveWrapper<Uuid> {

  private static final Pattern REGEX_PATTERN =
      Pattern.compile(AnnotationUtils.getValueRegexForPrimitiveType(Uuid.getDefaultInstance()));

  @Override
  protected Pattern getPattern() {
    return REGEX_PATTERN;
  }

  private static final Uuid NULL_UUID =
      Uuid.newBuilder()
          .addExtension(ProtoUtils.fieldWiseCopy(getNoValueExtension(), Extension.newBuilder()))
          .build();

  /** Create an UuidWrapper from a Uuid. */
  public UuidWrapper(Uuid uri) {
    super(uri);
  }

  public UuidWrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, Uuid.newBuilder()).build());
  }

  /** Create an UuidWrapper from a java String. */
  public UuidWrapper(String input) {
    super(input == null ? NULL_UUID : parseAndValidate(input));
  }

  @Override
  protected String printValue() {
    return getWrapped().getValue();
  }

  private static Uuid parseAndValidate(String input) {
    validateUsingPattern(REGEX_PATTERN, input);
    return Uuid.newBuilder().setValue(input).build();
  }
}
