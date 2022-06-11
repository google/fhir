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

#include "google/protobuf/text_format.h"
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
using ::testing::UnorderedElementsAre;

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

TEST(OperationOutcomeErrorHandlerTest, GetFhirWarningsSucceeds) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeFatal("p1", "m1");
  *outcome.add_issue() = MakeWarning("p2", "m2");
  *outcome.add_issue() = MakeError("p3", "m3");
  *outcome.add_issue() = MakeFatal("p4", "m4");
  *outcome.add_issue() = MakeWarning("p5", "m5");
  *outcome.add_issue() = MakeError("p6", "m6");

  EXPECT_THAT(OperationOutcomeErrorHandler::GetFhirWarnings(outcome),
              UnorderedElementsAre(EqualsProto(MakeWarning("p2", "m2")),
                                   EqualsProto(MakeWarning("p5", "m5"))));
}

TEST(OperationOutcomeErrorHandlerTest, GetFhirErrorsSucceeds) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeFatal("p1", "m1");
  *outcome.add_issue() = MakeWarning("p2", "m2");
  *outcome.add_issue() = MakeError("p3", "m3");
  *outcome.add_issue() = MakeFatal("p4", "m4");
  *outcome.add_issue() = MakeWarning("p5", "m5");
  *outcome.add_issue() = MakeError("p6", "m6");

  EXPECT_THAT(OperationOutcomeErrorHandler::GetFhirErrors(outcome),
              UnorderedElementsAre(EqualsProto(MakeError("p3", "m3")),
                                   EqualsProto(MakeError("p6", "m6"))));
}

TEST(OperationOutcomeErrorHandlerTest, GetFhirFatalsSucceeds) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeFatal("p1", "m1");
  *outcome.add_issue() = MakeWarning("p2", "m2");
  *outcome.add_issue() = MakeError("p3", "m3");
  *outcome.add_issue() = MakeFatal("p4", "m4");
  *outcome.add_issue() = MakeWarning("p5", "m5");
  *outcome.add_issue() = MakeError("p6", "m6");

  EXPECT_THAT(OperationOutcomeErrorHandler::GetFhirFatals(outcome),
              UnorderedElementsAre(EqualsProto(MakeFatal("p1", "m1")),
                                   EqualsProto(MakeFatal("p4", "m4"))));
}

TEST(OperationOutcomeErrorHandlerTest, GetFhirErrorsAndFatalsSucceeds) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeFatal("p1", "m1");
  *outcome.add_issue() = MakeWarning("p2", "m2");
  *outcome.add_issue() = MakeError("p3", "m3");
  *outcome.add_issue() = MakeFatal("p4", "m4");
  *outcome.add_issue() = MakeWarning("p5", "m5");
  *outcome.add_issue() = MakeError("p6", "m6");

  EXPECT_THAT(OperationOutcomeErrorHandler::GetFhirErrorsAndFatals(outcome),
              UnorderedElementsAre(EqualsProto(MakeFatal("p1", "m1")),
                                   EqualsProto(MakeError("p3", "m3")),
                                   EqualsProto(MakeFatal("p4", "m4")),
                                   EqualsProto(MakeError("p6", "m6"))));
}

TEST(OperationOutcomeErrorHandlerTest, HasFhirWarningTrue) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeFatal("p1", "m1");
  *outcome.add_issue() = MakeWarning("p2", "m2");
  *outcome.add_issue() = MakeError("p3", "m3");
  *outcome.add_issue() = MakeFatal("p4", "m4");
  *outcome.add_issue() = MakeWarning("p5", "m5");
  *outcome.add_issue() = MakeError("p6", "m6");

  EXPECT_TRUE(OperationOutcomeErrorHandler::HasFhirWarnings(outcome));
}

TEST(OperationOutcomeErrorHandlerTest, HasFhirWarningFalse) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeFatal("p1", "m1");
  *outcome.add_issue() = MakeError("p3", "m3");
  *outcome.add_issue() = MakeFatal("p4", "m4");
  *outcome.add_issue() = MakeError("p6", "m6");

  EXPECT_FALSE(OperationOutcomeErrorHandler::HasFhirWarnings(outcome));
}

TEST(OperationOutcomeErrorHandlerTest, HasFhirErrorTrue) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeFatal("p1", "m1");
  *outcome.add_issue() = MakeWarning("p2", "m2");
  *outcome.add_issue() = MakeError("p3", "m3");
  *outcome.add_issue() = MakeFatal("p4", "m4");
  *outcome.add_issue() = MakeWarning("p5", "m5");
  *outcome.add_issue() = MakeError("p6", "m6");

  EXPECT_TRUE(OperationOutcomeErrorHandler::HasFhirErrors(outcome));
}

