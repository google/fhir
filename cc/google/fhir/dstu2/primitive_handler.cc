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

#include "google/fhir/dstu2/primitive_handler.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "google/protobuf/descriptor.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/fhir/annotations.h"
#include "google/fhir/json_format_results.h"
#include "google/fhir/primitive_wrapper.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "proto/google/fhir/proto/dstu2/codes.pb.h"
#include "proto/google/fhir/proto/dstu2/datatypes.pb.h"
#include "proto/google/fhir/proto/dstu2/fhirproto_extensions.pb.h"
#include "proto/google/fhir/proto/dstu2/resources.pb.h"

namespace google {
namespace fhir {
namespace dstu2 {

using ::absl::InvalidArgumentError;
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

absl::StatusOr<std::unique_ptr<PrimitiveWrapper>>
DSTU2PrimitiveHandler::GetWrapper(const Descriptor* target_descriptor) const {
  if (HasValueset(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        (new CodeWrapper<dstu2::proto::String>()));
  }
  if (IsMessageType<dstu2::proto::Code>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        (new CodeWrapper<dstu2::proto::Code>()));
  }
  if (IsMessageType<dstu2::proto::Base64Binary>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new Base64BinaryWrapper<dstu2::proto::Base64Binary,
                                dstu2::proto::Extension>());
  }
  if (IsMessageType<dstu2::proto::Boolean>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new BooleanWrapper<dstu2::proto::Boolean>());
  }
  if (IsMessageType<dstu2::proto::Date>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeTypeWrapper<dstu2::proto::Date>());
  }
  if (IsMessageType<dstu2::proto::DateTime>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeTypeWrapper<dstu2::proto::DateTime>());
  }
  if (IsMessageType<dstu2::proto::Decimal>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new DecimalWrapper<dstu2::proto::Decimal>());
  }
  if (IsMessageType<dstu2::proto::Id>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<dstu2::proto::Id>());
  }
  if (IsMessageType<dstu2::proto::Instant>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeTypeWrapper<dstu2::proto::Instant>());
  }
  if (IsMessageType<dstu2::proto::Integer>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new IntegerTypeWrapper<dstu2::proto::Integer>());
  }
  if (IsMessageType<dstu2::proto::Oid>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<dstu2::proto::Oid>());
  }
  if (IsMessageType<dstu2::proto::Markdown>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new CodeWrapper<dstu2::proto::Markdown>());
  }
  if (IsMessageType<dstu2::proto::PositiveInt>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new PositiveIntWrapper<dstu2::proto::PositiveInt>());
  }
  if (IsMessageType<dstu2::proto::String>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<dstu2::proto::String>());
  }
  if (IsMessageType<dstu2::proto::Time>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeWrapper<dstu2::proto::Time>());
  }
  if (IsMessageType<dstu2::proto::UnsignedInt>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new UnsignedIntWrapper<dstu2::proto::UnsignedInt>());
  }
  if (IsMessageType<dstu2::proto::Uri>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<dstu2::proto::Uri>());
  }
  if (IsMessageType<dstu2::proto::Xhtml>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new XhtmlWrapper<dstu2::proto::Xhtml>());
  }
  return InvalidArgumentError(
      absl::StrCat("Unexpected DSTU2 primitive FHIR type: ",
                   target_descriptor->full_name()));
}

const DSTU2PrimitiveHandler* DSTU2PrimitiveHandler::GetInstance() {
  static DSTU2PrimitiveHandler* instance = new DSTU2PrimitiveHandler;
  return instance;
}

absl::Status DSTU2PrimitiveHandler::CopyCodeableConcept(
    const google::protobuf::Message& source, google::protobuf::Message* target) const {
  if (source.GetDescriptor()->full_name() !=
          target->GetDescriptor()->full_name() ||
      source.GetDescriptor()->full_name() !=
          dstu2::proto::CodeableConcept::descriptor()->full_name()) {
    return absl::InvalidArgumentError(
        absl::Substitute("DSTU2 CopyCodeableConcept must be called with 2 core "
                         "CodeableConcept messages.  Found: $0 and $1.",
                         source.GetDescriptor()->full_name(),
                         target->GetDescriptor()->full_name()));
  }
  target->CopyFrom(source);
  return absl::OkStatus();
}

}  // namespace dstu2
}  // namespace fhir
}  // namespace google
