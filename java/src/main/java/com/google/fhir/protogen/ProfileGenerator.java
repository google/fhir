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

package com.google.fhir.protogen;

import com.google.common.base.Ascii;
import com.google.common.base.Splitter;
import com.google.common.collect.MoreCollectors;
import com.google.fhir.common.FhirVersion;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.ChoiceTypeRestriction;
import com.google.fhir.proto.CodeData;
import com.google.fhir.proto.CodeableConceptSlice;
import com.google.fhir.proto.CodeableConceptSlice.CodingSlice;
import com.google.fhir.proto.ComplexExtension;
import com.google.fhir.proto.ElementData;
import com.google.fhir.proto.ExtensionSlice;
import com.google.fhir.proto.Extensions;
import com.google.fhir.proto.FieldRestriction;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.proto.Profile;
import com.google.fhir.proto.Profiles;
import com.google.fhir.proto.ReferenceRestriction;
import com.google.fhir.proto.SimpleExtension;
import com.google.fhir.proto.SizeRestriction;
import com.google.fhir.r4.core.BindingStrengthCode;
import com.google.fhir.r4.core.Bundle;
import com.google.fhir.r4.core.Canonical;
import com.google.fhir.r4.core.Code;
import com.google.fhir.r4.core.CodeableConcept;
import com.google.fhir.r4.core.Coding;
import com.google.fhir.r4.core.ContactDetail;
import com.google.fhir.r4.core.ContactPoint;
import com.google.fhir.r4.core.ContactPointSystemCode;
import com.google.fhir.r4.core.ContainedResource;
import com.google.fhir.r4.core.DateTime;
import com.google.fhir.r4.core.DiscriminatorTypeCode;
import com.google.fhir.r4.core.ElementDefinition;
import com.google.fhir.r4.core.ElementDefinition.ElementDefinitionBinding;
import com.google.fhir.r4.core.ElementDefinitionOrBuilder;
import com.google.fhir.r4.core.Extension;
import com.google.fhir.r4.core.ExtensionContextTypeCode;
import com.google.fhir.r4.core.Id;
import com.google.fhir.r4.core.Markdown;
import com.google.fhir.r4.core.SlicingRulesCode;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.StructureDefinition.Differential;
import com.google.fhir.r4.core.StructureDefinition.Snapshot;
import com.google.fhir.r4.core.StructureDefinitionKindCode;
import com.google.fhir.r4.core.TypeDerivationRuleCode;
import com.google.fhir.r4.core.UnsignedInt;
import com.google.fhir.r4.core.Uri;
import com.google.fhir.wrappers.DateTimeWrapper;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Message;
import java.time.LocalDate;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

/**
 * JSON structure definition generator that consumes Profiles and Extensions proto definitions in
 * and generates one StructureDefinition per extension and profile.
 */
final class ProfileGenerator {
  private final PackageInfo packageInfo;
  private final Profiles profiles;
  private final Extensions extensions;
  private final Map<String, StructureDefinition> urlToStructDefMap;
  private final Map<String, StructureDefinition> idToBaseStructDefMap;
  private final DateTime creationDateTime;

  ProfileGenerator(
      PackageInfo packageInfo,
      Profiles profiles,
      Extensions extensions,
      List<StructureDefinition> knownTypesList,
      LocalDate creationTime) {
    this.packageInfo = packageInfo;
    this.profiles = profiles;
    this.extensions = extensions;
    this.urlToStructDefMap =
        knownTypesList.stream().collect(Collectors.toMap(def -> def.getUrl().getValue(), f -> f));
    this.idToBaseStructDefMap =
        knownTypesList.stream()
            .filter(
                def -> def.getDerivation().getValue() != TypeDerivationRuleCode.Value.CONSTRAINT)
            .collect(Collectors.toMap(def -> def.getId().getValue(), f -> f));
    this.creationDateTime = buildCreationDateTime(creationTime);
  }

  Bundle generateProfiles() {
    Bundle.Builder bundle = Bundle.newBuilder();
    for (Profile profile : profiles.getProfileList()) {
      StructureDefinition structDef = makeProfile(profile);
      bundle.addEntry(
          Bundle.Entry.newBuilder()
              .setResource(ContainedResource.newBuilder().setStructureDefinition(structDef)));
      urlToStructDefMap.put(structDef.getUrl().getValue(), structDef);
    }
    return bundle.build();
  }

  Bundle generateExtensions() {
    Bundle.Builder bundle = Bundle.newBuilder();
    for (SimpleExtension simpleExtension : extensions.getSimpleExtensionList()) {
      bundle.addEntry(
          Bundle.Entry.newBuilder()
              .setResource(
                  ContainedResource.newBuilder()
                      .setStructureDefinition(makeSimpleExtension(simpleExtension))));
    }
    for (ComplexExtension complexExtension : extensions.getComplexExtensionList()) {
      bundle.addEntry(
          Bundle.Entry.newBuilder()
              .setResource(
                  ContainedResource.newBuilder()
                      .setStructureDefinition(makeComplexExtension(complexExtension))));
    }
    return bundle.build();
  }

