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

package com.google.fhir.dstu2;

import static com.google.common.truth.Truth.assertThat;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link StructureDefinitionTransformer}. */
@RunWith(JUnit4.class)
public class StructureDefinitionTransformerTest {

  private void runTransformTest(JsonObject input, JsonObject expected) throws Exception {
    JsonObject output = StructureDefinitionTransformer.transformDstu2ToStu3(input);
    assertThat(output).isEqualTo(expected);
  }

  private JsonObject getElementsWrapper(JsonObject element) {
    JsonArray elements = new JsonArray();
    if (element != null) {
      elements.add(element);
    }
    JsonObject elementsWrapper = new JsonObject();
    elementsWrapper.add("element", elements);
    return elementsWrapper;
  }

  private JsonObject getCanonicalInput(String id, String kind) {
    JsonObject input = new JsonObject();
    input.addProperty("resourceType", "StructureDefinition");
    input.addProperty("id", id);
    input.addProperty("url", "url");
    input.addProperty("date", "2018-01-01");
    input.addProperty("kind", kind);
    input.add("snapshot", getElementsWrapper(null));
    input.add("differential", getElementsWrapper(null));
    return input;
  }

  private JsonObject getCanonicalOutput(String id, String kind) {
    JsonObject output = getCanonicalInput(id, kind);
    output.addProperty("type", id);
    return output;
  }

  /** Test required fields in StructureDefinition. */
  @Test
  public void requiredFields() throws Exception {
    JsonObject input = getCanonicalInput("Resource", "Resource");
    JsonObject output = getCanonicalOutput("Resource", "Resource");
    runTransformTest(input, output);
  }

  /** Test unwanted FHIR comments in StructureDefinition. */
  @Test
  public void dropFhirComments() throws Exception {
    JsonObject input = getCanonicalInput("Resource", "Resource");
    JsonArray fhirComments = new JsonArray();
    fhirComments.add("comment1");
    fhirComments.add("comment2");
    JsonObject text = new JsonObject();
    text.addProperty("sec", "sec1");
    text.add("fhir_comments", fhirComments);
    input.add("text", text);
    input.add("fhir_comments", fhirComments);

    JsonObject output = getCanonicalOutput("Resource", "Resource");
    JsonObject newText = new JsonObject();
    newText.addProperty("sec", "sec1");
    output.add("text", text);

    runTransformTest(input, output);
  }

  /** Test optional fields in StructureDefinition. */
  @Test
  public void optionalFields() throws Exception {
    JsonObject input = getCanonicalInput("Resource", "Resource");
    input.addProperty("text", "text");
    input.addProperty("contact", "contact");
    input.addProperty("name", "name");
    input.addProperty("publisher", "publisher");
    input.addProperty("status", "draft");
    input.addProperty("description", "some descriptions");
    input.addProperty("fhirVersion", "1.0.2");
    input.addProperty("abstract", false);

    JsonObject output = getCanonicalOutput("Resource", "Resource");
    output.addProperty("text", "text");
    output.addProperty("contact", "contact");
    output.addProperty("name", "name");
    output.addProperty("publisher", "publisher");
    output.addProperty("status", "draft");
    output.addProperty("description", "some descriptions");
    output.addProperty("fhirVersion", "1.0.2");
    output.addProperty("abstract", false);

    runTransformTest(input, output);
  }

  /** Test primitive type in StructureDefinition. */
  @Test
  public void primitiveType() throws Exception {
    JsonObject input = getCanonicalInput("base64Binary", "datatype");
    JsonObject output = getCanonicalOutput("base64Binary", "primitive-type");
    runTransformTest(input, output);
  }

  /** Test complex type in StructureDefinition. */
  @Test
  public void complexType() throws Exception {
    JsonObject input = getCanonicalInput("Age", "datatype");
    JsonObject output = getCanonicalOutput("Age", "complex-type");
    runTransformTest(input, output);
  }

  /** Test the base in StructureDefinition. */
  @Test
  public void baseField() throws Exception {
    JsonObject input = getCanonicalInput("Resource", "Resource");
    input.addProperty("base", "base");

    JsonObject output = getCanonicalOutput("Resource", "Resource");
    output.addProperty("baseDefinition", "base");

    runTransformTest(input, output);
  }

  /** Test the path in ElementDefinition. */
  @Test
  public void path() throws Exception {
    JsonObject inputElement = new JsonObject();
    inputElement.addProperty("path", "base64Binary.id");
    JsonObject input = getCanonicalInput("base64Binary", "datatype");
    input.add("snapshot", getElementsWrapper(inputElement));

    JsonObject outputElement = new JsonObject();
    outputElement.addProperty("path", "base64Binary.id");
    outputElement.addProperty("id", "base64Binary.id");
    JsonObject output = getCanonicalOutput("base64Binary", "primitive-type");
    output.add("snapshot", getElementsWrapper(outputElement));

    runTransformTest(input, output);
  }

  /** Test path override in ElementDefinition. */
  @Test
  public void pathOverride() throws Exception {
    JsonObject inputElement = new JsonObject();
    inputElement.addProperty("path", "Quantity.id");
    JsonObject input = getCanonicalInput("Age", "datatype");
    input.add("snapshot", getElementsWrapper(inputElement));

    JsonObject outputElement = new JsonObject();
    outputElement.addProperty("path", "Age.id");
    outputElement.addProperty("id", "Age.id");
    JsonObject output = getCanonicalOutput("Age", "complex-type");
    output.add("snapshot", getElementsWrapper(outputElement));

    runTransformTest(input, output);
  }

