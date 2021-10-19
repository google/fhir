// Copyright 2021 Google LLC
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

#ifndef GOOGLE_FHIR_VALUE_SET_REPOSITORY_H_
#define GOOGLE_FHIR_VALUE_SET_REPOSITORY_H_

#include "absl/status/statusor.h"
#include "absl/types/optional.h"

namespace google {
namespace fhir {

class ValueSetRepository {
 public:
  virtual ~ValueSetRepository() = default;

  // Return whether the specified `code_string` belongs to the value set.
  // If the `value_set_id` is not found in the loader, returns a NotFoundError.
  // Optionally specify a system to make the requirement more strict.
  virtual absl::StatusOr<bool> BelongsToValueSet(
      const std::string& value_set_id, const std::string& code_string,
      const absl::optional<std::string>& code_system) const = 0;

  // Returns the number of distinct code systems the value set contains.
  // If the `value_set_id` is not found in the loader, returns a NotFoundError.
  virtual absl::StatusOr<int> NumberOfCodeSystemsInValueSet(
      const std::string& value_set_id) const = 0;
};

}  // namespace fhir
}  // namespace google


#endif  // GOOGLE_FHIR_VALUE_SET_REPOSITORY_H_
