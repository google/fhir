//    Copyright 2023 Google LLC.
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

import static com.google.fhir.protogen.FieldRetagger.retagTerminologyFile;

import com.google.common.base.Ascii;
import com.google.common.base.CaseFormat;
import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.Iterables;
import com.google.fhir.common.AnnotationUtils;
import com.google.fhir.common.Codes;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.common.TerminologyExpander;
import com.google.fhir.common.TerminologyExpander.ValueSetCode;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.ProtoGeneratorAnnotations;
import com.google.fhir.proto.ProtogenConfig;
import com.google.fhir.r4.core.BindingStrengthCode;
import com.google.fhir.r4.core.Code;
import com.google.fhir.r4.core.CodeSystem;
import com.google.fhir.r4.core.CodeSystem.ConceptDefinition;
import com.google.fhir.r4.core.ElementDefinition;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.ValueSet;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumValueDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldOptions;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileOptions;
import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.TreeSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Generator for FHIR Terminology protos. */
class ValueSetGeneratorV2 {
  // The input FHIR package.
  private final FhirPackage fhirPackage;
  // Config information to use for generating protos.
  private final ProtogenConfig protogenConfig;
  // Utility for expanding terminologies
  private final TerminologyExpander terminologyExpander;

  // From http://hl7.org/fhir/concept-properties to tag code values as deprecated
  public static final String CODE_VALUE_STATUS_PROPERTY =
      "http://hl7.org/fhir/concept-properties#status";
  public static final String CODE_VALUE_STATUS = "status";
  public static final String CODE_VALUE_STATUS_DEPRECATED = "deprecated";

  public ValueSetGeneratorV2(FhirPackage inputPackage, ProtogenConfig protogenConfig) {
    this.fhirPackage = inputPackage;
    this.protogenConfig = protogenConfig;
    this.terminologyExpander = new TerminologyExpander(fhirPackage);
  }

  FileDescriptorProto makeCodeSystemFile() throws InvalidFhirException {
    FileDescriptorProto.Builder builder = FileDescriptorProto.newBuilder();
    builder.setPackage(protogenConfig.getProtoPackage()).setSyntax("proto3");
    builder.addDependency(new File(GeneratorUtils.ANNOTATION_PATH, "annotations.proto").toString());
    FileOptions.Builder options = FileOptions.newBuilder();
    if (!protogenConfig.getJavaProtoPackage().isEmpty()) {
      options.setJavaPackage(protogenConfig.getJavaProtoPackage()).setJavaMultipleFiles(true);
    }
    builder.setOptions(options);

    List<DescriptorProto> messages = new ArrayList<>();
    for (CodeSystem codeSystem : getCodeSystemsUsedInPackage()) {
      messages.add(generateCodeSystemProto(codeSystem));
    }

    messages.stream()
        .sorted((p1, p2) -> p1.getName().compareTo(p2.getName()))
        .forEach(proto -> builder.addMessageType(proto));

    return protogenConfig.getLegacyRetagging()
        ? retagTerminologyFile(builder.build(), protogenConfig)
        : builder.build();
  }

  FileDescriptorProto makeValueSetFile() throws InvalidFhirException {
    FileDescriptorProto.Builder builder = FileDescriptorProto.newBuilder();
    builder.setPackage(protogenConfig.getProtoPackage()).setSyntax("proto3");
    builder.addDependency(new File(GeneratorUtils.ANNOTATION_PATH, "annotations.proto").toString());
    FileOptions.Builder options = FileOptions.newBuilder();
    if (!protogenConfig.getJavaProtoPackage().isEmpty()) {
      options.setJavaPackage(protogenConfig.getJavaProtoPackage()).setJavaMultipleFiles(true);
    }
    builder.setOptions(options);

    Set<DescriptorProto> messages = new TreeSet<>((p1, p2) -> p1.getName().compareTo(p2.getName()));

    for (ValueSet vs : getValueSetsUsedInPackage()) {
      // Check if there is a one-to-one code system for this value set.  If so, we don't bother
      // generating a value set and just use the enums from the code system.
      Optional<CodeSystem> oneToOneCodeSystem = getOneToOneCodeSystem(vs);
      if (!oneToOneCodeSystem.isPresent()) {
        Optional<DescriptorProto> proto = generateValueSetProto(vs);
        if (proto.isPresent()) {
          messages.add(proto.get());
        }
      }
    }
    builder.addAllMessageType(messages);

    return protogenConfig.getLegacyRetagging()
        ? retagTerminologyFile(builder.build(), protogenConfig)
        : builder.build();
  }

