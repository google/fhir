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
import com.google.fhir.r4.core.Boolean;
import com.google.fhir.r4.core.Element;
import com.google.fhir.r4.core.Extension;
import com.google.fhir.r4.google.PrimitiveHasNoValue;
import com.google.gson.JsonPrimitive;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Message;
import com.google.protobuf.MessageOrBuilder;
import java.time.OffsetDateTime;
import java.util.Collections;
import java.util.List;
import java.util.regex.Pattern;

/**
 * Base class for wrappers around FHIR primitive type protos, providing shared functionality used
 * for serializing and validating data.
 */
public abstract class PrimitiveWrapper<T extends Message> {
  private final T wrapped;

  private static final PrimitiveHasNoValue PRIMITIVE_HAS_NO_VALUE =
      PrimitiveHasNoValue.newBuilder().setValueBoolean(Boolean.newBuilder().setValue(true)).build();

  protected static com.google.fhir.r4.core.Extension getNoValueExtension() {
    return ExtensionWrapper.of().add(PRIMITIVE_HAS_NO_VALUE).build().get(0);
  }

  protected static void validateUsingPattern(Pattern pattern, String input) {
    if (!pattern.matcher(input).matches()) {
      throw new IllegalArgumentException("Invalid input: " + input);
    }
  }

  protected PrimitiveWrapper(T t) {
    wrapped = t;
  }

  /**
   * True if the primitive wapped by this is has a value, as opposed to being purely defined by
   * extensions.
   */
  public boolean hasValue() {
    return hasValue(wrapped);
  }

  public static boolean hasValue(MessageOrBuilder message) {
    List<PrimitiveHasNoValue> extensions =
        ExtensionWrapper.fromExtensionsIn(message).getMatchingExtensions(PRIMITIVE_HAS_NO_VALUE);
    for (PrimitiveHasNoValue e : extensions) {
      if (e.getValueBoolean().getValue()) {
        return false;
      }
    }
    return true;
  }

  public T getWrapped() {
    return wrapped;
  }

  @SuppressWarnings("unchecked")
  public <B extends Message.Builder> B copyInto(B builder) {
    Descriptor wrappedDescriptor = wrapped.getDescriptorForType();
    Descriptor builderDescriptor = builder.getDescriptorForType();

    if (wrappedDescriptor.getFullName().equals(builderDescriptor.getFullName())) {
      // We're trying to copy into a builder of the same type.
      return (B) builder.mergeFrom(wrapped);
    }

    if (wrappedDescriptor.getName().equals(builderDescriptor.getName())) {
      // They're not the same type, but they have the same name.  (e.g., STU3 String vs R4 String).
      // Attempt a field-wise copy.
      return ProtoUtils.fieldWiseCopy(wrapped, builder);
    }

    throw new IllegalArgumentException(
        "Attempted to copy incompatible primitives.  From: "
            + wrappedDescriptor.getFullName()
            + " into: "
            + builderDescriptor.getFullName());
  }

  @Override
  public String toString() {
    return printValue();
  }

  protected abstract String printValue();

  public JsonPrimitive toJson() {
    return new JsonPrimitive(toString());
  }

  protected List<Message> getInternalExtensions() {
    return Collections.<Message>emptyList();
  }

  /** Get the Element part of this primitive, including any publicly visible extensions. */
  public Element getElement() {
    Descriptor descriptor = wrapped.getDescriptorForType();
    FieldDescriptor idField = descriptor.findFieldByName("id");
    ExtensionWrapper extensionWrapper =
        ExtensionWrapper.fromExtensionsIn(wrapped).clearMatchingExtensions(PRIMITIVE_HAS_NO_VALUE);
    for (Message template : getInternalExtensions()) {
      extensionWrapper = extensionWrapper.clearMatchingExtensions(template);
    }
    List<Extension> extensions = extensionWrapper.build();
    if (!wrapped.hasField(idField) && extensions.isEmpty()) {
      return null;
    }
    Element.Builder builder = Element.newBuilder();
    if (wrapped.hasField(idField)) {
      ProtoUtils.fieldWiseCopy((Message) wrapped.getField(idField), builder.getIdBuilder());
    }
    if (!extensions.isEmpty()) {
      builder.addAllExtension(extensions);
    }
    return builder.build();
  }

  protected static String withOriginalTimezone(String timeString, String originalTimezone) {
    // Restore [+-]00:00 if necessary.
    return timeString.endsWith("Z")
            && (originalTimezone.startsWith("+") || originalTimezone.startsWith("-"))
        ? timeString.replace("Z", originalTimezone)
        : timeString;
  }

  // Extracts a valid fhir timezone from a time string, maintaining original.  This is necessary
  // because OffsetDateTime will convert +00:00, -00:00, and Z to Z, which is not reversible.
  protected static String extractFhirTimezone(String timeString, OffsetDateTime offsetDateTime) {
    return timeString.endsWith("+00:00")
        ? "+00:00"
        : (timeString.endsWith("-00:00") ? "-00:00" : offsetDateTime.getOffset().toString());
  }
}