  /** Test target profile in ElementDefinition. */
  @Test
  public void targetProfile() throws Exception {
    JsonObject inputElement = new JsonObject();
    inputElement.addProperty("path", "base64Binary.id");
    JsonArray profiles = new JsonArray();
    profiles.add("profile");
    JsonObject type = new JsonObject();
    type.add("profile", profiles);
    JsonArray types = new JsonArray();
    types.add(type);
    inputElement.add("type", types);
    JsonObject input = getCanonicalInput("base64Binary", "datatype");
    input.add("snapshot", getElementsWrapper(inputElement));

    JsonObject outputElement = new JsonObject();
    outputElement.addProperty("path", "base64Binary.id");
    outputElement.addProperty("id", "base64Binary.id");
    JsonObject newType = new JsonObject();
    newType.addProperty("targetProfile", "profile");
    JsonArray newTypes = new JsonArray();
    newTypes.add(newType);
    outputElement.add("type", newTypes);
    JsonObject output = getCanonicalOutput("base64Binary", "primitive-type");
    output.add("snapshot", getElementsWrapper(outputElement));

    runTransformTest(input, output);
  }

  /** Test content reference in ElementDefinition. */
  @Test
  public void contentReference() throws Exception {
    JsonObject inputElement = new JsonObject();
    inputElement.addProperty("path", "Composition.section.section");
    inputElement.addProperty("nameReference", "section");
    JsonObject input = getCanonicalInput("Composition", "Resource");
    input.add("snapshot", getElementsWrapper(inputElement));

    JsonObject outputElement = new JsonObject();
    outputElement.addProperty("path", "Composition.section.section");
    outputElement.addProperty("id", "Composition.section.section");
    outputElement.addProperty("contentReference", "#Composition.section");
    JsonObject output = getCanonicalOutput("Composition", "Resource");
    output.add("snapshot", getElementsWrapper(outputElement));

    runTransformTest(input, output);
  }

  /** Test extensible binding in ElementDefinition. */
  @Test
  public void extensibleBinding() throws Exception {
    JsonObject inputElement = new JsonObject();
    inputElement.addProperty("path", "base64Binary.id");
    JsonObject binding = new JsonObject();
    binding.addProperty("strength", "extensible");
    binding.addProperty("valueSetReference", "123");
    inputElement.add("binding", binding);
    JsonObject input = getCanonicalInput("base64Binary", "datatype");
    input.add("snapshot", getElementsWrapper(inputElement));

    JsonObject outputElement = new JsonObject();
    outputElement.addProperty("path", "base64Binary.id");
    outputElement.addProperty("id", "base64Binary.id");
    JsonObject output = getCanonicalOutput("base64Binary", "primitive-type");
    output.add("snapshot", getElementsWrapper(outputElement));

    runTransformTest(input, output);
  }

  /** Test required binding in ElementDefinition. */
  @Test
  public void requiredBinding() throws Exception {
    JsonObject inputElement = new JsonObject();
    inputElement.addProperty("path", "base64Binary.id");
    JsonObject binding = new JsonObject();
    binding.addProperty("strength", "required");
    binding.addProperty("valueSetReference", "123");
    inputElement.add("binding", binding);
    JsonObject input = getCanonicalInput("base64Binary", "datatype");
    input.add("snapshot", getElementsWrapper(inputElement));

    JsonObject outputElement = new JsonObject();
    outputElement.addProperty("path", "base64Binary.id");
    outputElement.addProperty("id", "base64Binary.id");
    outputElement.add("binding", binding);
    JsonObject output = getCanonicalOutput("base64Binary", "primitive-type");
    output.add("snapshot", getElementsWrapper(outputElement));

    runTransformTest(input, output);
  }

  /** Test language binding in ElementDefinition. */
  @Test
  public void languageBinding() throws Exception {
    JsonObject inputElement = new JsonObject();
    inputElement.addProperty("path", "base64Binary.id");
    JsonObject binding = new JsonObject();
    binding.addProperty("strength", "required");
    binding.addProperty("valueSetUri", "http://tools.ietf.org/html/bcp47");
    inputElement.add("binding", binding);
    JsonObject input = getCanonicalInput("base64Binary", "datatype");
    input.add("snapshot", getElementsWrapper(inputElement));

    JsonObject outputElement = new JsonObject();
    outputElement.addProperty("path", "base64Binary.id");
    outputElement.addProperty("id", "base64Binary.id");
    JsonObject newBinding = new JsonObject();
    newBinding.addProperty("strength", "required");
    newBinding.addProperty("valueSetUri", "http://hl7.org/fhir/ValueSet/all-languages");
    outputElement.add("binding", newBinding);
    JsonObject output = getCanonicalOutput("base64Binary", "primitive-type");
    output.add("snapshot", getElementsWrapper(outputElement));

    runTransformTest(input, output);
  }

  /** Test the optional fields in ElementDefinition. */
  @Test
  public void optionalElementFields() throws Exception {
    JsonObject inputElement = new JsonObject();
    inputElement.addProperty("path", "base64Binary.id");
    inputElement.addProperty("representation", "representation");
    inputElement.addProperty("short", "short");
    inputElement.addProperty("definition", "definition");
    inputElement.addProperty("min", 0);
    inputElement.addProperty("max", "*");
    JsonObject input = getCanonicalInput("base64Binary", "datatype");
    input.add("snapshot", getElementsWrapper(inputElement));

    JsonObject outputElement = new JsonObject();
    outputElement.addProperty("path", "base64Binary.id");
    outputElement.addProperty("id", "base64Binary.id");
    outputElement.addProperty("representation", "representation");
    outputElement.addProperty("short", "short");
    outputElement.addProperty("definition", "definition");
    outputElement.addProperty("min", 0);
    outputElement.addProperty("max", "*");
    JsonObject output = getCanonicalOutput("base64Binary", "primitive-type");
    output.add("snapshot", getElementsWrapper(outputElement));

    runTransformTest(input, output);
  }
}
