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

import static com.google.fhir.common.ProtoUtils.findField;

import com.google.common.base.CaseFormat;
import com.google.common.base.Splitter;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Message;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Helper methods for handling FHIR resource protos. */
// TODO(b/177246116): Break reference utils into standalone References.java
public final class ResourceUtils {

  private ResourceUtils() {}

  public static Message getContainedResource(Message resource) throws InvalidFhirException {
    // TODO(b/154059162): Use an annotation here.
    if (!resource.getDescriptorForType().getName().equals("ContainedResource")) {
      throw new InvalidFhirException(
          "Not a ContainedResource: " + resource.getDescriptorForType().getName());
    }
    Collection<Object> values = resource.getAllFields().values();
    if (values.size() != 1) {
      throw new InvalidFhirException("ContainedResource has no resource field set.");
    }
    return (Message) values.iterator().next();
  }

  /**
   * Convert any absolute references in the provided bundle to relative references, assuming the
   * targets of the references are also present in the bundle.
   */
  @SuppressWarnings("unchecked") // Cast to List<Message> guaranteed by preceding inspection
  public static void resolveBundleReferences(Message.Builder bundle) throws InvalidFhirException {
    Map<String, String> referenceMap = new HashMap<>();
    FieldDescriptor entryField = findField(bundle, "entry");
    FieldDescriptor entryFullUrlField = findField(entryField.getMessageType(), "full_url");
    FieldDescriptor entryResourceField = findField(entryField.getMessageType(), "resource");

    if (!entryField.isRepeated() || entryField.getType() != FieldDescriptor.Type.MESSAGE) {
      throw new InvalidFhirException("Unexpected Bundle entry field: " + entryField.getFullName());
    }
    for (Message entry : (List<Message>) bundle.getField(entryField)) {
      if (entry.hasField(entryFullUrlField) && entry.hasField(entryResourceField)) {
        Message resource =
            ResourceUtils.getContainedResource((Message) entry.getField(entryResourceField));
        String id = (String) getValue((Message) resource.getField(findField(resource, "id")));
        String relativeReference = resource.getDescriptorForType().getName() + "/" + id;
        referenceMap.put(
            (String) getValue((Message) entry.getField(entryFullUrlField)), relativeReference);
      }
    }
    replaceReferences(bundle, referenceMap);
  }

  private static void replaceOneReference(Message.Builder builder, Map<String, String> referenceMap)
      throws InvalidFhirException {
    FieldDescriptor uriField = builder.getDescriptorForType().findFieldByName("uri");
    FieldDescriptor uriValueField = uriField.getMessageType().findFieldByName("value");
    if (!builder.hasField(uriField)) {
      return;
    }
    String oldValue = (String) ((Message) builder.getField(uriField)).getField(uriValueField);
    if (!referenceMap.containsKey(oldValue)) {
      return;
    }
    String newValue = referenceMap.get(oldValue);
    builder.getFieldBuilder(uriField).setField(uriValueField, newValue);
    splitIfRelativeReference(builder);
  }

  private static void replaceReferences(Message.Builder builder, Map<String, String> referenceMap)
      throws InvalidFhirException {
    for (FieldDescriptor field : builder.getAllFields().keySet()) {
      if (field.getType() == FieldDescriptor.Type.MESSAGE) {
        for (int i = 0; i < ProtoUtils.fieldSize(builder, field); i++) {
          Message.Builder itemBuilder = ProtoUtils.getBuilderAtIndex(builder, field, i);
          if (AnnotationUtils.isReference(itemBuilder)) {
            replaceOneReference(itemBuilder, referenceMap);
          } else {
            replaceReferences(itemBuilder, referenceMap);
          }
        }
      }
    }
  }

  private static Message.Builder setStringField(
      Message.Builder builder, String field, String idString) throws InvalidFhirException {
    Message.Builder idBuilder = builder.getFieldBuilder(findField(builder, field));
    idBuilder.setField(findField(idBuilder, "value"), idString);
    return builder;
  }

  /**
   * Given a reference that uses the generic {@code uri} field, splits the reference URI into
   * component fields. For example,
   *
   * <ol>
   *   <li>"Patient/ABCD" would be split into the {@code patientId} field getting the value "ABCD".
   *   <li>"Patient/ABCD/_history/1234" would also populate the {@code history} field with "1234".
   *   <li>A fragment, such as "#my-fragment", results in the {@code fragment} field being populated
   *       with "my-fragment"
   * </ol>
   *
   * If the reference URI matches one of these schemas, the {@code uri} field will be cleared, and
   * the appropriate structured fields set. If it does not match any of these schemas, the reference
   * will be unchanged, and an OK status will be returned.
   *
   * <p>For more information on the form of uri references, see {@link
   * https://www.hl7.org/fhir/references.html#Reference}.
   */
  // TODO(b/177249530): Ensure consistent behavior/testing with python/c++.
  public static void splitIfRelativeReference(Message.Builder builder) throws InvalidFhirException {
    Descriptor descriptor = builder.getDescriptorForType();
    FieldDescriptor uriField = descriptor.findFieldByName("uri");
    if (!builder.hasField(uriField)) {
      return;
    }
    Message uri = (Message) builder.getField(uriField);
    String uriValue = (String) uri.getField(findField(uri, "value"));
    if (uriValue.startsWith("#")) {
      Message.Builder fragmentBuilder = builder.getFieldBuilder(findField(builder, "fragment"));
      fragmentBuilder.setField(findField(fragmentBuilder, "value"), uriValue.substring(1));
      return;
    }
    // Look for references of type "ResourceType/ResourceId"
    List<String> parts = Splitter.on('/').splitToList(uriValue);
    if (parts.size() == 2 || (parts.size() == 4 && "_history".equals(parts.get(2)))) {
      String resourceFieldName =
          CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, parts.get(0)) + "_id";
      FieldDescriptor field = descriptor.findFieldByName(resourceFieldName);
      if (field == null) {
        throw new InvalidFhirException(
            "Invalid resource type in reference: " + resourceFieldName + ":" + parts.get(0));
      } else {
        Message.Builder referenceIdBuilder = builder.getFieldBuilder(field);
        referenceIdBuilder.setField(findField(referenceIdBuilder, "value"), parts.get(1));
        if (parts.size() == 4) {
          setStringField(referenceIdBuilder, "history", parts.get(3));
        }
      }
    }
  }

  // TODO(b/176651098): Replace with utility that throws a checked exception
  public static Object getValue(Message primitive) {
    return primitive.getField(primitive.getDescriptorForType().findFieldByName("value"));
  }
}
