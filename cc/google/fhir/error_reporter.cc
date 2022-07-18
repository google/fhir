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

void ErrorReporter::EnterScope(absl::string_view scope,
                               std::optional<uint> index) {
  scope_stack_.push_back({std::string(scope), index});
}

void ErrorReporter::ExitScope() {
  if (scope_stack_.empty()) {
    // Should be impossible since scopes are only grabbed by ErrorScope objects
    LOG(WARNING) << "ExitScope called when scope stack is empty";
    return;
  }
  scope_stack_.pop_back();
}

std::string ErrorReporter::CurrentElementPath() {
  if (scope_stack_.empty()) {
    return "";
  }
  std::string path = "";
  for (const auto& token : scope_stack_) {
    path.append(token.first);
    if (token.second.has_value()) {
      absl::StrAppend(&path, "[", std::to_string(token.second.value()), "]");
    }
    path += '.';
  }
  path.pop_back();  // remove trailing dot
  return path;
}

std::string ErrorReporter::CurrentFieldPath() {
  if (scope_stack_.empty()) {
    return "";
  }
  std::string path = "";
  for (const auto& token : scope_stack_) {
    path.append(token.first);
    path += ".";
  }
  path.pop_back();  // remove trailing dot
  return path;
}

absl::Status ErrorReporter::ReportFhirFatal(const absl::Status& status,
                                            absl::string_view field_name,
                                            std::optional<uint> index) {
  if (!field_name.empty()) {
    ErrorScope scope(this, field_name, index);
    return handler_.HandleFhirFatal(status, CurrentElementPath(),
                                    CurrentFieldPath());
  }
  return handler_.HandleFhirFatal(status, CurrentElementPath(),
                                  CurrentFieldPath());
}
absl::Status ErrorReporter::ReportFhirError(std::string_view msg,
                                            absl::string_view field_name,
                                            std::optional<uint> index) {
  if (!field_name.empty()) {
    ErrorScope scope(this, field_name, index);
    return handler_.HandleFhirError(msg, CurrentElementPath(),
                                    CurrentFieldPath());
  }
  return handler_.HandleFhirError(msg, CurrentElementPath(),
                                  CurrentFieldPath());
}
absl::Status ErrorReporter::ReportFhirWarning(std::string_view msg,
                                              absl::string_view field_name,
                                              std::optional<uint> index) {
  if (!field_name.empty()) {
    ErrorScope scope(this, field_name, index);
    return handler_.HandleFhirWarning(msg, CurrentElementPath(),
                                      CurrentFieldPath());
  }
  return handler_.HandleFhirWarning(msg, CurrentElementPath(),
                                    CurrentFieldPath());
}

absl::Status ErrorReporter::ReportFhirPathFatal(const absl::Status& status,
                                                std::string_view expression,
                                                absl::string_view field_name,
                                                std::optional<uint> index) {
  if (!field_name.empty()) {
    ErrorScope scope(this, field_name, index);
    return handler_.HandleFhirPathFatal(
        status, expression, CurrentElementPath(), CurrentFieldPath());
  }
  return handler_.HandleFhirPathFatal(status, expression, CurrentElementPath(),
                                      CurrentFieldPath());
}
absl::Status ErrorReporter::ReportFhirPathError(std::string_view expression,
                                                absl::string_view field_name,
                                                std::optional<uint> index) {
  if (!field_name.empty()) {
    ErrorScope scope(this, field_name, index);
    return handler_.HandleFhirPathError(expression, CurrentElementPath(),
                                        CurrentFieldPath());
  }
  return handler_.HandleFhirPathError(expression, CurrentElementPath(),
                                      CurrentFieldPath());
}
absl::Status ErrorReporter::ReportFhirPathWarning(std::string_view expression,
                                                  absl::string_view field_name,
                                                  std::optional<uint> index) {
  if (!field_name.empty()) {
    ErrorScope scope(this, field_name, index);
    return handler_.HandleFhirPathWarning(expression, CurrentElementPath(),
                                          CurrentFieldPath());
  }
  return handler_.HandleFhirPathWarning(expression, CurrentElementPath(),
                                        CurrentFieldPath());
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
