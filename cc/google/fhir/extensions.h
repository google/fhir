/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GOOGLE_FHIR_EXTENSIONS_H_
#define GOOGLE_FHIR_EXTENSIONS_H_

#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/reflection.h"
#include "absl/strings/str_cat.h"
#include "absl/types/optional.h"
#include "google/fhir/codes.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {

// Provides utility functions for dealing with FHIR extensions.
// In order to implement this in a version-indepdendent way, this file defines
// three extensions namespaces:
//
// 1) extensions_internal for functions that should never be used outside of
// this compilation unit.
// 2) extensions_lib for functions that are fully version-independent
// 3) extensions_templates for template functions that cannot be used without
//    a specific extension version.
//
// For each version, there is a corresponding extensions.h defined in
// cc/$VERSION/extensions.h.  This exists on ::google::fhir::$VERSION namespace,
// and provides both the functions defined on extensions_lib, and definitions
// of the extensions_templates.
// This means that in any case where the version is known, files should import
// JUST the version-specific extensions.h, so code doesn't need to care which
// functions are defined where.

namespace extensions_lib {

Status ValidateExtension(const ::google::protobuf::Descriptor* descriptor);

// Extract all matching extensions from a container into a vector, and parse
// them into protos. Example usage:
// Patient patient = ...
// std::vector<MyExtension> my_extensions;
// auto status = GetRepeatedFromExtension(patient.extension(), &my_extension);
template <class C, class T>
Status GetRepeatedFromExtension(const C& extension_container,
                                std::vector<T>* result);

// Extracts a single extension of type T from 'entity'. Returns a NotFound error
// if there are zero extensions of that type. Returns an InvalidArgument error
// if there are more than one.
template <class T, class C>
StatusOr<T> ExtractOnlyMatchingExtension(const C& entity);

Status ClearTypedExtensions(const ::google::protobuf::Descriptor* descriptor,
                            ::google::protobuf::Message* message);

Status ClearExtensionsWithUrl(const std::string& url,
                              ::google::protobuf::Message* message);

std::string GetInlinedExtensionUrl(const ::google::protobuf::FieldDescriptor* field);

const std::string& GetExtensionUrl(const google::protobuf::Message& extension,
                                   std::string* scratch);

const std::string& GetExtensionSystem(const google::protobuf::Message& extension,
                                      std::string* scratch);

}  // namespace extensions_lib

namespace extensions_templates {

template <class ExtensionLike>
Status ValueToMessage(const ExtensionLike& extension,
                      ::google::protobuf::Message* message,
                      const ::google::protobuf::FieldDescriptor* field);

template <class ExtensionLike>
Status ExtensionToMessage(const ExtensionLike& extension,
                          ::google::protobuf::Message* message);

// Given a datatype message (E.g., String, Code, Boolean, etc.),
// finds the appropriate field on the target extension and sets it.
// Returns ::tensorflow::errors::InvalidArgument if there's no matching oneof
// type on the extension for the message.
template <class ExtensionLike>
Status SetDatatypeOnExtension(const ::google::protobuf::Message& datum,
                              ExtensionLike* extension);

template <class ExtensionLike>
Status ConvertToExtension(const ::google::protobuf::Message& message,
                          ExtensionLike* extension);

}  // namespace extensions_templates

