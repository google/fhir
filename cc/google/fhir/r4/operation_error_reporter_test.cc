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

#include "google/fhir/r4/operation_error_reporter.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "google/fhir/status/status.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/r4/core/resources/operation_outcome.pb.h"

namespace google::fhir::r4 {

namespace {

using ::google::fhir::r4::core::OperationOutcome;
using Issue = ::google::fhir::r4::core::OperationOutcome::Issue;
using ::google::fhir::r4::core::IssueSeverityCode;
using ::google::fhir::r4::core::IssueTypeCode;

using ::google::fhir::testutil::EqualsProto;
using ::testing::ElementsAre;

Issue MakeWarning(const std::string& path, const std::string& diagnostics) {
  Issue issue;
  issue.mutable_code()->set_value(IssueTypeCode::VALUE);
  issue.mutable_severity()->set_value(IssueSeverityCode::WARNING);
  issue.add_expression()->set_value(path);
  issue.mutable_diagnostics()->set_value(diagnostics);

  return issue;
}

Issue MakeError(const std::string& path, const std::string& diagnostics) {
  Issue issue;
  issue.mutable_code()->set_value(IssueTypeCode::VALUE);
  issue.mutable_severity()->set_value(IssueSeverityCode::ERROR);
  issue.add_expression()->set_value(path);
  issue.mutable_diagnostics()->set_value(diagnostics);

  return issue;
}

Issue MakeFatal(const std::string& path, const std::string& diagnostics) {
  Issue issue;
  issue.mutable_code()->set_value(IssueTypeCode::STRUCTURE);
  issue.mutable_severity()->set_value(IssueSeverityCode::FATAL);
  issue.add_expression()->set_value(path);
  issue.mutable_diagnostics()->set_value(diagnostics);

  return issue;
}

TEST(OperationOutcomeErrorReporterTest, GetFhirWarningsSucceeds) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeFatal("p1", "m1");
  *outcome.add_issue() = MakeWarning("p2", "m2");
  *outcome.add_issue() = MakeError("p3", "m3");
  *outcome.add_issue() = MakeFatal("p4", "m4");
  *outcome.add_issue() = MakeWarning("p5", "m5");
  *outcome.add_issue() = MakeError("p6", "m6");

  EXPECT_THAT(OperationOutcomeErrorReporter::GetFhirWarnings(outcome),
              ElementsAre(EqualsProto(MakeWarning("p2", "m2")),
                          EqualsProto(MakeWarning("p5", "m5"))));
}

TEST(OperationOutcomeErrorReporterTest, GetFhirErrorsSucceeds) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeFatal("p1", "m1");
  *outcome.add_issue() = MakeWarning("p2", "m2");
  *outcome.add_issue() = MakeError("p3", "m3");
  *outcome.add_issue() = MakeFatal("p4", "m4");
  *outcome.add_issue() = MakeWarning("p5", "m5");
  *outcome.add_issue() = MakeError("p6", "m6");

  EXPECT_THAT(OperationOutcomeErrorReporter::GetFhirErrors(outcome),
              ElementsAre(EqualsProto(MakeError("p3", "m3")),
                          EqualsProto(MakeError("p6", "m6"))));
}

TEST(OperationOutcomeErrorReporterTest, GetFhirFatalsSucceeds) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeFatal("p1", "m1");
  *outcome.add_issue() = MakeWarning("p2", "m2");
  *outcome.add_issue() = MakeError("p3", "m3");
  *outcome.add_issue() = MakeFatal("p4", "m4");
  *outcome.add_issue() = MakeWarning("p5", "m5");
  *outcome.add_issue() = MakeError("p6", "m6");

  EXPECT_THAT(OperationOutcomeErrorReporter::GetFhirFatals(outcome),
              ElementsAre(EqualsProto(MakeFatal("p1", "m1")),
                          EqualsProto(MakeFatal("p4", "m4"))));
}

TEST(OperationOutcomeErrorReporterTest, ReportWarningSucceeds) {
  OperationOutcome outcome;
  OperationOutcomeErrorReporter reporter(&outcome);

  FHIR_ASSERT_OK(reporter.ReportFhirWarning("Foo.bar.baz", "Foo.bar[2].baz[3]",
                                            "warning_msg"));
  FHIR_ASSERT_OK(
      reporter.ReportFhirWarning("Foo.b.c", "no element path warning_msg"));
  FHIR_ASSERT_OK(
      reporter.ReportFhirPathWarning("Foo.f", "Foo.f[3]", "p.exists()"));

  std::vector<OperationOutcome::Issue> warnings =
      OperationOutcomeErrorReporter::GetFhirWarnings(outcome);

  EXPECT_THAT(
      warnings,
      ElementsAre(
          EqualsProto(MakeWarning("Foo.bar[2].baz[3]", "warning_msg")),
          EqualsProto(MakeWarning("Foo.b.c", "no element path warning_msg")),
          EqualsProto(MakeWarning("Foo.f[3]", "p.exists()")))

  );
}

TEST(OperationOutcomeErrorReporterTest, ReportErrorSucceeds) {
  OperationOutcome outcome;
  OperationOutcomeErrorReporter reporter(&outcome);

  FHIR_ASSERT_OK(reporter.ReportFhirError("Foo.bar.baz", "Foo.bar[2].baz[3]",
                                          "warning_msg"));
  FHIR_ASSERT_OK(
      reporter.ReportFhirError("Foo.b.c", "no element path warning_msg"));
  FHIR_ASSERT_OK(
      reporter.ReportFhirPathError("Foo.f", "Foo.f[3]", "p.exists()"));

  std::vector<OperationOutcome::Issue> warnings =
      OperationOutcomeErrorReporter::GetFhirErrors(outcome);

  EXPECT_THAT(
      warnings,
      ElementsAre(
          EqualsProto(MakeError("Foo.bar[2].baz[3]", "warning_msg")),
          EqualsProto(MakeError("Foo.b.c", "no element path warning_msg")),
          EqualsProto(MakeError("Foo.f[3]", "p.exists()")))

  );
}

TEST(OperationOutcomeErrorReporterTest, ReportFatalSucceeds) {
  OperationOutcome outcome;
  OperationOutcomeErrorReporter reporter(&outcome);

  FHIR_ASSERT_OK(
      reporter.ReportFhirFatal("Foo.bar.baz", "Foo.bar[2].baz[3]",
                               absl::InvalidArgumentError("fatal error")));
  FHIR_ASSERT_OK(reporter.ReportFhirFatal(
      "Foo.b.c", absl::InvalidArgumentError("no element path warning_msg")));
  FHIR_ASSERT_OK(reporter.ReportFhirPathFatal(
      "Foo.f", "Foo.f[3]", "p.exists()",
      absl::InvalidArgumentError("fhirpath error")));

  std::vector<OperationOutcome::Issue> issues =
      OperationOutcomeErrorReporter::GetFhirFatals(outcome);

  EXPECT_THAT(
      issues,
      ElementsAre(
          EqualsProto(MakeFatal("Foo.bar[2].baz[3]", "fatal error")),
          EqualsProto(MakeFatal("Foo.b.c", "no element path warning_msg")),
          EqualsProto(MakeFatal("Foo.f[3]", "p.exists():fhirpath error")))

  );
}

}  // namespace

}  // namespace google::fhir::r4
