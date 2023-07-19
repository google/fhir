//    Copyright 2019 Google Inc.
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

import static java.time.ZoneOffset.UTC;

import com.google.common.base.Ascii;
import com.google.common.base.CaseFormat;
import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.Iterables;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.r4.core.Canonical;
import com.google.fhir.r4.core.DateTime;
import com.google.fhir.r4.core.ElementDefinition;
import com.google.fhir.r4.core.ExtensionContextTypeCode;
import com.google.fhir.r4.core.ResourceTypeCode;
import com.google.fhir.r4.core.SearchParameter;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.TypeDerivationRuleCode;
import java.time.LocalDate;
import java.util.AbstractMap.SimpleEntry;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

/** Common Utilites for proto generation. */
final class GeneratorUtils {
  private GeneratorUtils() {}

  // The path for annotation definition of proto options.
  public static final String ANNOTATION_PATH = "proto/google/fhir/proto";

  private static final Pattern WORD_BREAK_PATTERN = Pattern.compile("[^A-Za-z0-9]+([A-Za-z0-9])");
  private static final Pattern ACRONYM_PATTERN = Pattern.compile("([A-Z])([A-Z]+)(?![a-z])");

  static String toFieldNameCase(String fieldName) {
    // Make sure the field name is snake case, as required by the proto style guide.
    String normalizedFieldName =
        CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, resolveAcronyms(fieldName));
    // TODO(b/244184211): add more normalization here if necessary.  I think this is enough for now.
    return normalizedFieldName;
  }

  // Converts a FHIR id strings to UpperCamelCasing for FieldTypes using a regex pattern that
  // considers hyphen, underscore and space to be word breaks.
  static String toFieldTypeCase(String type) {
    String normalizedType = type;
    if (Character.isLowerCase(type.charAt(0))) {
      normalizedType = Ascii.toUpperCase(type.substring(0, 1)) + type.substring(1);
    }
    Matcher matcher = WORD_BREAK_PATTERN.matcher(normalizedType);
    StringBuffer typeBuilder = new StringBuffer();
    boolean foundMatch = false;
    while (matcher.find()) {
      foundMatch = true;
      matcher.appendReplacement(typeBuilder, Ascii.toUpperCase(matcher.group(1)));
    }
    return foundMatch ? matcher.appendTail(typeBuilder).toString() : normalizedType;
  }

  static String resolveAcronyms(String input) {
    // Turn acronyms into single words, e.g., FHIRIsGreat -> FhirIsGreat, so that it ultimately
    // becomes FhirIsGreat instead of f_h_i_r_is_great
    Matcher matcher = ACRONYM_PATTERN.matcher(input);
    StringBuffer sb = new StringBuffer();
    while (matcher.find()) {
      matcher.appendReplacement(sb, matcher.group(1) + Ascii.toLowerCase(matcher.group(2)));
    }
    matcher.appendTail(sb);
    return sb.toString();
  }

  /** Struct representing a proto type and package. */
  static class QualifiedType {
    final String type;
    final String packageName;

    QualifiedType(String type, String packageName) {
      this.type = type;
      this.packageName = packageName;
    }

    String toQualifiedTypeString() {
      return "." + packageName + "." + type;
    }

    String getName() {
      return nameFromQualifiedName(toQualifiedTypeString());
    }

    QualifiedType childType(String name) {
      return new QualifiedType(type + "." + name, packageName);
    }
  }

  static String nameFromQualifiedName(String qualifiedName) {
    List<String> messageNameParts = Splitter.on('.').splitToList(qualifiedName);
    return messageNameParts.get(messageNameParts.size() - 1);
  }

  // Token in a id string.  The sequence of tokens forms a heirarchical relationship, where each
  // dot-delimited token is of the form pathpart:slicename/reslicename.
  // See https://www.hl7.org/fhir/elementdefinition.html#id
  static class IdToken {
    final String pathpart;
    final boolean isChoiceType;
    final String slicename;
    final String reslice;

    private IdToken(String pathpart, boolean isChoiceType, String slicename, String reslice) {
      this.pathpart = pathpart;
      this.isChoiceType = isChoiceType;
      this.slicename = slicename;
      this.reslice = reslice;
    }

    static IdToken fromTokenString(String tokenString) {
      List<String> colonSplit = Splitter.on(':').splitToList(tokenString);
      String pathpart = colonSplit.get(0);
      boolean isChoiceType = pathpart.endsWith("[x]");
      if (isChoiceType) {
        pathpart = pathpart.substring(0, pathpart.length() - "[x]".length());
      }
      if (colonSplit.size() == 1) {
        return new IdToken(pathpart, isChoiceType, null, null);
      }
      if (colonSplit.size() != 2) {
        throw new IllegalArgumentException("Bad token string: " + tokenString);
      }
      List<String> slashSplit = Splitter.on('/').splitToList(colonSplit.get(1));
      if (slashSplit.size() == 1) {
        return new IdToken(pathpart, isChoiceType, slashSplit.get(0), null);
      }
      if (slashSplit.size() == 2) {
        return new IdToken(pathpart, isChoiceType, slashSplit.get(0), slashSplit.get(1));
      }
      throw new IllegalArgumentException("Bad token string: " + tokenString);
    }

    boolean isSlice() {
      return slicename != null;
    }
  }

  static IdToken lastIdToken(String idString) {
    List<String> tokenStrings = Splitter.on('.').splitToList(idString);
    return IdToken.fromTokenString(Iterables.getLast(tokenStrings));
  }

  static IdToken lastIdToken(ElementDefinition element) {
    return lastIdToken(element.getId().getValue());
  }

  static boolean isChoiceTypeSlice(ElementDefinition element) {
    IdToken token = lastIdToken(element);
    return token.isChoiceType && token.isSlice();
  }

  static boolean isSlice(ElementDefinition element) {
    return lastIdToken(element).isSlice();
  }

  static boolean isProfile(StructureDefinition def) {
    return def.getDerivation().getValue() == TypeDerivationRuleCode.Value.CONSTRAINT;
  }

  // Returns the only element in the list matching a given id.
  // Throws IllegalArgumentException if zero or more than one matching element is found.
  static ElementDefinition getElementById(String id, List<ElementDefinition> elements)
      throws InvalidFhirException {
    return getOptionalElementById(id, elements)
        .orElseThrow(() -> new InvalidFhirException("No element with id: " + id));
  }

  // Returns the only element in the list matching a given id, or an empty optional if none are
  // found.
  // Throws IllegalArgumentException if more than one matching element is found.
  static Optional<ElementDefinition> getOptionalElementById(
      String id, List<ElementDefinition> elements) throws InvalidFhirException {
    List<ElementDefinition> matchingElements =
        elements.stream()
            .filter(element -> element.getId().getValue().equals(id))
            .collect(Collectors.toList());
    if (matchingElements.isEmpty()) {
      return Optional.empty();
    } else if (matchingElements.size() == 1) {
      return Optional.of(matchingElements.get(0));
    } else {
      throw new InvalidFhirException("Multiple elements found matching id: " + id);
    }
  }

  static Optional<ElementDefinition> getParent(
      ElementDefinition element, List<ElementDefinition> elementList) throws InvalidFhirException {
    String elementId = element.getId().getValue();
    int lastDotIndex = elementId.lastIndexOf('.');
    if (lastDotIndex == -1) {
      // This is a top-level Element.
      return Optional.empty();
    }
    String parentElementId = elementId.substring(0, lastDotIndex);
    // Note that we don't use getOptionalElementById, because the assumption is that if a parent id
    // exists, the parent element should also exist.
    return Optional.of(getElementById(parentElementId, elementList));
  }

  static List<ElementDefinition> getSlices(
      ElementDefinition element, List<ElementDefinition> elementList) {
    if (isSlice(element)) {
      throw new IllegalArgumentException(
          "Cannot call getSlices on a slice: " + element.getId().getValue());
    }
    final String id = element.getId().getValue();
    return elementList.stream()
        .filter(
            candidateElement -> {
              String candidateId = candidateElement.getId().getValue();
              return isSlice(candidateElement)
                  && id.equals(candidateId.substring(0, candidateId.lastIndexOf(":")));
            })
        .collect(Collectors.toList());
  }

  // Extract the uri component from a canonical, which can be of the form
  // uri|version
  static String getCanonicalUri(Canonical canonical) {
    String value = canonical.getValue();
    int pipeIndex = value.indexOf("|");
    return pipeIndex != -1 ? value.substring(0, pipeIndex) : value;
  }

  // Map from StructureDefinition url to explicit renaming for the type that should be generated.
  // This is necessary for cases where the generated name type is problematic, e.g., when two
  // generated name types collide, or just to provide nicer names.
  private static final ImmutableMap<String, String> STRUCTURE_DEFINITION_RENAMINGS =
      new ImmutableMap.Builder<String, String>()
          .put("http://hl7.org/fhir/StructureDefinition/valueset-reference", "ValueSetReference")
          .put(
              "http://hl7.org/fhir/StructureDefinition/codesystem-reference", "CodeSystemReference")
          .put(
              "http://hl7.org/fhir/StructureDefinition/request-statusReason",
              "StatusReasonExtension")
          .put(
              "http://hl7.org/fhir/StructureDefinition/workflow-relatedArtifact",
              "RelatedArtifactExtension")
          .put("http://hl7.org/fhir/StructureDefinition/event-location", "LocationExtension")
          .put(
              "http://hl7.org/fhir/StructureDefinition/workflow-episodeOfCare",
              "EpisodeOfCareExtension")
          .put(
              "http://hl7.org/fhir/StructureDefinition/workflow-researchStudy",
              "ResearchStudyExtension")
          .put(
              "http://hl7.org/fhir/us/core/StructureDefinition/us-core-condition",
              "UsCoreCondition")
          .put(
              "http://hl7.org/fhir/us/core/StructureDefinition/us-core-direct", "UsCoreDirectEmail")
          // https://gforge.hl7.org/gf/project/fhir/tracker/?action=TrackerItemEdit&tracker_item_id=22531
          .put("http://hl7.org/fhir/StructureDefinition/hdlcholesterol", "HdlCholesterol")
          .put("http://hl7.org/fhir/StructureDefinition/ldlcholesterol", "LdlCholesterol")
          .put("http://hl7.org/fhir/StructureDefinition/lipidprofile", "LipidProfile")
          .put("http://hl7.org/fhir/StructureDefinition/cholesterol", "Cholesterol")
          .put("http://hl7.org/fhir/StructureDefinition/triglyceride", "Triglyceride")
          .put("http://hl7.org/fhir/StructureDefinition/cqf-expression", "CqfExpression")
          .put("http://hl7.org/fhir/StructureDefinition/cqf-library", "CqfLibrary")
          .build();

  // Given a structure definition, gets the name of the top-level message that will be generated.
  static String getTypeName(StructureDefinition def) {
    if (STRUCTURE_DEFINITION_RENAMINGS.containsKey(def.getUrl().getValue())) {
      return STRUCTURE_DEFINITION_RENAMINGS.get(def.getUrl().getValue());
    }
    return isProfile(def) ? getProfileTypeName(def) : toFieldTypeCase(def.getId().getValue());
  }

  /**
   * Derives a message type for a Profile. Uses the name field from the StructureDefinition. If the
   * StructureDefinition has a context indicating a single Element type, that type is used as a
   * prefix. If the element is a simple extension, returns the type defined by the extension.
   */
  private static String getProfileTypeName(StructureDefinition def) {
    String name = def.getName().getValue();

    if (!isExtensionProfile(def)) {
      return toFieldTypeCase(name);
    }

    Set<String> contexts = new HashSet<>();
    Splitter splitter = Splitter.on('.').limit(2);
    for (StructureDefinition.Context context : def.getContextList()) {
      // Only interest in the top-level resource.
      if (context.getType().getValue() == ExtensionContextTypeCode.Value.ELEMENT) {
        contexts.add(splitter.splitToList(context.getExpression().getValue()).get(0));
      }
    }
    if (contexts.size() == 1) {
      String context = Iterables.getOnlyElement(contexts);
      if (!context.equals("*") && !context.equals("Element")) {
        return toFieldTypeCase(context) + toFieldTypeCase(name);
      }
    }
    return toFieldTypeCase(name);
  }

  /** Coverts a LocalDate to a FHIR DateTime for use as a creation time in generated resources. */
  static DateTime buildCreationDateTime(LocalDate localDate) {
    return DateTime.newBuilder()
        .setTimezone("+00:00")
        .setPrecision(DateTime.Precision.DAY)
        .setValueUs(localDate.atStartOfDay().toInstant(UTC).toEpochMilli() * 1000)
        .build();
  }

  static boolean isExtensionProfile(StructureDefinition def) {
    return def.getType().getValue().equals("Extension")
        && def.getDerivation().getValue() == TypeDerivationRuleCode.Value.CONSTRAINT;
  }

  // Create a map from resource types to search parameters.
  static Map<ResourceTypeCode.Value, List<SearchParameter>> getSearchParameterMap(
      List<SearchParameter> searchParameters) {
    return searchParameters.stream()
        .flatMap(
            searchParameter ->
                searchParameter.getBaseList().stream()
                    .map(base -> new SimpleEntry<>(base.getValue(), searchParameter)))
        .collect(
            Collectors.groupingBy(
                Map.Entry::getKey, Collectors.mapping(Map.Entry::getValue, Collectors.toList())));
  }
}
