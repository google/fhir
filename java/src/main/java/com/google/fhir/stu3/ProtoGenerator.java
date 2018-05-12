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

package com.google.fhir.stu3;

import com.google.common.base.CaseFormat;
import com.google.common.base.Joiner;
import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.google.fhir.stu3.proto.Annotations;
import com.google.fhir.stu3.proto.BindingStrengthCode;
import com.google.fhir.stu3.proto.ContainedResource;
import com.google.fhir.stu3.proto.ElementDefinition;
import com.google.fhir.stu3.proto.ElementDefinitionBindingName;
import com.google.fhir.stu3.proto.StructureDefinition;
import com.google.fhir.stu3.proto.StructureDefinitionExplicitTypeName;
import com.google.fhir.stu3.proto.StructureDefinitionKindCode;
import com.google.fhir.stu3.proto.Uri;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumValueDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldOptions;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileOptions;
import com.google.protobuf.DescriptorProtos.MessageOptions;
import com.google.protobuf.DescriptorProtos.OneofDescriptorProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Set;
import java.util.TreeSet;

/** A class which turns FHIR StructureDefinitions into protocol messages. */
public class ProtoGenerator {

  // Certain type names are reserved symbols in various languages.
  private static final ImmutableMap<String, String> RESERVED_TYPE_NAMES =
      ImmutableMap.of("Object", "ObjectType");

  // Map of primitive type ids to proto message names.
  private static final ImmutableMap<String, String> FIELD_TYPE_MAP = getFieldTypeMap();

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

  // For various reasons, we rename certain codes.
  private static final ImmutableMap<String, String> RENAMED_CODE_TYPES = getRenamedCodeTypes();

  // Certain field names are reserved symbols in various languages.
  private static final ImmutableMap<String, String> reservedFieldNames =
      ImmutableMap.of(
          "assert", "assert_value",
          "for", "for_value",
          "hasAnswer", "has_answer_value",
          "package", "package_value",
          "string", "string_value");

  private static final String STRUCTURE_DEFINITION_PREFIX =
      "http://hl7.org/fhir/StructureDefinition/";

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
      FieldDescriptorProto.newBuilder()
          .setName("timezone")
          .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
          .setType(FieldDescriptorProto.Type.TYPE_STRING)
          .setNumber(2)
          .build();

  private static final ImmutableMap<String, FieldDescriptorProto.Type> PRIMITIVE_TYPE_OVERRIDES =
      ImmutableMap.of(
          "base64Binary", FieldDescriptorProto.Type.TYPE_BYTES,
          "boolean", FieldDescriptorProto.Type.TYPE_BOOL,
          "integer", FieldDescriptorProto.Type.TYPE_SINT32,
          "positiveInt", FieldDescriptorProto.Type.TYPE_UINT32,
          "unsignedInt", FieldDescriptorProto.Type.TYPE_UINT32);

  // Should we use custom types for constrained references?
  private static final boolean USE_TYPED_REFERENCES = false;

  private final String packageName;
  private final String protoRootPath;

  public ProtoGenerator(String packageName, String protoRootPath) {
    this.packageName = packageName;
    this.protoRootPath = protoRootPath;
  }

  private static ImmutableMap<String, String> getFieldTypeMap() {
    Map<String, String> fieldTypeMap = new HashMap<>();
    fieldTypeMap.put("base64Binary", "Base64Binary");
    fieldTypeMap.put("boolean", "Boolean");
    fieldTypeMap.put("code", "Code");
    fieldTypeMap.put("decimal", "Decimal");
    fieldTypeMap.put("dateTime", "DateTime");
    fieldTypeMap.put("date", "Date");
    fieldTypeMap.put("id", "Id");
    fieldTypeMap.put("instant", "Instant");
    fieldTypeMap.put("integer", "Integer");
    fieldTypeMap.put("markdown", "Markdown");
    fieldTypeMap.put("positiveInt", "PositiveInt");
    fieldTypeMap.put("oid", "Oid");
    fieldTypeMap.put("string", "String");
    fieldTypeMap.put("time", "Time");
    fieldTypeMap.put("unsignedInt", "UnsignedInt");
    fieldTypeMap.put("uri", "Uri");
    fieldTypeMap.put("xhtml", "String");
    fieldTypeMap.put("Resource", "ContainedResource");
    return ImmutableMap.copyOf(fieldTypeMap);
  }

