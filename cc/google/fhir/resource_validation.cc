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

#include <string>

#include "google/protobuf/any.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/fhir/annotations.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/fhir_path/fhir_path_validation.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace fhir {

using ::google::fhir::proto::validation_requirement;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

namespace {

absl::Status CheckField(const Message& message, const FieldDescriptor* field,
                        const PrimitiveHandler* primitive_handler,
                        const ScopedErrorReporter& error_reporter,
                        bool validate_reference_field_ids = false);

absl::Status ValidateFhirConstraints(
    const Message& message, const PrimitiveHandler* primitive_handler,
    const ScopedErrorReporter& error_reporter,
    const bool validate_reference_field_ids = false) {
  if (IsPrimitive(message.GetDescriptor())) {
    return primitive_handler->ValidatePrimitive(message, error_reporter);
  }

  if (IsMessageType<::google::protobuf::Any>(message)) {
    // We do not validate "Any" contained resources.
    // TODO(b/155339868): Potentially unpack the correct type and validate?
    return absl::OkStatus();
  }

  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* reflection = message.GetReflection();

  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    FHIR_RETURN_IF_ERROR(CheckField(message, field, primitive_handler,
                                    error_reporter,
                                    validate_reference_field_ids));
  }
  // Also verify that oneof fields are set.
  // Note that optional choice-types should have the containing message unset -
  // if the containing message is set, it should have a value set as well.
  for (int i = 0; i < descriptor->oneof_decl_count(); i++) {
    const ::google::protobuf::OneofDescriptor* oneof = descriptor->oneof_decl(i);
    if (!reflection->HasOneof(message, oneof) &&
        !oneof->options().GetExtension(
            ::google::fhir::proto::fhir_oneof_is_optional)) {
      FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirError("empty-oneof"));
    }
  }
  return absl::OkStatus();
}

// Check if a required field is missing.
absl::Status CheckField(const Message& message, const FieldDescriptor* field,
                        const PrimitiveHandler* primitive_handler,
                        const ScopedErrorReporter& error_reporter,
                        const bool validate_reference_field_ids) {
  if (field->options().HasExtension(validation_requirement) &&
      field->options().GetExtension(validation_requirement) ==
          ::google::fhir::proto::REQUIRED_BY_FHIR) {
    if (!FieldHasValue(message, field)) {
      FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirError(
          "missing-required-field", field->json_name()));
    }
  }

  if (IsReference(field->message_type())) {
    return primitive_handler->ValidateReferenceField(
        message, field, error_reporter, validate_reference_field_ids);
  }

  if (field->cpp_type() == ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    for (int i = 0; i < PotentiallyRepeatedFieldSize(message, field); i++) {
      FHIR_RETURN_IF_ERROR(ValidateFhirConstraints(
          GetPotentiallyRepeatedMessage(message, field, i), primitive_handler,
          error_reporter.WithScope(field, i)));
    }
  }

  return absl::OkStatus();
}

}  // namespace

::absl::Status Validate(const ::google::protobuf::Message& resource,
                        const PrimitiveHandler* primitive_handler,
                        const fhir_path::FhirPathValidator* message_validator,
                        ErrorHandler& error_handler,
                        const bool validate_reference_field_ids) {
  FHIR_RETURN_IF_ERROR(ValidateWithoutFhirPath(resource, primitive_handler,
                                               error_handler,
                                               validate_reference_field_ids));
  return message_validator->Validate(resource, primitive_handler,
                                     error_handler);
}

::absl::Status ValidateWithoutFhirPath(
    const ::google::protobuf::Message& resource,
    const PrimitiveHandler* primitive_handler, ErrorHandler& error_handler,
    const bool validate_reference_field_ids) {
  if (IsContainedResource(resource)) {
    FHIR_ASSIGN_OR_RETURN(const google::protobuf::Message* contained,
                          GetContainedResource(resource));
    return ValidateWithoutFhirPath(*contained, primitive_handler, error_handler,
                                   validate_reference_field_ids);
  }

  return ValidateFhirConstraints(
      resource, primitive_handler,
      ScopedErrorReporter(&error_handler, resource.GetDescriptor()->name()),
      validate_reference_field_ids);
}

}  // namespace fhir
}  // namespace google
