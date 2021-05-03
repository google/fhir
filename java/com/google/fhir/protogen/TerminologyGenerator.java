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

package com.google.fhir.protogen;

import static java.util.stream.Collectors.toList;

import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.common.ResourceValidator;
import com.google.fhir.proto.CodeSystemConfig;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.proto.Terminologies;
import com.google.fhir.proto.ValueSetConfig;
import com.google.fhir.r4.core.Bundle;
import com.google.fhir.r4.core.BundleTypeCode;
import com.google.fhir.r4.core.Code;
import com.google.fhir.r4.core.CodeSystem;
import com.google.fhir.r4.core.CodeSystemContentModeCode;
import com.google.fhir.r4.core.DateTime;
import com.google.fhir.r4.core.PropertyTypeCode;
import com.google.fhir.r4.core.ValueSet;
import java.time.LocalDate;

/**
 * Generator for creating CodeSystem and ValueSet resources from Terminologies protos from
 * profile_config.proto
 */
final class TerminologyGenerator {
  private final PackageInfo packageInfo;
  private final DateTime creationDateTime;
  private final ResourceValidator resourceValidator = new ResourceValidator();

  // From http://hl7.org/fhir/concept-properties to tag code values as deprecated
  public static final String CODE_VALUE_STATUS_PROPERTY =
      "http://hl7.org/fhir/concept-properties#status";
  public static final String CODE_VALUE_STATUS = "status";
  public static final String CODE_VALUE_STATUS_DEPRECATED = "deprecated";

  TerminologyGenerator(PackageInfo packageInfo, LocalDate creationTime) {
    this.packageInfo = packageInfo;
    this.creationDateTime = GeneratorUtils.buildCreationDateTime(creationTime);
  }

  Bundle generateTerminologies(Terminologies terminologies) {
    Bundle.Builder bundle = Bundle.newBuilder();
    bundle.getTypeBuilder().setValue(BundleTypeCode.Value.COLLECTION);
    for (CodeSystemConfig codeSystemConfig : terminologies.getCodeSystemList()) {
      bundle
          .addEntryBuilder()
          .getResourceBuilder()
          .setCodeSystem(buildCodeSystem(codeSystemConfig));
    }
    for (ValueSetConfig valueSetConfig : terminologies.getValueSetList()) {
      bundle.addEntryBuilder().getResourceBuilder().setValueSet(buildValueSet(valueSetConfig));
    }

    try {
      new ResourceValidator().validateResource(bundle);
    } catch (InvalidFhirException e) {
      throw new IllegalArgumentException(e);
    }

    return bundle.build();
  }

  private CodeSystem buildCodeSystem(CodeSystemConfig config) {
    CodeSystem.Builder builder = CodeSystem.newBuilder();
    builder.getNameBuilder().setValue(config.getName());
    builder.getIdBuilder().setValue(config.getName());
    builder.getTitleBuilder().setValue(config.getName());
    if (!config.getDescription().isEmpty()) {
      builder.getDescriptionBuilder().setValue(config.getDescription());
    }
    builder.getStatusBuilder().setValue(config.getStatus());
    builder.setDate(creationDateTime);
    builder.getPublisherBuilder().setValue(packageInfo.getPublisher());
    builder.getContentBuilder().setValue(CodeSystemContentModeCode.Value.COMPLETE);

    String url =
        config.getUrlOverride().isEmpty()
            ? packageInfo.getBaseUrl() + "/" + config.getName()
            : config.getUrlOverride();
    builder.getUrlBuilder().setValue(url);

    // Declare the presence of the deprecated property on this value set so it can be
    // used on individual concepts.
    CodeSystem.Property.Builder propertyBuilder = builder.addPropertyBuilder();
    propertyBuilder.getUriBuilder().setValue(CODE_VALUE_STATUS_PROPERTY);
    propertyBuilder.getCodeBuilder().setValue(CODE_VALUE_STATUS);
    propertyBuilder.getTypeBuilder().setValue(PropertyTypeCode.Value.CODE);

    for (CodeSystemConfig.Concept concept : config.getConceptList()) {
      addConcept(builder, concept);
    }

    CodeSystem codeSystem = builder.build();
    try {
      resourceValidator.validateResource(codeSystem);
    } catch (InvalidFhirException e) {
      System.out.println("Invalid Generated CodeSystem: " + e.getMessage());
      System.out.println(codeSystem);
      throw new IllegalArgumentException(e);
    }
    return codeSystem;
  }

