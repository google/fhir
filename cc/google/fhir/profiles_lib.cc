/*
 * Copyright 2019 Google LLC
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

#include "google/fhir/profiles_lib.h"

#include <string>

#include "absl/status/status.h"
#include "absl/synchronization/mutex.h"

namespace google {
namespace fhir {
namespace profiles_internal {


using ::google::protobuf::Descriptor;

namespace {

const std::set<std::string> GetAncestorSet(
    const ::google::protobuf::Descriptor* descriptor) {
  std::set<std::string> ancestors;
  ancestors.insert(GetStructureDefinitionUrl(descriptor));

  for (int i = 0;
       i < descriptor->options().ExtensionSize(proto::fhir_profile_base); i++) {
    ancestors.insert(
        descriptor->options().GetExtension(proto::fhir_profile_base, i));
  }
  return ancestors;
}

bool AddSharedCommonAncestorMemo(
    const std::string& first_url, const std::string& second_url,
    const bool value,
    std::unordered_map<std::string, std::unordered_map<std::string, bool>>*
        memos) {
  (*memos)[first_url][second_url] = value;
  (*memos)[second_url][first_url] = value;
  return value;
}

}  // namespace

const bool SharesCommonAncestor(const ::google::protobuf::Descriptor* first,
                                const ::google::protobuf::Descriptor* second) {
  static std::unordered_map<std::string, std::unordered_map<std::string, bool>>
      memos;
  static absl::Mutex memos_mutex;

  const std::string& first_url = GetStructureDefinitionUrl(first);
  const std::string& second_url = GetStructureDefinitionUrl(second);

  absl::MutexLock lock(&memos_mutex);
  const auto& first_url_memo_entry = memos[first_url];
  const auto& memo_entry = first_url_memo_entry.find(second_url);
  if (memo_entry != first_url_memo_entry.end()) {
    return memo_entry->second;
  }

  const std::set<std::string> first_set = GetAncestorSet(first);
  for (const std::string& entry_from_second : GetAncestorSet(second)) {
    if (first_set.find(entry_from_second) != first_set.end()) {
      return AddSharedCommonAncestorMemo(first_url, second_url, true, &memos);
    }
  }
  return AddSharedCommonAncestorMemo(first_url, second_url, false, &memos);
}

const unordered_map<std::string, const FieldDescriptor*>& GetExtensionMap(
    const Descriptor* descriptor) {
  // Note that we memoize on descriptor address, since the values include
  // FieldDescriptor addresses, which will only be valid for a given address
  // of input descriptor
  static auto* memos =
      new unordered_map<intptr_t,
                        unordered_map<std::string, const FieldDescriptor*>>();
  static absl::Mutex memos_mutex;

  const intptr_t memo_key = (intptr_t)descriptor;

  memos_mutex.ReaderLock();
  const auto iter = memos->find(memo_key);
  if (iter != memos->end()) {
    memos_mutex.ReaderUnlock();
    return iter->second;
  }
  memos_mutex.ReaderUnlock();

  absl::MutexLock lock(&memos_mutex);
  auto& extension_map = (*memos)[memo_key];
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    if (HasInlinedExtensionUrl(field)) {
      extension_map[extensions_lib::GetInlinedExtensionUrl(field)] = field;
    }
  }
  return extension_map;
}

// Returns the corresponding FieldDescriptor on a target message for a given
// field on a source message, or nullptr if none can be found.
// Returns a status error if any subprocess encounters a problem.
// Note that the inability to find a suitable target field does NOT constitute
// a failure with a status return.
StatusOr<const FieldDescriptor*> FindTargetField(
    const Message& source, const Message* target,
    const FieldDescriptor* source_field) {
  const Descriptor* target_descriptor = target->GetDescriptor();

  const FieldDescriptor* target_field =
      target_descriptor->FindFieldByName(source_field->name());
  if (target_field) {
    return target_field;
  }
  // If the source and target are contained resources, and the fields don't
  // match up, it can be a profile that exists in one but not the other.
  // In this case, use the base resource type if available, otherwise fail.
  if (IsContainedResource(*target) && IsContainedResource(source)) {
    FHIR_ASSIGN_OR_RETURN(
        const Descriptor* source_base_type,
        GetBaseResourceDescriptor(source_field->message_type()));
    const std::string base_field_name = ToSnakeCase(source_base_type->name());
    return target_descriptor->FindFieldByName(base_field_name);
  }
  return nullptr;
}

Status CopyProtoPrimitiveField(const Message& source,
                               const FieldDescriptor* source_field,
                               Message* target,
                               const FieldDescriptor* target_field) {
  if (source_field->type() != target_field->type()) {
    return InvalidArgumentError(absl::StrCat(
        "Primitive field type mismatch between ", source_field->full_name(),
        " and ", target_field->full_name()));
  }
  const auto* source_reflection = source.GetReflection();
  const auto* target_reflection = target->GetReflection();

  switch (source_field->type()) {
    case google::protobuf::FieldDescriptor::TYPE_STRING:
      target_reflection->SetString(
          target, target_field,
          source_reflection->GetString(source, source_field));
      break;
    case google::protobuf::FieldDescriptor::TYPE_BOOL:
      target_reflection->SetBool(
          target, target_field,
          source_reflection->GetBool(source, source_field));
      break;
    case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
      target_reflection->SetDouble(
          target, target_field,
          source_reflection->GetDouble(source, source_field));
      break;
    case google::protobuf::FieldDescriptor::TYPE_INT64:
      target_reflection->SetInt64(
          target, target_field,
          source_reflection->GetInt64(source, source_field));
      break;
    case google::protobuf::FieldDescriptor::TYPE_SINT32:
      target_reflection->SetInt32(
          target, target_field,
          source_reflection->GetInt32(source, source_field));
      break;
    case google::protobuf::FieldDescriptor::TYPE_UINT32:
      target_reflection->SetUInt32(
          target, target_field,
          source_reflection->GetUInt32(source, source_field));
      break;
    case google::protobuf::FieldDescriptor::TYPE_ENUM:
      target_reflection->SetEnum(
          target, target_field,
          source_reflection->GetEnum(source, source_field));
      break;
    default:
      return InvalidArgumentError(absl::StrCat(
          "Invalid primitive type for FHIR: ",
          google::protobuf::FieldDescriptor::TypeName(source_field->type())));
  }
  return absl::OkStatus();
}

}  // namespace profiles_internal
}  // namespace fhir
}  // namespace google
