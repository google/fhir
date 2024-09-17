/*
 * Copyright 2023 Google LLC
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

#include "google/fhir/profiled_extensions.h"

#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"

namespace google::fhir::profiled {

std::string GetInlinedExtensionUrl(const google::protobuf::FieldDescriptor* field) {
  return field->options().HasExtension(
             ::google::fhir::proto::fhir_inlined_extension_url)
             ? field->options().GetExtension(
                   ::google::fhir::proto::fhir_inlined_extension_url)
             : std::string(field->json_name());
}

namespace internal {

std::vector<const google::protobuf::FieldDescriptor*> FindValueFields(
    const google::protobuf::Descriptor* descriptor) {
  std::vector<const google::protobuf::FieldDescriptor*> value_fields;
  for (int i = 0; i < descriptor->field_count(); i++) {
    absl::string_view name = descriptor->field(i)->name();
    if (name != "id" && name != "extension") {
      value_fields.push_back(descriptor->field(i));
    }
  }
  return value_fields;
}

absl::Status ValidateExtension(const google::protobuf::Descriptor* descriptor) {
  if (!IsProfileOfExtension(descriptor)) {
    return absl::InvalidArgumentError(
        absl::StrCat(descriptor->full_name(), " is not a FHIR extension type"));
  }
  if (!descriptor->options().HasExtension(
          ::google::fhir::proto::fhir_structure_definition_url)) {
    return absl::InvalidArgumentError(
        absl::StrCat(descriptor->full_name(),
                     " is not a valid FHIR extension type: No "
                     "fhir_structure_definition_url."));
  }
  return absl::OkStatus();
}

bool IsSimpleExtension(const google::protobuf::Descriptor* descriptor) {
  // Simple extensions have only a single, non-repeated value field.
  // However, it is also possible to have a complex extension with only
  // a single non-repeated field.  In that case, is_complex_extension is used to
  // disambiguate.
  const std::vector<const google::protobuf::FieldDescriptor*> value_fields =
      internal::FindValueFields(descriptor);
  return IsProfileOfExtension(descriptor) && value_fields.size() == 1 &&
         !value_fields.front()->is_repeated() &&
         !descriptor->options().GetExtension(proto::is_complex_extension);
}

absl::Status CheckIsMessage(const google::protobuf::FieldDescriptor* field) {
  if (field->type() != google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Encountered unexpected proto primitive: ", field->full_name(),
        ".  Should be FHIR type"));
  }
  return absl::OkStatus();
}
}  // namespace internal

}  // namespace google::fhir::profiled
