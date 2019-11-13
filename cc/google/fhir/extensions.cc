// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "google/fhir/extensions.h"

#include <unordered_map>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codes.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/platform/logging.h"

namespace google {
namespace fhir {

using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using ::tensorflow::errors::InvalidArgument;

namespace {

const std::unordered_map<std::string, const FieldDescriptor*>*
GetExtensionValueFieldsMap() {
  static const std::unordered_map<std::string, const FieldDescriptor*>*
      extension_value_fields_by_type = []() {
        std::unordered_map<std::string, const FieldDescriptor*>* map =
            new std::unordered_map<std::string, const FieldDescriptor*>;

        const google::protobuf::OneofDescriptor* stu3_value_oneof =
            stu3::proto::Extension::Value::descriptor()->FindOneofByName(
                "value");
        CHECK(stu3_value_oneof != nullptr);
        for (int i = 0; i < stu3_value_oneof->field_count(); i++) {
          const FieldDescriptor* field = stu3_value_oneof->field(i);
          (*map)[field->message_type()->full_name()] = field;
        }
        const google::protobuf::OneofDescriptor* r4_value_oneof =
            r4::core::Extension::Value::descriptor()->FindOneofByName("value");
        CHECK(r4_value_oneof != nullptr);
        for (int i = 0; i < r4_value_oneof->field_count(); i++) {
          const FieldDescriptor* field = r4_value_oneof->field(i);
          (*map)[field->message_type()->full_name()] = field;
        }
        return map;
      }();
  return extension_value_fields_by_type;
}

Status AddFieldsToExtension(const Message& message,
                            ::google::fhir::stu3::proto::Extension* extension);
Status AddFieldsToExtension(const Message& message,
                            ::google::fhir::r4::core::Extension* extension);

Status CheckIsMessage(const FieldDescriptor* field) {
  if (field->type() != FieldDescriptor::Type::TYPE_MESSAGE) {
    return InvalidArgument("Encountered unexpected proto primitive: ",
                           field->full_name(), ".  Should be FHIR type");
  }
  return Status::OK();
}

// Given a datatype message (E.g., String, Code, Boolean, etc.),
// finds the appropriate field on the target extension and sets it.
// Returns InvalidArgument if there's no matching oneof type on the extension
// for the message.
template <class ExtensionLike>
Status SetDatatypeOnExtensionInternal(const Message& message,
                                      ExtensionLike* extension) {
  const Descriptor* descriptor = message.GetDescriptor();
  auto value_field_iter =
      GetExtensionValueFieldsMap()->find(descriptor->full_name());
  if (value_field_iter != GetExtensionValueFieldsMap()->end()) {
    extension->value()
        .GetReflection()
        ->MutableMessage(extension->mutable_value(), value_field_iter->second)
        ->CopyFrom(message);
    return Status::OK();
  }
  if (HasValueset(message.GetDescriptor())) {
    // The source message is a bound code type.
    // Convert it to a generic code, and add it to the extension.
    return ConvertToGenericCode(message,
                                extension->mutable_value()->mutable_code());
  }
  if (IsTypeOrProfileOfCoding(message)) {
    // The source message is a bound coding type.
    // Convert it to a generic coding, and add it to the extension.
    return ConvertToGenericCoding(message,
                                  extension->mutable_value()->mutable_coding());
  }
  return InvalidArgument(descriptor->full_name(),
                         " is not a valid value type on Extension.");
}

template <class ExtensionLike>
Status AddValueToExtensionInternal(const Message& message,
                                   ExtensionLike* extension,
                                   bool is_choice_type) {
  const Descriptor* descriptor = message.GetDescriptor();

  if (is_choice_type) {
    const google::protobuf::OneofDescriptor* oneof = descriptor->oneof_decl(0);
    if (oneof == nullptr) {
      return InvalidArgument("Choice type is missing a oneof: ",
                             descriptor->full_name());
    }
    const Reflection* message_reflection = message.GetReflection();
    const FieldDescriptor* value_field =
        message_reflection->GetOneofFieldDescriptor(message, oneof);
    if (value_field == nullptr) {
      return InvalidArgument("Choice type has no value set: ",
                             descriptor->full_name());
    }
    FHIR_RETURN_IF_ERROR(CheckIsMessage(value_field));
    return AddValueToExtensionInternal(
        message_reflection->GetMessage(message, value_field), extension, false);
  }
  // Try to set the message directly as a datatype value on the extension.
  // E.g., put message of type boolean into the value.boolean field
  if (SetDatatypeOnExtensionInternal(message, extension).ok()) {
    return Status::OK();
  }
  // Fall back to adding individual fields as sub-extensions.
  return AddFieldsToExtension(message, extension);
}

}  // namespace

Status SetDatatypeOnExtension(const ::google::protobuf::Message& message,
                              stu3::proto::Extension* extension) {
  return SetDatatypeOnExtensionInternal(message, extension);
}

Status SetDatatypeOnExtension(const ::google::protobuf::Message& message,
                              r4::core::Extension* extension) {
  return SetDatatypeOnExtensionInternal(message, extension);
}

Status AddValueToExtension(const Message& message,
                           ::google::fhir::stu3::proto::Extension* extension,
                           bool is_choice_type) {
  return AddValueToExtensionInternal<::google::fhir::stu3::proto::Extension>(
      message, extension, is_choice_type);
}

Status AddValueToExtension(const Message& message,
                           ::google::fhir::r4::core::Extension* extension,
                           bool is_choice_type) {
  return AddValueToExtensionInternal<::google::fhir::r4::core::Extension>(
      message, extension, is_choice_type);
}

namespace {

template <class ExtensionLike>
Status AddFieldsToExtensionInternal(const Message& message,
                                    ExtensionLike* extension) {
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* reflection = message.GetReflection();
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  for (const auto* field : fields) {
    if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
      return InvalidArgument(descriptor->full_name(),
                             " is not a FHIR extension type");
    }
    // Add submessages to nested extensions.
    if (field->is_repeated()) {
      for (int j = 0; j < reflection->FieldSize(message, field); j++) {
        ExtensionLike* child = extension->add_extension();
        child->mutable_url()->set_value(GetInlinedExtensionUrl(field));
        FHIR_RETURN_IF_ERROR(CheckIsMessage(field));
        FHIR_RETURN_IF_ERROR(AddValueToExtensionInternal(
            reflection->GetRepeatedMessage(message, field, j), child,
            IsChoiceType(field)));
      }
    } else {
      ExtensionLike* child = extension->add_extension();
      child->mutable_url()->set_value(GetInlinedExtensionUrl(field));
      FHIR_RETURN_IF_ERROR(CheckIsMessage(field));
      FHIR_RETURN_IF_ERROR(AddValueToExtensionInternal(
          reflection->GetMessage(message, field), child, IsChoiceType(field)));
    }
  }
  return Status::OK();
}

Status AddFieldsToExtension(const Message& message,
                            ::google::fhir::stu3::proto::Extension* extension) {
  return AddFieldsToExtensionInternal(message, extension);
}
Status AddFieldsToExtension(const Message& message,
                            ::google::fhir::r4::core::Extension* extension) {
  return AddFieldsToExtensionInternal(message, extension);
}

template <class ExtensionLike>
StatusOr<const FieldDescriptor*> GetExtensionValueField(
    const ExtensionLike& extension) {
  static const google::protobuf::OneofDescriptor* value_oneof =
      ExtensionLike::Value::descriptor()->FindOneofByName("value");
  const auto& value = extension.value();
  const Reflection* value_reflection = value.GetReflection();
  const FieldDescriptor* field =
      value_reflection->GetOneofFieldDescriptor(value, value_oneof);
  if (field == nullptr) {
    return InvalidArgument("No value set on extension.");
  }
  FHIR_RETURN_IF_ERROR(CheckIsMessage(field));
  return field;
}

template <class ExtensionLike>
Status ValueToMessageInternal(const ExtensionLike& extension, Message* message,
                              const FieldDescriptor* field) {
  const Descriptor* descriptor = message->GetDescriptor();
  if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    return InvalidArgument(descriptor->full_name(),
                           " is not a FHIR extension type");
  }
  // If there's a value, there can not be any extensions set.
  if (extension.extension_size() > 0) {
    return InvalidArgument("Invalid extension: ", extension.DebugString());
  }
  FHIR_ASSIGN_OR_RETURN(const FieldDescriptor* value_field,
                        GetExtensionValueField(extension));

