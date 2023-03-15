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
#include "google/fhir/fhir_types.h"
#include "google/fhir/util.h"
#include "proto/google/fhir/proto/annotations.pb.h"

namespace google {
namespace fhir {

using ::absl::InvalidArgumentError;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

absl::Status ClearExtensionsWithUrl(const std::string& url, Message* message) {
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

std::string GetInlinedExtensionUrl(const FieldDescriptor* field) {
  return field->options().HasExtension(
             ::google::fhir::proto::fhir_inlined_extension_url)
             ? field->options().GetExtension(
                   ::google::fhir::proto::fhir_inlined_extension_url)
             : field->json_name();
}

namespace {
absl::StatusOr<std::vector<const google::protobuf::Message*>>
GetAllUntypedMatchingExtensions(absl::string_view url,
                                const google::protobuf::Message& element) {
  const FieldDescriptor* extension_field =
      element.GetDescriptor()->FindFieldByName("extension");

  if (extension_field == nullptr) {
    // This element doesn't support extensions.
    return std::vector<const google::protobuf::Message*>();
  }
  if (extension_field->message_type() == nullptr ||
      !extension_field->is_repeated()) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Invalid extension field on ", element.GetDescriptor()->full_name()));
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

}  // namespace

namespace internal {
bool IsSimpleExtension(const ::google::protobuf::Descriptor* descriptor) {
  // Simple extensions have only a single, non-repeated value field.
  // However, it is also possible to have a complex extension with only
  // a single non-repeated field.  In that case, is_complex_extension is used to
  // disambiguate.
  const std::vector<const FieldDescriptor*> value_fields =
      internal::FindValueFields(descriptor);
  return IsProfileOfExtension(descriptor) && value_fields.size() == 1 &&
         !value_fields.front()->is_repeated() &&
         !descriptor->options().GetExtension(proto::is_complex_extension);
}

absl::Status CheckIsMessage(const FieldDescriptor* field) {
  if (field->type() != FieldDescriptor::Type::TYPE_MESSAGE) {
    return InvalidArgumentError(absl::StrCat(
        "Encountered unexpected proto primitive: ", field->full_name(),
        ".  Should be FHIR type"));
  }
  return absl::OkStatus();
}

std::vector<const FieldDescriptor*> FindValueFields(
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

absl::Status ValidateExtension(const Descriptor* descriptor) {
  if (!IsProfileOfExtension(descriptor)) {
    return InvalidArgumentError(
        absl::StrCat(descriptor->full_name(), " is not a FHIR extension type"));
  }
  if (!descriptor->options().HasExtension(
          ::google::fhir::proto::fhir_structure_definition_url)) {
    return InvalidArgumentError(
        absl::StrCat(descriptor->full_name(),
                     " is not a valid FHIR extension type: No "
                     "fhir_structure_definition_url."));
  }
  return absl::OkStatus();
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

absl::StatusOr<const google::protobuf::Message*> GetOnlyUntypedMatchingExtension(
    absl::string_view url, const google::protobuf::Message& element) {
  FHIR_ASSIGN_OR_RETURN(std::vector<const google::protobuf::Message*> matches,
                        GetAllUntypedMatchingExtensions(url, element));
  if (matches.empty()) return nullptr;
  if (matches.size() > 1) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Expected maximum one extension with url: $0.  Found: $1", url,
        matches.size()));
  }
  return matches.front();
}

}  // namespace internal

}  // namespace fhir
}  // namespace google
