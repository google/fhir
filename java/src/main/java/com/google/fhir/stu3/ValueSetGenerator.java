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
import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.Iterables;
import com.google.fhir.common.FhirVersion;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.proto.ProtoGeneratorAnnotations;
import com.google.fhir.r4.proto.BindingStrengthCode;
import com.google.fhir.r4.proto.Bundle;
import com.google.fhir.r4.proto.Code;
import com.google.fhir.r4.proto.CodeSystem.ConceptDefinition;
import com.google.fhir.r4.proto.ElementDefinition;
import com.google.fhir.r4.proto.ValueSet;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumValueDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldOptions;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileOptions;
import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

/** */
public class ValueSetGenerator {
  private final PackageInfo packageInfo;
  private final FhirVersion fhirVersion;
  private final Map<String, List<EnumValueDescriptorProto.Builder>> codesByUrl;
  private final Map<String, ValueSet> valueSetsByUrl;

  public ValueSetGenerator(PackageInfo packageInfo, Set<Bundle> valuesetBundles) {
    this.packageInfo = packageInfo;
    this.fhirVersion = FhirVersion.fromAnnotation(packageInfo.getFhirVersion());

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
    this.valueSetsByUrl =
        allEntries.stream()
            .filter(e -> e.getResource().hasValueSet())
            .map(e -> e.getResource().getValueSet())
            .collect(Collectors.toMap(vs -> vs.getUrl().getValue(), vs -> vs));
  }

  public FileDescriptorProto forCodesUsedIn(Set<Bundle> codeUsers, Set<Bundle> excludingCodesIn) {
    Set<ValueSet> valueSetsToGenerate = getValueSetsUsedInBundles(codeUsers);
    valueSetsToGenerate.removeAll(getValueSetsUsedInBundles(excludingCodesIn));
    return generateValueSetFile(valueSetsToGenerate);
  }

  public FileDescriptorProto generateValueSetFile() {
    return generateValueSetFile(valueSetsByUrl.values());
  }

  private FileDescriptorProto generateValueSetFile(Collection<ValueSet> valueSetsToGenerate) {
    FileDescriptorProto.Builder builder = FileDescriptorProto.newBuilder();
    builder.setPackage(packageInfo.getProtoPackage()).setSyntax("proto3");
    builder.addDependency(new File(fhirVersion.coreProtoImportRoot, "datatypes.proto").toString());
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
    for (ValueSet valueSet : valueSetsToGenerate) {
      messages.add(generateValueSetProto(valueSet));
    }

    messages.stream()
        .sorted((p1, p2) -> p1.getName().compareTo(p2.getName()))
        .forEach(proto -> builder.addMessageType(proto));

    return builder.build();
  }

