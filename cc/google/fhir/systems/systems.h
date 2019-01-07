/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GOOGLE_FHIR_SYSTEMS_SYSTEMS_H_
#define GOOGLE_FHIR_SYSTEMS_SYSTEMS_H_

#include <string>
#include <vector>

namespace google {
namespace fhir {
namespace systems {

using std::string;

// These systems are standard values for valuesets described by the FHIR
// standard, either at the resource level or in the code system listing found
// at https://www.hl7.org/fhir/terminologies-systems.html.
const char kAdmitSource[] = "http://hl7.org/fhir/admit-source";
const char kDiagnosisRole[] = "http://hl7.org/fhir/diagnosis-role";
const char kDischargeDisposition[] =
    "http://hl7.org/fhir/discharge-disposition";
const char kEncounterType[] = "http://hl7.org/fhir/encounter-type";
const char kEncounterClass[] = "http://hl7.org/fhir/v3/ActCode";
const char kLocationType[] = "http://hl7.org/fhir/v3/RoleCode";
const char kIcd9[] = "http://hl7.org/fhir/sid/icd-9-cm";
const char kIcd9Diagnosis[] = "http://hl7.org/fhir/sid/icd-9-cm/diagnosis";
const char kIcd9Procedure[] = "http://hl7.org/fhir/sid/icd-9-cm/procedure";
const char kIcd10Diagnosis[] = "http://hl7.org/fhir/sid/icd-10";
const char kLoinc[] = "http://loinc.org";
const char kUnitsOfMeasure[] = "http://unitsofmeasure.org";
const char kCpt[] = "http://www.ama-assn.org/go/cpt";
const char kRxNorm[] = "http://www.nlm.nih.gov/research/umls/rxnorm";
const char kNdc[] = "http://hl7.org/fhir/sid/ndc";
const char kSnomed[] = "http://snomed.info/sct";
const char kObservationCategory[] = "http://hl7.org/fhir/observation-category";
const char kClaimCategory[] = "http://hl7.org/fhir/claiminformationcategory";
const char kMaritalStatus[] = "http://hl7.org/fhir/v3/MaritalStatus";
const char kNUBCDischarge[] = "http://www.nubc.org/patient-discharge";
const char kLanguage[] = "urn:ietf:bcp:47";

// Format ICD9 Diagnosis code according to
// http://www.icd9data.com/2015/Volume1/default.htm.
// NOTE: Current implementation is fairly naive without validation / padding.
string FormatIcd9Diagnosis(const string& icd9);

// Format ICD9 Procedure code according to
// http://www.icd9data.com/2012/Volume3/.
// NOTE: Current implementation is fairly naive without validation / padding.
string FormatIcd9Procedure(const string& icd9);

// Return a short form of the system name.
string ToShortSystemName(const string& system);

// We accept multiple different icd9/icd10/ccs subset coding schemes.
static const std::vector<string>* const kIcd9Schemes =
    new std::vector<string>({kIcd9, kIcd9Diagnosis, kIcd9Procedure});
static const std::vector<string>* const kIcd10Schemes =
    new std::vector<string>({kIcd10Diagnosis});

}  // namespace systems
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_SYSTEMS_SYSTEMS_H_
