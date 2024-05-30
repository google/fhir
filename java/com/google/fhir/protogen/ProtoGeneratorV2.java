//    Copyright 2023 Google Inc.
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

import static com.google.common.collect.ImmutableList.toImmutableList;
import static com.google.common.collect.Streams.stream;
import static com.google.fhir.protogen.FieldRetagger.retagFile;
import static com.google.fhir.protogen.GeneratorUtils.isProfile;
import static com.google.fhir.protogen.GeneratorUtils.lastIdToken;
import static com.google.fhir.protogen.GeneratorUtils.nameFromQualifiedName;
import static com.google.fhir.protogen.GeneratorUtils.toFieldNameCase;
import static com.google.fhir.protogen.GeneratorUtils.toFieldTypeCase;

import com.google.common.base.Ascii;
import com.google.common.base.CaseFormat;
import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Iterables;
import com.google.errorprone.annotations.CanIgnoreReturnValue;
import com.google.fhir.common.Extensions;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.ProtoGeneratorAnnotations;
import com.google.fhir.proto.ProtogenConfig;
import com.google.fhir.protogen.GeneratorUtils.QualifiedType;
import com.google.fhir.r4.core.BindingStrengthCode;
import com.google.fhir.r4.core.Canonical;
import com.google.fhir.r4.core.ConstraintSeverityCode;
import com.google.fhir.r4.core.ElementDefinition;
import com.google.fhir.r4.core.Extension;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.StructureDefinitionKindCode;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumValueDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldOptions;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileOptions;
import com.google.protobuf.DescriptorProtos.MessageOptions;
import com.google.protobuf.DescriptorProtos.OneofDescriptorProto;
import com.google.protobuf.DescriptorProtos.OneofOptions;
import com.google.protobuf.Message;
import java.io.File;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.TreeSet;
import java.util.function.Supplier;

/** A class which turns FHIR StructureDefinitions into protocol messages. */
// TODO(b/244184211): Move a bunch of the public static methods into ProtoGeneratorUtils.
public class ProtoGeneratorV2 {

  public static final String REGEX_EXTENSION_URL = "http://hl7.org/fhir/StructureDefinition/regex";
  public static final String EXPLICIT_TYPE_NAME_EXTENSION_URL =
      "http://hl7.org/fhir/StructureDefinition/structuredefinition-explicit-type-name";

  // Map of time-like primitive type ids to supported granularity
  private static final ImmutableMap<String, List<String>> TIME_LIKE_PRECISION_MAP =
      ImmutableMap.of(
          "date", ImmutableList.of("YEAR", "MONTH", "DAY"),
          "dateTime",
              ImmutableList.of("YEAR", "MONTH", "DAY", "SECOND", "MILLISECOND", "MICROSECOND"),
          "instant", ImmutableList.of("SECOND", "MILLISECOND", "MICROSECOND"),
          "time", ImmutableList.of("SECOND", "MILLISECOND", "MICROSECOND"));
  private static final ImmutableSet<String> TYPES_WITH_TIMEZONE =
      ImmutableSet.of("date", "dateTime", "instant");

  // Certain field names are reserved symbols in various languages.
  private static final ImmutableSet<String> RESERVED_FIELD_NAMES =
      ImmutableSet.of(
          "assert",
          "for",
          "hasAnswer",
          "hasSeverity",
          "hasStage",
          "hasBodySite",
          "package",
          "string",
          "class");

  private static final EnumDescriptorProto PRECISION_ENUM =
      EnumDescriptorProto.newBuilder()
          .setName("Precision")
          .addValue(
              EnumValueDescriptorProto.newBuilder()
                  .setName("PRECISION_UNSPECIFIED")
                  .setNumber(0)
                  .build())
          .build();

  private static final FieldDescriptorProto TIMEZONE_FIELD =
      ((Supplier<FieldDescriptorProto>)
              () -> {
                FieldDescriptorProto.Builder builder =
                    FieldDescriptorProto.newBuilder()
                        .setName("timezone")
                        .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                        .setType(FieldDescriptorProto.Type.TYPE_STRING)
                        .setNumber(2);
                builder
                    .getOptionsBuilder()
                    .setExtension(
                        ProtoGeneratorAnnotations.fieldDescription,
                        "The local timezone in which the event was recorded.");
                return builder.build();
              })
          .get();

  private static final ImmutableMap<String, FieldDescriptorProto.Type> PRIMITIVE_TYPE_OVERRIDES =
      ImmutableMap.of(
          "base64Binary", FieldDescriptorProto.Type.TYPE_BYTES,
          "boolean", FieldDescriptorProto.Type.TYPE_BOOL,
          "integer", FieldDescriptorProto.Type.TYPE_SINT32,
          "integer64", FieldDescriptorProto.Type.TYPE_SINT64,
          "positiveInt", FieldDescriptorProto.Type.TYPE_UINT32,
          "unsignedInt", FieldDescriptorProto.Type.TYPE_UINT32);

  // Exclude constraints from the DomainResource until we refactor
  // them to a common place rather on every resource.
  // TODO(b/244184211): remove these with the above refactoring.
  private static final ImmutableSet<String> DOMAIN_RESOURCE_CONSTRAINTS =
      ImmutableSet.of(
          "contained.contained.empty()",
          "contained.meta.versionId.empty() and contained.meta.lastUpdated.empty()",
          "contained.where((('#'+id in (%resource.descendants().reference"
              + " | %resource.descendants().as(canonical) | %resource.descendants().as(uri)"
              + " | %resource.descendants().as(url))) or descendants().where(reference = '#')"
              + ".exists() or descendants().where(as(canonical) = '#').exists() or"
              + " descendants().where(as(canonical) = '#').exists()).not())"
              + ".trace('unmatched', id).empty()",
          "text.div.exists()",
          "text.`div`.exists()",
          "contained.meta.security.empty()",
          "contained.where((('#'+id in (%resource.descendants().reference |"
              + " %resource.descendants().ofType(canonical) |"
              + " %resource.descendants().ofType(uri) | %resource.descendants().ofType(url)))"
              + " or descendants().where(reference = '#').exists() or"
              + " descendants().where(ofType(canonical) = '#').exists() or"
              + " descendants().where(ofType(canonical) ="
              + " '#').exists()).not()).trace('unmatched', id).empty()");

