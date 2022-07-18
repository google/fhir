/*
 * Copyright 2022 Google LLC
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

#include <string>
#include <string_view>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "google/fhir/status/status.h"
#include "proto/google/fhir/proto/r4/core/resources/patient.pb.h"

namespace google::fhir {

namespace {

class TestErrorHandler : public ErrorHandler {
 public:
  absl::Status HandleFhirFatal(const absl::Status& status,
                               std::string_view element_path,
                               std::string_view field_path) override {
    Handle(status.message(), "fatal", element_path, field_path);
    return absl::OkStatus();
  }

  absl::Status HandleFhirError(std::string_view msg,
                               std::string_view element_path,
                               std::string_view field_path) override {
    Handle(msg, "error", element_path, field_path);
    return absl::OkStatus();
  }

  absl::Status HandleFhirWarning(std::string_view msg,
                                 std::string_view element_path,
                                 std::string_view field_path) override {
    Handle(msg, "warning", element_path, field_path);
    return absl::OkStatus();
  }

  absl::Status HandleFhirPathFatal(const absl::Status& status,
                                   std::string_view expression,
                                   std::string_view element_path,
                                   std::string_view field_path) override {
    HandleFhirPath(status.message(), expression, "fatal", element_path,
                   field_path);
    return absl::OkStatus();
  }

  absl::Status HandleFhirPathError(std::string_view expression,
                                   std::string_view element_path,
                                   std::string_view field_path) override {
    HandleFhirPath("", expression, "error", element_path, field_path);
    return absl::OkStatus();
  }

  absl::Status HandleFhirPathWarning(std::string_view expression,
                                     std::string_view element_path,
                                     std::string_view field_path) override {
    HandleFhirPath("", expression, "warning", element_path, field_path);
    return absl::OkStatus();
  }

  bool HasWarnings() const override {
    // Not implemented for test reporter
    return false;
  }
  bool HasErrors() const override {
    // Not implemented for test reporter
    return false;
  }
  bool HasFatals() const override {
    // Not implemented for test reporter
    return false;
  }

  std::vector<std::string> reports_;

 private:
  void Handle(absl::string_view message, absl::string_view severity,
              std::string_view element_path, std::string_view field_path) {
    reports_.push_back(absl::StrCat(message, ":", severity, ":", element_path,
                                    ":", field_path));
  }
  void HandleFhirPath(absl::string_view message, absl::string_view expression,
                      absl::string_view severity, std::string_view element_path,
                      std::string_view field_path) {
    reports_.push_back(absl::StrCat(message, "@", expression, ":", severity,
                                    ":", element_path, ":", field_path));
  }
};

TEST(ErrorReporterTest, CorrectHandlerCalled) {
  TestErrorHandler handler;
  ErrorReporter reporter(&handler);

  ErrorScope foo_scope(&reporter, "Foo");
  FHIR_ASSERT_OK(reporter.ReportFhirFatal(absl::InternalError("msg-1")));
  FHIR_ASSERT_OK(reporter.ReportFhirError("msg-2"));
  FHIR_ASSERT_OK(reporter.ReportFhirWarning("msg-3"));

  FHIR_ASSERT_OK(reporter.ReportFhirPathFatal(absl::InternalError("msg-4"),
                                              "bar.exists()"));
  FHIR_ASSERT_OK(reporter.ReportFhirPathError("bar.exists()"));
  FHIR_ASSERT_OK(reporter.ReportFhirPathWarning("bar.exists()"));

  EXPECT_THAT(handler.reports_,
              testing::ElementsAre("msg-1:fatal:Foo:Foo", "msg-2:error:Foo:Foo",
                                   "msg-3:warning:Foo:Foo",
                                   "msg-4@bar.exists():fatal:Foo:Foo",
                                   "@bar.exists():error:Foo:Foo",
                                   "@bar.exists():warning:Foo:Foo"));
}

TEST(ErrorReporterTest, ScopesAppliedCorretly) {
  TestErrorHandler handler;
  ErrorReporter reporter(&handler);

  {
    ErrorScope foo_scope(&reporter, "Foo");
    FHIR_ASSERT_OK(reporter.ReportFhirFatal(absl::InternalError("msg-1")));

    {
      ErrorScope bar_scope(&reporter, "bar", 2);
      {
        ErrorScope baz_scope(&reporter, "baz");
        FHIR_ASSERT_OK(reporter.ReportFhirError("msg-2"));
      }
    }
    {
      ErrorScope bar_scope(&reporter, "bar", 5);
      FHIR_ASSERT_OK(reporter.ReportFhirWarning("msg-3"));
    }
  }

  {
    ErrorScope foo_scope(&reporter, "Quux");
    FHIR_ASSERT_OK(reporter.ReportFhirWarning("msg-4"));
  }

  EXPECT_THAT(handler.reports_,
              testing::ElementsAre("msg-1:fatal:Foo:Foo",
                                   "msg-2:error:Foo.bar[2].baz:Foo.bar.baz",
                                   "msg-3:warning:Foo.bar[5]:Foo.bar",
                                   "msg-4:warning:Quux:Quux"));
}

TEST(ErrorReporterTest, ReportWithScopeApisApplyScopeCorrectly) {
  TestErrorHandler handler;
  ErrorReporter reporter(&handler);

  ErrorScope foo_scope(&reporter, "Foo");

  FHIR_ASSERT_OK(reporter.ReportFhirFatal(absl::InternalError("msg-1"), "s1"));
  FHIR_ASSERT_OK(reporter.ReportFhirError("msg-2", "s2", 3));
  FHIR_ASSERT_OK(reporter.ReportFhirWarning("msg-3", "s2", 4));

  FHIR_ASSERT_OK(reporter.ReportFhirPathFatal(absl::InternalError("msg-4"),
                                              "bar.exists()", "s3", 0));
  FHIR_ASSERT_OK(reporter.ReportFhirPathError("bar.exists()", "s4"));
  FHIR_ASSERT_OK(reporter.ReportFhirPathWarning("bar.exists()", "s5"));

  EXPECT_THAT(handler.reports_,
              testing::ElementsAre("msg-1:fatal:Foo.s1:Foo.s1",
                                   "msg-2:error:Foo.s2[3]:Foo.s2",
                                   "msg-3:warning:Foo.s2[4]:Foo.s2",
                                   "msg-4@bar.exists():fatal:Foo.s3[0]:Foo.s3",
                                   "@bar.exists():error:Foo.s4:Foo.s4",
                                   "@bar.exists():warning:Foo.s5:Foo.s5"));
}

TEST(ErrorReporterTest, ScopesCreatedThroughFieldsSucceed) {
  TestErrorHandler handler;
  ErrorReporter reporter(&handler);

  auto patient_descriptor = r4::core::Patient::descriptor();

  {
    ErrorScope foo_scope(&reporter, "Patient");
    {
      auto contact_field = patient_descriptor->FindFieldByName("contact");
      // Contact is repeated - 2 is respected
      ErrorScope contact_scope(&reporter, contact_field, 2);
      {
        auto name_field =
            r4::core::Patient::Contact::descriptor()->FindFieldByName("name");
        // Name is not repeated on contact - index should be ignored
        ErrorScope name_scope(&reporter, name_field, 5);
        FHIR_ASSERT_OK(reporter.ReportFhirError("msg"));
      }
    }
  }

  EXPECT_THAT(handler.reports_,
              testing::ElementsAre(
                  "msg:error:Patient.contact[2].name:Patient.contact.name"));
}

}  // namespace

}  // namespace google::fhir
