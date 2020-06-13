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

#include "google/fhir/fhir_path/fhir_path_validation_rule.h"

namespace google::fhir::fhir_path {

ValidationRuleBuilder& ValidationRuleBuilder::IgnoringConstraint(
    const std::string& constraint_path, const std::string& constraint) {
  ignored_constraints_.emplace(
      std::pair<std::string, std::string>(constraint_path, constraint));
  return *this;
}

ValidationRuleBuilder& ValidationRuleBuilder::IgnoringConstraints(
    const absl::flat_hash_set<std::pair<std::string, std::string>>&
        constraints) {
  ignored_constraints_.insert(constraints.begin(), constraints.end());
  return *this;
}

ValidationRule ValidationRuleBuilder::CustomValidation(
    ValidationRule validation_fn) const {
  return [validation_fn, ignored_constraints = this->ignored_constraints_](
             const ValidationResult& result) {
    return validation_fn(result) ||
           ignored_constraints.contains(
               {result.ConstraintPath(), result.Constraint()});
  };
}

}  // namespace google::fhir::fhir_path
