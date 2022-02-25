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

#include <algorithm>
#include <functional>
#include <string>
#include <utility>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/util/message_differencer.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/fhir/annotations.h"
#include "google/fhir/fhir_path/fhir_path.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/google/fhir/proto/annotations.pb.h"

namespace google {
namespace fhir {
namespace fhir_path {

using ::google::fhir::GetPotentiallyRepeatedMessage;
using ::google::fhir::PotentiallyRepeatedFieldSize;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;

bool ValidationResults::StrictValidationFn(const ValidationResult& result) {
  return result.EvaluationResult().ok() && result.EvaluationResult().value();
}

bool ValidationResults::RelaxedValidationFn(const ValidationResult& result) {
  return !result.EvaluationResult().ok() || result.EvaluationResult().value();
}

bool ValidationResults::IsValid(
    std::function<bool(const ValidationResult&)> validation_fn) const {
  return std::all_of(results_.begin(), results_.end(), validation_fn);
}

std::vector<ValidationResult> ValidationResults::Results() const {
  return results_;
}

// ErrorReporter that aggregates FhirPath validations as ValidationResults
// This is useful for providing the deprecated Validate API that returns
// ValidationResults.  New usages should use an OperationOutcomeErrorReporter.
class ValidationResultsErrorReporter : public ErrorReporter {
 public:
  ValidationResults GetValidationResults() const {
    return ValidationResults(results_);
  }
  absl::Status ReportFhirPathFatal(absl::string_view field_path,
                                   absl::string_view element_path,
                                   absl::string_view fhir_path_constraint,
                                   const absl::Status& status) override {
    results_.push_back(ValidationResult(field_path, element_path,
                                        fhir_path_constraint, status));
    return absl::OkStatus();
  }

  absl::Status ReportFhirPathError(
      absl::string_view field_path, absl::string_view element_path,
      absl::string_view fhir_path_constraint) override {
    results_.push_back(ValidationResult(field_path, element_path,
                                        fhir_path_constraint, false));
    return absl::OkStatus();
  }

  absl::Status ReportFhirPathWarning(
      absl::string_view field_path, absl::string_view element_path,
      absl::string_view fhir_path_constraint) override {
    return absl::InternalError(
        "ValidationResultsErrorReporter does not support WARNINGS.");
  }

  absl::Status ReportFhirFatal(absl::string_view field_path,
                               absl::string_view element_path,
                               const absl::Status& status) override {
    return absl::InternalError(
        "ValidationResultsErrorReporter should only use FhirPath APIs.");
  }

  absl::Status ReportFhirWarning(absl::string_view field_path,
                                 absl::string_view element_path,
                                 absl::string_view message) override {
    return absl::InternalError(
        "ValidationResultsErrorReporter should only use FhirPath APIs.");
  }

  absl::Status ReportFhirError(absl::string_view field_path,
                               absl::string_view element_path,
                               absl::string_view message) override {
    return absl::InternalError(
        "ValidationResultsErrorReporter should only use FhirPath APIs.");
  }

