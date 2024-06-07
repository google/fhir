// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.fhir.wrappers.r5;

import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.proto.Annotations;
import com.google.gson.JsonElement;
import com.google.gson.JsonNull;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.MessageOrBuilder;
import java.time.ZoneId;

/** Utility functions for working with primitive wrappers */
// TODO(b/176651098): Some of these functions catch IllegalArgumentExceptions and rethrow as
// InvalidFhirException.  Once wrappers all throw InvalidFhirException, remove this catch-and-throw
public class PrimitiveWrappers {

  private PrimitiveWrappers() {}

  /**
   * Throws an IllegalArgumentException if the primitive is invalid. The three main cases of this
   * are:
   *
   * <ol>
   *   <li>A primitive does not meet the primitive regex - e.g., a decimal with value "1.2.3"
   *   <li>The primitive has no value, and has no extensions other than PrimitiveHasNoValue
   *   <li>The primitive has no value, and does NOT have the PrimitiveHasNoValue extension
   * </ol>
   */
  public static void validatePrimitive(MessageOrBuilder primitive) throws InvalidFhirException {
    try {
      primitiveWrapperOf(primitive).validateWrapped();
    } catch (IllegalArgumentException e) {
      throw new InvalidFhirException(e.getMessage(), e);
    }
  }

  /**
   * Given a message containing a primitive message, returns a PrimitiveWrapper around the message.
   * This allows some useful API calls like printValue, which prints the primitive to its JSON
   * value. Throws an InvalidArgumentException if the message is not a FHIR primitive.
   */
  public static PrimitiveWrapper<?> primitiveWrapperOf(MessageOrBuilder message)
      throws InvalidFhirException {
    Descriptor descriptor = message.getDescriptorForType();
    if (descriptor.getOptions().hasExtension(Annotations.fhirValuesetUrl)) {
      return CodeWrapper.of(message);
    }
    switch (descriptor.getName()) {
      case "Base64Binary":
        return new Base64BinaryWrapper(message);
      case "Boolean":
        return new BooleanWrapper(message);
      case "Code":
        return new CodeWrapper(message);
      case "Date":
        return new DateWrapper(message);
      case "DateTime":
        return new DateTimeWrapper(message);
      case "Decimal":
        return new DecimalWrapper(message);
      case "Id":
        return new IdWrapper(message);
      case "Instant":
        return new InstantWrapper(message);
      case "Integer":
        return new IntegerWrapper(message);
      case "Integer64":
        return new Integer64Wrapper(message);
      case "Markdown":
        return new MarkdownWrapper(message);
      case "Oid":
        return new OidWrapper(message);
      case "PositiveInt":
        return new PositiveIntWrapper(message);
      case "String":
        return new StringWrapper(message);
      case "Time":
        return new TimeWrapper(message);
      case "UnsignedInt":
        return new UnsignedIntWrapper(message);
      case "Uri":
        return new UriWrapper(message);
      case "Xhtml":
        return new XhtmlWrapper(message);
      case "Canonical":
        return new CanonicalWrapper(message);
      case "Url":
        return new UrlWrapper(message);
      case "Uuid":
        return new UuidWrapper(message);
      default:
        throw new InvalidFhirException("Unexpected primitive FHIR type: " + descriptor.getName());
    }
  }