  // FHIR elements may have core constraint definitions that do not add
  // value to protocol buffers, so we exclude them.
  private static final ImmutableSet<String> EXCLUDED_FHIR_CONSTRAINTS =
      ImmutableSet.<String>builder()
          .addAll(DOMAIN_RESOURCE_CONSTRAINTS)
          .add(
              "extension.exists() != value.exists()",
              "hasValue() | (children().count() > id.count())",
              "hasValue() or (children().count() > id.count())",
              "hasValue() or (children().count() > id.count()) or $this is Parameters",
              // Exclude the FHIR-provided element name regex, since field names are known at
              // compile time
              "path.matches('[^\\\\s\\\\.,:;\\\\\\'\"\\\\/|?!@#$%&*()\\\\[\\\\]{}]{1,64}"
                  + "(\\\\.[^\\\\s\\\\.,:;\\\\\\'\"\\\\/|?!@#$%&*()\\\\[\\\\]{}]{1,64}"
                  + "(\\\\[x\\\\])?(\\\\:[^\\\\s\\\\.]+)?)*')",
              "path.matches('^[^\\\\s\\\\.,:;\\\\\\'\"\\\\/|?!@#$%&*()\\\\[\\\\]{}]{1,64}"
                  + "(\\\\.[^\\\\s\\\\.,:;\\\\\\'\"\\\\/|?!@#$%&*()\\\\[\\\\]{}]{1,64}"
                  + "(\\\\[x\\\\])?(\\\\:[^\\\\s\\\\.]+)?)*$')",
              // "telcom or endpoint" is an invalid expression that shows up in USCore
              "telecom or endpoint",
              "fullUrl.contains('/_history/').not()", // See https://jira.hl7.org/browse/FHIR-25525
              // Invalid FHIRPath constraint on StructureDefinition.snapshot in STU3. Fixed in R4
              // but unlikely to be backported.
              "element.all(definition and min and max)",
              // See https://jira.hl7.org/browse/FHIR-25796
              "probability is decimal implies (probability as decimal) <= 100",
              // R4 Appointment constraint is disabled for legacy reasons (b/302720776)
              "Appointment.cancelationReason.exists() implies (Appointment.status='no-show' or"
                  + " Appointment.status='cancelled')",
              // Contains an invalid double-quoted string.
              // See: https://jira.hl7.org/browse/FHIR-41873
              "binding.empty() or type.code.empty() or type.code.contains(\":\") or"
                  + " type.select((code = 'code') or (code = 'Coding') or (code='CodeableConcept')"
                  + " or (code = 'Quantity') or (code = 'string') or (code = 'uri') or (code ="
                  + " 'Duration')).exists()")
          .build();

  private static final String FHIRPATH_TYPE_PREFIX = "http://hl7.org/fhirpath/";

  private static final String FHIR_TYPE_EXTENSION_URL =
      "http://hl7.org/fhir/StructureDefinition/structuredefinition-fhir-type";

  // The package being generated.
  private final FhirPackage fhirPackage;

  // Config information for generating the protos
  private final ProtogenConfig protogenConfig;

  // A valueset generator for the package, using the same config info.
  private final ValueSetGeneratorV2 valueSetGenerator;

  public ProtoGeneratorV2(FhirPackage fhirPackage, ProtogenConfig protogenConfig) {
    this.fhirPackage = fhirPackage;
    this.protogenConfig = protogenConfig;
    this.valueSetGenerator = new ValueSetGeneratorV2(fhirPackage, protogenConfig);
  }

  private StructureDefinition fixIdBug(StructureDefinition def) {
    boolean isResource = def.getKind().getValue() == StructureDefinitionKindCode.Value.RESOURCE;

    // FHIR uses "compiler magic" to set the type of id fields, but it has errors in several
    // versions of FHIR.  Here, assume all Resource IDs are type "id", and all DataType IDs are
    // type "string", which is the intention of the spec.
    StructureDefinition.Builder defBuilder = def.toBuilder();
    for (ElementDefinition.Builder elementBuilder :
        defBuilder.getSnapshotBuilder().getElementBuilderList()) {
      if (elementBuilder.getId().getValue().matches("[A-Za-z]*\\.id")) {
        for (int i = 0; i < elementBuilder.getTypeBuilder(0).getExtensionCount(); i++) {
          Extension.Builder extensionBuilder =
              elementBuilder.getTypeBuilder(0).getExtensionBuilder(i);
          if (extensionBuilder.getUrl().getValue().equals(FHIR_TYPE_EXTENSION_URL)) {
            extensionBuilder
                .getValueBuilder()
                .getUrlBuilder()
                .setValue(isResource ? "id" : "string");
          }
        }
      }
    }
    return defBuilder.build();
  }

  // Class for generating a single message from a single StructureDefinition.
  // This contains additional context from the containing ProtoGenerator, specific to the definition
  // it is the generator for.
  private final class PerDefinitionGenerator {
    private final StructureDefinition structureDefinition;
    private final ImmutableList<ElementDefinition> allElements;

    PerDefinitionGenerator(StructureDefinition structureDefinition) {
      this.structureDefinition = fixIdBug(structureDefinition);
      this.allElements =
          ImmutableList.copyOf(this.structureDefinition.getSnapshot().getElementList());
    }

    DescriptorProto generate() throws InvalidFhirException {
      DescriptorProto.Builder builder = DescriptorProto.newBuilder();
      builder.setOptions(generateOptions());
      generateMessage(allElements.get(0), builder);

      if (isProfile(structureDefinition)) {
        builder
            .getOptionsBuilder()
            .addExtension(
                Annotations.fhirProfileBase, structureDefinition.getBaseDefinition().getValue());
      }

      return builder.build();
    }

    private MessageOptions generateOptions() {
      // Build a top-level message description.
      StringBuilder comment =
          new StringBuilder()
              .append("Auto-generated from StructureDefinition for ")
              .append(structureDefinition.getName().getValue());
      comment.append(".");
      if (structureDefinition.getSnapshot().getElement(0).hasShort()) {
        String shortString = structureDefinition.getSnapshot().getElement(0).getShort().getValue();
        if (!shortString.endsWith(".")) {
          shortString += ".";
        }
        comment.append("\n").append(shortString.replaceAll("[\\n\\r]", "\n"));
      }
      comment.append("\nSee ").append(structureDefinition.getUrl().getValue());

      // Add message-level annotations.
      MessageOptions.Builder optionsBuilder =
          MessageOptions.newBuilder()
              .setExtension(
                  Annotations.structureDefinitionKind,
                  Annotations.StructureDefinitionKindValue.valueOf(
                      "KIND_" + structureDefinition.getKind().getValue()))
              .setExtension(ProtoGeneratorAnnotations.messageDescription, comment.toString())
              .setExtension(
                  Annotations.fhirStructureDefinitionUrl, structureDefinition.getUrl().getValue());
      if (structureDefinition.getAbstract().getValue()) {
        optionsBuilder.setExtension(
            Annotations.isAbstractType, structureDefinition.getAbstract().getValue());
      }
      return optionsBuilder.build();
    }

    @CanIgnoreReturnValue
    private DescriptorProto generateMessage(
        ElementDefinition currentElement, DescriptorProto.Builder builder)
        throws InvalidFhirException {
      // Get the name of this message
      builder.setName(nameFromQualifiedName(getContainerType(currentElement)));

      // Add message-level FHIRPath constraints.
      ImmutableList<String> expressions = getFhirPathErrorConstraints(currentElement);
      if (!expressions.isEmpty()) {
        builder
            .getOptionsBuilder()
            .setExtension(Annotations.fhirPathMessageConstraint, expressions);
      }
      // Add warning constraints.
      ImmutableList<String> warnings = getFhirPathWarningConstraints(currentElement);
      if (!warnings.isEmpty()) {
        builder
            .getOptionsBuilder()
            .setExtension(Annotations.fhirPathMessageWarningConstraint, warnings);
      }

      // When generating a descriptor for a primitive type, the value part may already be present.
      int nextTag = builder.getFieldCount() + 1;

      // Loop over the direct children of this element.
      for (ElementDefinition element : getDirectChildren(currentElement)) {
        if (element.getId().getValue().matches("[^.]*\\.value")
            && (element.getType(0).getCode().getValue().isEmpty()
                || element.getType(0).getCode().getValue().startsWith(FHIRPATH_TYPE_PREFIX))) {
          // This is a primitive value element.
          generatePrimitiveValue(element, builder);
          nextTag++;
          continue;
        }

        if (!isChoiceType(element) && !isSingleType(element)) {
          throw new IllegalArgumentException(
              "Illegal field has multiple types but is not a Choice Type:\n" + element);
        }

        if (isContainedResourceField(element)) {
          buildAndAddField(element, nextTag++, builder);
          builder
              .addFieldBuilder()
              .setNumber(nextTag)
              .getOptionsBuilder()
              .setExtension(
                  ProtoGeneratorAnnotations.reservedReason,
                  "Field "
                      + nextTag
                      + " reserved for strongly-typed ContainedResource for id: "
                      + element.getId().getValue());
          nextTag++;
        } else {
          int thisTag = nextTag;
          nextTag++;
          // For legacy reasons, hard-code Extension.url and Extension.extension
          // tag numbers.  This makes them match the old, R4 hardcoded Extension message.
          if (element.getId().getValue().equals("Extension.url")) {
            thisTag = 2;
          }
          if (element.getId().getValue().equals("Extension.extension")) {
            thisTag = 3;
          }
          buildAndAddField(element, thisTag, builder);
        }
      }
      return builder.build();
    }

