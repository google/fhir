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

import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableList;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import com.google.protobuf.TextFormat.ParseException;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

@RunWith(Parameterized.class)
public final class ExtensionsTest {
  private final Message extensionType;
  private final Message patientType;

  public ExtensionsTest(Message extensionType, Message patientType) {
    this.extensionType = extensionType;
    this.patientType = patientType;
  }

  @Parameterized.Parameters
  public static ImmutableCollection<Object[]> params() {
    return ImmutableList.of(
        new Object[] {
          com.google.fhir.r4.core.Extension.getDefaultInstance(),
          com.google.fhir.r4.core.Patient.getDefaultInstance()
        },
        new Object[] {
          com.google.fhir.stu3.proto.Extension.getDefaultInstance(),
          com.google.fhir.stu3.proto.Patient.getDefaultInstance()
        });
  }

  public Message makeComplexExtension() throws ParseException {
    Message.Builder builder = extensionType.newBuilderForType();

    TextFormat.merge(
        ""
            + "url { value: \"myUrl\" }"
            + "extension { "
            + "  url { value: \"fieldOne\" } "
            + "  value { string_value { value: \"myValue\" } }"
            + "}"
            + "extension { "
            + "  url { value: \"fieldTwo\" } "
            + "  value { boolean { value: true } }"
            + "}",
        builder);

    return builder.build();
  }

  private Message makeExtension(String textformat) throws ParseException {
    Message.Builder builder = extensionType.newBuilderForType();
    TextFormat.merge(textformat, builder);
    return builder.build();
  }

  @Test
  public void testAddExtensionToMessage_simpleExtension() throws Exception {
    Message extension =
        makeExtension("url { value: \"myUrl\" } value { string_value { value: \"myValue\" } }");

    Message.Builder patient = patientType.newBuilderForType();
    Extensions.addExtensionToMessage(extension, patient);

    List<Message> extensions = Extensions.getExtensions(patient);
    assertThat(extensions).containsExactly(extension);
  }

  @Test
  public void testAddExtensionToMessage_complexExtension() throws Exception {
    Message extension = makeComplexExtension();

    Message.Builder patient = patientType.newBuilderForType();
    Extensions.addExtensionToMessage(extension, patient);

    List<Message> extensions = Extensions.getExtensions(patient);
    assertThat(extensions).containsExactly(extension);
  }

  @Test
  public void testGetExtensionUrl() throws ParseException {
    Message extension =
        makeExtension("url { value: \"myUrl\" } value { string_value { value: \"myValue\" } }");

    assertThat(Extensions.getExtensionUrl(extension)).isEqualTo("myUrl");
  }

  @Test
  public void testSetExtensionValue() throws Exception {
    Message.Builder stringExtension = extensionType.newBuilderForType();
    Extensions.setExtensionValue(stringExtension, "string_value", "myValue");
    assertThat(stringExtension.build())
        .isEqualTo(makeExtension("value { string_value { value: \"myValue\" } }"));

    Message.Builder integerExtension = extensionType.newBuilderForType();
    Extensions.setExtensionValue(integerExtension, "integer", 5);
    assertThat(integerExtension.build()).isEqualTo(makeExtension("value { integer { value: 5 } }"));

    Message.Builder booleanExtension = extensionType.newBuilderForType();
    Extensions.setExtensionValue(booleanExtension, "boolean", true);
    assertThat(booleanExtension.build())
        .isEqualTo(makeExtension("value { boolean { value: true } }"));
  }

  @Test
  public void testGetExtensionValue() throws Exception {
    Message stringExtension =
        makeExtension("url { value: \"myUrl\" } value { string_value { value: \"myValue\" } }");
    assertThat(Extensions.getExtensionValue(stringExtension, "string_value")).isEqualTo("myValue");

    Message booleanExtension =
        makeExtension("url { value: \"myUrl\" } value { boolean { value: true } }");
    assertThat(Extensions.getExtensionValue(booleanExtension, "boolean")).isEqualTo(true);

    Message integerExtension =
        makeExtension("url { value: \"myUrl\" } value { integer { value: 5 } }");
    assertThat(Extensions.getExtensionValue(integerExtension, "integer")).isEqualTo(5);
  }