  private DescriptorProto generateCodeSystemProto(CodeSystem codeSystem) {
    String codeSystemName = getCodeSystemName(codeSystem);
    DescriptorProto.Builder descriptor = DescriptorProto.newBuilder().setName(codeSystemName);

    // Build a top-level message description.
    String comment =
        codeSystem.getDescription().getValue() + "\nSee " + codeSystem.getUrl().getValue();
    descriptor
        .getOptionsBuilder()
        .setExtension(ProtoGeneratorAnnotations.messageDescription, comment);

    return descriptor.addEnumType(generateCodeSystemEnum(codeSystem)).build();
  }

  private EnumDescriptorProto generateCodeSystemEnum(CodeSystem codeSystem) {
    String url = codeSystem.getUrl().getValue();
    EnumDescriptorProto.Builder enumDescriptor = EnumDescriptorProto.newBuilder();
    enumDescriptor
        .setName("Value")
        .addValue(
            EnumValueDescriptorProto.newBuilder().setNumber(0).setName("INVALID_UNINITIALIZED"));

    int enumNumber = 1;
    List<ConceptDefinition> concepts = TerminologyExpander.expandCodeSystem(codeSystem);

    for (ConceptDefinition concept : concepts) {
      EnumValueDescriptorProto.Builder valueBuilder =
          EnumValueDescriptorProto.newBuilder()
              .setNumber(enumNumber++)
              .setName(toEnumCase(concept.getCode().getValue(), concept.getDisplay().getValue()));
      String originalCode = concept.getCode().getValue();

      // Try to use the standard heuristics to the enum name to determine the original code.
      // If that doesn't work, add an original-code annotation so that we can always determine the
      // correct original code.
      if (!Codes.enumValueToCodeString(valueBuilder).equals(originalCode)) {
        valueBuilder.getOptionsBuilder().setExtension(Annotations.fhirOriginalCode, originalCode);
      }

      // Check the http://hl7.org/fhir/concept-properties to determine if the code value
      // has been deprecated.
      boolean isDeprecated =
          concept.getPropertyList().stream()
              .anyMatch(
                  property ->
                      property.getCode().getValue().equals(CODE_VALUE_STATUS)
                          && property.getValue().hasCode()
                          && property
                              .getValue()
                              .getCode()
                              .getValue()
                              .equals(CODE_VALUE_STATUS_DEPRECATED));
      if (isDeprecated) {
        valueBuilder.getOptionsBuilder().setExtension(Annotations.deprecatedCode, true);
      }

      enumDescriptor.addValue(valueBuilder);
    }
    if (url.equals("http://hl7.org/fhir/FHIR-version")
        && codeSystem.getVersion().getValue().equals("4.0.1")) {
      // In order to more easily facilitate cross-version support, add an R5 code to the R4 Versions
      // codesystem, using enum number 51 to match the enum number in R5 and later versions.
      // This helps in cases where things are incorrectly labeled as R5, such as the R4 Extensions
      // IG.  It also allows R5 resources to be parsed into R4 protos if they are otherwise
      // compatible.
      // See:
      // https://chat.fhir.org/#narrow/stream/179166-implementers/topic/Confused.20about.20hl7.2Efhir.2Euv.2Eextensions.2Er4.20npm
      EnumValueDescriptorProto.Builder v5Builder =
          EnumValueDescriptorProto.newBuilder().setNumber(51).setName("V_5_0_0");
      v5Builder.getOptionsBuilder().setExtension(Annotations.fhirOriginalCode, "5.0.0");
      enumDescriptor.addValue(v5Builder);
    }

    enumDescriptor.getOptionsBuilder().setExtension(Annotations.fhirCodeSystemUrl, url);
    return enumDescriptor.build();
  }

  private Optional<DescriptorProto> generateValueSetProto(ValueSet valueSet)
      throws InvalidFhirException {
    String valueSetName = getValueSetName(valueSet);
    String url = valueSet.getUrl().getValue();
    DescriptorProto.Builder descriptor = DescriptorProto.newBuilder().setName(valueSetName);

    // Build a top-level message description.
    String comment = valueSet.getDescription().getValue() + "\nSee " + url;
    descriptor
        .getOptionsBuilder()
        .setExtension(ProtoGeneratorAnnotations.messageDescription, comment);

    Optional<EnumDescriptorProto> valueSetEnum = generateValueSetEnum(valueSet);
    if (!valueSetEnum.isPresent()) {
      return Optional.empty();
    }
    return Optional.of(descriptor.addEnumType(valueSetEnum.get()).build());
  }

