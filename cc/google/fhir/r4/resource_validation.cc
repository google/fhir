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
#include "google/fhir/fhir_path/fhir_path_validation.h"
#include "google/fhir/r4/primitive_handler.h"
#include "google/fhir/resource_validation.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/fhirproto.pb.h"

namespace google {
namespace fhir {
namespace r4 {

absl::Status Validate(const ::google::protobuf::Message& resource,
                      ErrorHandler& error_handler,
                      const bool validate_reference_field_ids) {
  return ::google::fhir::Validate(resource, R4PrimitiveHandler::GetInstance(),
                                  &fhir_path::FhirPathValidator::GetInstance(),
                                  error_handler, validate_reference_field_ids);
}

absl::Status ValidateWithoutFhirPath(const ::google::protobuf::Message& resource) {
  auto reporter = google::fhir::FailFastErrorHandler::FailOnErrorOrFatal();
  return ValidateWithoutFhirPath(resource, R4PrimitiveHandler::GetInstance(),
                                 reporter);
}

}  // namespace r4
}  // namespace fhir
}  // namespace google
