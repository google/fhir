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
#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "google/fhir/annotations.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/util.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace fhir {

using ::absl::InvalidArgumentError;
using ::google::protobuf::Descriptor;
using ::google::protobuf::EnumDescriptor;
using ::google::protobuf::EnumValueDescriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

namespace codes_internal {

std::string TitleCaseToUpperUnderscores(absl::string_view src) {
  std::string dst;
  for (auto iter = src.begin(); iter != src.end(); ++iter) {
    if (absl::ascii_isupper(*iter) && iter != src.begin()) {
      dst.push_back('_');
    }
    dst.push_back(absl::ascii_toupper(*iter));
  }
  return dst;
}

}  // namespace codes_internal

std::string EnumValueToCodeString(const EnumValueDescriptor* enum_value) {
  if (enum_value->options().HasExtension(
          ::google::fhir::proto::fhir_original_code)) {
    return enum_value->options().GetExtension(
        ::google::fhir::proto::fhir_original_code);
  }

  std::string code_string(enum_value->name());
  std::transform(code_string.begin(), code_string.end(), code_string.begin(),
                 tolower);
  std::replace(code_string.begin(), code_string.end(), '_', '-');
  return code_string;
}

absl::StatusOr<const EnumValueDescriptor*> CodeStringToEnumValue(
    absl::string_view code_string, const EnumDescriptor* target_enum_type) {
  // Map from (target_enum_type->full_name() x code_string) -> result
  // for previous runs.
  static auto* memos = new absl::flat_hash_map<
      std::string,
      absl::flat_hash_map<std::string, const EnumValueDescriptor*>>();
  static absl::Mutex memos_mutex;

  // Check for memoized result.  Note we lock the mutex, in case something tries
  // to read this map while it is being written to.
  {
    absl::ReaderMutexLock l(&memos_mutex);
    const auto memos_for_enum_type = memos->find(target_enum_type->full_name());
    if (memos_for_enum_type != memos->end()) {
      const auto enum_result = memos_for_enum_type->second.find(code_string);
      if (enum_result != memos_for_enum_type->second.end()) {
        return enum_result->second;
      }
    }
  }

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

  return InvalidArgumentError(
      absl::StrCat("Failed to convert `", code_string, "` to ",
                   target_enum_type->full_name(), ": No matching enum found."));
}

absl::Status CopyCoding(const Message& source, Message* target) {
  const Descriptor* source_descriptor = source.GetDescriptor();
  const Descriptor* target_descriptor = target->GetDescriptor();

  if (source_descriptor->full_name() == target_descriptor->full_name()) {
    target->CopyFrom(source);
    return absl::OkStatus();
  }

  // Copy fields present in both profiled and unprofiled codings.
  FHIR_RETURN_IF_ERROR(CopyCommonField(source, target, "id"));
  FHIR_RETURN_IF_ERROR(CopyCommonField(source, target, "extension"));
  FHIR_RETURN_IF_ERROR(CopyCommonField(source, target, "version"));
  FHIR_RETURN_IF_ERROR(CopyCommonField(source, target, "display"));
  FHIR_RETURN_IF_ERROR(CopyCommonField(source, target, "user_selected"));

  const FieldDescriptor* source_code_field =
      source_descriptor->FindFieldByName("code");
  const FieldDescriptor* target_code_field =
      target_descriptor->FindFieldByName("code");

  if (!source_code_field) {
    return InvalidArgumentError(
        absl::StrCat("Invalid Coding: ", source_descriptor->full_name(),
                     " has no code field."));
  }
  if (!target_code_field) {
    return InvalidArgumentError(
        absl::StrCat("Invalid Coding: ", target_descriptor->full_name(),
                     " has no code field."));
  }

  const Message& source_code =
      source.GetReflection()->GetMessage(source, source_code_field);
  Message* target_code =
      target->GetReflection()->MutableMessage(target, target_code_field);

  FHIR_RETURN_IF_ERROR(CopyCode(source_code, target_code));

  const FieldDescriptor* target_system_field =
      target_descriptor->FindFieldByName("system");

  // TODO(b/177480280): This will fail if there is a target system field,
  // *and* a source system field, since in this case the source code will not
  // contain the system information, the containing Coding would.  In general,
  // it's not quite right to get the system from Code, since unprofiled codes
  // don't contain system information.  In practice, this isn't a problem,
  // because the only kind of profiled Codings we currently support are
  // Codings with typed Codes (which contain source information) but this is
  // not neccessary according to FHIR spec.  Also,this method is NOT currently
  // used by CopyCodeableConcept.
  if (target_system_field) {
    FHIR_ASSIGN_OR_RETURN(const std::string& source_system,
                          GetSystemForCode(source_code));
    FHIR_RETURN_IF_ERROR(SetPrimitiveStringValue(
        target->GetReflection()->MutableMessage(target, target_system_field),
        source_system));
  }

  return absl::OkStatus();
}

