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

#ifndef GOOGLE_FHIR_DSTU2_JSON_FORMAT_H_
#define GOOGLE_FHIR_DSTU2_JSON_FORMAT_H_

#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/json_format.h"

namespace google {
namespace fhir {
namespace dstu2 {

// DSTU2-only API for cc/json_format.h
// See cc/json_format.h for documentation on these methods

absl::Status MergeJsonFhirStringIntoProto(
    absl::string_view raw_json, google::protobuf::Message* target,
    absl::TimeZone default_timezone, const bool validate,
    ErrorHandler& error_handler = FailFastErrorHandler::FailOnErrorOrFatal());

template <typename R>
absl::StatusOr<R> JsonFhirStringToProto(
    absl::string_view raw_json, const absl::TimeZone default_timezone,
    ErrorHandler& error_handler = FailFastErrorHandler::FailOnErrorOrFatal()) {
  R resource;
  FHIR_RETURN_IF_ERROR(MergeJsonFhirStringIntoProto(
      raw_json, &resource, default_timezone, true, error_handler));
  return resource;
}

template <typename R>
absl::StatusOr<R> JsonFhirStringToProtoWithoutValidating(
    absl::string_view raw_json, const absl::TimeZone default_timezone,
    ErrorHandler& error_handler = FailFastErrorHandler::FailOnErrorOrFatal()) {
  R resource;
  FHIR_RETURN_IF_ERROR(MergeJsonFhirStringIntoProto(
      raw_json, &resource, default_timezone, false, error_handler));
  return resource;
}

absl::StatusOr<std::string> PrintFhirPrimitive(
    const ::google::protobuf::Message& message);

absl::StatusOr<std::string> PrintFhirToJsonString(
    const google::protobuf::Message& fhir_proto);

absl::StatusOr<std::string> PrettyPrintFhirToJsonString(
    const google::protobuf::Message& fhir_proto);

absl::StatusOr<std::string> PrintFhirToJsonStringForAnalytics(
    const google::protobuf::Message& fhir_proto);

absl::StatusOr<std::string> PrettyPrintFhirToJsonStringForAnalytics(
    const google::protobuf::Message& fhir_proto);

}  // namespace dstu2
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_DSTU2_JSON_FORMAT_H_