  private StructureDefinition makeProfile(Profile profile) {
    ElementData elementData = profile.getElementData();
    StructureDefinition baseStructDef = getStructDefForUrl(profile.getBaseUrl());

    StructureDefinition.Builder structDefBuilder = StructureDefinition.newBuilder();
    setMetadata(
        structDefBuilder,
        baseStructDef.getSnapshot().getElement(0).getPath().getValue(),
        profile.getBaseUrl(),
        elementData,
        packageInfo,
        StructureDefinitionKindCode.Value.RESOURCE);

    List<ElementDefinition> baseElements = baseStructDef.getSnapshot().getElementList();
    List<ElementDefinition> elementList =
        baseElements.stream()
            .map(element -> element.toBuilder().build())
            .collect(Collectors.toList());

    profile
        .getRestrictionList()
        .forEach(restriction -> applyFieldRestriction(restriction, elementList, elementData));

    profile
        .getExtensionSliceList()
        .forEach(slice -> elementList.add(buildExtensionSliceElement(slice, elementList)));

    for (CodeableConceptSlice codeableConceptSlice : profile.getCodeableConceptSliceList()) {
      // Make sure there is a valid CodeableConcept field to slice.
      String codeableConceptFieldId = codeableConceptSlice.getFieldId();
      getOptionalElementById(codeableConceptFieldId, elementList)
          .orElseThrow(
              () ->
                  new IllegalArgumentException(
                      "Unable to locate CodeableConcept field "
                          + codeableConceptFieldId
                          + " for profile "
                          + elementData.getName()));
      // The slicing directive appears on the Coding element within CodeableConcept.
      // However, since the coding is a subfield of CodeableConcept, it does not appear on the base
      // element list.
      // E.g., Observation structure profile has an ElementDefinition for
      // Observation.code of type CodeableConcept, but not Observation.code.coding of type Coding,
      // since that is implied by the CodeableConcept.
      // Thus, in order to modify the coding, we must first copy over the coding element from the
      // structure definition for CodeableConcept, so that we can add slicing information.
      ElementDefinition.Builder codingElement = getCodeableConceptCodingElement().toBuilder();
      String codingFieldId = codeableConceptSlice.getFieldId() + ".coding";
      setIdAndPath(codingElement, codingFieldId, codingFieldId, "");
      codingElement.setBase(
          buildBase("CodeableConcept.coding", getStructDef(CodeableConcept.getDescriptor())));
      codingElement
          .getSlicingBuilder()
          .setOrdered(fhirBoolean(false))
          .setRules(
              ElementDefinition.Slicing.RulesCode.newBuilder()
                  .setValue(
                      codeableConceptSlice.getRules()
                              == SlicingRulesCode.Value.INVALID_UNINITIALIZED
                          ? SlicingRulesCode.Value.CLOSED
                          : codeableConceptSlice.getRules()))
          .addDiscriminatorBuilder()
          .setPath(fhirString("code"))
          .getTypeBuilder()
          .setValue(DiscriminatorTypeCode.Value.VALUE);
      elementList.add(codingElement.build());
      for (CodingSlice codingSlice : codeableConceptSlice.getCodingSliceList()) {
        elementList.addAll(buildCodeableConceptSliceElements(codingFieldId, codingSlice));
      }
    }

    elementList.forEach(element -> structDefBuilder.getSnapshotBuilder().addElement(element));
    addDifferentialElements(structDefBuilder, baseElements);

    return structDefBuilder.build();
  }

  private StructureDefinition makeComplexExtension(ComplexExtension extensionProto) {
    ElementData elementData = extensionProto.getElementData();

    StructureDefinition.Builder structDefBuilder = StructureDefinition.newBuilder();
    setMetadata(
        structDefBuilder,
        "Extension",
        getExtensionStructDef().getUrl().getValue(),
        elementData,
        packageInfo,
        StructureDefinitionKindCode.Value.COMPLEX_TYPE);

    structDefBuilder
        .getSnapshotBuilder()
        .addAllElement(
            buildComplexExtensionElements(
                extensionProto, "Extension", "Extension", getStructureDefinitionUrl(elementData)));

    addDifferentialElements(
        structDefBuilder, getExtensionStructDef().getSnapshot().getElementList());

    return structDefBuilder.build();
  }

  private List<ElementDefinition> buildComplexExtensionElements(
      ComplexExtension extensionProto, String rootId, String rootPath, String url) {
    List<ElementDefinition> complexElements = new ArrayList<>();
    complexElements.addAll(buildBackboneExtensionElements(extensionProto, rootId, rootPath, url));
    extensionProto.getElementData();
    for (SimpleExtension field : extensionProto.getSimpleFieldList()) {
      String name = field.getElementData().getName();
      complexElements.addAll(
          buildSimpleExtensionElements(
              field, rootId + ".extension:" + name, rootPath + ".extension", name));
    }
    for (ComplexExtension field : extensionProto.getComplexFieldList()) {
      String name = field.getElementData().getName();
      complexElements.addAll(
          buildComplexExtensionElements(
              field, rootId + ".extension:" + name, rootPath + ".extension", name));
    }
    return complexElements;
  }