    /** Generate the primitive value part of a datatype. */
    private void generatePrimitiveValue(
        ElementDefinition valueElement, DescriptorProto.Builder builder)
        throws InvalidFhirException {
      String defId = structureDefinition.getId().getValue();

      // If a regex for this primitive type is present, add it as a message-level annotation.
      if (valueElement.getTypeCount() == 1) {
        Optional<String> regexOptional = getPrimitiveRegex(valueElement);
        if (regexOptional.isPresent()) {
          // Handling the regex for Decimal definition in FHIR R5 separately. Refer b/342413826
          if (builder.getName().equals("Decimal")
              && regexOptional
                  .get()
                  .equals("-?(0|[1-9][0-9]{0,17})(\\.[0-9]{1,17})?([eE][+-]?[0-9]{1,9}})?")) {
            builder.setOptions(
                builder.getOptions().toBuilder()
                    .setExtension(
                        Annotations.valueRegex,
                        "-?(0|[1-9][0-9]{0,17})(\\.[0-9]{1,17})?([eE][+-]?[0-9]{1,9})?")
                    .build());
          } else {
            builder.setOptions(
                builder.getOptions().toBuilder()
                    .setExtension(Annotations.valueRegex, regexOptional.get())
                    .build());
          }
        }
      }

      // For historical reasons, primitive value fields appear first in primitive protos.
      // Therefore, we start numbering at one for these fields.
      // At the end of this function, we will shift all other fields down by the number of fields
      // we are adding.
      List<FieldDescriptorProto> fieldsToAdd = new ArrayList<>();
      if (TIME_LIKE_PRECISION_MAP.containsKey(defId)) {
        // Handle time-like types.
        EnumDescriptorProto.Builder enumBuilder = PRECISION_ENUM.toBuilder();
        for (String value : TIME_LIKE_PRECISION_MAP.get(defId)) {
          enumBuilder.addValue(
              EnumValueDescriptorProto.newBuilder()
                  .setName(value)
                  .setNumber(enumBuilder.getValueCount()));
        }
        builder.addEnumType(enumBuilder);
        FieldDescriptorProto.Builder valueField =
            FieldDescriptorProto.newBuilder()
                .setType(FieldDescriptorProto.Type.TYPE_INT64)
                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                .setName("value_us")
                .setNumber(1);
        valueField
            .getOptionsBuilder()
            .setExtension(
                ProtoGeneratorAnnotations.fieldDescription,
                "The absolute time of the event as a Unix epoch in microseconds.");
        fieldsToAdd.add(valueField.build());
        if (TYPES_WITH_TIMEZONE.contains(defId)) {
          fieldsToAdd.add(TIMEZONE_FIELD);
        }
        fieldsToAdd.add(
            FieldDescriptorProto.newBuilder()
                .setName("precision")
                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                .setType(FieldDescriptorProto.Type.TYPE_ENUM)
                .setTypeName(
                    "."
                        + protogenConfig.getProtoPackage()
                        + "."
                        + toFieldTypeCase(defId)
                        + ".Precision")
                .setNumber(TYPES_WITH_TIMEZONE.contains(defId) ? 3 : 2)
                .build());
      } else {
        // Handle non-time-like types by just adding the value field.
        FieldDescriptorProto.Builder valueField =
            FieldDescriptorProto.newBuilder()
                .setType(
                    PRIMITIVE_TYPE_OVERRIDES.getOrDefault(
                        defId, FieldDescriptorProto.Type.TYPE_STRING))
                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                .setName("value")
                .setNumber(1);
        String description =
            valueElement.hasShort()
                ? valueElement.getShort().getValue()
                : "Primitive value for " + defId;
        valueField
            .getOptionsBuilder()
            .setExtension(ProtoGeneratorAnnotations.fieldDescription, description);
        if (isRequiredByFhir(valueElement)) {
          valueField
              .getOptionsBuilder()
              .setExtension(
                  Annotations.validationRequirement, Annotations.Requirement.REQUIRED_BY_FHIR);
        }
        fieldsToAdd.add(valueField.build());
      }
      // For historical reasons, the primitive value field (an correspoding timezone and precision
      // fields in the case of timelike primitives) are the first tag numbers added.  Therefore,
      // shift all pre-existing fields to be after those fields.
      int shiftSize = fieldsToAdd.size();
      for (FieldDescriptorProto oldField : builder.getFieldList()) {
        fieldsToAdd.add(oldField.toBuilder().setNumber(oldField.getNumber() + shiftSize).build());
      }
      builder.clearField().addAllField(fieldsToAdd);
    }

    private void buildAndAddField(
        ElementDefinition element, int tag, DescriptorProto.Builder builder)
        throws InvalidFhirException {
      // Generate the field. If this field doesn't actually exist in this version of the
      // message, for example, the max attribute is 0, buildField returns null and no field
      // should be added.
      Optional<FieldDescriptorProto> fieldOptional = buildField(element, tag);
      if (fieldOptional.isPresent()) {
        FieldDescriptorProto field = fieldOptional.get();
        Optional<DescriptorProto> optionalNestedType = buildNestedTypeIfNeeded(element);
        if (optionalNestedType.isPresent()) {
          builder.addNestedType(optionalNestedType.get());
        } else {
          // There is no submessage defined for this field, so apply constraints to the field
          // itself.
          ImmutableList<String> expressions = getFhirPathErrorConstraints(element);
          if (!expressions.isEmpty()) {
            field =
                field.toBuilder()
                    .setOptions(
                        field.getOptions().toBuilder()
                            .setExtension(Annotations.fhirPathConstraint, expressions))
                    .build();
          }
          // Add warning constraints.
          ImmutableList<String> warnings = getFhirPathWarningConstraints(element);
          if (!warnings.isEmpty()) {
            field =
                field.toBuilder()
                    .setOptions(
                        field.getOptions().toBuilder()
                            .setExtension(Annotations.fhirPathWarningConstraint, warnings))
                    .build();
          }
        }

        builder.addField(field);
      } else if (!element.getPath().getValue().equals("Extension.extension")
          && !element.getPath().getValue().equals("Extension.value[x]")) {
        // Don't bother adding reserved messages for Extension.extension or Extension.value[x]
        // since that's part of the extension definition, and adds a lot of unhelpful noise.
        builder
            .addFieldBuilder()
            .setNumber(tag)
            .getOptionsBuilder()
            .setExtension(
                ProtoGeneratorAnnotations.reservedReason,
                element.getPath().getValue() + " not present on profile.");
      }
    }