 private:
  std::vector<ValidationResult> results_;
};

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
  AddMessageConstraints(descriptor, constraints.get());

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
              std::make_pair(field, constraint.value()));
        } else {
          LOG(WARNING) << "Ignoring field constraint on " << descriptor->name()
                       << "." << field_type->name() << " (" << fhir_path
                       << "). " << constraint.status().message();
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

// Build the message constraints for the given message type and
// add it to the constraints cache.
void FhirPathValidator::AddMessageConstraints(const Descriptor* descriptor,
                                              MessageConstraints* constraints) {
  int ext_size =
      descriptor->options().ExtensionSize(proto::fhir_path_message_constraint);

  for (int i = 0; i < ext_size; ++i) {
    const std::string& fhir_path = descriptor->options().GetExtension(
        proto::fhir_path_message_constraint, i);
    auto constraint =
        CompiledExpression::Compile(descriptor, primitive_handler_, fhir_path);
    if (constraint.ok()) {
      CompiledExpression expression = constraint.value();
      constraints->message_expressions.push_back(expression);
    } else {
      LOG(WARNING) << "Ignoring message constraint on " << descriptor->name()
                   << " (" << fhir_path << "). "
                   << constraint.status().message();
    }

    // TODO: Unsupported FHIRPath expressions are simply not
    // validated for now; this should produce an error once we support
    // all of FHIRPath.
  }
}

// Validates the given message against a given FHIRPath expression.
// Reports failures to the provided error reporter
absl::Status ValidateConstraint(const absl::string_view constraint_parent_path,
                                const absl::string_view node_parent_path,
                                const internal::WorkspaceMessage& message,
                                const CompiledExpression& expression,
                                ErrorReporter* error_reporter) {
  absl::StatusOr<EvaluationResult> expr_result = expression.Evaluate(message);
  if (!expr_result.ok()) {
    // Error evaluating expression
    return error_reporter->ReportFhirPathFatal(
        constraint_parent_path, node_parent_path, expression.fhir_path(),
        expr_result.status());
  }
  if (!expr_result->GetBoolean().ok()) {
    // Expression did not evaluate to a boolean
    return error_reporter->ReportFhirPathFatal(
        constraint_parent_path, node_parent_path, expression.fhir_path(),
        absl::FailedPreconditionError(
            "Cannot Validate non-boolean FhirPath expression."));
  }
  if (!*expr_result->GetBoolean()) {
    // Expression evaluated to a boolean value of "false", indicating the
    // resource failed to meet the constraint.
    return error_reporter->ReportFhirPathError(
        constraint_parent_path, node_parent_path, expression.fhir_path());
  }
  // The expression evaluated to the boolean value `true`.
  // Nothing needs to be reported.
  return absl::OkStatus();
}

std::string PathTerm(const Message& message, const FieldDescriptor* field) {
  return IsContainedResource(message) ||
                 IsChoiceTypeContainer(message.GetDescriptor())
             ? absl::StrCat("ofType(", field->message_type()->name(), ")")
             : field->json_name();
}

absl::Status FhirPathValidator::Validate(
    absl::string_view constraint_path, absl::string_view node_path,
    const internal::WorkspaceMessage& message, ErrorReporter* error_reporter) {
  // ConstraintsFor may recursively build constraints so
  // we lock the mutex here to ensure thread safety.
  mutex_.Lock();
  MessageConstraints* constraints =
      ConstraintsFor(message.Message()->GetDescriptor());
  mutex_.Unlock();

  // Validate the constraints attached to the message root.
  for (const CompiledExpression& expr : constraints->message_expressions) {
    FHIR_RETURN_IF_ERROR(ValidateConstraint(constraint_path, node_path, message,
                                            expr, error_reporter));
  }

  // Validate the constraints attached to the message's fields.
  for (const auto& expression : constraints->field_expressions) {
    const FieldDescriptor* field = expression.first;
    const CompiledExpression& expr = expression.second;
    const std::string path_term = PathTerm(*message.Message(), field);
    const Message& proto = *message.Message();

    for (int i = 0; i < PotentiallyRepeatedFieldSize(proto, field); i++) {
      const Message& child = GetPotentiallyRepeatedMessage(proto, field, i);

      FHIR_RETURN_IF_ERROR(ValidateConstraint(
          absl::StrCat(constraint_path, ".", path_term),
          field->is_repeated()
              ? absl::StrCat(node_path, ".", path_term, "[", i, "]")
              : absl::StrCat(node_path, ".", path_term),
          internal::WorkspaceMessage(message, &child), expr, error_reporter));
    }
  }

  // Recursively validate constraints for nested messages that have them.
  for (const FieldDescriptor* field : constraints->nested_with_constraints) {
    const std::string path_term = PathTerm(*message.Message(), field);
    const Message& proto = *message.Message();

    for (int i = 0; i < PotentiallyRepeatedFieldSize(proto, field); i++) {
      const Message& child = GetPotentiallyRepeatedMessage(proto, field, i);

      FHIR_RETURN_IF_ERROR(Validate(
          absl::StrCat(constraint_path, ".", path_term),
          field->is_repeated()
              ? absl::StrCat(node_path, ".", path_term, "[", i, "]")
              : absl::StrCat(node_path, ".", path_term),
          internal::WorkspaceMessage(message, &child), error_reporter));
    }
  }
  return absl::OkStatus();
}

absl::Status ValidationResults::LegacyValidationResult() const {
  if (IsValid(&ValidationResults::RelaxedValidationFn)) {
    return absl::OkStatus();
  }

  auto result = find_if(results_.begin(), results_.end(), [](auto result) {
    return !result.EvaluationResult().ok() ||
           !result.EvaluationResult().value();
  });

  return ::absl::FailedPreconditionError(
      absl::StrCat("fhirpath-constraint-violation-", (*result).ConstraintPath(),
                   ": \"", (*result).Constraint(), "\""));
}

absl::Status FhirPathValidator::Validate(const Message& message,
                                         ErrorReporter* error_reporter) {
  // ContainedResource is an implementation detail of FHIR protos. Extract the
  // resource from the wrapper before prcoessing so that wrapper is not included
  // in the node/constraint path of the validation results.
  if (IsContainedResource(message)) {
    FHIR_ASSIGN_OR_RETURN(const Message* resource,
                          GetContainedResource(message));
    return Validate(*resource, error_reporter);
  }

  return Validate(message.GetDescriptor()->name(),
                  message.GetDescriptor()->name(),
                  internal::WorkspaceMessage(&message), error_reporter);
}

absl::StatusOr<ValidationResults> FhirPathValidator::Validate(
    const ::google::protobuf::Message& message) {
  ValidationResultsErrorReporter error_reporter;
  FHIR_RETURN_IF_ERROR(Validate(message, &error_reporter));
  return error_reporter.GetValidationResults();
}

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