  private List<ElementDefinition> buildBackboneExtensionElements(
      ComplexExtension complexExtension, String rootId, String rootPath, String url) {
    SimpleExtension simpleExtension =
        SimpleExtension.newBuilder()
            .setElementData(complexExtension.getElementData())
            .setCanHaveExtensions(complexExtension.getCanHaveAdditionalExtensions())
            .build();
    List<ElementDefinition> backboneElements =
        buildSimpleExtensionElements(simpleExtension, rootId, rootPath, url);
    return backboneElements;
  }

  private StructureDefinition makeSimpleExtension(SimpleExtension extensionProto) {
    ElementData elementData = extensionProto.getElementData();

    StructureDefinition.Builder structDefBuilder = StructureDefinition.newBuilder();
    setMetadata(
        structDefBuilder,
        "Extension",
        getExtensionStructDef().getUrl().getValue(),
        elementData,
        packageInfo,
        StructureDefinitionKindCode.Value.COMPLEX_TYPE);

    structDefBuilder
        .getSnapshotBuilder()
        .addAllElement(
            buildSimpleExtensionElements(
                extensionProto, "Extension", "Extension", getStructureDefinitionUrl(elementData)));
    addDifferentialElements(
        structDefBuilder, getExtensionStructDef().getSnapshot().getElementList());

    return structDefBuilder.build();
  }

  private List<ElementDefinition> buildSimpleExtensionElements(
      SimpleExtension extensionProto, String rootId, String rootPath, String url) {
    StructureDefinition extensionBaseStructDef = getExtensionStructDef();
    List<ElementDefinition> extensionBaseElements =
        extensionBaseStructDef.getSnapshot().getElementList();
    List<ElementDefinition.Builder> elementList =
        new ArrayList<>(
            Snapshot.newBuilder().addAllElement(extensionBaseElements).getElementBuilderList());
    ElementData elementData = extensionProto.getElementData();

    // Root Element
    ElementDefinition.Builder rootElement = getElementBuilderById("Extension", elementList);
    customizeExtensionRootElement(rootElement, rootId, elementData);

    // URL Element
    ElementDefinition.Builder urlElement = getElementBuilderById("Extension.url", elementList);
    urlElement.getFixedBuilder().setUri(fhirUri(url));

    // Extension Element
    ElementDefinition.Builder extensionElement =
        getElementBuilderById("Extension.extension", elementList);
    if (!extensionProto.getCanHaveExtensions()) {
      extensionElement.getMaxBuilder().setValue("0");
    }

    // Value Element
    ElementDefinition.Builder valueElement =
        getElementBuilderById("Extension.value[x]", elementList);
    valueElement.setBase(buildBase("Extension.value[x]", extensionBaseStructDef));
    if (extensionProto.getTypeCount() > 0 && extensionProto.hasCodeType()) {
      throw new IllegalArgumentException(
          "SimpleExtension must have either a type or code_type set, but not both: "
              + extensionProto.getElementData().getName());
    }
    if (extensionProto.getTypeCount() == 0 && !extensionProto.hasCodeType()) {
      valueElement.getMaxBuilder().setValue("0");
      setIdAndPath(valueElement, rootId, rootPath, ".value[x]");
    } else {
      valueElement.clearType();
      if (extensionProto.hasCodeType()) {
        valueElement.addType(ElementDefinition.TypeRef.newBuilder().setCode(fhirUri("code")));
        setIdAndPath(valueElement, rootId, rootPath, ".valueCode");
        valueElement.setBinding(buildCodeBinding(extensionProto.getCodeType()));
      } else {
        for (String type : extensionProto.getTypeList()) {
          valueElement.addType(ElementDefinition.TypeRef.newBuilder().setCode(fhirUri(type)));
        }
        String valueTail =
            extensionProto.getTypeCount() > 1
                ? ".value[x]"
                : ".value"
                    + (Ascii.toUpperCase(extensionProto.getType(0).substring(0, 1))
                        + extensionProto.getType(0).substring(1));
        setIdAndPath(valueElement, rootId, rootPath, valueTail);
      }
    }

    // Update Id, Path, and Base for all elements.
    for (ElementDefinition.Builder element : elementList) {
      if (element == valueElement) {
        // The value element gets its path/id/base set manually above.
        continue;
      }

      String originalId = element.getId().getValue();
      int lastDotIndex = originalId.lastIndexOf(".");
      String lastIdToken =
          lastDotIndex == -1 ? "" : originalId.substring(originalId.lastIndexOf("."));

      element.setBase(buildBase(originalId, extensionBaseStructDef));
      setIdAndPath(element, rootId, rootPath, lastIdToken);
    }

    if (isSliceId(rootId)) {
      rootElement.getBaseBuilder().getPathBuilder().setValue("Extension.extension");
    }

    return elementList.stream().map(ElementDefinition.Builder::build).collect(Collectors.toList());
  }

