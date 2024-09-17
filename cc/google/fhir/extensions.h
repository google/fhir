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

#include <optional>
#include <string>
#include <vector>


#include "absl/container/flat_hash_map.h"
#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codes.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/status/statusor.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "google/protobuf/message.h"

namespace google::fhir {

// Util for safely adding an extension of type `CoreExtensionType` to proto
// `message`.
// Returns a status error if `message` does not  have a repeated extension field
// with type `CoreExtensionType`.
template <class CoreExtensionType>
absl::StatusOr<CoreExtensionType*> AddExtension(google::protobuf::Message* message);

// Provides utility functions for dealing with FHIR extensions.
absl::Status ClearExtensionsWithUrl(const std::string& url,
                                    ::google::protobuf::Message* message);

const std::string& GetExtensionUrl(const google::protobuf::Message& extension,
                                   std::string* scratch);

// Given a datatype message (E.g., String, Code, Boolean, etc.),
// finds the appropriate field on the target extension and sets it.
// Returns ::absl::InvalidArgumentError if there's no matching oneof
// type on the extension for the message.
template <class CoreExtensionType>
absl::Status SetDatatypeOnExtension(const ::google::protobuf::Message& datum,
                                    CoreExtensionType* extension);

// Locates and returns a pointer to the only extension on `element` with url
// matching `url`. Returns nullptr if no matching extension is found. Returns
// InvalidArgument if `element` does not have a valid extension field, or if
// multiple matching urls are present.
//
// This is templated on core Extension type and does a safe cast of the located
// extension to that type (returning an InvalidArgument if the type is
// incorrect).
template <typename CoreExtensionType>
absl::StatusOr<const CoreExtensionType*> GetOnlyMatchingExtension(
    absl::string_view url, const google::protobuf::Message& element);

// Returns a vector of all extensions with a given url in a given FHIR element.
// This is templated on core (i.e., unprofiled) extension proto type, e.g.,
// google::fhir::r4::core::Extension.
// Throws an InvalidArgument error if the extension version found does not match
// the template parameter.
template <typename CoreExtensionType>
absl::StatusOr<std::vector<const CoreExtensionType*>> GetAllMatchingExtensions(
    absl::string_view url, const google::protobuf::Message& element);

// Given a simple extension url, simple extension type T, and a FHIR element,
// returns a vector pointing to all simple extension values with that url and
// type found on the FHIR element. Returns an InvalidArgument error if any
// extensions have the matching url but incorrect datatype.
template <typename T>
absl::StatusOr<std::vector<const T*>> GetAllSimpleExtensionValues(
    absl::string_view url, const google::protobuf::Message& element);

// Locates the only extension on `element` with url matching `url`
// and returns a pointer to the set value element, or nullptr if no matching
// extension is found. Returns an InvalidArgument error if the set value
// element is not of type T, or if `element` doesn't have a valid extension
// field, or the value element is not set (e.g., in a complex extension), or
// if multiple matching urls are present.
template <typename T>
absl::StatusOr<const T*> GetOnlySimpleExtensionValue(
    absl::string_view url, const google::protobuf::Message& element);

namespace internal {

// Given a proto `element`, returs the FieldDescriptor for the extension field,
// if one is found.
// Returns a nullptr if there is no "extension" field is present, or
// it is not a repeated message.
const google::protobuf::FieldDescriptor* GetExtensionField(
    const google::protobuf::Message& element);

absl::StatusOr<std::vector<const google::protobuf::Message*>>
GetAllUntypedMatchingExtensions(absl::string_view url,
                                const google::protobuf::Message& element);

absl::StatusOr<const google::protobuf::Message*> GetSimpleExtensionValueFromExtension(
    const google::protobuf::Message& extension);

// For the Extension message in the template, returns the appropriate field on
// the extension for a descriptor of a given type.
// For example, given a Base64Binary, this will return the base64_binary field
// on an extension.
template <typename ExtensionType>
std::optional<const ::google::protobuf::FieldDescriptor*> GetExtensionValueFieldByType(
    const ::google::protobuf::Descriptor* field_type) {
  static const absl::flat_hash_map<
      std::string,
      const ::google::protobuf::FieldDescriptor*>* extension_value_fields_by_type = []() {
    absl::flat_hash_map<std::string, const ::google::protobuf::FieldDescriptor*>* map =
        new absl::flat_hash_map<std::string, const ::google::protobuf::FieldDescriptor*>;

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
             ? std::optional<const ::google::protobuf::FieldDescriptor*>()
             : std::make_optional(iter->second);
}

}  // namespace internal

template <class CoreExtensionType>
absl::StatusOr<CoreExtensionType*> AddExtension(google::protobuf::Message* message) {
  const google::protobuf::FieldDescriptor* extension_field =
      internal::GetExtensionField(*message);

  if (extension_field == nullptr) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Cannot add extension to message type: ", message->GetTypeName()));
  }

  if (extension_field->message_type()->full_name() !=
      CoreExtensionType::descriptor()->full_name()) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Invalid extension type on `$0`: expected `$1`, found `$2`",
        message->GetTypeName(), CoreExtensionType::descriptor()->full_name(),
        extension_field->message_type()->full_name()));
  }

  return dynamic_cast<CoreExtensionType*>(
      message->GetReflection()->AddMessage(message, extension_field));
}

