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

#include "google/fhir/systems/systems.h"

#include "gtest/gtest.h"

namespace google {
namespace fhir {
namespace systems {
namespace {


TEST(UtilTest, FormatIcd9Diagnosis) {
  EXPECT_EQ("768.70", FormatIcd9Diagnosis("76870"));
  EXPECT_EQ("768.0", FormatIcd9Diagnosis("768.0"));
  EXPECT_EQ("768", FormatIcd9Diagnosis("768"));
  EXPECT_EQ("E005.4", FormatIcd9Diagnosis("E0054"));
  EXPECT_EQ("E005", FormatIcd9Diagnosis("E005"));
}

TEST(UtilTest, FormatIcd9Procedure) {
  EXPECT_EQ("93.04", FormatIcd9Procedure("9304"));
  EXPECT_EQ("93.0", FormatIcd9Procedure("93.0"));
  EXPECT_EQ("93", FormatIcd9Procedure("93"));
}

TEST(UtilTest, ToShortSystemNameLoinc) {
  EXPECT_EQ("loinc", ToShortSystemName("http://loinc.org"));
}

TEST(UtilTest, ToShortSystemNameIcd9) {
  EXPECT_EQ("icd9", ToShortSystemName("http://hl7.org/fhir/sid/icd-9-cm"));
}

TEST(UtilTest, ToShortSystemNameActCode) {
  EXPECT_EQ("actcode", ToShortSystemName("http://hl7.org/fhir/v3/ActCode"));
}

TEST(UtilTest, ToShortSystemNameUnknown) {
  EXPECT_EQ("http-hl7-org-fhir-sid-cvx",
            ToShortSystemName("http://hl7.org/fhir/sid/cvx"));
}

}  // namespace
}  // namespace systems
}  // namespace fhir
}  // namespace google
