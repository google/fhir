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

#include "google/fhir/stu3/extensions.h"

#include <unordered_map>

#include "google/protobuf/descriptor.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/status/status.h"
#include "google/fhir/stu3/codes.h"
#include "google/fhir/stu3/proto_util.h"
#include "google/fhir/stu3/util.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/logging.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;
using ::google::fhir::stu3::proto::Extension;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using ::tensorflow::Status;
using ::tensorflow::errors::InvalidArgument;

namespace {

const std::unordered_map<string, const FieldDescriptor*>*
GetExtensionValueFieldsMap() {
  static const std::unordered_map<string, const FieldDescriptor*>*
      extension_value_fields_by_type = []() {
        const google::protobuf::OneofDescriptor* value_oneof =
            Extension::Value::descriptor()->FindOneofByName("value");
        CHECK(value_oneof != nullptr);

        std::unordered_map<string, const FieldDescriptor*>* map =
            new std::unordered_map<string, const FieldDescriptor*>;
        for (int i = 0; i < value_oneof->field_count(); i++) {
          const FieldDescriptor* field = value_oneof->field(i);
          (*map)[field->message_type()->full_name()] = field;
        }
        return map;
      }();
  return extension_value_fields_by_type;
}

Status AddFieldsToExtension(const Message& message, Extension* extension);

Status CheckIsMessage(const FieldDescriptor* field) {
  if (field->type() != FieldDescriptor::Type::TYPE_MESSAGE) {
    return InvalidArgument("Encountered unexpected proto primitive: ",
                           field->full_name(), ".  Should be FHIR type");
  }
  return Status::OK();
}

Status AddValueToExtension(const Message& message, Extension* extension,
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
    return AddValueToExtension(
        message_reflection->GetMessage(message, value_field), extension, false);
  }
  // Try to set the message directly as a datatype value on the extension.
  // E.g., put message of type boolean into the value.boolean field
  if (SetDatatypeOnExtension(message, extension).ok()) {
    return Status::OK();
  }
  // Fall back to adding individual fields as sub-extensions.
  return AddFieldsToExtension(message, extension);
}

Status AddFieldsToExtension(const Message& message, Extension* extension) {
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
        Extension* child = extension->add_extension();
        child->mutable_url()->set_value(GetInlinedExtensionUrl(field));
        FHIR_RETURN_IF_ERROR(CheckIsMessage(field));
        FHIR_RETURN_IF_ERROR(AddValueToExtension(
            reflection->GetRepeatedMessage(message, field, j), child,
            IsChoiceType(field)));
      }
    } else {
      Extension* child = extension->add_extension();
      child->mutable_url()->set_value(GetInlinedExtensionUrl(field));
      FHIR_RETURN_IF_ERROR(CheckIsMessage(field));
      FHIR_RETURN_IF_ERROR(AddValueToExtension(
          reflection->GetMessage(message, field), child, IsChoiceType(field)));
    }
  }
  return Status::OK();
}

StatusOr<const FieldDescriptor*> GetExtensionValueField(
    const Extension& extension) {
  static const google::protobuf::OneofDescriptor* value_oneof =
      Extension::Value::descriptor()->FindOneofByName("value");
  const Extension::Value value = extension.value();
  const Reflection* value_reflection = value.GetReflection();
  const FieldDescriptor* field =
      value_reflection->GetOneofFieldDescriptor(value, value_oneof);
  if (field == nullptr) {
    return InvalidArgument("No value set on extension.");
  }
  FHIR_RETURN_IF_ERROR(CheckIsMessage(field));
  return field;
}

Status ValueToMessage(const Extension& extension, Message* message,
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
        return ValueToMessage(extension, choice_message, choice_field);
      }
    }
    return InvalidArgument("No field on Choice Type ",
                           choice_descriptor->full_name(), " for extension ",
                           extension.DebugString());
  }

  if (HasValueset(field->message_type())) {
    // The target message is a bound code type.  Convert the generic code
    // field from the extension into the target typed code.
    return ConvertToTypedCode(
        extension.value().code(),
        message_reflection->MutableMessage(message, field));
  }

  // Value types must match.
  if (value_field->message_type()->full_name() !=
      field->message_type()->full_name()) {
    return InvalidArgument("Missing expected value of type ",
                           field->message_type()->full_name(), " in extension ",
                           extension.DebugString());
  }
  const Extension::Value value = extension.value();
  const Reflection* value_reflection = value.GetReflection();
  MutableOrAddMessage(message, field)
      ->CopyFrom(value_reflection->GetMessage(value, value_field));
  return Status::OK();
}

