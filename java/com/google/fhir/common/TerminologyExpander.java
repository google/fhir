//    Copyright 2024 Google LLC
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

import static com.google.common.collect.ImmutableList.toImmutableList;
import static com.google.common.collect.ImmutableSet.toImmutableSet;

import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableSet;
import com.google.fhir.protogen.FhirPackage;
import com.google.fhir.r4.core.Canonical;
import com.google.fhir.r4.core.CodeSystem;
import com.google.fhir.r4.core.CodeSystem.ConceptDefinition;
import com.google.fhir.r4.core.ValueSet;
import com.google.fhir.r4.core.ValueSet.Compose.ConceptSet;
import com.google.fhir.r4.core.ValueSet.Compose.ConceptSet.Filter;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Stream;

/**
 * Utility for expanding CodeSystems and ValueSets into flattened lists. Uses a FhirPackage to
 * manage dependencies.
 */
public final class TerminologyExpander {

  private final FhirPackage fhirPackage;

  public TerminologyExpander(FhirPackage fhirPackage) {
    this.fhirPackage = fhirPackage;
  }

  /** Class representing a code, along with data about what CodeSystem it comes from. */
  public static class ValueSetCode {
    public final String code;
    public final String display;

    // To support conversion of legacy profiled protos, ValueSet enums need metadata about the
    // system that a code came from. Once support for generating profiled protos is eliminated, this
    // can be eliminated.
    public final String sourceSystem;

    public ValueSetCode(String code, String display, String sourceSystem) {
      this.code = code;
      this.display = display;
      this.sourceSystem = sourceSystem;
    }

    @Override
    public boolean equals(Object other) {
      if (other == null) {
        return false;
      }
      if (!(other instanceof ValueSetCode)) {
        return false;
      }
      ValueSetCode otherCode = (ValueSetCode) other;
      return code.equals(otherCode.code) && sourceSystem.equals(otherCode.sourceSystem);
    }

    @Override
    public int hashCode() {
      return Objects.hash(code, sourceSystem);
    }

    @Override
    public String toString() {
      return String.format(
          "{code: %s, display: %s, sourceSystem: %s}", code, display, sourceSystem);
    }
  }

  /**
   * Returns all codes included in `valueSet`, expanding any referenced ValueSets and CodeSystems.
   */
  public ImmutableList<ValueSetCode> expandValueSet(ValueSet valueSet) {
    return getCodeStreamForValueSet(valueSet).collect(toImmutableList());
  }

  /**
   * Gets a flattened list of all concepts in a CodeSystem.
   *
   * <p>Note this is static, since it doesn't require access to any other resources.
   */
  public static List<ConceptDefinition> expandCodeSystem(CodeSystem system) {
    return getFlattenedCodesForConcepts(
        system.getConceptList(),
        new HashSet<>() /* at top level of system - codes have no ancestors */,
        new ArrayList<>() /* no filters */);
  }

  /**
   * Given a stream of `Contains` codes, each of which can contain child codes, returns a flattened
   * stream containing all codes in the expanded hierarchy.
   */
  private static Stream<ValueSet.Expansion.Contains> flattenContainsStream(
      Stream<ValueSet.Expansion.Contains> containsList) {
    return containsList.flatMap(
        element ->
            Stream.concat(
                Stream.of(element), flattenContainsStream(element.getContainsList().stream())));
  }

  /**
   * Given a `valueSet` and a list of `filters`, returns a stream of all codes in `valueSet`
   * matching `filter`, expanding any referenced ValueSets CodeSystems.
   */
  private Stream<ValueSetCode> getCodeStreamForValueSet(ValueSet valueSet) {
    if (valueSet.hasExpansion()) {
      // If the ValueSet is already expanded, trust the expansion, and ignore anything else.
      return flattenContainsStream(valueSet.getExpansion().getContainsList().stream())
          .map(
              element ->
                  new ValueSetCode(
                      element.getCode().getValue(),
                      element.getDisplay().getValue(),
                      element.getSystem().getValue()));
    }

    Stream<ValueSetCode> allIncludeCodes =
        valueSet.getCompose().getIncludeList().stream().flatMap(this::expandConceptSet);

    ImmutableSet<ValueSetCode> allExcludeCodes =
        valueSet.getCompose().getExcludeList().stream()
            .flatMap(this::expandConceptSet)
            .collect(toImmutableSet());

    return allIncludeCodes.filter(code -> !allExcludeCodes.contains(code));
  }

  /**
   * Given a list of ConceptDefinitions, that may have child concepts, returns a flattened list of
   * all ConceptDefinitions in the trees that match a given list of filters.
   *
   * <p>The `ancestors` param contains any codes that these concepts are children of. This allows
   * evaluating filters like `descendent-of`.
   */
  private static List<ConceptDefinition> getFlattenedCodesForConcepts(
      List<ConceptDefinition> concepts, Set<String> ancestors, List<Filter> filters) {
    ArrayList<ConceptDefinition> filteredConcepts = new ArrayList<>();
    for (ConceptDefinition concept : concepts) {
      if (applyAllFilters(concept, ancestors, filters)) {
        // Since this is a flattened list, clear any child concepts.
        filteredConcepts.add(concept.toBuilder().clearConcept().build());
      }
      if (!concept.getConceptList().isEmpty()) {
        // This concept has child concepts.
        // Recurse to child concepts, including this node as an ancestor.
        HashSet<String> newAncestors = new HashSet<>(ancestors);
        newAncestors.add(concept.getCode().getValue());
        filteredConcepts.addAll(
            getFlattenedCodesForConcepts(concept.getConceptList(), newAncestors, filters));
      }
    }

    return filteredConcepts;
  }