    private Optional<DescriptorProto> buildNestedTypeIfNeeded(ElementDefinition element)
        throws InvalidFhirException {
      return isChoiceType(element)
          ? Optional.of(makeChoiceType(element))
          : buildNonChoiceTypeNestedTypeIfNeeded(element);
    }

    private Optional<DescriptorProto> buildNonChoiceTypeNestedTypeIfNeeded(
        ElementDefinition element) throws InvalidFhirException {
      if (element.getTypeCount() != 1) {
        return Optional.empty();
      }

      Optional<DescriptorProto> profiledCode = makeBoundCodeIfRequired(element);
      if (profiledCode.isPresent()) {
        return profiledCode;
      }

      // If this is a container type, extension, define the inner message.
      if (isContainer(element)) {
        return Optional.of(generateMessage(element, DescriptorProto.newBuilder()));
      }
      return Optional.empty();
    }

    private Optional<DescriptorProto> makeBoundCodeIfRequired(ElementDefinition element)
        throws InvalidFhirException {
      if (element.getTypeCount() != 1 || !element.getType(0).getCode().getValue().equals("code")) {
        return Optional.empty();
      }

      Optional<String> boundValueSetUrl = getBindingValueSetUrl(element);
      if (!boundValueSetUrl.isPresent()
          // TODO(b/297040090): We treat Language code as untyped, to match R4 behavior.
          || boundValueSetUrl.get().equals("http://hl7.org/fhir/ValueSet/all-languages")) {
        return Optional.empty();
      }

      Optional<QualifiedType> typeWithBoundValueSet = checkForTypeWithBoundValueSet(element);
      if (!typeWithBoundValueSet.isPresent()) {
        return Optional.empty();
      }

      return Optional.of(
          valueSetGenerator.generateCodeBoundToValueSet(
              typeWithBoundValueSet.get().getName(), boundValueSetUrl.get()));
    }

    private Optional<QualifiedType> checkForTypeWithBoundValueSet(ElementDefinition element)
        throws InvalidFhirException {
      if (getDistinctTypeCount(element) == 1) {
        String containerName = getContainerType(element);
        ElementDefinition.TypeRef type = element.getType(0);

        String typeName = type.getCode().getValue();
        Optional<String> valueSetUrl = getBindingValueSetUrl(element);
        if (valueSetUrl.isPresent()) {
          if (typeName.equals("code")) {
            if (!containerName.endsWith("Code") && !containerName.endsWith(".CodeType")) {
              // Carve out some exceptions because CodeCode and CodeTypeCode sounds silly.
              containerName = containerName + "Code";
            }
            return Optional.of(new QualifiedType(containerName, protogenConfig.getProtoPackage()));
          }
        }
      }
      return Optional.empty();
    }

    /** Extract the type of a container field, possibly by reference. */
    private String getContainerType(ElementDefinition element) throws InvalidFhirException {
      if (element.hasContentReference()) {
        // Find the named element which was referenced. We'll use the type of that element.
        // Strip the first character from the content reference since it is a '#'
        String referencedElementId = element.getContentReference().getValue().substring(1);
        ElementDefinition referencedElement = getElementById(referencedElementId);
        if (!isContainer(referencedElement)) {
          throw new IllegalArgumentException(
              "ContentReference does not reference a container: " + element.getContentReference());
        }
        return getContainerType(referencedElement);
      }

      // The container type is the full type of the message that will be generated (minus package).
      // It is derived from the id (e.g., Medication.package.content), and these are usually equal
      // other than casing (e.g., Medication.Package.Content).
      // However, any parent in the path could have been renamed via a explicit type name
      // extensions.

      // Check for explicit renamings on this element.
      List<Message> explicitTypeNames =
          Extensions.getExtensionsWithUrl(EXPLICIT_TYPE_NAME_EXTENSION_URL, element);

      if (explicitTypeNames.size() > 1) {
        throw new InvalidFhirException(
            "Element has multiple explicit type names: " + element.getId().getValue());
      }

      // Use explicit type name if present.  Otherwise, use the field_name, converted to FieldType
      // casing, as the submessage name.
      String typeName =
          toFieldTypeCase(
              explicitTypeNames.isEmpty()
                  ? getNameForElement(element)
                  : (String)
                      Extensions.getExtensionValue(explicitTypeNames.get(0), "string_value"));
      if (isChoiceType(element)) {
        typeName = typeName + "X";
      }

      Optional<ElementDefinition> parentOpt = GeneratorUtils.getParent(element, allElements);

      String packageString;
      if (parentOpt.isPresent()) {
        ElementDefinition parent = parentOpt.get();
        String parentType = getContainerType(parent);
        packageString = parentType + ".";
      } else {
        packageString = "";
      }

      if (packageString.startsWith(typeName + ".")
          || packageString.contains("." + typeName + ".")
          || (typeName.equals("Code") && parentOpt.isPresent())) {
        typeName = typeName + "Type";
      }
      return packageString + typeName;
    }

    // Returns the only element in the list matching a given id.
    // Throws IllegalArgumentException if zero or more than one matching element is found.
    private ElementDefinition getElementById(String id) throws InvalidFhirException {
      return GeneratorUtils.getElementById(id, allElements);
    }

    /**
     * Gets the field type and package of a potentially complex element. This handles choice types,
     * types that reference other elements, references, profiles, etc.
     */
    private QualifiedType getQualifiedFieldType(ElementDefinition element)
        throws InvalidFhirException {
      Optional<QualifiedType> valueSetType = checkForTypeWithBoundValueSet(element);
      if (valueSetType.isPresent()) {
        return valueSetType.get();
      }

      if (isContainer(element) || isChoiceType(element)) {
        return new QualifiedType(getContainerType(element), protogenConfig.getProtoPackage());
      } else if (element.hasContentReference()) {
        // Get the type for this container from a named reference to another element.
        return new QualifiedType(getContainerType(element), protogenConfig.getProtoPackage());
      } else {
        if (element.getTypeCount() > 1) {
          throw new IllegalArgumentException(
              "Unknown multiple type definition on element: " + element.getId());
        }
        // Note: this is the "fhir type", e.g., Resource, BackboneElement, boolean,
        // not the field type name.
        String normalizedFhirTypeName =
            normalizeType(Iterables.getOnlyElement(element.getTypeList()));

        // See https://jira.hl7.org/browse/FHIR-25262
        if (element.getId().getValue().equals("xhtml.id")) {
          normalizedFhirTypeName = "String";
        }

        if (normalizedFhirTypeName.equals("Resource")) {
          // We represent "Resource" FHIR types as "Any",
          // unless we are on the Bundle type, in which case we use "ContainedResources" type.
          // This allows defining resources in separate files without circular dependencies.
          if (allElements.get(0).getId().getValue().equals("Bundle")) {
            return new QualifiedType("ContainedResource", protogenConfig.getProtoPackage());
          } else {
            return new QualifiedType("Any", "google.protobuf");
          }
        }
        return new QualifiedType(normalizedFhirTypeName, protogenConfig.getProtoPackage());
      }
    }

    private ImmutableList<ElementDefinition> getDescendants(ElementDefinition element) {
      // The id of descendants should start with the parent id + at least one more token.
      String parentIdPrefix = element.getId().getValue() + ".";
      return allElements.stream()
          .filter(
              candidateElement -> candidateElement.getId().getValue().startsWith(parentIdPrefix))
          .collect(toImmutableList());
    }

