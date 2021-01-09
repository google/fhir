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

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.extensions.proto.ProtoTruth.assertThat;
import static com.google.fhir.common.ProtoUtils.findField;
import static org.junit.Assert.assertThrows;

import com.google.fhir.testdata.GenericMessage;
import com.google.fhir.testdata.GenericMessage.SubMessage;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

// TODO: Add test coverage for older functions.
@RunWith(JUnit4.class)
public final class ProtoUtilsTest {

  private static final FieldDescriptor SINGULAR_PRIMITIVE =
      GenericMessage.getDescriptor().findFieldByName("singular_primitive");
  private static final FieldDescriptor REPEATED_PRIMITIVE =
      GenericMessage.getDescriptor().findFieldByName("repeated_primitive");

  private static final FieldDescriptor SINGULAR_MESSAGE_FIELD =
      GenericMessage.getDescriptor().findFieldByName("singular_message");
  private static final FieldDescriptor REPEATED_MESSAGE_FIELD =
      GenericMessage.getDescriptor().findFieldByName("repeated_message");

  @Test
  public void testFieldSize_singular() {
    assertThat(ProtoUtils.fieldSize(GenericMessage.newBuilder(), SINGULAR_PRIMITIVE)).isEqualTo(0);

    assertThat(
            ProtoUtils.fieldSize(
                GenericMessage.newBuilder().setSingularPrimitive("foo"), SINGULAR_PRIMITIVE))
        .isEqualTo(1);
  }

  @Test
  public void testFieldSize_repeated() {
    assertThat(ProtoUtils.fieldSize(GenericMessage.newBuilder(), REPEATED_PRIMITIVE)).isEqualTo(0);

    assertThat(
            ProtoUtils.fieldSize(
                GenericMessage.newBuilder().addRepeatedPrimitive("foo"), REPEATED_PRIMITIVE))
        .isEqualTo(1);

    assertThat(
            ProtoUtils.fieldSize(
                GenericMessage.newBuilder().addRepeatedPrimitive("foo").addRepeatedPrimitive("bar"),
                REPEATED_PRIMITIVE))
        .isEqualTo(2);
  }

  @Test
  public void testFieldIsSet_singular() {
    assertThat(ProtoUtils.fieldIsSet(GenericMessage.newBuilder(), SINGULAR_PRIMITIVE)).isFalse();

    assertThat(
            ProtoUtils.fieldIsSet(
                GenericMessage.newBuilder().setSingularPrimitive("foo"), SINGULAR_PRIMITIVE))
        .isTrue();
  }

  @Test
  public void testFieldIsSet_repeated() {
    assertThat(ProtoUtils.fieldIsSet(GenericMessage.newBuilder(), REPEATED_PRIMITIVE)).isFalse();

    assertThat(
            ProtoUtils.fieldIsSet(
                GenericMessage.newBuilder().addRepeatedPrimitive("foo"), REPEATED_PRIMITIVE))
        .isTrue();
  }

  @Test
  public void testFindField_descriptor_success() {
    Descriptor descriptor = GenericMessage.getDescriptor();
    assertThat(findField(descriptor, "singular_primitive"))
        .isEqualTo(descriptor.findFieldByName("singular_primitive"));
  }

  @Test
  public void testFindField_message_success() {
    Descriptor descriptor = GenericMessage.getDescriptor();
    assertThat(findField(GenericMessage.getDefaultInstance(), "singular_primitive"))
        .isEqualTo(descriptor.findFieldByName("singular_primitive"));
  }

  @Test
  public void testFindField_descriptor_failure() {
    Descriptor descriptor = GenericMessage.getDescriptor();
    assertThrows(IllegalArgumentException.class, () -> findField(descriptor, "wizbang"));
  }

  @Test
  public void testFindField_message_failure() {
    assertThrows(
        IllegalArgumentException.class,
        () -> findField(GenericMessage.getDefaultInstance(), "wizbang"));
  }