  /** Expands a given `conceptSet`. */
  private Stream<ValueSetCode> expandConceptSet(ConceptSet conceptSet) {
    // Per https://www.hl7.org/fhir/valueset.html#compositions, there are three cases to consider:
    // * valueSet(s) only: Include all codes from the intersection of ValueSets
    // * System only: Include codes specified by system, or a subset if `concept` is also specified.
    // * valueSet and System: Include all codes from the system that are in the ValueSet
    // In all cases, we should apply any filters specified the the resulting set.
    boolean hasValueSets = !conceptSet.getValueSetList().isEmpty();
    boolean hasSystem = conceptSet.hasSystem();
    if (hasValueSets && !hasSystem) {
      return getCodesFromValueSetIntersection(conceptSet);
    } else if (hasSystem && !hasValueSets) {
      return getCodesFromReferencedSystem(conceptSet);
    } else if (hasValueSets && hasSystem) {
      ImmutableSet<ValueSetCode> codeSetFromValueSets =
          getCodesFromValueSetIntersection(conceptSet).collect(toImmutableSet());
      return getCodesFromReferencedSystem(conceptSet)
          .filter(code -> codeSetFromValueSets.contains(code));
    } else {
      throw new IllegalArgumentException(
          "Invalid composition of ConceptSets " + hasValueSets + " " + hasSystem);
    }
  }

  private Stream<ValueSetCode> getCodesFromValueSetIntersection(ConceptSet conceptSet) {
    // Get a set containing the unordered expansion of all ValueSets.
    Set<ImmutableSet<ValueSetCode>> includedValueSets = new HashSet<>();
    for (Canonical canonical : conceptSet.getValueSetList()) {
      try {
        ValueSet valueSet =
            fhirPackage
                .getValueSet(canonical.getValue())
                .orElseThrow(
                    () ->
                        new IllegalArgumentException(
                            "Unable to resolve ValueSet: " + canonical.getValue()));
        includedValueSets.add(getCodeStreamForValueSet(valueSet).collect(toImmutableSet()));
      } catch (InvalidFhirException e) {
        throw new IllegalArgumentException(e);
      }
    }

    // Flatten the set of sets to a single stream, and then filter to elements that are present in
    // each of the sets.
    return includedValueSets.stream()
        .flatMap(valueSet -> valueSet.stream())
        .filter(code -> includedValueSets.stream().allMatch(list -> list.contains(code)));
  }

  private Stream<ValueSetCode> getCodesFromReferencedSystem(ConceptSet conceptSet) {
    if (conceptSet.hasSystem()) {
      if (conceptSet.getConceptList().isEmpty()) {
        // No concept list specified - Include all codes from this system that match the filter.
        try {
          Optional<CodeSystem> system =
              fhirPackage.getCodeSystem(conceptSet.getSystem().getValue());
          if (system.isEmpty()) {
            // This system cannot be expanded (e.g., ICD10)
            return Stream.empty();
          }
          return getFlattenedCodesForConcepts(
                  system.get().getConceptList(),
                  new HashSet<>() /* at top level of system - codes have no ancestors */,
                  conceptSet.getFilterList())
              .stream()
              .map(
                  element ->
                      new ValueSetCode(
                          element.getCode().getValue(),
                          element.getDisplay().getValue(),
                          system.get().getUrl().getValue()));
        } catch (InvalidFhirException e) {
          throw new IllegalArgumentException(e);
        }
      } else {
        // There is an explicit list of codes from that system to use.  Trust it.
        return conceptSet.getConceptList().stream()
            .map(
                element ->
                    new ValueSetCode(
                        element.getCode().getValue(),
                        element.getDisplay().getValue(),
                        conceptSet.getSystem().getValue()));
      }
    }
    return Stream.empty();
  }

  /**
   * Applies a list of filters to a code, and returns true if all filters return true. This is
   * equivalent to "anding" all of the filters together.
   */
  private static boolean applyAllFilters(
      ConceptDefinition code, Set<String> ancestors, List<ConceptSet.Filter> filters) {
    return filters.stream().allMatch(filter -> applyFilter(code, ancestors, filter));
  }

  private static boolean applyFilter(
      ConceptDefinition concept, Set<String> ancestors, ConceptSet.Filter filter) {
    // Filter properties other than "concept" are not well-defined by the spec.  Ignore them here,
    // but print a warning.
    if (!filter.getProperty().getValue().equals("concept")) {
      System.err.println(
          "Warning: value filters by property other than concept are not supported. "
              + " Found: "
              + filter.getProperty().getValue());
      return true;
    }

    String codeString = concept.getCode().getValue();
    String filterValue = filter.getValue().getValue();
    switch (filter.getOp().getValue()) {
      case EQUALS:
        return codeString.equals(filterValue);
      case IS_A:
        return codeString.equals(filterValue) || ancestors.contains(filterValue);
      case DESCENDENT_OF:
        return ancestors.contains(filterValue);
      case IS_NOT_A:
        return !codeString.equals(filterValue) && !ancestors.contains(filterValue);
      case REGEX:
        return codeString.matches(filterValue);
      case IN:
        return Splitter.on(",").splitToList(filterValue).contains(codeString);
      case NOT_IN:
        return !Splitter.on(",").splitToList(filterValue).contains(codeString);
      case EXISTS:
        return filterValue.equals("true") == codeString.isEmpty();
      case GENERALIZES:
        // "generalizes" is not supported - default to returning true so we're overly permissive
        // rather than rejecting valid codes.
        return true;
      default:
        throw new IllegalArgumentException(
            "Unrecognized filter operator: " + filter.getOp().getValue());
    }
  }
}
