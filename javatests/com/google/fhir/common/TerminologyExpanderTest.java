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

import static com.google.common.truth.Truth.assertThat;

import com.google.fhir.common.TerminologyExpander.ValueSetCode;
import com.google.fhir.protogen.FhirPackage;
import com.google.fhir.r4.core.Canonical;
import com.google.fhir.r4.core.Code;
import com.google.fhir.r4.core.CodeSystem;
import com.google.fhir.r4.core.CodeSystem.ConceptDefinition;
import com.google.fhir.r4.core.FilterOperatorCode;
import com.google.fhir.r4.core.Uri;
import com.google.fhir.r4.core.ValueSet;
import java.util.Arrays;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class TerminologyExpanderTest {

  private ConceptDefinition.Builder makeConceptDefinition(
      String code, String display, ConceptDefinition.Builder... children) {
    ConceptDefinition.Builder builder =
        ConceptDefinition.newBuilder()
            .setCode(Code.newBuilder().setValue(code))
            .setDisplay(com.google.fhir.r4.core.String.newBuilder().setValue(display));
    for (ConceptDefinition.Builder child : children) {
      builder.addConcept(child);
    }
    return builder;
  }

  private ValueSet.Expansion.Contains.Builder makeContains(
      String code, String display, String system) {
    return ValueSet.Expansion.Contains.newBuilder()
        .setSystem(Uri.newBuilder().setValue(system))
        .setCode(Code.newBuilder().setValue(code))
        .setDisplay(com.google.fhir.r4.core.String.newBuilder().setValue(display));
  }

  private ValueSet.Compose.ConceptSet.ConceptReference.Builder makeConceptReference(
      String code, String display) {
    return ValueSet.Compose.ConceptSet.ConceptReference.newBuilder()
        .setCode(Code.newBuilder().setValue(code))
        .setDisplay(com.google.fhir.r4.core.String.newBuilder().setValue(display));
  }

  private ValueSet.Compose.ConceptSet.ConceptReference.Builder makeConceptReference(String code) {
    return ValueSet.Compose.ConceptSet.ConceptReference.newBuilder()
        .setCode(Code.newBuilder().setValue(code));
  }

  private Uri.Builder makeUri(String url) {
    return Uri.newBuilder().setValue(url);
  }

  private ValueSetCode makeValueSetCode(String code, String display, String sourceSystem) {
    return new ValueSetCode(code, display, sourceSystem);
  }

  private ValueSet.Compose.ConceptSet.Filter makeFilter(FilterOperatorCode.Value op, String value) {
    return ValueSet.Compose.ConceptSet.Filter.newBuilder()
        .setOp(ValueSet.Compose.ConceptSet.Filter.OpCode.newBuilder().setValue(op))
        .setValue(com.google.fhir.r4.core.String.newBuilder().setValue(value))
        .setProperty(Code.newBuilder().setValue("concept"))
        .build();
  }

  @Test
  public void expandCodeSystem_returnsAllFlattenedCodes() {
    CodeSystem system =
        CodeSystem.newBuilder()
            .setUrl(makeUri("foo-system"))
            .addConcept(
                makeConceptDefinition(
                    "foo",
                    "f",
                    makeConceptDefinition("sub foo 1", "sf1"),
                    makeConceptDefinition(
                        "sub foo 2", "sf2", makeConceptDefinition("sub sub foo 2", "ssf2"))))
            .addConcept(makeConceptDefinition("bar", "b"))
            .build();

    assertThat(TerminologyExpander.expandCodeSystem(system))
        .containsExactly(
            makeConceptDefinition("foo", "f").build(),
            makeConceptDefinition("sub foo 1", "sf1").build(),
            makeConceptDefinition("sub foo 2", "sf2").build(),
            makeConceptDefinition("sub sub foo 2", "ssf2").build(),
            makeConceptDefinition("bar", "b").build());
  }

  @Test
  public void expandCodeSystem_noExpansionReturnsEmpty() {
    CodeSystem system = CodeSystem.getDefaultInstance();
    assertThat(TerminologyExpander.expandCodeSystem(system)).isEmpty();
  }

  @Test
  public void expandValueSet_oneToOneWithCodeSystem() throws Exception {
    CodeSystem system =
        CodeSystem.newBuilder()
            .setUrl(makeUri("foo-system"))
            .addConcept(
                makeConceptDefinition(
                    "foo",
                    "f",
                    makeConceptDefinition("sub foo 1", "sf1"),
                    makeConceptDefinition(
                        "sub foo 2", "sf2", makeConceptDefinition("sub sub foo 2", "ssf2"))))
            .addConcept(makeConceptDefinition("bar", "b"))
            .build();

    ValueSet.Builder valueSet = ValueSet.newBuilder();
    valueSet
        .getComposeBuilder()
        .addInclude(ValueSet.Compose.ConceptSet.newBuilder().setSystem(makeUri("foo-system")));

    FhirPackage fhirPackage =
        new FhirPackage(Arrays.asList(), Arrays.asList(), Arrays.asList(system), Arrays.asList());

    // No filters - get entire code system
    assertThat(new TerminologyExpander(fhirPackage).expandValueSet(valueSet.build()))
        .containsExactly(
            makeValueSetCode("foo", "f", "foo-system"),
            makeValueSetCode("sub foo 1", "sf1", "foo-system"),
            makeValueSetCode("sub foo 2", "sf2", "foo-system"),
            makeValueSetCode("sub sub foo 2", "ssf2", "foo-system"),
            makeValueSetCode("bar", "b", "foo-system"));
  }

  @Test
  public void expandValueSet_withEqualsFilter() throws Exception {
    CodeSystem system =
        CodeSystem.newBuilder()
            .setUrl(makeUri("foo-system"))
            .addConcept(
                makeConceptDefinition(
                    "foo",
                    "f",
                    makeConceptDefinition("sub foo 1", "sf1"),
                    makeConceptDefinition(
                        "sub foo 2", "sf2", makeConceptDefinition("sub sub foo 2", "ssf2"))))
            .addConcept(makeConceptDefinition("bar", "b"))
            .build();
    FhirPackage fhirPackage =
        new FhirPackage(Arrays.asList(), Arrays.asList(), Arrays.asList(system), Arrays.asList());

    // "equals" filter
    ValueSet.Builder valueSet = ValueSet.newBuilder();
    valueSet
        .getComposeBuilder()
        .addInclude(
            ValueSet.Compose.ConceptSet.newBuilder()
                .setSystem(makeUri("foo-system"))
                .addFilter(makeFilter(FilterOperatorCode.Value.EQUALS, "sub foo 1")));

    assertThat(new TerminologyExpander(fhirPackage).expandValueSet(valueSet.build()))
        .containsExactly(makeValueSetCode("sub foo 1", "sf1", "foo-system"));
  }

  @Test
  public void expandValueSet_withIsAFilter() throws Exception {
    CodeSystem system =
        CodeSystem.newBuilder()
            .setUrl(makeUri("foo-system"))
            .addConcept(
                makeConceptDefinition(
                    "foo",
                    "f",
                    makeConceptDefinition("sub foo 1", "sf1"),
                    makeConceptDefinition(
                        "sub foo 2", "sf2", makeConceptDefinition("sub sub foo 2", "ssf2"))))
            .addConcept(makeConceptDefinition("bar", "b"))
            .build();
    FhirPackage fhirPackage =
        new FhirPackage(Arrays.asList(), Arrays.asList(), Arrays.asList(system), Arrays.asList());

    // "is-a" filter
    ValueSet.Builder valueSet = ValueSet.newBuilder();
    valueSet
        .getComposeBuilder()
        .addInclude(
            ValueSet.Compose.ConceptSet.newBuilder()
                .setSystem(makeUri("foo-system"))
                .addFilter(makeFilter(FilterOperatorCode.Value.IS_A, "sub foo 2")));

    assertThat(new TerminologyExpander(fhirPackage).expandValueSet(valueSet.build()))
        .containsExactly(
            makeValueSetCode("sub foo 2", "sf2", "foo-system"),
            makeValueSetCode("sub sub foo 2", "ssf2", "foo-system"));
  }

  @Test
  public void expandValueSet_withIsDescendentOfFilter() throws Exception {
    CodeSystem system =
        CodeSystem.newBuilder()
            .setUrl(makeUri("foo-system"))
            .addConcept(
                makeConceptDefinition(
                    "foo",
                    "f",
                    makeConceptDefinition("sub foo 1", "sf1"),
                    makeConceptDefinition(
                        "sub foo 2", "sf2", makeConceptDefinition("sub sub foo 2", "ssf2"))))
            .addConcept(makeConceptDefinition("bar", "b"))
            .build();
    FhirPackage fhirPackage =
        new FhirPackage(Arrays.asList(), Arrays.asList(), Arrays.asList(system), Arrays.asList());

    // "descendent-of" filter
    ValueSet.Builder valueSet = ValueSet.newBuilder();
    valueSet
        .getComposeBuilder()
        .addInclude(
            ValueSet.Compose.ConceptSet.newBuilder()
                .setSystem(makeUri("foo-system"))
                .addFilter(makeFilter(FilterOperatorCode.Value.DESCENDENT_OF, "foo")));

    assertThat(new TerminologyExpander(fhirPackage).expandValueSet(valueSet.build()))
        .containsExactly(
            makeValueSetCode("sub foo 1", "sf1", "foo-system"),
            makeValueSetCode("sub foo 2", "sf2", "foo-system"),
            makeValueSetCode("sub sub foo 2", "ssf2", "foo-system"));
  }

  @Test
  public void expandValueSet_withIsNotAFilter() throws Exception {
    CodeSystem system =
        CodeSystem.newBuilder()
            .setUrl(makeUri("foo-system"))
            .addConcept(
                makeConceptDefinition(
                    "foo",
                    "f",
                    makeConceptDefinition("sub foo 1", "sf1"),
                    makeConceptDefinition(
                        "sub foo 2", "sf2", makeConceptDefinition("sub sub foo 2", "ssf2"))))
            .addConcept(makeConceptDefinition("bar", "b"))
            .build();
    FhirPackage fhirPackage =
        new FhirPackage(Arrays.asList(), Arrays.asList(), Arrays.asList(system), Arrays.asList());

    ValueSet.Builder valueSet = ValueSet.newBuilder();
    valueSet
        .getComposeBuilder()
        .addInclude(
            ValueSet.Compose.ConceptSet.newBuilder()
                .setSystem(makeUri("foo-system"))
                .addFilter(makeFilter(FilterOperatorCode.Value.IS_NOT_A, "foo")));

    assertThat(new TerminologyExpander(fhirPackage).expandValueSet(valueSet.build()))
        .containsExactly(makeValueSetCode("bar", "b", "foo-system"));
  }

  @Test
  public void expandValueSet_withRegexFilter() throws Exception {
    CodeSystem system =
        CodeSystem.newBuilder()
            .setUrl(makeUri("foo-system"))
            .addConcept(
                makeConceptDefinition(
                    "foo",
                    "f",
                    makeConceptDefinition("sub foo 1", "sf1"),
                    makeConceptDefinition(
                        "sub foo 2", "sf2", makeConceptDefinition("sub sub foo 2", "ssf2"))))
            .addConcept(makeConceptDefinition("bar", "b"))
            .build();
    FhirPackage fhirPackage =
        new FhirPackage(Arrays.asList(), Arrays.asList(), Arrays.asList(system), Arrays.asList());

    ValueSet.Builder valueSet = ValueSet.newBuilder();
    valueSet
        .getComposeBuilder()
        .addInclude(
            ValueSet.Compose.ConceptSet.newBuilder()
                .setSystem(makeUri("foo-system"))
                .addFilter(makeFilter(FilterOperatorCode.Value.REGEX, "^sub foo.*")));

    assertThat(new TerminologyExpander(fhirPackage).expandValueSet(valueSet.build()))
        .containsExactly(
            makeValueSetCode("sub foo 1", "sf1", "foo-system"),
            makeValueSetCode("sub foo 2", "sf2", "foo-system"));
  }

  @Test
  public void expandValueSet_withInFilter() throws Exception {
    CodeSystem system =
        CodeSystem.newBuilder()
            .setUrl(makeUri("foo-system"))
            .addConcept(
                makeConceptDefinition(
                    "foo",
                    "f",
                    makeConceptDefinition("sub foo 1", "sf1"),
                    makeConceptDefinition(
                        "sub foo 2", "sf2", makeConceptDefinition("sub sub foo 2", "ssf2"))))
            .addConcept(makeConceptDefinition("bar", "b"))
            .build();
    FhirPackage fhirPackage =
        new FhirPackage(Arrays.asList(), Arrays.asList(), Arrays.asList(system), Arrays.asList());

    ValueSet.Builder valueSet = ValueSet.newBuilder();
    valueSet
        .getComposeBuilder()
        .addInclude(
            ValueSet.Compose.ConceptSet.newBuilder()
                .setSystem(makeUri("foo-system"))
                .addFilter(makeFilter(FilterOperatorCode.Value.IN, "foo,bar")));

    assertThat(new TerminologyExpander(fhirPackage).expandValueSet(valueSet.build()))
        .containsExactly(
            makeValueSetCode("foo", "f", "foo-system"), makeValueSetCode("bar", "b", "foo-system"));
  }

  @Test
  public void expandValueSet_withNotInFilter() throws Exception {
    CodeSystem system =
        CodeSystem.newBuilder()
            .setUrl(makeUri("foo-system"))
            .addConcept(
                makeConceptDefinition(
                    "foo",
                    "f",
                    makeConceptDefinition("sub foo 1", "sf1"),
                    makeConceptDefinition(
                        "sub foo 2", "sf2", makeConceptDefinition("sub sub foo 2", "ssf2"))))
            .addConcept(makeConceptDefinition("bar", "b"))
            .build();
    FhirPackage fhirPackage =
        new FhirPackage(Arrays.asList(), Arrays.asList(), Arrays.asList(system), Arrays.asList());

    ValueSet.Builder valueSet = ValueSet.newBuilder();
    valueSet
        .getComposeBuilder()
        .addInclude(
            ValueSet.Compose.ConceptSet.newBuilder()
                .setSystem(makeUri("foo-system"))
                .addFilter(makeFilter(FilterOperatorCode.Value.NOT_IN, "foo,bar")));

    assertThat(new TerminologyExpander(fhirPackage).expandValueSet(valueSet.build()))
        .containsExactly(
            makeValueSetCode("sub foo 1", "sf1", "foo-system"),
            makeValueSetCode("sub foo 2", "sf2", "foo-system"),
            makeValueSetCode("sub sub foo 2", "ssf2", "foo-system"));
  }

  @Test
  public void expandValueSet_withExplicitExpansion() throws Exception {

    ValueSet.Builder valueSet = ValueSet.newBuilder();
    valueSet.getExpansionBuilder().addContains(makeContains("foo", "f", "foo-system"));

    ValueSet.Expansion.Contains.Builder containsWithSubCode =
        makeContains("bar", "b", "bar-system");
    containsWithSubCode.addContains(makeContains("sub bar", "sb", "bar-system"));
    valueSet.getExpansionBuilder().addContains(containsWithSubCode);

    FhirPackage fhirPackage =
        new FhirPackage(Arrays.asList(), Arrays.asList(), Arrays.asList(), Arrays.asList());

    assertThat(new TerminologyExpander(fhirPackage).expandValueSet(valueSet.build()))
        .containsExactly(
            makeValueSetCode("foo", "f", "foo-system"),
            makeValueSetCode("bar", "b", "bar-system"),
            makeValueSetCode("sub bar", "sb", "bar-system"));
  }

  @Test
  public void expandValueSet_withSystemAndConcepts() throws Exception {
    CodeSystem system =
        CodeSystem.newBuilder()
            .setUrl(makeUri("foo-system"))
            .addConcept(makeConceptDefinition("foo", "f"))
            .addConcept(makeConceptDefinition("bar", "b"))
            .addConcept(makeConceptDefinition("quux", "q"))
            .build();
    FhirPackage fhirPackage =
        new FhirPackage(Arrays.asList(), Arrays.asList(), Arrays.asList(system), Arrays.asList());

    ValueSet.Builder valueSet = ValueSet.newBuilder();
    valueSet
        .getComposeBuilder()
        .addInclude(
            ValueSet.Compose.ConceptSet.newBuilder()
                .setSystem(makeUri("foo-system"))
                .addConcept(makeConceptReference("foo", "f"))
                .addConcept(makeConceptReference("quux", "q")));

    assertThat(new TerminologyExpander(fhirPackage).expandValueSet(valueSet.build()))
        .containsExactly(
            makeValueSetCode("foo", "f", "foo-system"),
            makeValueSetCode("quux", "q", "foo-system"));
  }

  @Test
  public void expandValueSet_multipleIncludesAreCumulative() throws Exception {
    CodeSystem fooSystem =
        CodeSystem.newBuilder()
            .setUrl(makeUri("foo-system"))
            .addConcept(makeConceptDefinition("foo1", "f1"))
            .addConcept(makeConceptDefinition("foo2", "f2"))
            .build();
    CodeSystem barSystem =
        CodeSystem.newBuilder()
            .setUrl(makeUri("bar-system"))
            .addConcept(makeConceptDefinition("bar1", "b1"))
            .addConcept(makeConceptDefinition("bar2", "b2"))
            .build();
    FhirPackage fhirPackage =
        new FhirPackage(
            Arrays.asList(), Arrays.asList(), Arrays.asList(fooSystem, barSystem), Arrays.asList());

    ValueSet.Builder valueSet = ValueSet.newBuilder();
    valueSet
        .getComposeBuilder()
        .addInclude(ValueSet.Compose.ConceptSet.newBuilder().setSystem(makeUri("foo-system")))
        .addInclude(ValueSet.Compose.ConceptSet.newBuilder().setSystem(makeUri("bar-system")));

    assertThat(new TerminologyExpander(fhirPackage).expandValueSet(valueSet.build()))
        .containsExactly(
            makeValueSetCode("foo1", "f1", "foo-system"),
            makeValueSetCode("foo2", "f2", "foo-system"),
            makeValueSetCode("bar1", "b1", "bar-system"),
            makeValueSetCode("bar2", "b2", "bar-system"));
  }

  @Test
  public void expandValueSet_withinAConceptSetAllConstraintsMustApply() throws Exception {
    CodeSystem fooSystem =
        CodeSystem.newBuilder()
            .setUrl(makeUri("foo-system"))
            .addConcept(makeConceptDefinition("foo1", "f1"))
            .addConcept(makeConceptDefinition("foo2", "f2"))
            .addConcept(makeConceptDefinition("foo3", "f3"))
            .addConcept(makeConceptDefinition("foo4", "f4"))
            .addConcept(makeConceptDefinition("foo5", "f5"))
            .addConcept(makeConceptDefinition("foo6", "f6"))
            .addConcept(makeConceptDefinition("foo7", "f7"))
            .build();

    ValueSet largeFoos =
        ValueSet.newBuilder()
            .setUrl(makeUri("large-foos"))
            .setCompose(
                ValueSet.Compose.newBuilder()
                    .addInclude(
                        ValueSet.Compose.ConceptSet.newBuilder()
                            .setSystem(makeUri("foo-system"))
                            .addConcept(makeConceptReference("foo4"))
                            .addConcept(makeConceptReference("foo5"))
                            .addConcept(makeConceptReference("foo6"))
                            .addConcept(makeConceptReference("foo7"))))
            .build();

    ValueSet evenFoos =
        ValueSet.newBuilder()
            .setUrl(makeUri("even-foos"))
            .setCompose(
                ValueSet.Compose.newBuilder()
                    .addInclude(
                        ValueSet.Compose.ConceptSet.newBuilder()
                            .setSystem(makeUri("foo-system"))
                            .addConcept(makeConceptReference("foo2"))
                            .addConcept(makeConceptReference("foo4"))
                            .addConcept(makeConceptReference("foo6"))))
            .build();

    FhirPackage fhirPackage =
        new FhirPackage(
            Arrays.asList(),
            Arrays.asList(),
            Arrays.asList(fooSystem),
            Arrays.asList(largeFoos, evenFoos));

    ValueSet.Builder largeEvenFoos = ValueSet.newBuilder();
    largeEvenFoos
        .getComposeBuilder()
        .addInclude(
            ValueSet.Compose.ConceptSet.newBuilder()
                .setSystem(makeUri("foo-system"))
                .addValueSet(Canonical.newBuilder().setValue("large-foos"))
                .addValueSet(Canonical.newBuilder().setValue("even-foos")));

    assertThat(new TerminologyExpander(fhirPackage).expandValueSet(largeEvenFoos.build()))
        .containsExactly(
            makeValueSetCode("foo4", "f4", "foo-system"),
            makeValueSetCode("foo6", "f6", "foo-system"));
  }
}
