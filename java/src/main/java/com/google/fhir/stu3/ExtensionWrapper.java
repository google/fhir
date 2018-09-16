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

import com.google.fhir.stu3.proto.Annotations;
import com.google.fhir.stu3.proto.Extension;
import com.google.fhir.stu3.proto.Uri;
import com.google.protobuf.DescriptorProtos.MessageOptions;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Message;
import com.google.protobuf.MessageOrBuilder;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

/** Helper methods for handling extensions. */
public final class ExtensionWrapper {
  private List<Extension> content;

  private ExtensionWrapper(List<Extension> content) {
    this.content = content;
  }

  /** Create an empty ExtensionWrapper. */
  public static ExtensionWrapper of() {
    return new ExtensionWrapper(new ArrayList<Extension>());
  }

  /** Create from a List of Extensions, making a copy. */
  public static ExtensionWrapper of(List<Extension> input) {
    return new ExtensionWrapper(new ArrayList<Extension>(input));
  }

  /** Create from the extension field in a message. */
  @SuppressWarnings("unchecked")
  public static ExtensionWrapper fromExtensionsIn(MessageOrBuilder input) {
    FieldDescriptor field = input.getDescriptorForType().findFieldByName("extension");
    if (field == null
        || !field.isRepeated()
        || field.getType() != FieldDescriptor.Type.MESSAGE
        || !field.getMessageType().equals(Extension.getDescriptor())) {
      throw new IllegalArgumentException(
          "Message type "
              + input.getDescriptorForType().getFullName()
              + " is not a valid FHIR type with extensions");
    }
    List<Extension> extensions = (List<Extension>) input.getField(field);
    return ExtensionWrapper.of(extensions);
  }

  /** Clear all extensions matching the template type from this. */
  public <T extends Message> ExtensionWrapper clearMatchingExtensions(T template) {
    if (content.isEmpty()) {
      return this;
    }
    validateFhirExtension(template);
    List<Extension> result = new ArrayList<>();
    String type =
        template
            .getDescriptorForType()
            .getOptions()
            .getExtension(Annotations.fhirStructureDefinitionUrl);
    for (Extension e : content) {
      if (!e.getUrl().getValue().equals(type)) {
        result.add(e);
      }
    }
    content = result;
    return this;
  }

  /** Return a version of the content suitable for inclusion in protocol messages. * */
  public List<Extension> build() {
    return content;
  }

  /**
   * Verify that the given message is a valid fhir extension, throwing an IllegalArgumentException
   * otherwise.
   */
  private void validateFhirExtension(MessageOrBuilder message) {
    MessageOptions options = message.getDescriptorForType().getOptions();
    // Note that this method checks proto extensions, which are different from FHIR extensions.
    String baseUrl = options.getExtension(Annotations.fhirProfileBase);
    // TODO(nickgeorge): This would reject profiles on profiles on extensions (and so on).
    // If we want to support that, we'll probably need a "fhir_is_extension" annotation,
    // or else load the structure definitions and walk back.
    if (!baseUrl.equals(
        Extension.getDescriptor()
            .getOptions()
            .getExtension(Annotations.fhirStructureDefinitionUrl))) {
      throw new IllegalArgumentException(
          "Message type "
              + message.getDescriptorForType().getFullName()
              + " is not a FHIR extension.  Base Profile: "
              + baseUrl);
    }
    if (!options.hasExtension(Annotations.fhirStructureDefinitionUrl)) {
      throw new IllegalArgumentException(
          "Message type "
              + message.getDescriptorForType().getFullName()
              + " is an invalid FHIR extension: Missing fhir_structure_definition_url annotation.");
    }
  }

  /** Add a new message, converting it to a FHIR Extension. */
  public <T extends MessageOrBuilder> ExtensionWrapper add(T message) {
    validateFhirExtension(message);
    Extension.Builder extension =
        Extension.newBuilder()
            .setUrl(
                Uri.newBuilder()
                    .setValue(
                        message
                            .getDescriptorForType()
                            .getOptions()
                            .getExtension(Annotations.fhirStructureDefinitionUrl)));
    List<FieldDescriptor> messageFields =
        message.getDescriptorForType().getFields().stream()
            .filter(field -> !field.getName().equals("extension") && !field.getName().equals("id"))
            .collect(Collectors.toList());
    // Copy the id field if present.
    FieldDescriptor idField = message.getDescriptorForType().findFieldByName("id");
    if (idField != null && message.hasField(idField)) {
      extension.setId((com.google.fhir.stu3.proto.String) message.getField(idField));
    }
    boolean isSingleValueExtension =
        messageFields.size() == 1
            && !messageFields.get(0).isRepeated()
            && messageFields.get(0).getType() == FieldDescriptor.Type.MESSAGE
            && AnnotationUtils.isPrimitiveType(messageFields.get(0).getMessageType());
    if (isSingleValueExtension) {
      if (message.hasField(messageFields.get(0))) {
        addValueToExtension((MessageOrBuilder) message.getField(messageFields.get(0)), extension);
      }
    } else {
      addMessageToExtension(message, extension);
    }
    content.add(extension.build());
    return this;
  }

