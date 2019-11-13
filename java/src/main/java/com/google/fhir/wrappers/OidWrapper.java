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
import com.google.fhir.r4.core.Oid;
import com.google.protobuf.MessageOrBuilder;
import java.util.regex.Pattern;

/** A wrapper around the Oid FHIR primitive type. */
public class OidWrapper extends PrimitiveWrapper<Oid> {

  private static final Pattern OID_PATTERN =
      Pattern.compile(AnnotationUtils.getValueRegexForPrimitiveType(Oid.getDefaultInstance()));
  private static final Oid NULL_OID = Oid.newBuilder().addExtension(getNoValueExtension()).build();

  /** Create an OidWrapper from an Oid. */
  public OidWrapper(Oid oid) {
    super(oid);
  }

  public OidWrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, Oid.newBuilder()).build());
  }

  /** Create an OidWrapper from a java String. */
  public OidWrapper(String input) {
    super(input == null ? NULL_OID : parseAndValidate(input));
  }

  private static Oid parseAndValidate(String input) {
    validateUsingPattern(OID_PATTERN, input);
    return Oid.newBuilder().setValue(input).build();
  }

  @Override
  protected String printValue() {
    return getWrapped().getValue();
  }
}
