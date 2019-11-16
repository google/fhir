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
import com.google.fhir.r4.core.Uri;
import com.google.protobuf.MessageOrBuilder;
import java.util.regex.Pattern;

/** A wrapper around the Uri FHIR primitive type. */
public class UriWrapper extends PrimitiveWrapper<Uri> {

  private static final Pattern REGEX_PATTERN =
      Pattern.compile(AnnotationUtils.getValueRegexForPrimitiveType(Uri.getDefaultInstance()));

  @Override
  protected Pattern getPattern() {
    return REGEX_PATTERN;
  }

  private static final Uri NULL_URI = Uri.newBuilder().addExtension(getNoValueExtension()).build();

  /** Create an UriWrapper from a Uri. */
  public UriWrapper(Uri uri) {
    super(uri);
  }

  public UriWrapper(MessageOrBuilder message) {
    super(ProtoUtils.fieldWiseCopy(message, Uri.newBuilder()).build());
  }

  /** Create an UriWrapper from a java String. */
  public UriWrapper(String input) {
    super(input == null ? NULL_URI : parseAndValidate(input));
  }

  private static Uri parseAndValidate(String input) {
    validateUsingPattern(REGEX_PATTERN, input);
    return Uri.newBuilder().setValue(input).build();
  }

  @Override
  protected String printValue() {
    return getWrapped().getValue();
  }
}
