// Copyright 2018 Google LLC
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

#include "google/fhir/r4/profiles.h"

#include "google/protobuf/message.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/profiles_lib.h"
#include "google/fhir/r4/operation_error_reporter.h"
#include "google/fhir/r4/primitive_handler.h"
#include "google/fhir/status/status.h"

namespace google {
namespace fhir {

using ::google::fhir::r4::OperationOutcomeErrorHandler;
using ::google::fhir::r4::core::OperationOutcome;

namespace r4 {

absl::Status ConvertToProfile(const ::google::protobuf::Message& source,
                              ::google::protobuf::Message* target,
                              ::google::fhir::ErrorHandler& handler) {
  return ConvertToProfileInternal<r4::R4PrimitiveHandler>(source, target,
                                                          handler);
}

absl::StatusOr<OperationOutcome> ConvertToProfile(
    const ::google::protobuf::Message& source, ::google::protobuf::Message* target) {
  ::google::fhir::r4::core::OperationOutcome outcome;
  OperationOutcomeErrorHandler handler(&outcome);
  FHIR_RETURN_IF_ERROR(ConvertToProfile(source, target, handler));
  return outcome;
}
}  // namespace r4

absl::Status ConvertToProfileR4(const ::google::protobuf::Message& source,
                                ::google::protobuf::Message* target) {
  return ConvertToProfileInternal<r4::R4PrimitiveHandler>(
      source, target, FailFastErrorHandler::FailOnErrorOrFatal());
}

// Identical to ConvertToProfile, except does not run the validation step.
absl::Status ConvertToProfileLenientR4(const ::google::protobuf::Message& source,
                                       ::google::protobuf::Message* target) {
  return ConvertToProfileLenientInternal<r4::R4PrimitiveHandler>(
      source, target, google::fhir::FailFastErrorHandler::FailOnErrorOrFatal());
}

}  // namespace fhir
}  // namespace google
