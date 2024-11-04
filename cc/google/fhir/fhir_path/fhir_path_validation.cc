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
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/fhir/annotations.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/fhir_path/fhir_path.h"
#include "google/fhir/json/json_util.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "google/protobuf/descriptor.h"

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

// ErrorHandler that aggregates FhirPath validations as ValidationResults
// This is useful for providing the deprecated Validate API that returns
// ValidationResults.  New usages should use an OperationOutcomeErrorHandler.
class ValidationResultsErrorHandler : public ErrorHandler {
 public:
  ValidationResults GetValidationResults() const {
    return ValidationResults(results_);
  }
  absl::Status HandleFhirPathFatal(const absl::Status& status,
                                   absl::string_view expression,
                                   absl::string_view element_path,
                                   absl::string_view field_path) override {
    results_.push_back(
        ValidationResult(field_path, element_path, expression, status));
    return absl::OkStatus();
  }

  absl::Status HandleFhirPathError(absl::string_view expression,
                                   absl::string_view element_path,
                                   absl::string_view field_path) override {
    results_.push_back(
        ValidationResult(field_path, element_path, expression, false));
    return absl::OkStatus();
  }

  absl::Status HandleFhirPathWarning(absl::string_view msg,
                                     absl::string_view element_path,
                                     absl::string_view field_path) override {
    // Legacy FHIRPath API does not support warnings.
    return absl::OkStatus();
  }

  absl::Status HandleFhirFatal(const absl::Status& status,
                               absl::string_view element_path,
                               absl::string_view field_path) override {
    return absl::InternalError(
        "ValidationResultsErrorHandler should only use FHIRPath APIs.");
    return absl::OkStatus();
  }

  absl::Status HandleFhirError(absl::string_view msg,
                               absl::string_view element_path,
                               absl::string_view field_path) override {
    return absl::InternalError(
        "ValidationResultsErrorHandler should only use FHIRPath APIs.");
    return absl::OkStatus();
  }

  absl::Status HandleFhirWarning(absl::string_view msg,
                                 absl::string_view element_path,
                                 absl::string_view field_path) override {
    return absl::InternalError(
        "ValidationResultsErrorHandler should only use FHIRPath APIs.");
    return absl::OkStatus();
  }

  bool HasFatals() const override {
    for (const ValidationResult& result : results_) {
      if (!result.EvaluationResult().ok()) {
        return true;
      }
    }
    return false;
  }
  bool HasErrors() const override {
    for (const ValidationResult& result : results_) {
      if (result.EvaluationResult().ok() && !*result.EvaluationResult()) {
        return true;
      }
    }
    return false;
  }
  bool HasWarnings() const override {
    // Legacy FHIRPath API does not support warnings.
    return false;
  }

 private:
  std::vector<ValidationResult> results_;
};

namespace {

enum class Severity { kFailure, kWarning };

void AddMessageConstraints(const Descriptor* descriptor,
                           const Severity severity,
                           const PrimitiveHandler* primitive_handler,
                           std::vector<CompiledExpression>* constraints) {
  auto extension_type = severity == Severity::kWarning
                            ? proto::fhir_path_message_warning_constraint
                            : proto::fhir_path_message_constraint;
  int ext_size = descriptor->options().ExtensionSize(extension_type);

  for (int i = 0; i < ext_size; ++i) {
    const std::string& fhir_path =
        descriptor->options().GetExtension(extension_type, i);
    auto constraint =
        CompiledExpression::Compile(descriptor, primitive_handler, fhir_path);
    if (constraint.ok()) {
      CompiledExpression expression = constraint.value();
      constraints->push_back(expression);
    } else {
      LOG(WARNING) << "Ignoring message constraint on " << descriptor->name()
                   << " (" << fhir_path << "). "
                   << constraint.status().message();
    }

    // TODO(b/151745508): Unsupported FHIRPath expressions are simply not
    // validated for now; this should produce an error once we support
    // all of FHIRPath.
  }
}

void AddFieldConstraints(
    const FieldDescriptor* field, const Severity severity,
    const PrimitiveHandler* primitive_handler,
    std::vector<std::pair<const ::google::protobuf::FieldDescriptor*,
                          const CompiledExpression>>* constraints) {
  const Descriptor* field_type = field->message_type();
  const auto extension_type = severity == Severity::kWarning
                                  ? proto::fhir_path_warning_constraint
                                  : proto::fhir_path_constraint;

  // Constraints only apply to non-primitives.
  if (field_type != nullptr) {
    int ext_size = field->options().ExtensionSize(extension_type);

    for (int j = 0; j < ext_size; ++j) {
      const std::string& fhir_path =
          field->options().GetExtension(extension_type, j);
      auto constraint =
          CompiledExpression::Compile(field_type, primitive_handler, fhir_path);

      if (constraint.ok()) {
        constraints->push_back(std::make_pair(field, *constraint));
      } else {
        LOG(WARNING) << "Ignoring field constraint on "
                     << field->message_type()->name() << "."
                     << field_type->name() << " (" << fhir_path << ")."
                     << constraint.status().message();
      }

      // TODO(b/151745508): Unsupported FHIRPath expressions are simply not
      // validated for now; this should produce an error once we support
      // all of FHIRPath.
    }
  }
}

}  // namespace

