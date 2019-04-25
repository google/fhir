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

#include "google/fhir/stu3/resource_validation.h"

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/stu3/annotations.h"
#include "google/fhir/stu3/primitive_wrapper.h"
#include "google/fhir/stu3/proto_util.h"
#include "proto/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;

using ::google::fhir::stu3::proto::Reference;
using ::google::fhir::stu3::proto::ReferenceId;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::OneofDescriptor;
using ::google::protobuf::Reflection;
using ::tensorflow::errors::FailedPrecondition;
using ::tensorflow::errors::InvalidArgument;

namespace {

Status ValidateReference(const Message& message, const FieldDescriptor* field,
                         const string& base_name) {
  static const Descriptor* descriptor = Reference::descriptor();
  static const OneofDescriptor* oneof = descriptor->oneof_decl(0);

  for (int i = 0; i < PotentiallyRepeatedFieldSize(message, field); i++) {
    const Reference& reference =
        GetPotentiallyRepeatedMessage<Reference>(message, field, i);
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
    if (field->options().ExtensionSize(stu3::proto::valid_reference_type) ==
        0) {
      // The reference field does not have restrictions, so any value is fine.
      return Status::OK();
    }
    if (reference.has_uri() || reference.has_fragment()) {
      // Uri and Fragment references are untyped.
      return Status::OK();
    }

    if (IsMessageType<ReferenceId>(reference_field->message_type())) {
      const string& reference_type = reference_field->options().GetExtension(
          stu3::proto::referenced_fhir_type);
      bool is_allowed = false;
      for (int i = 0; i < field->options().ExtensionSize(
                              stu3::proto::valid_reference_type);
           i++) {
        const string& valid_type =
            field->options().GetExtension(stu3::proto::valid_reference_type, i);
        if (valid_type == reference_type || valid_type == "Resource") {
          is_allowed = true;
          break;
        }
      }
      if (!is_allowed) {
        return FailedPrecondition("invalid-reference-", base_name, ".",
                                  field->name(), "-", reference_type);
      }
    }
  }

  return Status::OK();
}

Status CheckField(const Message& message, const FieldDescriptor* field,
                  const string& base_name);

Status ValidateFhirConstraints(const Message& message,
                               const string& base_name) {
  if (IsPrimitive(message.GetDescriptor())) {
    return ValidatePrimitive(message).ok()
               ? Status::OK()
               : FailedPrecondition("invalid-primitive-", base_name);
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
        !oneof->options().GetExtension(stu3::proto::fhir_oneof_is_optional)) {
      FHIR_RETURN_IF_ERROR(::tensorflow::errors::FailedPrecondition(
          "empty-oneof-", oneof->full_name()));
    }
  }
  return Status::OK();
}

// Check if a required field is missing.
Status CheckField(const Message& message, const FieldDescriptor* field,
                  const string& base_name) {
  const string& new_base = absl::StrCat(base_name, ".", field->json_name());
  if (field->options().HasExtension(stu3::proto::validation_requirement) &&
      field->options().GetExtension(stu3::proto::validation_requirement) ==
          stu3::proto::REQUIRED_BY_FHIR) {
    if (!FieldHasValue(message, field)) {
      return FailedPrecondition("missing-", new_base);
    }
  }
  if (IsMessageType<Reference>(field->message_type())) {
    return ValidateReference(message, field, base_name);
  }
  if (field->cpp_type() == ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    for (int i = 0; i < PotentiallyRepeatedFieldSize(message, field); i++) {
      FHIR_RETURN_IF_ERROR(ValidateFhirConstraints(
          GetPotentiallyRepeatedMessage(message, field, i), new_base));
    }
  }
  return Status::OK();
}

}  // namespace

Status ValidateFhirConstraints(const ::google::protobuf::Message& message) {
  return ValidateFhirConstraints(message, message.GetDescriptor()->name());
}

Status ValidateResource(const Message& resource) {
  FHIR_RETURN_IF_ERROR(ValidateFhirConstraints(resource));

  return Status::OK();
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
