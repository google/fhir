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
import com.google.fhir.stu3.proto.ContainedResource;
import com.google.fhir.stu3.proto.Decimal;
import com.google.fhir.stu3.proto.Extension;
import com.google.fhir.stu3.proto.Identifier;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import java.util.ArrayList;
import java.util.List;

/** A simple class to infer a BigQuery schema from protocol buffer messages. */
public final class BigQuerySchema {

  /* Generate a schema for a specific FieldDescriptor, with an optional message instance. */
  private static TableFieldSchema fromFieldDescriptor(FieldDescriptor descriptor) {
    // We directly use the jsonName of the field when generating the schema.
    String fieldName = descriptor.getJsonName();
    TableFieldSchema field =
        new TableFieldSchema()
            .setName(fieldName)
            .setMode(descriptor.isRepeated() ? "REPEATED" : "NULLABLE");
    if (descriptor.getType() != FieldDescriptor.Type.MESSAGE) {
      throw new IllegalStateException("Unexpected primitive field: " + descriptor.getFullName());
    }
    Descriptor fieldType = descriptor.getMessageType();
    // We don't include the "id" except for resources.
    if (fieldName.equals("id") && !AnnotationUtils.isResource(descriptor.getContainingType())) {
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
    if (fieldType.equals(descriptor.getContainingType())) {
      return null;
    }
    // Identifier and Reference refer to each other. We stop the recursion by not including
    // Identifier.assigner unless it exists in the data.
    if (descriptor.getContainingType().equals(Identifier.getDescriptor())
        && fieldName.equals("assigner")) {
      return null;
    }
    field.setType("RECORD");
    return field.setFields(fromDescriptor(fieldType).getFields());
  }

  /** Build a BigQuery schema for this message type when serialized to analytic JSON. */
  public static TableSchema fromDescriptor(Descriptor descriptor) {
    List<TableFieldSchema> fields = new ArrayList<>();
    for (FieldDescriptor field : descriptor.getFields()) {
      TableFieldSchema fieldSchema = fromFieldDescriptor(field);
      if (fieldSchema != null) {
        fields.add(fieldSchema);
      }
    }
    return new TableSchema().setFields(fields);
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
