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

#include "google/fhir/stu3/codes.h"

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/fhir/stu3/proto_util.h"
#include "google/fhir/stu3/util.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"

namespace google {
namespace fhir {
namespace stu3 {

using ::google::fhir::stu3::proto::Code;
using ::google::protobuf::Descriptor;
using ::google::protobuf::EnumDescriptor;
using ::google::protobuf::EnumValueDescriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Reflection;
using ::tensorflow::errors::InvalidArgument;

::google::fhir::Status ConvertToTypedCode(
    const google::fhir::stu3::proto::Code& generic_code,
    google::protobuf::Message* target) {
  const Descriptor* target_descriptor = target->GetDescriptor();
  const Reflection* target_reflection = target->GetReflection();

  // If there is no valueset url, assume we're just copying a plain old Code
  if (!HasValueset(target_descriptor)) {
    if (!IsMessageType<Code>(*target)) {
      return InvalidArgument("Type ", target_descriptor->full_name(),
                             " is not a valid FHIR code type.");
    }
    target->CopyFrom(generic_code);
  }

  // Handle specialized codes.
  if (generic_code.has_id()) {
    target_reflection
        ->MutableMessage(target, target_descriptor->FindFieldByName("id"))
        ->CopyFrom(generic_code.id());
  }
  const FieldDescriptor* extension_field =
      target_descriptor->FindFieldByName("extension");
  if (!extension_field) {
    return InvalidArgument("Type ", target_descriptor->full_name(),
                           " has no extension field");
  }
  for (const Extension& extension : generic_code.extension()) {
    target_reflection->AddMessage(target, extension_field)->CopyFrom(extension);
  }
  if (generic_code.value().empty()) {
    // We're done if there is no value to parse.
    return Status::OK();
  }

  const FieldDescriptor* target_value_field =
      target_descriptor->FindFieldByName("value");
  if (!target_value_field) {
    return InvalidArgument("Type ", target_descriptor->full_name(),
                           " has no value field");
  }
  // If target it a string, just set it from the wrapped value.
  if (target_value_field->type() == FieldDescriptor::Type::TYPE_STRING) {
    target_reflection->SetString(target, target_value_field,
                                 generic_code.value());
    return Status::OK();
  }
  // If target field is not string, it has to be an Enum.
  if (target_value_field->type() != FieldDescriptor::Type::TYPE_ENUM) {
    return InvalidArgument("Invalid target message: ",
                           target_descriptor->full_name());
  }

  // Try to find the Enum value by name (with some common substitutions).
  string target_enum_name = generic_code.value();
  for (std::string::size_type i = 0; i < target_enum_name.length(); i++) {
    target_enum_name[i] = std::toupper(target_enum_name[i]);
  }
  std::replace(target_enum_name.begin(), target_enum_name.end(), '-', '_');
  const EnumValueDescriptor* target_enum_value =
      target_value_field->enum_type()->FindValueByName(target_enum_name);
  if (target_enum_value != nullptr) {
    target_reflection->SetEnum(target, target_value_field, target_enum_value);
    return Status::OK();
  }

  // Finally, some codes had to be renamed to make them valid enum values.
  // Iterate through all target enum values, and ook for the
  // "fhir_original_code" annotation for the original name.
  const EnumDescriptor* target_enum_descriptor =
      target_value_field->enum_type();
  for (int i = 0; i < target_enum_descriptor->value_count(); i++) {
    const EnumValueDescriptor* target_value = target_enum_descriptor->value(i);
    if (target_value->options().HasExtension(
            ::google::fhir::stu3::proto::fhir_original_code) &&
        target_value->options().GetExtension(
            ::google::fhir::stu3::proto::fhir_original_code) ==
            generic_code.value()) {
      target_reflection->SetEnum(target, target_value_field, target_value);
      return Status::OK();
    }
  }

  return InvalidArgument("Failed to convert to ",
                         target_descriptor->full_name(), ": \"",
                         generic_code.value(), "\" is not a valid enum entry");
}

::google::fhir::Status ConvertToGenericCode(
    const google::protobuf::Message& typed_code,
    google::fhir::stu3::proto::Code* generic_code) {
  if (IsMessageType<Code>(typed_code)) {
    generic_code->CopyFrom(typed_code);
    return Status::OK();
  }

  const Descriptor* descriptor = typed_code.GetDescriptor();
  const Reflection* reflection = typed_code.GetReflection();
  if (!HasValueset(descriptor)) {
    return InvalidArgument("Type ", descriptor->full_name(),
                           " is not a FHIR code type.");
  }

  // Copy the Element parts
  const FieldDescriptor* id_field = descriptor->FindFieldByName("id");
  if (reflection->HasField(typed_code, id_field)) {
    generic_code->mutable_id()->CopyFrom(
        reflection->GetMessage(typed_code, id_field));
  }
  const FieldDescriptor* extension_field =
      descriptor->FindFieldByName("extension");
  if (!extension_field) {
    return InvalidArgument("Type ", descriptor->full_name(),
                           " has no extension field");
  }
  for (int i = 0; i < reflection->FieldSize(typed_code, extension_field); i++) {
    generic_code->add_extension()->CopyFrom(
        reflection->GetRepeatedMessage(typed_code, extension_field, i));
  }

  const FieldDescriptor* value_field = descriptor->FindFieldByName("value");
  if (!value_field) {
    return InvalidArgument("Type ", descriptor->full_name(),
                           " has no value field");
  }
  if (!reflection->HasField(typed_code, value_field)) {
    return Status::OK();
  }

  if (value_field->type() == FieldDescriptor::Type::TYPE_STRING) {
    generic_code->set_value(reflection->GetString(typed_code, value_field));
    return Status::OK();
  }

  if (value_field->type() != FieldDescriptor::TYPE_ENUM) {
    return InvalidArgument("Invalid Code type: ", descriptor->full_name());
  }

  const ::google::protobuf::EnumValueDescriptor* enum_descriptor =
      reflection->GetEnum(typed_code, value_field);
  if (enum_descriptor->options().HasExtension(
          ::google::fhir::stu3::proto::fhir_original_code)) {
    generic_code->set_value(enum_descriptor->options().GetExtension(
        ::google::fhir::stu3::proto::fhir_original_code));
    return Status::OK();
  }

  string code_string = enum_descriptor->name();
  std::transform(code_string.begin(), code_string.end(), code_string.begin(),
                 tolower);
  std::replace(code_string.begin(), code_string.end(), '_', '-');
  generic_code->set_value(code_string);
  return Status::OK();
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