  @Test
  public void testGetExtensions() throws Exception {
    Message.Builder patientBuilder = patientType.newBuilderForType();
    TextFormat.merge(
        ""
            + "extension { "
            + "  url { value: \"urlOne\" }"
            + "  value { string_value { value: \"myValue\" } }"
            + "}"
            + "extension { "
            + "  url { value: \"urlTwo\" }"
            + "  value { integer { value: 5 } }"
            + " } ",
        patientBuilder);

    List<Message> extensions = Extensions.getExtensions(patientBuilder);
    assertThat(extensions.get(0))
        .isEqualTo(
            makeExtension(
                "url { value: \"urlOne\" } value { string_value { value: \"myValue\" } }"));
    assertThat(extensions.get(1))
        .isEqualTo(makeExtension("url { value: \"urlTwo\" } value { integer { value: 5 } }"));
  }

  @Test
  public void testGetExtensionsWithUrl() throws Exception {
    Message.Builder patientBuilder = patientType.newBuilderForType();
    TextFormat.merge(
        ""
            + "extension { "
            + "  url { value: \"one\" } value { string_value { value: \"first_value\" } }"
            + "}"
            + "extension { "
            + "  url { value: \"two\" } value { integer { value: 5 } }"
            + "}"
            + "extension { "
            + "  url { value: \"one\" } value { string_value { value: \"second_value\" } }"
            + "}",
        patientBuilder);

    List<Message> extensions = Extensions.getExtensionsWithUrl("one", patientBuilder);
    assertThat(extensions.get(0))
        .isEqualTo(
            makeExtension(
                "url { value: \"one\" } value { string_value { value: \"first_value\" } }"));
    assertThat(extensions.get(1))
        .isEqualTo(
            makeExtension(
                "url { value: \"one\" } value { string_value { value: \"second_value\" } }"));
  }

  @Test
  public void testForEachExtension() throws Exception {
    Message.Builder patientBuilder = patientType.newBuilderForType();
    TextFormat.merge(
        ""
            + "extension { "
            + "  url { value: \"one\" } value { string_value { value: \"first_value\" } }"
            + "}"
            + "extension { "
            + "  url { value: \"two\" } value { integer { value: 5 } }"
            + "}"
            + "extension { "
            + "  url { value: \"one\" } value { string_value { value: \"second_value\" } }"
            + "}",
        patientBuilder);

    List<String> urls = new ArrayList<>();
    Extensions.forEachExtension(
        patientBuilder, extension -> urls.add(Extensions.getExtensionUrl(extension)));

    assertThat(urls).containsExactly("one", "two", "one").inOrder();
  }

  @Test
  public void testMakeExtensionWithUrl() throws Exception {
    Message.Builder patientBuilder = patientType.newBuilderForType();
    Message.Builder extension = Extensions.makeExtensionWithUrl("my-url", patientBuilder);

    assertThat(Extensions.getExtensionUrl(extension)).isEqualTo("my-url");
    // Assert that makeExtensionWithUrl doesn't modify the base
    assertThat(patientBuilder.build()).isEqualTo(patientBuilder.getDefaultInstanceForType());
    // Assert that the produced extension is valid to be added to patientBuilder
    Extensions.addExtensionToMessage(extension.build(), patientBuilder);
  }

  @Test
  public void testAddPrimitiveHasNoValue() throws Exception {
    Message.Builder patientBuilder = patientType.newBuilderForType();
    Extensions.addPrimitiveHasNoValue(patientBuilder);

    List<Message> phnvExtensions =
        Extensions.getExtensionsWithUrl(Extensions.PRIMITIVE_HAS_NO_VALUE_URL, patientBuilder);
    assertThat(phnvExtensions).hasSize(1);
    assertThat((boolean) Extensions.getExtensionValue(phnvExtensions.get(0), "boolean")).isTrue();
  }
}