  private void customizeRootElement(
      ElementDefinition.Builder rootElement, ElementData elementData) {
    if (!elementData.getDescription().isEmpty()) {
      rootElement.setDefinition(Markdown.newBuilder().setValue(elementData.getDescription()));
    }
    rootElement.setMin(minSize(elementData.getSizeRestriction()));
    rootElement.setMax(maxSize(elementData.getSizeRestriction()));
    if (!elementData.getShort().isEmpty() || !elementData.getDescription().isEmpty()) {
      rootElement
          .getShortBuilder()
          .setValue(
              elementData.getShort().isEmpty()
                  ? elementData.getDescription()
                  : elementData.getShort());
    }

    if (!elementData.getComment().isEmpty()) {
      rootElement.getCommentBuilder().setValue(elementData.getComment());
    }
  }

  private void customizeExtensionRootElement(
      ElementDefinition.Builder rootElement, String rootId, ElementData elementData) {
    customizeRootElement(rootElement, elementData);
    for (ElementDefinition.Constraint.Builder constraint : rootElement.getConstraintBuilderList()) {
      if (constraint.getKey().getValue().startsWith("ext")) {
        constraint.getSourceBuilder().setValue("Extension");
      }
    }

    // If this is a slice (in other words, not a top-level extension), type it.
    if (isSliceId(rootId)) {
      rootElement.addType(ElementDefinition.TypeRef.newBuilder().setCode(fhirUri("Extension")));
      rootElement.setSliceName(fhirString(rootId.substring(rootId.lastIndexOf(":") + 1)));
      rootElement.clearMapping();
    }
  }

  @SuppressWarnings("unchecked")
  private void addDifferentialElements(
      StructureDefinition.Builder structDefBuilder, List<ElementDefinition> baseElements) {
    List<ElementDefinition> newElements = structDefBuilder.getSnapshot().getElementList();
    Differential.Builder differentialBuilder = Differential.newBuilder();
    Descriptor elementDescriptor = ElementDefinition.getDescriptor();
    List<FieldDescriptor> elementFields = elementDescriptor.getFields();
    FieldDescriptor idField = elementDescriptor.findFieldByName("id");
    FieldDescriptor pathField = elementDescriptor.findFieldByName("path");
    FieldDescriptor baseField = elementDescriptor.findFieldByName("base");

    for (ElementDefinition newElement : newElements) {
      ElementDefinition baseElement = getBaseElement(newElement, baseElements);
      boolean foundDiff = false;
      ElementDefinition.Builder diffElement = ElementDefinition.newBuilder();
      for (FieldDescriptor field : elementFields) {
        if (field.getName().equals(baseField.getName())) {
          // "base" is not included in differential, as it is metadata about difference.
          continue;
        }
        if (isSliceId(newElement.getId().getValue()) && field.getName().equals("slicing")) {
          // Information on how to slice a field is never included in the slice itself.
          continue;
        }
        if (field.equals(idField) || field.equals(pathField)) {
          diffElement.setField(field, newElement.getField(field));
        }
        if (field.isRepeated()) {
          if (field.getType() != FieldDescriptor.Type.MESSAGE) {
            throw new IllegalArgumentException("Encountered unexpected primitive field:"
                                                   + field.getFullName());
          }
          List<Message> newValues = (List<Message>) newElement.getField(field);
          List<Message> baseValues = (List<Message>) baseElement.getField(field);
          if (newValues.size() != baseValues.size()) {
            // The lists are not 1-1 matches.  So, the whole list counts as a diff.
            diffElement.setField(field, newValues);
            foundDiff = true;
          } else {
            for (int i = 0; i < baseValues.size(); i++) {
              if (!newValues.get(i).equals(baseValues.get(i))) {
                // TODO: do we need finer grain than element field diffing?
                diffElement.addRepeatedField(field, newValues.get(i));
                foundDiff = true;
              }
            }
          }
        } else {
          if (newElement.hasField(field) && !baseElement.hasField(field)) {
            diffElement.setField(field, newElement.getField(field));
            foundDiff = true;
          }
          Message newValue = (Message) newElement.getField(field);
          Message baseValue = (Message) baseElement.getField(field);
          if (!newValue.equals(baseValue)) {
            // TODO: There's currently a bug where the differential emits empty markdown
            // for removed fields, and empty markdown is invalid.
            if (newValue instanceof Markdown && ((Markdown) newValue).getValue().isEmpty()) {
              continue;
            }
            // TODO: do we need finer grain than element field diffing?
            diffElement.setField(field, newValue);
            foundDiff = true;
          }
        }
      }
      if (foundDiff) {
        differentialBuilder.addElement(diffElement);
      }
    }
    if (differentialBuilder.getElementCount() > 0) {
      structDefBuilder.setDifferential(differentialBuilder);
    }
  }