  private DescriptorProto generateValueSetProto(ValueSet valueSet) {
    String valueSetName = getValueSetName(valueSet);
    DescriptorProto.Builder descriptor = DescriptorProto.newBuilder().setName(valueSetName);

    Optional<EnumDescriptorProto> valueEnumOptional = buildValueEnum(valueSet);
    if (valueEnumOptional.isPresent()) {
      descriptor
          .addEnumType(valueEnumOptional.get())
          .addField(
              FieldDescriptorProto.newBuilder()
                  .setNumber(1)
                  .setName("value")
                  .setTypeName(
                      "."
                          + packageInfo.getProtoPackage()
                          + "."
                          + valueSetName
                          + "."
                          + valueEnumOptional.get().getName())
                  .setType(FieldDescriptorProto.Type.TYPE_ENUM));
    } else {
      descriptor
          .addField(
              FieldDescriptorProto.newBuilder()
                  .setNumber(1)
                  .setOptions(
                      FieldOptions.newBuilder()
                          .setExtension(
                              ProtoGeneratorAnnotations.reservedReason,
                              "Field 1 reserved to allow enumeration in the future.")))
          .addField(
              FieldDescriptorProto.newBuilder()
                  .setNumber(4)
                  .setName("value")
                  .setType(FieldDescriptorProto.Type.TYPE_STRING)
                  .setOptions(
                      FieldOptions.newBuilder()
                          .setExtension(
                              ProtoGeneratorAnnotations.fieldDescription,
                              "This valueset is not enumerable, and so is represented as a"
                                  + " string.")));
    }
    descriptor
        .addField(
            FieldDescriptorProto.newBuilder()
                .setNumber(2)
                .setName("id")
                .setTypeName("." + fhirVersion.coreProtoPackage + ".String")
                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE))
        .addField(
            FieldDescriptorProto.newBuilder()
                .setNumber(3)
                .setName("extension")
                .setTypeName("." + fhirVersion.coreProtoPackage + ".Extension")
                .setLabel(FieldDescriptorProto.Label.LABEL_REPEATED)
                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE));

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
        .setExtension(ProtoGeneratorAnnotations.messageDescription, comment.toString())
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

  private Optional<EnumDescriptorProto> buildValueEnum(ValueSet valueSet) {
    String url = valueSet.getUrl().getValue();
    List<ValueSet.Compose.ConceptSet> includes = valueSet.getCompose().getIncludeList();
    if (!valueSet.getCompose().getExcludeList().isEmpty()) {
      printNoEnumWarning(url, "Excludes is not yet implemented");
      return Optional.empty();
    }
    EnumDescriptorProto.Builder builder = EnumDescriptorProto.newBuilder().setName("Value");
    builder.addValue(
        EnumValueDescriptorProto.newBuilder().setNumber(0).setName("INVALID_UNINITIALIZED"));
    int enumNumber = 1;
    Set<String> codes = new HashSet<>();
    for (ValueSet.Compose.ConceptSet conceptSet : includes) {
      if (!conceptSet.getValueSetList().isEmpty() || !conceptSet.getFilterList().isEmpty()) {
        printNoEnumWarning(url, "Complex ConceptSets are not yet implemented");
        return Optional.empty();
      }
      List<EnumValueDescriptorProto.Builder> enums = getEnumsForConceptSet(conceptSet);

      for (EnumValueDescriptorProto.Builder valueBuilder : enums) {
        if (valueBuilder == null) {
          printNoEnumWarning(
              url, "Unable to find all codes for system: " + conceptSet.getSystem().getValue());
          return Optional.empty();
        }
        if (!codes.add(valueBuilder.getName())) {
          printNoEnumWarning(url, "Valueset contains duplicate code");
          return Optional.empty();
        }
        builder.addValue(valueBuilder.setNumber(enumNumber++));
      }
    }
    if (builder.getValueCount() == 1) {
      // Note: 1 because we start by adding the INVALID_UNINITIALIZED code
      printNoEnumWarning(url, "no codes found");
      return Optional.empty();
    }
    return Optional.of(builder.build());
  }

  private static void printNoEnumWarning(String url, String warning) {
    System.out.println("Warning: Not generating enum for " + url + " - " + warning);
  }

  private List<EnumValueDescriptorProto.Builder> getEnumsForConceptSet(
      ValueSet.Compose.ConceptSet conceptSet) {
    String system = conceptSet.getSystem().getValue();
    boolean isKnownSystem = codesByUrl.containsKey(system);
    if (isKnownSystem) {
      if (conceptSet.getConceptList().isEmpty()) {
        // There is no explicit concept list, so default to all codes from that system.
        return codesByUrl.get(system);
      }
      // Only take the codes from that system that are explicitly listed.
      final Map<String, EnumValueDescriptorProto.Builder> valuesByCode =
          codesByUrl.get(system).stream()
              .collect(Collectors.toMap(c -> JsonFormat.getOriginalCode(c), c -> c));
      return conceptSet.getConceptList().stream()
          .map(concept -> valuesByCode.get(concept.getCode().getValue()))
          .collect(Collectors.toList());
    } else {
      return conceptSet.getConceptList().stream()
          .map(concept -> buildEnumValue(concept.getCode()))
          .collect(Collectors.toList());
    }
  }

  private static List<EnumValueDescriptorProto.Builder> buildEnumValues(
      List<ConceptDefinition> concepts) {
    List<EnumValueDescriptorProto.Builder> valueList = new ArrayList<>();
    for (ConceptDefinition concept : concepts) {
      valueList.add(buildEnumValue(concept.getCode()));
      valueList.addAll(buildEnumValues(concept.getConceptList()));
    }
    return valueList;
  }

  private static EnumValueDescriptorProto.Builder buildEnumValue(Code code) {
    String originalCode = code.getValue();
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
          .put("!=", "NOT_EQUAL_TO")
          .put("*", "STAR")
          .put("%", "PERCENT")
          .put("NaN", "NOT_A_NUMBER")
          .put("…", "DOT_DOT_DOT")
          .put("ANS+", "ANS_PLUS")
          .build();

  private static final Pattern ACRONYM_PATTERN = Pattern.compile("([A-Z])([A-Z]+)(?![a-z])");

  private static String toEnumCase(String rawCode) {
    if (CODE_RENAMES.containsKey(rawCode)) {
      return CODE_RENAMES.get(rawCode);
    }
    String sanitizedCode =
        rawCode
            .replaceAll("[',]", "")
            .replace('\u00c2' /* Â */, 'A')
            .replaceAll("[^A-Za-z0-9]", "_");
    if (sanitizedCode.startsWith("_")) {
      sanitizedCode = sanitizedCode.substring(1);
    }
    if (sanitizedCode.endsWith("_")) {
      sanitizedCode = sanitizedCode.substring(0, sanitizedCode.length() - 1);
    }
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

  // ValueSets that we hard-code to specific names, e.g., to avoid a colision.
  private static final ImmutableMap<String, String> VALUE_SET_RENAMES =
      ImmutableMap.of(
          "http://hl7.org/fhir/ValueSet/medication-statement-status", "MedicationStatementStatus");

  public String getValueSetName(ValueSet valueSet) {
    if (VALUE_SET_RENAMES.containsKey(valueSet.getUrl().getValue())) {
      return VALUE_SET_RENAMES.get(valueSet.getUrl().getValue());
    }
    String name = ProtoGenerator.getTypeNameFromId(valueSet.getName().getValue());
    return name.replaceAll("[^A-Za-z0-9]", "") + "Code";
  }

  private Set<ValueSet> getValueSetsUsedInBundles(Set<Bundle> bundles) {
    final Set<String> valueSetUrls = new HashSet<>();
    for (Bundle bundle : bundles) {
      for (Bundle.Entry entry : bundle.getEntryList()) {
        if (entry.getResource().hasStructureDefinition()) {
          entry.getResource().getStructureDefinition().getSnapshot().getElementList().stream()
              .map(element -> getBindingValueSetUrl(element))
              .filter(optionalUrl -> optionalUrl.isPresent())
              .forEach(
                  optionalUrl ->
                      valueSetUrls.add(
                          Iterables.get(Splitter.on('|').split(optionalUrl.get()), 0)));
        }
      }
    }
    return valueSetUrls.stream()
        .map(url -> valueSetsByUrl.get(url))
        .filter(vs -> vs != null)
        .collect(Collectors.toSet());
  }

  private static Optional<String> getBindingValueSetUrl(ElementDefinition element) {
    if (element.getBinding().getStrength().getValue() != BindingStrengthCode.Value.REQUIRED) {
      return Optional.empty();
    }
    String url = CanonicalWrapper.getUri(element.getBinding().getValueSet());
    return url.isEmpty() ? Optional.empty() : Optional.<String>of(url);
  }
}