// Build the constraints for the given message type and
// add it to the constraints cache.
FhirPathValidator::MessageConstraints* FhirPathValidator::ConstraintsFor(
    const Descriptor* descriptor,
    const PrimitiveHandler* primitive_handler) const {
  // Simply return the cached constraint if it exists.
  std::unique_ptr<MessageConstraints>& constraints =
      constraints_cache_[std::string(descriptor->full_name())];

  if (constraints != nullptr) {
    return constraints.get();
  }

  constraints = std::make_unique<MessageConstraints>();
  AddMessageConstraints(descriptor, Severity::kFailure, primitive_handler,
                        &constraints->message_error_expressions);
  AddMessageConstraints(descriptor, Severity::kWarning, primitive_handler,
                        &constraints->message_warning_expressions);

  for (int i = 0; i < descriptor->field_count(); i++) {
    AddFieldConstraints(descriptor->field(i), Severity::kFailure,
                        primitive_handler,
                        &constraints->field_error_expressions);
    AddFieldConstraints(descriptor->field(i), Severity::kWarning,
                        primitive_handler,
                        &constraints->field_warning_expressions);
  }

  // Now we recursively look for fields with constraints.
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);

    const Descriptor* field_type = field->message_type();

    // Constraints only apply to non-primitives.
    if (field_type != nullptr) {
      // Validate the field type.
      auto child_constraints = ConstraintsFor(field_type, primitive_handler);

      // Nested fields that directly or transitively have constraints
      // are retained and used when applying constraints.
      if (!child_constraints->message_error_expressions.empty() ||
          !child_constraints->message_warning_expressions.empty() ||
          !child_constraints->field_error_expressions.empty() ||
          !child_constraints->field_warning_expressions.empty() ||
          !child_constraints->nested_with_constraints.empty()) {
        constraints->nested_with_constraints.push_back(field);
      }
    }
  }

  return constraints.get();
}

// Validates the given message against a given FHIRPath expression.
// Reports failures to the provided error reporter
absl::Status ValidateConstraint(const internal::WorkspaceMessage& message,
                                const CompiledExpression& expression,
                                const Severity severity,
                                const ScopedErrorReporter& error_reporter) {
  static const auto* invalid_expressions = new absl::flat_hash_set<std::string>(
      {"where(category.memberOf('http://hl7.org/fhir/us/core/ValueSet/"
       "us-core-condition-category')).exists()",
       "where(category in 'http://hl7.org/fhir/us/core/ValueSet/"
       "us-core-condition-category').exists()",
       "name.matches('[A-Z]([A-Za-z0-9_]){0,254}')"});
  if (invalid_expressions->contains(expression.fhir_path())) {
    // TODO(b/221470795):  Eliminate this once technical corrections are handled
    // upstream.
    return absl::OkStatus();
  }
  absl::StatusOr<EvaluationResult> expr_result = expression.Evaluate(message);
  if (!expr_result.ok()) {
    // Error evaluating expression
    return error_reporter.ReportFhirPathFatal(expr_result.status(),
                                              expression.fhir_path());
  }
  if (!expr_result->GetBoolean().ok()) {
    // Expression did not evaluate to a boolean
    return error_reporter.ReportFhirPathFatal(
        absl::FailedPreconditionError(
            "Cannot validate non-boolean FHIRPath expression"),
        expression.fhir_path());
  }
  if (!*expr_result->GetBoolean()) {
    // Expression evaluated to a boolean value of "false", indicating the
    // resource failed to meet the constraint.
    switch (severity) {
      case Severity::kFailure:
        return error_reporter.ReportFhirPathError(expression.fhir_path());
      case Severity::kWarning:
        return error_reporter.ReportFhirPathWarning(expression.fhir_path());
    }
  }
  // The expression evaluated to the boolean value `true`.
  // Nothing needs to be reported.
  return absl::OkStatus();
}

