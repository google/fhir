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

#include "google/protobuf/descriptor.h"
#include "google/fhir/status/status.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/platform/logging.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;
using ::google::fhir::stu3::proto::Extension;
using ::tensorflow::Status;

namespace {

Status AddFieldsToExtension(const google::protobuf::Message& message,
                            Extension* extension);

Status AddValueToExtension(const google::protobuf::Message& message,
                           Extension* extension) {
  const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
  const google::protobuf::Reflection* extension_value_reflection =
      extension->value().GetReflection();
  static const google::protobuf::OneofDescriptor* value =
      Extension::Value::descriptor()->FindOneofByName("value");
  CHECK(value != nullptr);
  for (int i = 0; i < value->field_count(); i++) {
    if (value->field(i)->message_type()->full_name() ==
        descriptor->full_name()) {
      extension_value_reflection
          ->MutableMessage(extension->mutable_value(), value->field(i))
          ->CopyFrom(message);
      return Status::OK();
    }
  }
  // Fall back to adding individual fields as sub-extensions.
  return AddFieldsToExtension(message, extension);
}

Status AddFieldsToExtension(const google::protobuf::Message& message,
                            Extension* extension) {
  const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
  const google::protobuf::Reflection* reflection = message.GetReflection();
  std::vector<const google::protobuf::FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  for (const auto* field : fields) {
    if (field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      return ::tensorflow::errors::InvalidArgument(absl::StrCat(
          descriptor->full_name(), " is not a FHIR extension type"));
    }
    // Add submessages to nested extensions.
    if (field->is_repeated()) {
      for (int j = 0; j < reflection->FieldSize(message, field); j++) {
        Extension* child = extension->add_extension();
        child->mutable_url()->set_value(field->name());
        TF_RETURN_IF_ERROR(AddValueToExtension(
            reflection->GetRepeatedMessage(message, field, j), child));
      }
    } else {
      Extension* child = extension->add_extension();
      child->mutable_url()->set_value(field->name());
      TF_RETURN_IF_ERROR(
          AddValueToExtension(reflection->GetMessage(message, field), child));
    }
  }
  return Status::OK();
}

Status ValueToMessage(const Extension& extension, google::protobuf::Message* message,
                      const google::protobuf::FieldDescriptor* field) {
  const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
  if (field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    return ::tensorflow::errors::InvalidArgument(
        absl::StrCat(descriptor->full_name(), " is not a FHIR extension type"));
  }
  // If there's a value, there can not be any extensions set.
  if (extension.extension_size() > 0) {
    return ::tensorflow::errors::InvalidArgument(
        absl::StrCat("Invalid extension: ", extension.DebugString()));
  }
  // Value types must match.
  // TODO(sundberg): optimize
  static const google::protobuf::OneofDescriptor* value_oneof =
      Extension::Value::descriptor()->FindOneofByName("value");
  const google::protobuf::Reflection* message_reflection = message->GetReflection();
  const Extension::Value value = extension.value();
  const google::protobuf::Reflection* value_reflection = value.GetReflection();
  for (int i = 0; i < value_oneof->field_count(); i++) {
    if (value_reflection->HasField(value, value_oneof->field(i)) &&
        value_oneof->field(i)->cpp_type() ==
            google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE &&
        value_oneof->field(i)->message_type()->full_name() ==
            field->message_type()->full_name()) {
      if (field->is_repeated()) {
        message_reflection->AddMessage(message, field)
            ->CopyFrom(
                value_reflection->GetMessage(extension, value_oneof->field(i)));
      } else {
        message_reflection->MutableMessage(message, field)
            ->CopyFrom(
                value_reflection->GetMessage(value, value_oneof->field(i)));
      }
      return Status::OK();
    }
  }
  return ::tensorflow::errors::InvalidArgument(absl::StrCat(
      "Missing expected value of type ", field->message_type()->full_name(),
      " in extension ", extension.DebugString()));
}

}  // namespace

Status ConvertToExtension(const google::protobuf::Message& message,
                          Extension* extension) {
  const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
  if (!descriptor->options().HasExtension(stu3::proto::fhir_extension_url)) {
    return ::tensorflow::errors::InvalidArgument(
        absl::StrCat(descriptor->full_name(), " is not a FHIR extension type"));
  }
  extension->mutable_url()->set_value(
      descriptor->options().GetExtension(stu3::proto::fhir_extension_url));

  // TODO(sundberg): also check that the field is a primitive type.
  bool is_single_value_extension = descriptor->field_count() == 1 &&
                                   !descriptor->field(0)->is_repeated() &&
                                   descriptor->field(0)->cpp_type() ==
                                       google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE;
  if (is_single_value_extension) {
    const google::protobuf::Reflection* reflection = message.GetReflection();
    if (reflection->HasField(message, descriptor->field(0))) {
      return AddValueToExtension(
          reflection->GetMessage(message, descriptor->field(0)), extension);

    } else {
      return Status::OK();
    }
  } else {
    return AddFieldsToExtension(message, extension);
  }
}

Status ExtensionToMessage(const Extension& extension,
                          google::protobuf::Message* message) {
  const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
  const google::protobuf::Reflection* reflection = message->GetReflection();

  if (extension.value().value_case() != Extension::Value::VALUE_NOT_SET) {
    // This is a simple extension, with only one value.
    if (descriptor->field_count() != 1 || descriptor->field(0)->is_repeated()) {
      return ::tensorflow::errors::InvalidArgument(absl::StrCat(
          descriptor->full_name(), " is not a FHIR extension type"));
    }
    return ValueToMessage(extension, message, descriptor->field(0));
  }

  for (const Extension& inner : extension.extension()) {
    const google::protobuf::FieldDescriptor* field =
        descriptor->FindFieldByName(inner.url().value());

    if (field == nullptr) {
      return ::tensorflow::errors::InvalidArgument(
          absl::StrCat("Message of type ", descriptor->full_name(),
                       " has no field with name ", inner.url().value()));
    }

    if (inner.value().value_case() != Extension::Value::VALUE_NOT_SET) {
      TF_RETURN_IF_ERROR(ValueToMessage(inner, message, field));
    } else {
      google::protobuf::Message* child;
      if (field->is_repeated()) {
        child = reflection->AddMessage(message, field);
      } else if (reflection->HasField(*message, field) ||
                 inner.extension_size() > 0) {
        return ::tensorflow::errors::AlreadyExists(
            absl::StrCat("Unexpected repeated value for tag ", field->name(),
                         " in extension ", inner.DebugString()));
      } else {
        child = reflection->MutableMessage(message, field);
      }
      TF_RETURN_IF_ERROR(ExtensionToMessage(inner, child));
    }
  }
  return Status::OK();
}

void ClearTypedExtensions(const google::protobuf::Descriptor* descriptor,
                          google::protobuf::Message* message) {
  if (!descriptor->options().HasExtension(stu3::proto::fhir_extension_url)) {
    LOG(ERROR) << descriptor->full_name() << " is not a FHIR extension type";
    return;
  }
  const string url =
      descriptor->options().GetExtension(stu3::proto::fhir_extension_url);

  const google::protobuf::Reflection* reflection = message->GetReflection();
  const google::protobuf::FieldDescriptor* field =
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
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
