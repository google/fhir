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

#include <memory>
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "google/fhir/annotations.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/json_format_results.h"
#include "google/fhir/primitive_wrapper.h"
#include "google/fhir/status/status.h"

namespace google {
namespace fhir {

using ::absl::InvalidArgumentError;
using primitives_internal::PrimitiveWrapper;
using ::google::protobuf::Descriptor;
using ::google::protobuf::Message;

absl::StatusOr<ParseResult> PrimitiveHandler::ParseInto(
    const internal::FhirJson& json, const absl::TimeZone tz, Message* target,
    ErrorReporter& error_reporter) const {
  FHIR_RETURN_IF_ERROR(CheckVersion(*target));

  if (json.isArray() || json.isObject()) {
    FHIR_RETURN_IF_ERROR(
        error_reporter.ReportFhirFatal(InvalidArgumentError(absl::StrCat(
            "Invalid JSON type for ", target->GetDescriptor()->full_name()))));
    return ParseResult::kFailed;
  }
  FHIR_ASSIGN_OR_RETURN(std::unique_ptr<PrimitiveWrapper> wrapper,
                        GetWrapper(target->GetDescriptor()));
  FHIR_ASSIGN_OR_RETURN(ParseResult result,
                        wrapper->Parse(json, tz, error_reporter));
  if (result == ParseResult::kFailed) {
    return ParseResult::kFailed;
  }
  return wrapper->MergeInto(target, error_reporter);
}

::absl::StatusOr<ParseResult> PrimitiveHandler::ParseInto(
    const internal::FhirJson& json, Message* target,
    ErrorReporter& error_reporter) const {
  return ParseInto(json, absl::UTCTimeZone(), target, error_reporter);
}

absl::StatusOr<JsonPrimitive> PrimitiveHandler::WrapPrimitiveProto(
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

absl::Status PrimitiveHandler::ValidatePrimitive(
    const ::google::protobuf::Message& primitive, ErrorReporter& error_reporter) const {
  FHIR_RETURN_IF_ERROR(CheckVersion(primitive));

  if (!IsPrimitive(primitive.GetDescriptor())) {
    return InvalidArgumentError(absl::StrCat(
        "Not a primitive type: ", primitive.GetDescriptor()->full_name()));
  }

  const ::google::protobuf::Descriptor* descriptor = primitive.GetDescriptor();
  std::unique_ptr<PrimitiveWrapper> wrapper;
  FHIR_ASSIGN_OR_RETURN(wrapper, GetWrapper(descriptor));

  FHIR_RETURN_IF_ERROR(wrapper->Wrap(primitive));
  return wrapper->ValidateProto(error_reporter);
}

absl::Status PrimitiveHandler::CheckVersion(const Message& message) const {
  return CheckVersion(message.GetDescriptor());
}

absl::Status PrimitiveHandler::CheckVersion(
    const Descriptor* descriptor) const {
  auto test_version = GetFhirVersion(descriptor);
  if (test_version != version_) {
    return InvalidArgumentError(
        absl::StrCat("Invalid message for PrimitiveHandler.  Handler is ",
                     proto::FhirVersion_Name(version_), " but message `",
                     descriptor->full_name(), " is ",
                     proto::FhirVersion_Name(test_version)));
  }
  return absl::OkStatus();
}

}  // namespace fhir
}  // namespace google
