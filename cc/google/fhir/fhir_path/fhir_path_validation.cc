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
using ::google::fhir::ForEachMessage;
using ::google::fhir::StatusOr;
using ::tensorflow::errors::InvalidArgument;

bool ValidationResults::IsValid(ValidationBehavior behavior) const {
  for (const ValidationResult& result : results_) {
    if (!result.EvaluationResult().ok()) {
      if (behavior == ValidationBehavior::kStrict) {
        return false;
      }

      continue;
    }

    if (!result.EvaluationResult().ValueOrDie()) {
      return false;
    }
  }

  return true;
}

std::vector<ValidationResult> ValidationResults::Results() const {
  return results_;
}

FhirPathValidator::~FhirPathValidator() {}

// Build the constraints for the given message type and
// add it to the constraints cache.
FhirPathValidator::MessageConstraints* FhirPathValidator::ConstraintsFor(
    const Descriptor* descriptor) {
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
        CompiledExpression::Compile(descriptor, primitive_handler_, fhir_path);
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
            field_type, primitive_handler_, fhir_path);

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
      auto child_constraints = ConstraintsFor(field_type);

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

// Validates that the given message satisfies the given FHIRPath expression.
ValidationResult ValidateMessageConstraint(
    const internal::WorkspaceMessage& message,
    const CompiledExpression& expression) {
  StatusOr<EvaluationResult> expr_result = expression.Evaluate(message);
  return ValidationResult(
      message.Message()->GetDescriptor()->name(), expression.fhir_path(),
      expr_result.ok() ? expr_result.ValueOrDie().GetBoolean()
                       : expr_result.status());
}

// Validates that the given field in the given parent satisfies the given
// FHIRPath expression.
ValidationResult ValidateFieldConstraint(
    const Message& parent, const FieldDescriptor* field,
    const internal::WorkspaceMessage& field_value,
    const CompiledExpression& expression) {
  StatusOr<EvaluationResult> expr_result = expression.Evaluate(field_value);
  return ValidationResult(
      absl::StrCat(field->containing_type()->name(), ".", field->json_name()),
      expression.fhir_path(),
      expr_result.ok() ? expr_result.ValueOrDie().GetBoolean()
                       : expr_result.status());
}

void FhirPathValidator::Validate(const internal::WorkspaceMessage& message,
                                 std::vector<ValidationResult>* results) {
  // ConstraintsFor may recursively build constraints so
  // we lock the mutex here to ensure thread safety.
  mutex_.Lock();
  MessageConstraints* constraints =
      ConstraintsFor(message.Message()->GetDescriptor());
  mutex_.Unlock();

  // Validate the constraints attached to the message root.
  for (const CompiledExpression& expr : constraints->message_expressions) {
    results->push_back(ValidateMessageConstraint(message, expr));
  }

  // Validate the constraints attached to the message's fields.
  for (auto expression : constraints->field_expressions) {
    const FieldDescriptor* field = expression.first;
    const CompiledExpression& expr = expression.second;

    ForEachMessage<Message>(
        *message.Message(), field, [&](const Message& child) {
          results->push_back(ValidateFieldConstraint(
              *message.Message(), field,
              internal::WorkspaceMessage(message, &child), expr));
        });
  }

  // Recursively validate constraints for nested messages that have them.
  for (const FieldDescriptor* field : constraints->nested_with_constraints) {
    ForEachMessage<Message>(
        *message.Message(), field, [&](const Message& child) {
          Validate(internal::WorkspaceMessage(message, &child), results);
        });
  }
}

Status ValidationResults::LegacyValidationResult() const {
  if (IsValid(ValidationResults::ValidationBehavior::kRelaxed)) {
    return Status::OK();
  }

  auto result = find_if(results_.begin(), results_.end(), [](auto result) {
    return !result.EvaluationResult().ok() ||
           !result.EvaluationResult().ValueOrDie();
  });

  return ::tensorflow::errors::FailedPrecondition(
      "fhirpath-constraint-violation-", (*result).DebugPath(), ": \"",
      (*result).Constraint(), "\"");
}

ValidationResults FhirPathValidator::Validate(
    const ::google::protobuf::Message& message) {
  std::vector<ValidationResult> results;
  Validate(internal::WorkspaceMessage(&message), &results);
  return ValidationResults(results);
}

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
