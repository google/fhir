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

import com.google.api.services.bigquery.model.TableFieldSchema;
import com.google.api.services.bigquery.model.TableSchema;
import com.google.common.base.CaseFormat;
import com.google.common.collect.ImmutableSet;
import com.google.fhir.stu3.proto.Annotations;
import com.google.fhir.stu3.proto.ContainedResource;
import com.google.fhir.stu3.proto.Decimal;
import com.google.fhir.stu3.proto.Extension;
import com.google.fhir.stu3.proto.Identifier;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/** A simple class to infer a BigQuery schema from protocol buffer messages. */
public final class BigQuerySchema {

  /* Generate a schema for a specific FieldDescriptor, with an optional message instance. */
  private static TableFieldSchema fromFieldDescriptor(FieldDescriptor fieldDescriptor) {
    // We directly use the jsonName of the field when generating the schema.
    String fieldName = fieldDescriptor.getJsonName();
    TableFieldSchema field =
        new TableFieldSchema()
            .setName(fieldName)
            .setMode(fieldDescriptor.isRepeated() ? "REPEATED" : "NULLABLE");
    if (fieldDescriptor.getType() != FieldDescriptor.Type.MESSAGE) {
      throw new IllegalStateException(
          "Unexpected primitive field: " + fieldDescriptor.getFullName());
    }
    Descriptor fieldType = fieldDescriptor.getMessageType();
    // We don't include the "id" except for resources.
    if (fieldName.equals("id")
        && !AnnotationUtils.isResource(fieldDescriptor.getContainingType())) {
      return null;
    }
    if (AnnotationUtils.isPrimitiveType(fieldType)) {
      return field.setType(schemaTypeForPrimitive(fieldType));
    }

    // We don't include extensions or contained resources unless they exist in the data.
    if (fieldType.equals(Extension.getDescriptor())
        || fieldType.equals(ContainedResource.getDescriptor())) {
      return field.setType("STRING");
    }
    // We don't include nested types.
    // TODO: consider allowing a certain level of nesting.
    if (fieldType.equals(fieldDescriptor.getContainingType())) {
      return null;
    }
    // Identifier and Reference refer to each other. We stop the recursion by not including
    // Identifier.assigner unless it exists in the data.
    if (fieldDescriptor.getContainingType().equals(Identifier.getDescriptor())
        && fieldName.equals("assigner")) {
      return null;
    }
    field.setType("RECORD");
    return field.setFields(fromDescriptor(fieldType, fieldDescriptor).getFields());
  }

  /** Build a BigQuery schema for this message type when serialized to analytic JSON. */
  public static TableSchema fromDescriptor(Descriptor descriptor) {
    return fromDescriptor(descriptor, null);
  }

  public static TableSchema fromDescriptor(Descriptor descriptor, FieldDescriptor fieldDescriptor) {
    List<TableFieldSchema> fields = new ArrayList<>();
    for (FieldDescriptor field : getValidFields(descriptor, fieldDescriptor)) {
      TableFieldSchema fieldSchema = fromFieldDescriptor(field);
      if (fieldSchema != null) {
        fields.add(fieldSchema);
      }
    }
    return new TableSchema().setFields(fields);
  }

  private static List<FieldDescriptor> getValidFields(
      Descriptor descriptor, FieldDescriptor fieldDescriptor) {
    if (fieldDescriptor != null && AnnotationUtils.isReference(descriptor)) {
      Set<String> descriptorReferences =
          new HashSet<>(descriptor.getOptions().getExtension(Annotations.fhirReferenceType));
      // We currently only support the full reference type.
      if (descriptorReferences.size() != 1
          || !descriptorReferences.iterator().next().equals("Resource")) {
        throw new IllegalArgumentException("Invalid reference type:" + descriptor.getFullName());
      }
      Set<String> fieldReferences =
          new HashSet<>(fieldDescriptor.getOptions().getExtension(Annotations.validReferenceType));
      if (fieldReferences.size() == 1
          && fieldReferences
              .iterator()
              .next()
              .equals("http://hl7.org/fhir/StructureDefinition/Resource")) {
        return descriptor.getFields();
      }

      // Accept the generic types (uri, fragment) as well as the specified ones.
      List<FieldDescriptor> fields = new ArrayList<>();
      Set<String> staticNames =
          ImmutableSet.of("uri", "fragment", "extension", "identifier", "display");
      for (FieldDescriptor field : descriptor.getFields()) {
        String resourceFieldName =
            CaseFormat.LOWER_UNDERSCORE.to(CaseFormat.UPPER_CAMEL, field.getName());
        if (resourceFieldName.endsWith("Id")) {
          String url =
              "http://hl7.org/fhir/StructureDefinition/"
                  + resourceFieldName.substring(0, resourceFieldName.length() - 2);
          if (fieldReferences.contains(url)) {
            fields.add(field);
          }
        } else if (staticNames.contains(field.getName())) {
          fields.add(field);
        }
      }
      return fields;
    } else {
      return descriptor.getFields();
    }
  }

  private static String schemaTypeForPrimitive(Descriptor primitive) {
    if (primitive.getFullName().equals(Decimal.getDescriptor().getFullName())) {
      return "FLOAT";
    }
    FieldDescriptor valueField = primitive.findFieldByNumber(1);
    // If this is a timelike value field (value_us, for value_microseconds),
    // it will be rendered as a date string.
    if (valueField.getName().equals("value_us")) {
      return "STRING";
    }
    switch (valueField.getType()) {
      case BOOL:
        return "BOOLEAN";
      case BYTES:
        return "BYTES";
      case ENUM:
      case STRING:
        return "STRING";
      case INT64:
      case SINT32:
      case UINT32:
        return "INTEGER";
      default:
        throw new IllegalArgumentException("Unsupported field type " + valueField.getType());
    }
  }
}
