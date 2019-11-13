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

import com.google.common.io.BaseEncoding;
import com.google.fhir.common.ProtoUtils;
import com.google.fhir.r4.core.Base64Binary;
import com.google.fhir.r4.core.PositiveInt;
import com.google.fhir.r4.google.Base64BinarySeparatorStride;
import com.google.protobuf.ByteString;
import com.google.protobuf.Message;
import com.google.protobuf.MessageOrBuilder;
import java.util.Collections;
import java.util.List;

/** A wrapper around the Base64Binary FHIR primitive type. */
public class Base64BinaryWrapper extends PrimitiveWrapper<Base64Binary> {

  private static final Base64Binary NULL_BASE64_BINARY =
      Base64Binary.newBuilder().addExtension(getNoValueExtension()).build();

  /** Create an Base64BinaryWrapper from a Base64Binary. */
  public Base64BinaryWrapper(Base64Binary base64Binary) {
    super(base64Binary);
  }

  public Base64BinaryWrapper(MessageOrBuilder base64Binary) {
    super(ProtoUtils.fieldWiseCopy(base64Binary, Base64Binary.newBuilder()).build());
  }

  /** Create an Base64BinaryWrapper from a java String. */
  public Base64BinaryWrapper(String input) {
    super(input == null ? NULL_BASE64_BINARY : parseAndValidate(input));
  }

  private static Base64Binary parseAndValidate(String input) {
    BaseEncoding encoding = BaseEncoding.base64();
    Base64Binary.Builder builder = Base64Binary.newBuilder();
    int stride = input.indexOf(' ');
    if (stride != -1) {
      int end = stride;
      while (end < input.length() && input.charAt(end) == ' ') {
        end++;
      }
      String separator = input.substring(stride, end);
      Base64BinarySeparatorStride strideExtension =
          Base64BinarySeparatorStride.newBuilder()
              .setSeparator(com.google.fhir.r4.core.String.newBuilder().setValue(separator))
              .setStride(PositiveInt.newBuilder().setValue(stride))
              .build();
      encoding = encoding.withSeparator(separator, stride);
      ExtensionWrapper.of().add(strideExtension).addToMessage(builder);
    }
    try {
      return builder.setValue(ByteString.copyFrom(encoding.decode(input))).build();
    } catch (IllegalArgumentException e) {
      throw new IllegalArgumentException("Invalid base64", e);
    }
  }

  @Override
  protected String printValue() {
    BaseEncoding encoding = BaseEncoding.base64();
    List<Base64BinarySeparatorStride> strideExtension =
        ExtensionWrapper.fromExtensionsIn(getWrapped())
            .getMatchingExtensions(Base64BinarySeparatorStride.getDefaultInstance());
    if (!strideExtension.isEmpty()) {
      encoding =
          encoding.withSeparator(
              strideExtension.get(0).getSeparator().getValue(),
              strideExtension.get(0).getStride().getValue());
    }
    return encoding.encode(getWrapped().getValue().toByteArray());
  }

  @Override
  protected List<Message> getInternalExtensions() {
    return Collections.singletonList(Base64BinarySeparatorStride.getDefaultInstance());
  }
}
