//    Copyright 2021 Google Inc.
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
import static com.google.common.collect.ImmutableMap.toImmutableMap;
import static java.lang.Math.max;
import static java.util.stream.Collectors.toMap;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.fhir.common.Codes;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.ProtoGeneratorAnnotations;
import com.google.fhir.proto.ProtogenConfig;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.DescriptorProtoOrBuilder;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProtoOrBuilder;
import com.google.protobuf.DescriptorProtos.EnumValueDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

/**
 * Utility for comparing new versions of a proto to old versions of the proto, to ensure that no tag
 * numbers are altered.
 */
final class FieldRetagger {

  private FieldRetagger() {}

  /**
   * Retags the DescriptorProto in a FileDescriptorProto against their current R4 counterparts,
   * where possible. This process ensures that any field that is the same in the reference package
   * (current R4) as the package being generated will have the same tag numbers, and any new fields
   * in the new message will use a tag number that is not in use by the reference counterpart.
   *
   * <p>This is done to grant the maximum possible flexibility for ultimately moving to a combined
   * versionless representation of normative resources. For instance, since Patient is normative, an
   * R4 or an R5 Patient should theoretically "fit" in an R6 proto, but this is only possible if tag
   * numbers line up between versions. This currently uses R4 as the reference version, but
   * ultimately this should use the most recent published version.
   *
   * <p>Note that this is a best-effort algorithm, and does not guarantee binary compatibility.
   * Binary compatibility should be independently verified before anything relies on it.
   */
  static FileDescriptorProto retagFile(
      FileDescriptorProto original, ProtogenConfig protogenConfig) {
    List<DescriptorProto> retaggedMessages = new ArrayList<>();

    for (DescriptorProto originalDescriptor : original.getMessageTypeList()) {
      String fullName = protogenConfig.getJavaProtoPackage() + "." + originalDescriptor.getName();

      // Name of this message in the reference (i.e., current R4) package.
      String r4FullName = fullName.replaceFirst("\\.r[0-9]*\\.", ".r4.");

      // Skip ContainedResource - each version gets its own range of numbers in ContainedResource,
      // governed by the `contained_resource_offset` flag.
      if (fullName.endsWith(".ContainedResource")) {
        retaggedMessages.add(originalDescriptor);
        continue;
      }

      try {
        Class<?> r4MessageClass = Class.forName(r4FullName);
        try {
          DescriptorProto r4Descriptor =
              ((Descriptor) r4MessageClass.getMethod("getDescriptor").invoke(null)).toProto();
          retaggedMessages.add(FieldRetagger.retagMessage(originalDescriptor, r4Descriptor));
        } catch (ReflectiveOperationException e) {
          // If we find a class with the expected name, it should always have a "getDescriptor".
          throw new IllegalStateException(e);
        }
      } catch (ClassNotFoundException e) {
        // No matching class in R4 - that's ok, it's something new in this version.
        retaggedMessages.add(originalDescriptor);
      }
    }
    return original.toBuilder().clearMessageType().addAllMessageType(retaggedMessages).build();
  }

  /**
   * Given a FileDescriptorProto containing terminology enums, retags the enums against their R4
   * counterparts.
   *
   * <p>This is done to grant the maximum possible flexibility for ultimately moving to a combined
   * versionless representation of normative resources. For instance, since Patient is normative, an
   * R4 or an R5 Patient should theoretically "fit" in an R6 proto, but this is only possible if tag
   * numbers line up between versions. This currently uses R4 as the reference version, but
   * ultimately this should use the most recent published version.
   *
   * <p>Note that this is a best-effort algorithm, and does not guarantee binary compatibility.
   * Binary compatibility should be independently verified before anything relies on it.
   */
  static FileDescriptorProto retagTerminologyFile(
      FileDescriptorProto original, ProtogenConfig protogenConfig) {
    ImmutableMap<String, Map<String, Integer>> goldenValueSetsByUrl =
        com.google.fhir.r4.core.BodyLengthUnitsValueSet.getDescriptor()
            .getFile()
            .getMessageTypes()
            .stream()
            .map(descriptor -> descriptor.getEnumTypes().get(0))
            .collect(
                toImmutableMap(
                    enumDescriptor ->
                        enumDescriptor.getOptions().getExtension(Annotations.enumValuesetUrl),
                    enumDescriptor -> getTerminologyMap(enumDescriptor)));

    ImmutableMap<String, Map<String, Integer>> goldenCodeSystemsByUrl =
        com.google.fhir.r4.core.AbstractTypeCode.getDescriptor()
            .getFile()
            .getMessageTypes()
            .stream()
            .map(descriptor -> descriptor.getEnumTypes().get(0))
            .collect(
                toImmutableMap(
                    enumDescriptor ->
                        enumDescriptor.getOptions().getExtension(Annotations.fhirCodeSystemUrl),
                    enumDescriptor -> getTerminologyMap(enumDescriptor)));

    Map<String, Map<String, Integer>> goldenTerminologiesByUrl = new HashMap<>();
    goldenTerminologiesByUrl.putAll(goldenValueSetsByUrl);
    goldenTerminologiesByUrl.putAll(goldenCodeSystemsByUrl);

    ImmutableList<DescriptorProto> retaggedMessages =
        original.getMessageTypeList().stream()
            .map(descriptor -> retagTerminology(descriptor, goldenTerminologiesByUrl))
            .collect(toImmutableList());

    return original.toBuilder().clearMessageType().addAllMessageType(retaggedMessages).build();
  }

