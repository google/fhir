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

package com.google.fhir.common;

import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.Annotations.FhirVersion;
import com.google.fhir.proto.Annotations.StructureDefinitionKindValue;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.MessageOrBuilder;

/** Helper methods for dealing with FHIR protocol buffer annotations. */
public final class AnnotationUtils {

  public static boolean isResource(MessageOrBuilder message) {
    return isResource(message.getDescriptorForType());
  }

  public static boolean isResource(Descriptor descriptor) {
    return isResource(descriptor.toProto());
  }

  public static boolean isResource(DescriptorProto descriptor) {
    return descriptor.getOptions().hasExtension(Annotations.structureDefinitionKind)
        && descriptor.getOptions().getExtension(Annotations.structureDefinitionKind)
            == StructureDefinitionKindValue.KIND_RESOURCE;
  }

  public static boolean isPrimitiveType(MessageOrBuilder message) {
    return isPrimitiveType(message.getDescriptorForType());
  }

  public static boolean isPrimitiveType(Descriptor descriptor) {
    return isPrimitiveType(descriptor.toProto());
  }

  public static boolean isPrimitiveType(DescriptorProto descriptor) {
    return descriptor.getOptions().hasExtension(Annotations.structureDefinitionKind)
        && descriptor.getOptions().getExtension(Annotations.structureDefinitionKind)
            == StructureDefinitionKindValue.KIND_PRIMITIVE_TYPE;
  }

  public static boolean isChoiceType(FieldDescriptor field) {
    return field.getType() == FieldDescriptor.Type.MESSAGE
        && field.getMessageType().getOptions().getExtension(Annotations.isChoiceType);
  }

  public static boolean isChoiceType(Descriptor descriptor) {
    return descriptor.getOptions().getExtension(Annotations.isChoiceType);
  }

  public static boolean isReference(MessageOrBuilder message) {
    return isReference(message.getDescriptorForType());
  }

  public static boolean isReference(Descriptor descriptor) {
    return isReference(descriptor.toProto());
  }

  public static boolean isReference(DescriptorProto descriptor) {
    return descriptor.getOptions().getExtensionCount(Annotations.fhirReferenceType) > 0;
  }

  public static String getValueRegexForPrimitiveType(MessageOrBuilder message) {
    return getValueRegexForPrimitiveType(message.getDescriptorForType());
  }

  public static String getValueRegexForPrimitiveType(Descriptor descriptor) {
    return getValueRegexForPrimitiveType(descriptor.toProto());
  }

  public static String getValueRegexForPrimitiveType(DescriptorProto descriptor) {
    if (!isPrimitiveType(descriptor)
        || !descriptor.getOptions().hasExtension(Annotations.valueRegex)) {
      return null;
    }
    return descriptor.getOptions().getExtension(Annotations.valueRegex);
  }

  public static String getStructureDefinitionUrl(Descriptor descriptor) {
    return descriptor.getOptions().getExtension(Annotations.fhirStructureDefinitionUrl);
  }

  public static String getFhirValuesetUrl(Descriptor descriptor) {
    return descriptor.getOptions().getExtension(Annotations.fhirValuesetUrl);
  }

  public static String getFhirCodeSystemUrl(EnumDescriptor descriptor) {
    return descriptor.getOptions().getExtension(Annotations.fhirCodeSystemUrl);
  }

  public static String getEnumValuesetUrl(EnumDescriptor descriptor) {
    return descriptor.getOptions().getExtension(Annotations.enumValuesetUrl);
  }

  public static boolean isProfileOf(Descriptor base, Descriptor test) {
    for (int i = 0; i < test.getOptions().getExtensionCount(Annotations.fhirProfileBase); i++) {
      if (test.getOptions()
          .getExtension(Annotations.fhirProfileBase, i)
          .equals(base.getOptions().getExtension(Annotations.fhirStructureDefinitionUrl))) {
        return true;
      }
    }
    return false;
  }

  public static FhirVersion getFhirVersion(Descriptor descriptor) {
    return descriptor.getFile().getOptions().getExtension(Annotations.fhirVersion);
  }

  public static boolean sameFhirType(Descriptor descriptorA, Descriptor descriptorB) {
    return getStructureDefinitionUrl(descriptorA).equals(getStructureDefinitionUrl(descriptorB));
  }
}
