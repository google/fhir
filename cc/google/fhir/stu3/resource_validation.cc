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

#include "google/fhir/stu3/resource_validation.h"

#include "google/fhir/error_reporter.h"
#include "google/fhir/fhir_path/stu3_fhir_path_validation.h"
#include "google/fhir/resource_validation.h"
#include "google/fhir/stu3/operation_error_reporter.h"
#include "google/fhir/stu3/primitive_handler.h"

namespace google {
namespace fhir {
namespace stu3 {

using ::google::fhir::stu3::proto::OperationOutcome;

absl::Status Validate(const ::google::protobuf::Message& resource,
                      ErrorHandler& error_handler) {
  return ::google::fhir::Validate(resource, Stu3PrimitiveHandler::GetInstance(),
                                  GetFhirPathValidator(), error_handler);
}

absl::StatusOr<OperationOutcome> Validate(const ::google::protobuf::Message& resource) {
  OperationOutcome outcome;
  OperationOutcomeErrorHandler error_reporter(&outcome);
  FHIR_RETURN_IF_ERROR(Validate(resource, error_reporter));
  return outcome;
}

absl::Status ValidateWithoutFhirPath(const ::google::protobuf::Message& resource) {
  return ValidateWithoutFhirPath(resource, Stu3PrimitiveHandler::GetInstance(),
                                 FailFastErrorHandler::FailOnErrorOrFatal());
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