  private boolean valueSetHasProto(ValueSet valueSet) {
    try {
      return generateValueSetProto(valueSet).isPresent();
    } catch (InvalidFhirException e) {
      return false;
    }
  }

  private Optional<EnumDescriptorProto> generateValueSetEnum(ValueSet valueSet) {
    EnumDescriptorProto.Builder builder = EnumDescriptorProto.newBuilder().setName("Value");
    builder
        .getOptionsBuilder()
        .setExtension(Annotations.enumValuesetUrl, valueSet.getUrl().getValue());
    builder.addValue(
        EnumValueDescriptorProto.newBuilder().setNumber(0).setName("INVALID_UNINITIALIZED"));
    int enumNumber = 1;

    ImmutableList<ValueSetCode> codesWithSource = terminologyExpander.expandValueSet(valueSet);
    if (codesWithSource.isEmpty()) {
      // This ValueSet could not be expanded
      return Optional.empty();
    }

    for (ValueSetCode valueSetCode : codesWithSource) {
      EnumValueDescriptorProto.Builder valueBuilder =
          EnumValueDescriptorProto.newBuilder()
              .setNumber(enumNumber++)
              .setName(toEnumCase(valueSetCode.code, valueSetCode.display));
      // Add a system annotation
      valueBuilder
          .getOptionsBuilder()
          .setExtension(Annotations.sourceCodeSystem, valueSetCode.sourceSystem);

      // Try to use the standard enum-to-code heuristic to determine the original code.
      // If that doesn't work, add an original-code annotation so that we can always determine the
      // correct original code.
      if (!Codes.enumValueToCodeString(valueBuilder).equals(valueSetCode.code)) {
        valueBuilder
            .getOptionsBuilder()
            .setExtension(Annotations.fhirOriginalCode, valueSetCode.code);
      }
      builder.addValue(valueBuilder);
    }
    if (builder.getValueCount() == 1) {
      // Note: 1 because we start by adding the INVALID_UNINITIALIZED code
      return Optional.empty();
    }
    builder = dedupValueSetEnum(builder);
    return Optional.of(builder.build());
  }

  /**
   * Dedupes codes within a valueset. Note that a code is only considered a dupe if it has the same
   * value AND is from the same code system. Two identically-named codes from different codesystems
   * are considered different codes. True dupes can occur when ValueSets are composed from other
   * ValueSets.
   */
  private static EnumDescriptorProto.Builder dedupValueSetEnum(
      EnumDescriptorProto.Builder enumBuilder) {
    EnumDescriptorProto.Builder dedupedEnum = enumBuilder.clone();
    dedupedEnum.clearValue();

    Map<String, Set<String>> codesBySystem = new HashMap<>();
    for (EnumValueDescriptorProto enumValue : enumBuilder.getValueList()) {
      String codeSystem = enumValue.getOptions().getExtension(Annotations.sourceCodeSystem);
      String enumName = enumValue.getName();
      codesBySystem.putIfAbsent(codeSystem, new HashSet<>());
      if (codesBySystem.get(codeSystem).add(enumName)) {
        dedupedEnum.addValue(enumValue);
      }
    }
    return dedupedEnum;
  }

  private static final ImmutableMap<String, String> CODE_RENAMES =
      ImmutableMap.<String, String>builder()
          .put("NaN", "NOT_A_NUMBER")
          .put("_", "UNDERSCORE")
          .put("'", "APOSTROPHE")
          .build();

  private static final ImmutableMap<String, String> SYMBOLS =
      ImmutableMap.<String, String>builder()
          .put("<=", "LESS_THAN_OR_EQUAL_TO")
          .put(">=", "GREATER_THAN_OR_EQUAL_TO")
          .put(">", "GREATER_THAN")
          .put("!=", "NOT_EQUAL_TO")
          .put("=", "EQUALS")
          .put("<", "LESS_THAN")
          .put("*", "STAR")
          .put("%", "PERCENT")
          .put("…", "DOT_DOT_DOT")
          .put("+", "PLUS")
          .put("?", "QM")
          .put("#", "NUM")
          .build();

  private static final Pattern ACRONYM_PATTERN = Pattern.compile("([A-Z])([A-Z]+)(?![a-z])");