namespace extensions_internal {

Status CheckIsMessage(const ::google::protobuf::FieldDescriptor* field);

const std::vector<const ::google::protobuf::FieldDescriptor*> FindValueFields(
    const ::google::protobuf::Descriptor* descriptor);

template <class ExtensionLike>
Status AddValueToExtension(const ::google::protobuf::Message& message,
                           ExtensionLike* extension, bool is_choice_type);

// For the Extension message in the template, returns the appropriate field on
// the extension for a descriptor of a given type.
// For example, given a Base64Binary, this will return the base64_binary field
// on an extension.
template <typename ExtensionType>
const absl::optional<const ::google::protobuf::FieldDescriptor*>
GetExtensionValueFieldByType(const ::google::protobuf::Descriptor* field_type) {
  static const std::unordered_map<
      std::string,
      const ::google::protobuf::FieldDescriptor*>* extension_value_fields_by_type = []() {
    std::unordered_map<std::string, const ::google::protobuf::FieldDescriptor*>* map =
        new std::unordered_map<std::string, const ::google::protobuf::FieldDescriptor*>;

    const google::protobuf::OneofDescriptor* value_oneof =
        ExtensionType::ValueX::descriptor()->FindOneofByName("choice");
    CHECK(value_oneof != nullptr);
    for (int i = 0; i < value_oneof->field_count(); i++) {
      const ::google::protobuf::FieldDescriptor* field = value_oneof->field(i);
      (*map)[field->message_type()->full_name()] = field;
    }
    return map;
  }();
  auto iter = extension_value_fields_by_type->find(field_type->full_name());
  return iter == extension_value_fields_by_type->end()
             ? absl::optional<const ::google::protobuf::FieldDescriptor*>()
             : absl::make_optional(iter->second);
}

// Given an extension, returns the field descriptor for the populated choice
// type in the value oneof.
// Returns an InvalidArgument status if no value field is set on the extension.
template <class ExtensionLike>
StatusOr<const ::google::protobuf::FieldDescriptor*> GetPopulatedExtensionValueField(
    const ExtensionLike& extension) {
  static const google::protobuf::OneofDescriptor* value_oneof =
      ExtensionLike::ValueX::descriptor()->FindOneofByName("choice");
  const auto& value = extension.value();
  const ::google::protobuf::Reflection* value_reflection = value.GetReflection();
  const ::google::protobuf::FieldDescriptor* field =
      value_reflection->GetOneofFieldDescriptor(value, value_oneof);
  if (field == nullptr) {
    return ::tensorflow::errors::InvalidArgument("No value set on extension.");
  }
  FHIR_RETURN_IF_ERROR(extensions_internal::CheckIsMessage(field));
  return field;
}

template <class ExtensionLike>
Status AddFieldsToExtension(const ::google::protobuf::Message& message,
                            ExtensionLike* extension) {
  const ::google::protobuf::Descriptor* descriptor = message.GetDescriptor();
  const ::google::protobuf::Reflection* reflection = message.GetReflection();
  std::vector<const ::google::protobuf::FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  for (const auto* field : fields) {
    if (field->cpp_type() != ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      return ::tensorflow::errors::InvalidArgument(
          descriptor->full_name(), " is not a FHIR extension type");
    }
    // Add submessages to nested extensions.
    if (field->is_repeated()) {
      for (int j = 0; j < reflection->FieldSize(message, field); j++) {
        ExtensionLike* child = extension->add_extension();
        child->mutable_url()->set_value(
            extensions_lib::GetInlinedExtensionUrl(field));
        FHIR_RETURN_IF_ERROR(extensions_internal::CheckIsMessage(field));
        FHIR_RETURN_IF_ERROR(extensions_internal::AddValueToExtension(
            reflection->GetRepeatedMessage(message, field, j), child,
            IsChoiceType(field)));
      }
    } else {
      ExtensionLike* child = extension->add_extension();
      child->mutable_url()->set_value(
          extensions_lib::GetInlinedExtensionUrl(field));
      FHIR_RETURN_IF_ERROR(extensions_internal::CheckIsMessage(field));
      FHIR_RETURN_IF_ERROR(extensions_internal::AddValueToExtension(
          reflection->GetMessage(message, field), child, IsChoiceType(field)));
    }
  }
  return Status::OK();
}

template <class ExtensionLike>
Status AddValueToExtension(const ::google::protobuf::Message& message,
                           ExtensionLike* extension, bool is_choice_type) {
  const ::google::protobuf::Descriptor* descriptor = message.GetDescriptor();

  if (is_choice_type) {
    const google::protobuf::OneofDescriptor* oneof = descriptor->oneof_decl(0);
    if (oneof == nullptr) {
      return ::tensorflow::errors::InvalidArgument(
          "Choice type is missing a oneof: ", descriptor->full_name());
    }
    const ::google::protobuf::Reflection* message_reflection = message.GetReflection();
    const ::google::protobuf::FieldDescriptor* value_field =
        message_reflection->GetOneofFieldDescriptor(message, oneof);
    if (value_field == nullptr) {
      return ::tensorflow::errors::InvalidArgument(
          "Choice type has no value set: ", descriptor->full_name());
    }
    FHIR_RETURN_IF_ERROR(extensions_internal::CheckIsMessage(value_field));
    return extensions_internal::AddValueToExtension(
        message_reflection->GetMessage(message, value_field), extension, false);
  }
  // Try to set the message directly as a datatype value on the extension.
  // E.g., put message of type boolean into the value.boolean field
  if (extensions_templates::SetDatatypeOnExtension(message, extension).ok()) {
    return Status::OK();
  }
  // Fall back to adding individual fields as sub-extensions.
  return AddFieldsToExtension(message, extension);
}

}  // namespace extensions_internal