const std::vector<const FieldDescriptor*> FindValueFields(
    const Descriptor* descriptor) {
  std::vector<const FieldDescriptor*> value_fields;
  for (int i = 0; i < descriptor->field_count(); i++) {
    const string& name = descriptor->field(i)->name();
    if (name != "id" && name != "extension") {
      value_fields.push_back(descriptor->field(i));
    }
  }
  return value_fields;
}

}  // namespace
// Given a datatype message (E.g., String, Code, Boolean, etc.),
// finds the appropriate field on the target extension and sets it.
// Returns InvalidArgument if there's no matching oneof type on the extension
// for the message.
Status SetDatatypeOnExtension(const Message& message, Extension* extension) {
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
  return InvalidArgument(descriptor->full_name(),
                         " is not a valid value type on Extension.");
}

Status ValidateExtension(const Descriptor* descriptor) {
  if (descriptor->options().GetExtension(stu3::proto::fhir_profile_base) !=
      Extension::descriptor()->options().GetExtension(
          stu3::proto::fhir_structure_definition_url)) {
    return InvalidArgument(descriptor->full_name(),
                           " is not a FHIR extension type");
  }
  if (!descriptor->options().HasExtension(
          stu3::proto::fhir_structure_definition_url)) {
    return InvalidArgument(descriptor->full_name(),
                           " is not a valid FHIR extension type: No "
                           "fhir_structure_definition_url.");
  }
  return Status::OK();
}

Status ConvertToExtension(const Message& message, Extension* extension) {
  const Descriptor* descriptor = message.GetDescriptor();
  FHIR_RETURN_IF_ERROR(ValidateExtension(descriptor));

  extension->mutable_url()->set_value(descriptor->options().GetExtension(
      stu3::proto::fhir_structure_definition_url));

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

Status ExtensionToMessage(const Extension& extension, Message* message) {
  const Descriptor* descriptor = message->GetDescriptor();
  const Reflection* reflection = message->GetReflection();

  std::unordered_map<string, const FieldDescriptor*> fields_by_url;
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

  if (extension.value().value_case() != Extension::Value::VALUE_NOT_SET) {
    // This is a simple extension, with only one value.
    if (fields_by_url.size() != 1 ||
        fields_by_url.begin()->second->is_repeated()) {
      return InvalidArgument(descriptor->full_name(),
                             " is not a FHIR extension type");
    }
    return ValueToMessage(extension, message, fields_by_url.begin()->second);
  }

  for (const Extension& inner : extension.extension()) {
    const FieldDescriptor* field = fields_by_url[inner.url().value()];

    if (field == nullptr) {
      return InvalidArgument("Message of type ", descriptor->full_name(),
                             " has no field with name ", inner.url().value());
    }

    if (inner.value().value_case() != Extension::Value::VALUE_NOT_SET) {
      FHIR_RETURN_IF_ERROR(ValueToMessage(inner, message, field));
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
      FHIR_RETURN_IF_ERROR(ExtensionToMessage(inner, child));
    }
  }
  return Status::OK();
}

Status ClearTypedExtensions(const Descriptor* descriptor, Message* message) {
  FHIR_RETURN_IF_ERROR(ValidateExtension(descriptor));
  const string url = descriptor->options().GetExtension(
      stu3::proto::fhir_structure_definition_url);

  const Reflection* reflection = message->GetReflection();
  const FieldDescriptor* field =
      message->GetDescriptor()->FindFieldByName("extension");
  std::vector<Extension> other_extensions;
  for (int i = 0; i < reflection->FieldSize(*message, field); i++) {
    Extension extension = dynamic_cast<const Extension&>(
        reflection->GetRepeatedMessage(*message, field, i));
    if (extension.url().value() != url) {
      other_extensions.push_back(extension);
    }
  }
  reflection->ClearField(message, field);
  for (const Extension& extension : other_extensions) {
    message->GetReflection()->AddMessage(message, field)->CopyFrom(extension);
  }
  return Status::OK();
}

string GetInlinedExtensionUrl(const FieldDescriptor* field) {
  return field->options().HasExtension(proto::fhir_inlined_extension_url)
             ? field->options().GetExtension(proto::fhir_inlined_extension_url)
             : field->json_name();
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