  private static ImmutableMap<String, Integer> getTerminologyMap(EnumDescriptor terminologyEnum) {
    return terminologyEnum.getValues().stream()
        .collect(
            toImmutableMap(
                enumValue -> Codes.enumValueToCodeString(enumValue),
                enumValue -> enumValue.getNumber()));
  }

  /**
   * Given a new message and a golden message, returns a copy of the new message that guarantees
   * that, for that message and all submessages contained within both the new and golden message:
   *
   * <ol>
   *   <li>If a field within the message exists in both new and golden, they will have the same tag
   *       number
   *   <li>If a field exists with the new but not the golden, it will have a tag number greater than
   *       any in the golden, or than the last reserved field in the golden.
   *   <li>If the highest-numbered field from the golden has been removed and no new fields added,
   *       adds a reserved field with the same number as the removed field, to guarantee that that
   *       number will never be reused. Note that this does not insert reserved ranges for
   *       lower-numbered removed fields, since all new fields are added with numbers greater than
   *       any in the golden, so there is no risk of reusing a lower number.
   * </ol>
   *
   * Note that this does not sort fields by tag numbers, so the tag numbers may no longer be
   * consecutive. This causes the fields to appear in the printed proto in the same order they are
   * defined in the FHIR Profile, regardless of any tag adjustments needed.
   */
  static DescriptorProto retagMessage(DescriptorProto newMessage, DescriptorProto golden) {
    DescriptorProto.Builder newBuilder = newMessage.toBuilder();
    retagMessage(newBuilder, golden);
    return newBuilder.build();
  }

  private static void retagMessage(DescriptorProto.Builder newBuilder, DescriptorProto golden) {
    Map<String, FieldDescriptorProto> referenceMap = getFieldMap(golden.getFieldList());
    // List of fields with no corresponding field in the golden message.
    // These should be assigned numbers greater than any in use in the golden message.
    List<FieldDescriptorProto.Builder> newFields = new ArrayList<>();

    for (FieldDescriptorProto.Builder fieldBuilder : newBuilder.getFieldBuilderList()) {
      FieldDescriptorProto goldenField = referenceMap.get(fieldBuilder.getName());
      if (goldenField == null
          || !sameFhirType(newBuilder.build(), fieldBuilder.build(), golden, goldenField)
          || fieldBuilder.getLabel() != goldenField.getLabel()) {
        if (fieldBuilder.getOptions().hasExtension(ProtoGeneratorAnnotations.reservedReason)) {
          checkReservedField(fieldBuilder.getNumber(), golden);
        } else {
          newFields.add(fieldBuilder);
        }
      } else {
        if (goldenField.getNumber() != fieldBuilder.getNumber()) {
          fieldBuilder.setNumber(goldenField.getNumber());
        }
      }
    }

    int highestGoldenTag = getHighestNumberInUse(golden);

    if (!newFields.isEmpty()) {
      // Assign any new fields to numbers greater than any on the golden message.
      // Assuming both builder file and golden file were not malformed to begin with, this will
      // guarantee that no field is reused and all corresponding fields have identical numbers.
      int nextTag = highestGoldenTag + 1;
      for (FieldDescriptorProto.Builder newField : newFields) {
        newField.setNumber(nextTag++);
      }
    }

    // If the golden has a higher tag number (either present or reserved) than the highest tag
    // in the new file, add a reserved field for the highest golden tag.
    // This ensures that all subsequent iterations of the proto will never use that or any
    // lower-numbered tag.
    // TODO(b/192419079): For historical reasons, ProtoGenerator generates reserved fields using
    // normal fields, with the "reservedReason" annotation.  These should be updated to just use
    // ReservedRanges.
    if (getHighestNumberInUse(newBuilder) < highestGoldenTag) {
      newBuilder
          .addFieldBuilder()
          .setNumber(highestGoldenTag)
          .getOptionsBuilder()
          .setExtension(
              ProtoGeneratorAnnotations.reservedReason,
              "Field "
                  + highestGoldenTag
                  + " reserved to prevent reuse of field that was previously deleted.");
    }

    retagMessages(newBuilder.getNestedTypeBuilderList(), golden.getNestedTypeList());
  }

