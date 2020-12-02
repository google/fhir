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

import com.google.common.base.CaseFormat;
import com.google.common.base.Splitter;
import com.google.fhir.r4.core.Bundle;
import com.google.fhir.r4.core.Id;
import com.google.fhir.r4.core.ReferenceId;
import com.google.fhir.wrappers.IdWrapper;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Message;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Helper methods for handling FHIR resource protos. */
public final class ResourceUtils {

  public static String getResourceType(Message message) {
    if (!AnnotationUtils.isResource(message)) {
      throw new IllegalArgumentException(
          "Message type " + message.getDescriptorForType().getFullName() + " is not a resource.");
    }
    return message.getDescriptorForType().getName();
  }

  public static String getResourceId(Message message) {
    FieldDescriptor id = message.getDescriptorForType().findFieldByName("id");
    if (!AnnotationUtils.isResource(message) || id == null) {
      throw new IllegalArgumentException(
          "Message type " + message.getDescriptorForType().getFullName() + " is not a resource.");
    }
    if (!message.hasField(id)) {
      return null;
    }
    return ((Id) message.getField(id)).getValue();
  }

  private static Message getContainedResourceInternal(Message resource) {
    Collection<Object> values = resource.getAllFields().values();
    if (values.size() == 1) {
      return (Message) values.iterator().next();
    } else {
      return null;
    }
  }

  public static Message getContainedResource(
      com.google.fhir.stu3.proto.ContainedResource resource) {
    return getContainedResourceInternal(resource);
  }

  public static Message getContainedResource(com.google.fhir.r4.core.ContainedResource resource) {
    return getContainedResourceInternal(resource);
  }

  @SuppressWarnings("unchecked")
  public static <V> V getValue(Message primitive) {
    return (V) primitive.getField(primitive.getDescriptorForType().findFieldByName("value"));
  }

  /*
   * Convert any absolute references in the provided bundle to relative references, assuming the
   * targets of the references are also present in the bundle.
   */
  public static Bundle resolveBundleReferences(Bundle bundle) {
    Map<String, String> referenceMap = new HashMap<>();
    for (Bundle.Entry entry : bundle.getEntryList()) {
      if (entry.hasFullUrl()) {
        Message resource = ResourceUtils.getContainedResource(entry.getResource());
        String resourceType = ResourceUtils.getResourceType(resource);
        String resourceId = ResourceUtils.getResourceId(resource);
        String relativeReference = resourceType + "/" + resourceId;
        referenceMap.put(entry.getFullUrl().getValue(), relativeReference);
      }
    }
    return (Bundle) replaceReferences(bundle, referenceMap);
  }

  private static Message replaceOneReference(Message message, Map<String, String> referenceMap) {
    FieldDescriptor uri = message.getDescriptorForType().findFieldByName("uri");
    if (!message.hasField(uri)) {
      return message;
    }
    String oldValue = ((com.google.fhir.r4.core.String) message.getField(uri)).getValue();
    if (!referenceMap.containsKey(oldValue)) {
      return message;
    }
    Message.Builder builder = message.toBuilder();
    String newValue = referenceMap.get(oldValue);
    builder.setField(uri, com.google.fhir.r4.core.String.newBuilder().setValue(newValue).build());
    return splitIfRelativeReference(builder);
  }

  @SuppressWarnings("unchecked")
  private static Message replaceReferences(Message message, Map<String, String> referenceMap) {
    Message.Builder builder = null;
    for (Map.Entry<FieldDescriptor, Object> field : message.getAllFields().entrySet()) {
      if (field.getKey().getType() == FieldDescriptor.Type.MESSAGE) {
        Object newValue = field.getValue();
        if (field.getKey().isRepeated()) {
          List<Message> newList = new ArrayList<>();
          for (Message item : (List<Message>) field.getValue()) {
            Message newItem;
            if (AnnotationUtils.isReference(item)) {
              newItem = replaceOneReference(item, referenceMap);
            } else {
              newItem = replaceReferences(item, referenceMap);
            }
            if (newItem != item) {
              newValue = newList;
            }
            newList.add(newItem);
          }
        } else {
          Message value = (Message) field.getValue();
          if (AnnotationUtils.isReference(field.getKey().getMessageType())) {
            newValue = replaceOneReference(value, referenceMap);
          } else {
            newValue = replaceReferences(value, referenceMap);
          }
        }
        if (newValue != field.getValue()) {
          if (builder == null) {
            builder = message.toBuilder();
          }
          builder.setField(field.getKey(), newValue);
        }
      }
    }
    if (builder == null) {
      return message;
    } else {
      return builder.build();
    }
  }

  /*
   * Split relative references into their components, for example, "Patient/ABCD" will result in
   * the patientId field getting the value "ABCD".
   */
  public static Message splitIfRelativeReference(Message.Builder builder) {
    Descriptor descriptor = builder.getDescriptorForType();
    FieldDescriptor uriField = descriptor.findFieldByName("uri");
    if (!builder.hasField(uriField)) {
      return builder.build();
    }
    Message uri = (Message) builder.getField(uriField);
    String uriValue = (String) uri.getField(uri.getDescriptorForType().findFieldByName("value"));
    if (uriValue.startsWith("#")) {
      FieldDescriptor fragmentField = descriptor.findFieldByName("fragment");
      Message.Builder fragmentBuilder = builder.getFieldBuilder(fragmentField);
      return ProtoUtils.fieldWiseCopy(
              com.google.fhir.r4.core.String.newBuilder()
                  .setValue(new IdWrapper(uriValue.substring(1)).getWrapped().getValue()),
              fragmentBuilder)
          .build();
    }
    // Look for references of type "ResourceType/ResourceId"
    List<String> parts = Splitter.on('/').splitToList(uriValue);
    if (parts.size() == 2 || (parts.size() == 4 && "_history".equals(parts.get(2)))) {
      String resourceFieldName =
          CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, parts.get(0)) + "_id";
      FieldDescriptor field = descriptor.findFieldByName(resourceFieldName);
      if (field == null) {
        throw new IllegalArgumentException(
            "Invalid resource type in reference: " + resourceFieldName + ":" + parts.get(0));
      } else {
        ReferenceId.Builder refId =
            ReferenceId.newBuilder().setValue(new IdWrapper(parts.get(1)).getWrapped().getValue());
        if (parts.size() == 4) {
          refId.setHistory(new IdWrapper(parts.get(3)).getWrapped());
        }
        ProtoUtils.fieldWiseCopy(refId, builder.getFieldBuilder(field));
        return builder.build();
      }
    }
    // Keep the uri field.
    return builder.build();
  }
}