  /**
   * Return a list of all extensions which match the type of the provided template, in their
   * protobuf representation.
   */
  @SuppressWarnings("unchecked")
  public <T extends Message> List<T> getMatchingExtensions(T template) {
    validateFhirExtension(template);
    if (content.isEmpty()) {
      return Collections.<T>emptyList();
    }
    List<T> result = new ArrayList<T>();
    String type =
        template
            .getDescriptorForType()
            .getOptions()
            .getExtension(Annotations.fhirStructureDefinitionUrl);
    for (Extension e : content) {
      if (e.getUrl().getValue().equals(type)) {
        Message.Builder builder = template.newBuilderForType();
        addExtensionToMessage(e, builder);
        result.add((T) builder.build());
      }
    }
    return result;
  }

  // Internal implementation details from here on.

  private static final List<FieldDescriptor> EXTENSION_VALUE_FIELDS =
      Extension.Value.getDescriptor().getOneofs().get(0).getFields();

  private static void addValueToExtension(MessageOrBuilder value, Extension.Builder result) {
    // TODO(sundberg): use a map
    for (FieldDescriptor field : EXTENSION_VALUE_FIELDS) {
      if (field.getMessageType().equals(value.getDescriptorForType())) {
        result.setValue(Extension.Value.newBuilder().setField(field, value).build());
        return;
      }
    }
    // Fall back to adding the value as a message.
    addMessageToExtension(value, result);
  }

  private static void addFieldToExtension(
      String fieldName, MessageOrBuilder fieldValue, Extension.Builder result) {
    Extension.Builder subBuilder = Extension.newBuilder();
    subBuilder.setUrl(Uri.newBuilder().setValue(fieldName));
    addValueToExtension(fieldValue, subBuilder);
    result.addExtension(subBuilder);
  }

  private static void addMessageToExtension(MessageOrBuilder message, Extension.Builder result) {
    for (Map.Entry<FieldDescriptor, Object> entry : message.getAllFields().entrySet()) {
      if (entry.getKey().isRepeated()) {
        for (Object o : (List) entry.getValue()) {
          addFieldToExtension(entry.getKey().getName(), (MessageOrBuilder) o, result);
        }
      } else {
        addFieldToExtension(entry.getKey().getName(), (MessageOrBuilder) entry.getValue(), result);
      }
    }
  }

  // TODO(nickgeorge): This should handle the extension fields.
  private static void addExtensionToMessage(Extension extension, Message.Builder builder) {
    // Copy the id field if present.
    if (extension.hasId()) {
      FieldDescriptor idField = builder.getDescriptorForType().findFieldByName("id");
      // TODO(sundberg): handle copying the id field for all kinds of extensions.
      if (idField != null) {
        builder.setField(idField, extension.getId());
      }
    }

    if (extension.hasValue()) {
      // We only hit this case for simple extensions. The output type had better have just one
      // field other than extension and id, and it had better be of the right type.
      List<FieldDescriptor> fields =
          builder.getDescriptorForType().getFields().stream()
              .filter(
                  field -> !field.getName().equals("extension") && !field.getName().equals("id"))
              .collect(Collectors.toList());
      if (fields.size() == 1 && fields.get(0).getType() == FieldDescriptor.Type.MESSAGE) {
        FieldDescriptor targetField = fields.get(0);
        for (Map.Entry<FieldDescriptor, Object> entry :
            extension.getValue().getAllFields().entrySet()) {
          if (entry.getKey().getContainingOneof() != null) {
            if (entry.getKey().getMessageType().equals(targetField.getMessageType())) {
              builder.setField(targetField, entry.getValue());
              return;
            } else {
              throw new IllegalArgumentException(
                  "Unable to find field of type "
                      + entry.getKey().getMessageType().getName()
                      + " in "
                      + builder.getDescriptorForType().getFullName());
            }
          }
        }
      }
      throw new IllegalArgumentException(
          "Invalid extension proto " + builder.getDescriptorForType().getFullName());

    } else {
      Descriptor descriptor = builder.getDescriptorForType();
      for (Extension inner : extension.getExtensionList()) {
        String fieldName = inner.getUrl().getValue();
        FieldDescriptor field = descriptor.findFieldByName(fieldName);
        if (field == null) {
          throw new IllegalArgumentException(
              "Message "
                  + builder.getDescriptorForType().getFullName()
                  + " has no field named "
                  + fieldName);
        }
        if (field.getType() != FieldDescriptor.Type.MESSAGE) {
          throw new IllegalArgumentException(
              "Field "
                  + fieldName
                  + " in Message "
                  + builder.getDescriptorForType().getFullName()
                  + " is of invalid type");
        }
        Message.Builder subBuilder = builder.newBuilderForField(field);
        if (inner.hasValue()) {
          // TODO(sundberg): handle ids on inner extensions
          if (inner.getExtensionCount() > 0) {
            throw new IllegalArgumentException(
                "Extension holds both a value and sub-extensions: " + inner);
          }
          addValueToMessage(inner.getValue(), subBuilder);
        } else {
          addExtensionToMessage(inner, subBuilder);
        }
        if (field.isRepeated()) {
          builder.addRepeatedField(field, subBuilder.build());
        } else {
          builder.setField(field, subBuilder.build());
        }
      }
    }
  }

  private static void addValueToMessage(Extension.Value value, Message.Builder builder) {
    for (Map.Entry<FieldDescriptor, Object> field : value.getAllFields().entrySet()) {
      if (field.getKey().getContainingOneof() != null) {
        builder.mergeFrom((Message) field.getValue());
      }
    }
  }
}
