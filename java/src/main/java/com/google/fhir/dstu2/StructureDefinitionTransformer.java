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

import com.google.common.base.Strings;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.util.Map.Entry;

/**
 * A class that transforms a DSTU2 StructureDefinition to STU3. The goal is not to represent dstu2
 * structure definition 100% in stu3. Instead, we are looking to construct stu3 structure definition
 * with minimum information necessary to generate the proto. While many fields are structurally the
 * same between DSTU2 and STU3, this class does handle some structural differences, which are marked
 * by a "Structurally different between DSTU2 and STU3." comment.
 */
public class StructureDefinitionTransformer {
  private static final JsonParser PARSER = new JsonParser();
  private static final Gson GSON = new GsonBuilder().setPrettyPrinting().create();

  private static final String ID_KEY = "id";
  private static final String TYPE_KEY = "type";
  private static final String URL_KEY = "url";
  private static final String DATE_KEY = "date";
  private static final String KIND_KEY = "kind";
  private static final String BASE_KEY = "base";
  private static final String BASE_DEFINITION_KEY = "baseDefinition";
  private static final String ELEMENT_KEY = "element";
  private static final String PATH_KEY = "path";
  private static final String PROFILE_KEY = "profile";
  private static final String TARGET_PROFILE_KEY = "targetProfile";
  private static final String NAME_REFERENCE_KEY = "nameReference";
  private static final String CONTENT_REFERENCE_KEY = "contentReference";
  private static final String BINDING_KEY = "binding";
  private static final String STRENGTH_KEY = "strength";
  private static final String VALUE_SET_URI_KEY = "valueSetUri";
  private static final String FHIR_COMMENTS_KEY = "fhir_comments";
  private static final String REQUIRED_VALUE = "required";
  private static final String DATATYPE_VALUE = "datatype";
  private static final String PRIMITIVE_TYPE_VALUE = "primitive-type";
  private static final String COMPLEX_TYPE_VALUE = "complex-type";
  private static final String RESOURCE_TYPE_KEY = "resourceType";
  private static final String STRUCTURE_DEFINITION_VALUE = "StructureDefinition";

  // The primitive list. dstu2 categorizes both primitives and complex primitives as datatype. We
  // need to separate them in the stu3 structure definitions.
  private static final ImmutableSet<String> PRIMITIVES =
      ImmutableSet.of(
          "base64Binary",
          "boolean",
          "code",
          "date",
          "dateTime",
          "decimal",
          "id",
          "instant",
          "integer",
          "markdown",
          "oid",
          "positiveInt",
          "string",
          "time",
          "unsignedInt",
          "uri",
          "uuid",
          "xhtml");

  // Some extended datatypes are defined as profile on the basic datatypes but they are used as
  // first-class datatypes. We need special handling of their paths.
  private static final ImmutableSet<String> SPECIAL_FIRST_CLASS_TYPES =
      ImmutableSet.of(
          "Age",
          "Count",
          "Distance",
          "Duration",
          "Money",
          "SimpleQuantity",
          "id",
          "code",
          "markdown",
          "oid",
          "positiveInt",
          "unsignedInt",
          "uuid");

  // From the element path to the content_reference field. dstu2 does not have the full path for
  // content reference. This is a manually curated list that maps the element to its required
  // content reference.
  private static final ImmutableMap<String, String> CONTENT_REFERENCE_OVERRIDES =
      new ImmutableMap.Builder<String, String>()
          .put("Composition.section.section", "#Composition.section")
          .put("Bundle.entry.link", "#Bundle.link")
          .put("ConceptMap.element.target.product", "#ConceptMap.element.target.dependsOn")
          .put("Conformance.rest.searchParam", "#Conformance.rest.resource.searchParam")
          .put("Contract.term.group", "#Contract.term")
          .put("DiagnosticOrder.item.event", "#DiagnosticOrder.event")
          .put("ImplementationGuide.page.page", "#ImplementationGuide.page")
          .put("Observation.component.referenceRange", "#Observation.referenceRange")
          .put("OperationDefinition.parameter.part", "#OperationDefinition.parameter")
          .put("Parameters.parameter.part", "#Parameters.parameter")
          .put("Provenance.entity.agent", "#Provenance.agent")
          .put("Questionnaire.group.group", "#Questionnaire.group")
          .put("Questionnaire.group.question.group", "#Questionnaire.group")
          .put("QuestionnaireResponse.group.group", "#QuestionnaireResponse.group")
          .put("QuestionnaireResponse.group.question.answer.group", "#QuestionnaireResponse.group")
          .put("TestScript.setup.metadata", "#TestScript.metadata")
          .put("TestScript.teardown.action.operation", "#TestScript.setup.action.operation")
          .put("TestScript.test.action.assert", "#TestScript.setup.action.assert")
          .put("TestScript.test.action.operation", "#TestScript.setup.action.operation")
          .put("TestScript.test.metadata", "#TestScript.metadata")
          .put("ValueSet.codeSystem.concept.concept", "#ValueSet.codeSystem.concept")
          .put(
              "ValueSet.compose.include.concept.designation",
              "#ValueSet.codeSystem.concept.designation")
          .put("ValueSet.compose.exclude", "#ValueSet.compose.include")
          .put("ValueSet.expansion.contains.contains", "#ValueSet.expansion.contains")
          .build();

