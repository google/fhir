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

#include "google/fhir/r4/json_format.h"

#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/json_format.h"
#include "google/fhir/json_format_results.h"
#include "google/fhir/r4/primitive_handler.h"

namespace google {
namespace fhir {
namespace r4 {

namespace {

const Parser* GetParser() {
  static Parser* parser = new Parser(R4PrimitiveHandler::GetInstance());
  return parser;
}

const Printer* GetPrinter() {
  static Printer* printer = new Printer(R4PrimitiveHandler::GetInstance());
  return printer;
}

}  // namespace

absl::Status MergeJsonFhirStringIntoProto(const absl::string_view raw_json,
                                          google::protobuf::Message* target,
                                          absl::TimeZone default_timezone,
                                          const bool validate,
                                          ErrorHandler& error_handler) {
  FHIR_ASSIGN_OR_RETURN(
      ParseResult result,
      GetParser()->MergeJsonFhirStringIntoProto(
          raw_json, target, default_timezone, validate, error_handler));

  if (result == ParseResult::kFailed) {
    return absl::InvalidArgumentError(
        "Merge failure when parsing JSON.  See ErrorHandler for more info.");
  }
  return absl::OkStatus();
}

absl::Status MergeJsonFhirObjectIntoProto(
    const google::fhir::internal::FhirJson& json_object,
    google::protobuf::Message* target, absl::TimeZone default_timezone,
    const bool validate, ErrorHandler& error_handler) {
  FHIR_ASSIGN_OR_RETURN(
      ParseResult result,
      GetParser()->MergeJsonFhirObjectIntoProto(
          json_object, target, default_timezone, validate, error_handler));

  if (result == ParseResult::kFailed) {
    return absl::InvalidArgumentError(
        "Merge failure when parsing JSON.  See ErrorHandler for more info.");
  }
  return absl::OkStatus();
}

absl::StatusOr<std::string> PrintFhirPrimitive(
    const ::google::protobuf::Message& message) {
  return GetPrinter()->PrintFhirPrimitive(message);
}

absl::StatusOr<std::string> PrintFhirToJsonString(
    const google::protobuf::Message& fhir_proto) {
  return GetPrinter()->PrintFhirToJsonString(fhir_proto);
}

absl::StatusOr<std::string> PrettyPrintFhirToJsonString(
    const google::protobuf::Message& fhir_proto) {
  return GetPrinter()->PrettyPrintFhirToJsonString(fhir_proto);
}

absl::StatusOr<std::string> PrintFhirToJsonStringForAnalytics(
    const google::protobuf::Message& fhir_proto) {
  return GetPrinter()->PrintFhirToJsonStringForAnalytics(fhir_proto);
}

absl::StatusOr<std::string> PrettyPrintFhirToJsonStringForAnalytics(
    const google::protobuf::Message& fhir_proto) {
  return GetPrinter()->PrettyPrintFhirToJsonStringForAnalytics(fhir_proto);
}

}  // namespace r4
}  // namespace fhir
}  // namespace google