  private static void addConcept(CodeSystem.Builder builder, CodeSystemConfig.Concept concept) {
    CodeSystem.ConceptDefinition.Builder conceptBuilder = builder.addConceptBuilder();
    conceptBuilder.getCodeBuilder().setValue(concept.getCode());
    if (!concept.getDisplay().isEmpty()) {
      conceptBuilder.getDisplayBuilder().setValue(concept.getDisplay());
    }
    if (!concept.getDefinition().isEmpty()) {
      conceptBuilder.getDefinitionBuilder().setValue(concept.getDefinition());
    }

    if (concept.getDeprecated()) {
      CodeSystem.ConceptDefinition.ConceptProperty.Builder propertyBuilder =
          conceptBuilder.addPropertyBuilder();
      propertyBuilder.getCodeBuilder().setValue(CODE_VALUE_STATUS);
      propertyBuilder.getValueBuilder().getCodeBuilder().setValue(CODE_VALUE_STATUS_DEPRECATED);
    }

    // TODO: handle child concepts.
  }

  private ValueSet buildValueSet(ValueSetConfig config) {
    ValueSet.Builder builder = ValueSet.newBuilder();

    builder.getNameBuilder().setValue(config.getName());
    builder.getIdBuilder().setValue(config.getName());
    builder.getTitleBuilder().setValue(config.getName());
    if (!config.getDescription().isEmpty()) {
      builder.getDescriptionBuilder().setValue(config.getDescription());
    }
    builder.getStatusBuilder().setValue(config.getStatus());
    builder.setDate(creationDateTime);
    builder.getPublisherBuilder().setValue(packageInfo.getPublisher());

    String url =
        config.getUrlOverride().isEmpty()
            ? packageInfo.getBaseUrl() + "/ValueSet/" + config.getName()
            : config.getUrlOverride();
    builder.getUrlBuilder().setValue(url);

    ValueSet.Compose.Builder composeBuilder = builder.getComposeBuilder();

    for (ValueSetConfig.System system : config.getSystemList()) {
      ValueSet.Compose.ConceptSet.Builder includeBuilder = composeBuilder.addIncludeBuilder();
      includeBuilder.getSystemBuilder().setValue(system.getUrl());

      if (system.getIncludeCount() > 0 && system.getExcludeCount() > 0) {
        throw new IllegalArgumentException(
            "Error generating ValueSet `"
                + url
                + "`:  Cannot specify both `include` and `exclude` lists.");
      }

      includeBuilder.addAllConcept(
          system.getIncludeList().stream()
              .map(
                  code ->
                      ValueSet.Compose.ConceptSet.ConceptReference.newBuilder()
                          .setCode(Code.newBuilder().setValue(code))
                          .build())
              .collect(toList()));

      if (system.getExcludeCount() > 0) {
        ValueSet.Compose.ConceptSet.Builder excludeBuilder = composeBuilder.addExcludeBuilder();
        excludeBuilder.getSystemBuilder().setValue(system.getUrl());
        excludeBuilder.addAllConcept(
            system.getExcludeList().stream()
                .map(
                    code ->
                        ValueSet.Compose.ConceptSet.ConceptReference.newBuilder()
                            .setCode(Code.newBuilder().setValue(code))
                            .build())
                .collect(toList()));
      }
    }

    return builder.build();
  }
}
