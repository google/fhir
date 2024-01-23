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

import static java.lang.Math.max;
import static java.util.stream.Collectors.toMap;

import com.google.fhir.proto.ProtoGeneratorAnnotations;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.DescriptorProtoOrBuilder;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Utility for comparing new versions of a proto to old versions of the proto, to ensure that no tag
 * numbers are altered.
 */
final class FieldRetagger {

  private FieldRetagger() {}

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

  /**
   * Replaces a version-specific package token (e.g., r5 in google.fhir.r5.Code) with a versionless
   * token "V". This allows comparing two FHIR types from different versions to see if they refer to
   * the same underlying FHIR type.
   */
  private static String versionIndependantType(String typeName) {
    return typeName.replaceAll("\\.r[0-9]*\\.", ".V.");
  }

  private static boolean sameFhirType(FieldDescriptorProto first, FieldDescriptorProto second) {
    if (first.getType() != second.getType()) {
      // Different data types.  Definitely not the same FHIR type.
      return false;
    }

    if (first.getType() != FieldDescriptorProto.Type.TYPE_MESSAGE) {
      // Same primitive types.  That's always compatible.
      return true;
    }

    return versionIndependantType(first.getTypeName())
        .equals(versionIndependantType(second.getTypeName()));
  }

  private static void retagMessage(DescriptorProto.Builder newBuilder, DescriptorProto golden) {
    Map<String, FieldDescriptorProto> referenceMap = getFieldMap(golden.getFieldList());
    // List of fields with no corresponding field in the golden message.
    // These should be assigned numbers greater than any in use in the golden message.
    List<FieldDescriptorProto.Builder> newFields = new ArrayList<>();

    for (FieldDescriptorProto.Builder fieldBuilder : newBuilder.getFieldBuilderList()) {
      FieldDescriptorProto goldenField = referenceMap.get(fieldBuilder.getName());
      if (goldenField == null || !sameFhirType(fieldBuilder.build(), goldenField)) {
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
    return messages.stream().collect(toMap(DescriptorProto::getName, message -> message));
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
