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

#include "google/fhir/error_reporter.h"

#include <optional>
#include <string>
#include <string_view>

#include "glog/logging.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

namespace google::fhir {

const ScopedErrorReporter ScopedErrorReporter::WithScope(
    absl::string_view scope, std::optional<uint> index) const {
  return ScopedErrorReporter(&handler_, scope, index, this);
}

const ScopedErrorReporter ScopedErrorReporter::WithScope(
    const google::protobuf::FieldDescriptor* field,
    std::optional<std::uint8_t> index) const {
  return WithScope(field->json_name(),
                   field->is_repeated() ? index : std::nullopt);
}

std::string ScopedErrorReporter::CurrentFieldPath() const {
  if (prev_scope_ == nullptr) {
    return field_name_;
  }
  return absl::StrCat(prev_scope_->CurrentFieldPath(), ".", field_name_);
}

std::string ScopedErrorReporter::CurrentElementPath() const {
  std::string path_component =
      index_.has_value()
          ? absl::Substitute("$0[$1]", field_name_, index_.value())
          : field_name_;

  if (prev_scope_ == nullptr) {
    return path_component;
  }
  return absl::StrCat(prev_scope_->CurrentElementPath(), ".", path_component);
}

absl::Status ScopedErrorReporter::ReportFhirFatal(
    const absl::Status& status) const {
  return handler_.HandleFhirFatal(status, CurrentElementPath(),
                                  CurrentFieldPath());
}
absl::Status ScopedErrorReporter::ReportFhirFatal(
    const absl::Status& status, absl::string_view field_name,
    std::optional<uint> index) const {
  return WithScope(field_name, index).ReportFhirFatal(status);
}

absl::Status ScopedErrorReporter::ReportFhirError(absl::string_view msg) const {
  return handler_.HandleFhirError(msg, CurrentElementPath(),
                                  CurrentFieldPath());
}
absl::Status ScopedErrorReporter::ReportFhirError(
    absl::string_view msg, absl::string_view field_name,
    std::optional<uint> index) const {
  return WithScope(field_name, index).ReportFhirError(msg);
}

absl::Status ScopedErrorReporter::ReportFhirWarning(
    absl::string_view msg) const {
  return handler_.HandleFhirWarning(msg, CurrentElementPath(),
                                    CurrentFieldPath());
}
absl::Status ScopedErrorReporter::ReportFhirWarning(
    absl::string_view msg, absl::string_view field_name,
    std::optional<uint> index) const {
  return WithScope(field_name, index).ReportFhirWarning(msg);
}

absl::Status ScopedErrorReporter::ReportFhirPathFatal(
    const absl::Status& status, absl::string_view expression) const {
  return handler_.HandleFhirPathFatal(status, expression, CurrentElementPath(),
                                      CurrentFieldPath());
}
absl::Status ScopedErrorReporter::ReportFhirPathFatal(
    const absl::Status& status, absl::string_view expression,
    absl::string_view field_name, std::optional<uint> index) const {
  return WithScope(field_name, index).ReportFhirPathFatal(status, expression);
}

absl::Status ScopedErrorReporter::ReportFhirPathError(
    absl::string_view expression) const {
  return handler_.HandleFhirPathError(expression, CurrentElementPath(),
                                      CurrentFieldPath());
}
absl::Status ScopedErrorReporter::ReportFhirPathError(
    absl::string_view expression, absl::string_view field_name,
    std::optional<uint> index) const {
  return WithScope(field_name, index).ReportFhirPathError(expression);
}

absl::Status ScopedErrorReporter::ReportFhirPathWarning(
    absl::string_view expression) const {
  return handler_.HandleFhirPathWarning(expression, CurrentElementPath(),
                                        CurrentFieldPath());
}
absl::Status ScopedErrorReporter::ReportFhirPathWarning(
    absl::string_view expression, absl::string_view field_name,
    std::optional<uint> index) const {
  return WithScope(field_name, index).ReportFhirPathWarning(expression);
}

FailFastErrorHandler& FailFastErrorHandler::FailOnErrorOrFatal() {
  static FailFastErrorHandler* const kInstance =
      new FailFastErrorHandler(FailFastErrorHandler::FAIL_ON_ERROR_OR_FATAL);
  return *kInstance;
}

FailFastErrorHandler& FailFastErrorHandler::FailOnFatalOnly() {
  static FailFastErrorHandler* const kInstance =
      new FailFastErrorHandler(FailFastErrorHandler::FAIL_ON_FATAL_ONLY);
  return *kInstance;
}

std::optional<uint> IndexOrNullopt(const google::protobuf::FieldDescriptor* field,
                                   uint index) {
  return field->is_repeated() ? std::optional(index) : std::nullopt;
}

}  // namespace google::fhir