  private void applyFieldRestriction(
      FieldRestriction restriction, List<ElementDefinition> elementList, ElementData elementData) {
    ElementDefinition elementToModify =
        getOptionalElementById(restriction.getFieldId(), elementList)
            .orElseThrow(
                () ->
                    new IllegalArgumentException(
                        "Error Generating profile "
                            + elementData.getName()
                            + ": No base element with id "
                            + restriction.getFieldId()));
    ElementDefinition.Builder modifiedElement = elementToModify.toBuilder();
    SizeRestriction newSize = restriction.getSizeRestriction();
    if (newSize != SizeRestriction.UNKNOWN) {
      if (!validateSizeChange(restriction.getSizeRestriction(), elementToModify)) {
        throw new IllegalArgumentException(
            "Invalid size change for "
                + elementData.getName()
                + " to "
                + newSize
                + ". Original Element:\n"
                + elementToModify);
      }
      modifiedElement.setMin(minSize(newSize)).setMax(maxSize(newSize));
    }
    if (restriction.hasReferenceRestriction() || restriction.hasChoiceTypeRestriction()) {
      List<ElementDefinition.TypeRef> newTypes =
          applyChoiceTypeRestriction(
              elementToModify.getTypeList(),
              restriction.getChoiceTypeRestriction(),
              elementToModify.getId().getValue());
      newTypes =
          applyReferenceRestriction(
              newTypes, restriction.getReferenceRestriction(), elementToModify.getId().getValue());
      modifiedElement.clearType().addAllType(newTypes);
    }
    elementList.set(elementList.indexOf(elementToModify), modifiedElement.build());
  }

  private List<ElementDefinition.TypeRef> applyChoiceTypeRestriction(
      List<ElementDefinition.TypeRef> originalTypes,
      ChoiceTypeRestriction restriction,
      String fieldId) {
    if (restriction.getAllowedList().isEmpty()) {
      return originalTypes;
    }
    Set<String> restrictionSet = new HashSet<>(restriction.getAllowedList());
    List<ElementDefinition.TypeRef> finalTypes =
        originalTypes.stream()
            .filter(type -> restrictionSet.contains(type.getCode().getValue()))
            .collect(Collectors.toList());

    List<String> invalidTypes = new ArrayList<>(restrictionSet);
    invalidTypes.removeAll(
        finalTypes.stream().map(type -> type.getCode().getValue()).collect(Collectors.toList()));

    if (!invalidTypes.isEmpty()) {
      throw new IllegalArgumentException(
          "Invalid ChoiceType restriction for "
              + fieldId
              + ". The following types are not allowed by the parent:"
              + invalidTypes
              + ".  Allowed types: "
              + originalTypes.stream()
                  .map(type -> type.getCode().getValue())
                  .collect(Collectors.toList()));
    }
    return finalTypes;
  }

  private List<ElementDefinition.TypeRef> applyReferenceRestriction(
      List<ElementDefinition.TypeRef> originalTypes,
      ReferenceRestriction restriction,
      String fieldId) {
    if (restriction.getAllowedList().isEmpty()) {
      return originalTypes;
    }
    Set<String> originalReferenceTargets =
        originalTypes.stream()
            .filter(type -> type.getCode().getValue().equals("Reference"))
            .flatMap(
                type -> type.getTargetProfileList().stream().map(canonical -> canonical.getValue()))
            .collect(Collectors.toSet());
    if (originalReferenceTargets.isEmpty()) {
      throw new IllegalArgumentException(
          "Invalid FieldRestriction for "
              + fieldId
              + ". It contains reference restrictions, but references are not a valid type for"
              + " that field.");
    }
    List<ElementDefinition.TypeRef> newTypes =
        originalTypes.stream()
            .filter(type -> !type.getCode().getValue().equals("Reference"))
            .collect(Collectors.toList());
    Set<String> newReferenceTargets = new HashSet<>(restriction.getAllowedList());
    Set<String> invalidReferenceTargets = new HashSet<>();
    for (String referenceTarget : newReferenceTargets) {
      if (isAllowedReferenceTarget(referenceTarget, originalReferenceTargets)) {
        newTypes.add(
            ElementDefinition.TypeRef.newBuilder()
                .setCode(Uri.newBuilder().setValue("Reference"))
                .addTargetProfile(Canonical.newBuilder().setValue(referenceTarget))
                .build());
      } else {
        invalidReferenceTargets.add(referenceTarget);
      }
    }
    if (!invalidReferenceTargets.isEmpty()) {
      throw new IllegalArgumentException(
          "Invalid ReferenceRestriction for "
              + fieldId
              + ". The following types are not allowed by the parent:"
              + invalidReferenceTargets
              + ".  Allowed types: "
              + originalReferenceTargets);
    }
    return newTypes;
  }

  private boolean isAllowedReferenceTarget(String url, Set<String> allowedReferenceTargets) {
    if (allowedReferenceTargets.contains(url)) {
      return true;
    }
    StructureDefinition structDef = getStructDefForUrl(url);
    while (structDef.getDerivation().getValue() == TypeDerivationRuleCode.Value.CONSTRAINT) {
      String parentUrl = structDef.getBaseDefinition().getValue();
      if (allowedReferenceTargets.contains(parentUrl)) {
        return true;
      }
      structDef = getStructDefForUrl(parentUrl);
    }
    return false;
  }

