// Copyright 2019 Google LLC
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

#ifndef GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_VALIDATION_H_
#define GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_VALIDATION_H_

#include <unordered_map>

#include "google/protobuf/message.h"
#include "absl/base/macros.h"
#include "absl/synchronization/mutex.h"
#include "google/fhir/annotations.h"
#include "google/fhir/fhir_path/fhir_path.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/status/statusor.h"

namespace google {
namespace fhir {
namespace fhir_path {

// Class the holds the results of evaluating a FHIRPath constraint on a
// FHIR resource.
class ValidationResult {
 public:
  ValidationResult(const std::string& debug_path,
                   const std::string& fhirpath_constraint,
                   StatusOr<bool> result)
      : debug_path_(debug_path),
        fhirpath_constraint_(fhirpath_constraint),
        result_(result) {}

  // Returns the message or field that the FHIRPath constraint was evaluated on.
  // Either MessageName or MessageName.field.
  std::string DebugPath() const { return debug_path_; }

  // Returns the FHIRPath constraint that was evaluated.
  std::string Constraint() const { return fhirpath_constraint_; }

  // Returns the result of evaluating the FHIRPath constraint.
  //
  // For constraints that fail to compile/evaluate or do not evaluate to a
  // boolean, a status other than OK is returned.
  StatusOr<bool> EvaluationResult() const { return result_; }

 private:
  const std::string debug_path_;
  const std::string fhirpath_constraint_;
  const StatusOr<bool> result_;
};

// Class the holds the results of evaluating all FHIRPath constraints defined
// on a particular resource.
class ValidationResults {
 public:
  enum class ValidationBehavior {
    // All constraints must return true, if they evaluated to a boolean.
    // Expressions that fail to evaluate to a boolean value are ignored. Common
    // causes of an expression failing to evaluate to a boolean could be:
    //   - the constraint uses a portion of FHIRPath not currently supported by
    //     this library
    //   - the constraint is not a valid FHIRPath expression
    //   - the constraint does not yield a boolean value
    kRelaxed = 0,

    // All constraints must evaluate and return true.
    kStrict = 1,
  };

  ValidationResults(std::vector<ValidationResult> results)
      : results_(results) {}

  // Returns true if all FHIRPath constraints on the particular resource are
  // met. See ValidationBehavior for details on possible definitions of
  // validity. By default kStrict is used.
  bool IsValid(ValidationBehavior behavior = ValidationBehavior::kStrict) const;

  // Returns the result for each FHIRPath expressions that was evaluated.
  // TODO: Expose expressions that failed to compile.
  std::vector<ValidationResult> Results() const;

 private:
  const std::vector<ValidationResult> results_;
};

// This class validates that all fhir_path_constraint annotations on
// the given messages are valid. It will compile and cache the
// constraint expressions as it encounters them, so users are encouraged
// to create a single instance of this for the lifetime of the process.
// This class is thread safe.
class MessageValidator {
 public:
  MessageValidator();
  ~MessageValidator();

  // Validates the fhir_path_constraint annotations on the given message.
  // Returns Status::OK or the status of the first constraint violation
  // encountered. Users needing more details should use the overloaded
  // version of with a callback handler for each violation.
  ABSL_DEPRECATED(
    "Use Validate(const PrimitiveHandler*, const Message&) instead.")
  Status Validate(const ::google::protobuf::Message& message);

  // Validates the fhir_path_constraint annotations on the given message.
  ABSL_MUST_USE_RESULT
  ValidationResults Validate(const PrimitiveHandler* primitive_handler,
                             const ::google::protobuf::Message& message);

 private:
  // A cache of constraints for a given message definition
  struct MessageConstraints {
    // FHIRPath constraints at the "root" FHIR element, which is just the
    // protobuf message.
    std::vector<CompiledExpression> message_expressions;

    // FHIRPath constraints on fields
    std::vector<
        std::pair<const ::google::protobuf::FieldDescriptor*, const CompiledExpression>>
        field_expressions;

    // Nested messages that have constraints, so the evaluation logic
    // knows to check them.
    std::vector<const ::google::protobuf::FieldDescriptor*> nested_with_constraints;
  };

  // Loads constraints for the given descriptor.
  MessageConstraints* ConstraintsFor(
      const ::google::protobuf::Descriptor* descriptor,
      const PrimitiveHandler* primitive_handler);

  // Recursively called validation method that aggregates results into the
  // provided vector.
  void Validate(const internal::WorkspaceMessage& message,
                const PrimitiveHandler* primitive_handler,
                std::vector<ValidationResult>* results);

  absl::Mutex mutex_;
  std::unordered_map<std::string, std::unique_ptr<MessageConstraints>>
      constraints_cache_;
};

// Validates the fhir_path_constraint annotations on the given message.
// Returns Status::OK or the status of the first constraint violation
// encountered. Users needing more details should use the overloaded
// version of with a callback handler for each violation.
ABSL_DEPRECATED(
    "Use ValidateMessage(const PrimitiveHandler*, const Message&) instead.")
Status ValidateMessage(const ::google::protobuf::Message& message);

ABSL_MUST_USE_RESULT
ValidationResults ValidateMessage(const PrimitiveHandler* primitive_handler,
                                  const ::google::protobuf::Message& message);

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_VALIDATION_H_