  const Reflection* message_reflection = message->GetReflection();
  if (IsChoiceType(field)) {
    // We need to assign the value from the extension to the correct field
    // on the choice type.
    Message* choice_message =
        message_reflection->MutableMessage(message, field);
    const Descriptor* choice_descriptor = choice_message->GetDescriptor();
    for (int i = 0; i < choice_descriptor->field_count(); i++) {
      const FieldDescriptor* choice_field = choice_descriptor->field(i);
      if (value_field->message_type()->full_name() ==
          choice_field->message_type()->full_name()) {
        return ValueToMessageInternal(extension, choice_message, choice_field);
      }
    }
    return InvalidArgument("No field on Choice Type ",
                           choice_descriptor->full_name(), " for extension ",
                           extension.DebugString());
  }

  if (HasValueset(field->message_type())) {
    // The target message is a bound code type.  Convert the generic code
    // field from the extension into the target typed code.
    return ConvertToTypedCode(extension.value().code(),
                              MutableOrAddMessage(message, field));
  }

  if (IsTypeOrProfileOfCoding(field->message_type())) {
    // The target message is a bound codng type.  Convert the generic codng
    // field from the extension into the target typed coding.
    return ConvertToTypedCoding(extension.value().coding(),
                                MutableOrAddMessage(message, field));
  }