  @Test
  public void testForEachInstance_repeated() {
    GenericMessage builder =
        GenericMessage.newBuilder()
            .addRepeatedMessage(SubMessage.newBuilder().setValue("foo"))
            .addRepeatedMessage(SubMessage.newBuilder().setValue("bar"))
            .build();

    List<String> values = new ArrayList<>();
    List<Integer> indexes = new ArrayList<>();

    ProtoUtils.<SubMessage>forEachInstance(
        builder,
        REPEATED_MESSAGE_FIELD,
        (submessage, index) -> {
          values.add(submessage.getValue());
          indexes.add(index);
        });

    assertThat(values).containsExactly("foo", "bar").inOrder();
    assertThat(indexes).containsExactly(0, 1).inOrder();
  }

  @Test
  public void testForEachInstance_singular() {
    GenericMessage builder =
        GenericMessage.newBuilder()
            .setSingularMessage(SubMessage.newBuilder().setValue("foo"))
            .build();

    List<String> values = new ArrayList<>();
    List<Integer> indexes = new ArrayList<>();

    ProtoUtils.<SubMessage>forEachInstance(
        builder,
        SINGULAR_MESSAGE_FIELD,
        (submessage, index) -> {
          values.add(submessage.getValue());
          indexes.add(index);
        });

    assertThat(values).containsExactly("foo");
    assertThat(indexes).containsExactly(0);
  }

  @Test
  public void testForEachBuilder_repeated() {
    GenericMessage.Builder builder =
        GenericMessage.newBuilder()
            .addRepeatedMessage(SubMessage.newBuilder().setValue("foo"))
            .addRepeatedMessage(SubMessage.newBuilder().setValue("bar"));

    List<Integer> indexes = new ArrayList<>();
    ProtoUtils.<SubMessage.Builder>forEachBuilder(
        builder,
        REPEATED_MESSAGE_FIELD,
        (subbuilder, index) -> {
          subbuilder.setValue(subbuilder.getValue() + "-modified");
          indexes.add(index);
        });

    GenericMessage expected =
        GenericMessage.newBuilder()
            .addRepeatedMessage(SubMessage.newBuilder().setValue("foo-modified"))
            .addRepeatedMessage(SubMessage.newBuilder().setValue("bar-modified"))
            .build();

    assertThat(builder.build()).isEqualTo(expected);
    assertThat(indexes).containsExactly(0, 1).inOrder();
  }

  @Test
  public void testForEachBuilder_singular() {
    GenericMessage.Builder builder =
        GenericMessage.newBuilder().setSingularMessage(SubMessage.newBuilder().setValue("foo"));

    List<Integer> indexes = new ArrayList<>();
    ProtoUtils.<SubMessage.Builder>forEachBuilder(
        builder,
        SINGULAR_MESSAGE_FIELD,
        (subbuilder, index) -> {
          subbuilder.setValue(subbuilder.getValue() + "-modified");
          indexes.add(index);
        });

    GenericMessage expected =
        GenericMessage.newBuilder()
            .setSingularMessage(SubMessage.newBuilder().setValue("foo-modified"))
            .build();

    assertThat(builder.build()).isEqualTo(expected);
    assertThat(indexes).containsExactly(0);
  }

  @Test
  public void testGetOrAddBuilder_singular() {
    GenericMessage.Builder builder = GenericMessage.newBuilder();
    SubMessage.Builder subBuilder =
        (SubMessage.Builder) ProtoUtils.getOrAddBuilder(builder, SINGULAR_MESSAGE_FIELD);
    subBuilder.setValue("foo");

    GenericMessage expected =
        GenericMessage.newBuilder()
            .setSingularMessage(SubMessage.newBuilder().setValue("foo"))
            .build();

    assertThat(builder.build()).isEqualTo(expected);
  }

  @Test
  public void testGetOrAddBuilder_repeated() {
    GenericMessage.Builder builder = GenericMessage.newBuilder();
    ((SubMessage.Builder) ProtoUtils.getOrAddBuilder(builder, REPEATED_MESSAGE_FIELD))
        .setValue("foo");
    ((SubMessage.Builder) ProtoUtils.getOrAddBuilder(builder, REPEATED_MESSAGE_FIELD))
        .setValue("bar");

    GenericMessage expected =
        GenericMessage.newBuilder()
            .addRepeatedMessage(SubMessage.newBuilder().setValue("foo"))
            .addRepeatedMessage(SubMessage.newBuilder().setValue("bar"))
            .build();

    assertThat(builder.build()).isEqualTo(expected);
  }
}
