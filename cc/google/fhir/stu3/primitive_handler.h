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

#ifndef GOOGLE_FHIR_STU3_PRIMITIVE_HANDLER_H_
#define GOOGLE_FHIR_STU3_PRIMITIVE_HANDLER_H_

#include <memory>

#include "google/protobuf/descriptor.h"
#include "absl/status/statusor.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/primitive_wrapper.h"
#include "google/fhir/status/statusor.h"
#include "proto/google/fhir/proto/stu3/datatypes.pb.h"
#include "proto/google/fhir/proto/stu3/resources.pb.h"

namespace google {
namespace fhir {
namespace stu3 {

class Stu3PrimitiveHandler
    : public ::google::fhir::primitives_internal::PrimitiveHandlerTemplate<
          proto::Bundle> {
 public:
  static const Stu3PrimitiveHandler* GetInstance();

 protected:
  ::absl::StatusOr<std::unique_ptr<primitives_internal::PrimitiveWrapper>>
  GetWrapper(const ::google::protobuf::Descriptor* target_descriptor) const override;
};

}  // namespace stu3
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_PRIMITIVE_HANDLER_H_
