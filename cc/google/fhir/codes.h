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
#include "proto/r4/core/codes.pb.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/stu3/codes.pb.h"
#include "proto/stu3/datatypes.pb.h"

namespace google {
namespace fhir {

::google::fhir::StatusOr<const ::google::protobuf::EnumValueDescriptor*>
CodeStringToEnumValue(const std::string& code_string,
                      const ::google::protobuf::EnumDescriptor* target_enum_type);

::google::fhir::Status ConvertToTypedCode(
    const ::google::fhir::stu3::proto::Code& generic_code,
    google::protobuf::Message* target);

::google::fhir::Status ConvertToTypedCode(
    const ::google::fhir::r4::core::Code& generic_code,
    google::protobuf::Message* target);

::google::fhir::Status ConvertToGenericCode(
    const google::protobuf::Message& typed_code,
    ::google::fhir::stu3::proto::Code* generic_code);

::google::fhir::Status ConvertToGenericCode(
    const google::protobuf::Message& typed_code,
    ::google::fhir::r4::core::Code* generic_code);

::google::fhir::Status ConvertToTypedCoding(
    const google::fhir::stu3::proto::Coding& generic_coding,
    google::protobuf::Message* typed_coding);

::google::fhir::Status ConvertToGenericCoding(
    const google::protobuf::Message& typed_coding,
    google::fhir::stu3::proto::Coding* generic_coding);

::google::fhir::Status ConvertToTypedCoding(
    const google::fhir::r4::core::Coding& generic_coding,
    google::protobuf::Message* typed_coding);

::google::fhir::Status ConvertToGenericCoding(
    const google::protobuf::Message& typed_coding,
    google::fhir::r4::core::Coding* generic_coding);

::google::fhir::Status CopyCode(const google::protobuf::Message& source,
                                google::protobuf::Message* target);

template <typename TypedResourceTypeCode>
::google::fhir::StatusOr<typename TypedResourceTypeCode::Value>
GetCodeForResourceType(const google::protobuf::Message& resource);

::google::fhir::StatusOr<::google::fhir::stu3::proto::ResourceTypeCode::Value>
GetCodeForResourceType(const google::protobuf::Message& resource);

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_CODES_H_