std::string PathTerm(const Message& message, const FieldDescriptor* field) {
  if (IsContainedResource(message) ||
      IsChoiceTypeContainer(message.GetDescriptor())) {
    return absl::StrCat("ofType(", field->message_type()->name(), ")");
  }
  return std::string(FhirJsonName(field));
}

namespace {
absl::Status HandleFieldConstraint(
    const internal::WorkspaceMessage& workspace_message,
    const FieldDescriptor* field, const CompiledExpression& expr,
    const Severity severity, const ScopedErrorReporter& error_reporter) {
  const Message& message = *workspace_message.Message();
  for (int i = 0; i < PotentiallyRepeatedFieldSize(message, field); i++) {
    const Message& child = GetPotentiallyRepeatedMessage(message, field, i);
    FHIR_RETURN_IF_ERROR(ValidateConstraint(
        internal::WorkspaceMessage(workspace_message, &child), expr, severity,
        error_reporter.WithScope(PathTerm(message, field),
                                 field->is_repeated()
                                     ? std::optional<std::uint8_t>(i)
                                     : std::nullopt)

            ));
  }
  return absl::OkStatus();
}
}  // namespace

absl::Status FhirPathValidator::Validate(
    const internal::WorkspaceMessage& message,
    const PrimitiveHandler* primitive_handler,
    const ScopedErrorReporter& error_reporter) const {
  // ConstraintsFor may recursively build constraints so
  // we lock the mutex here to ensure thread safety.
  mutex_.Lock();
  MessageConstraints* constraints =
      ConstraintsFor(message.Message()->GetDescriptor(), primitive_handler);
  mutex_.Unlock();

  // Validate the constraints attached to the message root.
  for (const CompiledExpression& expr :
       constraints->message_error_expressions) {
    FHIR_RETURN_IF_ERROR(
        ValidateConstraint(message, expr, Severity::kFailure, error_reporter));
  }
  for (const CompiledExpression& expr :
       constraints->message_warning_expressions) {
    FHIR_RETURN_IF_ERROR(
        ValidateConstraint(message, expr, Severity::kWarning, error_reporter));
  }

  // Validate the constraints attached to the message's fields.
  for (const auto& expression : constraints->field_error_expressions) {
    const FieldDescriptor* field = expression.first;
    const CompiledExpression& expr = expression.second;
    FHIR_RETURN_IF_ERROR(HandleFieldConstraint(
        message, field, expr, Severity::kFailure, error_reporter));
  }
  for (const auto& expression : constraints->field_warning_expressions) {
    const FieldDescriptor* field = expression.first;
    const CompiledExpression& expr = expression.second;
    FHIR_RETURN_IF_ERROR(HandleFieldConstraint(
        message, field, expr, Severity::kWarning, error_reporter));
  }

  // Recursively validate constraints for nested messages that have them.
  for (const FieldDescriptor* field : constraints->nested_with_constraints) {
    const std::string path_term = PathTerm(*message.Message(), field);
    const Message& proto = *message.Message();

    for (int i = 0; i < PotentiallyRepeatedFieldSize(proto, field); i++) {
      const Message& child = GetPotentiallyRepeatedMessage(proto, field, i);

      FHIR_RETURN_IF_ERROR(Validate(
          internal::WorkspaceMessage(message, &child), primitive_handler,
          error_reporter.WithScope(
              path_term, field->is_repeated() ? std::optional<std::uint8_t>(i)
                                              : std::nullopt)

              ));
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

absl::Status FhirPathValidator::Validate(
    const Message& message, const PrimitiveHandler* primitive_handler,
    ErrorHandler& error_handler) const {
  // ContainedResource is an implementation detail of FHIR protos. Extract the
  // resource from the wrapper before processing so that wrapper is not included
  // in the node/constraint path of the validation results.
  if (IsContainedResource(message)) {
    FHIR_ASSIGN_OR_RETURN(const Message* resource,
                          GetContainedResource(message));
    return Validate(*resource, primitive_handler, error_handler);
  }

  const ScopedErrorReporter reporter(&error_handler,
                                     message.GetDescriptor()->name());
  return Validate(internal::WorkspaceMessage(&message), primitive_handler,
                  reporter);
}

absl::StatusOr<ValidationResults> FhirPathValidator::Validate(
    const ::google::protobuf::Message& message,
    const PrimitiveHandler* primitive_handler) const {
  ValidationResultsErrorHandler error_reporter;
  FHIR_RETURN_IF_ERROR(Validate(message, primitive_handler, error_reporter));
  return error_reporter.GetValidationResults();
}

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