namespace extensions_lib {

template <class C, class T>
Status GetRepeatedFromExtension(const C& extension_container,
                                std::vector<T>* result) {
  // This function will be called a huge number of times, usually when no
  // extensions are present.  Return early in this case to keep overhead as low
  // as possible.
  if (extension_container.empty()) {
    return Status::OK();
  }
  const ::google::protobuf::Descriptor* descriptor = T::descriptor();
  FHIR_RETURN_IF_ERROR(ValidateExtension(descriptor));
  const std::string& url = descriptor->options().GetExtension(
      ::google::fhir::proto::fhir_structure_definition_url);
  for (const auto& extension : extension_container) {
    if (extension.url().value() == url) {
      T message;
      FHIR_RETURN_IF_ERROR(
          extensions_templates::ExtensionToMessage<typename C::value_type>(
              extension, &message));
      result->emplace_back(message);
    }
  }
  return Status::OK();
}

template <class T, class C>
StatusOr<T> ExtractOnlyMatchingExtension(const C& entity) {
  std::vector<T> result;
  FHIR_RETURN_IF_ERROR(GetRepeatedFromExtension(entity.extension(), &result));
  if (result.empty()) {
    return ::tensorflow::errors::NotFound(
        "Did not find any extension with url: ",
        GetStructureDefinitionUrl(T::descriptor()), " on ",
        C::descriptor()->full_name(), ".");
  }
  if (result.size() > 1) {
    return ::tensorflow::errors::InvalidArgument(
        "Expected exactly 1 extension with url: ",
        GetStructureDefinitionUrl(T::descriptor()), " on ",
        C::descriptor()->full_name(), ". Found: ", result.size());
  }
  return result.front();
}

}  // namespace extensions_lib

