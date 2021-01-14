//    Copyright 2021 Google Inc.
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

package com.google.fhir.common;

import static com.google.fhir.common.ProtoUtils.findField;

import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Message;
import com.google.protobuf.MessageOrBuilder;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

/** Utilities for interacting with Extensions in a version-independent way. */
public final class Extensions {

  private Extensions() {}

  public static final String PRIMITIVE_HAS_NO_VALUE_URL =
      "https://g.co/fhir/StructureDefinition/primitiveHasNoValue";

  public static final String BINARY_STRIDE_EXTENSION_URL =
      "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride";

  @SuppressWarnings("unchecked") // safe by specification
  public static List<Message> getExtensions(MessageOrBuilder message) throws InvalidFhirException {
    return (List<Message>) message.getField(getExtensionField(message));
  }

  public static void addExtensionToMessage(Message extension, Message.Builder builder) {
    FieldDescriptor extensionField = builder.getDescriptorForType().findFieldByName("extension");
    builder.addRepeatedField(extensionField, extension);
  }

  public static void forEachExtension(MessageOrBuilder message, Consumer<Message> function)
      throws InvalidFhirException {
    ProtoUtils.<Message>forEachInstance(
        message, getExtensionField(message), (extension, index) -> function.accept(extension));
  }

  public static List<Message> getExtensionsWithUrl(String url, MessageOrBuilder message)
      throws InvalidFhirException {
    FieldDescriptor extensionField = getExtensionField(message);
    List<Message> result = new ArrayList<>();
    ProtoUtils.<Message>forEachInstance(
        message,
        extensionField,
        (extension, i) -> {
          if (getExtensionUrl(extension).equals(url)) {
            result.add(extension);
          }
        });
    return result;
  }

  public static String getExtensionUrl(MessageOrBuilder extension) {
    Message urlMessage =
        (Message) extension.getField(extension.getDescriptorForType().findFieldByName("url"));
    return (java.lang.String)
        urlMessage.getField(urlMessage.getDescriptorForType().findFieldByName("value"));
  }

  public static Object getExtensionValue(MessageOrBuilder extension, String valueField)
      throws InvalidFhirException {
    Message valueChoice = (Message) extension.getField(findField(extension, "value"));
    Message valueBuilder = (Message) valueChoice.getField(findField(valueChoice, valueField));
    return valueBuilder.getField(findField(valueBuilder, "value"));
  }

  public static void setExtensionValue(Message.Builder extension, String valueField, Object value)
      throws InvalidFhirException {
    Message.Builder valueChoice = extension.getFieldBuilder(findField(extension, "value"));
    Message.Builder valueBuilder = valueChoice.getFieldBuilder(findField(valueChoice, valueField));
    valueBuilder.setField(findField(valueBuilder, "value"), value);
  }

  private static FieldDescriptor getExtensionField(MessageOrBuilder message)
      throws InvalidFhirException {
    return findField(message, "extension");
  }
}
