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

import com.google.api.services.bigquery.model.TableFieldSchema;
import com.google.api.services.bigquery.model.TableSchema;
import com.google.common.base.CaseFormat;
import com.google.common.collect.ImmutableSet;
import com.google.fhir.proto.Annotations;
import com.google.fhir.r4.core.Decimal;
import com.google.fhir.r4.core.Extension;
import com.google.protobuf.Any;
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

    // We inline extensions and contained resources as simple strings describing the url or type,
    // respectively, to keep a well-structured schema.
    if (AnnotationUtils.sameFhirType(fieldType, Extension.getDescriptor())
        || fieldType.getName().equals("ContainedResource")
        || fieldType.getFullName().equals(Any.getDescriptor().getFullName())) {
      return field.setType("STRING");
    }
    // We don't include nested types.
    // TODO: consider allowing a certain level of nesting.
    if (fieldType.equals(fieldDescriptor.getContainingType())) {
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
      if (fieldReferences.size() == 1 && fieldReferences.iterator().next().equals("Resource")) {
        return descriptor.getFields();
      }

      // Accept the generic types (uri, fragment) as well as the specified ones.
      List<FieldDescriptor> fields = new ArrayList<>();
      // TODO: We're dropping "identifier" to avoid an infinite recursion where
      // identifiers have references and vice versa.  We should dig in to if there is a better
      // solution.
      Set<String> staticNames = ImmutableSet.of("uri", "fragment", "extension", "display");
      for (FieldDescriptor field : descriptor.getFields()) {
        String resourceFieldName =
            CaseFormat.LOWER_UNDERSCORE.to(CaseFormat.UPPER_CAMEL, field.getName());
        String fieldType = resourceFieldName.substring(0, resourceFieldName.length() - 2);
        if (resourceFieldName.endsWith("Id")) {
          if (fieldReferences.contains(fieldType)) {
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
    if (AnnotationUtils.sameFhirType(primitive, Decimal.getDescriptor())) {
      return "FLOAT";
    }
    FieldDescriptor valueField = primitive.findFieldByNumber(1);
    if (valueField == null) {
      // In non-enumberable CodeSystem/Valuesets, field 1 is reserved, and the value field is
      // string.  Check that this matches, and then return String.
      if (primitive.findFieldByName("value").getType() == FieldDescriptor.Type.STRING) {
        return "STRING";
      } else {
        throw new IllegalArgumentException("Unrecognized primitive: " + primitive.getFullName());
      }
    }
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
