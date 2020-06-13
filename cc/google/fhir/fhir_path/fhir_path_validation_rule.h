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

#ifndef GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_VALIDATION_RULE_H_
#define GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_VALIDATION_RULE_H_

#include "absl/container/flat_hash_set.h"
#include "absl/hash/hash.h"
#include "google/fhir/fhir_path/fhir_path_validation.h"

namespace google::fhir::fhir_path {

// A lightweight builder for creating custom validation functions.
//
// Example:
// auto validation_rule_function = ValidationRuleBuilder()
//     .IgnoringConstraint("Patient.contact", "name.exists()...")
//     .IgorningConstraints({
//         {"Bundle.entry", "fullUrl.contains('/_history/').not()"},
//         {"Bundle", "type = 'document' implies (timestamp.hasValue())"}
//     })
//     .StrictValidation();
//
//  ValidationResults validation_results = [...];
//  bool is_valid = validation_results.IsValid(validation_rule_function);
class ValidationRuleBuilder {
 public:
  // Causes all constraints that match the FHIRPath expression specified by
  // "constraint" and located at "constraint_path", a FHIRPath expression to the
  // constrained node, to be considered valid regardless of the actual result
  // of evaluating the constraint on matching nodes.
  //
  // Example: IgnoringConstraint("Patient.contact", "name.exists()...")
  ValidationRuleBuilder& IgnoringConstraint(const std::string& constraint_path,
                                            const std::string& constraint);

  // Causes a set of {constraint_path, constraint} pairs to be considered valid
  // regardless of the actual result of evaluating the constraints on matching
  // nodes.
  //
  // See also: IgnoringConstraint(...)
  //
  // Example: IgorningConstraints({
  //   {"Bundle.entry", "fullUrl.contains('/_history/').not()"},
  //   {"Bundle", "type = 'document' implies (timestamp.hasValue())"}
  // })
  ValidationRuleBuilder& IgnoringConstraints(
      const absl::flat_hash_set<std::pair<std::string, std::string>>&
          constraints);

  // Returns the configured ValidationRule. If all preconditions are met (e.g.
  // not an ignored constraint), the rule returns true if the result of the
  // constraint's evaluation is true. False if the constraint was unmet or
  // failed to evaluate to a boolean. If the constraint matches an ignored
  // constraint, the rule returns true.
  ValidationRule StrictValidation() const {
    return CustomValidation(ValidationResults::StrictValidationFn);
  }

  // Returns the configured ValidationRule. If all preconditions are met (e.g.
  // not an ignored constraint), the rule returns the result of the constraint's
  // evaluation, if it evaluated to a boolean. Otherwise returns true. Common
  // causes of an expression failing to evaluate to a boolean could be:
  //   - the constraint uses a portion of FHIRPath not currently supported by
  //     this library
  //   - the constraint is not a valid FHIRPath expression
  //   - the constraint does not yield a boolean value
  // If the constraint matches an ignored constraint, the rule returns true.
  ValidationRule RelaxedValidation() const {
    return CustomValidation(ValidationResults::RelaxedValidationFn);
  }

  // Returns the configured ValidationRule. If all preconditions are met (e.g.
  // not an ignored constraint), the rule returns the result of calling the
  // provided validation function. If the constraint matches an ignored
  // constraint, the rule returns true.
  ValidationRule CustomValidation(ValidationRule validation_fn) const;

 private:
  absl::flat_hash_set<std::pair<std::string, std::string>,
                      absl::Hash<std::pair<std::string, std::string>>>
      ignored_constraints_;
};

}  // namespace google::fhir::fhir_path

#endif  // GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_VALIDATION_RULE_H_