TEST(OperationOutcomeErrorHandlerTest, HasFhirErrorFalse) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeFatal("p1", "m1");
  *outcome.add_issue() = MakeWarning("p3", "m3");
  *outcome.add_issue() = MakeFatal("p4", "m4");
  *outcome.add_issue() = MakeWarning("p6", "m6");

  EXPECT_FALSE(OperationOutcomeErrorHandler::HasFhirErrors(outcome));
}

TEST(OperationOutcomeErrorHandlerTest, HasFhirFatalTrue) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeFatal("p1", "m1");
  *outcome.add_issue() = MakeWarning("p2", "m2");
  *outcome.add_issue() = MakeError("p3", "m3");
  *outcome.add_issue() = MakeFatal("p4", "m4");
  *outcome.add_issue() = MakeWarning("p5", "m5");
  *outcome.add_issue() = MakeError("p6", "m6");

  EXPECT_TRUE(OperationOutcomeErrorHandler::HasFhirFatals(outcome));
}

TEST(OperationOutcomeErrorHandlerTest, HasFhirFatalFalse) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeWarning("p1", "m1");
  *outcome.add_issue() = MakeError("p3", "m3");
  *outcome.add_issue() = MakeWarning("p4", "m4");
  *outcome.add_issue() = MakeError("p6", "m6");

  EXPECT_FALSE(OperationOutcomeErrorHandler::HasFhirFatals(outcome));
}

TEST(OperationOutcomeErrorHandlerTest, HasFhirErrorsOrFatalOnlyError) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeWarning("p2", "m2");
  *outcome.add_issue() = MakeError("p3", "m3");
  *outcome.add_issue() = MakeWarning("p5", "m5");
  *outcome.add_issue() = MakeError("p6", "m6");

  EXPECT_TRUE(OperationOutcomeErrorHandler::HasFhirErrorsOrFatals(outcome));
}

TEST(OperationOutcomeErrorHandlerTest, HasFhirErrorsOrFatalOnlyFatal) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeWarning("p2", "m2");
  *outcome.add_issue() = MakeFatal("p3", "m3");
  *outcome.add_issue() = MakeWarning("p5", "m5");
  *outcome.add_issue() = MakeFatal("p6", "m6");

  EXPECT_TRUE(OperationOutcomeErrorHandler::HasFhirErrorsOrFatals(outcome));
}

TEST(OperationOutcomeErrorHandlerTest, HasFhirErrorsOrFatalFalse) {
  OperationOutcome outcome;
  *outcome.add_issue() = MakeWarning("p2", "m2");
  *outcome.add_issue() = MakeWarning("p5", "m5");

  EXPECT_FALSE(OperationOutcomeErrorHandler::HasFhirErrorsOrFatals(outcome));
}

TEST(OperationOutcomeErrorHandlerTest, HandleAPIsAddIssuesSuccessfully) {
  OperationOutcome outcome;
  OperationOutcomeErrorHandler handler(&outcome);

  FHIR_ASSERT_OK(
      handler.HandleFhirFatal(absl::InternalError("err-1"), "ep1", "fp1"));
  FHIR_ASSERT_OK(handler.HandleFhirError("err-2", "ep2", "fp2"));
  FHIR_ASSERT_OK(handler.HandleFhirWarning("err-3", "ep3", "fp3"));
  FHIR_ASSERT_OK(handler.HandleFhirPathFatal(absl::InternalError("err-4"),
                                             "expr-4", "ep4", "fp4"));
  FHIR_ASSERT_OK(handler.HandleFhirPathError("expr-5", "ep5", "fp5"));
  FHIR_ASSERT_OK(handler.HandleFhirPathWarning("expr-6", "ep6", "fp6"));

  OperationOutcome expected;
  *expected.add_issue() = MakeFatal("ep1", "err-1");
  *expected.add_issue() = MakeError("ep2", "err-2");
  *expected.add_issue() = MakeWarning("ep3", "err-3");
  *expected.add_issue() = MakeFatal("ep4", "expr-4:err-4");
  *expected.add_issue() = MakeError("ep5", "expr-5");
  *expected.add_issue() = MakeWarning("ep6", "expr-6");

  EXPECT_THAT(outcome, EqualsProto(expected));
}

}  // namespace

}  // namespace google::fhir::r4
