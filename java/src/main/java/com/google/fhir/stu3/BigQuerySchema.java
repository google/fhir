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
import com.google.fhir.stu3.proto.ContainedResource;
import com.google.fhir.stu3.proto.Extension;
import com.google.fhir.stu3.proto.Identifier;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.MessageOrBuilder;
import java.util.ArrayList;
import java.util.List;

/** A simple class to infer a BigQuery schema from protocol buffer messages. */
final class BigQuerySchema {

  /* Generate a schema for a specific FieldDescriptor, with an optional message instance. */
  private static TableFieldSchema fromFieldDescriptor(
      FieldDescriptor descriptor, MessageOrBuilder instance) {
    String fieldName =
        CaseFormat.LOWER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, descriptor.getJsonName());
    TableFieldSchema field =
        new TableFieldSchema()
            .setName(fieldName)
            .setMode(descriptor.isRepeated() ? "REPEATED" : "NULLABLE");
    if (descriptor.getType() != FieldDescriptor.Type.MESSAGE) {
      field.setType(schemaTypeForField(descriptor));
    } else {
      field.setType("RECORD");
      boolean hasData =
          instance != null
              && ((descriptor.isRepeated() && instance.getRepeatedFieldCount(descriptor) > 0)
                  || (!descriptor.isRepeated() && instance.hasField(descriptor)));
      if (hasData) {
        if (descriptor.isRepeated()) {
          TableSchema schema = new TableSchema();
          for (Object child : (List) instance.getField(descriptor)) {
            TableSchema oneSchema = fromMessage((MessageOrBuilder) child);
            // TODO(sundberg): implement mergeTableSchema properly, it currently uses a simple
            // heuristic.
            schema = mergeTableSchema(schema, oneSchema);
          }
          field.setFields(schema.getFields());
        } else {
          field.setFields(
              fromMessage((MessageOrBuilder) instance.getField(descriptor)).getFields());
        }
      } else {
        // We don't include extensions or contained resources unless they exist in the data.
        if (descriptor.getMessageType().equals(Extension.getDescriptor())
            || descriptor.getMessageType().equals(ContainedResource.getDescriptor())) {
          return null;
        }
        // We don't include fields inside extensions or contained resources unless they exist.
        if (descriptor.getContainingType().equals(Extension.getDescriptor())
            || descriptor.getContainingType().equals(Extension.Value.getDescriptor())
            || descriptor.getContainingType().equals(ContainedResource.getDescriptor())) {
          return null;
        }
        // We don't include the "id" field unless it exists, except for resources.
        if (fieldName.equals("id") && !AnnotationUtils.isResource(descriptor.getMessageType())) {
          return null;
        }
        // We don't include nested types unless they exist in the data.
        if (descriptor.getMessageType().equals(descriptor.getContainingType())) {
          return null;
        }
        // Identifier and Reference refer to each other. We stop the recursion by not including
        // Identifier.assigner unless it exists in the data.
        if (descriptor.getContainingType().equals(Identifier.getDescriptor())
            && fieldName.equals("assigner")) {
          return null;
        }
        field.setFields(fromDescriptor(descriptor.getMessageType()).getFields());
      }
    }
    return field;
  }

  /** Build a BigQuery schema for this message type, assuming extensions are not populated. */
  public static TableSchema fromDescriptor(Descriptor descriptor) {
    List<TableFieldSchema> fields = new ArrayList<>();
    for (FieldDescriptor field : descriptor.getFields()) {
      TableFieldSchema fieldSchema = fromFieldDescriptor(field, null);
      if (fieldSchema != null) {
        fields.add(fieldSchema);
      }
    }
    return new TableSchema().setFields(fields);
  }

  /**
   * Build a BigQuery schema for this message instance, including any extensions that are present in
   * the given message.
   */
  public static TableSchema fromMessage(MessageOrBuilder message) {
    List<TableFieldSchema> fields = new ArrayList<>();
    for (FieldDescriptor field : message.getDescriptorForType().getFields()) {
      TableFieldSchema fieldSchema = fromFieldDescriptor(field, message);
      if (fieldSchema != null) {
        fields.add(fieldSchema);
      }
    }
    return new TableSchema().setFields(fields);
  }

  private static TableSchema mergeTableSchema(TableSchema first, TableSchema second) {
    // TODO(sundberg): implement properly
    if (first.toString().length() > second.toString().length()) {
      return first;
    } else {
      return second;
    }
  }

  private static String schemaTypeForField(FieldDescriptor field) {
    // If this is a timelike value field, mark it as timestamp. Note that this is always in UTC,
    // independent of the timezone of the data.
    if (field.getName().equals("value_us")) {
      return "TIMESTAMP";
    }
    switch (field.getType()) {
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
        throw new IllegalArgumentException("Unsupported field type " + field.getType());
    }
  }
}