  private ElementDefinition buildExtensionSliceElement(
      ExtensionSlice extensionSlice, List<ElementDefinition> elementList) {
    ElementDefinition.Builder extensionElement =
        getExtensionStructDef().getSnapshot().getElement(0).toBuilder();
    ElementData elementData = extensionSlice.getElementData();
    String fieldId =
        extensionSlice.getFieldId().isEmpty()
            ? elementList.get(0).getPath().getValue()
            : extensionSlice.getFieldId();
    String rootId = fieldId + ".extension:" + elementData.getName();
    String rootPath = fieldId + ".extension";
    customizeExtensionRootElement(
        extensionElement,
        extensionSlice.getFieldId() + ".extension:" + elementData.getName(),
        elementData);
    extensionElement.setBase(buildBase("Extension", getExtensionStructDef()));
    setIdAndPath(extensionElement, rootId, rootPath, "");
    extensionElement
        .clearType()
        .addType(
            ElementDefinition.TypeRef.newBuilder()
                .setCode(fhirUri("Extension"))
                .addProfile(fhirCanonical(extensionSlice.getUrl())));
    return extensionElement.build();
  }

  private List<ElementDefinition> buildCodeableConceptSliceElements(
      String codingFieldId, CodingSlice codingSlice) {
    ElementData elementData = codingSlice.getElementData();
    CodeData codeData = codingSlice.getCodeData();
    StructureDefinition baseStructDef = getStructDef(Coding.getDescriptor());
    List<ElementDefinition> codingBaseElements = baseStructDef.getSnapshot().getElementList();
    List<ElementDefinition.Builder> codingElements =
        codingBaseElements.stream().map(ElementDefinition::toBuilder).collect(Collectors.toList());

    // Root Element
    ElementDefinition.Builder rootElement = getElementBuilderById("Coding", codingElements);
    customizeRootElement(rootElement, elementData);
    rootElement.setSliceName(fhirString(elementData.getName()));

    // System Element
    ElementDefinition.Builder systemElement =
        getElementBuilderById("Coding.system", codingElements);
    systemElement.getFixedBuilder().setUri(fhirUri(codeData.getSystem()));

    // Code Element
    ElementDefinition.Builder codeElement = getElementBuilderById("Coding.code", codingElements);
    if (!codingSlice.getCodeData().getFixedValue().isEmpty()) {
      codeElement.getFixedBuilder().setCode(fhirCode(codeData.getFixedValue()));
    }
    codeElement.setBinding(buildCodeBinding(codeData));

    for (ElementDefinition.Builder element : codingElements) {
      String originalId = element.getId().getValue();
      int lastDotIndex = originalId.lastIndexOf(".");
      String lastIdToken =
          lastDotIndex == -1 ? "" : originalId.substring(originalId.lastIndexOf("."));
      element.setBase(buildBase(originalId, baseStructDef));
      setIdAndPath(
          element, codingFieldId + ":" + elementData.getName(), codingFieldId, lastIdToken);
    }

    return codingElements.stream()
        .map(ElementDefinition.Builder::build)
        .collect(Collectors.toList());
  }

  private void setMetadata(
      StructureDefinition.Builder structureDefinitionBuilder,
      String type,
      String baseDefinitionUrl,
      ElementData elementData,
      PackageInfo packageInfo,
      StructureDefinitionKindCode.Value structureDefinitionKind) {
    String url = getStructureDefinitionUrl(elementData);
    structureDefinitionBuilder
        .setId(Id.newBuilder().setValue(elementData.getName()))
        .setUrl(fhirUri(url))
        .setName(fhirString(elementData.getName()))
        .setDate(creationDateTime)
        .setPublisher(fhirString(packageInfo.getPublisher()))
        .setFhirVersion(
            StructureDefinition.FhirVersionCode.newBuilder()
                .setValue(FhirVersion.fromAnnotation(packageInfo.getFhirVersion()).minorVersion))
        .setKind(StructureDefinition.KindCode.newBuilder().setValue(structureDefinitionKind))
        .setAbstract(fhirBoolean(false))
        .addContext(
            StructureDefinition.Context.newBuilder()
                .setType(
                    StructureDefinition.Context.TypeCode.newBuilder()
                        .setValue(ExtensionContextTypeCode.Value.ELEMENT)))
        .setType(fhirUri(type))
        .setBaseDefinition(fhirCanonical(baseDefinitionUrl))
        .setDerivation(
            StructureDefinition.DerivationCode.newBuilder()
                .setValue(TypeDerivationRuleCode.Value.CONSTRAINT));
    if (!packageInfo.getTelcomUrl().isEmpty()) {
      structureDefinitionBuilder.addContact(
          ContactDetail.newBuilder()
              .addTelecom(
                  ContactPoint.newBuilder()
                      .setSystem(
                          ContactPoint.SystemCode.newBuilder()
                              .setValue(ContactPointSystemCode.Value.URL))
                      .setValue(fhirString(packageInfo.getTelcomUrl()))));
    }
    if (!elementData.getDescription().isEmpty()) {
      structureDefinitionBuilder.setDescription(
          Markdown.newBuilder().setValue(elementData.getDescription()));
    }
  }