template <class CoreExtensionType>
absl::Status SetDatatypeOnExtension(const ::google::protobuf::Message& datum,
                                    CoreExtensionType* extension) {
  const ::google::protobuf::Descriptor* descriptor = datum.GetDescriptor();
  auto value_field_optional =
      internal::GetExtensionValueFieldByType<CoreExtensionType>(descriptor);
  if (value_field_optional.has_value()) {
    extension->value()
        .GetReflection()
        ->MutableMessage(extension->mutable_value(), *value_field_optional)
        ->CopyFrom(datum);
    return absl::OkStatus();
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
  return ::absl::InvalidArgumentError(absl::StrCat(
      descriptor->full_name(), " is not a valid value type on Extension."));
}

template <typename CoreExtensionType>
absl::StatusOr<std::vector<const CoreExtensionType*>> GetAllMatchingExtensions(
    absl::string_view url, const google::protobuf::Message& element) {
  const google::protobuf::FieldDescriptor* extension_field =
      internal::GetExtensionField(element);
  if (extension_field == nullptr) {
    return std::vector<const CoreExtensionType*>();
  }
  if (extension_field->message_type()->full_name() !=
      CoreExtensionType::descriptor()->full_name()) {
    return absl::InvalidArgumentError(
        absl::Substitute("GetAllMatchingExtensions requested with type `$0`, "
                         "but found extensions with type `$1`.",
                         CoreExtensionType::descriptor()->full_name(),
                         extension_field->message_type()->full_name()));
  }

  std::vector<const CoreExtensionType*> matches;
  std::string scratch;
  for (int i = 0;
       i < element.GetReflection()->FieldSize(element, extension_field); i++) {
    const google::protobuf::Message& extension =
        element.GetReflection()->GetRepeatedMessage(element, extension_field,
                                                    i);
    if (GetExtensionUrl(extension, &scratch) == url) {
      matches.push_back(dynamic_cast<const CoreExtensionType*>(&extension));
    }
  }
  return matches;
}

// Locates and returns a pointer to the only extension on `element` with url
// matching `url`. Returns nullptr if no matching extension is found. Returns
// InvalidArgument if `element` does not have a valid extension field, or if
// multiple matching urls are present.
//
// This is templated on core Extension type and does a safe cast of the
// located extension to that type (returning an InvalidArgument if the type is
// incorrect).
template <typename CoreExtensionType>
absl::StatusOr<const CoreExtensionType*> GetOnlyMatchingExtension(
    absl::string_view url, const google::protobuf::Message& element) {
  FHIR_ASSIGN_OR_RETURN(
      std::vector<const CoreExtensionType*> matches,
      GetAllMatchingExtensions<CoreExtensionType>(url, element));
  if (matches.empty()) return nullptr;
  if (matches.size() > 1) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Expected maximum one extension with url: $0.  Found: $1", url,
        matches.size()));
  }

  return dynamic_cast<const CoreExtensionType*>(matches.front());
}

// Locates the only extension on `element` with url matching `url`
// and returns a pointer to the set value element, or nullptr if no matching
// extension is found. Returns an InvalidArgument error if the set value
// element is not of type T, or if `element` doesn't have a valid extension
// field, or the value element is not set (e.g., in a complex extension), or
// if multiple matching urls are present.
template <typename T>
absl::StatusOr<const T*> GetOnlySimpleExtensionValue(
    absl::string_view url, const google::protobuf::Message& element) {
  FHIR_ASSIGN_OR_RETURN(std::vector<const T*> all_extensions,
                        GetAllSimpleExtensionValues<T>(url, element));

  if (all_extensions.empty()) return nullptr;

  if (all_extensions.size() > 1) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Expected maximum one extension with url: $0.  Found: $1", url,
        all_extensions.size()));
  }

  return all_extensions.front();
}

template <typename T>
absl::StatusOr<std::vector<const T*>> GetAllSimpleExtensionValues(
    absl::string_view url, const google::protobuf::Message& element) {
  FHIR_ASSIGN_OR_RETURN(
      std::vector<const google::protobuf::Message*> extensions,
      internal::GetAllUntypedMatchingExtensions(url, element));
  if (extensions.empty()) return std::vector<const T*>();

  std::vector<const T*> values;
  for (const google::protobuf::Message* extension : extensions) {
    FHIR_ASSIGN_OR_RETURN(
        const google::protobuf::Message* value,
        internal::GetSimpleExtensionValueFromExtension(*extension));

    if (value->GetDescriptor()->full_name() != T::descriptor()->full_name()) {
      return absl::InvalidArgumentError(
          absl::Substitute("Invalid value type on extension with url: $0.  "
                           "Expected $1 but found $2",
                           url, T::descriptor()->full_name(),
                           value->GetDescriptor()->full_name()));
    }
    values.push_back(dynamic_cast<const T*>(value));
  }

  return values;
}

}  // namespace google::fhir

#endif  // GOOGLE_FHIR_EXTENSIONS_H_
