//    Copyright 2019 Google LLC.
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

package com.google.fhir.stu3;

import com.google.common.base.Ascii;
import com.google.common.base.CaseFormat;
import com.google.common.collect.ImmutableMap;
import com.google.fhir.common.FhirVersion;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.stu3.proto.Bundle;
import com.google.fhir.stu3.proto.CodeSystem.ConceptDefinition;
import com.google.fhir.stu3.proto.ValueSet;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumValueDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileOptions;
import java.io.File;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

/** */
public class ValueSetGenerator {
  private final PackageInfo packageInfo;
  private final String fhirProtoRootPath;
  private final Map<String, List<EnumValueDescriptorProto.Builder>> codesByUrl;
  private final List<ValueSet> valueSets;
  private final Set<String> valueSetsToSkip;

  public ValueSetGenerator(
      PackageInfo packageInfo,
      String fhirProtoRootPath,
      Set<Bundle> valuesetBundles,
      boolean includeCodesInDatatypes) {
    this.packageInfo = packageInfo;
    this.fhirProtoRootPath = fhirProtoRootPath;
    List<Bundle.Entry> allEntries =
        valuesetBundles.stream()
            .flatMap(bundle -> bundle.getEntryList().stream())
            .collect(Collectors.toList());
    this.codesByUrl =
        allEntries.stream()
            .filter(e -> e.getResource().hasCodeSystem())
            .map(e -> e.getResource().getCodeSystem())
            .collect(
                Collectors.toMap(
                    cs -> cs.getUrl().getValue(), cs -> buildEnumValues(cs.getConceptList())));
    this.valueSets =
        allEntries.stream()
            .filter(e -> e.getResource().hasValueSet())
            .map(e -> e.getResource().getValueSet())
            .collect(Collectors.toList());

    FhirVersion fhirVersion = FhirVersion.fromPackageInfo(packageInfo.getFhirVersion());
    valueSetsToSkip =
        includeCodesInDatatypes
            ? new HashSet<>()
            : ProtoGenerator.loadCodeTypesFromFile(fhirVersion.coreTypeMap.get("datatypes.proto"))
                .keySet();
  }

  public FileDescriptorProto generateValueSetFile() {
    FileDescriptorProto.Builder builder = FileDescriptorProto.newBuilder();
    builder.setPackage(packageInfo.getProtoPackage()).setSyntax("proto3");
    builder.addDependency(new File(fhirProtoRootPath, "datatypes.proto").toString());
    builder.addDependency(new File(ProtoGenerator.ANNOTATION_PATH, "annotations.proto").toString());
    FileOptions.Builder options = FileOptions.newBuilder();
    if (!packageInfo.getJavaProtoPackage().isEmpty()) {
      options.setJavaPackage(packageInfo.getJavaProtoPackage()).setJavaMultipleFiles(true);
    }
    if (!packageInfo.getGoProtoPackage().isEmpty()) {
      options.setGoPackage(packageInfo.getGoProtoPackage());
    }
    builder.setOptions(options);

    List<DescriptorProto> messages = new ArrayList<>();
    int unhandledTypes = 0;
    for (ValueSet valueSet : valueSets) {
      if (valueSet == null) {
        continue;
      }
      if (valueSetsToSkip.contains(valueSet.getUrl().getValue())) {
        continue;
      }
      try {
        messages.add(generateValueSetProto(valueSet));
      } catch (IllegalArgumentException e) {
        System.out.println(
            "Could not build ValueSet for " + valueSet.getId().getValue() + ": " + e.getMessage());
        unhandledTypes++;
      }
    }
    messages.stream()
        .sorted((p1, p2) -> p1.getName().compareTo(p2.getName()))
        .forEach(proto -> builder.addMessageType(proto));

    System.out.println("Unhandled ValueSets: " + unhandledTypes + "/" + valueSets.size());

    return builder.build();
  }

  private DescriptorProto generateValueSetProto(ValueSet valueSet) {
    String valueSetName = getValueSetName(valueSet);
    EnumDescriptorProto valueEnum = buildValueEnum(valueSet);
    DescriptorProto.Builder descriptor =
        DescriptorProto.newBuilder().setName(valueSetName).addEnumType(valueEnum);

    // Build a top-level message description.
    StringBuilder comment = new StringBuilder();
    addSentence(comment, valueSet.getDescription().getValue());
    comment.append("\nSee ").append(valueSet.getUrl().getValue());
    if (valueSet.getMeta().hasLastUpdated()) {
      comment
          .append("\nLast updated: ")
          .append(new InstantWrapper(valueSet.getMeta().getLastUpdated()));
    }

    descriptor
        .getOptionsBuilder()
        .setExtension(Annotations.messageDescription, comment.toString())
        .setExtension(
            Annotations.structureDefinitionKind,
            Annotations.StructureDefinitionKindValue.KIND_PRIMITIVE_TYPE)
        .setExtension(Annotations.fhirValuesetUrl, valueSet.getUrl().getValue());

    return descriptor.build();
  }

  private static void addSentence(StringBuilder sb, String s) {
    sb.append(s);
    if (!s.endsWith(".") && !s.endsWith("?") && !s.endsWith("!")) {
      sb.append(".");
    }
  }

