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

#ifndef GOOGLE_FHIR_STU3_CODES_H_
#define GOOGLE_FHIR_STU3_CODES_H_

#include "google/protobuf/message.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {

namespace codes_internal {

std::string TitleCaseToUpperUnderscores(const std::string& src);

}

StatusOr<const ::google::protobuf::EnumValueDescriptor*> CodeStringToEnumValue(
    const std::string& code_string,
    const ::google::protobuf::EnumDescriptor* target_enum_type);

Status CopyCoding(const ::google::protobuf::Message& source, google::protobuf::Message* target);
Status CopyCode(const ::google::protobuf::Message& source, google::protobuf::Message* target);

StatusOr<std::string> GetSystemForCode(const ::google::protobuf::Message& code);

template <typename TypedResourceTypeCode>
StatusOr<typename TypedResourceTypeCode::Value> GetCodeForResourceType(
    const ::google::protobuf::Message& resource) {
  const std::string& enum_string = codes_internal::TitleCaseToUpperUnderscores(
      resource.GetDescriptor()->name());
  typename TypedResourceTypeCode::Value value;
  if (TypedResourceTypeCode::Value_Parse(enum_string, &value)) {
    return value;
  }
  return ::tensorflow::errors::InvalidArgument(
      "No ResourceTypeCode found for type: ", resource.GetDescriptor()->name());
}

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_CODES_H_
