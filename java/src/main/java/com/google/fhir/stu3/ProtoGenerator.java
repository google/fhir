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
import com.google.common.collect.Iterables;
import com.google.common.collect.MoreCollectors;
import com.google.fhir.stu3.proto.Annotations;
import com.google.fhir.stu3.proto.BindingStrengthCode;
import com.google.fhir.stu3.proto.ContainedResource;
import com.google.fhir.stu3.proto.ElementDefinition;
import com.google.fhir.stu3.proto.ElementDefinitionBindingName;
import com.google.fhir.stu3.proto.ElementDefinitionExplicitTypeName;
import com.google.fhir.stu3.proto.ElementDefinitionRegex;
import com.google.fhir.stu3.proto.StructureDefinition;
import com.google.fhir.stu3.proto.StructureDefinitionKindCode;
import com.google.fhir.stu3.proto.TypeDerivationRuleCode;
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
import com.google.protobuf.Message;
import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.Set;
import java.util.TreeSet;
import java.util.function.Function;
import java.util.stream.Collectors;

/** A class which turns FHIR StructureDefinitions into protocol messages. */
// TODO(nickgeorge): Move a bunch of the public static methods into ProtoGeneratorUtils.
public class ProtoGenerator {

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
      new ImmutableMap.Builder<String, String>()
          .put("assert", "assert_value")
          .put("for", "for_value")
          .put("hasAnswer", "has_answer_value")
          .put("package", "package_value")
          .put("string", "string_value")
          .put("class", "class_value")
          .build();

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

  // Mapping from urls for Typed Extensions that the generator is aware of, to the type that they
  // should be inlined as.  For Extensions that only consist of a single primitive type, this is
  // that primitive type.  For Complex extensions (e.g., extensions containing other typed
  // extensions), this is a Message type name generated by getProfileTypeName.
  private final ImmutableMap<String, String> knownTypedExtensions;

  private final String packageName;
  private final String protoRootPath;

  public ProtoGenerator(
      String packageName, String protoRootPath, List<StructureDefinition> typedExtensions) {
    this.packageName = packageName;
    this.protoRootPath = protoRootPath;

    Map<String, String> mutableExtensionMap = new HashMap<>();
    for (StructureDefinition typedExtensionDef : typedExtensions) {
      String url = typedExtensionDef.getUrl().getValue();
      if (url.isEmpty()) {
        throw new IllegalArgumentException(
            "Invalid FHIR extension: "
                + typedExtensionDef.getId().getValue()
                + " has no url specified by the fhir_extension_url option");
      }
      String simpleExtensionType = getSimpleExtensionType(typedExtensionDef);
      String messageName =
          simpleExtensionType != null ? simpleExtensionType : getProfileTypeName(typedExtensionDef);
      mutableExtensionMap.put(url, messageName);
    }
    this.knownTypedExtensions = ImmutableMap.copyOf(mutableExtensionMap);
  }

