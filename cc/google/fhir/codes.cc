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

#include <unordered_map>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "absl/strings/ascii.h"
#include "absl/synchronization/mutex.h"
#include "google/fhir/annotations.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "proto/stu3/codes.pb.h"
#include "proto/stu3/datatypes.pb.h"

namespace google {
namespace fhir {

using ::google::fhir::Status;
using ::google::fhir::StatusOr;
using ::google::fhir::stu3::proto::Code;
using ::google::fhir::stu3::proto::ResourceTypeCode;
using ::google::protobuf::Descriptor;
using ::google::protobuf::EnumDescriptor;
using ::google::protobuf::EnumValueDescriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using std::unordered_map;
using ::tensorflow::errors::InvalidArgument;

namespace {

std::string TitleCaseToUpperUnderscores(const std::string& src) {
  std::string dst;
  for (auto iter = src.begin(); iter != src.end(); ++iter) {
    if (absl::ascii_isupper(*iter) && iter != src.begin()) {
      dst.push_back('_');
    }
    dst.push_back(absl::ascii_toupper(*iter));
  }
  return dst;
}

template <class CodeLike>
Status ConvertToTypedCodeInternal(const CodeLike& generic_code,
                                  Message* typed_code) {
  const Descriptor* target_descriptor = typed_code->GetDescriptor();
  const Reflection* target_reflection = typed_code->GetReflection();

  if (!(IsProfileOf<Code>(target_descriptor) ||
        HasValueset(target_descriptor))) {
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
  // If target is a string, just set it from the wrapped value.
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
  FHIR_ASSIGN_OR_RETURN(const EnumValueDescriptor* target_enum_value,
                        CodeStringToEnumValue(generic_code.value(),
                                              target_value_field->enum_type()));
  target_reflection->SetEnum(typed_code, target_value_field, target_enum_value);
  return Status::OK();
}

std::string EnumValueToString(const EnumValueDescriptor* enum_value) {
  if (enum_value->options().HasExtension(
          ::google::fhir::proto::fhir_original_code)) {
    return enum_value->options().GetExtension(
        ::google::fhir::proto::fhir_original_code);
  }

  std::string code_string = enum_value->name();
  std::transform(code_string.begin(), code_string.end(), code_string.begin(),
                 tolower);
  std::replace(code_string.begin(), code_string.end(), '_', '-');
  return code_string;
}

template <class CodeLike>
Status ConvertToGenericCodeInternal(const Message& typed_code,
                                    CodeLike* generic_code) {
  if (IsMessageType<CodeLike>(typed_code)) {
    generic_code->CopyFrom(typed_code);
    return Status::OK();
  }

  const Descriptor* descriptor = typed_code.GetDescriptor();
  const Reflection* reflection = typed_code.GetReflection();
  if (!HasValueset(descriptor) && !HasFixedSystem(descriptor)) {
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

  generic_code->set_value(
      EnumValueToString(reflection->GetEnum(typed_code, value_field)));
  return Status::OK();
}

template <typename CodingLike,
          typename CodeLike = FHIR_DATATYPE(CodingLike, code)>
Status ConvertToTypedCodingInternal(const CodingLike& generic_coding,
                                    Message* typed_coding) {
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
  return ConvertToTypedCodeInternal(
      generic_coding.code(),
      reflection->MutableMessage(typed_coding, code_field));
}

template <typename CodingLike,
          typename CodeLike = FHIR_DATATYPE(CodingLike, code)>
Status ConvertToGenericCodingInternal(const Message& typed_coding,
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

  const Message& typed_code = reflection->GetMessage(typed_coding, code_field);

  const std::string& system = GetFixedCodingSystem(typed_code.GetDescriptor());
  if (!system.empty()) {
    // The entire profiled coding can only be from a single system.  Use that.
    generic_coding->mutable_system()->set_value(system);
  } else {
    // There is no single system for the whole coding - look for system
    // annotation on the enum.
    const FieldDescriptor* enum_field =
        typed_code.GetDescriptor()->FindFieldByName("value");
    if (enum_field->type() != FieldDescriptor::Type::TYPE_ENUM) {
      return InvalidArgument(
          "Invalid profiled Coding: missing system information on string code");
    }
    const ::google::protobuf::EnumValueDescriptor* enum_descriptor =
        typed_code.GetReflection()->GetEnum(typed_code, enum_field);
    if (!HasSourceCodeSystem(enum_descriptor)) {
      return InvalidArgument(
          "Invalid profiled Coding: missing system information on enum code");
    }
    generic_coding->mutable_system()->set_value(
        GetSourceCodeSystem(enum_descriptor));
  }

  return ConvertToGenericCodeInternal(typed_code,
                                      generic_coding->mutable_code());
}

template <typename TypedResourceTypeCode>
StatusOr<typename TypedResourceTypeCode::Value> GetCodeForResourceTypeTemplate(
    const Message& resource) {
  const std::string& enum_string =
      TitleCaseToUpperUnderscores(resource.GetDescriptor()->name());
  typename TypedResourceTypeCode::Value value;
  if (TypedResourceTypeCode::Value_Parse(enum_string, &value)) {
    return value;
  }
  return InvalidArgument("No ResourceTypeCode found for type: ",
                         resource.GetDescriptor()->name());
}

}  // namespace

StatusOr<const EnumValueDescriptor*> CodeStringToEnumValue(
    const std::string& code_string, const EnumDescriptor* target_enum_type) {
  // Map from (target_enum_type->full_name() x code_string) -> result
  // for previous runs.
  static auto* memos = new unordered_map<
      std::string, unordered_map<std::string, const EnumValueDescriptor*>>();
  static absl::Mutex memos_mutex;

  // Check for memoized result.  Note we lock the mutex, in case something tries
  // to read this map while it is being written to.
  memos_mutex.ReaderLock();
  const auto memos_for_enum_type = memos->find(target_enum_type->full_name());
  if (memos_for_enum_type != memos->end()) {
    const auto enum_result = memos_for_enum_type->second.find(code_string);
    if (enum_result != memos_for_enum_type->second.end()) {
      memos_mutex.ReaderUnlock();
      return enum_result->second;
    }
  }
  memos_mutex.ReaderUnlock();

  // Try to find the Enum value by name (with some common substitutions).
  std::string enum_case_code_string = absl::AsciiStrToUpper(code_string);
  std::replace(enum_case_code_string.begin(), enum_case_code_string.end(), '-',
               '_');
  const EnumValueDescriptor* target_enum_value =
      target_enum_type->FindValueByName(enum_case_code_string);
  if (target_enum_value != nullptr) {
    absl::MutexLock lock(&memos_mutex);
    (*memos)[target_enum_type->full_name()][code_string] = target_enum_value;
    return target_enum_value;
  }

  // Finally, some codes had to be renamed to make them valid enum values.
  // Iterate through all target enum values, and look for the
  // "fhir_original_code" annotation for the original name.
  for (int i = 0; i < target_enum_type->value_count(); i++) {
    const EnumValueDescriptor* target_value = target_enum_type->value(i);
    if (target_value->options().HasExtension(
            ::google::fhir::proto::fhir_original_code) &&
        target_value->options().GetExtension(
            ::google::fhir::proto::fhir_original_code) == code_string) {
      absl::MutexLock lock(&memos_mutex);
      (*memos)[target_enum_type->full_name()][code_string] = target_value;
      return target_value;
    }
  }

  return InvalidArgument("Failed to convert ", code_string, " to ",
                         target_enum_type->full_name(),
                         ": No matching enum found.");
}

Status ConvertToTypedCode(const ::google::fhir::stu3::proto::Code& generic_code,
                          Message* target) {
  return ConvertToTypedCodeInternal(generic_code, target);
}

Status ConvertToTypedCode(const ::google::fhir::r4::core::Code& generic_code,
                          Message* target) {
  return ConvertToTypedCodeInternal(generic_code, target);
}

Status ConvertToGenericCode(const Message& typed_code,
                            google::fhir::stu3::proto::Code* generic_code) {
  return ConvertToGenericCodeInternal(typed_code, generic_code);
}

Status ConvertToGenericCode(const Message& typed_code,
                            google::fhir::r4::core::Code* generic_code) {
  return ConvertToGenericCodeInternal(typed_code, generic_code);
}

Status ConvertToTypedCoding(
    const ::google::fhir::stu3::proto::Coding& generic_coding,
    Message* typed_coding) {
  return ConvertToTypedCodingInternal(generic_coding, typed_coding);
}

Status ConvertToGenericCoding(
    const Message& typed_coding,
    google::fhir::stu3::proto::Coding* generic_coding) {
  return ConvertToGenericCodingInternal(typed_coding, generic_coding);
}

Status ConvertToTypedCoding(
    const ::google::fhir::r4::core::Coding& generic_coding,
    Message* typed_coding) {
  return ConvertToTypedCodingInternal(generic_coding, typed_coding);
}

Status ConvertToGenericCoding(const Message& typed_coding,
                              google::fhir::r4::core::Coding* generic_coding) {
  return ConvertToGenericCodingInternal(typed_coding, generic_coding);
}

template <>
::google::fhir::StatusOr<::google::fhir::stu3::proto::ResourceTypeCode::Value>
GetCodeForResourceType<::google::fhir::stu3::proto::ResourceTypeCode>(
    const Message& resource) {
  return GetCodeForResourceTypeTemplate<
      ::google::fhir::stu3::proto::ResourceTypeCode>(resource);
}

template <>
::google::fhir::StatusOr<::google::fhir::r4::core::ResourceTypeCode::Value>
GetCodeForResourceType<::google::fhir::r4::core::ResourceTypeCode>(
    const Message& resource) {
  return GetCodeForResourceTypeTemplate<
      ::google::fhir::r4::core::ResourceTypeCode>(resource);
}

::google::fhir::StatusOr<::google::fhir::stu3::proto::ResourceTypeCode::Value>
GetCodeForResourceType(const Message& resource) {
  return GetCodeForResourceTypeTemplate<
      ::google::fhir::stu3::proto::ResourceTypeCode>(resource);
}

// TODO: deprecate most of the API in this file in favor of this
// function.
Status CopyCode(const Message& source, Message* target) {
  const Descriptor* source_descriptor = source.GetDescriptor();
  const Descriptor* target_descriptor = target->GetDescriptor();

  if (source_descriptor->full_name() == target_descriptor->full_name()) {
    target->CopyFrom(source);
    return Status::OK();
  }

  const Reflection* source_reflection = source.GetReflection();
  const Reflection* target_reflection = target->GetReflection();

  const FieldDescriptor* source_value_field =
      source_descriptor->FindFieldByName("value");
  const FieldDescriptor* target_value_field =
      target_descriptor->FindFieldByName("value");

  if (!source_value_field || !target_value_field) {
    return InvalidArgument(
        "Unable to copy code from ", source_descriptor->full_name(), " to ",
        target_descriptor->full_name(), ". Both must have a `value` field.");
  }

  FieldDescriptor::Type source_type = source_value_field->type();
  FieldDescriptor::Type target_type = target_value_field->type();

  FHIR_RETURN_IF_ERROR(CopyCommonField(source, target, "id"));
  FHIR_RETURN_IF_ERROR(CopyCommonField(source, target, "extension"));

  if (!source_reflection->HasField(source, source_value_field)) {
    return Status::OK();
  }

  if (source_type == FieldDescriptor::Type::TYPE_ENUM) {
    const EnumDescriptor* source_enum_type = source_value_field->enum_type();
    const EnumValueDescriptor* source_value =
        source_reflection->GetEnum(source, source_value_field);

    if (target_type == FieldDescriptor::Type::TYPE_ENUM) {
      const EnumDescriptor* target_enum_type = target_value_field->enum_type();
      // Enum to Enum
      if (source_enum_type == target_enum_type) {
        target_reflection->SetEnum(target, target_value_field, source_value);
        return Status::OK();
      } else {
        const EnumValueDescriptor* target_value =
            target_enum_type->FindValueByName(source_value->name());
        if (target_value) {
          target_reflection->SetEnum(target, target_value_field, target_value);
          return Status::OK();
        } else {
          return InvalidArgument(
              "Unable to copy code from ", source_descriptor->full_name(),
              " to ", target_descriptor->full_name(),
              ". Found incompatible value: ", source_value->name());
        }
      }
    } else if (target_type == FieldDescriptor::Type::TYPE_STRING) {
      // Enum to String
      target_reflection->SetString(target, target_value_field,
                                   EnumValueToString(source_value));
      return Status::OK();
    } else {
      return InvalidArgument(
          "Cannot copy code to ", target_descriptor->full_name(),
          ".  Must have a value field of either String type or Enum type.");
    }
  } else if (source_type == FieldDescriptor::Type::TYPE_STRING) {
    const std::string& source_value =
        source_reflection->GetString(source, source_value_field);
    if (target_type == FieldDescriptor::Type::TYPE_STRING) {
      // String to String
      target_reflection->SetString(target, target_value_field, source_value);
      return Status::OK();
    } else if (target_type == FieldDescriptor::Type::TYPE_ENUM) {
      // String to Enum
      FHIR_ASSIGN_OR_RETURN(
          const EnumValueDescriptor* target_enum_value,
          CodeStringToEnumValue(source_value, target_value_field->enum_type()));
      target_reflection->SetEnum(target, target_value_field, target_enum_value);
      return Status::OK();
    } else {
      return InvalidArgument(
          "Cannot copy code to ", target_descriptor->full_name(),
          ".  Must have a value field of either String type or Enum type.");
    }
  } else {
    return InvalidArgument(
        "Cannot copy code from ", source_descriptor->full_name(),
        ".  Must have a value field of either String type or Enum type.");
  }
}

}  // namespace fhir
}  // namespace google
