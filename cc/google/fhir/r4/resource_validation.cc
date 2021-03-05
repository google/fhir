/*
 * Copyright 2021 Google LLC
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

#include "google/fhir/r4/resource_validation.h"

#include "absl/status/status.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/fhir_path/r4_fhir_path_validation.h"
#include "google/fhir/r4/operation_error_reporter.h"
#include "google/fhir/r4/primitive_handler.h"
#include "google/fhir/resource_validation.h"

namespace google {
namespace fhir {
namespace r4 {

using ::google::fhir::r4::core::OperationOutcome;

absl::Status Validate(const ::google::protobuf::Message& resource,
                      ErrorReporter* error_reporter) {
  return ::google::fhir::Validate(resource, R4PrimitiveHandler::GetInstance(),
                                  GetFhirPathValidator(), error_reporter);
}

absl::StatusOr<OperationOutcome> Validate(const ::google::protobuf::Message& resource) {
  OperationOutcome outcome;
  OperationOutcomeErrorReporter error_reporter(&outcome);
  FHIR_RETURN_IF_ERROR(Validate(resource, &error_reporter));
  return outcome;
}

absl::Status ValidateResource(const ::google::protobuf::Message& resource) {
  return ValidateResource(resource, R4PrimitiveHandler::GetInstance());
}

absl::Status ValidateResourceWithFhirPath(const ::google::protobuf::Message& resource) {
  return ValidateResourceWithFhirPath(
      resource, R4PrimitiveHandler::GetInstance(), GetFhirPathValidator());
}

}  // namespace r4
}  // namespace fhir
}  // namespace google