  public ProtoGenerator(String packageName, String protoRootPath) {
    this.packageName = packageName;
    this.protoRootPath = protoRootPath;
    this.knownTypedExtensions = ImmutableMap.of();
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
    renamedCodeTypes.put("DayOfWeekCode", "DaysOfWeekCode");
    renamedCodeTypes.put("DetectedIssueStatusCode", "ObservationStatusCode");
    renamedCodeTypes.put("DeviceRequestStatusCode", "RequestStatusCode");
    renamedCodeTypes.put("DocumentConfidentialityCode", "ConfidentialityClassificationCode");
    renamedCodeTypes.put("EligibilityRequestStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("EligibilityResponseStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("EnrollmentRequestStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("EnrollmentResponseStatusCode", "FinancialResourceStatusCode");
    renamedCodeTypes.put("ImmunizationStatusCode", "ImmunizationStatusCodesCode");
    renamedCodeTypes.put("ParameterStatusCode", "OperationParameterStatusCode");
    renamedCodeTypes.put("ParameterUseCode", "OperationParameterUseCode");
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

  // Given a structure definition, gets the name of the top-level message that will be generated.
  public static String getTypeName(StructureDefinition def) {
    return isTypedExtensionDefinition(def)
        ? getProfileTypeName(def)
        : normalizeType(def.getId().getValue());
  }

  /**
   * Generate a proto descriptor from a StructureDefinition, using the snapshot form of the
   * definition. For a more elaborate discussion of these versions, see
   * https://www.hl7.org/fhir/structuredefinition.html.
   */
  public DescriptorProto generateProto(StructureDefinition def) {
    def = reconcileSnapshotAndDifferential(def);
    List<ElementDefinition> elementList = def.getSnapshot().getElementList();
    ElementDefinition root = elementList.get(0);

    boolean isPrimitive =
        def.getKind().getValue() == StructureDefinitionKindCode.Value.PRIMITIVE_TYPE;

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
    comment.append("\nSee ").append(def.getUrl().getValue());

    // Add message-level annotations.
    DescriptorProto.Builder builder = DescriptorProto.newBuilder();
    builder.setOptions(
        MessageOptions.newBuilder()
            .setExtension(
                Annotations.structureDefinitionKind,
                Annotations.StructureDefinitionKindValue.valueOf(
                    "KIND_" + def.getKind().getValue()))
            .setExtension(Annotations.messageDescription, comment.toString())
            .setExtension(Annotations.fhirStructureDefinitionUrl, def.getUrl().getValue())
            .build());

    // If this is a primitive type, generate the value field first.
    if (isPrimitive) {
      generatePrimitiveValue(def, builder);
    }

    builder = generateMessage(root, elementList, builder).toBuilder();

    if (isProfile(def)) {
      String name = getTypeName(def);
      // This is a profile on a pre-existing type.
      // Make sure any nested subtypes use the profile name, not the base name
      replaceType(builder, def.getType().getValue(), name);
      builder
          .setName(name)
          .getOptionsBuilder()
          .setExtension(Annotations.fhirProfileBase, def.getType().getValue());
    }
    return builder.build();
  }

  private static boolean isProfile(StructureDefinition def) {
    return def.getDerivation().getValue() == TypeDerivationRuleCode.Value.CONSTRAINT;
  }

  /** Generate a .proto file descriptor from a list of StructureDefinitions. */
  public FileDescriptorProto generateFileDescriptor(List<StructureDefinition> defs) {
    FileDescriptorProto.Builder builder = FileDescriptorProto.newBuilder();
    builder.setPackage(packageName).setSyntax("proto3");
    FileOptions.Builder options = FileOptions.newBuilder();
    options.setJavaPackage("com." + packageName).setJavaMultipleFiles(true);
    builder.setOptions(options);
    boolean hasPrimitiveType = false;
    boolean hasCodeType = false;
    for (StructureDefinition def : defs) {
      DescriptorProto proto = generateProto(def);
      if (AnnotationUtils.isPrimitiveType(proto)) {
        hasPrimitiveType = true;
      }
      if (usesCodeType(proto)) {
        hasCodeType = true;
      }
      builder.addMessageType(proto);
    }
    // Add imports. Annotations is always needed; datatypes is needed unless we are building them.
    builder.addDependency(new File(protoRootPath, "annotations.proto").toString());
    if (hasCodeType && !hasPrimitiveType) {
      builder.addDependency(new File(protoRootPath, "codes.proto").toString());
    }
    if (!hasPrimitiveType) {
      builder.addDependency(new File(protoRootPath, "datatypes.proto").toString());
    }
    return builder.build();
  }

  private boolean usesCodeType(DescriptorProto proto) {
    for (FieldDescriptorProto field : proto.getFieldList()) {
      if (field.getType() == FieldDescriptorProto.Type.TYPE_MESSAGE
          && field.getTypeName().endsWith("Code")) {
        return true;
      }
    }
    for (DescriptorProto nested : proto.getNestedTypeList()) {
      if (usesCodeType(nested)) {
        return true;
      }
    }
    return false;
  }

  // Does a global find-and-replace of a given token in a message namespace.
  // This is necessary for profiles, since the StructureDefinition uses the base Resource name
  // throughout, but we wish to use the Profiled resource name.
  // So, for instance, for a profile MyProfiledResource on MyResource, this would be used to turn
  // some.package.MyResource.Submessage into some.package.MyProfiledResource.Submessage
  private void replaceType(DescriptorProto.Builder protoBuilder, String from, String to) {
    String fromWithDots = "." + from + ".";
    String toWithDots = "." + to + ".";
    for (FieldDescriptorProto.Builder field : protoBuilder.getFieldBuilderList()) {
      if (field.getTypeName().contains(fromWithDots)) {
        field.setTypeName(field.getTypeName().replaceFirst(fromWithDots, toWithDots));
      }
    }
    for (DescriptorProto.Builder nested : protoBuilder.getNestedTypeBuilderList()) {
      replaceType(nested, from, to);
    }
  }

  /**
   * Returns a version of the passed-in FileDescriptor that contains a ContainedResource message,
   * which contains only the resource types present in the generated FileDescriptorProto.
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

  private DescriptorProto generateMessage(
      ElementDefinition currentElement,
      List<ElementDefinition> elementList,
      DescriptorProto.Builder builder) {
    // Get the name of this message.
    List<String> messageNameParts =
        Splitter.on('.').splitToList(getContainerType(currentElement, elementList));
    String messageName = messageNameParts.get(messageNameParts.size() - 1);
    builder.setName(messageName);

    // When generating a descriptor for a primitive type, the value part may already be present.
    int nextTag = builder.getFieldCount() + 1;

    // Some repeated fields can have profiled elements in them, that get inlined as fields.
    // The most common case of this is typed extensions.  We defer adding these to the end of the
    // message, so that non-profiled messages will be binary compatiple with this proto.
    // Note that the inverse is not true - loading a profiled message bytes into the non-profiled
    // will result in the data in the typed fields being dropped.
    List<ElementDefinition> deferredElements = new ArrayList<>();

    // Loop over the elements.
    for (ElementDefinition element : getFieldsToPrint(elementList, currentElement)) {
      // If this is a hardcoded extension url, it should get inlined instead of being treated like a
      // field.
      // TODO(nickgeorge): generalize this beyond extensions and urls
      if ("Extension.url".equals(element.getId().getValue()) && element.getFixed().hasUri()) {
        builder.setOptions(
            builder
                .getOptions()
                .toBuilder()
                .setExtension(
                    Annotations.fhirExtensionUrl, element.getFixed().getUri().getValue()));
        continue;
      }

      // If this is a container type, define the inner message.
      if (isContainer(element)) {
        builder.addNestedType(generateMessage(element, elementList, DescriptorProto.newBuilder()));
      }

      if (!isChoiceType(element) && !isSingleType(element)) {
        throw new IllegalArgumentException(
            "Illegal field has multiple types but is not a Choice Type:\n" + element);
      }

      if (isTypedExtension(element)) {
        // Defer this field until the end of the message.
        deferredElements.add(element);
      } else {
        buildAndAddField(element, elementList, nextTag++, builder);
      }
    }

    for (ElementDefinition deferredElement : deferredElements) {
      buildAndAddField(deferredElement, elementList, nextTag++, builder);
    }

    return builder.build();
  }

  private void buildAndAddField(
      ElementDefinition element,
      List<ElementDefinition> elementList,
      int tag,
      DescriptorProto.Builder builder) {
    // Generate the field. If this field doesn't actually exist in this version of the
    // message, for example, the max attribute is 0, buildField returns null and no field
    // should be added.
    FieldDescriptorProto field = buildField(element, elementList, tag);
    if (field != null) {
      builder.addField(field);
      if (isChoiceType(element)) {
        addChoiceType(element, field, builder);
      }
    }
  }

  private static List<ElementDefinition> getFieldsToPrint(
      List<ElementDefinition> allElements, ElementDefinition parentElement) {
    List<String> messagePathParts =
        Splitter.on('.').splitToList(parentElement.getPath().getValue());
    return allElements
        .stream()
        .filter(
            element -> {
              if (element.getTypeCount() == 1
                  && element.getType(0).getCode().getValue().isEmpty()) {
                // This is a primitive value field.  Ignore it, as it should be already handled by
                // generatePrimitiveValue.
                return false;
              }
              List<String> parts = Splitter.on('.').splitToList(element.getPath().getValue());
              if (!element.getPath().getValue().startsWith(parentElement.getPath().getValue() + ".")
                  || parts.size() != messagePathParts.size() + 1) {
                // This field doesn't "live" on the parent message. It's irrelevant here.
                return false;
              }
              return true;
            })
        .collect(Collectors.toList());
  }

  private static final ElementDefinition.TypeRef STRING_TYPE =
      ElementDefinition.TypeRef.newBuilder().setCode(Uri.newBuilder().setValue("string")).build();

  /** Generate the primitive value part of a datatype. */
  private void generatePrimitiveValue(StructureDefinition def, DescriptorProto.Builder builder) {
    String valueFieldId = def.getId().getValue() + ".value";
    // Find the value field.
    ElementDefinition valueElement;
    try {
      valueElement =
          getElementDefinitionById(def.getSnapshot().getElementList(), valueFieldId)
              .orElseThrow(
                  () -> new IllegalArgumentException("Unable to find value field on: " + def));
    } catch (IllegalArgumentException e) {
      throw new IllegalArgumentException(
          "Multiple Elements found on StructureDefinition with id: "
              + valueFieldId
              + ".\nStructureDefinition: "
              + def);
    }
    // If present, add the regex for this primitive type as a message-level annotation.
    if (valueElement.getTypeCount() == 1) {
      List<ElementDefinitionRegex> regex =
          ExtensionWrapper.fromExtensionsIn(valueElement.getType(0))
              .getMatchingExtensions(ElementDefinitionRegex.getDefaultInstance());
      if (regex.size() == 1) {
        builder.setOptions(
            builder
                .getOptions()
                .toBuilder()
                .setExtension(Annotations.valueRegex, regex.get(0).getValueString().getValue())
                .build());
      }
    }

    // The value field sometimes has no type. We need to add a fake one here for buildField
    // to succeed.  This will get overridden with the correct time,
    ElementDefinition elementWithType =
        valueElement.toBuilder().clearType().addType(STRING_TYPE).build();
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
                .setTypeName(
                    "." + packageName + "." + normalizeType(def.getId().getValue()) + ".Precision")
                .setNumber(builder.getFieldCount() + 1));
      } else {
        // Handle non-time-like types.
        // If they don't explicitly appear in the PRIMITIVE_TYPE_OVERRIDES, they are assumed
        // to be of type TYPE_STRING.
        FieldDescriptorProto.Type primitiveType =
            PRIMITIVE_TYPE_OVERRIDES.getOrDefault(
                def.getId().getValue(), FieldDescriptorProto.Type.TYPE_STRING);
        builder.addField(field.toBuilder().clearTypeName().setType(primitiveType));
      }
    }
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

  /** Returns true if this ElementDefinition is an extension with a profile. */
  private static boolean isTypedExtension(ElementDefinition element) {
    return element.getTypeCount() == 1
        && element.getType(0).getCode().getValue().equals("Extension")
        && element.getType(0).hasProfile();
  }

  private static boolean isTypedExtensionDefinition(StructureDefinition def) {
    return def.getType().getValue().equals("Extension")
        && def.getDerivation().getValue() == TypeDerivationRuleCode.Value.CONSTRAINT;
  }

  private static boolean isChoiceType(ElementDefinition element) {
    return element.getPath().getValue().endsWith("[x]");
  }

  /** Extract the type of a container field, possibly by reference. */
  private static String getContainerType(
      ElementDefinition element, List<ElementDefinition> elementList) {
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
          elementList
              .stream()
              .filter(e -> e.getPath().getValue().equals(path))
              .findFirst()
              .orElseThrow(
                  () ->
                      new IllegalArgumentException(
                          "No elements with path: " + path + "on element: " + element.getId()));
      List<ElementDefinitionExplicitTypeName> typeNames =
          ExtensionWrapper.fromExtensionsIn(pathElement)
              .getMatchingExtensions(ElementDefinitionExplicitTypeName.getDefaultInstance());
      String typeName;
      if (!typeNames.isEmpty()) {
        typeName = typeNames.get(0).getValueString().getValue();
      } else {
        typeName =
            normalizeType(
                part.endsWith("[x]") ? part.substring(0, part.length() - "[x]".length()) : part);
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

  /**
   * Gets the field type of a potentially complex element. This handles choice types, types that
   * reference other elements, references, profiles, etc.
   */
  private String getFieldType(ElementDefinition element, List<ElementDefinition> elementList) {
    if (isContainer(element) || element.hasContentReference() || isChoiceType(element)) {
      // Get the type for this container, possibly from a named reference to another element.
      return getContainerType(element, elementList);
    } else if (element.getType(0).getCode().getValue().equals("Reference")) {
      return USE_TYPED_REFERENCES ? getTypedReferenceName(element.getTypeList()) : "Reference";
    } else {
      if (element.getTypeCount() > 1) {
        throw new IllegalArgumentException(
            "Unknown multiple type definition on element: " + element.getId());
      }
      // Note: this is the "fhir type", e.g., Resource, BackboneElement, boolean,
      // not the field type name.
      ElementDefinition.TypeRef fhirType = Iterables.getOnlyElement(element.getTypeList());

      if (fhirType.getCode().getValue().equals("Resource")) {
        // We represent "Resource" FHIR types as "ContainedResource"
        return "ContainedResource";
      }

      String normalizedFhirTypeName = normalizeType(fhirType.getCode().getValue());
      if (normalizedFhirTypeName.equals("Code")) {
        // If this is a code, check for a binding name and handle it here.
        String bindingName = getBindingName(element);

        if (bindingName != null) {
          String typeWithBindingName = bindingName + "Code";
          return RENAMED_CODE_TYPES.getOrDefault(typeWithBindingName, typeWithBindingName);
        }
      }
      return normalizedFhirTypeName;
    }
  }

  private String getBindingName(ElementDefinition element) {
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
    return bindingName;
  }

  /** Build a single field for the proto. */
  private FieldDescriptorProto buildField(
      ElementDefinition element, List<ElementDefinition> elementList, int nextTag) {
    FieldOptions.Builder options = FieldOptions.newBuilder();

    // Add a short description of the field.
    if (element.hasShort()) {
      options.setExtension(Annotations.fieldDescription, element.getShort().getValue());
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

    if (isTypedExtension(element)) {
      // This is an extension with a type defined by a profile.
      // If we know about it, we'll inline a field for it.
      // Otherwise, we'll return null here, which means it'll get ignored
      String profileUrl = element.getType(0).getProfile().getValue();
      if (!knownTypedExtensions.containsKey(profileUrl)) {
        // We don't know about this extension.
        // Return null, indicating that we shouldn't generate a proto field for this extension.
        return null;
      }

      String fieldTypeString = knownTypedExtensions.get(profileUrl);
      String fieldName = element.getSliceName().getValue();
      options.setExtension(
          Annotations.fieldDescription,
          options.getExtension(Annotations.fieldDescription)
              + "\nThis is a FHIR extension that has been inlined based on the profile for this "
              + "resource.");
      options.setExtension(Annotations.fhirInlinedExtensionUrl, profileUrl);
      return buildFieldInternal(fieldName, fieldTypeString, nextTag, repeated, options.build())
          .build();
    }

    // Extract the name of the field.
    String fieldName =
        element.getPath().getValue().substring(element.getPath().getValue().lastIndexOf('.') + 1);
    // Is this field a choice type?
    if (isChoiceType(element)) {
      fieldName = fieldName.substring(0, fieldName.length() - "[x]".length());
      options.setExtension(Annotations.isChoiceType, true);
    }

    String fieldTypeString = getFieldType(element, elementList);

    return buildFieldInternal(fieldName, fieldTypeString, nextTag, repeated, options.build())
        .build();
  }

  /** Add a choice type field to the proto. */
  private void addChoiceType(
      ElementDefinition element, FieldDescriptorProto field, DescriptorProto.Builder builder) {

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
    List<ElementDefinition.TypeRef> types = new ArrayList<>();
    Set<String> foundTypes = new HashSet<>();
    for (ElementDefinition.TypeRef type : element.getTypeList()) {
      if (!foundTypes.contains(type.getCode().getValue())) {
        types.add(type);
        foundTypes.add(type.getCode().getValue());
      }
    }

    for (ElementDefinition.TypeRef t : types) {
      String fieldType = normalizeType(t.getCode().getValue());
      String fieldName = t.getCode().getValue();
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

  private FieldDescriptorProto.Builder buildFieldInternal(
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
      fieldTypeParts.add(normalizeType(part));
    }
    builder.setTypeName("." + packageName + "." + Joiner.on('.').join(fieldTypeParts));

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

  // Normalizes the casing of a type string.
  // E.g., FHIR types primitives with all lower-case, but we use Title case for our primitive
  // messages (e.g., boolean -> Boolean)
  private static String normalizeType(String type) {
    String normalizedType = type;
    if (Character.isLowerCase(type.charAt(0))) {
      normalizedType = CaseFormat.LOWER_CAMEL.to(CaseFormat.UPPER_CAMEL, type);
    }
    if (type.contains("-")) {
      normalizedType = CaseFormat.LOWER_HYPHEN.to(CaseFormat.UPPER_CAMEL, type);
    }
    // We don't support Xhtml yet - remove this special case once we do.
    if (type.equals("Xhtml")) {
      return "String";
    }
    return normalizedType;
  }

  // Returns an optional containing the only element in the list matching a given id, if present.
  // Throws IllegalArgumentException if multiple matching elements are found.
  private static Optional<ElementDefinition> getElementDefinitionById(
      List<ElementDefinition> elements, String id) {
    return elements
        .stream()
        .filter(element -> element.getId().getValue().equals(id))
        .collect(MoreCollectors.toOptional());
  }

  private static String getTypedReferenceName(List<ElementDefinition.TypeRef> typeList) {
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
      return refType + "Reference";
    }
    return "Reference";
  }

  // If an extension consists of a single primitive type, returns that type as a string.
  // Otherwise, returns null.
  private static String getSimpleExtensionType(StructureDefinition def) {
    if (!isTypedExtensionDefinition(def)) {
      return null;
    }
    for (ElementDefinition element : def.getSnapshot().getElementList()) {
      if (element.getBase().getPath().getValue().equals("Extension.value[x]")) {
        if (element.getTypeCount() != 1) {
          return null;
        }
        return normalizeType(element.getType(0).getCode().getValue());
      }
    }
    return null;
  }

  public static boolean isSimpleExtension(StructureDefinition def) {
    return getSimpleExtensionType(def) != null;
  }

  /**
   * Derives a message name for the typed extension. Uses the name field from the
   * StructureDefinition. If the context field indicates a single Element type the extension should
   * apply to, uses that as a prefix.
   */
  private static String getProfileTypeName(StructureDefinition def) {
    String name = normalizeType(def.getName().getValue());
    Set<String> contexts = new HashSet<>();
    Splitter splitter = Splitter.on('.').limit(2);
    for (com.google.fhir.stu3.proto.String context : def.getContextList()) {
      // Only interest in the top-level resource.
      contexts.add(splitter.splitToList(context.getValue()).get(0));
    }
    if (contexts.size() == 1) {
      name = Iterables.getOnlyElement(contexts) + name;
    }
    return name;
  }

  // We generate protos based on the "Snapshot" view of the proto, but these are often
  // generated off of the "Differential" view, which can be buggy.  So, before processing the
  // snapshot, do a pass over the differential and correct any inconsistencies in the snapshot.
  private static StructureDefinition reconcileSnapshotAndDifferential(StructureDefinition def) {
    // Generate a map from (element id) -> (element) for all elements in the Differential view.
    Map<String, ElementDefinition> diffs =
        def.getDifferential()
            .getElementList()
            .stream()
            .collect(
                Collectors.toMap((element) -> element.getId().getValue(), Function.identity()));

    StructureDefinition.Builder defBuilder = def.toBuilder();
    defBuilder.getSnapshotBuilder().clearElement();
    for (ElementDefinition element : def.getSnapshot().getElementList()) {
      ElementDefinition elementDiffs = diffs.get(element.getId().getValue());
      defBuilder
          .getSnapshotBuilder()
          .addElement(
              elementDiffs == null
                  ? element
                  : (ElementDefinition)
                      reconcileMessage(
                          element,
                          elementDiffs,
                          def.getId().getValue(),
                          element.getId().getValue(),
                          ""));
    }
    return defBuilder.build();
  }

  private static Message reconcileMessage(
      Message snapshot,
      Message differential,
      String structureDefinitionId,
      String elementId,
      String fieldpath) {
    Message.Builder reconciledElement = snapshot.toBuilder();
    for (FieldDescriptor field : reconciledElement.getDescriptorForType().getFields()) {
      String subFieldpath = (fieldpath.isEmpty() ? "" : (fieldpath + ".")) + field.getJsonName();
      if (!field.isRepeated()
          && differential.hasField(field)
          && !differential.getField(field).equals(reconciledElement.getField(field))) {
        if (AnnotationUtils.isPrimitiveType(field.getMessageType())) {
          reconciledElement.setField(field, differential.getField(field));
          System.out.println(
              "Warning: found inconsistent Snapshot for "
                  + structureDefinitionId
                  + ".  Field \""
                  + subFieldpath
                  + "\" on "
                  + elementId
                  + " has snapshot\n"
                  + snapshot.getField(field)
                  + "but differential\n"
                  + differential.getField(field)
                  + "\nUsing differential value for protogeneration.\n\n");
        } else {
          reconciledElement.setField(
              field,
              reconcileMessage(
                  (Message) snapshot.getField(field),
                  (Message) differential.getField(field),
                  structureDefinitionId,
                  elementId,
                  subFieldpath));
        }
      } else if (field.isRepeated()) {
        // For repeated fields, make sure each element in the differential appears in the snapshot.
        Set<Message> valuesInSnapshot = new HashSet<>();
        for (int i = 0; i < snapshot.getRepeatedFieldCount(field); i++) {
          valuesInSnapshot.add((Message) snapshot.getRepeatedField(field, i));
        }
        for (int i = 0; i < differential.getRepeatedFieldCount(field); i++) {
          Message differentialValue = (Message) differential.getRepeatedField(field, i);
          if (!valuesInSnapshot.contains(differentialValue)) {
            System.out.println(
                "Warning: found inconsistent Snapshot for "
                    + structureDefinitionId
                    + ".  Field \""
                    + subFieldpath
                    + "\" on "
                    + elementId
                    + " has value on differential that is missing from snapshot:\n"
                    + differentialValue
                    + "\nAdding in for use in protogeneration.\n\n");
            reconciledElement.addRepeatedField(field, differentialValue);
          }
        }
      }
    }
    return reconciledElement.build();
  }
}
