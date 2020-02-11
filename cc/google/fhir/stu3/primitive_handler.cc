/*
 * Copyright 2020 Google LLC
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
#include "google/fhir/stu3/primitive_handler.h"

#include <memory>
#include <unordered_map>
#include <utility>

#include "google/protobuf/descriptor.h"
#include "absl/memory/memory.h"
#include "absl/strings/string_view.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codes.h"
#include "google/fhir/extensions.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/primitive_wrapper.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "include/json/json.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace stu3 {

using primitives_internal::Base64BinaryWrapper;
using primitives_internal::BooleanWrapper;
using primitives_internal::CodeWrapper;
using primitives_internal::DecimalWrapper;
using primitives_internal::IntegerTypeWrapper;
using primitives_internal::PositiveIntWrapper;
using primitives_internal::PrimitiveWrapper;
using primitives_internal::StringTypeWrapper;
using primitives_internal::TimeTypeWrapper;
using primitives_internal::TimeWrapper;
using primitives_internal::UnsignedIntWrapper;
using primitives_internal::XhtmlWrapper;
using ::google::protobuf::Descriptor;
using ::tensorflow::errors::InvalidArgument;

StatusOr<std::unique_ptr<PrimitiveWrapper>> Stu3PrimitiveHandler::GetWrapper(
    const Descriptor* target_descriptor) const {
  if (IsMessageType<stu3::proto::Code>(target_descriptor) ||
      HasValueset(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        (new CodeWrapper<stu3::proto::Code>()));
  } else if (IsMessageType<stu3::proto::Base64Binary>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new Base64BinaryWrapper<stu3::proto::Base64Binary,
                                stu3::google::Base64BinarySeparatorStride>());
  } else if (IsMessageType<stu3::proto::Boolean>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new BooleanWrapper<stu3::proto::Boolean>());
  } else if (IsMessageType<stu3::proto::Date>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeTypeWrapper<stu3::proto::Date>());
  } else if (IsMessageType<stu3::proto::DateTime>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeTypeWrapper<stu3::proto::DateTime>());
  } else if (IsMessageType<stu3::proto::Decimal>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new DecimalWrapper<stu3::proto::Decimal>());
  } else if (IsMessageType<stu3::proto::Id>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<stu3::proto::Id>());
  } else if (IsMessageType<stu3::proto::Instant>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeTypeWrapper<stu3::proto::Instant>());
  } else if (IsMessageType<stu3::proto::Integer>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new IntegerTypeWrapper<stu3::proto::Integer>());
  } else if (IsMessageType<stu3::proto::Markdown>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<stu3::proto::Markdown>());
  } else if (IsMessageType<stu3::proto::Oid>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<stu3::proto::Oid>());
  } else if (IsMessageType<stu3::proto::PositiveInt>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new PositiveIntWrapper<stu3::proto::PositiveInt>());
  } else if (IsMessageType<stu3::proto::String>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<stu3::proto::String>());
  } else if (IsMessageType<stu3::proto::Time>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeWrapper<stu3::proto::Time>());
  } else if (IsMessageType<stu3::proto::UnsignedInt>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new UnsignedIntWrapper<stu3::proto::UnsignedInt>());
  } else if (IsMessageType<stu3::proto::Uri>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<stu3::proto::Uri>());
  } else if (IsMessageType<stu3::proto::Xhtml>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new XhtmlWrapper<stu3::proto::Xhtml>());
  } else {
    return InvalidArgument("Unexpected STU3 primitive FHIR type: ",
                           target_descriptor->full_name());
  }
}

const Stu3PrimitiveHandler* Stu3PrimitiveHandler::GetInstance() {
  static Stu3PrimitiveHandler* instance = new Stu3PrimitiveHandler;
  return instance;
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
