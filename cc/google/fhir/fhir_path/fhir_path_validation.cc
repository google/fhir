// Copyright 2020 Google LLC
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

#include "google/fhir/fhir_path/fhir_path_validation.h"

#include <utility>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/util/message_differencer.h"
#include "absl/memory/memory.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/time/civil_time.h"
#include "absl/time/time.h"
#include "absl/types/optional.h"
#include "google/fhir/annotations.h"
#include "google/fhir/fhir_path/fhir_path.h"
#include "google/fhir/fhir_path/utils.h"
#include "google/fhir/primitive_wrapper.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/r4/primitive_handler.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/stu3/primitive_handler.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "proto/r4/core/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace fhir_path {

using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::fhir::ForEachMessageHalting;
using ::google::fhir::StatusOr;
using ::tensorflow::errors::InvalidArgument;

// TODO: This method forces linking in all supported versions of
// FHIR. It should be replaced by a passed-in PrimitiveHandler.
StatusOr<const PrimitiveHandler*> GetPrimitiveHandler(
    const Message& message) {
  const auto version = GetFhirVersion(message.GetDescriptor());
  switch (version) {
    case proto::FhirVersion::STU3:
      return stu3::Stu3PrimitiveHandler::GetInstance();
    case proto::FhirVersion::R4:
      return r4::R4PrimitiveHandler::GetInstance();
    default:
      return InvalidArgument("Invalid FHIR version for FhirPath: ",
                             FhirVersion_Name(version));
  }
}

MessageValidator::MessageValidator() {}
MessageValidator::~MessageValidator() {}

// Build the constraints for the given message type and
// add it to the constraints cache.
StatusOr<MessageValidator::MessageConstraints*>
MessageValidator::ConstraintsFor(const Descriptor* descriptor,
                                 const PrimitiveHandler* primitive_handler) {
  // Simply return the cached constraint if it exists.
  auto iter = constraints_cache_.find(descriptor->full_name());

  if (iter != constraints_cache_.end()) {
    return iter->second.get();
  }

  auto constraints = absl::make_unique<MessageConstraints>();

  int ext_size =
      descriptor->options().ExtensionSize(proto::fhir_path_message_constraint);

  for (int i = 0; i < ext_size; ++i) {
    const std::string& fhir_path = descriptor->options().GetExtension(
        proto::fhir_path_message_constraint, i);
    auto constraint =
        CompiledExpression::Compile(descriptor, primitive_handler, fhir_path);
    if (constraint.ok()) {
      CompiledExpression expression = constraint.ValueOrDie();
      constraints->message_expressions.push_back(expression);
    } else {
      LOG(WARNING) << "Ignoring message constraint on " << descriptor->name()
                   << " (" << fhir_path << "). "
                   << constraint.status().error_message();
    }

    // TODO: Unsupported FHIRPath expressions are simply not
    // validated for now; this should produce an error once we support
    // all of FHIRPath.
  }

  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);

    const Descriptor* field_type = field->message_type();

    // Constraints only apply to non-primitives.
    if (field_type != nullptr) {
      int ext_size =
          field->options().ExtensionSize(proto::fhir_path_constraint);

      for (int j = 0; j < ext_size; ++j) {
        const std::string& fhir_path =
            field->options().GetExtension(proto::fhir_path_constraint, j);

        auto constraint = CompiledExpression::Compile(
            field_type, primitive_handler, fhir_path);

        if (constraint.ok()) {
          constraints->field_expressions.push_back(
              std::make_pair(field, constraint.ValueOrDie()));
        } else {
          LOG(WARNING) << "Ignoring field constraint on " << descriptor->name()
                       << "." << field_type->name() << " (" << fhir_path
                       << "). " << constraint.status().error_message();
        }

        // TODO: Unsupported FHIRPath expressions are simply not
        // validated for now; this should produce an error once we support
        // all of FHIRPath.
      }
    }
  }

  // Add the successful constraints to the cache while keeping a local
  // reference.
  MessageConstraints* constraints_local = constraints.get();
  constraints_cache_[descriptor->full_name()] = std::move(constraints);

  // Now we recursively look for fields with constraints.
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);

    const Descriptor* field_type = field->message_type();

    // Constraints only apply to non-primitives.
    if (field_type != nullptr) {
      // Validate the field type.
      FHIR_ASSIGN_OR_RETURN(auto child_constraints,
                            ConstraintsFor(field_type, primitive_handler));

      // Nested fields that directly or transitively have constraints
      // are retained and used when applying constraints.
      if (!child_constraints->message_expressions.empty() ||
          !child_constraints->field_expressions.empty() ||
          !child_constraints->nested_with_constraints.empty()) {
        constraints_local->nested_with_constraints.push_back(field);
      }
    }
  }

  return constraints_local;
}

// Default handler halts on first error
bool HaltOnErrorHandler(const Message& message, const FieldDescriptor* field,
                        const std::string& constraint) {
  return true;
}

Status MessageValidator::Validate(const Message& message) {
  return Validate(message, HaltOnErrorHandler);
}