  private ElementDefinition.Base buildBase(String path, StructureDefinition parentStructDef) {
    ElementDefinition parentElement =
        getElementById(path, parentStructDef.getSnapshot().getElementList());
    Optional<ElementDefinition> parentElementInDifferential =
        getOptionalElementById(path, parentStructDef.getDifferential().getElementList());
    if (parentElementInDifferential.isPresent()) {
      // This element was modified by the parent element, therefore we should base the new element
      // off of it.
      return ElementDefinition.Base.newBuilder()
          .setPath(fhirString(path))
          .setMin(parentElement.getMin())
          .setMax(parentElement.getMax())
          .build();
    } else {
      // This element was unmodified by the parent element.
      // Go a level higher.
      // (note that the top level is Element, which lists all elements in the differential).
      return buildBase(
          parentElement.getBase().getPath().getValue(),
          getStructDefForUrl(parentStructDef.getBaseDefinition().getValue()));
    }
  }

  // Returns the only element in the list matching a given id.
  // Throws IllegalArgumentException if zero or more than one matching elements are found.
  // TODO: consolidate with ProtoGenerator
  private static ElementDefinition getElementById(String id, List<ElementDefinition> elements) {
    return getOptionalElementById(id, elements)
        .orElseThrow(() -> new IllegalArgumentException("No element with id: " + id));
  }

  private static ElementDefinition.Builder getElementBuilderById(
      String id, List<ElementDefinition.Builder> elements) {
    return getOptionalElementBuilderById(id, elements)
        .orElseThrow(() -> new IllegalArgumentException("No element with id: " + id));
  }

  private static Optional<ElementDefinition> getOptionalElementById(
      String id, List<ElementDefinition> elements) {
    return elements.stream()
        .filter(element -> element.getId().getValue().equals(id))
        .collect(MoreCollectors.toOptional());
  }

  private static Optional<ElementDefinition.Builder> getOptionalElementBuilderById(
      String id, List<ElementDefinition.Builder> elements) {
    return elements.stream()
        .filter(element -> element.getId().getValue().equals(id))
        .collect(MoreCollectors.toOptional());
  }

  private static final Pattern SUB_EXTENSION_PATH_PATTERN =
      Pattern.compile("^Extension(\\.extension)+(\\.[a-zA-Z]+)$");

  private ElementDefinition getBaseElement(
      ElementDefinition element, List<ElementDefinition> elements) {
    String path = element.getPath().getValue();
    Optional<ElementDefinition> exactMatch = getOptionalElementById(path, elements);
    if (exactMatch.isPresent()) {
      return exactMatch.get();
    }
    // Check base path in case of choice type specializations.
    String basePath = element.getBase().getPath().getValue();
    Optional<ElementDefinition> baseMatch = getOptionalElementById(basePath, elements);
    if (baseMatch.isPresent()) {
      return baseMatch.get();
    }

    // Sub-extension fields on complex extensions are a special case because they are not 1-1 with
    // fields on the base extension (they can be recursively nested).
    // But, it's extensions all the way down, so we can drop all ".extension" tokens from the path
    // to get the root path.
    // E.g., the root path of Extension.extension.extension.id is Extension.id.
    Matcher subExtensionMatcher = SUB_EXTENSION_PATH_PATTERN.matcher(path);
    if (subExtensionMatcher.matches()) {
      Optional<ElementDefinition> baseExtensionElement =
          getOptionalElementById("Extension" + subExtensionMatcher.group(2), elements);
      if (baseExtensionElement.isPresent()) {
        return baseExtensionElement.get();
      }
    }

    // Sometimes elements represent specializations of subfields of base elements.
    // E.g., for CodeableConcept slicing, we can have an element with
    // path: SomeResource.code.coding,
    // which corresponds to a specialization of the field CodeableConcept.coding, on the element
    // SomeResource.code.
    // In this case, we have to load the structure definition of the base resource (CodeableConcept
    // in the above example) in order to find the base element definition.
    String elementBaseType = Splitter.on(".").limit(2).splitToList(basePath).get(0);
    StructureDefinition baseStructDef = idToBaseStructDefMap.get(elementBaseType);
    if (baseStructDef != null) {
      Optional<ElementDefinition> baseElementOptional =
          getOptionalElementById(basePath, baseStructDef.getSnapshot().getElementList());
      if (baseElementOptional.isPresent()) {
        return baseElementOptional.get();
      }
    }
    throw new IllegalArgumentException(
        "No matching base element for: " + element.getId().getValue());
  }

  private static ElementDefinitionBinding buildCodeBinding(CodeData codeData) {
    ElementDefinitionBinding.Builder binding =
        ElementDefinitionBinding.newBuilder()
            .setValueSet(fhirCanonical(codeData.getSystem()))
            .setStrength(
                ElementDefinition.ElementDefinitionBinding.StrengthCode.newBuilder()
                    .setValue(
                        codeData
                                .getBindingStrength()
                                .equals(BindingStrengthCode.Value.INVALID_UNINITIALIZED)
                            ? BindingStrengthCode.Value.REQUIRED
                            : codeData.getBindingStrength()));

    return binding.build();
  }

