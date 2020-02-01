/*
 * Copyright 2018 Google LLC
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

#include "google/fhir/primitive_wrapper.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "absl/memory/memory.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codes.h"
#include "google/fhir/extensions.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/r4/google_extensions.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "include/json/json.h"
#include "tensorflow/core/lib/core/errors.h"
#include "re2/re2.h"

namespace google {
namespace fhir {

using ::google::fhir::Status;
using ::google::fhir::StatusOr;
using primitives_internal::PrimitiveWrapper;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using ::tensorflow::errors::InvalidArgument;

namespace primitives_internal {

StatusOr<bool> HasPrimitiveHasNoValue(const Message& message) {
  const FieldDescriptor* field =
      message.GetDescriptor()->FindFieldByName("extension");
  std::vector<const Message*> no_value_extensions;
  ForEachMessage<Message>(message, field, [&](const Message& extension) {
    std::string scratch;
    const std::string& url_value =
        extensions_lib::GetExtensionUrl(extension, &scratch);
    if (url_value == kPrimitiveHasNoValueUrl) {
      no_value_extensions.push_back(&extension);
    }
  });
  if (no_value_extensions.size() > 1) {
    return InvalidArgument(
        "Message has more than one PrimitiveHasNoValue extension: ",
        message.GetDescriptor()->full_name());
  }
  if (no_value_extensions.empty()) {
    return false;
  }
  const Message& no_value_extension = *no_value_extensions.front();
  const Message& value_msg = no_value_extension.GetReflection()->GetMessage(
      no_value_extension,
      no_value_extension.GetDescriptor()->FindFieldByName("value"));
  const Message& boolean_msg = value_msg.GetReflection()->GetMessage(
      value_msg, value_msg.GetDescriptor()->FindFieldByName("boolean"));
  return boolean_msg.GetReflection()->GetBool(
      boolean_msg, boolean_msg.GetDescriptor()->FindFieldByName("value"));
}

namespace {

// TODO: These get wrapper functions should be deleted once
// the versions in primitive_handlers are used.

StatusOr<std::unique_ptr<PrimitiveWrapper>> GetStu3Wrapper(
    const Descriptor* target_descriptor) {
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

StatusOr<std::unique_ptr<PrimitiveWrapper>> GetR4Wrapper(
    const Descriptor* target_descriptor) {
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

}  // namespace

}  // namespace primitives_internal

Status BuildHasNoValueExtension(Message* extension) {
  const Descriptor* descriptor = extension->GetDescriptor();
  const Reflection* reflection = extension->GetReflection();

  if (!IsExtension(descriptor)) {
    return InvalidArgument("Not a valid extension type: ",
                           descriptor->full_name());
  }

  const FieldDescriptor* url_field = descriptor->FindFieldByName("url");
  FHIR_RETURN_IF_ERROR(
      SetPrimitiveStringValue(reflection->MutableMessage(extension, url_field),
                              primitives_internal::kPrimitiveHasNoValueUrl));

  Message* value = reflection->MutableMessage(
      extension, descriptor->FindFieldByName("value"));

  Message* boolean_message = value->GetReflection()->MutableMessage(
      value, value->GetDescriptor()->FindFieldByName("boolean"));

  boolean_message->GetReflection()->SetBool(
      boolean_message,
      boolean_message->GetDescriptor()->FindFieldByName("value"), true);

  return Status::OK();
}

::google::fhir::Status ParseInto(const Json::Value& json,
                                 const proto::FhirVersion fhir_version,
                                 const absl::TimeZone tz,
                                 ::google::protobuf::Message* target) {
  if (json.type() == Json::ValueType::arrayValue ||
      json.type() == Json::ValueType::objectValue) {
    return InvalidArgument("Invalid JSON type for ",
                           absl::StrCat(json.toStyledString()));
  }
  switch (fhir_version) {
    case proto::STU3: {
      FHIR_ASSIGN_OR_RETURN(
          std::unique_ptr<primitives_internal::PrimitiveWrapper> wrapper,
          primitives_internal::GetStu3Wrapper(target->GetDescriptor()));
      FHIR_RETURN_IF_ERROR(wrapper->Parse(json, tz));
      return wrapper->MergeInto(target);
    }
    case proto::R4: {
      FHIR_ASSIGN_OR_RETURN(
          std::unique_ptr<PrimitiveWrapper> wrapper,
          primitives_internal::GetR4Wrapper(target->GetDescriptor()));
      FHIR_RETURN_IF_ERROR(wrapper->Parse(json, tz));
      return wrapper->MergeInto(target);
    }
    default:
      return InvalidArgument("Unsupported Fhir Version: ",
                             proto::FhirVersion_Name(fhir_version));
  }
}

StatusOr<JsonPrimitive> WrapPrimitiveProto(const ::google::protobuf::Message& proto) {
  const ::google::protobuf::Descriptor* descriptor = proto.GetDescriptor();
  std::unique_ptr<PrimitiveWrapper> wrapper;
  switch (GetFhirVersion(proto)) {
    case proto::STU3: {
      FHIR_ASSIGN_OR_RETURN(wrapper,
                            primitives_internal::GetStu3Wrapper(descriptor));
      break;
    }
    case proto::R4: {
      FHIR_ASSIGN_OR_RETURN(wrapper,
                            primitives_internal::GetR4Wrapper(descriptor));
      break;
    }
    default:
      return InvalidArgument(
          "Unsupported Fhir Version: ",
          proto::FhirVersion_Name(GetFhirVersion(proto)),
          " for proto: ", proto.GetDescriptor()->full_name());
  }
  FHIR_RETURN_IF_ERROR(wrapper->Wrap(proto));
  FHIR_ASSIGN_OR_RETURN(const std::string value, wrapper->ToValueString());
  if (wrapper->HasElement()) {
    FHIR_ASSIGN_OR_RETURN(std::unique_ptr<Message> wrapped,
                          wrapper->GetElement());
    return JsonPrimitive{value, std::move(wrapped)};
  }
  return JsonPrimitive{value, nullptr};
}

Status ValidatePrimitive(const ::google::protobuf::Message& primitive) {
  if (!IsPrimitive(primitive.GetDescriptor())) {
    return InvalidArgument("Not a primitive type: ",
                           primitive.GetDescriptor()->full_name());
  }

  const ::google::protobuf::Descriptor* descriptor = primitive.GetDescriptor();
  std::unique_ptr<PrimitiveWrapper> wrapper;
  switch (GetFhirVersion(primitive)) {
    case proto::STU3: {
      FHIR_ASSIGN_OR_RETURN(wrapper,
                            primitives_internal::GetStu3Wrapper(descriptor));
      break;
    }
    case proto::R4: {
      FHIR_ASSIGN_OR_RETURN(wrapper,
                            primitives_internal::GetR4Wrapper(descriptor));
      break;
    }
    default:
      return InvalidArgument("Unsupported FHIR Version: ",
                             proto::FhirVersion_Name(GetFhirVersion(primitive)),
                             " for proto: ", descriptor->full_name());
  }

  FHIR_RETURN_IF_ERROR(wrapper->Wrap(primitive));
  return wrapper->ValidateProto();
}

}  // namespace fhir
}  // namespace google
