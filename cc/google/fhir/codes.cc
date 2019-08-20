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

#include "google/fhir/codes.h"

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/fhir/annotations.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "proto/stu3/codes.pb.h"
#include "proto/stu3/datatypes.pb.h"

namespace google {
namespace fhir {

using ::google::fhir::stu3::proto::Code;
using ::google::fhir::stu3::proto::ResourceTypeCode;
using ::google::protobuf::Descriptor;
using ::google::protobuf::EnumDescriptor;
using ::google::protobuf::EnumValueDescriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Reflection;
using ::tensorflow::errors::InvalidArgument;

namespace {

string TitleCaseToUpperUnderscores(const string& src) {
  string dst;
  for (auto iter = src.begin(); iter != src.end(); ++iter) {
    if (absl::ascii_isupper(*iter) && iter != src.begin()) {
      dst.push_back('_');
    }
    dst.push_back(absl::ascii_toupper(*iter));
  }
  return dst;
}

template <class CodeLike>
::google::fhir::Status ConvertToTypedCodeInternal(const CodeLike& generic_code,
                                                  google::protobuf::Message* typed_code) {
  const Descriptor* target_descriptor = typed_code->GetDescriptor();
  const Reflection* target_reflection = typed_code->GetReflection();

  // If there is no valueset url, assume we're just copying a plain old Code
  if (!HasValueset(target_descriptor)) {
    if (!AreSameMessageType(generic_code, *typed_code)) {
      return InvalidArgument("Type ", target_descriptor->full_name(),
                             " is not a valid FHIR code type.");
    }
    typed_code->CopyFrom(generic_code);
  }

  // Handle specialized codes.
  if (generic_code.has_id()) {
    target_reflection
        ->MutableMessage(typed_code, target_descriptor->FindFieldByName("id"))
        ->CopyFrom(generic_code.id());
  }
  const FieldDescriptor* extension_field =
      target_descriptor->FindFieldByName("extension");
  if (!extension_field) {
    return InvalidArgument("Type ", target_descriptor->full_name(),
                           " has no extension field");
  }
  for (const auto& extension : generic_code.extension()) {
    target_reflection->AddMessage(typed_code, extension_field)
        ->CopyFrom(extension);
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
    target_reflection->SetString(typed_code, target_value_field,
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
    target_reflection->SetEnum(typed_code, target_value_field,
                               target_enum_value);
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
            ::google::fhir::proto::fhir_original_code) &&
        target_value->options().GetExtension(
            ::google::fhir::proto::fhir_original_code) ==
            generic_code.value()) {
      target_reflection->SetEnum(typed_code, target_value_field, target_value);
      return Status::OK();
    }
  }

  return InvalidArgument("Failed to convert to ",
                         target_descriptor->full_name(), ": \"",
                         generic_code.value(), "\" is not a valid enum entry");
}

template <class CodeLike>
::google::fhir::Status ConvertToGenericCodeInternal(
    const google::protobuf::Message& typed_code, CodeLike* generic_code) {
  if (IsMessageType<CodeLike>(typed_code)) {
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
  FHIR_RETURN_IF_ERROR(CopyCommonField(typed_code, generic_code, "id"));
  FHIR_RETURN_IF_ERROR(CopyCommonField(typed_code, generic_code, "extension"));

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
          ::google::fhir::proto::fhir_original_code)) {
    generic_code->set_value(enum_descriptor->options().GetExtension(
        ::google::fhir::proto::fhir_original_code));
    return Status::OK();
  }

