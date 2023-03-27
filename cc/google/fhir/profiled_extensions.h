/*
 * Copyright 2023 Google LLC
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

#ifndef GOOGLE_FHIR_PROFILED_EXTENSIONS_H_
#define GOOGLE_FHIR_PROFILED_EXTENSIONS_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "absl/status/statusor.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codes.h"
#include "google/fhir/extensions.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"

namespace google::fhir::profiled {

// Extract all matching extensions from a container into a vector, and parse
// them into protos. Example usage:
// Patient patient = ...
// std::vector<MyExtension> my_extensions;
// auto status = GetAllMatchingExtensions(patient.extension(), &my_extension);
// DEPRECATED - this relies on profiled protos, which are being deprecated.
template <class C, class T>
absl::Status GetAllMatchingExtensions(const C& extension_container,
                                      std::vector<T>* result);

template <class ExtensionLike>
absl::Status ExtensionToMessage(const ExtensionLike& extension,
                                google::protobuf::Message* message);

template <class ExtensionLike>
absl::Status ConvertToExtension(const google::protobuf::Message& message,
                                ExtensionLike* extension);

std::string GetInlinedExtensionUrl(const google::protobuf::FieldDescriptor* field);

// Implementation details only past this point.
namespace internal {

absl::Status ValidateExtension(const google::protobuf::Descriptor* descriptor);
absl::Status CheckIsMessage(const google::protobuf::FieldDescriptor* field);

std::vector<const google::protobuf::FieldDescriptor*> FindValueFields(
    const google::protobuf::Descriptor* descriptor);

// Given an extension, returns the field descriptor for the populated choice
// type in the value oneof.
// Returns an InvalidArgument status if no value field is set on the extension.
template <class ExtensionLike>
absl::StatusOr<const google::protobuf::FieldDescriptor*> GetPopulatedExtensionValueField(
    const ExtensionLike& extension) {
  static const google::protobuf::OneofDescriptor* value_oneof =
      ExtensionLike::ValueX::descriptor()->FindOneofByName("choice");
  const auto& value = extension.value();
  const google::protobuf::Reflection* value_reflection = value.GetReflection();
  const google::protobuf::FieldDescriptor* field =
      value_reflection->GetOneofFieldDescriptor(value, value_oneof);
  if (field == nullptr) {
    return ::absl::InvalidArgumentError("No value set on extension.");
  }
  FHIR_RETURN_IF_ERROR(internal::CheckIsMessage(field));
  return field;
}

template <class ExtensionLike>
absl::Status ValueToMessage(const ExtensionLike& extension,
                            google::protobuf::Message* message,
                            const google::protobuf::FieldDescriptor* field) {
  const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
  if (field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    return ::absl::InvalidArgumentError(
        absl::StrCat(descriptor->full_name(), " is not a FHIR extension type"));
  }
  // If there's a value, there can not be any extensions set.
  if (extension.extension_size() > 0) {
    return ::absl::InvalidArgumentError(
        absl::StrCat("Invalid extension: ", extension.DebugString()));
  }
  FHIR_ASSIGN_OR_RETURN(const google::protobuf::FieldDescriptor* value_field,
                        internal::GetPopulatedExtensionValueField(extension));

  const google::protobuf::Reflection* message_reflection = message->GetReflection();
  if (IsChoiceType(field)) {
    // We need to assign the value from the extension to the correct field
    // on the choice type.
    google::protobuf::Message* choice_message =
        message_reflection->MutableMessage(message, field);
    const google::protobuf::Descriptor* choice_descriptor =
        choice_message->GetDescriptor();
    for (int i = 0; i < choice_descriptor->field_count(); i++) {
      const google::protobuf::FieldDescriptor* choice_field = choice_descriptor->field(i);
      if (value_field->message_type()->full_name() ==
          choice_field->message_type()->full_name()) {
        return ValueToMessage(extension, choice_message, choice_field);
      }
    }
    return ::absl::InvalidArgumentError(
        absl::StrCat("No field on Choice Type ", choice_descriptor->full_name(),
                     " for extension ", extension.DebugString()));
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
    return ::absl::InvalidArgumentError(absl::StrCat(
        "Missing expected value of type ", field->message_type()->full_name(),
        " in extension ", extension.GetDescriptor()->full_name()));
  }
  const auto& value = extension.value();
  const google::protobuf::Reflection* value_reflection = value.GetReflection();
  MutableOrAddMessage(message, field)
      ->CopyFrom(value_reflection->GetMessage(value, value_field));
  return absl::OkStatus();
}

// Returns true if the passed-in descriptor descriptor describes a "Simple"
// extension, which corresponds to an unprofiled extension with the "value"
// choice type set, rather than itself having extensions.
bool IsSimpleExtension(const google::protobuf::Descriptor* descriptor);

template <class ExtensionLike>
absl::Status AddValueToExtension(const google::protobuf::Message& message,
                                 ExtensionLike* extension, bool is_choice_type);

template <class ExtensionLike>
absl::Status AddFieldsToExtension(const google::protobuf::Message& message,
                                  ExtensionLike* extension) {
  const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
  const google::protobuf::Reflection* reflection = message.GetReflection();
  std::vector<const google::protobuf::FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  for (const auto* field : fields) {
    if (field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      return ::absl::InvalidArgumentError(absl::StrCat(
          descriptor->full_name(), " is not a FHIR extension type"));
    }
    if (field->name() == "id") {
      // Skip the ID field since it is shouldn't be added as an extension.
      continue;
    }
    // Add submessages to nested extensions.
    if (field->is_repeated()) {
      for (int j = 0; j < reflection->FieldSize(message, field); j++) {
        ExtensionLike* child = extension->add_extension();
        child->mutable_url()->set_value(GetInlinedExtensionUrl(field));
        FHIR_RETURN_IF_ERROR(internal::CheckIsMessage(field));
        FHIR_RETURN_IF_ERROR(internal::AddValueToExtension(
            reflection->GetRepeatedMessage(message, field, j), child,
            IsChoiceType(field)));
      }
    } else {
      ExtensionLike* child = extension->add_extension();
      child->mutable_url()->set_value(GetInlinedExtensionUrl(field));
      FHIR_RETURN_IF_ERROR(internal::CheckIsMessage(field));
      FHIR_RETURN_IF_ERROR(internal::AddValueToExtension(
          reflection->GetMessage(message, field), child, IsChoiceType(field)));
    }
  }
  return absl::OkStatus();
}

template <class ExtensionLike>
absl::Status AddValueToExtension(const google::protobuf::Message& message,
                                 ExtensionLike* extension,
                                 bool is_choice_type) {
  const google::protobuf::Descriptor* descriptor = message.GetDescriptor();

  if (is_choice_type) {
    const google::protobuf::OneofDescriptor* oneof = descriptor->oneof_decl(0);
    if (oneof == nullptr) {
      return ::absl::InvalidArgumentError(absl::StrCat(
          "Choice type is missing a oneof: ", descriptor->full_name()));
    }
    const google::protobuf::Reflection* message_reflection = message.GetReflection();
    const google::protobuf::FieldDescriptor* value_field =
        message_reflection->GetOneofFieldDescriptor(message, oneof);
    if (value_field == nullptr) {
      return ::absl::InvalidArgumentError(absl::StrCat(
          "Choice type has no value set: ", descriptor->full_name()));
    }
    FHIR_RETURN_IF_ERROR(internal::CheckIsMessage(value_field));
    return internal::AddValueToExtension(
        message_reflection->GetMessage(message, value_field), extension, false);
  }
  // Try to set the message directly as a datatype value on the extension.
  // E.g., put message of type boolean into the value.boolean field
  if (SetDatatypeOnExtension(message, extension).ok()) {
    return absl::OkStatus();
  }
  // Fall back to adding individual fields as sub-extensions.
  return AddFieldsToExtension(message, extension);
}
}  // namespace internal

// Extract all matching extensions from a container into a vector, and parse
// them into protos. Example usage:
// Patient patient = ...
// std::vector<MyExtension> my_extensions;
// auto status = GetAllMatchingExtensions(patient.extension(), &my_extension);
// DEPRECATED - this relies on profiled protos, which are being deprecated.
template <class C, class T>
absl::Status GetAllMatchingExtensions(const C& extension_container,
                                      std::vector<T>* result) {
  // This function will be called a huge number of times, usually when no
  // extensions are present.  Return early in this case to keep overhead as low
  // as possible.
  if (extension_container.empty()) {
    return absl::OkStatus();
  }
  const google::protobuf::Descriptor* descriptor = T::descriptor();
  FHIR_RETURN_IF_ERROR(internal::ValidateExtension(descriptor));
  const std::string& url = descriptor->options().GetExtension(
      ::google::fhir::proto::fhir_structure_definition_url);
  for (const auto& extension : extension_container) {
    if (extension.url().value() == url) {
      T message;
      FHIR_RETURN_IF_ERROR(
          ExtensionToMessage<typename C::value_type>(extension, &message));
      result->emplace_back(message);
    }
  }
  return absl::OkStatus();
}

// Extracts a single extension of type T from 'entity'. Returns a NotFound error
// if there are zero extensions of that type. Returns an InvalidArgument error
// if there are more than one.
// DEPRECATED - this relies on profiled protos, which are being deprecated.
template <class T, class C>
absl::StatusOr<T> ExtractOnlyMatchingExtension(const C& entity) {
  std::vector<T> result;
  FHIR_RETURN_IF_ERROR(GetAllMatchingExtensions(entity.extension(), &result));
  if (result.empty()) {
    return ::absl::NotFoundError(
        absl::StrCat("Did not find any extension with url: ",
                     GetStructureDefinitionUrl(T::descriptor()), " on ",
                     C::descriptor()->full_name(), "."));
  }
  if (result.size() > 1) {
    return ::absl::InvalidArgumentError(
        absl::StrCat("Expected exactly 1 extension with url: ",
                     GetStructureDefinitionUrl(T::descriptor()), " on ",
                     C::descriptor()->full_name(), ". Found: ", result.size()));
  }
  return result.front();
}

template <class ExtensionLike>
absl::Status ExtensionToMessage(const ExtensionLike& extension,
                                google::protobuf::Message* message) {
  const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
  const google::protobuf::Reflection* reflection = message->GetReflection();

  std::unordered_map<std::string, const google::protobuf::FieldDescriptor*> fields_by_url;
  const google::protobuf::FieldDescriptor* id_field = nullptr;
  for (int i = 0; i < descriptor->field_count(); i++) {
    // We need to handle the "id" field separately, since it corresponds to
    // Extension.id, not the slice "id".
    const google::protobuf::FieldDescriptor* field = descriptor->field(i);
    if (field->name() == "id") {
      id_field = field;
    } else {
      fields_by_url[GetInlinedExtensionUrl(field)] = field;
    }
  }

  // Copy the id of the extension if present (this is uncommon).
  if (extension.has_id() && id_field != nullptr) {
    const google::protobuf::Reflection* message_reflection = message->GetReflection();
    message_reflection->MutableMessage(message, id_field)
        ->CopyFrom(extension.id());
  }

  if (extension.value().choice_case() !=
      ExtensionLike::ValueX::CHOICE_NOT_SET) {
    // This is a simple extension, with only one value.
    if (fields_by_url.size() != 1 ||
        fields_by_url.begin()->second->is_repeated()) {
      return ::absl::InvalidArgumentError(absl::StrCat(
          descriptor->full_name(), " is not a FHIR extension type"));
    }
    return internal::ValueToMessage(extension, message,
                                    fields_by_url.begin()->second);
  }

  for (const ExtensionLike& inner : extension.extension()) {
    const google::protobuf::FieldDescriptor* field = fields_by_url[inner.url().value()];

    if (field == nullptr) {
      return ::absl::InvalidArgumentError(
          absl::StrCat("Message of type ", descriptor->full_name(),
                       " has no field with name ", inner.url().value()));
    }

    if (inner.value().choice_case() != ExtensionLike::ValueX::CHOICE_NOT_SET) {
      FHIR_RETURN_IF_ERROR(internal::ValueToMessage(inner, message, field));
    } else {
      google::protobuf::Message* child;
      if (field->is_repeated()) {
        child = reflection->AddMessage(message, field);
      } else if (reflection->HasField(*message, field)) {
        return ::absl::AlreadyExistsError(
            absl::StrCat("Unexpected repeated value for tag ", field->name(),
                         " in extension ", inner.DebugString()));
      } else {
        child = reflection->MutableMessage(message, field);
      }

      FHIR_RETURN_IF_ERROR(ExtensionToMessage(inner, child));
    }
  }
  return absl::OkStatus();
}

template <class ExtensionLike>
absl::Status ConvertToExtension(const google::protobuf::Message& message,
                                ExtensionLike* extension) {
  const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
  FHIR_RETURN_IF_ERROR(internal::ValidateExtension(descriptor));

  extension->mutable_url()->set_value(GetStructureDefinitionUrl(descriptor));

  // Carry over the id field if present.
  const google::protobuf::FieldDescriptor* id_field =
      descriptor->field_count() > 1 && descriptor->field(0)->name() == "id"
          ? descriptor->field(0)
          : nullptr;
  const google::protobuf::Reflection* reflection = message.GetReflection();
  if (id_field != nullptr && reflection->HasField(message, id_field)) {
    extension->mutable_id()->CopyFrom(
        reflection->GetMessage(message, id_field));
  }

  const std::vector<const google::protobuf::FieldDescriptor*> value_fields =
      internal::FindValueFields(descriptor);
  if (value_fields.empty()) {
    return ::absl::InvalidArgumentError(
        absl::StrCat("Extension has no value fields: ", descriptor->name()));
  }
  if (internal::IsSimpleExtension(descriptor)) {
    const google::protobuf::FieldDescriptor* value_field = value_fields[0];
    FHIR_RETURN_IF_ERROR(internal::CheckIsMessage(value_field));
    if (reflection->HasField(message, value_field)) {
      return internal::AddValueToExtension(
          reflection->GetMessage(message, value_field), extension,
          IsChoiceType(value_field));
    } else {
      // TODO(b/152902402): Invalid FHIR; throw an error here?
      return absl::OkStatus();
    }
  } else {
    return internal::AddFieldsToExtension(message, extension);
  }
}

}  // namespace google::fhir::profiled

#endif  // GOOGLE_FHIR_PROFILED_EXTENSIONS_H_
