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
import com.google.fhir.r4.core.Markdown;
import com.google.protobuf.MessageOrBuilder;

/** A wrapper around the Markdown FHIR primitive type. */
public class MarkdownWrapper extends PrimitiveWrapper<Markdown> {

  private static final Markdown NULL_MARKDOWN =
      Markdown.newBuilder().addExtension(getNoValueExtension()).build();

  /** Create an MarkdownWrapper from a Markdown. */
  public MarkdownWrapper(Markdown markdown) {
    super(markdown);
  }

  public MarkdownWrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, Markdown.newBuilder()).build());
  }

  /** Create an MarkdownWrapper from a java String. */
  public MarkdownWrapper(String input) {
    super(input == null ? NULL_MARKDOWN : Markdown.newBuilder().setValue(input).build());
  }

  @Override
  protected String printValue() {
    return getWrapped().getValue();
  }
}