  private static ElementDefinition.Builder setIdAndPath(
      ElementDefinition.Builder element, String rootId, String rootPath, String tail) {
    return element.setId(fhirString(rootId + tail)).setPath(fhirString(rootPath + tail));
  }

  private static com.google.fhir.r4.core.String fhirString(String value) {
    return com.google.fhir.r4.core.String.newBuilder().setValue(value).build();
  }

  private static com.google.fhir.r4.core.Boolean fhirBoolean(boolean value) {
    return com.google.fhir.r4.core.Boolean.newBuilder().setValue(value).build();
  }

  private static Uri fhirUri(String value) {
    return Uri.newBuilder().setValue(value).build();
  }

  private static Canonical fhirCanonical(String value) {
    return Canonical.newBuilder().setValue(value).build();
  }

  private static Code fhirCode(String value) {
    return Code.newBuilder().setValue(value).build();
  }

  private static DateTime buildCreationDateTime(LocalDate localDate) {
    return new DateTimeWrapper(
            localDate.format(DateTimeFormatter.ISO_LOCAL_DATE), ZoneId.of("US/Pacific"))
        .copyInto(DateTime.newBuilder())
        .build();
  }

  private StructureDefinition getExtensionStructDef() {
    return getStructDef(Extension.getDescriptor());
  }

  private StructureDefinition getStructDef(Descriptor descriptor) {
    return getStructDefForUrl(
        descriptor.getOptions().getExtension(Annotations.fhirStructureDefinitionUrl));
  }

  private StructureDefinition getStructDefForUrl(String url) {
    StructureDefinition structDef = urlToStructDefMap.get(url);
    if (structDef == null) {
      throw new IllegalArgumentException("No known StructureDefinition for url: " + url);
    }
    return structDef;
  }

  private ElementDefinition codeableConceptCodingStructDef = null;

  private ElementDefinition getCodeableConceptCodingElement() {
    if (codeableConceptCodingStructDef != null) {
      return codeableConceptCodingStructDef;
    }
    codeableConceptCodingStructDef =
        getElementById(
            "CodeableConcept.coding",
            getStructDef(CodeableConcept.getDescriptor()).getSnapshot().getElementList());
    return codeableConceptCodingStructDef;
  }

  private String getStructureDefinitionUrl(ElementData elementData) {
    return elementData.getUrlOverride().isEmpty()
        ? packageInfo.getBaseUrl() + "/" + elementData.getName()
        : elementData.getUrlOverride();
  }

  // Size changes can only get *more* restrictive.
  // In other words, for a size to be a valid new size, it must be a valid subset of the
  // original size.
  private static boolean validateSizeChange(
      SizeRestriction newSize, ElementDefinitionOrBuilder element) {
    SizeRestriction oldSize = getSizeRestrictionFromElement(element);
    if (newSize == oldSize) {
      return true;
    }
    switch (oldSize) {
      case OPTIONAL:
        return newSize == SizeRestriction.REQUIRED || newSize == SizeRestriction.ABSENT;
      case AT_LEAST_ONE:
        return newSize == SizeRestriction.REQUIRED;
      case REPEATED:
        return true;
      case ABSENT:
      case REQUIRED:
      default:
        return false;
    }
  }

  private static SizeRestriction getSizeRestrictionFromElement(ElementDefinitionOrBuilder element) {
    int minValue = element.getMin().getValue();
    String maxValue = element.getMax().getValue();

    if (minValue == 0) {
      if (maxValue.equals("0")) {
        return SizeRestriction.ABSENT;
      }
      if (maxValue.equals("1")) {
        return SizeRestriction.OPTIONAL;
      }
      if (maxValue.equals("*")) {
        return SizeRestriction.REPEATED;
      }
    }
    if (minValue == 1) {
      if (maxValue.equals("1")) {
        return SizeRestriction.REQUIRED;
      }
      if (maxValue.equals("*")) {
        return SizeRestriction.AT_LEAST_ONE;
      }
    }
    return SizeRestriction.UNKNOWN;
  }

  private static UnsignedInt minSize(SizeRestriction size) {
    switch (size) {
      case UNKNOWN: // no size specified is treated as optional.
      case ABSENT:
      case OPTIONAL:
      case REPEATED:
        return UnsignedInt.newBuilder().setValue(0).build();
      case REQUIRED:
      case AT_LEAST_ONE:
        return UnsignedInt.newBuilder().setValue(1).build();
      default:
        throw new IllegalArgumentException("Unrecognized SizeRestriction: " + size);
    }
  }

  private static com.google.fhir.r4.core.String maxSize(SizeRestriction size) {
    switch (size) {
      case ABSENT:
        return fhirString("0");
      case UNKNOWN: // no size specified is treated as optional.
      case OPTIONAL:
      case REQUIRED:
        return fhirString("1");
      case REPEATED:
      case AT_LEAST_ONE:
        return fhirString("*");
      default:
        throw new IllegalArgumentException("Unrecognized SizeRestriction: " + size);
    }
  }

  private static boolean isSliceId(String id) {
    return (id.lastIndexOf(":") > id.lastIndexOf("."));
  }
}
