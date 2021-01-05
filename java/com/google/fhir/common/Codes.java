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

package com.google.fhir.common;

import com.google.common.base.Ascii;
import com.google.fhir.proto.Annotations;
import com.google.protobuf.DescriptorProtos.EnumValueDescriptorProtoOrBuilder;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.MessageOrBuilder;

/** Utilities for interacting with FHIR terminologies in a version-independent way. */
public final class Codes {

  private Codes() {}

  public static String enumValueToCodeString(EnumValueDescriptor codeEnumValue) {
    return ENUM_VALUE_TO_CODE_STRING_MEMOS.computeIfAbsent(
        codeEnumValue, descriptor -> enumValueToCodeString(descriptor.toProto()));
  }

  public static String enumValueToCodeString(EnumValueDescriptorProtoOrBuilder codeEnumValue) {
    return codeEnumValue.getOptions().hasExtension(Annotations.fhirOriginalCode)
        ? codeEnumValue.getOptions().getExtension(Annotations.fhirOriginalCode)
        : enumCaseToFhirCase(codeEnumValue.getName());
  }

  private static final DescriptorMemosMap<EnumValueDescriptor, String>
      ENUM_VALUE_TO_CODE_STRING_MEMOS = new DescriptorMemosMap<>();

  public static String getCodeAsString(MessageOrBuilder code) {
    Descriptor descriptor = code.getDescriptorForType();

    if (!FhirTypes.isTypeOrProfileOfCode(code.getDescriptorForType())) {
      throw new IllegalArgumentException("Not a code: " + descriptor.getFullName());
    }

    FieldDescriptor valueField = ProtoUtils.findField(code, "value");
    switch (valueField.getType()) {
      case STRING:
        return (String) code.getField(valueField);
      case ENUM:
        return enumValueToCodeString(((EnumValueDescriptor) code.getField(valueField)));
      default:
        throw new IllegalArgumentException(
            "Invalid value type for Code: " + descriptor.getFullName());
    }
  }

  private static String enumCaseToFhirCase(String enumCase) {
    return Ascii.toLowerCase(enumCase).replace('_', '-');
  }
}
