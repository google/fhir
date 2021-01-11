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
import static java.nio.charset.StandardCharsets.UTF_8;
import static org.junit.Assert.assertThrows;

import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableList;
import com.google.common.io.Files;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import java.io.File;
import java.io.IOException;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

@RunWith(Parameterized.class)
public final class ResourceUtilsTest {
  private final Message bundleType;
  private final Message referenceType;

  private Runfiles runfiles;

  public ResourceUtilsTest(Message bundleType, Message referenceType) {
    this.bundleType = bundleType;
    this.referenceType = referenceType;
  }

  @Before
  public void setUp() throws IOException {
    runfiles = Runfiles.create();
  }

  @Parameterized.Parameters
  public static ImmutableCollection<Object[]> params() {
    return ImmutableList.of(
        new Object[] {
          com.google.fhir.r4.core.Bundle.getDefaultInstance(),
          com.google.fhir.r4.core.Reference.getDefaultInstance()
        },
        new Object[] {
          com.google.fhir.stu3.proto.Bundle.getDefaultInstance(),
          com.google.fhir.stu3.proto.Reference.getDefaultInstance()
        });
  }

  private static Message.Builder setStringField(
      Message.Builder builder, String field, String stringValue) {
    Message.Builder fieldBuilder = builder.getFieldBuilder(findField(builder, field));
    fieldBuilder.setField(findField(fieldBuilder, "value"), stringValue);
    return builder;
  }

  /** Read the specifed prototxt file from the testdata directory and parse it. */
  private void mergeText(String filename, Message.Builder builder) throws IOException {
    File file = new File(runfiles.rlocation("com_google_fhir/testdata/" + filename));
    TextFormat.merge(Files.asCharSource(file, UTF_8).read(), builder);
  }

  @Test
  public void testGetContainedResource_hasContained() {
    Message.Builder entryBuilder =
        bundleType.newBuilderForType().newBuilderForField(findField(bundleType, "entry"));
    Message.Builder containedResourceBuilder =
        entryBuilder.newBuilderForField(findField(entryBuilder, "resource"));
    Message.Builder conceptMap =
        containedResourceBuilder.getFieldBuilder(
            findField(containedResourceBuilder, "concept_map"));
    setStringField(conceptMap, "id", "my-concept-map");

    assertThat(ResourceUtils.getContainedResource(containedResourceBuilder.build()).get())
        .isEqualTo(conceptMap.build());
  }

  @Test
  public void testGetContainedResource_noContained() {
    Message.Builder entryBuilder =
        bundleType.newBuilderForType().newBuilderForField(findField(bundleType, "entry"));
    Message.Builder containedResourceBuilder =
        entryBuilder.newBuilderForField(findField(entryBuilder, "resource"));

    assertThat(ResourceUtils.getContainedResource(containedResourceBuilder.build()).isPresent())
        .isFalse();
  }

  @Test
  public void testGetContainedResource_notContainedThrows() {
    assertThrows(
        IllegalArgumentException.class,
        () -> ResourceUtils.getContainedResource(bundleType.getDefaultInstanceForType()));
  }

  @Test
  public void testSplitIfRelativeReference_referenceWithoutUriUnchanged() {
    Message.Builder reference = referenceType.newBuilderForType();
    setStringField(reference, "fragment", "arbitrary-fragment");

    Message expected = reference.build();
    ResourceUtils.splitIfRelativeReference(reference);
    assertThat(reference.build()).isEqualTo(expected);
  }

  @Test
  public void testSplitIfRelativeReference_fragmentMovedToFragmentField() {
    Message.Builder reference = referenceType.newBuilderForType();
    setStringField(reference, "uri", "#someFragment");

    Message.Builder expected = referenceType.newBuilderForType();
    setStringField(expected, "fragment", "someFragment");

    ResourceUtils.splitIfRelativeReference(reference);
    assertThat(reference.build()).isEqualTo(expected.build());
  }

  @Test
  public void testSplitIfRelativeReference_relativeReferenceSlotted_withoutHistory() {
    Message.Builder reference = referenceType.newBuilderForType();
    setStringField(reference, "uri", "ActivityDefinition/ABCD");

    Message.Builder expected = referenceType.newBuilderForType();
    setStringField(expected, "activity_definition_id", "ABCD");

    ResourceUtils.splitIfRelativeReference(reference);
    assertThat(reference.build()).isEqualTo(expected.build());
  }

  @Test
  public void testSplitIfRelativeReference_relativeReferenceSlotted_withHistory() {
    Message.Builder reference = referenceType.newBuilderForType();
    setStringField(reference, "uri", "ActivityDefinition/ABCD/_history/some_version_id");

    Message.Builder expected = referenceType.newBuilderForType();
    Message.Builder referenceId =
        expected.getFieldBuilder(findField(expected, "activity_definition_id"));
    referenceId.setField(findField(referenceId, "value"), "ABCD");
    setStringField(referenceId, "history", "some_version_id");

    ResourceUtils.splitIfRelativeReference(reference);
    assertThat(reference.build()).isEqualTo(expected.build());
  }

  @Test
  public void testResolveBundleReferences() throws IOException {
    Message.Builder bundle = bundleType.newBuilderForType();
    mergeText("resource_utils-resolveBundleReferences-input.prototxt", bundle);

    ResourceUtils.resolveBundleReferences(bundle);

    Message.Builder expected = bundleType.newBuilderForType();
    mergeText("resource_utils-resolveBundleReferences-output.prototxt", expected);

    assertThat(bundle.build()).isEqualTo(expected.build());
  }
}
