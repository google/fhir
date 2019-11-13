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
import com.google.fhir.r4.core.Canonical;
import com.google.protobuf.MessageOrBuilder;
import java.util.regex.Pattern;

/** A wrapper around the Canonical FHIR primitive type. */
public class CanonicalWrapper extends PrimitiveWrapper<Canonical> {

  private static final Pattern CANONICAL_PATTERN =
      Pattern.compile(
          AnnotationUtils.getValueRegexForPrimitiveType(Canonical.getDefaultInstance()));
  private static final Canonical NULL_CANONICAL =
      Canonical.newBuilder().addExtension(getNoValueExtension()).build();

  /** Create an CanonicalWrapper from an Canonical. */
  public CanonicalWrapper(Canonical canonical) {
    super(canonical);
  }

  /** Create an CanonicalWrapper from an Canonical. */
  public CanonicalWrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, Canonical.newBuilder()).build());
  }

  /** Create an CanonicalWrapper from a java String. */
  public CanonicalWrapper(String input) {
    super(input == null ? NULL_CANONICAL : parseAndValidate(input));
  }

  private static Canonical parseAndValidate(String input) {
    validateUsingPattern(CANONICAL_PATTERN, input);
    return Canonical.newBuilder().setValue(input).build();
  }

  @Override
  protected String printValue() {
    return getWrapped().getValue();
  }

  // Extract the uri component from a canonical, which can be of the form
  // uri|version
  // TODO: Consider separating this in the proto into value_uri and version
  public static String getUri(Canonical canonical) {
    String value = canonical.getValue();
    int pipeIndex = value.indexOf("|");
    return pipeIndex != -1 ? value.substring(0, pipeIndex) : value;
  }
}
