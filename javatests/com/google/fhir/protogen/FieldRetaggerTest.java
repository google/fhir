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

package com.google.fhir.protogen;

import static com.google.common.truth.extensions.proto.ProtoTruth.assertThat;
import static org.junit.Assert.assertThrows;

import com.google.common.collect.ImmutableMap;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.ProtoGeneratorAnnotations;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.MessageOptions;
import java.util.Map;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class FieldRetaggerTest {

  private DescriptorProto.Builder makeDescriptor(
      String messageName, Map<String, Integer> fieldMap) {
    DescriptorProto.Builder builder = DescriptorProto.newBuilder().setName(messageName);
    fieldMap.forEach((name, number) -> builder.addFieldBuilder().setName(name).setNumber(number));
    return builder;
  }

  // TODO(b/192419079): For historical reasons, ProtoGenerator generates reserved fields using
  // normal fields, with the "reservedReason" annotation.  These should be updated to just use
  // ReservedRanges.
  private DescriptorProto.Builder addReservedField(DescriptorProto.Builder builder, int number) {
    builder
        .addFieldBuilder()
        .setNumber(number)
        .getOptionsBuilder()
        .setExtension(
            ProtoGeneratorAnnotations.reservedReason,
            "Field " + number + " reserved to prevent reuse of field that was previously deleted.");
    return builder;
  }

  private DescriptorProto.Builder addReservedFieldViaRange(
      DescriptorProto.Builder builder, int number) {
    builder.addReservedRangeBuilder().setStart(number).setEnd(number + 1);
    return builder;
  }

  @Test
  public void retagMessage_identicalMessages() {
    DescriptorProto newMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f1", 1, "m1f2", 2, "m1f3", 3))
            .addNestedType(makeDescriptor("m1s1", ImmutableMap.of("s1f1", 1, "s1f2", 2, "s1f3", 3)))
            .build();

    // Note that in goldens, the order of fields is different than that in newFile.  This should
    // have no effect.

    DescriptorProto goldenMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f1", 1, "m1f3", 3, "m1f2", 2))
            .addNestedType(makeDescriptor("m1s1", ImmutableMap.of("s1f2", 2, "s1f3", 3, "s1f1", 1)))
            .build();

    DescriptorProto expectedMessage = newMessage.toBuilder().build();

    assertThat(FieldRetagger.retagMessage(newMessage, goldenMessage)).isEqualTo(expectedMessage);
  }

  @Test
  public void retagMessage_noMismatch_newFieldsAtEnd() {
    DescriptorProto newMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f1", 1, "m1f2", 2, "m1f3", 3))
            .addNestedType(makeDescriptor("m1s1", ImmutableMap.of("s1f1", 1, "s1f2", 2, "s1f3", 3)))
            .build();

    DescriptorProto goldenMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f1", 1, "m1f2", 2))
            .addNestedType(makeDescriptor("m1s1", ImmutableMap.of("s1f1", 1, "s1f2", 2)))
            .build();

    DescriptorProto expectedMessage = newMessage.toBuilder().build();

    assertThat(FieldRetagger.retagMessage(newMessage, goldenMessage)).isEqualTo(expectedMessage);
  }

  @Test
  public void retagMessage_noMismatch_newFieldsNotAtEndMovedToEnd() {
    // In these examples, the "f5" and "f6" fields have tag numbers 2 and 3, but no corresponding
    // field in the goldens.  These should be moved tag numbers greater than any in the golden
    // message. The fields are not reordered.

    DescriptorProto newMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f1", 1, "m1f5", 2, "m1f6", 3, "m1f4", 4))
            .addNestedType(
                makeDescriptor("m1s1", ImmutableMap.of("s1f1", 1, "s1f5", 2, "s1f6", 3, "s1f4", 4)))
            .build();

    DescriptorProto goldenMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f1", 1, "m1f4", 4))
            .addNestedType(makeDescriptor("m1s1", ImmutableMap.of("s1f1", 1, "s1f4", 4)))
            .build();

    DescriptorProto expectedMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f1", 1, "m1f5", 5, "m1f6", 6, "m1f4", 4))
            .addNestedType(
                makeDescriptor("m1s1", ImmutableMap.of("s1f1", 1, "s1f5", 5, "s1f6", 6, "s1f4", 4)))
            .build();

    assertThat(FieldRetagger.retagMessage(newMessage, goldenMessage)).isEqualTo(expectedMessage);
  }

  @Test
  public void retagMessage_mismatchFound_noNewFields() {
    // In this example, the "f2" and "f3" fields have been reversed in order and tag number.
    // They should keep the ordering of the new message, but renumber to use the field numbers of
    // the golden.

    DescriptorProto newMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f1", 1, "m1f3", 2, "m1f2", 3))
            .addNestedType(makeDescriptor("m1s1", ImmutableMap.of("s1f1", 1, "s1f3", 2, "s1f2", 3)))
            .build();

    DescriptorProto goldenMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f1", 1, "m1f2", 2, "m1f3", 3))
            .addNestedType(makeDescriptor("m1s1", ImmutableMap.of("s1f1", 1, "s1f2", 2, "s1f3", 3)))
            .build();

    DescriptorProto expectedMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f1", 1, "m1f3", 3, "m1f2", 2))
            .addNestedType(makeDescriptor("m1s1", ImmutableMap.of("s1f1", 1, "s1f3", 3, "s1f2", 2)))
            .build();

    assertThat(FieldRetagger.retagMessage(newMessage, goldenMessage)).isEqualTo(expectedMessage);
  }

  @Test
  public void retagMessage_mismatchFound_newFields() {
    // In this example, the "f5" has been added to the new message using a tag number (3) that is
    // not in use, but is not greater than any already in use, and "f2" and "f4" fields have
    // incorrect tag numbers. The ordering of fields should be unchanged, but "f2" and "f4" should
    // use the same tag numbers as in the golden, and "f5" will be assigned a new tag number.

    DescriptorProto newMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f1", 1, "m1f4", 2, "m1f5", 3, "m1f2", 4))
            .addNestedType(
                makeDescriptor("m1s1", ImmutableMap.of("s1f1", 1, "s1f4", 2, "s1f5", 3, "s1f2", 4)))
            .build();

    DescriptorProto goldenMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f1", 1, "m1f2", 2, "m1f4", 4))
            .addNestedType(makeDescriptor("m1s1", ImmutableMap.of("s1f1", 1, "s1f2", 2, "s1f4", 4)))
            .build();

    DescriptorProto expectedMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f1", 1, "m1f4", 4, "m1f5", 5, "m1f2", 2))
            .addNestedType(
                makeDescriptor("m1s1", ImmutableMap.of("s1f1", 1, "s1f4", 4, "s1f5", 5, "s1f2", 2)))
            .build();

    assertThat(FieldRetagger.retagMessage(newMessage, goldenMessage)).isEqualTo(expectedMessage);
  }

  @Test
  public void retagMessage_lastFieldDeleted_reservedFieldAdded() {
    // In this example, the "f1" and "f3" fields have been removed.  Since "f3" fields are the
    // highest-numbered fields present, they should be replaced with a reserved number.

    DescriptorProto newMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f2", 2))
            .addNestedType(makeDescriptor("m1s1", ImmutableMap.of("s1f2", 2)))
            .build();

    DescriptorProto goldenMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f1", 1, "m1f2", 2, "m1f3", 3))
            .addNestedType(makeDescriptor("m1s1", ImmutableMap.of("s1f1", 1, "s1f2", 2, "s1f3", 3)))
            .build();

    DescriptorProto expectedMessage =
        addReservedField(makeDescriptor("m1", ImmutableMap.of("m1f2", 2)), 3)
            .addNestedType(addReservedField(makeDescriptor("m1s1", ImmutableMap.of("s1f2", 2)), 3))
            .build();

    assertThat(FieldRetagger.retagMessage(newMessage, goldenMessage)).isEqualTo(expectedMessage);
  }

  @Test
  public void retagMessage_noChanges_reservedFieldPreserved() {
    // In this example, no field changes occur, but the golden contains a high reserved range.
    // That should be persisted in the new file.

    DescriptorProto newMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f2", 2))
            .addNestedType(makeDescriptor("m1s1", ImmutableMap.of("s1f2", 2)))
            .build();

    DescriptorProto goldenMessage =
        addReservedFieldViaRange(makeDescriptor("m1", ImmutableMap.of("m1f2", 2)), 3)
            .addNestedType(
                addReservedFieldViaRange(makeDescriptor("m1s1", ImmutableMap.of("s1f2", 2)), 3))
            .build();

    DescriptorProto expectedMessage =
        addReservedField(makeDescriptor("m1", ImmutableMap.of("m1f2", 2)), 3)
            .addNestedType(addReservedField(makeDescriptor("m1s1", ImmutableMap.of("s1f2", 2)), 3))
            .build();

    assertThat(FieldRetagger.retagMessage(newMessage, goldenMessage)).isEqualTo(expectedMessage);
  }

  @Test
  public void retagMessage_newFieldAndReservedRange_fieldAddedAboveReservedRange() {
    // In this example, no field changes occur, but the golden contains a high reserved range.
    // That should be persisted in the new file.

    DescriptorProto newMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f2", 2, "m1f4", 3))
            .addNestedType(makeDescriptor("m1s1", ImmutableMap.of("s1f2", 2, "s1f4", 3)))
            .build();

    DescriptorProto goldenMessage =
        addReservedFieldViaRange(makeDescriptor("m1", ImmutableMap.of("m1f2", 2)), 3)
            .addNestedType(
                addReservedFieldViaRange(makeDescriptor("m1s1", ImmutableMap.of("s1f2", 2)), 3))
            .build();

    DescriptorProto expectedMessage =
        makeDescriptor("m1", ImmutableMap.of("m1f2", 2, "m1f4", 4))
            .addNestedType(makeDescriptor("m1s1", ImmutableMap.of("s1f2", 2, "s1f4", 4)))
            .build();

    assertThat(FieldRetagger.retagMessage(newMessage, goldenMessage)).isEqualTo(expectedMessage);
  }

  @Test
  public void retagMessage_matchingReservedField_preservedInResult() {
    DescriptorProto newMessage =
        addReservedField(makeDescriptor("m1", ImmutableMap.of("m1f2", 2, "m1f4", 3)), 1).build();

    DescriptorProto goldenMessage =
        addReservedFieldViaRange(makeDescriptor("m1", ImmutableMap.of("m1f2", 2)), 1).build();

    DescriptorProto expectedMessage = newMessage.toBuilder().build();

    assertThat(FieldRetagger.retagMessage(newMessage, goldenMessage)).isEqualTo(expectedMessage);
  }

  @Test
  public void retagMessage_mismatchingReserved_throwsException() {
    DescriptorProto newMessage =
        addReservedField(makeDescriptor("m1", ImmutableMap.of("m1f2", 2, "m1f4", 3)), 1).build();

    DescriptorProto goldenMessage = makeDescriptor("m1", ImmutableMap.of("m1f2", 2)).build();

    assertThrows(
        IllegalStateException.class, () -> FieldRetagger.retagMessage(newMessage, goldenMessage));
  }

  @Test
  public void retagMessage_changedPrimitiveType_moved() {
    DescriptorProto newMessage =
        DescriptorProto.newBuilder()
            .setName("Top")
            .addField(
                FieldDescriptorProto.newBuilder()
                    .setName("f1")
                    .setNumber(1)
                    .setType(FieldDescriptorProto.Type.TYPE_STRING))
            .build();

    DescriptorProto goldenMessage =
        DescriptorProto.newBuilder()
            .setName("Top")
            .addField(
                FieldDescriptorProto.newBuilder()
                    .setName("f1")
                    .setNumber(1)
                    .setType(FieldDescriptorProto.Type.TYPE_INT32))
            .build();

    DescriptorProto.Builder expectedMessage = newMessage.toBuilder();
    expectedMessage.getFieldBuilder(0).setNumber(2);

    assertThat(FieldRetagger.retagMessage(newMessage, goldenMessage))
        .isEqualTo(expectedMessage.build());
  }

  @Test
  public void retagMessage_changedMessageType_moved() {
    DescriptorProto newMessage =
        DescriptorProto.newBuilder()
            .setName("Top")
            .addField(
                FieldDescriptorProto.newBuilder()
                    .setName("f1")
                    .setNumber(1)
                    .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                    .setTypeName("Wibble"))
            .addField(
                FieldDescriptorProto.newBuilder()
                    .setName("f2")
                    .setNumber(2)
                    .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                    .setTypeName("Blat"))
            .build();

    DescriptorProto goldenMessage =
        DescriptorProto.newBuilder()
            .setName("Top")
            .addField(
                FieldDescriptorProto.newBuilder()
                    .setName("f1")
                    .setNumber(1)
                    .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                    .setTypeName("Wobble"))
            .addField(
                FieldDescriptorProto.newBuilder()
                    .setName("f2")
                    .setNumber(2)
                    .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                    .setTypeName("Blat"))
            .build();

    DescriptorProto.Builder expectedMessage = newMessage.toBuilder();
    expectedMessage.getFieldBuilder(0).setNumber(3);

    assertThat(FieldRetagger.retagMessage(newMessage, goldenMessage))
        .isEqualTo(expectedMessage.build());
  }

  @Test
  public void retagMessage_changedValueSetBinding_moved() {
    DescriptorProto newMessage =
        DescriptorProto.newBuilder()
            .setName("Top")
            .addField(
                FieldDescriptorProto.newBuilder()
                    .setName("status")
                    .setNumber(1)
                    .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                    .setTypeName("package.Top.StatusCode"))
            .addNestedType(
                DescriptorProto.newBuilder()
                    .setName("StatusCode")
                    .setOptions(
                        MessageOptions.newBuilder()
                            .setExtension(Annotations.fhirValuesetUrl, "url-1"))
                    .build())
            .build();

    DescriptorProto goldenMessage =
        DescriptorProto.newBuilder()
            .setName("Top")
            .addField(
                FieldDescriptorProto.newBuilder()
                    .setName("status")
                    .setNumber(1)
                    .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                    .setTypeName("package.Top.StatusCode"))
            .addNestedType(
                DescriptorProto.newBuilder()
                    .setName("StatusCode")
                    .setOptions(
                        MessageOptions.newBuilder()
                            .setExtension(Annotations.fhirValuesetUrl, "url-2"))
                    .build())
            .build();

    DescriptorProto.Builder expectedMessage = newMessage.toBuilder();
    expectedMessage.getFieldBuilder(0).setNumber(2);

    assertThat(FieldRetagger.retagMessage(newMessage, goldenMessage))
        .isEqualTo(expectedMessage.build());
  }

  @Test
  public void retagMessage_changedCardinality_moved() {
    DescriptorProto newMessage =
        DescriptorProto.newBuilder()
            .setName("Top")
            .addField(
                FieldDescriptorProto.newBuilder()
                    .setName("status")
                    .setNumber(1)
                    .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                    .setTypeName("package.Faaz")
                    .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL))
            .build();

    DescriptorProto goldenMessage =
        DescriptorProto.newBuilder()
            .setName("Top")
            .addField(
                FieldDescriptorProto.newBuilder()
                    .setName("status")
                    .setNumber(1)
                    .setLabel(FieldDescriptorProto.Label.LABEL_REPEATED)
                    .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                    .setTypeName("package.Faaz"))
            .build();

    DescriptorProto.Builder expectedMessage = newMessage.toBuilder();
    expectedMessage.getFieldBuilder(0).setNumber(2);

    assertThat(FieldRetagger.retagMessage(newMessage, goldenMessage))
        .isEqualTo(expectedMessage.build());
  }
}
