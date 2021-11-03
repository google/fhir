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

#ifndef GOOGLE_FHIR_TERMINOLOGY_TERMINOLOGY_RESOLVER_H_
#define GOOGLE_FHIR_TERMINOLOGY_TERMINOLOGY_RESOLVER_H_

#include "absl/container/flat_hash_set.h"
#include "absl/status/statusor.h"

namespace google::fhir::terminology {

// Interface for object capable of resolving FHIR CodeSystems and ValueSets by
// URL.
//
// Provides functions to retrieve enumerations of codes, as well as functions
// to check if a given code is present in CodeSystems or ValueSets.
class TerminologyResolver {
 public:
  virtual ~TerminologyResolver() = default;

  // Gets an enumeration of all codes in a CodeSystem, by URL.
  //
  // Returns status NotFound if the resolver is unable to resolve a CodeSystem
  // at that URL.
  virtual absl::StatusOr<absl::flat_hash_set<std::string>>
  GetCodeStringsInCodeSystem(absl::string_view code_system_url) const = 0;

  // Given a CodeSystem URL and a code value, returns whether or not that value
  // is valid for the given CodeSystem.
  //
  // Returns status NotFound if the resolver is unable to resolve a CodeSystem
  // at that URL.
  virtual absl::StatusOr<bool> IsCodeInCodeSystem(
      absl::string_view code_value,
      absl::string_view code_system_url) const = 0;

  // Gets an enumeration of all codes in a ValueSet, by URL.
  //
  // Returns status NotFound if the resolver is unable to resolve a ValueSet
  // at that URL.
  virtual absl::StatusOr<absl::flat_hash_set<std::string>>
  GetCodeStringsInValueSet(absl::string_view value_set_url) const = 0;

  // Returns true if the ValueSet has more than one CodeSystem.
  //
  // Returns status NotFound if the resolver is unable to resolve a ValueSet
  // at that URL.
  virtual absl::StatusOr<bool> ValueSetHasMultipleCodeSystems(
      absl::string_view value_set_url) const = 0;

  // Given a ValueSet URL and a code value, returns whether or not that value
  // is valid for the given ValueSet.
  //
  // Returns status NotFound if the resolver is unable to resolve a ValueSet
  // at that URL.
  virtual absl::StatusOr<bool> IsCodeInValueSet(
      absl::string_view code_value, absl::string_view value_set_url) const = 0;

  // Given a ValueSet URL, a code value, and a CodeSystem URL, returns whether
  // or not there is a code in the ValueSet that matches that value and
  // CodeSystem.
  //
  // Returns status NotFound if the resolver is unable to resolve a ValueSet
  // at that URL.
  virtual absl::StatusOr<bool> IsCodeInValueSet(
      absl::string_view code_value, absl::string_view code_system_url,
      absl::string_view value_set_url) const = 0;
};

}  // namespace google::fhir::terminology

#endif  // GOOGLE_FHIR_TERMINOLOGY_TERMINOLOGY_RESOLVER_H_
