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

#ifndef GOOGLE_FHIR_CODEABLE_CONCEPTS_H_
#define GOOGLE_FHIR_CODEABLE_CONCEPTS_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/fhir/annotations.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/r4/codeable_concepts.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/stu3/codeable_concepts.h"
#include "proto/annotations.pb.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {

typedef std::function<void(const std::string& system, const std::string& code)>
    CodeFunc;
typedef std::function<bool(const std::string& system, const std::string& code)>
    CodeBoolFunc;

// Performs a function on all System/Code pairs, where all codes are treated
// like strings, until the function returns true.
// If the function returns true on a Coding, ceases visiting Codings and returns
// true.  If the function returned false for all Codings, returns false.
const bool FindSystemCodeStringPair(const ::google::protobuf::Message& concept,
                                    const CodeBoolFunc& func,
                                    std::string* found_system,
                                    std::string* found_code);

// Variant of FindSystemCodeStringPair that does not explicitly set return
// pointers.
// This can be used when the actual result is unused, or when the coding is used
// to set a return pointers in the function closure itself.
const bool FindSystemCodeStringPair(const ::google::protobuf::Message& concept,
                                    const CodeBoolFunc& func);

// Performs a function on all System/Code pairs, where all codes are treated
// like strings.  Visits all codings.
void ForEachSystemCodeStringPair(const ::google::protobuf::Message& concept,
                                 const CodeFunc& func);

// Gets a vector of all codes with a given system, where codes are represented
// as strings.
const std::vector<std::string> GetCodesWithSystem(
    const ::google::protobuf::Message& concept, const absl::string_view target_system);

// Gets the only code with a given system.
// If no code is found with that system, returns a NotFound status.
// If more than one code is found with that system, returns AlreadyExists
// status.
StatusOr<const std::string> GetOnlyCodeWithSystem(
    const ::google::protobuf::Message& concept, const absl::string_view system);

// Extract first code value with a given system.
// Returns NOT_FOUND if none exists.
// This is different from GetOnlyCodeWithSystem in that it doesn't fail if
// there is more than one code with the given system.
template <typename CodeableConceptLike>
StatusOr<std::string> ExtractCodeBySystem(
    const CodeableConceptLike& codeable_concept,
    absl::string_view system_value) {
  switch (google::fhir::GetFhirVersion(codeable_concept)) {
    case google::fhir::proto::STU3:
      return stu3::ExtractCodeBySystem(codeable_concept, system_value);
    case google::fhir::proto::R4: {
      return r4::ExtractCodeBySystem(codeable_concept, system_value);
    }
    default:
      return ::tensorflow::errors::InvalidArgument(
          absl::StrCat("FHIR version not supported by codeable_concepts.h: ",
                       google::fhir::proto::FhirVersion_Name(
                           google::fhir::GetFhirVersion(codeable_concept))));
  }
}

Status AddCoding(::google::protobuf::Message* concept, const std::string& system,
                 const std::string& code);

template <typename CodeableConceptLike>
Status ClearAllCodingsWithSystem(CodeableConceptLike* concept,
                                 const std::string& system) {
  switch (google::fhir::GetFhirVersion(*concept)) {
    case google::fhir::proto::STU3:
      return stu3::ClearAllCodingsWithSystem(concept, system);
    case google::fhir::proto::R4: {
      return r4::ClearAllCodingsWithSystem(concept, system);
    }
    default:
      return ::tensorflow::errors::InvalidArgument(
          absl::StrCat("FHIR version not supported by codeable_concepts.h: ",
                       google::fhir::proto::FhirVersion_Name(
                           google::fhir::GetFhirVersion(*concept))));
  }
}

// Copies any codeable concept-like message into any other codeable concept-like
// message.  An ok status guarantees that all codings present on the source will
// be present on the target, in the correct profiled fields on the target.
Status CopyCodeableConcept(const ::google::protobuf::Message& source,
                           ::google::protobuf::Message* target);

bool IsCodeableConceptLike(const ::google::protobuf::Descriptor* descriptor);

bool IsCodeableConceptLike(const ::google::protobuf::Message& message);

int CodingSize(const ::google::protobuf::Message& concept);

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_CODEABLE_CONCEPTS_H_
