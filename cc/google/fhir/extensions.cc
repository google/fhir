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

#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/util.h"
#include "proto/google/fhir/proto/annotations.pb.h"

namespace google {
namespace fhir {

using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

absl::Status ClearExtensionsWithUrl(const std::string& url, Message* message) {
  const Reflection* reflection = message->GetReflection();
  const FieldDescriptor* extension_field =
      internal::GetExtensionField(*message);
  if (extension_field == nullptr) {
    return absl::OkStatus();
  }
  google::protobuf::RepeatedPtrField<Message>* repeated_ptr_field =
      reflection->MutableRepeatedPtrField<Message>(message, extension_field);
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
  return absl::OkStatus();
}

const std::string& GetExtensionUrl(const google::protobuf::Message& extension,
                                   std::string* scratch) {
  const Message& url_message = extension.GetReflection()->GetMessage(
      extension, extension.GetDescriptor()->FindFieldByName("url"));
  return url_message.GetReflection()->GetStringReference(
      url_message, url_message.GetDescriptor()->FindFieldByName("value"),
      scratch);
}

namespace internal {

const FieldDescriptor* GetExtensionField(const google::protobuf::Message& element) {
  const FieldDescriptor* extension_field =
      element.GetDescriptor()->FindFieldByName("extension");

  if (extension_field == nullptr ||
      extension_field->message_type() == nullptr ||
      !extension_field->is_repeated()) {
    return nullptr;
  }
  return extension_field;
}

absl::StatusOr<std::vector<const google::protobuf::Message*>>
GetAllUntypedMatchingExtensions(absl::string_view url,
                                const google::protobuf::Message& element) {
  const FieldDescriptor* extension_field = GetExtensionField(element);

  if (extension_field == nullptr) {
    // This element doesn't support extensions.
    return std::vector<const google::protobuf::Message*>();
  }

  std::string scratch;
  std::vector<const google::protobuf::Message*> matches;
  for (int i = 0;
       i < element.GetReflection()->FieldSize(element, extension_field); i++) {
    const Message& extension = element.GetReflection()->GetRepeatedMessage(
        element, extension_field, i);
    if (GetExtensionUrl(extension, &scratch) == url) {
      matches.push_back(&extension);
    }
  }
  return matches;
}

absl::StatusOr<const Message*> GetSimpleExtensionValueFromExtension(
    const google::protobuf::Message& extension) {
  const google::protobuf::FieldDescriptor* value_field =
      extension.GetDescriptor()->FindFieldByName("value");
  if (value_field == nullptr || value_field->message_type() == nullptr) {
    return absl::InvalidArgumentError(
        absl::StrCat("Value field missing or invalid on extension: ",
                     extension.GetDescriptor()->full_name()));
  }
  const google::protobuf::Message& value_element =
      extension.GetReflection()->GetMessage(extension, value_field);

  const google::protobuf::OneofDescriptor* value_oneof =
      value_element.GetDescriptor()->FindOneofByName("choice");

  if (value_oneof == nullptr) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Not a valid extension: ", extension.GetDescriptor()->full_name()));
  }
  const google::protobuf::FieldDescriptor* set_field =
      value_element.GetReflection()->GetOneofFieldDescriptor(value_element,
                                                             value_oneof);
  if (set_field == nullptr) {
    return absl::InvalidArgumentError("Value field not set on extension.");
  }
  const google::protobuf::Descriptor* set_type = set_field->message_type();

  if (set_type == nullptr) {
    return absl::InvalidArgumentError(
        absl::StrCat("Invalid primitive value field on extension ",
                     extension.GetDescriptor()->full_name()));
  }

  return &value_element.GetReflection()->GetMessage(value_element, set_field);
}

}  // namespace internal

}  // namespace fhir
}  // namespace google