  // Value types must match.
  if (!AreSameMessageType(value_field->message_type(), field->message_type())) {
    return InvalidArgument("Missing expected value of type ",
                           field->message_type()->full_name(), " in extension ",
                           extension.GetDescriptor()->full_name());
  }
  const auto& value = extension.value();
  const Reflection* value_reflection = value.GetReflection();
  MutableOrAddMessage(message, field)
      ->CopyFrom(value_reflection->GetMessage(value, value_field));
  return Status::OK();
}

}  // namespace

Status ValueToMessage(const ::google::fhir::stu3::proto::Extension& extension,
                      Message* message, const FieldDescriptor* field) {
  return ValueToMessageInternal(extension, message, field);
}

Status ValueToMessage(const ::google::fhir::r4::core::Extension& extension,
                      Message* message, const FieldDescriptor* field) {
  return ValueToMessageInternal(extension, message, field);
}

const std::vector<const FieldDescriptor*> FindValueFields(
    const Descriptor* descriptor) {
  std::vector<const FieldDescriptor*> value_fields;
  for (int i = 0; i < descriptor->field_count(); i++) {
    const std::string& name = descriptor->field(i)->name();
    if (name != "id" && name != "extension") {
      value_fields.push_back(descriptor->field(i));
    }
  }
  return value_fields;
}

Status ValidateExtension(const Descriptor* descriptor) {
  if (!IsProfileOf<stu3::proto::Extension>(descriptor)) {
    return InvalidArgument(descriptor->full_name(),
                           " is not a FHIR extension type");
  }
  if (!descriptor->options().HasExtension(
          ::google::fhir::proto::fhir_structure_definition_url)) {
    return InvalidArgument(descriptor->full_name(),
                           " is not a valid FHIR extension type: No "
                           "fhir_structure_definition_url.");
  }
  return Status::OK();
}

namespace {

template <class ExtensionLike>
Status ConvertToExtensionInternal(const Message& message,
                                  ExtensionLike* extension) {
  const Descriptor* descriptor = message.GetDescriptor();
  FHIR_RETURN_IF_ERROR(ValidateExtension(descriptor));

  extension->mutable_url()->set_value(GetStructureDefinitionUrl(descriptor));

  // Carry over the id field if present.
  const FieldDescriptor* id_field =
      descriptor->field_count() > 1 && descriptor->field(0)->name() == "id"
          ? descriptor->field(0)
          : nullptr;
  const Reflection* reflection = message.GetReflection();
  if (id_field != nullptr && reflection->HasField(message, id_field)) {
    extension->mutable_id()->CopyFrom(
        reflection->GetMessage(message, id_field));
  }

  const std::vector<const FieldDescriptor*> value_fields =
      FindValueFields(descriptor);
  if (value_fields.empty()) {
    return InvalidArgument("Extension has no value fields: ",
                           descriptor->name());
  }
  if (value_fields.size() == 1 && !value_fields[0]->is_repeated()) {
    const FieldDescriptor* value_field = value_fields[0];
    FHIR_RETURN_IF_ERROR(CheckIsMessage(value_field));
    if (reflection->HasField(message, value_field)) {
      return AddValueToExtension(reflection->GetMessage(message, value_field),
                                 extension, IsChoiceType(value_field));
    } else {
      // TODO(nickgeorge, sundberg): This is an invalid extension.  Maybe we
      // should be erroring out here?
      return Status::OK();
    }
  } else {
    return AddFieldsToExtension(message, extension);
  }
}

template <class ExtensionLike>
Status ExtensionToMessageInternal(const ExtensionLike& extension,
                                  Message* message) {
  const Descriptor* descriptor = message->GetDescriptor();
  const Reflection* reflection = message->GetReflection();

  std::unordered_map<std::string, const FieldDescriptor*> fields_by_url;
  const FieldDescriptor* id_field = nullptr;
  for (int i = 0; i < descriptor->field_count(); i++) {
    // We need to handle the "id" field separately, since it corresponds to
    // Extension.id, not the slice "id".
    const FieldDescriptor* field = descriptor->field(i);
    if (field->name() == "id") {
      id_field = field;
    } else {
      fields_by_url[GetInlinedExtensionUrl(field)] = field;
    }
  }

  // Copy the id of the extension if present (this is uncommon).
  if (extension.has_id() && id_field != nullptr) {
    const Reflection* message_reflection = message->GetReflection();
    message_reflection->MutableMessage(message, id_field)
        ->CopyFrom(extension.id());
  }

  if (extension.value().value_case() != ExtensionLike::Value::VALUE_NOT_SET) {
    // This is a simple extension, with only one value.
    if (fields_by_url.size() != 1 ||
        fields_by_url.begin()->second->is_repeated()) {
      return InvalidArgument(descriptor->full_name(),
                             " is not a FHIR extension type");
    }
    return ValueToMessageInternal(extension, message,
                                  fields_by_url.begin()->second);
  }

  for (const ExtensionLike& inner : extension.extension()) {
    const FieldDescriptor* field = fields_by_url[inner.url().value()];

    if (field == nullptr) {
      return InvalidArgument("Message of type ", descriptor->full_name(),
                             " has no field with name ", inner.url().value());
    }

    if (inner.value().value_case() != ExtensionLike::Value::VALUE_NOT_SET) {
      FHIR_RETURN_IF_ERROR(ValueToMessageInternal(inner, message, field));
    } else {
      Message* child;
      if (field->is_repeated()) {
        child = reflection->AddMessage(message, field);
      } else if (reflection->HasField(*message, field) ||
                 inner.extension_size() > 0) {
        return ::tensorflow::errors::AlreadyExists(
            "Unexpected repeated value for tag ", field->name(),
            " in extension ", inner.DebugString());
      } else {
        child = reflection->MutableMessage(message, field);
      }
      FHIR_RETURN_IF_ERROR(ExtensionToMessageInternal(inner, child));
    }
  }
  return Status::OK();
}

}  // namespace