  private static String toEnumCase(String code, String display) {
    String enumCaseCode = code;
    if (Character.isDigit(enumCaseCode.charAt(0))) {
      if (!display.isEmpty() && !Character.isDigit(display.charAt(0))) {
        enumCaseCode = display;
      } else {
        enumCaseCode = Ascii.toUpperCase("V_" + enumCaseCode);
      }
    }
    if (CODE_RENAMES.containsKey(enumCaseCode)) {
      return CODE_RENAMES.get(enumCaseCode);
    }
    for (Map.Entry<String, String> entry : SYMBOLS.entrySet()) {
      if (enumCaseCode.contains(entry.getKey())) {
        enumCaseCode =
            enumCaseCode.replaceAll(Pattern.quote(entry.getKey()), "_" + entry.getValue() + "_");
        if (enumCaseCode.endsWith("_")) {
          enumCaseCode = enumCaseCode.substring(0, enumCaseCode.length() - 1);
        }
        if (enumCaseCode.startsWith("_")) {
          enumCaseCode = enumCaseCode.substring(1);
        }
      }
    }
    if (enumCaseCode.charAt(0) == '/') {
      enumCaseCode = "PER_" + enumCaseCode.substring(1);
    }
    enumCaseCode =
        enumCaseCode
            .replaceAll("[',]", "")
            .replace('\u00c2' /* Â */, 'A')
            .replaceAll("[^A-Za-z0-9]", "_");
    if (enumCaseCode.startsWith("_")) {
      enumCaseCode = enumCaseCode.substring(1);
    }
    if (enumCaseCode.endsWith("_")) {
      enumCaseCode = enumCaseCode.substring(0, enumCaseCode.length() - 1);
    }
    if (enumCaseCode.length() == 0) {
      throw new IllegalArgumentException("Unable to generate enum for code: " + code);
    }
    if (Character.isDigit(enumCaseCode.charAt(0))) {
      return Ascii.toUpperCase("NUM_" + enumCaseCode);
    }
    // Don't change FOO into F_O_O
    if (enumCaseCode.equals(Ascii.toUpperCase(enumCaseCode))) {
      return enumCaseCode.replaceAll("__+", "_");
    }

    // Turn acronyms into single words, e.g., FHIR_is_GREAT -> Fhir_is_Great, so that it ultimately
    // becomes FHIR_IS_GREAT instead of F_H_I_R_IS_G_R_E_A_T
    Matcher matcher = ACRONYM_PATTERN.matcher(enumCaseCode);
    StringBuffer sb = new StringBuffer();
    while (matcher.find()) {
      matcher.appendReplacement(sb, matcher.group(1) + Ascii.toLowerCase(matcher.group(2)));
    }
    matcher.appendTail(sb);
    enumCaseCode = sb.toString();

    return CaseFormat.LOWER_CAMEL
        .to(CaseFormat.UPPER_UNDERSCORE, enumCaseCode)
        .replaceAll("__+", "_");
  }

  // CodeSystems that we hard-code to specific names, e.g., to avoid a collision.
  private static final ImmutableMap<String, String> CODE_SYSTEM_RENAMES =
      ImmutableMap.<String, String>builder()
          .put("http://hl7.org/fhir/secondary-finding", "ObservationSecondaryFindingCode")
          .put(
              "http://terminology.hl7.org/CodeSystem/composition-altcode-kind",
              "CompositionAlternativeCodeKindCode")
          .put(
              "http://hl7.org/fhir/contract-security-classification",
              "ContractResourceSecurityClassificationCode")
          .put("http://hl7.org/fhir/device-definition-status", "FHIRDeviceDefinitionStatusCode")
          // These CodeSystems have colliding names in R5
          // See: https://jira.hl7.org/browse/FHIR-41817
          .put("http://hl7.org/fhir/eligibility-outcome", "EligibilityOutcomeCode")
          .put("http://hl7.org/fhir/payment-outcome", "PaymentOutcomeCode")
          .put("http://hl7.org/fhir/enrollment-outcome", "EnrollmentOutcomeCode")
          .put(
              "http://hl7.org/fhir/deviceassociation-status-reason",
              "DeviceAssociationStatusReason")
          .put(
              "http://hl7.org/fhir/CodeSystem/medication-statement-status",
              "MedicationStatementStatusCodes")
          .build();

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

