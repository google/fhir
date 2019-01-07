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

#include "google/fhir/systems/systems.h"

#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"

namespace google {
namespace fhir {
namespace systems {

using std::string;

string FormatIcd9Diagnosis(const string& icd9) {
  if (icd9.length() <= 3 || (icd9[0] == 'E' && icd9.length() <= 4) ||
      icd9.find('.') != std::string::npos) {
    return icd9;
  }
  if (icd9[0] == 'E') {
    return absl::StrCat(icd9.substr(0, 4), ".", icd9.substr(4, icd9.length()));
  }
  return absl::StrCat(icd9.substr(0, 3), ".", icd9.substr(3, icd9.length()));
}

string FormatIcd9Procedure(const string& icd9) {
  if (icd9.length() <= 2 || icd9.find('.') != std::string::npos) {
    return icd9;
  }
  return absl::StrCat(icd9.substr(0, 2), ".", icd9.substr(2, icd9.length()));
}

string ToShortSystemName(const string& system) {
  if (system == kLoinc) {
    return "loinc";
  }
  for (const auto& icd9_system : *kIcd9Schemes) {
    if (system == icd9_system) {
      return "icd9";
    }
  }
  for (const auto& icd10_system : *kIcd10Schemes) {
    if (system == icd10_system) {
      return "icd10";
    }
  }
  if (system == kCpt) {
    return "cpt";
  }
  if (system == kNdc) {
    return "ndc";
  }
  if (system == kSnomed) {
    return "snomed";
  }
  if (system == kObservationCategory) {
    return "observation_category";
  }
  if (system == kClaimCategory) {
    return "claim_category";
  }
  if (system == kMaritalStatus) {
    return "marital_status";
  }
  if (system == kNUBCDischarge) {
    return "nubc_discharge";
  }
  if (system == kLanguage) {
    return "language";
  }
  if (system == kRxNorm) {
    return "rxnorm";
  }
  if (system == kAdmitSource) {
    return "admit_source";
  }
  if (system == kEncounterClass) {
    return "actcode";
  }
  if (system == kDischargeDisposition) {
    return "discharge_disposition";
  }
  if (system == kEncounterType) {
    return "encounter_type";
  }
  if (system == kDiagnosisRole) {
    return "diagnosis_role";
  }
  return absl::StrReplaceAll(
      system, {{"://", "-"}, {":", "-"}, {"/", "-"}, {".", "-"}});
}

}  // namespace systems
}  // namespace fhir
}  // namespace google
