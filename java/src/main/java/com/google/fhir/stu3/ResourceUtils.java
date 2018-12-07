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
//    limitations under the License.package com.google.research.health.fhir.stu3;

package com.google.fhir.stu3;

import com.google.common.base.CaseFormat;
import com.google.common.base.Splitter;
import com.google.fhir.stu3.proto.Bundle;
import com.google.fhir.stu3.proto.ContainedResource;
import com.google.fhir.stu3.proto.Id;
import com.google.fhir.stu3.proto.ReferenceId;
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

  public static Message getContainedResource(ContainedResource resource) {
    Collection<Object> values = resource.getAllFields().values();
    if (values.size() == 1) {
      return (Message) values.iterator().next();
    } else {
      return null;
    }
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
    String oldValue = ((com.google.fhir.stu3.proto.String) message.getField(uri)).getValue();
    if (!referenceMap.containsKey(oldValue)) {
      return message;
    }
    Message.Builder builder = message.toBuilder();
    String newValue = referenceMap.get(oldValue);
    builder.setField(
        uri, com.google.fhir.stu3.proto.String.newBuilder().setValue(newValue).build());
    return splitIfRelativeReference(builder);
  }

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
    FieldDescriptor uri = builder.getDescriptorForType().findFieldByName("uri");
    if (!builder.hasField(uri)) {
      return builder.build();
    }
    String string = ((com.google.fhir.stu3.proto.String) builder.getField(uri)).getValue();
    if (string.startsWith("#")) {
      FieldDescriptor fragment = builder.getDescriptorForType().findFieldByName("fragment");
      return builder
          .setField(
              fragment,
              com.google.fhir.stu3.proto.String.newBuilder()
                  .setValue(new IdWrapper(string.substring(1)).getWrapped().getValue())
                  .build())
          .build();
    }
    // Look for references of type "ResourceType/ResourceId"
    List<String> parts = Splitter.on('/').splitToList(string);
    if (parts.size() == 2 || (parts.size() == 4 && "_history".equals(parts.get(2)))) {
      String resourceFieldName =
          CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, parts.get(0)) + "_id";
      FieldDescriptor field = builder.getDescriptorForType().findFieldByName(resourceFieldName);
      if (field == null) {
        throw new IllegalArgumentException(
            "Invalid resource type in reference: " + resourceFieldName + ":" + parts.get(0));
      } else {
        // Parse as an Id to ensure validation.
        ReferenceId.Builder refId =
            ReferenceId.newBuilder().setValue(new IdWrapper(parts.get(1)).getWrapped().getValue());
        if (parts.size() == 4) {
          refId.setHistory(new IdWrapper(parts.get(3)).getWrapped());
        }
        return builder.setField(field, refId.build()).build();
      }
    }
    // Keep the uri field.
    return builder.build();
  }
}