  private static IllegalArgumentException missingFieldException(String field) {
    return new IllegalArgumentException(String.format("the input is missing the %s field", field));
  }

  private static String getString(JsonObject json, String field) {
    if (!json.has(field)) {
      throw missingFieldException(field);
    }
    return json.getAsJsonPrimitive(field).getAsString();
  }

  private static void setFieldIfNotEmpty(JsonObject input, JsonObject output, String field) {
    if (input.has(field)) {
      output.add(field, input.get(field));
    }
  }

  /** Method to strip all instances of a field recursively in a JSON structure. */
  private static void stripAllInstancesOfField(JsonElement data, String field) {
    if (data.isJsonArray()) {
      for (int i = 0; i < data.getAsJsonArray().size(); i++) {
        stripAllInstancesOfField(data.getAsJsonArray().get(i), field);
      }
    } else if (data.isJsonObject()) {
      data.getAsJsonObject().remove(field);
      for (Entry<String, JsonElement> entry : data.getAsJsonObject().entrySet()) {
        stripAllInstancesOfField(entry.getValue(), field);
      }
    }
  }

  private static void changeToTargetProfile(JsonObject element) {
    if (!element.has(TYPE_KEY)) {
      // No types information.
      return;
    }
    JsonArray types = element.getAsJsonArray(TYPE_KEY);
    for (int i = 0; i < types.size(); i++) {
      JsonObject type = types.get(i).getAsJsonObject();
      JsonArray profiles = type.getAsJsonArray(PROFILE_KEY);
      if (profiles == null || profiles.isJsonNull() || profiles.size() == 0) {
        // No profile information.
        continue;
      }
      if (profiles.size() != 1) {
        throw new IllegalArgumentException(
            String.format("expecting only one profile; but got %s", profiles));
      }
      // Replace the profile list with a targetProfile field.
      type.add(TARGET_PROFILE_KEY, profiles.get(0));
      type.remove(PROFILE_KEY);
    }
  }

  private static void changeToContentReference(
      String path, JsonObject element, JsonObject newElement) {
    if (!element.has(NAME_REFERENCE_KEY)) {
      return;
    }
    String ref = element.get(NAME_REFERENCE_KEY).getAsString();
    if (!CONTENT_REFERENCE_OVERRIDES.containsKey(path)) {
      throw new IllegalArgumentException(
          String.format("no override for path %s that have name reference %s", path, ref));
    }
    newElement.addProperty(CONTENT_REFERENCE_KEY, CONTENT_REFERENCE_OVERRIDES.get(path));
  }

  private static void addBindingIfApplicable(JsonObject element, JsonObject newElement) {
    if (!element.has(BINDING_KEY)) {
      return;
    }
    JsonObject binding = element.getAsJsonObject(BINDING_KEY);
    String strength = binding.getAsJsonPrimitive(STRENGTH_KEY).getAsString();
    if (Strings.isNullOrEmpty(strength) || !strength.equals(REQUIRED_VALUE)) {
      // No required binding.
      return;
    }
    if (binding.has(VALUE_SET_URI_KEY)) {
      String valueSetUri = binding.getAsJsonPrimitive(VALUE_SET_URI_KEY).getAsString();
      if (valueSetUri.equals("http://tools.ietf.org/html/bcp47")) {
        // Special handling to rewrite the uri for language code binding.
        binding.addProperty(VALUE_SET_URI_KEY, "http://hl7.org/fhir/ValueSet/all-languages");
      }
    }
    newElement.add(BINDING_KEY, binding);
  }