Status ConvertToExtension(const Message& message,
                          ::google::fhir::stu3::proto::Extension* extension) {
  return ConvertToExtensionInternal(message, extension);
}

Status ConvertToExtension(const Message& message,
                          ::google::fhir::r4::core::Extension* extension) {
  return ConvertToExtensionInternal(message, extension);
}

Status ExtensionToMessage(
    const ::google::fhir::stu3::proto::Extension& extension, Message* message) {
  return ExtensionToMessageInternal(extension, message);
}

Status ExtensionToMessage(const ::google::fhir::r4::core::Extension& extension,
                          Message* message) {
  return ExtensionToMessageInternal(extension, message);
}

Status ClearTypedExtensions(const Descriptor* descriptor, Message* message) {
  FHIR_RETURN_IF_ERROR(ValidateExtension(descriptor));
  return ClearExtensionsWithUrl(GetStructureDefinitionUrl(descriptor), message);
}

Status ClearExtensionsWithUrl(const std::string& url, Message* message) {
  const Reflection* reflection = message->GetReflection();
  const FieldDescriptor* field =
      message->GetDescriptor()->FindFieldByName("extension");
  google::protobuf::RepeatedPtrField<Message>* repeated_ptr_field =
      reflection->MutableRepeatedPtrField<Message>(message, field);
  for (auto iter = repeated_ptr_field->begin();
       iter != repeated_ptr_field->end();) {
    const Message& extension = *iter;
    std::string scratch;
    const std::string& url_value = GetExtensionUrl(extension, &scratch);

    if (url_value == url) {
      iter = repeated_ptr_field->erase(iter);
    } else {
      iter++;
    }
  }
  return Status::OK();
}

std::string GetInlinedExtensionUrl(const FieldDescriptor* field) {
  return field->options().HasExtension(
             ::google::fhir::proto::fhir_inlined_extension_url)
             ? field->options().GetExtension(
                   ::google::fhir::proto::fhir_inlined_extension_url)
             : field->json_name();
}

const std::string& GetExtensionUrl(const google::protobuf::Message& extension,
                                   std::string* scratch) {
  const Message& url_message = extension.GetReflection()->GetMessage(
      extension, extension.GetDescriptor()->FindFieldByName("url"));
  return url_message.GetReflection()->GetStringReference(
      url_message, url_message.GetDescriptor()->FindFieldByName("value"),
      scratch);
}

const std::string& GetExtensionSystem(const google::protobuf::Message& extension,
                                      std::string* scratch) {
  const Message& url_message = extension.GetReflection()->GetMessage(
      extension, extension.GetDescriptor()->FindFieldByName("system"));
  return url_message.GetReflection()->GetStringReference(
      url_message, url_message.GetDescriptor()->FindFieldByName("value"),
      scratch);
}

}  // namespace fhir
}  // namespace google
