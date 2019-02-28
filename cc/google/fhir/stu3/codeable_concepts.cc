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

#include "google/fhir/stu3/codeable_concepts.h"


#include "google/protobuf/message.h"
#include "absl/memory/memory.h"
#include "google/fhir/status/statusor.h"

namespace google {
namespace fhir {
namespace stu3 {

namespace internal {

std::unique_ptr<proto::Coding> CodingFromFixedSystemCoding(
    const google::protobuf::FieldDescriptor* field,
    const proto::CodingWithFixedSystem fixed_system_coding) {
  std::unique_ptr<proto::Coding> coding = absl::make_unique<proto::Coding>();
  CopyCommonCodingFields(fixed_system_coding, coding.get());
  coding->mutable_system()->set_value(GetInlinedCodingSystem(field));
  *coding->mutable_code() = fixed_system_coding.code();
  return coding;
}

std::unique_ptr<proto::Coding> CodingFromFixedCodeCoding(
    const google::protobuf::FieldDescriptor* field,
    const proto::CodingWithFixedCode fixed_code_coding) {
  std::unique_ptr<proto::Coding> coding = absl::make_unique<proto::Coding>();
  CopyCommonCodingFields(fixed_code_coding, coding.get());
  coding->mutable_system()->set_value(GetInlinedCodingSystem(field));
  coding->mutable_code()->set_value(GetInlinedCodingCode(field));
  return coding;
}

}  // namespace internal

}  // namespace stu3
}  // namespace fhir
}  // namespace google
