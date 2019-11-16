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
#include "absl/strings/str_cat.h"
#include "google/fhir/annotations.h"
#include "google/fhir/fhir_path/fhir_path.h"
#include "google/fhir/primitive_wrapper.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {


using ::google::fhir::proto::valid_reference_type;
using ::google::fhir::proto::validation_requirement;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::OneofDescriptor;
using ::google::protobuf::Reflection;
using ::tensorflow::errors::FailedPrecondition;

namespace {

template <class TypedReference, class TypedReferenceId>
Status ValidateReference(const Message& message, const FieldDescriptor* field,
                         const std::string& base_name) {
  static const Descriptor* descriptor = TypedReference::descriptor();
  static const OneofDescriptor* oneof = descriptor->oneof_decl(0);

  for (int i = 0; i < PotentiallyRepeatedFieldSize(message, field); i++) {
    const TypedReference& reference =
        GetPotentiallyRepeatedMessage<TypedReference>(message, field, i);
    const Reflection* reflection = reference.GetReflection();
    const FieldDescriptor* reference_field =
        reflection->GetOneofFieldDescriptor(reference, oneof);

    if (!reference_field) {
      if (reference.extension_size() == 0 && !reference.has_identifier() &&
          !reference.has_display()) {
        return FailedPrecondition("empty-reference-", base_name, ".",
                                  field->name());
      }
      // There's no reference field, but there is other data.  That's valid.
      return Status::OK();
    }
    if (field->options().ExtensionSize(valid_reference_type) == 0) {
      // The reference field does not have restrictions, so any value is fine.
      return Status::OK();
    }
    if (reference.has_uri() || reference.has_fragment()) {
      // Uri and Fragment references are untyped.
      return Status::OK();
    }

    if (IsMessageType<TypedReferenceId>(reference_field->message_type())) {
      const std::string& reference_type =
          reference_field->options().GetExtension(
              ::google::fhir::proto::referenced_fhir_type);
      bool is_allowed = false;
      for (int i = 0; i < field->options().ExtensionSize(valid_reference_type);
           i++) {
        const std::string& valid_type =
            field->options().GetExtension(valid_reference_type, i);
        if (valid_type == reference_type || valid_type == "Resource") {
          is_allowed = true;
          break;
        }
      }
      if (!is_allowed) {
        return FailedPrecondition("invalid-reference-", base_name, ".",
                                  field->name(), "-disallowed-type-",
                                  reference_type);
      }
    }
  }

  return Status::OK();
}

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
    // "some time Tuesday", this is valid, even thought the timestamp used for
    // "some time Tuesday" is Tuesday 00:00, since the precision for the start
    // is higher than the end.
    //
    // Also note the GetUpperBoundFromTimelikeElement is always greater than
    // the time itself by exactly one time unit, and hence start needs to be
    // strictly less than end upper bound of end, so as to not allow ranges like
    // [Tuesday, Monday] to be valid.
    if (google::fhir::GetTimeFromTimelikeElement(start) >=
        google::fhir::GetUpperBoundFromTimelikeElement(end)) {
      return ::tensorflow::errors::FailedPrecondition(
          base, "-start-time-later-than-end-time");
    }
  }

  return Status::OK();
}

Status CheckField(const Message& message, const FieldDescriptor* field,
                  const std::string& base_name);

Status ValidateFhirConstraints(const Message& message,
                               const std::string& base_name) {
  if (IsPrimitive(message.GetDescriptor())) {
    return ValidatePrimitive(message).ok()
               ? Status::OK()
               : FailedPrecondition("invalid-primitive-", base_name);
  }

  if (IsMessageType<::google::protobuf::Any>(message)) {
    // We do not validate "Any" contained resources.
    // TODO: maybe we should though... we'd need a registry that
    // allows us to automatically unpack into the correct type based on url.
    return Status::OK();
  }

  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* reflection = message.GetReflection();
  for (int i = 0; i < descriptor->field_count(); i++) {
    FHIR_RETURN_IF_ERROR(CheckField(message, descriptor->field(i), base_name));
  }
  // Also verify that oneof fields are set.
  // Note that optional choice-types should have the containing message unset -
  // if the containing message is set, it should have a value set as well.
  for (int i = 0; i < descriptor->oneof_decl_count(); i++) {
    const ::google::protobuf::OneofDescriptor* oneof = descriptor->oneof_decl(i);
    if (!reflection->HasOneof(message, oneof) &&
        !oneof->options().GetExtension(
            ::google::fhir::proto::fhir_oneof_is_optional)) {
      FHIR_RETURN_IF_ERROR(::tensorflow::errors::FailedPrecondition(
          "empty-oneof-", oneof->full_name()));
    }
  }
  return Status::OK();
}

// Check if a required field is missing.
Status CheckField(const Message& message, const FieldDescriptor* field,
                  const std::string& base_name) {
  const std::string& new_base =
      absl::StrCat(base_name, ".", field->json_name());
  if (field->options().HasExtension(validation_requirement) &&
      field->options().GetExtension(validation_requirement) ==
          ::google::fhir::proto::REQUIRED_BY_FHIR) {
    if (!FieldHasValue(message, field)) {
      return FailedPrecondition("missing-", new_base);
    }
  }
  if (IsMessageType<::google::fhir::stu3::proto::Reference>(
          field->message_type())) {
    return ValidateReference<::google::fhir::stu3::proto::Reference,
                             ::google::fhir::stu3::proto::ReferenceId>(
        message, field, base_name);
  }
  if (IsMessageType<::google::fhir::r4::core::Reference>(
          field->message_type())) {
    return ValidateReference<::google::fhir::r4::core::Reference,
                             ::google::fhir::r4::core::ReferenceId>(
        message, field, base_name);
  }
  if (field->cpp_type() == ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    for (int i = 0; i < PotentiallyRepeatedFieldSize(message, field); i++) {
      const auto& submessage = GetPotentiallyRepeatedMessage(message, field, i);
      FHIR_RETURN_IF_ERROR(ValidateFhirConstraints(submessage, new_base));

      // Run extra validation for some types, until FHIRPath validation covers
      // these cases as well.
      if (IsMessageType<::google::fhir::stu3::proto::Period>(
              field->message_type())) {
        FHIR_RETURN_IF_ERROR(
            ValidatePeriod<::google::fhir::stu3::proto::DateTime>(submessage,
                                                                  new_base));
      }
      if (IsMessageType<::google::fhir::r4::core::Period>(
              field->message_type())) {
        FHIR_RETURN_IF_ERROR(ValidatePeriod<::google::fhir::r4::core::DateTime>(
            submessage, new_base));
      }
    }
  }

  return Status::OK();
}

}  // namespace

// TODO: Invert the default here for FHIRPath handling, and have
// ValidateWithoutFhirPath instead of ValidateWithFhirPath

Status ValidateResourceWithFhirPath(const Message& resource) {
  FHIR_RETURN_IF_ERROR(
      ValidateFhirConstraints(resource, resource.GetDescriptor()->name()));
  return fhir_path::ValidateMessage(resource);
}

// TODO(nickgeorge, rbrush): Consider integrating handler func into validations
// in this file.
Status ValidateResourceWithFhirPath(const Message& resource,
                                    fhir_path::ViolationHandlerFunc handler) {
  FHIR_RETURN_IF_ERROR(
      ValidateFhirConstraints(resource, resource.GetDescriptor()->name()));
  return fhir_path::ValidateMessage(resource, handler);
}

Status ValidateResource(const Message& resource) {
  return ValidateFhirConstraints(resource, resource.GetDescriptor()->name());
}

}  // namespace fhir
}  // namespace google