// Validates that the given message satisfies the
// the given FHIRPath expression, invoking the handler in case
// of failure.
Status ValidateMessageConstraint(const internal::WorkspaceMessage& message,
                                 const CompiledExpression& expression,
                                 const ViolationHandlerFunc handler,
                                 bool* halt_validation) {
  FHIR_ASSIGN_OR_RETURN(const EvaluationResult expr_result,
                        expression.Evaluate(message));

  if (!expr_result.GetBoolean().ok()) {
    *halt_validation = true;
    return InvalidArgument("Constraint did not evaluate to boolean: ",
                           message.Message()->GetDescriptor()->name(), ": \"",
                           expression.fhir_path(), "\"");
  }

  if (!expr_result.GetBoolean().ValueOrDie()) {
    std::string err_msg =
        absl::StrCat("fhirpath-constraint-violation-",
                     message.Message()->GetDescriptor()->name(), ": \"",
                     expression.fhir_path(), "\"");

    *halt_validation =
        handler(*message.Message(), nullptr, expression.fhir_path());
    return ::tensorflow::errors::FailedPrecondition(err_msg);
  }

  return Status::OK();
}

// Validates that the given field in the given parent satisfies the
// the given FHIRPath expression, invoking the handler in case
// of failure.
Status ValidateFieldConstraint(const Message& parent,
                               const FieldDescriptor* field,
                               const internal::WorkspaceMessage& field_value,
                               const CompiledExpression& expression,
                               const ViolationHandlerFunc handler,
                               bool* halt_validation) {
  FHIR_ASSIGN_OR_RETURN(const EvaluationResult expr_result,
                        expression.Evaluate(field_value));
  FHIR_ASSIGN_OR_RETURN(bool result, expr_result.GetBoolean());

  if (!result) {
    std::string err_msg = absl::StrCat(
        "fhirpath-constraint-violation-", field->containing_type()->name(), ".",
        field->json_name(), ": \"", expression.fhir_path(), "\"");

    *halt_validation = handler(parent, field, expression.fhir_path());
    return ::tensorflow::errors::FailedPrecondition(err_msg);
  }

  return Status::OK();
}

// Store the first detected failure in the accumulative status.
void UpdateStatus(Status* accumulative_status, const Status& current_status) {
  if (accumulative_status->ok() && !current_status.ok()) {
    *accumulative_status = current_status;
  }
}

Status MessageValidator::Validate(const Message& message,
                                  ViolationHandlerFunc handler) {
  bool halt_validation = false;
  FHIR_ASSIGN_OR_RETURN(auto primitive_handler, GetPrimitiveHandler(message));
  return Validate(internal::WorkspaceMessage(&message), primitive_handler,
                  handler, &halt_validation);
}

Status MessageValidator::Validate(const internal::WorkspaceMessage& message,
                                  const PrimitiveHandler* primitive_handler,
                                  ViolationHandlerFunc handler,
                                  bool* halt_validation) {
  // ConstraintsFor may recursively build constraints so
  // we lock the mutex here to ensure thread safety.
  mutex_.Lock();
  auto status_or_constraints =
      ConstraintsFor(message.Message()->GetDescriptor(), primitive_handler);
  mutex_.Unlock();

  if (!status_or_constraints.ok()) {
    return status_or_constraints.status();
  }

  auto constraints = status_or_constraints.ValueOrDie();

  // Keep the first failure to return to the caller.
  Status accumulative_status = Status::OK();

  // Validate the constraints attached to the message root.
  for (const CompiledExpression& expr : constraints->message_expressions) {
    UpdateStatus(
        &accumulative_status,
        ValidateMessageConstraint(message, expr, handler, halt_validation));
    if (*halt_validation) {
      return accumulative_status;
    }
  }

  // Validate the constraints attached to the message's fields.
  for (auto expression : constraints->field_expressions) {
    if (*halt_validation) {
      return accumulative_status;
    }

    const FieldDescriptor* field = expression.first;
    const CompiledExpression& expr = expression.second;

    ForEachMessageHalting<Message>(
        *message.Message(), field, [&](const Message& child) {
          UpdateStatus(&accumulative_status,
                       ValidateFieldConstraint(
                           *message.Message(), field,
                           internal::WorkspaceMessage(message, &child), expr,
                           handler, halt_validation));

          return *halt_validation;
        });
  }

  // Recursively validate constraints for nested messages that have them.
  for (const FieldDescriptor* field : constraints->nested_with_constraints) {
    if (*halt_validation) {
      return accumulative_status;
    }

    ForEachMessageHalting<Message>(
        *message.Message(), field, [&](const Message& child) {
          UpdateStatus(&accumulative_status,
                       Validate(internal::WorkspaceMessage(message, &child),
                                primitive_handler, handler, halt_validation));
          return *halt_validation;
        });
  }

  return accumulative_status;
}

// Common validator instance for the lifetime of the process.
static MessageValidator* validator = new MessageValidator();

Status ValidateMessage(const ::google::protobuf::Message& message) {
  return validator->Validate(message);
}

Status ValidateMessage(const ::google::protobuf::Message& message,
                       ViolationHandlerFunc handler) {
  return validator->Validate(message, handler);
}

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