    private ImmutableList<ElementDefinition> getDirectChildren(ElementDefinition element) {
      List<String> messagePathParts = Splitter.on('.').splitToList(element.getId().getValue());
      return getDescendants(element).stream()
          .filter(
              candidateElement -> {
                List<String> parts =
                    Splitter.on('.').splitToList(candidateElement.getId().getValue());
                // To be a direct child, the id should start with the parent id, and add a single
                // additional token.
                return parts.size() == messagePathParts.size() + 1;
              })
          .collect(toImmutableList());
    }

    private Optional<String> getBindingValueSetUrl(ElementDefinition element) {
      // TODO(b/297040090): We treat Language code as untyped, to match R4 behavior.
      if (element.getBinding().getStrength().getValue() != BindingStrengthCode.Value.REQUIRED
          || element
              .getBinding()
              .getValueSet()
              .getValue()
              .startsWith("http://hl7.org/fhir/ValueSet/all-languages")) {
        return Optional.empty();
      }
      String url = GeneratorUtils.getCanonicalUri(element.getBinding().getValueSet());
      if (url.isEmpty()) {
        return Optional.empty();
      }
      return Optional.of(url);
    }

    /** Returns the field name that should be used for an element. */
    private String getNameForElement(ElementDefinition element) {
      if (element
          .getId()
          .getValue()
          .equals(structureDefinition.getSnapshot().getElement(0).getId().getValue())) {
        return GeneratorUtils.getTypeName(structureDefinition);
      }

      return hyphenToCamel(lastIdToken(element).pathpart);
    }

    private long getDistinctTypeCount(ElementDefinition element) {
      // Don't do fancier logic if fast logic is sufficient.
      if (element.getTypeCount() < 2) {
        return element.getTypeCount();
      }
      return element.getTypeList().stream().map(type -> type.getCode()).distinct().count();
    }

    /** Build a single field for the proto. */
    private Optional<FieldDescriptorProto> buildField(ElementDefinition element, int nextTag)
        throws InvalidFhirException {
      FieldDescriptorProto.Label fieldSize = getFieldSize(element);
      if (fieldSize == null) {
        // This field has a max size of zero.  Do not emit a field.
        return Optional.empty();
      }

      FieldOptions.Builder options = FieldOptions.newBuilder();

      // Add a short description of the field.
      if (element.hasShort()) {
        options.setExtension(
            ProtoGeneratorAnnotations.fieldDescription, element.getShort().getValue());
      }

      if (isRequiredByFhir(element)) {
        options.setExtension(
            Annotations.validationRequirement, Annotations.Requirement.REQUIRED_BY_FHIR);
      } else if (element.getMin().getValue() != 0) {
        System.out.println("Unexpected minimum field count: " + element.getMin().getValue());
      }

      // Add typed reference options
      if (!isChoiceType(element)
          && element.getTypeCount() > 0
          && element.getType(0).getCode().getValue().equals("Reference")) {
        for (ElementDefinition.TypeRef type : element.getTypeList()) {
          if (type.getCode().getValue().equals("Reference") && type.getTargetProfileCount() > 0) {
            for (Canonical referenceType : type.getTargetProfileList()) {
              if (!referenceType.getValue().isEmpty()) {
                addReferenceTypeExtension(options, referenceType.getValue());
              }
            }
          }
        }
      }

      QualifiedType fieldType = getQualifiedFieldType(element);
      FieldDescriptorProto.Builder fieldBuilder =
          buildFieldInternal(
              getNameForElement(element),
              fieldType.type,
              fieldType.packageName,
              nextTag,
              fieldSize,
              options.build());
      return Optional.of(fieldBuilder.build());
    }

    /** Add a choice type container message to the proto. */
    private DescriptorProto makeChoiceType(ElementDefinition element) throws InvalidFhirException {
      QualifiedType choiceQualifiedType = getQualifiedFieldType(element);
      DescriptorProto.Builder choiceType =
          DescriptorProto.newBuilder().setName(choiceQualifiedType.getName());
      choiceType.getOptionsBuilder().setExtension(Annotations.isChoiceType, true);

      // Add error constraints on choice types.
      ImmutableList<String> expressions = getFhirPathErrorConstraints(element);
      if (!expressions.isEmpty()) {
        choiceType
            .getOptionsBuilder()
            .setExtension(Annotations.fhirPathMessageConstraint, expressions);
      }
      // Add warning constraints.
      ImmutableList<String> warnings = getFhirPathWarningConstraints(element);
      if (!warnings.isEmpty()) {
        choiceType
            .getOptionsBuilder()
            .setExtension(Annotations.fhirPathMessageWarningConstraint, warnings);
      }

      choiceType.addOneofDeclBuilder().setName("choice");

      int nextTag = 1;
      // Group types.
      List<ElementDefinition.TypeRef> types = new ArrayList<>();
      List<String> referenceTypes = new ArrayList<>();
      Set<String> foundTypes = new HashSet<>();
      for (ElementDefinition.TypeRef type : element.getTypeList()) {
        if (fhirPackage.getSemanticVersion().equals("4.0.1")
            && element.getId().getValue().equals("Extension.value[x]")
            && type.getCode().getValue().equals("Meta")) {
          // To match legacy behavior, skip the meta field in extension value.
          continue;
        }
        if (!foundTypes.contains(type.getCode().getValue())) {
          types.add(type);
          foundTypes.add(type.getCode().getValue());
        }

        if (type.getCode().getValue().equals("Reference") && type.getTargetProfileCount() > 0) {
          for (Canonical referenceType : type.getTargetProfileList()) {
            if (!referenceType.getValue().isEmpty()) {
              referenceTypes.add(referenceType.getValue());
            }
          }
        }
      }

      for (ElementDefinition.TypeRef type : types) {
        String fieldName =
            Ascii.toLowerCase(type.getCode().getValue().substring(0, 1))
                + type.getCode().getValue().substring(1);

        // There are two cases of choice type fields:
        // a) we need to generate a custom type for the field (e.g., a bound code, or a profiled
        // type)
        // b) it's an already existing type, so just generate the field.

        // Check for a slice of this type on the choice, e.g., value[x]:valueCode
        // If one is found, use that to create the choice field and type.  Otherwise, just use the
        // value[x] element itself.
        Optional<DescriptorProto> typeFromChoiceElement =
            buildNonChoiceTypeNestedTypeIfNeeded(element);

        if (typeFromChoiceElement.isPresent()) {
          // There is a sub type
          choiceType.addNestedType(typeFromChoiceElement.get());
          QualifiedType sliceType =
              choiceQualifiedType.childType(typeFromChoiceElement.get().getName());
          FieldDescriptorProto.Builder fieldBuilder =
              buildFieldInternal(
                      fieldName,
                      sliceType.type,
                      sliceType.packageName,
                      nextTag++,
                      FieldDescriptorProto.Label.LABEL_OPTIONAL,
                      FieldOptions.getDefaultInstance())
                  .setOneofIndex(0);
          choiceType.addField(fieldBuilder);
        } else {
          // If no custom type was generated, just use the type name from the core FHIR types.
          FieldOptions.Builder options = FieldOptions.newBuilder();
          if (fieldName.equals("reference")) {
            for (String referenceType : referenceTypes) {
              addReferenceTypeExtension(options, referenceType);
            }
          }
          FieldDescriptorProto.Builder fieldBuilder =
              buildFieldInternal(
                      fieldName,
                      normalizeType(type),
                      protogenConfig.getProtoPackage(),
                      nextTag++,
                      FieldDescriptorProto.Label.LABEL_OPTIONAL,
                      options.build())
                  .setOneofIndex(0);
          choiceType.addField(fieldBuilder);
        }
      }
      return choiceType.build();
    }

