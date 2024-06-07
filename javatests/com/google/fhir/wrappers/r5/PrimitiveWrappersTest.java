//    Copyright 2024 Google Inc.
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

package com.google.fhir.wrappers.r5;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
import static com.google.fhir.common.ProtoUtils.findField;
import static java.nio.charset.StandardCharsets.UTF_8;
import static java.util.stream.Collectors.toSet;
import static org.junit.Assert.assertThrows;

import com.google.common.io.Files;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.common.AnnotationUtils;
import com.google.fhir.common.Extensions;
import com.google.fhir.common.FhirTypes;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.common.ProtoUtils;
import com.google.fhir.r5.testing.PrimitiveTestSuite;
import com.google.gson.JsonParser;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import java.io.File;
import java.io.IOException;
import java.time.ZoneId;
import java.util.Collection;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

@RunWith(Parameterized.class)
@SuppressWarnings("unchecked") // Reflection used to allow a single suite for all versions of FHIR
public final class PrimitiveWrappersTest {
  private static final ZoneId TIMEZONE = ZoneId.of("Australia/Sydney");

  private final Message primitiveType;
  private final Message suite;

  // Note that `suiteName` param is unused, only for naming the tests in a meaningful way.
  public PrimitiveWrappersTest(
      Message primitiveTestSuite, Descriptor primitiveType, String suiteName)
      throws IOException, ReflectiveOperationException {
    this.primitiveType =
        (Message)
            Class.forName("com." + primitiveType.getFullName())
                .getMethod("getDefaultInstance")
                .invoke(null);
    // Note that this throws an IOException if a testsuite is missing for any datatype, ensuring
    // total coverage.
    File file =
        new File(
            Runfiles.create()
                .rlocation(
                    "com_google_fhir/testdata/r5/primitives/"
                        + primitiveType.getName()
                        + ".txtpb"));
    Message.Builder builder = primitiveTestSuite.newBuilderForType();
    TextFormat.merge(Files.asCharSource(file, UTF_8).read(), builder);
    this.suite = builder.build();
  }

  @Parameters(name = "{2}")
  public static Collection<Object[]> getParameters() {
    return getParamsForVersion(
        com.google.fhir.r5.core.String.getDescriptor().getFile(),
        PrimitiveTestSuite.getDefaultInstance(),
        "R5");
  }

  private static Collection<Object[]> getParamsForVersion(
      FileDescriptor file, Message primitiveTestSuite, String versionString) {
    return file.getMessageTypes().stream()
        .filter(
            type ->
                AnnotationUtils.isPrimitiveType(type)
                    // TODO(b/178495903): Remove "ReferenceId" handling once ReferenceId is no
                    // longer (erroneously) marked as a primitive.
                    && !type.getName().equals("ReferenceId"))
        .map(
            descriptor ->
                new Object[] {
                  primitiveTestSuite, descriptor, versionString + ";" + descriptor.getName()
                })
        .collect(toSet());
  }

  private static String protoToString(Message message) throws InvalidFhirException {
    return PrimitiveWrappers.primitiveWrapperOf(message).toJson().toString();
  }

  private static Message stringToProto(String jsonString, Message.Builder builder)
      throws InvalidFhirException {
    return PrimitiveWrappers.parseAndWrap(JsonParser.parseString(jsonString), builder, TIMEZONE)
        .getWrapped();
  }

  @Test
  public void testValidPairs() throws Exception {
    List<Message> validPairs = (List<Message>) suite.getField(findField(suite, "valid_pairs"));
    assertWithMessage("Suite has no valid_pair cases").that(validPairs).isNotEmpty();
    for (Message pair : validPairs) {
      String jsonString = (String) pair.getField(findField(pair, "json_string"));
      Message message = getOneofValue((Message) pair.getField(findField(pair, "proto")));

      // Test print
      assertThat(protoToString(message)).isEqualTo(jsonString);

      // Test parse
      // TODO(b/178424920): Compare as protos once there is true multiversion support for Java.
      assertThat(stringToProto(jsonString, message.newBuilderForType()).toString())
          .isEqualTo(message.toString());

      // Test Validate - note that this throws an exception if the message is invalid.
      PrimitiveWrappers.validatePrimitive(message);

      if (!FhirTypes.isXhtml(message.getDescriptorForType())) {
        // Test that adding the PrimitiveHasNoValue extension to a message with value is invalid.
        // Note Xhtml cannot have extensions.
        Message.Builder withNoValue = message.toBuilder();
        Extensions.addPrimitiveHasNoValue(withNoValue);
        assertThrows(
            "Message with no-value extension should have failed validation:\n" + withNoValue,
            InvalidFhirException.class,
            () -> PrimitiveWrappers.validatePrimitive(withNoValue.build()));
      }
    }
  }

