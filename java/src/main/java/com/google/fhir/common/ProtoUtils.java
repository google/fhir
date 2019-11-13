//    Copyright 2019 Google Inc.
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

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Message;
import com.google.protobuf.MessageOrBuilder;
import java.util.function.BiConsumer;

/** Utilities to make it easier to work with proto reflection. */
// These utilities do a lot of unchecked casts based on generic types, use with caution!
@SuppressWarnings("unchecked")
public class ProtoUtils {

  private ProtoUtils() {}

  public static boolean fieldIsSet(MessageOrBuilder message, FieldDescriptor field) {
    return field.isRepeated() ? message.getRepeatedFieldCount(field) > 0 : message.hasField(field);
  }

  public static <T> T getBuilderAtIndex(Message.Builder builder, FieldDescriptor field, int index) {
    if (field.isRepeated()) {
      return (T) builder.getRepeatedFieldBuilder(field, index);
    }
    if (index != 0) {
      throw new IllegalArgumentException(
          "Attempted to get non-zero index on singular field: " + field.getFullName());
    }
    return (T) builder.getFieldBuilder(field);
  }

  public static <T> T getAtIndex(MessageOrBuilder message, FieldDescriptor field, int index) {
    if (field.isRepeated()) {
      return (T) message.getRepeatedField(field, index);
    }
    if (index != 0) {
      throw new IllegalArgumentException(
          "Attempted to get non-zero index on singular field: " + field.getFullName());
    }
    return (T) message.getField(field);
  }

  public static void setAtIndex(
      Message.Builder builder, FieldDescriptor field, int index, Object value) {
    if (field.isRepeated()) {
      builder.setRepeatedField(field, index, value);
    }
    if (index != 0) {
      throw new IllegalArgumentException(
          "Attempted to set non-zero index on singular field: " + field.getFullName());
    }
    builder.setField(field, value);
  }

  public static <T> void forEachInstance(
      MessageOrBuilder message, FieldDescriptor field, BiConsumer<T, Integer> function) {
    if (field.isRepeated()) {
      for (int i = 0; i < message.getRepeatedFieldCount(field); i++) {
        function.accept((T) message.getRepeatedField(field, i), i);
      }
    } else {
      function.accept((T) message.getField(field), 0);
    }
  }

  public static <B extends Message.Builder> B getOrAddBuilder(
      Message.Builder builder, FieldDescriptor field) {
    if (field.isRepeated()) {
      builder.addRepeatedField(field, builder.newBuilderForField(field).build());
      return (B) builder.getRepeatedFieldBuilder(field, builder.getRepeatedFieldCount(field) - 1);
    }
    return (B) builder.getFieldBuilder(field);
  }

  public static <B extends Message.Builder> B fieldWiseCopy(MessageOrBuilder source, B target) {
    Descriptor sourceDescriptor = source.getDescriptorForType();
    Descriptor targetDescriptor = target.getDescriptorForType();

    if (!AnnotationUtils.sameFhirType(sourceDescriptor, targetDescriptor)) {
      throw new IllegalArgumentException(
          "Unable to do a fieldwise copy from "
              + sourceDescriptor.getFullName()
              + " to "
              + targetDescriptor.getFullName()
              + ". They are not the same FHIR types.");
    }

    for (FieldDescriptor sourceField : sourceDescriptor.getFields()) {
      if (!ProtoUtils.fieldIsSet(source, sourceField)) {
        continue;
      }
      FieldDescriptor targetField = targetDescriptor.findFieldByName(sourceField.getName());
      if (targetField == null || sourceField.getType() != targetField.getType()) {
        throw new IllegalArgumentException(
            "Unable to do a fieldwise copy from "
                + sourceDescriptor.getFullName()
                + " to "
                + targetDescriptor.getFullName()
                + ".  Mismatch for field: "
                + sourceField.getFullName());
      }
      if (sourceField.getType() == FieldDescriptor.Type.MESSAGE) {
        ProtoUtils.<Message>forEachInstance(
            source,
            sourceField,
            (sourceValue, index) ->
                fieldWiseCopy(sourceValue, getOrAddBuilder(target, targetField)));
      } else {
        target.setField(targetField, source.getField(sourceField));
      }
    }
    return target;
  }
}