  /** Returns a transformed ElementDefinition in JSONObject. */
  private static JsonObject transformElement(JsonObject element, String defID) {
    JsonObject ret = new JsonObject();
    // ElementDefinition.path; required.
    String path = getString(element, PATH_KEY);
    if (path.isEmpty()) {
      throw missingFieldException(PATH_KEY);
    }
    // Structurally different between DSTU2 and STU3.
    if (SPECIAL_FIRST_CLASS_TYPES.contains(defID)) {
      String[] components = path.split("\\.");
      components[0] = defID;
      path = String.join(".", components);
    }
    ret.addProperty(PATH_KEY, path);
    // Structurally different between DSTU2 and STU3.
    ret.addProperty(ID_KEY, path);
    // Optional fields for the proto generation.
    setFieldIfNotEmpty(element, ret, "representation");
    setFieldIfNotEmpty(element, ret, "short");
    setFieldIfNotEmpty(element, ret, "definition");
    setFieldIfNotEmpty(element, ret, "min");
    setFieldIfNotEmpty(element, ret, "max");
    setFieldIfNotEmpty(element, ret, "type");

    // Structurally different between DSTU2 and STU3.
    changeToTargetProfile(ret);
    changeToContentReference(path, element, ret);
    addBindingIfApplicable(element, ret);
    return ret;
  }

  private static void fillElements(String key, JsonObject input, JsonObject output) {
    JsonArray elements = input.getAsJsonObject(key).getAsJsonArray(ELEMENT_KEY);
    JsonArray newElements = new JsonArray();
    for (int i = 0; i < elements.size(); i++) {
      newElements.add(
          transformElement(elements.get(i).getAsJsonObject(), getString(input, ID_KEY)));
    }
    JsonObject elementsWrapper = new JsonObject();
    elementsWrapper.add(ELEMENT_KEY, newElements);
    output.add(key, elementsWrapper);
  }

  private static void fillMetadata(JsonObject input, JsonObject output) {
    output.addProperty(RESOURCE_TYPE_KEY, STRUCTURE_DEFINITION_VALUE);
    String id = getString(input, ID_KEY);
    // StructureDefinition.id; required.
    if (id.isEmpty()) {
      throw missingFieldException(ID_KEY);
    }
    output.addProperty(ID_KEY, id);
    // Structurally different between DSTU2 and STU3.
    output.addProperty(TYPE_KEY, id);
    // StructureDefinition.url; required.
    String url = getString(input, URL_KEY);
    if (url.isEmpty()) {
      throw missingFieldException(URL_KEY);
    }
    output.addProperty(URL_KEY, url);
    // StructureDefinition.date; required.
    String date = getString(input, DATE_KEY);
    if (date.isEmpty()) {
      throw missingFieldException(DATE_KEY);
    }
    output.addProperty(DATE_KEY, date);
    // StructureDefinition.kind; required.
    String kind = getString(input, KIND_KEY);
    if (kind.isEmpty()) {
      throw missingFieldException(KIND_KEY);
    }
    if (kind.equals(DATATYPE_VALUE)) {
      // Structurally different between DSTU2 and STU3.
      if (PRIMITIVES.contains(id)) {
        output.addProperty(KIND_KEY, PRIMITIVE_TYPE_VALUE);
      } else {
        output.addProperty(KIND_KEY, COMPLEX_TYPE_VALUE);
      }
    } else {
      output.addProperty(KIND_KEY, kind);
    }
    // StructureDefinition.base.
    if (input.has(BASE_KEY)) {
      // Structurally different between DSTU2 and STU3.
      output.addProperty(BASE_DEFINITION_KEY, getString(input, BASE_KEY));
    }
    // Optional fields for the proto generation.
    setFieldIfNotEmpty(input, output, "text");
    setFieldIfNotEmpty(input, output, "contact");
    setFieldIfNotEmpty(input, output, "name");
    setFieldIfNotEmpty(input, output, "publisher");
    setFieldIfNotEmpty(input, output, "status");
    setFieldIfNotEmpty(input, output, "description");
    setFieldIfNotEmpty(input, output, "fhirVersion");
    setFieldIfNotEmpty(input, output, "abstract");
  }

  /** Returns the transformed structure definition in JSONObject. */
  public static JsonObject transform(JsonObject input) {
    if (!getString(input, RESOURCE_TYPE_KEY).equals(STRUCTURE_DEFINITION_VALUE)) {
      throw new IllegalArgumentException("the input must be a StructureDefinition");
    }
    JsonObject output = new JsonObject();

    fillMetadata(input, output);
    fillElements("snapshot", input, output);
    fillElements("differential", input, output);

    // Structurally different between DSTU2 and STU3.
    stripAllInstancesOfField(output, FHIR_COMMENTS_KEY);

    return output;
  }

  /** Returns the transformed structure definition in JSON String. */
  public static String transform(String input) {
    JsonObject json = PARSER.parse(input).getAsJsonObject();
    JsonObject output = transform(json);
    return GSON.toJson(output);
  }
}