  /** A mapping from FHIR terminology urls to their equivalent in R4, for when they are changed. */
  private static final ImmutableMap<String, String> EXPLICIT_TERMINOLOGY_MAPPINGS =
      ImmutableMap.of(
          // The resource-types and request-resource-types ValueSets were a 1-1 with CodeSystems in
          // R4, so no ValueSet enum was generated.  In R5, these are complex ValueSets, and so
          // have a ValueSet enum.
          // Retag these R5 ValueSets against the corresponding CodeSystems in R4.
          "http://hl7.org/fhir/ValueSet/resource-types",
          "http://hl7.org/fhir/resource-types",
          "http://hl7.org/fhir/ValueSet/request-resource-types",
          "http://hl7.org/fhir/request-resource-types",

          // Between R4 and R5, the medicationknowledge-status CodeSystem changed urls.
          "http://hl7.org/fhir/CodeSystem/medicationknowledge-status",
          "http://terminology.hl7.org/CodeSystem/medicationknowledge-status");

  private static Optional<Map<String, Integer>> getGoldenTerminology(
      EnumDescriptorProtoOrBuilder terminologyEnum,
      Map<String, Map<String, Integer>> goldenTerminologiesByUrl) {
    String url = getTerminologyUrl(terminologyEnum);
    if (goldenTerminologiesByUrl.containsKey(url)) {
      return Optional.of(goldenTerminologiesByUrl.get(url));
    }
    if (EXPLICIT_TERMINOLOGY_MAPPINGS.containsKey(url)) {
      return Optional.of(goldenTerminologiesByUrl.get(EXPLICIT_TERMINOLOGY_MAPPINGS.get(url)));
    }
    return Optional.empty();
  }

  private static DescriptorProto retagTerminology(
      DescriptorProto original, Map<String, Map<String, Integer>> goldenTerminologiesByUrl) {
    DescriptorProto.Builder newBuilder = original.toBuilder();
    EnumDescriptorProto.Builder terminologyEnum = newBuilder.getEnumTypeBuilderList().get(0);

    Optional<Map<String, Integer>> goldenTerminologyOptional =
        getGoldenTerminology(terminologyEnum, goldenTerminologiesByUrl);
    if (goldenTerminologyOptional.isEmpty()) {
      // No golden to tag against.
      return original;
    }
    Map<String, Integer> goldenTerminology = goldenTerminologyOptional.get();

    List<EnumValueDescriptorProto.Builder> newCodes = new ArrayList<>();
    for (EnumValueDescriptorProto.Builder value : terminologyEnum.getValueBuilderList()) {
      String code = Codes.enumValueToCodeString(value);
      if (goldenTerminology.containsKey(code)) {
        value.setNumber(goldenTerminology.get(code));
      } else {
        newCodes.add(value);
      }
    }

    int nextTag = Collections.max(goldenTerminology.values()) + 1;

    // There is a bug in the R4 Extensions IG that causes it to be mislabeled as R5.
    // To make these resources parse, we hardcode an R5 version enum into the R4 version
    // terminology.
    // The hardcoded enum is assigned tag number 51 in the ValueSetGeneratorV2 in order to match the
    // enum assigned in R5.  However, this messes with the retagging logic here, since this starts
    // assigning new tag numbers after the max tag number in use, because the hardcoded tag changes
    // the max tag number.  So, in this special case, start from the max non-hardcoded tag number
    // (22), and skip over 51.
    boolean isR5VersionEnum =
        getTerminologyUrl(terminologyEnum).equals("http://hl7.org/fhir/FHIR-version")
            && nextTag == 52;
    if (isR5VersionEnum) {
      nextTag = 23;
    }

    for (EnumValueDescriptorProto.Builder value : newCodes) {
      if (isR5VersionEnum && nextTag == 51) {
        nextTag++;
      }
      value.setNumber(nextTag++);
    }

    return newBuilder.build();
  }

