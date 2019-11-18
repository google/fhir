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
import com.google.fhir.r4.core.Element;
import com.google.fhir.r4.core.Xhtml;
import com.google.protobuf.MessageOrBuilder;
import java.util.regex.Pattern;

/** A wrapper around the Xhtml FHIR primitive type. */
public class XhtmlWrapper extends PrimitiveWrapper<Xhtml> {

  private static final Pattern REGEX_PATTERN =
      Pattern.compile(".*"); // Xhtml has no validation pattern.

  @Override
  protected Pattern getPattern() {
    return REGEX_PATTERN;
  }

  /** Create an XhtmlWrapper from an Xhtml. */
  public XhtmlWrapper(Xhtml xhtml) {
    super(xhtml);
  }

  public XhtmlWrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, Xhtml.newBuilder()).build());
  }

  /** Create an XhtmlWrapper from a java String, disallowing null inputs */
  public XhtmlWrapper(String input) {
    super(input == null ? null : Xhtml.newBuilder().setValue(input).build());
    if (input == null) {
      throw new IllegalArgumentException("Invalid input: null");
    }
  }

  /**
   * We need a custom validateWrapped function because Xhtml doesn't have an extension field. Since
   * it can't have extensions, and there's no regex for validation, it's always valid
   */
  @Override
  public void validateWrapped() {
    return;
  }

  @Override
  protected String printValue() {
    return getWrapped().getValue();
  }

  /** All valid Xhtml objects contain a value. */
  @Override
  public boolean hasValue() {
    return true;
  }

  /** Get the Element part of this primitive. Xhtml objects can never have any extensions. */
  @Override
  public Element getElement() {
    if (!getWrapped().hasId()) {
      return null;
    }
    Element.Builder builder = Element.newBuilder();
    ProtoUtils.fieldWiseCopy(getWrapped().getId(), builder.getIdBuilder());
    return builder.build();
  }
}
