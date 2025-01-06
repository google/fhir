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

#ifndef GOOGLE_FHIR_CODES_H_
#define GOOGLE_FHIR_CODES_H_

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"

namespace google {
namespace fhir {

namespace codes_internal {

std::string TitleCaseToUpperUnderscores(absl::string_view src);
}

absl::StatusOr<const ::google::protobuf::EnumValueDescriptor*> CodeStringToEnumValue(
    absl::string_view code_string,
    const ::google::protobuf::EnumDescriptor* target_enum_type);
std::string EnumValueToCodeString(
    const ::google::protobuf::EnumValueDescriptor* enum_value);

absl::StatusOr<std::string> GetCodeAsString(const ::google::protobuf::Message& code);

absl::Status CopyCoding(const ::google::protobuf::Message& source,
                        google::protobuf::Message* target);
absl::Status CopyCode(const ::google::protobuf::Message& source, google::protobuf::Message* target);

absl::StatusOr<std::string> GetSystemForCode(const ::google::protobuf::Message& code);

template <typename TypedResourceTypeCode>
absl::StatusOr<typename TypedResourceTypeCode::Value> GetCodeForResourceType(
    const ::google::protobuf::Message& resource) {
  const std::string enum_string = codes_internal::TitleCaseToUpperUnderscores(
      resource.GetDescriptor()->name());
  typename TypedResourceTypeCode::Value value;
  if (TypedResourceTypeCode::Value_Parse(enum_string, &value)) {
    return value;
  }
  return ::absl::InvalidArgumentError(
      absl::StrCat("No ResourceTypeCode found for type: ",
                   resource.GetDescriptor()->name()));
}

template <typename TypedContainedResource>
absl::StatusOr<const ::google::protobuf::Descriptor*> GetDescriptorForResourceType(
    const ::google::protobuf::EnumValueDescriptor* code) {
  const std::string code_string = EnumValueToCodeString(code);
  const ::google::protobuf::OneofDescriptor* resource_oneof =
      TypedContainedResource::descriptor()->FindOneofByName("oneof_resource");
  if (resource_oneof == nullptr) {
    return ::absl::InvalidArgumentError(
        ::absl::StrCat("Invalid ContainedResource type",
                       TypedContainedResource::descriptor()->full_name()));
  }
  const ::google::protobuf::FieldDescriptor* resource_field = nullptr;
  for (int i = 0; i < resource_oneof->field_count(); i++) {
    const ::google::protobuf::FieldDescriptor* field = resource_oneof->field(i);
    if (field->cpp_type() != ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      return ::absl::InvalidArgumentError(
          absl::StrCat("Field ", field->full_name(), "is not a message"));
    }
    if (field->message_type()->name() == code_string) {
      resource_field = field;
    }
  }
  if (resource_field == nullptr) {
    return ::absl::InvalidArgumentError(
        absl::StrCat("Resource type ", code_string, " not found in ",
                     TypedContainedResource::descriptor()->full_name()));
  }
  return resource_field->message_type();
}

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_CODES_H_