absl::Status CopyCode(const Message& source, Message* target) {
  const Descriptor* source_descriptor = source.GetDescriptor();
  const Descriptor* target_descriptor = target->GetDescriptor();

  if (source_descriptor->full_name() == target_descriptor->full_name()) {
    target->CopyFrom(source);
    return absl::OkStatus();
  }

  const Reflection* source_reflection = source.GetReflection();
  const Reflection* target_reflection = target->GetReflection();

  const FieldDescriptor* source_value_field =
      source_descriptor->FindFieldByName("value");
  const FieldDescriptor* target_value_field =
      target_descriptor->FindFieldByName("value");

  if (!source_value_field || !target_value_field) {
    return InvalidArgumentError(absl::StrCat(
        "Unable to copy code from ", source_descriptor->full_name(), " to ",
        target_descriptor->full_name(), ". Both must have a `value` field."));
  }

  FieldDescriptor::Type source_type = source_value_field->type();
  FieldDescriptor::Type target_type = target_value_field->type();

  FHIR_RETURN_IF_ERROR(CopyCommonField(source, target, "id"));
  FHIR_RETURN_IF_ERROR(CopyCommonField(source, target, "extension"));

  if (!source_reflection->HasField(source, source_value_field)) {
    return absl::OkStatus();
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
        return absl::OkStatus();
      } else {
        // TODO(b/177241602): This conversion should go through FHIR code,
        // since Enum value name is not meaningful.
        const EnumValueDescriptor* target_value =
            target_enum_type->FindValueByName(source_value->name());
        if (target_value) {
          target_reflection->SetEnum(target, target_value_field, target_value);
          return absl::OkStatus();
        } else {
          return InvalidArgumentError(absl::StrCat(
              "Unable to copy code from ", source_descriptor->full_name(),
              " to ", target_descriptor->full_name(),
              ". Found incompatible value: ", source_value->name()));
        }
      }
    } else if (target_type == FieldDescriptor::Type::TYPE_STRING) {
      // Enum to String
      target_reflection->SetString(target, target_value_field,
                                   EnumValueToCodeString(source_value));
      return absl::OkStatus();
    } else {
      return InvalidArgumentError(absl::StrCat(
          "Cannot copy code to ", target_descriptor->full_name(),
          ".  Must have a value field of either String type or Enum type."));
    }
  } else if (source_type == FieldDescriptor::Type::TYPE_STRING) {
    const std::string& source_value =
        source_reflection->GetString(source, source_value_field);
    if (target_type == FieldDescriptor::Type::TYPE_STRING) {
      // String to String
      target_reflection->SetString(target, target_value_field, source_value);
      return absl::OkStatus();
    } else if (target_type == FieldDescriptor::Type::TYPE_ENUM) {
      // String to Enum
      FHIR_ASSIGN_OR_RETURN(
          const EnumValueDescriptor* target_enum_value,
          CodeStringToEnumValue(source_value, target_value_field->enum_type()));
      target_reflection->SetEnum(target, target_value_field, target_enum_value);
      return absl::OkStatus();
    } else {
      return InvalidArgumentError(absl::StrCat(
          "Cannot copy code to ", target_descriptor->full_name(),
          ".  Must have a value field of either String type or Enum type."));
    }
  } else {
    return InvalidArgumentError(absl::StrCat(
        "Cannot copy code from ", source_descriptor->full_name(),
        ".  Must have a value field of either String type or Enum type."));
  }
}

absl::StatusOr<std::string> GetSystemForCode(const ::google::protobuf::Message& code) {
  const Descriptor* descriptor = code.GetDescriptor();

  const std::string& system = GetFixedCodingSystem(descriptor);
  if (!system.empty()) {
    // The entire profiled coding can only be from a single system.  Use that.
    return system;
  }

  // There is no single system for the whole code - look for system
  // annotation on the enum.
  const FieldDescriptor* enum_field =
      code.GetDescriptor()->FindFieldByName("value");
  if (enum_field->type() != FieldDescriptor::Type::TYPE_ENUM) {
    return InvalidArgumentError(
        "Invalid profiled Coding: missing system information on string code");
  }
  const ::google::protobuf::EnumValueDescriptor* enum_descriptor =
      code.GetReflection()->GetEnum(code, enum_field);
  if (!HasSourceCodeSystem(enum_descriptor)) {
    return InvalidArgumentError(
        "Invalid profiled Coding: missing system information on enum code");
  }
  return GetSourceCodeSystem(enum_descriptor);
}

absl::StatusOr<std::string> GetCodeAsString(const ::google::protobuf::Message& code) {
  const Descriptor* descriptor = code.GetDescriptor();
  if (!IsTypeOrProfileOfCode(code)) {
    return InvalidArgumentError(absl::StrCat(
        "Invalid type for GetCodeAsString: ", descriptor->full_name()));
  }

  const FieldDescriptor* value_field = descriptor->FindFieldByName("value");
  if (!value_field) {
    return InvalidArgumentError(absl::StrCat(
        "Invalid code type for GetCodeAsString: ", descriptor->full_name()));
  }
  const Reflection* reflection = code.GetReflection();

  switch (value_field->type()) {
    case FieldDescriptor::Type::TYPE_STRING:
      return reflection->GetString(code, value_field);
    case FieldDescriptor::Type::TYPE_ENUM:
      return EnumValueToCodeString(reflection->GetEnum(code, value_field));
    default:
      return InvalidArgumentError(
          absl::StrCat("Invalid value type field for GetCodeAsString: ",
                       descriptor->full_name()));
  }
}

}  // namespace fhir
}  // namespace google
