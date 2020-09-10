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
#include "google/fhir/stu3/primitive_handler.h"

#include <memory>
#include <unordered_map>
#include <utility>

#include "google/protobuf/descriptor.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codes.h"
#include "google/fhir/extensions.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/primitive_wrapper.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/fhirproto_extensions.pb.h"
#include "include/json/json.h"

namespace google {
namespace fhir {
namespace stu3 {

using ::absl::InvalidArgumentError;
using primitives_internal::PrimitiveWrapper;
using ::google::protobuf::Descriptor;

absl::StatusOr<std::unique_ptr<PrimitiveWrapper>>
Stu3PrimitiveHandler::GetWrapper(const Descriptor* target_descriptor) const {
  absl::optional<std::unique_ptr<PrimitiveWrapper>> wrapper =
      primitives_internal::GetWrapperForStu3Types<
          proto::Extension, proto::Xhtml,
          fhirproto::Base64BinarySeparatorStride>(target_descriptor);

  if (wrapper.has_value()) {
    return std::move(wrapper.value());
  }
  return InvalidArgumentError(absl::StrCat(
      "Unexpected STU3 primitive FHIR type: ", target_descriptor->full_name()));
}

const Stu3PrimitiveHandler* Stu3PrimitiveHandler::GetInstance() {
  static Stu3PrimitiveHandler* instance = new Stu3PrimitiveHandler;
  return instance;
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
