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

#include "google/fhir/r4/primitive_handler.h"

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
#include "proto/annotations.pb.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/r4/google_extensions.pb.h"
#include "include/json/json.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace r4 {

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

StatusOr<std::unique_ptr<PrimitiveWrapper>> R4PrimitiveHandler::GetWrapper(
    const Descriptor* target_descriptor) const {
  if (IsTypeOrProfileOfCode(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        (new CodeWrapper<r4::core::Code>()));
  } else if (IsMessageType<r4::core::Base64Binary>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new Base64BinaryWrapper<r4::core::Base64Binary,
                                r4::google::Base64BinarySeparatorStride>());
  } else if (IsMessageType<r4::core::Boolean>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new BooleanWrapper<r4::core::Boolean>());
  } else if (IsMessageType<r4::core::Canonical>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<r4::core::Canonical>());
  } else if (IsMessageType<r4::core::Date>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeTypeWrapper<r4::core::Date>());
  } else if (IsMessageType<r4::core::DateTime>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeTypeWrapper<r4::core::DateTime>());
  } else if (IsMessageType<r4::core::Decimal>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new DecimalWrapper<r4::core::Decimal>());
  } else if (IsMessageType<r4::core::Id>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<r4::core::Id>());
  } else if (IsMessageType<r4::core::Instant>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeTypeWrapper<r4::core::Instant>());
  } else if (IsMessageType<r4::core::Integer>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new IntegerTypeWrapper<r4::core::Integer>());
  } else if (IsMessageType<r4::core::Markdown>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<r4::core::Markdown>());
  } else if (IsMessageType<r4::core::Oid>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<r4::core::Oid>());
  } else if (IsMessageType<r4::core::PositiveInt>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new PositiveIntWrapper<r4::core::PositiveInt>());
  } else if (IsMessageType<r4::core::String>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<r4::core::String>());
  } else if (IsMessageType<r4::core::Time>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(new TimeWrapper<r4::core::Time>());
  } else if (IsMessageType<r4::core::UnsignedInt>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new UnsignedIntWrapper<r4::core::UnsignedInt>());
  } else if (IsMessageType<r4::core::Uri>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<r4::core::Uri>());
  } else if (IsMessageType<r4::core::Url>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<r4::core::Url>());
  } else if (IsMessageType<r4::core::Xhtml>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new XhtmlWrapper<r4::core::Xhtml>());
  } else {
    return InvalidArgument("Unexpected R4 primitive FHIR type: ",
                           target_descriptor->full_name());
  }
}

const R4PrimitiveHandler* R4PrimitiveHandler::GetInstance() {
  static R4PrimitiveHandler* instance = new R4PrimitiveHandler;
  return instance;
}

}  // namespace r4
}  // namespace fhir
}  // namespace google
