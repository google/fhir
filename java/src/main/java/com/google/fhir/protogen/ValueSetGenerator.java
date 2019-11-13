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

package com.google.fhir.protogen;

import com.google.common.base.Ascii;
import com.google.common.base.CaseFormat;
import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.Iterables;
import com.google.fhir.common.AnnotationUtils;
import com.google.fhir.common.FhirVersion;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.proto.ProtoGeneratorAnnotations;
import com.google.fhir.protogen.GeneratorUtils.QualifiedType;
import com.google.fhir.r4.core.BindingStrengthCode;
import com.google.fhir.r4.core.Bundle;
import com.google.fhir.r4.core.Code;
import com.google.fhir.r4.core.CodeSystem;
import com.google.fhir.r4.core.CodeSystem.ConceptDefinition;
import com.google.fhir.r4.core.Coding;
import com.google.fhir.r4.core.ElementDefinition;
import com.google.fhir.r4.core.FilterOperatorCode;
import com.google.fhir.r4.core.ValueSet;
import com.google.fhir.r4.core.ValueSet.Compose.ConceptSet.Filter;
import com.google.fhir.wrappers.CanonicalWrapper;
import com.google.fhir.wrappers.CodeWrapper;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumValueDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldOptions;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileOptions;
import com.google.protobuf.Descriptors.Descriptor;
import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
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
  private final Map<String, CodeSystem> codeSystemsByUrl;
  private final Map<String, ValueSet> valueSetsByUrl;
  private final Map<String, String> protoTypesByUrl;

  public ValueSetGenerator(PackageInfo packageInfo, Set<FhirPackage> fhirPackages) {
    this.packageInfo = packageInfo;
    this.fhirVersion = FhirVersion.fromAnnotation(packageInfo.getFhirVersion());

    this.codeSystemsByUrl =
        fhirPackages.stream()
            .flatMap(p -> p.codeSystems.stream())
            .collect(Collectors.toMap(cs -> cs.getUrl().getValue(), cs -> cs));

    this.valueSetsByUrl =
        fhirPackages.stream()
            .flatMap(p -> p.valueSets.stream())
            .collect(Collectors.toMap(vs -> vs.getUrl().getValue(), vs -> vs));

    // Make a map from url to proto type for each type we can inline.
    // This is assumed to be all the codes from all the included FhirPackages,
    // WITH THE EXCEPTION of the core fhir package.
    // This is due to the fact that there are prohibitively many codes defined in the core package
    // to generate them all - so instead we inspect the code files listed in FhirVersion to know
    this.protoTypesByUrl = new HashMap<>();
    for (final FhirPackage fhirPackage : fhirPackages) {
      if (!fhirPackage.packageInfo.getProtoPackage().equals(fhirVersion.coreProtoPackage)) {
        String packageString = "." + fhirPackage.packageInfo.getProtoPackage() + ".";
        protoTypesByUrl.putAll(
            fhirPackage.codeSystems.stream()
                .collect(
                    Collectors.toMap(
                        cs -> cs.getUrl().getValue(),
                        cs -> packageString + getCodeSystemName(cs))));
        protoTypesByUrl.putAll(
            fhirPackage.valueSets.stream()
                .collect(
                    Collectors.toMap(
                        vs -> vs.getUrl().getValue(), vs -> packageString + getValueSetName(vs))));
      }
    }

    // Add codes defined on core types proto
    this.protoTypesByUrl.putAll(
        fhirVersion.codeTypeList.stream()
            .flatMap(file -> file.getMessageTypes().stream())
            .filter(descriptor -> isEnumDefinition(descriptor))
            .collect(
                Collectors.toMap(
                    descriptor -> getEnumDefiningUrl(descriptor),
                    descriptor ->
                        "." + fhirVersion.coreProtoPackage + "." + descriptor.getName())));
  }

  private static boolean isEnumDefinition(Descriptor descriptor) {
    return descriptor.getFields().isEmpty() && descriptor.getEnumTypes().size() == 1;
  }

  private static String getEnumDefiningUrl(Descriptor descriptor) {
    String codeSystem = AnnotationUtils.getFhirCodeSystemUrl(descriptor.getEnumTypes().get(0));
    if (!codeSystem.isEmpty()) {
      return codeSystem;
    }
    String valueSet = AnnotationUtils.getEnumValuesetUrl(descriptor.getEnumTypes().get(0));
    if (!valueSet.isEmpty()) {
      return valueSet;
    }
    throw new IllegalArgumentException(
        "No Enum Defining Url found for: " + descriptor.getFullName());
  }

  public FileDescriptorProto generateCodeSystemAndValueSetsFile(FhirPackage fhirPackage) {
    return generateCodeSystemFile(fhirPackage).toBuilder()
        .addAllMessageType(generateValueSetFile(fhirPackage).getMessageTypeList())
        .build();
  }

  public FileDescriptorProto forCodesUsedIn(Collection<Bundle> codeUsers, boolean eagerMode) {
    Set<CodeSystem> codeSystemsToGenerate = getCodeSystemsUsedInBundles(codeUsers, eagerMode);
    return generateCodeSystemFile(codeSystemsToGenerate);
  }

  public FileDescriptorProto forValueSetsUsedIn(Collection<Bundle> valueSetUsers) {
    Set<ValueSet> valueSetsToGenerate = getValueSetsUsedInBundles(valueSetUsers);
    return generateValueSetFile(valueSetsToGenerate);
  }

  public FileDescriptorProto generateCodeSystemFile(FhirPackage fhirPackage) {
    return generateCodeSystemFile(fhirPackage.codeSystems);
  }

  private FileDescriptorProto generateCodeSystemFile(Collection<CodeSystem> codeSystemsToGenerate) {
    FileDescriptorProto.Builder builder = FileDescriptorProto.newBuilder();
    builder.setPackage(packageInfo.getProtoPackage()).setSyntax("proto3");
    builder.addDependency(new File(FhirVersion.ANNOTATION_PATH, "annotations.proto").toString());
    FileOptions.Builder options = FileOptions.newBuilder();
    if (!packageInfo.getJavaProtoPackage().isEmpty()) {
      options.setJavaPackage(packageInfo.getJavaProtoPackage()).setJavaMultipleFiles(true);
    }
    if (!packageInfo.getGoProtoPackage().isEmpty()) {
      options.setGoPackage(packageInfo.getGoProtoPackage());
    }
    builder.setOptions(options);
    List<DescriptorProto> messages = new ArrayList<>();
    for (CodeSystem codeSystem : codeSystemsToGenerate) {
      messages.add(generateCodeSystemProto(codeSystem));
    }

    messages.stream()
        .sorted((p1, p2) -> p1.getName().compareTo(p2.getName()))
        .forEach(proto -> builder.addMessageType(proto));

    return builder.build();
  }

  public FileDescriptorProto generateValueSetFile(FhirPackage fhirPackage) {
    return generateValueSetFile(fhirPackage.valueSets);
  }

  private FileDescriptorProto generateValueSetFile(Collection<ValueSet> valueSetsToGenerate) {
    FileDescriptorProto.Builder builder = FileDescriptorProto.newBuilder();
    builder.setPackage(packageInfo.getProtoPackage()).setSyntax("proto3");
    builder.addDependency(new File(FhirVersion.ANNOTATION_PATH, "annotations.proto").toString());
    FileOptions.Builder options = FileOptions.newBuilder();
    if (!packageInfo.getJavaProtoPackage().isEmpty()) {
      options.setJavaPackage(packageInfo.getJavaProtoPackage()).setJavaMultipleFiles(true);
    }
    if (!packageInfo.getGoProtoPackage().isEmpty()) {
      options.setGoPackage(packageInfo.getGoProtoPackage());
    }
    builder.setOptions(options);
    valueSetsToGenerate.stream()
        .filter(vs -> !getOneToOneCodeSystem(vs).isPresent())
        .map(vs -> generateValueSetProto(vs))
        .filter(op -> op.isPresent())
        .map(op -> op.get())
        .sorted((p1, p2) -> p1.getName().compareTo(p2.getName()))
        .forEach(proto -> builder.addMessageType(proto));

    return builder.build();
  }

  private DescriptorProto generateCodeSystemProto(CodeSystem codeSystem) {
    String codeSystemName = getCodeSystemName(codeSystem);
    String url = codeSystem.getUrl().getValue();
    DescriptorProto.Builder descriptor = DescriptorProto.newBuilder().setName(codeSystemName);

    // Build a top-level message description.
    String comment =
        codeSystem.getDescription().getValue() + "\nSee " + codeSystem.getUrl().getValue();
    descriptor
        .getOptionsBuilder()
        .setExtension(ProtoGeneratorAnnotations.messageDescription, comment);

    if (!codeSystemsByUrl.containsKey(url)) {
      throw new IllegalArgumentException("Unrecognized CodeSystem: " + url);
    }

    return descriptor.addEnumType(generateCodeSystemEnum(codeSystem)).build();
  }

  private Optional<DescriptorProto> generateValueSetProto(ValueSet valueSet) {
    String valueSetName = getValueSetName(valueSet);
    String url = valueSet.getUrl().getValue();
    DescriptorProto.Builder descriptor = DescriptorProto.newBuilder().setName(valueSetName);

    // Build a top-level message description.
    String comment = valueSet.getDescription().getValue() + "\nSee " + url;
    descriptor
        .getOptionsBuilder()
        .setExtension(ProtoGeneratorAnnotations.messageDescription, comment);

    if (!valueSetsByUrl.containsKey(url)) {
      throw new IllegalArgumentException("Unrecognized CodeSystem: " + url);
    }

    Optional<EnumDescriptorProto> valueSetEnum = generateValueSetEnum(valueSet);
    if (!valueSetEnum.isPresent()) {
      return Optional.empty();
    }
    return Optional.of(descriptor.addEnumType(valueSetEnum.get()).build());
  }

  private static EnumDescriptorProto generateCodeSystemEnum(CodeSystem codeSystem) {
    String url = codeSystem.getUrl().getValue();
    EnumDescriptorProto.Builder enumDescriptor = EnumDescriptorProto.newBuilder();
    enumDescriptor
        .setName("Value")
        .addValue(
            EnumValueDescriptorProto.newBuilder().setNumber(0).setName("INVALID_UNINITIALIZED"));

    int enumNumber = 1;
    for (EnumValueDescriptorProto.Builder enumValue :
        buildEnumValues(
            codeSystem,
            null /* don't include system annotation */,
            new ArrayList<>() /* no filters */)) {
      enumDescriptor.addValue(enumValue.setNumber(enumNumber++));
    }

    enumDescriptor.getOptionsBuilder().setExtension(Annotations.fhirCodeSystemUrl, url);
    return enumDescriptor.build();
  }

  private DescriptorProto generateCodeBoundToCodeSystem(String url, QualifiedType qualifiedType) {
    DescriptorProto.Builder descriptor =
        DescriptorProto.newBuilder().setName(qualifiedType.getName());

    descriptor
        .getOptionsBuilder()
        .setExtension(
            Annotations.structureDefinitionKind,
            Annotations.StructureDefinitionKindValue.KIND_PRIMITIVE_TYPE)
        .addExtension(
            Annotations.fhirProfileBase,
            AnnotationUtils.getStructureDefinitionUrl(Code.getDescriptor()))
        .setExtension(Annotations.fhirFixedSystem, url);

    FieldDescriptorProto.Builder enumField = descriptor.addFieldBuilder().setNumber(1);

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

    if (!codeSystemsByUrl.containsKey(url)) {
      enumField.setOptions(
          FieldOptions.newBuilder()
              .setExtension(
                  ProtoGeneratorAnnotations.reservedReason,
                  "Field 1 reserved to allow enumeration in the future."));
      descriptor.addField(
          FieldDescriptorProto.newBuilder()
              .setNumber(4)
              .setName("value")
              .setType(FieldDescriptorProto.Type.TYPE_STRING)
              .setOptions(
                  FieldOptions.newBuilder()
                      .setExtension(
                          ProtoGeneratorAnnotations.fieldDescription,
                          "This CodeSystem is not enumerable, and so is represented as a"
                              + " string.")));

      return descriptor.build();
    }

    CodeSystem codeSystem = codeSystemsByUrl.get(url);

    if (protoTypesByUrl.containsKey(url)) {
      enumField
          .setName("value")
          .setType(FieldDescriptorProto.Type.TYPE_ENUM)
          .setTypeName(protoTypesByUrl.get(url) + ".Value");
      return descriptor.build();
    }

    EnumDescriptorProto codeEnum = generateCodeSystemEnum(codeSystem);
    descriptor.addEnumType(codeEnum);
    enumField
        .setName("value")
        .setTypeName(
            "." + qualifiedType.packageName + "." + qualifiedType.type + "." + codeEnum.getName())
        .setType(FieldDescriptorProto.Type.TYPE_ENUM);
    return descriptor.build();
  }

  public DescriptorProto generateCodeBoundToValueSet(String url, QualifiedType qualifiedType) {
    if (!valueSetsByUrl.containsKey(url)) {
      throw new IllegalArgumentException("Encountered unrecognized ValueSet url: " + url);
    }

    ValueSet valueSet = valueSetsByUrl.get(url);
    DescriptorProto.Builder descriptor =
        DescriptorProto.newBuilder().setName(qualifiedType.getName());

    FieldDescriptorProto.Builder enumField = descriptor.addFieldBuilder().setNumber(1);

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

    descriptor
        .getOptionsBuilder()
        .setExtension(
            Annotations.structureDefinitionKind,
            Annotations.StructureDefinitionKindValue.KIND_PRIMITIVE_TYPE)
        .addExtension(
            Annotations.fhirProfileBase,
            AnnotationUtils.getStructureDefinitionUrl(Code.getDescriptor()))
        .setExtension(Annotations.fhirValuesetUrl, valueSet.getUrl().getValue());

    if (protoTypesByUrl.containsKey(url)) {
      enumField
          .setName("value")
          .setType(FieldDescriptorProto.Type.TYPE_ENUM)
          .setTypeName(protoTypesByUrl.get(url) + ".Value");
      return descriptor.build();
    }

    Optional<CodeSystem> oneToOneCodeSystem = getOneToOneCodeSystem(valueSet);
    if (oneToOneCodeSystem.isPresent()
        && protoTypesByUrl.containsKey(oneToOneCodeSystem.get().getUrl().getValue())) {
      enumField
          .setName("value")
          .setType(FieldDescriptorProto.Type.TYPE_ENUM)
          .setTypeName(
              protoTypesByUrl.get(oneToOneCodeSystem.get().getUrl().getValue()) + ".Value");
      return descriptor.build();
    }

    Optional<EnumDescriptorProto> valueEnumOptional = generateValueSetEnum(valueSet);
    if (valueEnumOptional.isPresent()) {
      descriptor.addEnumType(valueEnumOptional.get());
      enumField
          .setName("value")
          .setTypeName(
              "."
                  + qualifiedType.packageName
                  + "."
                  + qualifiedType.type
                  + "."
                  + valueEnumOptional.get().getName())
          .setType(FieldDescriptorProto.Type.TYPE_ENUM);
      return descriptor.build();
    }

    enumField.setOptions(
        FieldOptions.newBuilder()
            .setExtension(
                ProtoGeneratorAnnotations.reservedReason,
                "Field 1 reserved to allow enumeration in the future."));
    descriptor.addField(
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

    return descriptor.build();
  }

  DescriptorProto generateCodingWithBoundValueSet(String valueSetUrl, QualifiedType qualifiedType) {
    DescriptorProto.Builder codingMessage = DescriptorProto.newBuilder();
    codingMessage.setName(qualifiedType.getName());
    codingMessage
        .getOptionsBuilder()
        .addExtension(
            Annotations.fhirProfileBase,
            AnnotationUtils.getStructureDefinitionUrl(Coding.getDescriptor()));

    List<FieldDescriptorProto> codingFields = new ArrayList<>();
    // Add in all coding fields that aren't code or system
    for (FieldDescriptorProto field : Coding.getDescriptor().toProto().getFieldList()) {
      if (!field.getName().equals("code") && !field.getName().equals("system")) {
        codingFields.add(field);
      }
    }

    QualifiedType codeType = qualifiedType.childType("BoundCode");
    codingMessage.addNestedType(generateCodeBoundToValueSet(valueSetUrl, codeType));

    codingFields.add(
        FieldDescriptorProto.newBuilder()
            .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
            .setTypeName(codeType.toQualifiedTypeString())
            .setName("code")
            .setNumber(5)
            .build());
    codingMessage.addAllField(
        codingFields.stream()
            .sorted((a, b) -> a.getNumber() - b.getNumber())
            .collect(Collectors.toList()));

    return codingMessage.build();
  }

  DescriptorProto generateCodingWithFixedCodeSystem(
      String codeSystemUrl, QualifiedType qualifiedType) {
    DescriptorProto.Builder codingMessage = DescriptorProto.newBuilder();
    codingMessage.setName(qualifiedType.getName());
    codingMessage
        .getOptionsBuilder()
        .addExtension(
            Annotations.fhirProfileBase,
            AnnotationUtils.getStructureDefinitionUrl(Coding.getDescriptor()));

    List<FieldDescriptorProto> codingFields = new ArrayList<>();
    // Add in all coding fields that aren't code or system
    for (FieldDescriptorProto field : Coding.getDescriptor().toProto().getFieldList()) {
      if (!field.getName().equals("code") && !field.getName().equals("system")) {
        codingFields.add(field);
      }
    }

    QualifiedType codeType = qualifiedType.childType("FixedCode");
    codingMessage.addNestedType(generateCodeBoundToCodeSystem(codeSystemUrl, codeType));

    codingFields.add(
        FieldDescriptorProto.newBuilder()
            .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
            .setTypeName(codeType.toQualifiedTypeString())
            .setName("code")
            .setNumber(5)
            .build());
    codingMessage.addAllField(
        codingFields.stream()
            .sorted((a, b) -> a.getNumber() - b.getNumber())
            .collect(Collectors.toList()));

    return codingMessage.build();
  }

  private Optional<EnumDescriptorProto> generateValueSetEnum(ValueSet valueSet) {
    String url = valueSet.getUrl().getValue();
    List<ValueSet.Compose.ConceptSet> includes = valueSet.getCompose().getIncludeList();
    Map<String, ValueSet.Compose.ConceptSet> excludesBySystem =
        valueSet.getCompose().getExcludeList().stream()
            .collect(
                Collectors.toMap(exclude -> exclude.getSystem().getValue(), exclude -> exclude));

    EnumDescriptorProto.Builder builder = EnumDescriptorProto.newBuilder().setName("Value");
    builder.getOptionsBuilder().setExtension(Annotations.enumValuesetUrl, url);
    builder.addValue(
        EnumValueDescriptorProto.newBuilder().setNumber(0).setName("INVALID_UNINITIALIZED"));
    int enumNumber = 1;
    for (ValueSet.Compose.ConceptSet conceptSet : includes) {
      if (!conceptSet.getValueSetList().isEmpty()) {
        printNoEnumWarning(url, "Complex ConceptSets are not yet implemented");
        return Optional.empty();
      }
      List<EnumValueDescriptorProto.Builder> enums =
          getEnumsForValueConceptSet(
              conceptSet,
              excludesBySystem.getOrDefault(
                  conceptSet.getSystem().getValue(),
                  ValueSet.Compose.ConceptSet.getDefaultInstance()));

      for (EnumValueDescriptorProto.Builder valueBuilder : enums) {
        if (valueBuilder == null) {
          printNoEnumWarning(
              url, "Unable to find all codes for system: " + conceptSet.getSystem().getValue());
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

  private static class EnumValueWithBackupName {
    final EnumValueDescriptorProto.Builder enumValue;
    final String backupName;

    EnumValueWithBackupName(EnumValueDescriptorProto.Builder enumValue, String backupName) {
      this.enumValue = enumValue;
      this.backupName = backupName;
    }

    String originalName() {
      return enumValue.getName();
    }

    EnumValueDescriptorProto.Builder get(boolean withBackupName) {
      if (!withBackupName) {
        return enumValue;
      }
      return enumValue.setName(backupName);
    }
  }

  private static List<EnumValueDescriptorProto.Builder> toEnumValueList(
      List<EnumValueWithBackupName> enumsWithBackup) {
    Set<String> usedNames = new HashSet<>();
    Set<String> duplicatedNames = new HashSet<>();
    for (EnumValueWithBackupName enumWithBackup : enumsWithBackup) {
      if (!usedNames.add(enumWithBackup.originalName())) {
        duplicatedNames.add(enumWithBackup.originalName());
      }
    }
    List<EnumValueDescriptorProto.Builder> enumList = new ArrayList<>();
    Set<String> finalUsedNames = new HashSet<>();
    for (EnumValueWithBackupName enumWithBackup : enumsWithBackup) {
      EnumValueDescriptorProto.Builder finalEnum =
          enumWithBackup.get(duplicatedNames.contains(enumWithBackup.originalName()));
      if (finalUsedNames.add(finalEnum.getName())) {
        enumList.add(finalEnum);
      } else if (finalEnum
          .getOptions()
          .getExtension(Annotations.sourceCodeSystem)
          .equals("http://hl7.org/fhir/sid/ndc")) {
        // This valueset has duplicate codes :( ignore.
      } else {
        throw new IllegalArgumentException("Found duplicate code: " + finalEnum);
      }
    }
    return enumList;
  }

  private List<EnumValueDescriptorProto.Builder> getEnumsForValueConceptSet(
      ValueSet.Compose.ConceptSet conceptSet, ValueSet.Compose.ConceptSet excludeSet) {
    String system = conceptSet.getSystem().getValue();
    boolean isKnownSystem = codeSystemsByUrl.containsKey(system);
    Set<String> excludeCodes =
        excludeSet.getConceptList().stream()
            .map(concept -> concept.getCode().getValue())
            .collect(Collectors.toSet());
    if (isKnownSystem) {
      if (conceptSet.getConceptList().isEmpty()) {
        // There is no explicit concept list, so default to all codes from that system that aren't
        // in the excludes set.
        return buildEnumValues(codeSystemsByUrl.get(system), system, conceptSet.getFilterList())
            .stream()
            .filter(enumValue -> !excludeCodes.contains(CodeWrapper.getOriginalCode(enumValue)))
            .collect(Collectors.toList());
      }
      // Only take the codes from that system that are explicitly listed.
      final Map<String, EnumValueDescriptorProto.Builder> valuesByCode =
          buildEnumValues(codeSystemsByUrl.get(system), system, conceptSet.getFilterList()).stream()
              .collect(Collectors.toMap(c -> JsonFormat.getOriginalCode(c), c -> c));
      return conceptSet.getConceptList().stream()
          .map(concept -> valuesByCode.get(concept.getCode().getValue()))
          .collect(Collectors.toList());
    } else {
      return toEnumValueList(
          conceptSet.getConceptList().stream()
              .map(
                  concept ->
                      buildEnumValue(concept.getCode(), concept.getDisplay().getValue(), system))
              .collect(Collectors.toList()));
    }
  }

  private static List<EnumValueDescriptorProto.Builder> buildEnumValues(
      CodeSystem codeSystem, String system, List<Filter> filters) {
    filters =
        filters.stream()
            .filter(
                filter -> {
                  if (filter.getOp().getValue() != FilterOperatorCode.Value.IS_A) {
                    System.out.println(
                        "Warning: value filters other than is-a are ignored.  Found: "
                            + CodeWrapper.getOriginalCode(
                                filter.getOp().getValue().getValueDescriptor().toProto()));
                    return false;
                  }
                  if (!filter.getProperty().getValue().equals("concept")) {
                    System.out.println(
                        "Warning: value filters by property other than concept are not supported. "
                            + " Found: "
                            + filter.getProperty().getValue());
                    return false;
                  }
                  return true;
                })
            .collect(Collectors.toList());

    return toEnumValueList(
        buildEnumValues(codeSystem.getConceptList(), system, new HashSet<>(), filters));
  }

  private static List<EnumValueWithBackupName> buildEnumValues(
      List<ConceptDefinition> concepts,
      String system,
      Set<String> classifications,
      List<Filter> filters) {
    List<EnumValueWithBackupName> valueList = new ArrayList<>();
    for (ConceptDefinition concept : concepts) {
      if (conceptMatchesFilters(concept, classifications, filters)) {
        valueList.add(buildEnumValue(concept.getCode(), concept.getDisplay().getValue(), system));
      }

      Set<String> childClassifications = new HashSet<>(classifications);
      childClassifications.add(concept.getCode().getValue());
      valueList.addAll(
          buildEnumValues(concept.getConceptList(), system, childClassifications, filters));
    }
    return valueList;
  }

  private static boolean conceptMatchesFilters(
      ConceptDefinition concept, Set<String> classifications, List<Filter> filters) {
    if (filters.isEmpty()) {
      return true;
    }
    for (Filter filter : filters) {
      if (conceptMatchesFilter(concept, classifications, filter)) {
        return true;
      }
    }
    return false;
  }

  // See http://hl7.org/fhir/valueset-filter-operator.html
  private static boolean conceptMatchesFilter(
      ConceptDefinition concept, Set<String> classifications, Filter filter) {
    String codeString = concept.getCode().getValue();
    String filterValue = filter.getValue().getValue();
    switch (filter.getOp().getValue()) {
      case EQUALS:
        return codeString.equals(filterValue);
      case IS_A:
        return codeString.equals(filterValue) || classifications.contains(filterValue);
      case DESCENDENT_OF:
        return classifications.contains(codeString);
      case IS_NOT_A:
        return !codeString.equals(filterValue);
      case REGEX:
        return codeString.matches(filterValue);
      case IN:
        return Splitter.on(",").splitToList(filterValue).contains(codeString);
      case NOT_IN:
        return !Splitter.on(",").splitToList(filterValue).contains(codeString);
      case EXISTS:
        return ("true".equals(filterValue)) == codeString.isEmpty();
      default:
        // "generalizes" is not supported - default to returning true so we're overly permissive
        // rather than rejecting valid codes.
        return true;
    }
  }

  private static EnumValueWithBackupName buildEnumValue(Code code, String display, String system) {
    String originalCode = code.getValue();
    String enumCase = toEnumCase(code, display, false);
    String backupName = toEnumCase(code, display, true);

    EnumValueDescriptorProto.Builder builder =
        EnumValueDescriptorProto.newBuilder().setName(enumCase);
    if (!JsonFormat.enumCodeToFhirCase(enumCase).equals(originalCode)) {
      builder.getOptionsBuilder().setExtension(Annotations.fhirOriginalCode, originalCode);
    }
    if (system != null) {
      builder.getOptionsBuilder().setExtension(Annotations.sourceCodeSystem, system);
    }
    return new EnumValueWithBackupName(builder, backupName);
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

  private static String toEnumCase(Code code, String display, boolean fullySpecify) {
    // TODO: handle more cases of fullySpecify
    String rawCode = code.getValue();
    if (CODE_RENAMES.containsKey(rawCode)) {
      return CODE_RENAMES.get(rawCode);
    }
    if (Character.isDigit(rawCode.charAt(0))) {
      if (!display.isEmpty() && !Character.isDigit(display.charAt(0))) {
        rawCode = fullySpecify ? display + "_" + rawCode : display;
      } else {
        rawCode = Ascii.toUpperCase("V_" + rawCode);
      }
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

  // CodeSystems that we hard-code to specific names, e.g., to avoid a colision.
  private static final ImmutableMap<String, String> CODE_SYSTEM_RENAMES =
      ImmutableMap.of(
          "http://hl7.org/fhir/CodeSystem/medication-statement-status",
          "MedicationStatementStatusCodes");

  public String getCodeSystemName(CodeSystem codeSystem) {
    if (CODE_SYSTEM_RENAMES.containsKey(codeSystem.getUrl().getValue())) {
      return CODE_SYSTEM_RENAMES.get(codeSystem.getUrl().getValue());
    }
    String name = GeneratorUtils.toFieldTypeCase(codeSystem.getName().getValue());
    name = name.replaceAll("[^A-Za-z0-9]", "");
    if (name.endsWith("Codes")) {
      return name.substring(0, name.length() - 1);
    }
    if (name.endsWith("Code")) {
      return name;
    }

    return name + "Code";
  }

  public String getValueSetName(ValueSet valueSet) {
    String name = GeneratorUtils.toFieldTypeCase(valueSet.getName().getValue());
    name = name.replaceAll("[^A-Za-z0-9]", "");
    if (name.endsWith("ValueSets")) {
      return name.substring(0, name.length() - 1);
    }
    if (name.endsWith("ValueSet")) {
      return name;
    }

    return name + "ValueSet";
  }

  private Set<ValueSet> getValueSetsUsedInBundles(Collection<Bundle> bundles) {
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

  private Set<CodeSystem> getCodeSystemsUsedInBundles(
      Collection<Bundle> bundles, boolean eagerMode) {
    Set<ValueSet> valueSets = getValueSetsUsedInBundles(bundles);

    if (eagerMode) {
      return valueSets.stream()
          .flatMap(vs -> getReferencedCodeSystems(vs).stream())
          .collect(Collectors.toSet());
    } else {
      return valueSets.stream()
          .map(vs -> getOneToOneCodeSystem(vs))
          .filter(op -> op.isPresent())
          .map(op -> op.get())
          .collect(Collectors.toSet());
    }
  }

  private Set<CodeSystem> getReferencedCodeSystems(ValueSet valueSet) {
    Set<CodeSystem> systems = new HashSet<>();
    for (ValueSet.Compose.ConceptSet include : valueSet.getCompose().getIncludeList()) {
      String systemUrl = include.getSystem().getValue();
      if (codeSystemsByUrl.containsKey(systemUrl)) {
        systems.add(codeSystemsByUrl.get(systemUrl));
      }
    }
    return systems;
  }

  private Optional<CodeSystem> getOneToOneCodeSystem(ValueSet valueSet) {
    if (!valueSet.getCompose().getExcludeList().isEmpty()) {
      return Optional.empty();
    }
    if (valueSet.getCompose().getIncludeCount() != 1) {
      return Optional.empty();
    }
    ValueSet.Compose.ConceptSet include = valueSet.getCompose().getIncludeList().get(0);

    if (!include.getValueSetList().isEmpty() || !include.getFilterList().isEmpty()) {
      return Optional.empty();
    }
    String systemUrl = include.getSystem().getValue();
    if (!codeSystemsByUrl.containsKey(systemUrl)) {
      return Optional.empty();
    }
    CodeSystem system = codeSystemsByUrl.get(systemUrl);

    if (!include.getConceptList().isEmpty()) {
      return Optional.empty();
    }

    return Optional.of(system);
  }

  private static Optional<String> getBindingValueSetUrl(ElementDefinition element) {
    if (element.getBinding().getStrength().getValue() != BindingStrengthCode.Value.REQUIRED) {
      return Optional.empty();
    }
    String url = CanonicalWrapper.getUri(element.getBinding().getValueSet());
    return url.isEmpty() ? Optional.empty() : Optional.<String>of(url);
  }
}