  private static String getTerminologyUrl(EnumDescriptorProtoOrBuilder terminologyEnum) {
    if (terminologyEnum.getOptions().hasExtension(Annotations.enumValuesetUrl)) {
      return terminologyEnum.getOptions().getExtension(Annotations.enumValuesetUrl);
    }
    if (terminologyEnum.getOptions().hasExtension(Annotations.fhirCodeSystemUrl)) {
      return terminologyEnum.getOptions().getExtension(Annotations.fhirCodeSystemUrl);
    }
    throw new IllegalArgumentException("Descriptor has no url: " + terminologyEnum.getName());
  }

  private static DescriptorProto findLocalType(DescriptorProto message, String name) {
    for (DescriptorProto nested : message.getNestedTypeList()) {
      if (name.endsWith(message.getName() + "." + nested.getName())) {
        return nested;
      }
    }
    return null;
  }

  /**
   * Replaces a version-specific package token (e.g., r5 in google.fhir.r5.Code) with a versionless
   * token "V". This allows comparing two FHIR types from different versions to see if they refer to
   * the same underlying FHIR type.
   */
  private static String versionIndependantType(String typeName) {
    return typeName.replaceAll("\\.r[0-9]*\\.", ".V.");
  }

  private static boolean sameFhirType(
      DescriptorProto firstParent,
      FieldDescriptorProto firstField,
      DescriptorProto secondParent,
      FieldDescriptorProto secondField) {
    if (firstField.getType() != secondField.getType()) {
      // Different data types.  Definitely not the same FHIR type.
      return false;
    }

    if (firstField.getType() != FieldDescriptorProto.Type.TYPE_MESSAGE) {
      // Same primitive types.  That's always compatible.
      return true;
    }

    DescriptorProto firstLocalType = findLocalType(firstParent, firstField.getTypeName());
    DescriptorProto secondLocalType = findLocalType(secondParent, secondField.getTypeName());

    if ((firstLocalType == null) != (secondLocalType == null)) {
      // One is a local type, the other is a datatype.  Not the same.
      return false;
    }

    if (firstLocalType != null
        && !findLocalType(firstParent, firstField.getTypeName())
            .getOptions()
            .getExtension(Annotations.fhirValuesetUrl)
            .equals(
                findLocalType(secondParent, secondField.getTypeName())
                    .getOptions()
                    .getExtension(Annotations.fhirValuesetUrl))) {
      // At least one is a code with a bound Valueset, but both are not bound to the same Valueset.
      return false;
    }

    return versionIndependantType(firstField.getTypeName())
        .equals(versionIndependantType(secondField.getTypeName()));
  }

  private static void retagMessages(
      List<DescriptorProto.Builder> builderMessages, List<DescriptorProto> goldenMessages) {
    Map<String, DescriptorProto> referenceMap = getMessageMap(goldenMessages);
    for (DescriptorProto.Builder messageBuilder : builderMessages) {
      DescriptorProto goldenMessage = referenceMap.get(messageBuilder.getName());
      if (goldenMessage != null) {
        retagMessage(messageBuilder, goldenMessage);
      }
    }
  }

  private static int getHighestNumberInUse(DescriptorProtoOrBuilder message) {
    int highestTagNumber =
        message.getFieldList().stream().map(FieldDescriptorProto::getNumber).reduce(0, Math::max);
    int highestReserved =
        message.getReservedRangeList().stream()
            .map(range -> range.getEnd() - 1 /* end is exclusive */)
            .reduce(0, Math::max);
    return max(highestTagNumber, highestReserved);
  }

  private static Map<String, DescriptorProto> getMessageMap(List<DescriptorProto> messages) {
    return messages.stream().collect(toImmutableMap(DescriptorProto::getName, message -> message));
  }

  private static Map<String, FieldDescriptorProto> getFieldMap(List<FieldDescriptorProto> fields) {
    return fields.stream().collect(toMap(FieldDescriptorProto::getName, field -> field));
  }

  private static void checkReservedField(int number, DescriptorProto golden) {
    for (DescriptorProto.ReservedRange range : golden.getReservedRangeList()) {
      if (number >= range.getStart() && number < range.getEnd()) {
        return;
      }
    }

    throw new IllegalStateException(
        "Encountered unexpected reserved field in new proto that does not exist in reference"
            + " proto.");
  }
}
