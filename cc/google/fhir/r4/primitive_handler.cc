/*
 * Copyright 2020 Google LLC
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

#include "google/fhir/r4/primitive_handler.h"

#include <memory>
#include <unordered_map>
#include <utility>

#include "google/protobuf/descriptor.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codes.h"
#include "google/fhir/extensions.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/primitive_wrapper.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/r4/codeable_concepts.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/fhirproto_extensions.pb.h"

namespace google {
namespace fhir {
namespace r4 {

using ::absl::InvalidArgumentError;
using ::google::fhir::primitives_internal::PrimitiveWrapper;
using ::google::protobuf::Descriptor;

absl::StatusOr<std::unique_ptr<PrimitiveWrapper>>
R4PrimitiveHandler::GetWrapper(const Descriptor* target_descriptor) const {
  absl::optional<std::unique_ptr<PrimitiveWrapper>> wrapper =
      primitives_internal::GetWrapperForR4Types<
          core::Extension, core::Xhtml, fhirproto::Base64BinarySeparatorStride>(
          target_descriptor);

  if (wrapper.has_value()) {
    return std::move(wrapper.value());
  }

  return InvalidArgumentError(absl::StrCat(
      "Unexpected R4 primitive FHIR type: ", target_descriptor->full_name()));
}

absl::Status R4PrimitiveHandler::CopyCodeableConcept(
    const google::protobuf::Message& source, google::protobuf::Message* target) const {
  return r4::CopyCodeableConcept(source, target);
}

const R4PrimitiveHandler* R4PrimitiveHandler::GetInstance() {
  static R4PrimitiveHandler* instance = new R4PrimitiveHandler;
  return instance;
}

}  // namespace r4
}  // namespace fhir
}  // namespace google