    private boolean isRequiredByFhir(ElementDefinition element) {
      return element.getMin().getValue() == 1;
    }
  }

  private static final ImmutableSet<String> DATATYPES_TO_SKIP =
      ImmutableSet.of(
          // References are handled separately, by addReferenceType, since they have typed reference
          // ID fields that are not a part of the FHIR spec.
          "http://hl7.org/fhir/StructureDefinition/Reference",
          // Skip over profile of ElementDefinition for Data Elements - we just use
          // ElementDefinition.
          "http://hl7.org/fhir/StructureDefinition/elementdefinition-de",
          // R4 mistakenly labels these example profiles as datatypes.
          "http://hl7.org/fhir/StructureDefinition/example-section-library",
          "http://hl7.org/fhir/StructureDefinition/example-composition");

  public FileDescriptorProto generateDatatypesFileDescriptor(List<String> resourceNames)
      throws InvalidFhirException {
    ImmutableList<StructureDefinition> messages =
        stream(fhirPackage.structureDefinitions())
            .filter(
                def ->
                    (def.getKind().getValue() == StructureDefinitionKindCode.Value.PRIMITIVE_TYPE
                            || def.getKind().getValue()
                                == StructureDefinitionKindCode.Value.COMPLEX_TYPE)
                        && !DATATYPES_TO_SKIP.contains(def.getUrl().getValue())
                        && !def.getBaseDefinition()
                            .getValue()
                            .equals("http://hl7.org/fhir/StructureDefinition/Extension"))
            .collect(toImmutableList());

    FileDescriptorProto.Builder fileBuilder =
        generateFileDescriptor(messages, fhirPackage.getSemanticVersion());

    addReferenceType(fileBuilder, resourceNames);
    addReferenceIdType(fileBuilder);

    return protogenConfig.getLegacyRetagging()
        ? retagFile(fileBuilder.build(), protogenConfig)
        : fileBuilder.build();
  }

  // TODO(b/292116008): Infer version directly from Structure Definition directly.  Currently not
  // possible since this uses R4 StructureDefinition, which can't parse R5 version code.
  public FileDescriptorProto generateResourceFileDescriptor(
      StructureDefinition def, String semanticVersion) throws InvalidFhirException {
    FileDescriptorProto file =
        generateFileDescriptor(ImmutableList.of(def), semanticVersion)
            .addDependency(protogenConfig.getSourceDirectory() + "/datatypes.proto")
            .addDependency("google/protobuf/any.proto")
            .build();
    return protogenConfig.getLegacyRetagging()
        ? FieldRetagger.retagFile(file, protogenConfig)
        : file;
  }

