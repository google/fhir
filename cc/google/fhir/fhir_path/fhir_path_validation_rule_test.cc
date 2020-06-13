// Copyright 2020 Google LLC
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

#include "google/fhir/fhir_path/fhir_path_validation_rule.h"

#include "gtest/gtest.h"

namespace google::fhir::fhir_path {

namespace {

bool FalseFunction(const ValidationResult& result) { return false; }
bool TrueFunction(const ValidationResult& result) { return true; }

TEST(ValidationRuleBuilder, TrueResultNotIgnored) {
  ValidationResult result("constraint_path", "node_path", "expression", true);
  const ValidationRuleBuilder builder;

  EXPECT_TRUE(builder.StrictValidation()(result));
  EXPECT_TRUE(builder.RelaxedValidation()(result));
  EXPECT_FALSE(builder.CustomValidation(FalseFunction)(result));
  EXPECT_TRUE(builder.CustomValidation(TrueFunction)(result));
}

TEST(ValidationRuleBuilder, TrueResultIgnored) {
  ValidationResult result("constraint_path", "node_path", "expression", true);

  const ValidationRuleBuilder builder =
      ValidationRuleBuilder().IgnoringConstraint("constraint_path",
                                                 "expression");

  EXPECT_TRUE(builder.StrictValidation()(result));
  EXPECT_TRUE(builder.RelaxedValidation()(result));
  EXPECT_TRUE(builder.CustomValidation(FalseFunction)(result));
  EXPECT_TRUE(builder.CustomValidation(TrueFunction)(result));
}

TEST(ValidationRuleBuilder, FalseResultNotIgnored) {
  ValidationResult result("constraint_path", "node_path", "expression", false);
  const ValidationRuleBuilder builder;

  EXPECT_FALSE(builder.StrictValidation()(result));
  EXPECT_FALSE(builder.RelaxedValidation()(result));
  EXPECT_FALSE(builder.CustomValidation(FalseFunction)(result));
  EXPECT_TRUE(builder.CustomValidation(TrueFunction)(result));
}

TEST(ValidationRuleBuilder, FalseResultIgnored) {
  ValidationResult result("constraint_path", "node_path", "expression", false);

  const ValidationRuleBuilder builder =
      ValidationRuleBuilder().IgnoringConstraint("constraint_path",
                                                 "expression");

  EXPECT_TRUE(builder.StrictValidation()(result));
  EXPECT_TRUE(builder.RelaxedValidation()(result));
  EXPECT_TRUE(builder.CustomValidation(FalseFunction)(result));
  EXPECT_TRUE(builder.CustomValidation(TrueFunction)(result));
}

TEST(ValidationRuleBuilder, ErrorResultNotIgnored) {
  ValidationResult result("constraint_path", "node_path", "expression",
                          absl::InvalidArgumentError("foo"));
  const ValidationRuleBuilder builder;

  EXPECT_FALSE(builder.StrictValidation()(result));
  EXPECT_TRUE(builder.RelaxedValidation()(result));
  EXPECT_FALSE(builder.CustomValidation(FalseFunction)(result));
  EXPECT_TRUE(builder.CustomValidation(TrueFunction)(result));
}

TEST(ValidationRuleBuilder, ErrorResultIgnored) {
  ValidationResult result("constraint_path", "node_path", "expression",
                          absl::InvalidArgumentError("foo"));

  const ValidationRuleBuilder builder =
      ValidationRuleBuilder().IgnoringConstraint("constraint_path",
                                                 "expression");

  EXPECT_TRUE(builder.StrictValidation()(result));
  EXPECT_TRUE(builder.RelaxedValidation()(result));
  EXPECT_TRUE(builder.CustomValidation(FalseFunction)(result));
  EXPECT_TRUE(builder.CustomValidation(TrueFunction)(result));
}

}  // namespace

}  // namespace google::fhir::fhir_path