  string code_string = enum_descriptor->name();
  std::transform(code_string.begin(), code_string.end(), code_string.begin(),
                 tolower);
  std::replace(code_string.begin(), code_string.end(), '_', '-');
  generic_code->set_value(code_string);
  return Status::OK();
}
template <typename CodingLike,
          typename CodeLike = FHIR_DATATYPE(CodingLike, code)>
Status ConvertToTypedCodingInternal(const CodingLike& generic_coding,
                                    google::protobuf::Message* typed_coding) {
  if (IsMessageType<CodingLike>(*typed_coding)) {
    typed_coding->CopyFrom(generic_coding);
    return Status::OK();
  }

  // Copy the Element parts
  FHIR_RETURN_IF_ERROR(CopyCommonField(generic_coding, typed_coding, "id"));
  FHIR_RETURN_IF_ERROR(
      CopyCommonField(generic_coding, typed_coding, "extension"));
  FHIR_RETURN_IF_ERROR(
      CopyCommonField(generic_coding, typed_coding, "version"));
  FHIR_RETURN_IF_ERROR(
      CopyCommonField(generic_coding, typed_coding, "display"));
  FHIR_RETURN_IF_ERROR(
      CopyCommonField(generic_coding, typed_coding, "user_selected"));

  const Descriptor* descriptor = typed_coding->GetDescriptor();
  const Reflection* reflection = typed_coding->GetReflection();
  const FieldDescriptor* code_field = descriptor->FindFieldByName("code");

  if (!code_field) {
    return InvalidArgument("Cannot convert ",
                           CodingLike::descriptor()->full_name(), " to typed ",
                           descriptor->full_name(), ": Must have code field.");
  }
  if (HasValueset(code_field->message_type()) &&
      GetValueset(code_field->message_type()) !=
          generic_coding.system().value()) {
    return InvalidArgument(
        "Cannot convert generic coding to typed code ", descriptor->full_name(),
        ": Target has valueset ", GetValueset(descriptor),
        " but generic coding has system ", generic_coding.system().value());
  }
  return ConvertToTypedCodeInternal(
      generic_coding.code(),
      reflection->MutableMessage(typed_coding, code_field));
}

template <typename CodingLike,
          typename CodeLike = FHIR_DATATYPE(CodingLike, code)>
Status ConvertToGenericCodingInternal(const google::protobuf::Message& typed_coding,
                                      CodingLike* generic_coding) {
  if (IsMessageType<CodingLike>(typed_coding)) {
    generic_coding->CopyFrom(typed_coding);
    return Status::OK();
  }

  // Copy the Element parts
  FHIR_RETURN_IF_ERROR(CopyCommonField(typed_coding, generic_coding, "id"));
  FHIR_RETURN_IF_ERROR(
      CopyCommonField(typed_coding, generic_coding, "extension"));
  FHIR_RETURN_IF_ERROR(
      CopyCommonField(typed_coding, generic_coding, "version"));
  FHIR_RETURN_IF_ERROR(
      CopyCommonField(typed_coding, generic_coding, "display"));
  FHIR_RETURN_IF_ERROR(
      CopyCommonField(typed_coding, generic_coding, "user_selected"));

  const Descriptor* descriptor = typed_coding.GetDescriptor();
  const Reflection* reflection = typed_coding.GetReflection();
  const FieldDescriptor* code_field = descriptor->FindFieldByName("code");

  if (!code_field) {
    return InvalidArgument(
        "Cannot convert ", descriptor->full_name(), " to generic ",
        CodingLike::descriptor()->full_name(), ": Must have code.");
  }

  generic_coding->mutable_system()->set_value(
      GetFixedCodingSystem(typed_coding.GetDescriptor()));

  return ConvertToGenericCodeInternal(
      reflection->GetMessage(typed_coding, code_field),
      generic_coding->mutable_code());
}

}  // namespace

::google::fhir::Status ConvertToTypedCode(
    const ::google::fhir::stu3::proto::Code& generic_code,
    google::protobuf::Message* target) {
  return ConvertToTypedCodeInternal(generic_code, target);
}

::google::fhir::Status ConvertToTypedCode(
    const ::google::fhir::r4::proto::Code& generic_code,
    google::protobuf::Message* target) {
  return ConvertToTypedCodeInternal(generic_code, target);
}

::google::fhir::Status ConvertToGenericCode(
    const google::protobuf::Message& typed_code,
    google::fhir::stu3::proto::Code* generic_code) {
  return ConvertToGenericCodeInternal(typed_code, generic_code);
}

::google::fhir::Status ConvertToGenericCode(
    const google::protobuf::Message& typed_code,
    google::fhir::r4::proto::Code* generic_code) {
  return ConvertToGenericCodeInternal(typed_code, generic_code);
}

::google::fhir::Status ConvertToTypedCoding(
    const ::google::fhir::stu3::proto::Coding& generic_coding,
    google::protobuf::Message* typed_coding) {
  return ConvertToTypedCodingInternal(generic_coding, typed_coding);
}

::google::fhir::Status ConvertToGenericCoding(
    const google::protobuf::Message& typed_coding,
    google::fhir::stu3::proto::Coding* generic_coding) {
  return ConvertToGenericCodingInternal(typed_coding, generic_coding);
}

::google::fhir::Status ConvertToTypedCoding(
    const ::google::fhir::r4::proto::Coding& generic_coding,
    google::protobuf::Message* typed_coding) {
  return ConvertToTypedCodingInternal(generic_coding, typed_coding);
}

::google::fhir::Status ConvertToGenericCoding(
    const google::protobuf::Message& typed_coding,
    google::fhir::r4::proto::Coding* generic_coding) {
  return ConvertToGenericCodingInternal(typed_coding, generic_coding);
}

::google::fhir::StatusOr<ResourceTypeCode::Value> GetCodeForResourceType(
    const google::protobuf::Message& resource) {
  const string& enum_string =
      TitleCaseToUpperUnderscores(resource.GetDescriptor()->name());
  ResourceTypeCode::Value value;
  if (ResourceTypeCode::Value_Parse(enum_string, &value)) {
    return value;
  }
  return InvalidArgument("No ResourceTypeCode found for type: ",
                         resource.GetDescriptor()->name());
}

}  // namespace fhir
}  // namespace google