  // Generates a single file for two types: the Bundle type, and the ContainedResource type.
  public FileDescriptorProto generateBundleAndContainedResource(
      StructureDefinition bundleDefinition,
      String semanticVersion,
      List<String> resourceTypes,
      int fieldNumberOffset)
      throws InvalidFhirException {
    FileDescriptorProto.Builder fileBuilder =
        generateResourceFileDescriptor(bundleDefinition, semanticVersion).toBuilder();

    DescriptorProto.Builder contained =
        fileBuilder
            .addMessageTypeBuilder()
            .setName("ContainedResource")
            .addOneofDecl(OneofDescriptorProto.newBuilder().setName("oneof_resource"));
    // When generating contained resources, iterate through all the resources sorted alphabetically,
    // assigning tag numbers as you go
    TreeSet<String> sortedResources = new TreeSet<>(resourceTypes);
    sortedResources.add("Bundle");
    int tagNumber = 1 + fieldNumberOffset;
    for (String type : sortedResources) {
      if (!type.equals("Bundle")) {
        // Don't add a dep for Bundle since it's defined in the same file.
        fileBuilder.addDependency(
            new File(
                    protogenConfig.getSourceDirectory()
                        + "/resources/"
                        + GeneratorUtils.resourceNameToFileName(type))
                .toString());
      }
      contained.addField(
          FieldDescriptorProto.newBuilder()
              .setName(CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, type))
              .setNumber(tagNumber++)
              .setTypeName("." + protogenConfig.getProtoPackage() + "." + type)
              .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
              .setOneofIndex(0 /* the oneof_resource */)
              .build());
    }
    return protogenConfig.getLegacyRetagging()
        ? retagFile(fileBuilder.build(), protogenConfig)
        : fileBuilder.build();
  }

  // We generate a custom Reference datatype, since we have typed reference ID fields that are
  // not a part of the FHIR spec.
  // For instance, a JSON FHIR Reference of {"reference: "Patient/1234"} is represented in FhirProto
  // as {patientId: "1234"}, where patientId is a a ReferenceId.
  private void addReferenceType(
      FileDescriptorProto.Builder fileBuilder, List<String> resourceNames) {
    int nextTag = 1;
    DescriptorProto.Builder reference =
        fileBuilder
            .addMessageTypeBuilder()
            .setName("Reference")
            .setOptions(
                MessageOptions.newBuilder()
                    .setExtension(
                        ProtoGeneratorAnnotations.messageDescription,
                        " A reference from one resource to another."
                            + " See https://www.hl7.org/fhir/datatypes.html#Reference.")
                    .setExtension(
                        Annotations.structureDefinitionKind,
                        Annotations.StructureDefinitionKindValue.KIND_COMPLEX_TYPE)
                    .addExtension(Annotations.fhirReferenceType, "Resource")
                    .setExtension(
                        Annotations.fhirStructureDefinitionUrl,
                        "http://hl7.org/fhir/StructureDefinition/Reference"));

    reference
        .addFieldBuilder()
        .setName("id")
        .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
        .setTypeName("." + protogenConfig.getProtoPackage() + ".String")
        .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
        .setOptions(
            FieldOptions.newBuilder()
                .setExtension(
                    ProtoGeneratorAnnotations.fieldDescription, "xml:id (or equivalent in JSON)"))
        .setNumber(nextTag++);
    reference
        .addFieldBuilder()
        .setName("extension")
        .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
        .setTypeName("." + protogenConfig.getProtoPackage() + ".Extension")
        .setLabel(FieldDescriptorProto.Label.LABEL_REPEATED)
        .setOptions(
            FieldOptions.newBuilder()
                .setExtension(
                    ProtoGeneratorAnnotations.fieldDescription,
                    "Additional Content defined by implementations"))
        .setNumber(nextTag++);
    reference
        .addFieldBuilder()
        .setName("type")
        .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
        .setTypeName("." + protogenConfig.getProtoPackage() + ".Uri")
        .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
        .setOptions(
            FieldOptions.newBuilder()
                .setExtension(
                    ProtoGeneratorAnnotations.fieldDescription,
                    "Type the reference refers to (e.g. \"Patient\")"
                        + " - must be a resource in resources"))
        .setNumber(nextTag++);

    reference
        .addOneofDeclBuilder()
        .setName("reference")
        .setOptions(OneofOptions.newBuilder().setExtension(Annotations.fhirOneofIsOptional, true));

    reference.addField(
        FieldDescriptorProto.newBuilder()
            .setName("uri")
            .setNumber(nextTag++)
            .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
            .setTypeName("." + protogenConfig.getProtoPackage() + ".String")
            .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
            .setOptions(
                FieldOptions.newBuilder()
                    .setExtension(
                        ProtoGeneratorAnnotations.fieldDescription,
                        "Field representing absolute URIs, which are untyped."))
            .setOneofIndex(0)
            .build());

    reference.addField(
        FieldDescriptorProto.newBuilder()
            .setName("fragment")
            .setNumber(nextTag++)
            .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
            .setTypeName("." + protogenConfig.getProtoPackage() + ".String")
            .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
            .setOptions(
                FieldOptions.newBuilder()
                    .setExtension(
                        ProtoGeneratorAnnotations.fieldDescription,
                        "Field representing fragment URIs, which are untyped, and represented here"
                            + " without the leading '#'"))
            .setOneofIndex(0)
            .build());

    reference.addField(
        FieldDescriptorProto.newBuilder()
            .setName("resource_id")
            .setNumber(nextTag++)
            .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
            .setTypeName("." + protogenConfig.getProtoPackage() + ".ReferenceId")
            .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
            .setOptions(
                FieldOptions.newBuilder()
                    .setExtension(Annotations.referencedFhirType, "Resource")
                    .setExtension(
                        ProtoGeneratorAnnotations.fieldDescription,
                        "Typed relative urls are represented here."))
            .setOneofIndex(0)
            .build());

    TreeSet<String> sortedResources = new TreeSet<>(resourceNames);
    // To match legacy R4 ordering, ensure MetdataResource is last (if present.
    boolean hasMetadataResource = false;
    for (String type : sortedResources) {
      if (type.equals("MetadataResource")) {
        hasMetadataResource = true;
      } else {
        reference.addField(
            FieldDescriptorProto.newBuilder()
                .setName(CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, type) + "_id")
                .setNumber(nextTag++)
                .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
                .setTypeName("." + protogenConfig.getProtoPackage() + ".ReferenceId")
                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                .setOptions(
                    FieldOptions.newBuilder().setExtension(Annotations.referencedFhirType, type))
                .setOneofIndex(0)
                .build());
      }
    }

    if (hasMetadataResource) {
      reference.addField(
          FieldDescriptorProto.newBuilder()
              .setName(
                  CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, "MetadataResource")
                      + "_id")
              .setNumber(nextTag++)
              .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
              .setTypeName("." + protogenConfig.getProtoPackage() + ".ReferenceId")
              .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
              .setOptions(
                  FieldOptions.newBuilder()
                      .setExtension(Annotations.referencedFhirType, "MetadataResource"))
              .setOneofIndex(0)
              .build());
    }

    reference.addField(
        FieldDescriptorProto.newBuilder()
            .setName("identifier")
            .setNumber(nextTag++)
            .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
            .setTypeName("." + protogenConfig.getProtoPackage() + ".Identifier")
            .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
            .setOptions(
                FieldOptions.newBuilder()
                    .setExtension(
                        ProtoGeneratorAnnotations.fieldDescription,
                        "Logical reference, when literal reference is not known"))
            .build());

    reference.addField(
        FieldDescriptorProto.newBuilder()
            .setName("display")
            .setNumber(nextTag++)
            .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
            .setTypeName("." + protogenConfig.getProtoPackage() + ".String")
            .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
            .setOptions(
                FieldOptions.newBuilder()
                    .setExtension(
                        ProtoGeneratorAnnotations.fieldDescription,
                        "Text alternative for the resource."))
            .build());
  }

  // Adds the "Reference ID" Struct.  This is a more-strongly typed representation for the
  // the Reference.reference field when used for relative reference.
  // For instance, if Reference.reference is "Patient/1234" in pure JSON FHIR, this is
  // represented in FhirProto as {patientId: "1234"}, where patientId is a a ReferenceId.
  private void addReferenceIdType(FileDescriptorProto.Builder fileBuilder) {
    DescriptorProto.Builder referenceId =
        fileBuilder
            .addMessageTypeBuilder()
            .setName("ReferenceId")
            .setOptions(
                MessageOptions.newBuilder()
                    .setExtension(
                        ProtoGeneratorAnnotations.messageDescription,
                        "Typed representation of relative references for the Reference.reference "
                            + " field. For instance, a JSON FHIR reference of 'Patient/1234' is"
                            + " represented in FhirProto as {patientId {value:'1234'} },"
                            + " where patientId is a field of type ReferenceId.")
                    .setExtension(
                        Annotations.structureDefinitionKind,
                        Annotations.StructureDefinitionKindValue.KIND_PRIMITIVE_TYPE));

    referenceId
        .addFieldBuilder()
        .setName("value")
        .setType(FieldDescriptorProto.Type.TYPE_STRING)
        .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
        .setOptions(
            FieldOptions.newBuilder()
                .setExtension(
                    ProtoGeneratorAnnotations.fieldDescription,
                    "Id of the resource being referenced."))
        .setNumber(1);
    referenceId
        .addFieldBuilder()
        .setName("history")
        .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
        .setTypeName("." + protogenConfig.getProtoPackage() + ".Id")
        .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
        .setOptions(
            FieldOptions.newBuilder()
                .setExtension(
                    ProtoGeneratorAnnotations.fieldDescription, "History version, if present."))
        .setNumber(2);
    referenceId
        .addFieldBuilder()
        .setName("id")
        .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
        .setTypeName("." + protogenConfig.getProtoPackage() + ".String")
        .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
        .setOptions(
            FieldOptions.newBuilder()
                .setExtension(
                    ProtoGeneratorAnnotations.fieldDescription, "xml:id (or equivalent in JSON)"))
        .setNumber(3);
    referenceId
        .addFieldBuilder()
        .setName("extension")
        .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
        .setTypeName("." + protogenConfig.getProtoPackage() + ".Extension")
        .setLabel(FieldDescriptorProto.Label.LABEL_REPEATED)
        .setOptions(
            FieldOptions.newBuilder()
                .setExtension(
                    ProtoGeneratorAnnotations.fieldDescription,
                    "Additional Content defined by implementations"))
        .setNumber(4);
  }

  private FileDescriptorProto.Builder generateFileDescriptor(
      List<StructureDefinition> defs, String semanticVersion) throws InvalidFhirException {
    FileDescriptorProto.Builder builder = FileDescriptorProto.newBuilder();
    builder.setPackage(protogenConfig.getProtoPackage()).setSyntax("proto3");
    FileOptions.Builder options = FileOptions.newBuilder();
    if (!protogenConfig.getJavaProtoPackage().isEmpty()) {
      options.setJavaPackage(protogenConfig.getJavaProtoPackage()).setJavaMultipleFiles(true);
    }

    // TODO(b/292116008): Don't rely on precompiled version enum.
    options.setExtension(
        Annotations.fhirVersion, convertSemanticVersionToAnnotation(semanticVersion));

    builder.setOptions(options);
    for (StructureDefinition def : defs) {
      builder.addMessageType(new PerDefinitionGenerator(def).generate());
    }

    builder.addDependency(new File(GeneratorUtils.ANNOTATION_PATH, "annotations.proto").toString());
    builder.addDependency(
        new File(protogenConfig.getSourceDirectory() + "/codes.proto").toString());
    builder.addDependency(
        new File(protogenConfig.getSourceDirectory() + "/valuesets.proto").toString());

    return builder;
  }

  private static boolean dependsOnTypes(DescriptorProto descriptor, ImmutableSet<String> types) {

    for (FieldDescriptorProto field : descriptor.getFieldList()) {
      if (types.contains(field.getTypeName())) {
        return true;
      }
    }
    for (DescriptorProto nested : descriptor.getNestedTypeList()) {
      if (dependsOnTypes(nested, types)) {
        return true;
      }
    }
    return false;
  }

  private static Optional<String> getPrimitiveRegex(ElementDefinition element)
      throws InvalidFhirException {
    List<Message> regexExtensions =
        Extensions.getExtensionsWithUrl(REGEX_EXTENSION_URL, element.getType(0));
    if (regexExtensions.isEmpty()) {
      return Optional.empty();
    }
    if (regexExtensions.size() > 1) {
      throw new InvalidFhirException(
          "Multiple regex extensions found on " + element.getId().getValue());
    }
    return Optional.of(
        (String) Extensions.getExtensionValue(regexExtensions.get(0), "string_value"));
  }

  /**
   * Fields of the abstract types Element or BackboneElement are containers, which contain internal
   * fields (including possibly nested containers). Also, the top-level message is a container. See
   * https://www.hl7.org/fhir/backboneelement.html for additional information.
   */
  private static boolean isContainer(ElementDefinition element) {
    if (element.getTypeCount() != 1) {
      return false;
    }
    if (element.getId().getValue().indexOf('.') == -1) {
      return true;
    }
    String type = element.getType(0).getCode().getValue();
    return type.equals("BackboneElement") || type.equals("Element");
  }

  /**
   * Does this element have a single, well-defined type? For example: string, Patient or
   * Observation.ReferenceRange, as opposed to one of a set of types, most commonly encoded in field
   * with names like "value[x]".
   */
  private static boolean isSingleType(ElementDefinition element) {
    // If the type of this element is defined by referencing another element,
    // then it has a single defined type.
    if (element.getTypeCount() == 0 && element.hasContentReference()) {
      return true;
    }

    // Loop through the list of types. There may be multiple copies of the same
    // high-level type, for example, multiple kinds of References.
    Set<String> types = new HashSet<>();
    for (ElementDefinition.TypeRef type : element.getTypeList()) {
      if (type.hasCode()) {
        types.add(type.getCode().getValue());
      }
    }
    return types.size() == 1;
  }

  private static boolean isChoiceType(ElementDefinition element) {
    return lastIdToken(element).isChoiceType;
  }

  // Returns the FHIRPath Error constraints on the given element, if any.
  private static ImmutableList<String> getFhirPathErrorConstraints(ElementDefinition element) {
    return element.getConstraintList().stream()
        .filter(ElementDefinition.Constraint::hasExpression)
        .filter(
            constraint -> constraint.getSeverity().getValue() == ConstraintSeverityCode.Value.ERROR)
        .map(constraint -> constraint.getExpression().getValue())
        .filter(expression -> !EXCLUDED_FHIR_CONSTRAINTS.contains(expression))
        .collect(toImmutableList());
  }

  // Returns the FHIRPath Warning constraints on the given element, if any.
  private static ImmutableList<String> getFhirPathWarningConstraints(ElementDefinition element) {
    return element.getConstraintList().stream()
        .filter(ElementDefinition.Constraint::hasExpression)
        .filter(
            constraint ->
                constraint.getSeverity().getValue() == ConstraintSeverityCode.Value.WARNING)
        .map(constraint -> constraint.getExpression().getValue())
        .filter(expression -> !EXCLUDED_FHIR_CONSTRAINTS.contains(expression))
        .collect(toImmutableList());
  }

  private static FieldDescriptorProto.Label getFieldSize(ElementDefinition element) {
    if (element.getMax().getValue().equals("0")) {
      // This field doesn't actually exist.
      return null;
    }
    return element.getMax().getValue().equals("1")
        ? FieldDescriptorProto.Label.LABEL_OPTIONAL
        : FieldDescriptorProto.Label.LABEL_REPEATED;
  }

  private static String hyphenToCamel(String fieldName) {
    int i;
    while ((i = fieldName.indexOf("-")) != -1) {
      fieldName =
          fieldName.substring(0, i)
              + Ascii.toUpperCase(fieldName.substring(i + 1, i + 2))
              + fieldName.substring(i + 2);
    }
    return fieldName;
  }

  private void addReferenceTypeExtension(FieldOptions.Builder options, String referenceUrl) {
    options.addExtension(
        Annotations.validReferenceType, referenceUrl.substring(referenceUrl.lastIndexOf('/') + 1));
  }

  private static FieldDescriptorProto.Builder buildFieldInternal(
      String fhirName,
      String fieldType,
      String fieldPackage,
      int tag,
      FieldDescriptorProto.Label size,
      FieldOptions options) {
    FieldDescriptorProto.Builder builder =
        FieldDescriptorProto.newBuilder()
            .setNumber(tag)
            .setType(FieldDescriptorProto.Type.TYPE_MESSAGE)
            .setLabel(size)
            .setTypeName("." + fieldPackage + "." + fieldType);

    String fieldName =
        RESERVED_FIELD_NAMES.contains(fhirName)
            ? toFieldNameCase(fhirName) + "_value"
            : toFieldNameCase(fhirName);

    builder.setName(fieldName);
    if (!fhirName.equals(snakeCaseToJsonCase(fieldName))) {
      // We can't recover the original FHIR field name with a to-json transform, so add an
      // annotation
      // for the original field name.
      builder.setJsonName(fhirName);
    }

    // Add annotations.
    if (!options.equals(FieldOptions.getDefaultInstance())) {
      builder.setOptions(options);
    }
    return builder;
  }

  private String normalizeType(ElementDefinition.TypeRef type) {
    String code = type.getCode().getValue();
    if (code.startsWith(FHIRPATH_TYPE_PREFIX)) {
      for (Extension extension : type.getExtensionList()) {
        if (extension.getUrl().getValue().equals(FHIR_TYPE_EXTENSION_URL)) {
          return toFieldTypeCase(extension.getValue().getUrl().getValue());
        }
      }
    }
    if (type.getProfileCount() == 0 || type.getProfile(0).getValue().isEmpty()) {
      return toFieldTypeCase(code);
    }
    String profileUrl = type.getProfile(0).getValue();
    return profileUrl.substring(profileUrl.lastIndexOf('/') + 1);
  }

  private static String snakeCaseToJsonCase(String snakeString) {
    return CaseFormat.LOWER_UNDERSCORE.to(CaseFormat.LOWER_CAMEL, snakeString);
  }

  private static boolean isContainedResourceField(ElementDefinition element) {
    return element.getBase().getPath().getValue().equals("DomainResource.contained");
  }

  /** Converts from a semantic version id, e.g., "4.0.1". */
  // TODO(b/292116008): Deprecate FhirVersion annotation and eliminate this method, since
  // this does not automatically adapt to new versions.
  private static Annotations.FhirVersion convertSemanticVersionToAnnotation(
      String semanticVersion) {
    if (semanticVersion.startsWith("2.")) {
      return Annotations.FhirVersion.DSTU2;
    } else if (semanticVersion.startsWith("3.")) {
      return Annotations.FhirVersion.STU3;
    } else if (semanticVersion.startsWith("4.")) {
      if (semanticVersion.startsWith("4.3.")) {
        return Annotations.FhirVersion.R4B;
      }
      return Annotations.FhirVersion.R4;
    } else if (semanticVersion.startsWith("5.")) {
      return Annotations.FhirVersion.R5;
    }
    return Annotations.FhirVersion.FHIR_VERSION_UNKNOWN;
  }
}