  @Test
  public void testInvalidJson() throws Exception {
    List<String> invalidJsonLines = (List<String>) suite.getField(findField(suite, "invalid_json"));
    assertWithMessage("Suite has no invalid_json cases").that(invalidJsonLines).isNotEmpty();
    for (String jsonString : invalidJsonLines) {

      // TODO(b/176651098): convert to only accepting InvalidFhirException
      assertThrows(
          "Should have failed: " + jsonString,
          Exception.class,
          () -> stringToProto(jsonString, primitiveType.newBuilderForType()));
    }
  }

  @Test
  public void testInvalidProtos() throws Exception {
    if (suite.hasField(findField(suite, "no_invalid_protos_reason"))) {
      return;
    }
    List<Message> invalidProtoCases =
        (List<Message>) suite.getField(findField(suite, "invalid_protos"));
    assertWithMessage("Suite has no invalid_protos cases").that(invalidProtoCases).isNotEmpty();
    for (Message union : invalidProtoCases) {
      Message message = getOneofValue(union);
      // TODO(b/176651098): convert to only accepting InvalidFhirException
      assertThrows(
          "Should have failed:\n" + message,
          Exception.class,
          () -> PrimitiveWrappers.validatePrimitive(message));
    }
  }

  @Test
  public void testNoValueBehavior_valid() throws Exception {
    if (FhirTypes.isXhtml(primitiveType.getDescriptorForType())) {
      // Xhtml cannot have extensions.  It is always considered to have a value.
      return;
    }

    // It's ok to have no value if there's another extension present.
    Message.Builder onlyExtensions = primitiveType.newBuilderForType();
    Extensions.addPrimitiveHasNoValue(onlyExtensions);
    Message.Builder arbitraryExtension =
        Extensions.makeExtensionWithUrl("arbitrary-url", onlyExtensions);
    Extensions.setExtensionValue(arbitraryExtension, "boolean", true);
    Extensions.addExtensionToMessage(arbitraryExtension.build(), onlyExtensions);

    PrimitiveWrappers.validatePrimitive(onlyExtensions);
  }

  @Test
  public void testNoValueBehavior_invalid_noOtherExtensions() throws Exception {
    if (FhirTypes.isXhtml(primitiveType.getDescriptorForType())) {
      // Xhtml cannot have extensions.  It is always considered to have a value.
      return;
    }

    // It's invalid to have no extensions (other than the no-value extension) and no value.
    Message.Builder onlyNoValueExtension = primitiveType.newBuilderForType();
    Extensions.addPrimitiveHasNoValue(onlyNoValueExtension);

    assertThrows(
        InvalidFhirException.class,
        () -> PrimitiveWrappers.validatePrimitive(onlyNoValueExtension));
  }

  private Message getOneofValue(Message primitiveUnionProto) {
    // Since all the fields in PrimitiveProtoUnion live in single oneof, only one field
    // can be set on the entire message.
    Message message =
        (Message) primitiveUnionProto.getAllFields().entrySet().iterator().next().getValue();
    assertWithMessage(
            "Found union type "
                + message.getDescriptorForType().getFullName()
                + " in test suite of type "
                + primitiveType.getDescriptorForType().getFullName())
        .that(
            ProtoUtils.areSameMessageType(
                message.getDescriptorForType(), primitiveType.getDescriptorForType()))
        .isTrue();
    return message;
  }
}
