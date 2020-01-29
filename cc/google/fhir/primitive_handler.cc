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

#include "google/fhir/primitive_handler.h"

#include "google/fhir/annotations.h"
#include "google/fhir/primitive_wrapper.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {

using primitives_internal::PrimitiveWrapper;
using ::google::protobuf::Descriptor;
using ::google::protobuf::Message;
using ::tensorflow::errors::InvalidArgument;

::google::fhir::Status PrimitiveHandler::ParseInto(const Json::Value& json,
                                                   const absl::TimeZone tz,
                                                   Message* target) const {
  FHIR_RETURN_IF_ERROR(CheckVersion(*target));

  if (json.type() == Json::ValueType::arrayValue ||
      json.type() == Json::ValueType::objectValue) {
    return InvalidArgument("Invalid JSON type for ", json.toStyledString());
  }
  FHIR_ASSIGN_OR_RETURN(std::unique_ptr<PrimitiveWrapper> wrapper,
                        GetWrapper(target->GetDescriptor()));
  FHIR_RETURN_IF_ERROR(wrapper->Parse(json, tz));
  return wrapper->MergeInto(target);
}

::google::fhir::Status PrimitiveHandler::ParseInto(const Json::Value& json,
                                                   Message* target) const {
  return ParseInto(json, absl::UTCTimeZone(), target);
}

StatusOr<JsonPrimitive> PrimitiveHandler::WrapPrimitiveProto(
    const Message& proto) const {
  FHIR_RETURN_IF_ERROR(CheckVersion(proto));

  const Descriptor* descriptor = proto.GetDescriptor();
  std::unique_ptr<PrimitiveWrapper> wrapper;

  FHIR_ASSIGN_OR_RETURN(wrapper, GetWrapper(descriptor));
  FHIR_RETURN_IF_ERROR(wrapper->Wrap(proto));
  FHIR_ASSIGN_OR_RETURN(const std::string value, wrapper->ToValueString());
  if (wrapper->HasElement()) {
    FHIR_ASSIGN_OR_RETURN(std::unique_ptr<Message> wrapped,
                          wrapper->GetElement());
    return JsonPrimitive{value, std::move(wrapped)};
  }
  return JsonPrimitive{value, nullptr};
}

Status PrimitiveHandler::ValidatePrimitive(
    const ::google::protobuf::Message& primitive) const {
  FHIR_RETURN_IF_ERROR(CheckVersion(primitive));

  if (!IsPrimitive(primitive.GetDescriptor())) {
    return InvalidArgument("Not a primitive type: ",
                           primitive.GetDescriptor()->full_name());
  }

  const ::google::protobuf::Descriptor* descriptor = primitive.GetDescriptor();
  std::unique_ptr<PrimitiveWrapper> wrapper;
  FHIR_ASSIGN_OR_RETURN(wrapper, GetWrapper(descriptor));

  FHIR_RETURN_IF_ERROR(wrapper->Wrap(primitive));
  return wrapper->ValidateProto();
}

Status PrimitiveHandler::CheckVersion(const Message& message) const {
  return CheckVersion(message.GetDescriptor());
}

Status PrimitiveHandler::CheckVersion(const Descriptor* descriptor) const {
  auto test_version = GetFhirVersion(descriptor);
  if (test_version != version_) {
    return InvalidArgument("Invalid message for PrimitiveHandler.  Handler is ",
                           proto::FhirVersion_Name(version_), " but message `",
                           descriptor->full_name(), " is ",
                           proto::FhirVersion_Name(test_version));
  }
  return Status::OK();
}

}  // namespace fhir
}  // namespace google
