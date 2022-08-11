//    Copyright 2020 Google Inc.
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

import com.google.common.io.Files;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.common.JsonFormat.Parser;
import com.google.fhir.protogen.FhirPackage;
import com.google.fhir.r4.core.ElementDefinition;
import com.google.fhir.r4.core.StructureDefinition;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link ProtoGenTransformerTest}. */
@RunWith(JUnit4.class)
public final class ProtoGenTransformerTest {

  ResourceValidator validator;
  ProtoGenTransformer.Builder builder;

  protected Runfiles runfiles;

  private String loadJson(String filename) throws IOException {
    File file =
        new File(
            runfiles.rlocation(
                "com_google_fhir/testdata/protogentransformer/" + filename + ".json"));
    return Files.asCharSource(file, StandardCharsets.UTF_8).read();
  }

  private static Parser makeParser(ProtoGenTransformer transformer) {
    return Parser.newBuilder().withProtoGenTransformer(transformer).build();
  }

  private static StructureDefinition parse(ProtoGenTransformer transformer, String json)
      throws InvalidFhirException {
    StructureDefinition.Builder def = StructureDefinition.newBuilder();
    return makeParser(transformer).merge(json, def).build();
  }

  @Before
  public void setUp() throws IOException {
    validator = new ResourceValidator();
    builder = new ProtoGenTransformer.Builder();
    runfiles = Runfiles.create();
  }

  @Test
  public void testIgnore() throws Exception {
    StructureDefinition def =
        parse(
            builder.ignore("fakeField", StructureDefinition.Mapping.newBuilder()).build(),
            loadJson("ignore"));

    assertThat(def.getMapping(0).getId().getValue()).isEqualTo("1234");
  }

  @Test
  public void testTransform() throws Exception {
    builder.addTransformer(
        "code",
        ElementDefinition.TypeRef.newBuilder(),
        (jsonElement, typeRef) ->
            typeRef.getCodeBuilder().setValue("prefix-" + jsonElement.getAsString()));

    StructureDefinition def = parse(builder.build(), loadJson("transform"));

    assertThat(def.getSnapshot().getElement(0).getType(0).getCode().getValue())
        .isEqualTo("prefix-original");
  }

  @Test
  public void testTransform_withPredicate() throws Exception {
    builder.addTransformer(
        "code",
        ElementDefinition.TypeRef.newBuilder(),
        code -> code.getAsString().equals("yes"),
        (jsonElement, typeRef) ->
            typeRef.getCodeBuilder().setValue("prefix-" + jsonElement.getAsString()));

    StructureDefinition def = parse(builder.build(), loadJson("transform_with_predicate"));
    ElementDefinition element = def.getSnapshot().getElement(0);

    assertThat(element.getType(0).getCode().getValue()).isEqualTo("no");
    assertThat(element.getType(1).getCode().getValue()).isEqualTo("prefix-yes");
    assertThat(element.getType(2).getCode().getValue()).isEqualTo("nope");
  }

  @Test
  public void testTransform_multiFieldTransformer() throws Exception {
    builder.addMultiFieldTransformer(
        ElementDefinition.newBuilder(),
        (json, elementDefinition) ->
            elementDefinition
                .getPathBuilder()
                .setValue(
                    json.getAsJsonPrimitive("pathPartA").getAsString()
                        + "-"
                        + json.getAsJsonPrimitive("pathPartB").getAsString()),
        "pathPartA",
        "pathPartB");

    StructureDefinition def = parse(builder.build(), loadJson("multi_field_transform"));
    ElementDefinition element = def.getSnapshot().getElement(0);

    assertThat(element.getPath().getValue()).isEqualTo("AAA-BBB");
  }

  @Test
  public void testStu3Transformer() throws Exception {
    // Test to ensure that the STU3 transformer is capable of loading the entire STU3 spec.
    FhirPackage fhirPackage = FhirPackage.load("spec/fhir_stu3_package.zip");

    assertThat(fhirPackage.structureDefinitions()).hasSize(585);
    assertThat(fhirPackage.codeSystems()).hasSize(941);
    assertThat(fhirPackage.valueSets()).hasSize(1154);
  }
}