  private Set<ValueSet> getValueSetsUsedInPackage() throws InvalidFhirException {
    final Set<String> valueSetUrls = new HashSet<>();
    for (StructureDefinition def : fhirPackage.structureDefinitions()) {
      def.getSnapshot().getElementList().stream()
          .map(element -> getBindingValueSetUrl(element))
          .filter(optionalUrl -> optionalUrl.isPresent())
          .forEach(
              optionalUrl ->
                  valueSetUrls.add(Iterables.get(Splitter.on('|').split(optionalUrl.get()), 0)));
    }
    Set<ValueSet> valueSets = new HashSet<>();
    for (String url : valueSetUrls) {
      Optional<ValueSet> valueSet = fhirPackage.getValueSet(url);
      if (valueSet.isPresent()) {
        valueSets.add(valueSet.get());
      }
    }
    return valueSets;
  }

  private Set<CodeSystem> getCodeSystemsUsedInPackage() throws InvalidFhirException {
    Set<CodeSystem> codeSystems = new HashSet<>();
    for (ValueSet vs : getValueSetsUsedInPackage()) {
      codeSystems.addAll(getReferencedCodeSystems(vs));
    }
    return codeSystems;
  }

  private Set<CodeSystem> getReferencedCodeSystems(ValueSet valueSet) throws InvalidFhirException {
    Set<CodeSystem> systems = new HashSet<>();
    for (ValueSet.Compose.ConceptSet include : valueSet.getCompose().getIncludeList()) {
      String systemUrl = include.getSystem().getValue();
      Optional<CodeSystem> system = fhirPackage.getCodeSystem(systemUrl);
      if (system.isPresent()) {
        systems.add(system.get());
      }
    }
    return systems;
  }

  private Optional<CodeSystem> getOneToOneCodeSystem(ValueSet valueSet)
      throws InvalidFhirException {
    if (!valueSet.getCompose().getExcludeList().isEmpty()) {
      return Optional.empty();
    }
    if (valueSet.getCompose().getIncludeCount() != 1) {
      return Optional.empty();
    }
    ValueSet.Compose.ConceptSet include = valueSet.getCompose().getIncludeList().get(0);

    if (!include.getValueSetList().isEmpty()
        || !include.getFilterList().isEmpty()
        || !include.getConceptList().isEmpty()) {
      return Optional.empty();
    }
    return fhirPackage.getCodeSystem(include.getSystem().getValue());
  }

  private static Optional<String> getBindingValueSetUrl(ElementDefinition element) {
    if (element.getBinding().getStrength().getValue() != BindingStrengthCode.Value.REQUIRED) {
      return Optional.empty();
    }
    String url = GeneratorUtils.getCanonicalUri(element.getBinding().getValueSet());
    return url.isEmpty() ? Optional.empty() : Optional.<String>of(url);
  }

  public DescriptorProto generateCodeBoundToValueSet(String typeName, String valueSetUrl)
      throws InvalidFhirException {

    ValueSet valueSet =
        fhirPackage
            .getValueSet(valueSetUrl)
            .orElseThrow(
                () ->
                    new IllegalArgumentException(
                        "Encountered unrecognized ValueSet url: " + valueSetUrl));

    DescriptorProto.Builder descriptor = DescriptorProto.newBuilder().setName(typeName);

    FieldDescriptorProto.Builder enumField = descriptor.addFieldBuilder().setNumber(1);

    descriptor
        .addField(
            FieldDescriptorProto.newBuilder()
                .setNumber(2)
                .setName("id")
                .setTypeName("." + protogenConfig.getProtoPackage() + ".String")
                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE))
        .addField(
            FieldDescriptorProto.newBuilder()
                .setNumber(3)
                .setName("extension")
                .setTypeName("." + protogenConfig.getProtoPackage() + ".Extension")
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

    Optional<CodeSystem> oneToOneCodeSystem = getOneToOneCodeSystem(valueSet);
    if (oneToOneCodeSystem.isPresent()) {
      enumField
          .setName("value")
          .setType(FieldDescriptorProto.Type.TYPE_ENUM)
          .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
          .setTypeName(
              "."
                  + protogenConfig.getProtoPackage()
                  + "."
                  + getCodeSystemName(oneToOneCodeSystem.get())
                  + ".Value");
      return descriptor.build();
    }

    if (valueSetHasProto(valueSet)) {
      enumField
          .setName("value")
          .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
          .setType(FieldDescriptorProto.Type.TYPE_ENUM)
          .setTypeName(
              "." + protogenConfig.getProtoPackage() + "." + getValueSetName(valueSet) + ".Value");
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
            .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
            .setOptions(
                FieldOptions.newBuilder()
                    .setExtension(
                        ProtoGeneratorAnnotations.fieldDescription,
                        "This valueset is not enumerable, and so is represented as a"
                            + " string.")));

    return descriptor.build();
  }
}
