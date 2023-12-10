/*
 * Copyright 2023 Google LLC
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

#include "google/fhir/r5/primitive_handler.h"

#include <memory>
#include <optional>
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/primitive_wrapper.h"
#include "proto/google/fhir/proto/r5/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r5/fhirproto_extensions.pb.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace fhir {
namespace r5 {

using ::absl::InvalidArgumentError;
using ::google::fhir::primitives_internal::PrimitiveWrapper;
using ::google::protobuf::Descriptor;

absl::StatusOr<std::unique_ptr<PrimitiveWrapper>>
R5PrimitiveHandler::GetWrapper(const Descriptor* target_descriptor) const {
  std::optional<std::unique_ptr<PrimitiveWrapper>> wrapper =
      primitives_internal::GetWrapperForR5Types<
          core::Extension, core::Xhtml, fhirproto::Base64BinarySeparatorStride>(
          target_descriptor);

  if (wrapper.has_value()) {
    return std::move(wrapper.value());
  }

  return InvalidArgumentError(absl::StrCat(
      "Unexpected R5 primitive FHIR type: ", target_descriptor->full_name()));
}

absl::Status R5PrimitiveHandler::CopyCodeableConcept(
    const google::protobuf::Message& source, google::protobuf::Message* target) const {
  // Profiled CodeableConcepts don't exist in R5, so we can just do a simple
  // copy.
  if (source.GetDescriptor()->full_name() !=
      target->GetDescriptor()->full_name()) {
    return InvalidArgumentError(absl::Substitute(
        "Invalid arguments to CopyCodeableConcepts.  Source: $0, Target: $1",
        source.GetDescriptor()->full_name(),
        target->GetDescriptor()->full_name()));
  }
  target->CopyFrom(source);
  return absl::OkStatus();
}

const R5PrimitiveHandler* R5PrimitiveHandler::GetInstance() {
  static R5PrimitiveHandler* instance = new R5PrimitiveHandler;
  return instance;
}

}  // namespace r5
}  // namespace fhir
}  // namespace google