namespace extensions_templates {

template <class ExtensionLike>
Status ValueToMessage(const ExtensionLike& extension,
                      ::google::protobuf::Message* message,
                      const ::google::protobuf::FieldDescriptor* field) {
  const ::google::protobuf::Descriptor* descriptor = message->GetDescriptor();
  if (field->cpp_type() != ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    return ::tensorflow::errors::InvalidArgument(
        descriptor->full_name(), " is not a FHIR extension type");
  }
  // If there's a value, there can not be any extensions set.
  if (extension.extension_size() > 0) {
    return ::tensorflow::errors::InvalidArgument("Invalid extension: ",
                                                 extension.DebugString());
  }
  FHIR_ASSIGN_OR_RETURN(
      const ::google::protobuf::FieldDescriptor* value_field,
      extensions_internal::GetPopulatedExtensionValueField(extension));

  const ::google::protobuf::Reflection* message_reflection = message->GetReflection();
  if (IsChoiceType(field)) {
    // We need to assign the value from the extension to the correct field
    // on the choice type.
    ::google::protobuf::Message* choice_message =
        message_reflection->MutableMessage(message, field);
    const ::google::protobuf::Descriptor* choice_descriptor =
        choice_message->GetDescriptor();
    for (int i = 0; i < choice_descriptor->field_count(); i++) {
      const ::google::protobuf::FieldDescriptor* choice_field =
          choice_descriptor->field(i);
      if (value_field->message_type()->full_name() ==
          choice_field->message_type()->full_name()) {
        return ValueToMessage(extension, choice_message, choice_field);
      }
    }
    return ::tensorflow::errors::InvalidArgument(
        "No field on Choice Type ", choice_descriptor->full_name(),
        " for extension ", extension.DebugString());
  }

  if (HasValueset(field->message_type())) {
    // The target message is a bound code type.  Convert the generic code
    // field from the extension into the target typed code.
    return CopyCode(extension.value().code(),
                    MutableOrAddMessage(message, field));
  }

  if (IsTypeOrProfileOfCoding(field->message_type())) {
    // The target message is a bound codng type.  Convert the generic codng
    // field from the extension into the target typed coding.
    return CopyCoding(extension.value().coding(),
                      MutableOrAddMessage(message, field));
  }

  // Value types must match.
  if (!AreSameMessageType(value_field->message_type(), field->message_type())) {
    return ::tensorflow::errors::InvalidArgument(
        "Missing expected value of type ", field->message_type()->full_name(),
        " in extension ", extension.GetDescriptor()->full_name());
  }
  const auto& value = extension.value();
  const ::google::protobuf::Reflection* value_reflection = value.GetReflection();
  MutableOrAddMessage(message, field)
      ->CopyFrom(value_reflection->GetMessage(value, value_field));
  return Status::OK();
}

template <class ExtensionLike>
Status ExtensionToMessage(const ExtensionLike& extension,
                          ::google::protobuf::Message* message) {
  const ::google::protobuf::Descriptor* descriptor = message->GetDescriptor();
  const ::google::protobuf::Reflection* reflection = message->GetReflection();

  std::unordered_map<std::string, const ::google::protobuf::FieldDescriptor*>
      fields_by_url;
  const ::google::protobuf::FieldDescriptor* id_field = nullptr;
  for (int i = 0; i < descriptor->field_count(); i++) {
    // We need to handle the "id" field separately, since it corresponds to
    // Extension.id, not the slice "id".
    const ::google::protobuf::FieldDescriptor* field = descriptor->field(i);
    if (field->name() == "id") {
      id_field = field;
    } else {
      fields_by_url[extensions_lib::GetInlinedExtensionUrl(field)] = field;
    }
  }

  // Copy the id of the extension if present (this is uncommon).
  if (extension.has_id() && id_field != nullptr) {
    const ::google::protobuf::Reflection* message_reflection = message->GetReflection();
    message_reflection->MutableMessage(message, id_field)
        ->CopyFrom(extension.id());
  }

  if (extension.value().choice_case() !=
      ExtensionLike::ValueX::CHOICE_NOT_SET) {
    // This is a simple extension, with only one value.
    if (fields_by_url.size() != 1 ||
        fields_by_url.begin()->second->is_repeated()) {
      return ::tensorflow::errors::InvalidArgument(
          descriptor->full_name(), " is not a FHIR extension type");
    }
    return ValueToMessage(extension, message, fields_by_url.begin()->second);
  }

  for (const ExtensionLike& inner : extension.extension()) {
    const ::google::protobuf::FieldDescriptor* field = fields_by_url[inner.url().value()];

    if (field == nullptr) {
      return ::tensorflow::errors::InvalidArgument(
          "Message of type ", descriptor->full_name(),
          " has no field with name ", inner.url().value());
    }

    if (inner.value().choice_case() != ExtensionLike::ValueX::CHOICE_NOT_SET) {
      FHIR_RETURN_IF_ERROR(ValueToMessage(inner, message, field));
    } else {
      ::google::protobuf::Message* child;
      if (field->is_repeated()) {
        child = reflection->AddMessage(message, field);
      } else if (reflection->HasField(*message, field)) {
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

template <class ExtensionLike>
Status SetDatatypeOnExtension(const ::google::protobuf::Message& datum,
                              ExtensionLike* extension) {
  const ::google::protobuf::Descriptor* descriptor = datum.GetDescriptor();
  auto value_field_optional =
      extensions_internal::GetExtensionValueFieldByType<ExtensionLike>(
          descriptor);
  if (value_field_optional.has_value()) {
    extension->value()
        .GetReflection()
        ->MutableMessage(extension->mutable_value(), *value_field_optional)
        ->CopyFrom(datum);
    return Status::OK();
  }
  if (HasValueset(datum.GetDescriptor())) {
    // The source datum is a bound code type.
    // Convert it to a generic code, and add it to the extension.
    return CopyCode(datum, extension->mutable_value()->mutable_code());
  }
  if (IsTypeOrProfileOfCoding(datum)) {
    // The source datum is a bound coding type.
    // Convert it to a generic coding, and add it to the extension.
    return CopyCoding(datum, extension->mutable_value()->mutable_coding());
  }
  return ::tensorflow::errors::InvalidArgument(
      descriptor->full_name(), " is not a valid value type on Extension.");
}

template <class ExtensionLike>
Status ConvertToExtension(const ::google::protobuf::Message& message,
                          ExtensionLike* extension) {
  const ::google::protobuf::Descriptor* descriptor = message.GetDescriptor();
  FHIR_RETURN_IF_ERROR(extensions_lib::ValidateExtension(descriptor));

  extension->mutable_url()->set_value(GetStructureDefinitionUrl(descriptor));

  // Carry over the id field if present.
  const ::google::protobuf::FieldDescriptor* id_field =
      descriptor->field_count() > 1 && descriptor->field(0)->name() == "id"
          ? descriptor->field(0)
          : nullptr;
  const ::google::protobuf::Reflection* reflection = message.GetReflection();
  if (id_field != nullptr && reflection->HasField(message, id_field)) {
    extension->mutable_id()->CopyFrom(
        reflection->GetMessage(message, id_field));
  }

  const std::vector<const ::google::protobuf::FieldDescriptor*> value_fields =
      extensions_internal::FindValueFields(descriptor);
  if (value_fields.empty()) {
    return ::tensorflow::errors::InvalidArgument(
        "Extension has no value fields: ", descriptor->name());
  }
  if (value_fields.size() == 1 && !value_fields[0]->is_repeated()) {
    const ::google::protobuf::FieldDescriptor* value_field = value_fields[0];
    FHIR_RETURN_IF_ERROR(extensions_internal::CheckIsMessage(value_field));
    if (reflection->HasField(message, value_field)) {
      return extensions_internal::AddValueToExtension(
          reflection->GetMessage(message, value_field), extension,
          IsChoiceType(value_field));
    } else {
      // TODO(nickgeorge, sundberg): This is an invalid extension.  Maybe we
      // should be erroring out here?
      return Status::OK();
    }
  } else {
    return extensions_internal::AddFieldsToExtension(message, extension);
  }
}

}  // namespace extensions_templates

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_EXTENSIONS_H_
