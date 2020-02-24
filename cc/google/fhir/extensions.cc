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
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/platform/logging.h"

namespace google {
namespace fhir {

using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using ::tensorflow::errors::InvalidArgument;

namespace extensions_internal {

const std::unordered_map<std::string, const ::google::protobuf::FieldDescriptor*>*
GetExtensionValueFieldsMap() {
  static const std::unordered_map<
      std::string,
      const ::google::protobuf::FieldDescriptor*>* extension_value_fields_by_type = []() {
    std::unordered_map<std::string, const ::google::protobuf::FieldDescriptor*>* map =
        new std::unordered_map<std::string, const ::google::protobuf::FieldDescriptor*>;

    const google::protobuf::OneofDescriptor* stu3_value_oneof =
        stu3::proto::Extension::ValueX::descriptor()->FindOneofByName("choice");
    CHECK(stu3_value_oneof != nullptr);
    for (int i = 0; i < stu3_value_oneof->field_count(); i++) {
      const ::google::protobuf::FieldDescriptor* field = stu3_value_oneof->field(i);
      (*map)[field->message_type()->full_name()] = field;
    }
    const google::protobuf::OneofDescriptor* r4_value_oneof =
        r4::core::Extension::ValueX::descriptor()->FindOneofByName("choice");
    CHECK(r4_value_oneof != nullptr);
    for (int i = 0; i < r4_value_oneof->field_count(); i++) {
      const ::google::protobuf::FieldDescriptor* field = r4_value_oneof->field(i);
      (*map)[field->message_type()->full_name()] = field;
    }
    return map;
  }();
  return extension_value_fields_by_type;
}

Status CheckIsMessage(const FieldDescriptor* field) {
  if (field->type() != FieldDescriptor::Type::TYPE_MESSAGE) {
    return InvalidArgument("Encountered unexpected proto primitive: ",
                           field->full_name(), ".  Should be FHIR type");
  }
  return Status::OK();
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

}  // namespace extensions_internal

namespace extensions_lib {

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

std::string GetInlinedExtensionUrl(const FieldDescriptor* field) {
  return field->options().HasExtension(
             ::google::fhir::proto::fhir_inlined_extension_url)
             ? field->options().GetExtension(
                   ::google::fhir::proto::fhir_inlined_extension_url)
             : field->json_name();
}

}  // namespace extensions_lib
}  // namespace fhir
}  // namespace google