  private EnumDescriptorProto buildValueEnum(ValueSet valueSet) {
    List<ValueSet.Compose.ConceptSet> includes = valueSet.getCompose().getIncludeList();
    if (!valueSet.getCompose().getExcludeList().isEmpty()) {
      throw new IllegalArgumentException("Excludes not yet implemented");
    }
    EnumDescriptorProto.Builder builder = EnumDescriptorProto.newBuilder().setName("Code");
    builder.addValue(
        EnumValueDescriptorProto.newBuilder().setNumber(0).setName("INVALID_UNINITIALIZED"));
    int enumNumber = 1;
    Set<String> codes = new HashSet<>();
    for (ValueSet.Compose.ConceptSet conceptSet : includes) {
      if (!conceptSet.getValueSetList().isEmpty() || !conceptSet.getFilterList().isEmpty()) {
        throw new IllegalArgumentException("Complex ConceptSets not yet implemented");
      }
      String system = conceptSet.getSystem().getValue();
      if (!codesByUrl.containsKey(system)) {
        throw new IllegalArgumentException("Unrecognized system: " + system);
      }
      List<EnumValueDescriptorProto.Builder> enums = codesByUrl.get(system);
      if (!conceptSet.getConceptList().isEmpty()) {
        final Map<String, EnumValueDescriptorProto.Builder> valuesByCode =
            codesByUrl.get(system).stream()
                .collect(Collectors.toMap(c -> JsonFormat.getOriginalCode(c), c -> c));
        enums =
            conceptSet.getConceptList().stream()
                .map(concept -> valuesByCode.get(concept.getCode().getValue()))
                .collect(Collectors.toList());
      }

      for (EnumValueDescriptorProto.Builder valueBuilder : enums) {
        if (valueBuilder == null) {
          throw new IllegalArgumentException("Unable to find codes for system: " + system);
        }
        if (!codes.add(valueBuilder.getName())) {
          throw new IllegalArgumentException("Valueset contains duplicate code");
        }
        builder.addValue(valueBuilder.setNumber(enumNumber++));
      }
    }
    if (builder.getValueCount() == 0) {
      throw new IllegalArgumentException("Invalid ValueSet: No codes found");
    }
    return builder.build();
  }

  private List<EnumValueDescriptorProto.Builder> buildEnumValues(List<ConceptDefinition> concepts) {
    List<EnumValueDescriptorProto.Builder> valueList = new ArrayList<>();
    for (ConceptDefinition concept : concepts) {
      if (concept.getConceptList().isEmpty()) {
        valueList.add(buildEnumValue(concept));
      } else {
        valueList.addAll(buildEnumValues(concept.getConceptList()));
      }
    }
    return valueList;
  }

  private EnumValueDescriptorProto.Builder buildEnumValue(ConceptDefinition concept) {
    String originalCode = concept.getCode().getValue();
    String enumCase = toEnumCase(originalCode);
    String fhirCase = JsonFormat.enumCodeToFhirCase(enumCase);

    EnumValueDescriptorProto.Builder builder =
        EnumValueDescriptorProto.newBuilder().setName(enumCase);
    if (!fhirCase.equals(originalCode)) {
      builder.getOptionsBuilder().setExtension(Annotations.fhirOriginalCode, originalCode);
    }
    return builder;
  }

  private static final ImmutableMap<String, String> CODE_RENAMES =
      ImmutableMap.<String, String>builder()
          .put("=", "EQUALS")
          .put("<", "LESS_THAN")
          .put("<=", "LESS_THAN_OR_EQUAL_TO")
          .put(">=", "GREATER_THAN_OR_EQUAL_TO")
          .put(">", "GREATER_THAN")
          .put("*", "STAR")
          .put("NaN", "NOT_A_NUMBER")
          .put("…", "DOT_DOT_DOT")
          .put("ANS+", "ANS_PLUS")
          .build();

  private static final Pattern ACRONYM_PATTERN = Pattern.compile("([A-Z])([A-Z]+)(?![a-z])");

  private String toEnumCase(String rawCode) {
    if (CODE_RENAMES.containsKey(rawCode)) {
      return CODE_RENAMES.get(rawCode);
    }
    String sanitizedCode =
        rawCode
            .replaceAll("[',]", "")
            .replace('\u00c2' /* Â */, 'A')
            .replaceAll("[^A-Za-z0-9]", "_");
    if (Character.isDigit(rawCode.charAt(0))) {
      return Ascii.toUpperCase("NUM_" + sanitizedCode);
    }
    // Don't change FOO into F_O_O
    if (sanitizedCode.equals(Ascii.toUpperCase(sanitizedCode))) {
      return sanitizedCode.replaceAll("__+", "_");
    }

    // Turn acronyms into single words, e.g., FHIR_is_GREAT -> Fhir_is_Great, so that it ultimately
    // becomes FHIR_IS_GREAT instead of F_H_I_R_IS_G_R_E_A_T
    Matcher matcher = ACRONYM_PATTERN.matcher(sanitizedCode);
    StringBuffer sb = new StringBuffer();
    while (matcher.find()) {
      matcher.appendReplacement(sb, matcher.group(1) + Ascii.toLowerCase(matcher.group(2)));
    }
    matcher.appendTail(sb);
    sanitizedCode = sb.toString();

    return CaseFormat.LOWER_CAMEL
        .to(CaseFormat.UPPER_UNDERSCORE, sanitizedCode)
        .replaceAll("__+", "_");
  }

  public String getValueSetName(ValueSet valueSet) {
    String name = ProtoGenerator.getTypeNameFromId(valueSet.getName().getValue());
    return name.replaceAll("[',]", "").replaceAll("[^A-Za-z0-9]", "_");
  }
}