  private static ImmutableMap<String, String> getRenamedCodeTypes() {
    Map<String, String> renamedCodeTypes = new HashMap<>();
    renamedCodeTypes.put("ActivityDefinitionKindCode", "ResourceTypeCode");
    renamedCodeTypes.put("ActivityParticipantTypeCode", "ActionParticipantTypeCode");
    renamedCodeTypes.put("ClaimResponseStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("ClaimStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("CommunicationRequestStatusCode", "RequestStatusCode");
    renamedCodeTypes.put("CommunicationPriorityCode", "RequestPriorityCode");
    renamedCodeTypes.put("CommunicationStatusCode", "EventStatusCode");
    renamedCodeTypes.put("CompartmentCodeCode", "CompartmentTypeCode");
    renamedCodeTypes.put("ConditionClinicalStatusCode", "ConditionClinicalStatusCodesCode");
    renamedCodeTypes.put("ContractStatusCode", "ContractResourceStatusCode");
    renamedCodeTypes.put("CoverageStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("DetectedIssueStatusCode", "ObservationStatusCode");
    renamedCodeTypes.put("DeviceRequestStatusCode", "RequestStatusCode");
    renamedCodeTypes.put("DocumentConfidentialityCode", "ConfidentialityClassificationCode");
    renamedCodeTypes.put("EligibilityRequestStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("EligibilityResponseStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("EnrollmentRequestStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("EnrollmentResponseStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("ImmunizationStatusCode", "ImmunizationStatusCodesCode");
    renamedCodeTypes.put("ParticipantStatusCode", "ParticipationStatusCode");
    renamedCodeTypes.put("PaymentNoticeStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("PaymentReconciliationStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("ProcedureRequestIntentCode", "RequestIntentCode");
    renamedCodeTypes.put("ProcedureRequestPriorityCode", "RequestPriorityCode");
    renamedCodeTypes.put("ProcedureRequestStatusCode", "RequestStatusCode");
    renamedCodeTypes.put("ProcedureStatusCode", "EventStatusCode");
    renamedCodeTypes.put("ProcessRequestStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("ProcessResponseStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("ReferralRequestIntentCode", "RequestIntentCode");
    renamedCodeTypes.put("ReferralRequestPriorityCode", "RequestPriorityCode");
    renamedCodeTypes.put("ReferralRequestStatusCode", "RequestStatusCode");
    renamedCodeTypes.put("ReferralCategoryCode", "RequestIntentCode");
    renamedCodeTypes.put("ReferralPriorityCode", "RequestPriorityCode");
    renamedCodeTypes.put("ReferredDocumentStatusCode", "CompositionStatusCode");
    renamedCodeTypes.put("RiskAssessmentStatusCode", "ObservationStatusCode");
    renamedCodeTypes.put("SectionModeCode", "ListModeCode");
    renamedCodeTypes.put("TaskIntentCode", "RequestIntentCode");
    renamedCodeTypes.put("TaskPriorityCode", "RequestPriorityCode");
    renamedCodeTypes.put("VisionStatusCode", "FinancialResourceStatusCode");
    return ImmutableMap.copyOf(renamedCodeTypes);
  }

  /**
   * Generate a proto descriptor from a StructureDefinition, using the snapshot form of the
   * definition. For a more elaborate discussion of these versions, see
   * https://www.hl7.org/fhir/structuredefinition.html.
   */
  public DescriptorProto generateProto(StructureDefinition def) {
    List<ElementDefinition> elementList = def.getSnapshot().getElementList();
    ElementDefinition root = elementList.get(0);

    // Build a top-level message description.
    StringBuilder comment =
        new StringBuilder()
            .append("Auto-generated from StructureDefinition for ")
            .append(def.getName().getValue());
    if (def.getMeta().hasLastUpdated()) {
      comment.append(", last updated ").append(new InstantWrapper(def.getMeta().getLastUpdated()));
    }
    comment.append(".");
    if (root.hasShort()) {
      comment.append("\n").append((root.getShort().getValue() + ".").replaceAll("[\\n\\r]", "\n"));
    }

    // Add message-level annotations.
    DescriptorProto.Builder builder = DescriptorProto.newBuilder();
    builder.setOptions(
        MessageOptions.newBuilder()
            .setExtension(
                Annotations.structureDefinitionKind,
                Annotations.StructureDefinitionKindValue.valueOf(
                    "KIND_" + def.getKind().getValue()))
            .setExtension(Annotations.messageDescription, comment.toString())
            .build());

    // Build the name of the descriptor.
    String name = def.getId().getValue();
    if (Character.isLowerCase(name.charAt(0))) {
      name = CaseFormat.LOWER_HYPHEN.to(CaseFormat.UPPER_CAMEL, name);
    }

    // If this is a primitive type, generate the value field first.
    if (def.getKind().getValue() == StructureDefinitionKindCode.Value.PRIMITIVE_TYPE) {
      // Fix up the name. TODO(sundberg): remove the Xhtml special case.
      name =
          "Xhtml".equals(name) ? name : FIELD_TYPE_MAP.getOrDefault(def.getId().getValue(), name);
      generatePrimitiveValue(def, builder.setName(name));
    }

    // Build the actual descriptor, and replace the top-level name. If the first character is
    // lowercase, assume it needs to be camelcased.
    return generateMessage(root, elementList, builder).toBuilder().setName(name).build();
  }

  /** Generate a .proto file descriptor from a list of StructureDefinitions. */
  public FileDescriptorProto generateFileDescriptor(List<StructureDefinition> defs) {
    FileDescriptorProto.Builder builder = FileDescriptorProto.newBuilder();
    builder.setPackage(packageName).setSyntax("proto3");
    builder.addDependency(new File(protoRootPath, "annotations.proto").toString());
    builder.addDependency(new File(protoRootPath, "codes.proto").toString());
    builder.addDependency(new File(protoRootPath, "datatypes.proto").toString());
    FileOptions.Builder options = FileOptions.newBuilder();
    options.setJavaPackage("com." + packageName).setJavaMultipleFiles(true);
    builder.setOptions(options);
    for (StructureDefinition def : defs) {
      builder.addMessageType(generateProto(def));
    }
    return builder.build();
  }

  /**
   * Generate a .proto file descriptor from a list of StructureDefinitions, along with
   * ContainedResource message that contains only the resource types present in the generated
   * FileDescriptorProto.
   */
  public FileDescriptorProto addContainedResource(FileDescriptorProto descriptor) {
    FileDescriptorProto.Builder resultBuilder = descriptor.toBuilder();
    Set<String> types = new HashSet<>();
    for (DescriptorProto type : resultBuilder.getMessageTypeList()) {
      types.add(type.getName());
    }
    Descriptor containedResource = ContainedResource.getDescriptor();
    DescriptorProto.Builder contained = containedResource.toProto().toBuilder().clearField();
    for (FieldDescriptor field : containedResource.getFields()) {
      if (types.contains(field.getMessageType().getName())) {
        contained.addField(field.toProto());
      }
    }
    return resultBuilder.addMessageType(contained).build();
  }

  DescriptorProto generateMessage(
      ElementDefinition currentElement,
      List<ElementDefinition> elementList,
      DescriptorProto.Builder builder) {
    // Get the name of this message.
    List<String> messageNameParts =
        Splitter.on('.').splitToList(getContainerType(currentElement, elementList));
    String messageName = messageNameParts.get(messageNameParts.size() - 1);
    builder.setName(messageName);

    List<String> messagePathParts =
        Splitter.on('.').splitToList(currentElement.getPath().getValue());
    // When generating a descriptor for a primitive type, the value part may already be present.
    int nextTag = builder.getFieldCount() + 1;

    // Loop over the elements.
    for (ElementDefinition element : elementList) {
      List<String> parts = Splitter.on('.').splitToList(element.getPath().getValue());
      if (!element.getPath().getValue().startsWith(currentElement.getPath().getValue() + ".")
          || parts.size() != messagePathParts.size() + 1) {
        // Not relevant to the message we're building. Skip.
        continue;
      }

      // If this is a container type, define the inner message.
      if (isContainer(element)) {
        builder.addNestedType(generateMessage(element, elementList, DescriptorProto.newBuilder()));
      }

      if (isSingleType(element)) {
        // Generate a single field. If this field doesn't actually exist in this version of the
        // message, for example, the max attribute is 0, buildField returns null and no field
        // should be added.
        FieldDescriptorProto field = buildField(element, elementList, nextTag++);
        if (field != null) {
          builder.addField(field);
        }
      } else if (element.getPath().getValue().endsWith("[x]")) {
        // Emit a oneof to handle fhir fields that don't have a fixed type.
        addChoiceType(element, elementList, nextTag++, builder);
      } else {
        // We don't know how to deal with this kind of field. Skip for now.
        System.out.println("Skipping field: " + element.getPath().getValue());
        // We still increment the tag number for stability, and to allow manual fixes.
        nextTag++;
      }
    }

    return builder.build();
  }

  /** Generate the primitive value part of a datatype. */
  private void generatePrimitiveValue(StructureDefinition def, DescriptorProto.Builder builder) {
    // Find the value field.
    String valueFieldId = def.getId().getValue() + ".value";
    FieldDescriptorProto.Type primitiveType =
        PRIMITIVE_TYPE_OVERRIDES.getOrDefault(
            def.getId().getValue(), FieldDescriptorProto.Type.TYPE_STRING);
    ElementDefinition.TypeRef mockType =
        ElementDefinition.TypeRef.newBuilder().setCode(Uri.newBuilder().setValue("string")).build();
    for (ElementDefinition element : def.getSnapshot().getElementList()) {
      if (valueFieldId.equals(element.getId().getValue())) {
        // The value field typically has no type. We need to add a fake one here.
        ElementDefinition elementWithType =
            element.toBuilder().clearType().addType(mockType).build();
        FieldDescriptorProto field =
            buildField(elementWithType, null /* no nested types */, 1 /* nextTag */);
        if (field != null) {
          if (TIME_LIKE_PRECISION_MAP.containsKey(def.getId().getValue())) {
            // Handle time-like types differently.
            EnumDescriptorProto.Builder enumBuilder = PRECISION_ENUM.toBuilder();
            for (String value : TIME_LIKE_PRECISION_MAP.get(def.getId().getValue())) {
              enumBuilder.addValue(
                  EnumValueDescriptorProto.newBuilder()
                      .setName(value)
                      .setNumber(enumBuilder.getValueCount()));
            }
            builder.addEnumType(enumBuilder);
            builder.addField(
                field
                    .toBuilder()
                    .clearTypeName()
                    .setType(FieldDescriptorProto.Type.TYPE_INT64)
                    .setName("value_us"));
            if (TYPES_WITH_TIMEZONE.contains(def.getId().getValue())) {
              builder.addField(TIMEZONE_FIELD);
            }
            builder.addField(
                FieldDescriptorProto.newBuilder()
                    .setName("precision")
                    .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                    .setType(FieldDescriptorProto.Type.TYPE_ENUM)
                    .setTypeName("." + packageName + "." + builder.getName() + ".Precision")
                    .setNumber(builder.getFieldCount() + 1));
          } else {
            builder.addField(field.toBuilder().clearTypeName().setType(primitiveType));
          }
        }
      }
    }
  }

  /**
   * Fields of the abstract types Element or BackboneElement are containers, which contain internal
   * fields (including possibly nested containers). Also, the top-level message is a container. See
   * https://www.hl7.org/fhir/backboneelement.html for additional information.
   */
  private boolean isContainer(ElementDefinition element) {
    if (element.getTypeCount() != 1) {
      return false;
    }
    if (element.getPath().getValue().indexOf('.') == -1) {
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
  private boolean isSingleType(ElementDefinition element) {
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

  /** Extract the type of a container field, possibly by reference. */
  private String getContainerType(ElementDefinition element, List<ElementDefinition> elementList) {
    if (element.hasContentReference()) {
      try {
        // Find the named element which was referenced. We'll use the type of that element.
        ElementDefinition referencedElement =
            elementList
                .stream()
                .filter(
                    e ->
                        element.getContentReference().getValue().equals("#" + e.getId().getValue()))
                .filter(e -> isContainer(e))
                .findFirst()
                .get();
        return getContainerType(referencedElement, elementList);
      } catch (NoSuchElementException e) {
        throw new IllegalArgumentException(
            "Undefined reference: " + element.getContentReference(), e);
      }
    }

    // Any part of the path could have been renamed. We need to translate them all.
    List<String> typeNameParts = new ArrayList<>();
    List<String> elementPathParts = new ArrayList<>();
    for (String part : Splitter.on('.').split(element.getPath().getValue())) {
      elementPathParts.add(part);
      String path = Joiner.on('.').join(elementPathParts);
      ElementDefinition pathElement =
          elementList.stream().filter(e -> e.getPath().getValue().equals(path)).findFirst().get();
      List<StructureDefinitionExplicitTypeName> typeNames =
          ExtensionWrapper.fromExtensionsIn(pathElement)
              .getMatchingExtensions(StructureDefinitionExplicitTypeName.getDefaultInstance());
      if (part.endsWith("[x]")) {
        part = part.substring(0, part.length() - "[x]".length());
      }
      String typeName;
      if (!typeNames.isEmpty()) {
        typeName = typeNames.get(0).getValueString().getValue();
      } else if (RESERVED_TYPE_NAMES.containsKey(part)) {
        typeName = RESERVED_TYPE_NAMES.get(part);
      } else {
        typeName = CaseFormat.LOWER_CAMEL.to(CaseFormat.UPPER_CAMEL, part);
      }
      if (typeNameParts.contains(typeName)
          || (!typeNameParts.isEmpty() && (typeName.equals("Timing") || typeName.equals("Age")))) {
        // There's already a message with this name, append "Type" to disambiguate.
        typeName = typeName + "Type";
      }
      typeNameParts.add(typeName);
    }
    return Joiner.on('.').join(typeNameParts);
  }

  private String getFieldType(List<ElementDefinition.TypeRef> typeList) {
    if (typeList.isEmpty()) {
      throw new IllegalArgumentException("Empty typeList");
    }

    // Either this field is of a primitive type, or the type code should be
    // copied verbatim.
    String typeCode = typeList.get(0).getCode().getValue();
    if (FIELD_TYPE_MAP.containsKey(typeCode)) {
      // This field is of a primitive type.
      return FIELD_TYPE_MAP.get(typeCode);
    } else if (typeCode.equals("Reference") && USE_TYPED_REFERENCES) {
      // Sort.
      Set<String> refTypes = new TreeSet<>();
      for (ElementDefinition.TypeRef type : typeList) {
        refTypes.add(type.getTargetProfile().getValue());
      }
      String refType = null;
      for (String r : refTypes) {
        if (!r.isEmpty()) {
          if (!r.startsWith(STRUCTURE_DEFINITION_PREFIX)) {
            throw new IllegalArgumentException("Unsupported reference profile: " + r);
          }
          r = r.substring(r.lastIndexOf('/') + 1);
          if (refType == null) {
            refType = r;
          } else {
            refType = refType + "Or" + r;
          }
        }
      }
      if (refType != null && !refType.equals("Resource")) {
        // Specialize the reference type.
        return refType + typeCode;
      }
    }
    return typeCode;
  }

  /** Build a single field for the proto. */
  private FieldDescriptorProto buildField(
      ElementDefinition element, List<ElementDefinition> elementList, int nextTag) {
    FieldOptions.Builder options = FieldOptions.newBuilder();

    // Is this a choice type field?
    boolean isChoiceType = element.getPath().getValue().endsWith("[x]");

    // If there's a binding name, extract it here.
    String bindingName = null;
    List<ElementDefinitionBindingName> bindingNames =
        ExtensionWrapper.fromExtensionsIn(element.getBinding())
            .getMatchingExtensions(ElementDefinitionBindingName.getDefaultInstance());
    if (bindingNames.size() == 1) {
      bindingName = bindingNames.get(0).getValueString().getValue();
    }
    if (element.getBinding().getStrength().getValue().equals(BindingStrengthCode.Value.REQUIRED)
        && bindingName == null) {
      System.out.println(
          "Required binding found, but category is unknown: " + element.getBinding());
    }

    // Deduce the type of the field.
    String fieldType;
    if (element.getTypeCount() == 1 && element.getType(0).getCode().getValue().isEmpty()) {
      // This happens for primitive types; we ignore those here.
      return null;
    }
    if (isContainer(element) || element.hasContentReference() || isChoiceType) {
      // Get the type for this container, possibly from a named reference to another element.
      fieldType = getContainerType(element, elementList);
    } else {
      fieldType = getFieldType(element.getTypeList());
      if (fieldType.equals("Code") && bindingName != null) {
        fieldType = bindingName + "Code";
        fieldType = RENAMED_CODE_TYPES.getOrDefault(fieldType, fieldType);
      }
    }

    // Is this field required?
    if (element.getMin().getValue() == 1) {
      options.setExtension(
          Annotations.validationRequirement, Annotations.Requirement.REQUIRED_BY_FHIR);
    } else if (element.getMin().getValue() != 0) {
      System.out.println("Unexpected minimum field count: " + element.getMin().getValue());
    }

    // Is this field repeated?
    boolean repeated;
    if (element.getMax().getValue().equals("0")) {
      // This field doesn't actually exist.
      return null;
    } else if (element.getMax().getValue().equals("1")) {
      repeated = false;
    } else {
      repeated = true;
    }

    // Extract the name of the field.
    String fieldName =
        element.getPath().getValue().substring(element.getPath().getValue().lastIndexOf('.') + 1);
    // Is this field a choice type?
    if (isChoiceType) {
      fieldName = fieldName.substring(0, fieldName.length() - "[x]".length());
      options.setExtension(Annotations.isChoiceType, true);
    }

    // Add a short description of the field.
    if (element.hasShort()) {
      options.setExtension(Annotations.fieldDescription, element.getShort().getValue());
    }

    FieldDescriptorProto.Builder builder =
        buildFieldInternal(fieldName, fieldType, nextTag, repeated, options.build());
    return builder.build();
  }

  /** Add a choice type field to the proto. */
  private void addChoiceType(
      ElementDefinition element,
      List<ElementDefinition> elementList,
      int tag,
      DescriptorProto.Builder builder) {
    FieldDescriptorProto field = buildField(element, elementList, tag);
    if (field == null) {
      // We're actually not adding this field, most likely because it has a max_count of 0.
      return;
    }
    builder.addField(field);

    List<String> typeNameParts = Splitter.on('.').splitToList(field.getTypeName());
    DescriptorProto.Builder nested =
        builder.addNestedTypeBuilder().setName(typeNameParts.get(typeNameParts.size() - 1));
    OneofDescriptorProto.Builder oneof = nested.addOneofDeclBuilder().setName(field.getName());
    // Add validation requirements as necessary.
    if (element.getMin().getValue() == 1) {
      oneof
          .getOptionsBuilder()
          .setExtension(
              Annotations.oneofValidationRequirement, Annotations.Requirement.REQUIRED_BY_FHIR);
    }

    int nextTag = 1;
    // Group types.
    List<List<ElementDefinition.TypeRef>> types = new ArrayList<>();
    for (ElementDefinition.TypeRef type : element.getTypeList()) {
      if (types.isEmpty() || !types.get(types.size() - 1).get(0).getCode().equals(type.getCode())) {
        types.add(new ArrayList<ElementDefinition.TypeRef>());
      }
      types.get(types.size() - 1).add(type);
    }

    for (List<ElementDefinition.TypeRef> t : types) {
      String fieldType = getFieldType(t);
      String fieldName = t.get(0).getCode().getValue();
      FieldDescriptorProto.Builder fieldBuilder =
          buildFieldInternal(
                  fieldName, fieldType, nextTag++, false, FieldOptions.getDefaultInstance())
              .setOneofIndex(0);
      // TODO(sundberg): change the oneof name to avoid this.
      if (fieldBuilder.getName().equals(oneof.getName())) {
        fieldBuilder.setJsonName(fieldBuilder.getName());
        fieldBuilder.setName(fieldBuilder.getName() + "_value");
      }
      nested.addField(fieldBuilder);
    }
  }

  FieldDescriptorProto.Builder buildFieldInternal(
      String fieldName, String fieldType, int tag, boolean repeated, FieldOptions options) {
    FieldDescriptorProto.Builder builder =
        FieldDescriptorProto.newBuilder()
            .setNumber(tag)
            .setType(FieldDescriptorProto.Type.TYPE_MESSAGE);
    if (repeated) {
      builder.setLabel(FieldDescriptorProto.Label.LABEL_REPEATED);
    } else {
      builder.setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL);
    }

    List<String> fieldTypeParts = new ArrayList<>();
    for (String part : Splitter.on('.').split(fieldType)) {
      if (RESERVED_TYPE_NAMES.containsKey(part)) {
        fieldTypeParts.add(RESERVED_TYPE_NAMES.get(fieldType));
      } else {
        fieldTypeParts.add(CaseFormat.LOWER_CAMEL.to(CaseFormat.UPPER_CAMEL, part));
      }
    }
    fieldType = Joiner.on('.').join(fieldTypeParts);

    // Handle reserved symbols by hard-coded substitutions.
    builder.setTypeName("." + packageName + "." + fieldType);

    if (reservedFieldNames.containsKey(fieldName)) {
      builder.setName(reservedFieldNames.get(fieldName)).setJsonName(fieldName);
    } else {
      // Make sure the field name is snake case, as required by the proto style guide.
      fieldName = CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, fieldName);
      builder.setName(fieldName);
    }

    // Add annotations.
    if (!options.equals(FieldOptions.getDefaultInstance())) {
      builder.setOptions(options);
    }
    return builder;
  }
}
