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
      extension_map[GetInlinedExtensionUrl(field)] = field;
    }
  }
  return extension_map;
}

}  // namespace profiles_internal
}  // namespace fhir
}  // namespace google
