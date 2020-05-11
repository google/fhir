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

#include "google/fhir/resource_validation.h"

#include "google/protobuf/any.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/annotations.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/stu3/datatypes.pb.h"

namespace google {
namespace fhir {

using ::absl::FailedPreconditionError;
using ::google::fhir::proto::validation_requirement;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

namespace {

template <class TypedDateTime>
Status ValidatePeriod(const Message& period, const std::string& base) {
  const Descriptor* descriptor = period.GetDescriptor();
  const Reflection* reflection = period.GetReflection();
  const FieldDescriptor* start_field = descriptor->FindFieldByName("start");
  const FieldDescriptor* end_field = descriptor->FindFieldByName("end");

  if (reflection->HasField(period, start_field) &&
      reflection->HasField(period, end_field)) {
    FHIR_ASSIGN_OR_RETURN(
        const TypedDateTime& start,
        GetMessageInField<TypedDateTime>(period, start_field));
    FHIR_ASSIGN_OR_RETURN(const TypedDateTime& end,
                          GetMessageInField<TypedDateTime>(period, end_field));
    // Start time is greater than end time, but that's not necessarily invalid,
    // since the precisions can be different.  So we need to compare the end
    // time at the upper bound of end element.
    // Example: If the start time is "Tuesday at noon", and the end time is
    // "some time Tuesday", this is valid, even though the timestamp used for
    // "some time Tuesday" is Tuesday 00:00, since the precision for the start
    // is higher than the end.
    //
    // Also note the GetUpperBoundFromTimelikeElement is always greater than
    // the time itself by exactly one time unit, and hence start needs to be
    // strictly less than the upper bound of end, so as to not allow ranges like
    // [Tuesday, Monday] to be valid.
    if (google::fhir::GetTimeFromTimelikeElement(start) >=
        google::fhir::GetUpperBoundFromTimelikeElement(end)) {
      return ::absl::FailedPreconditionError(
          absl::StrCat(base, "-start-time-later-than-end-time"));
    }
  }

  return absl::OkStatus();
}

Status CheckField(const Message& message, const FieldDescriptor* field,
                  const std::string& field_name,
                  const PrimitiveHandler* primitive_handler);

Status ValidateFhirConstraints(const Message& message,
                               const std::string& base_name,
                               const PrimitiveHandler* primitive_handler) {
  if (IsPrimitive(message.GetDescriptor())) {
    return primitive_handler->ValidatePrimitive(message).ok()
               ? absl::OkStatus()
               : FailedPreconditionError(
                     absl::StrCat("invalid-primitive-", base_name));
  }

  if (IsMessageType<::google::protobuf::Any>(message)) {
    // We do not validate "Any" contained resources.
    // TODO: Potentially unpack the correct type and validate?
    return absl::OkStatus();
  }

  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* reflection = message.GetReflection();

  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    const std::string& field_name =
        absl::StrCat(base_name, ".", field->json_name());
    FHIR_RETURN_IF_ERROR(
        CheckField(message, field, field_name, primitive_handler));
  }
  // Also verify that oneof fields are set.
  // Note that optional choice-types should have the containing message unset -
  // if the containing message is set, it should have a value set as well.
  for (int i = 0; i < descriptor->oneof_decl_count(); i++) {
    const ::google::protobuf::OneofDescriptor* oneof = descriptor->oneof_decl(i);
    if (!reflection->HasOneof(message, oneof) &&
        !oneof->options().GetExtension(
            ::google::fhir::proto::fhir_oneof_is_optional)) {
      FHIR_RETURN_IF_ERROR(::absl::FailedPreconditionError(
          absl::StrCat("empty-oneof-", oneof->full_name())));
    }
  }
  return absl::OkStatus();
}

// Check if a required field is missing.
Status CheckField(const Message& message, const FieldDescriptor* field,
                  const std::string& field_name,
                  const PrimitiveHandler* primitive_handler) {
  if (field->options().HasExtension(validation_requirement) &&
      field->options().GetExtension(validation_requirement) ==
          ::google::fhir::proto::REQUIRED_BY_FHIR) {
    if (!FieldHasValue(message, field)) {
      return FailedPreconditionError(absl::StrCat("missing-", field_name));
    }
  }

  if (IsReference(field->message_type())) {
    auto status = primitive_handler->ValidateReferenceField(message, field);
    return status.ok() ? status
                       : FailedPreconditionError(absl::StrCat(
                             status.message(), "-at-", field_name));
  }

  if (field->cpp_type() == ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    for (int i = 0; i < PotentiallyRepeatedFieldSize(message, field); i++) {
      const auto& submessage = GetPotentiallyRepeatedMessage(message, field, i);
      FHIR_RETURN_IF_ERROR(
          ValidateFhirConstraints(submessage, field_name, primitive_handler));

      // Run extra validation for some types, until FHIRPath validation covers
      // these cases as well.
      if (IsMessageType<::google::fhir::stu3::proto::Period>(
              field->message_type())) {
        FHIR_RETURN_IF_ERROR(
            ValidatePeriod<::google::fhir::stu3::proto::DateTime>(submessage,
                                                                  field_name));
      }
      if (IsMessageType<::google::fhir::r4::core::Period>(
              field->message_type())) {
        FHIR_RETURN_IF_ERROR(ValidatePeriod<::google::fhir::r4::core::DateTime>(
            submessage, field_name));
      }
    }
  }

  return absl::OkStatus();
}

}  // namespace

// TODO: Invert the default here for FHIRPath handling, and have
// ValidateWithoutFhirPath instead of ValidateWithFhirPath

Status ValidateResourceWithFhirPath(
    const Message& resource, const PrimitiveHandler* primitive_handler,
    fhir_path::FhirPathValidator* message_validator) {
  FHIR_RETURN_IF_ERROR(ValidateFhirConstraints(
      resource, resource.GetDescriptor()->name(), primitive_handler));
  return message_validator->Validate(resource).LegacyValidationResult();
}

Status ValidateResource(const Message& resource,
                        const PrimitiveHandler* primitive_handler) {
  return ValidateFhirConstraints(resource, resource.GetDescriptor()->name(),
                                 primitive_handler);
}

}  // namespace fhir
}  // namespace google