  /**
   * Given a JsonElement, and the expected target FHIR primitive type, wraps the JsonElement in the
   * appropriate PrimitiveWrapper. Throws an IllegalArgumentException if the JsonElement is not
   * valid for the message type requested - E.g., if the JsonElement is a string "foobar" and the
   * message type is a FHIR Decimal.
   */
  public static PrimitiveWrapper<?> parseAndWrap(
      JsonElement json, MessageOrBuilder message, ZoneId defaultTimeZone)
      throws InvalidFhirException {
    try {
      Descriptor descriptor = message.getDescriptorForType();
      if (json.isJsonArray()) {
        // JsonArrays are not allowed here
        throw new InvalidFhirException("Cannot wrap a JsonArray.  Found: " + json.getClass());
      }
      // JSON objects represents extension on a primitive, and are treated as null values.
      if (json.isJsonObject()) {
        json = JsonNull.INSTANCE;
      }
      String jsonString = json.isJsonNull() ? null : json.getAsJsonPrimitive().getAsString();

      if (descriptor.getOptions().hasExtension(Annotations.fhirValuesetUrl)) {
        return new CodeWrapper(jsonString);
      }
      // TODO(b/244184211): Make proper class hierarchy for wrapper input types,
      // so these can all accept JsonElement in constructor, and do type checking there.
      switch (descriptor.getName()) {
        case "Base64Binary":
          checkIsString(json);
          return new Base64BinaryWrapper(jsonString);
        case "Boolean":
          checkIsBoolean(json);
          return new BooleanWrapper(jsonString);
        case "Code":
          checkIsString(json);
          return new CodeWrapper(jsonString);
        case "Date":
          checkIsString(json);
          return new DateWrapper(jsonString, defaultTimeZone);
        case "DateTime":
          checkIsString(json);
          return new DateTimeWrapper(jsonString, defaultTimeZone);
        case "Decimal":
          checkIsNumber(json);
          return new DecimalWrapper(jsonString);
        case "Id":
          checkIsString(json);
          return new IdWrapper(jsonString);
        case "Instant":
          checkIsString(json);
          return new InstantWrapper(jsonString);
        case "Integer":
          checkIsNumber(json);
          return new IntegerWrapper(jsonString);
        case "Integer64":
          checkIsString(json);
          return new Integer64Wrapper(jsonString);
        case "Markdown":
          checkIsString(json);
          return new MarkdownWrapper(jsonString);
        case "Oid":
          checkIsString(json);
          return new OidWrapper(jsonString);
        case "PositiveInt":
          checkIsNumber(json);
          return new PositiveIntWrapper(jsonString);
        case "String":
          checkIsString(json);
          return new StringWrapper(jsonString);
        case "Time":
          checkIsString(json);
          return new TimeWrapper(jsonString);
        case "UnsignedInt":
          checkIsNumber(json);
          return new UnsignedIntWrapper(jsonString);
        case "Uri":
          checkIsString(json);
          return new UriWrapper(jsonString);
        case "Xhtml":
          checkIsString(json);
          return new XhtmlWrapper(jsonString);
        case "Canonical":
          checkIsString(json);
          return new CanonicalWrapper(jsonString);
        case "Url":
          checkIsString(json);
          return new UrlWrapper(jsonString);
        case "Uuid":
          checkIsString(json);
          return new UuidWrapper(jsonString);
        default:
          throw new InvalidFhirException(
              "Unexpected primitive FHIR type: " + descriptor.getName() + " : " + json);
      }
    } catch (IllegalArgumentException e) {
      throw new InvalidFhirException(e.getMessage(), e);
    }
  }

  private static void checkIsBoolean(JsonElement json) throws InvalidFhirException {
    if (!(json.isJsonNull() || json.isJsonObject())
        && !(json.isJsonPrimitive() && json.getAsJsonPrimitive().isBoolean())) {
      throw new InvalidFhirException("Invalid JSON element for boolean: " + json);
    }
  }

  private static void checkIsNumber(JsonElement json) throws InvalidFhirException {
    if (!(json.isJsonNull() || json.isJsonObject())
        && !(json.isJsonPrimitive() && json.getAsJsonPrimitive().isNumber())) {
      throw new InvalidFhirException("Invalid JSON element for number: " + json);
    }
  }

  private static void checkIsString(JsonElement json) throws InvalidFhirException {
    if (!(json.isJsonNull() || json.isJsonObject())
        && !(json.isJsonPrimitive() && json.getAsJsonPrimitive().isString())) {
      throw new InvalidFhirException("Invalid JSON element for string-like: " + json);
    }
  }
}
