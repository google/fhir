// Copyright 2019 Google LLC
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

#include "google/fhir/stu3/codeable_concepts.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/stu3/datatypes.pb.h"
#include "testdata/stu3/profiles/test.pb.h"
#include "tensorflow/core/lib/core/status_test_util.h"

namespace google {
namespace fhir {
namespace stu3 {

namespace {

using ::google::fhir::stu3::proto::CodeableConcept;
using ::google::fhir::stu3::proto::Coding;
using ::google::fhir::stu3::testing::TestObservation;
using ::testing::ElementsAre;

const TestObservation::CodeableConceptForCode GetConcept() {
  return ReadProto<TestObservation::CodeableConceptForCode>(
      "testdata/stu3/profiles/testobservation_codeableconceptforcode.prototxt");
}

TEST(CodeableConceptsTest, FindSystemCodeStringPairUnprofiled) {
  std::string found_system;
  std::string found_code;
  const auto concept = GetConcept();
  EXPECT_TRUE(FindSystemCodeStringPair(
      concept,
      [](const std::string& system, const std::string& code) {
        return system == "http://sysg.org" && code == "gcode1";
      },
      &found_system, &found_code));
  EXPECT_EQ(found_system, "http://sysg.org");
  EXPECT_EQ(found_code, "gcode1");
}

TEST(CodeableConceptsTest, FindSystemCodeStringPairFixedSystem) {
  std::string found_system;
  std::string found_code;
  const auto concept = GetConcept();
  EXPECT_TRUE(FindSystemCodeStringPair(
      concept,
      [](const std::string& system, const std::string& code) {
        return system == "http://sysb.org" && code == "bcode2";
      },
      &found_system, &found_code));

  EXPECT_EQ(found_system, "http://sysb.org");
  EXPECT_EQ(found_code, "bcode2");
}

TEST(CodeableConceptsTest, FindSystemCodeStringPairFixedCode) {
  std::string found_system;
  std::string found_code;
  const auto concept = GetConcept();
  EXPECT_TRUE(FindSystemCodeStringPair(
      concept,
      [](const std::string& system, const std::string& code) {
        return system == "http://sysd.org" && code == "8675329";
      },
      &found_system, &found_code));

  EXPECT_EQ(found_system, "http://sysd.org");
  EXPECT_EQ(found_code, "8675329");
}

TEST(CodeableConceptsTest, FindSystemCodeStringPairNotFound) {
  std::string found_system;
  std::string found_code;
  const auto concept = GetConcept();
  EXPECT_FALSE(FindSystemCodeStringPair(
      concept,
      [](const std::string& system, const std::string& code) { return false; },
      &found_system, &found_code));
}

TEST(CodeableConceptsTest, FindCodingUnprofiled) {
  const auto concept = GetConcept();
  std::shared_ptr<const stu3::proto::Coding> coding =
      FindCoding(concept, [](const Coding& test_coding) {
        return test_coding.display().value() == "FDisplay";
      });
  EXPECT_EQ(coding->code().value(), "fcode");
}

TEST(CodeableConceptsTest, FindCodingFixedSystem) {
  const auto concept = GetConcept();
  std::shared_ptr<const stu3::proto::Coding> coding =
      FindCoding(concept, [](const Coding& test_coding) {
        return test_coding.code().value() == "acode";
      });
  EXPECT_EQ(coding->display().value(), "A Display");
}

TEST(CodeableConceptsTest, FindCodingFixedCode) {
  const auto concept = GetConcept();
  std::shared_ptr<const stu3::proto::Coding> coding =
      FindCoding(concept, [](const Coding& test_coding) {
        return test_coding.code().value() == "8675329";
      });
  EXPECT_EQ(coding->display().value(), "display d");
}

TEST(CodeableConceptsTest, FindCodingNotFound) {
  const auto concept = GetConcept();
  std::shared_ptr<const stu3::proto::Coding> coding =
      FindCoding(concept, [](const Coding& test_coding) { return false; });
  EXPECT_FALSE(coding);
}

TEST(CodeableConceptsTest, ForEachSystemCodeStringPair) {
  const auto concept = GetConcept();
  std::string sys_accum = "";
  std::string code_accum = "";
  ForEachSystemCodeStringPair(
      concept, [&sys_accum, &code_accum](const std::string& sys,
                                         const std::string& code) {
        sys_accum = absl::StrCat(sys_accum, sys, ",");
        code_accum = absl::StrCat(code_accum, code, ",");
      });
  EXPECT_EQ(
      sys_accum,
      "http://sysf.org,http://sysg.org,http://sysg.org,http://sysa.org,http://"
      "sysb.org,http://sysb.org,http://sysc.org,http://sysd.org,");
  EXPECT_EQ(code_accum,
            "fcode,gcode1,gcode2,acode,bcode1,bcode2,8472,8675329,");
}

TEST(CodeableConceptsTest, ForEachCoding) {
  const auto concept = GetConcept();
  std::string display_accum = "";
  ForEachCoding(concept, [&display_accum](const Coding& coding) {
    display_accum = absl::StrCat(
        display_accum, coding.has_display() ? coding.display().value() : "NONE",
        ",");
  });
  EXPECT_EQ(display_accum,
            "FDisplay,GDisplay1,GDisplay2,A "
            "Display,BDisplay1,BDisplay2,NONE,display d,");
}

TEST(CodeableConceptsTest, GetCodesWithSystemUnprofiled) {
  const auto concept = GetConcept();
  ASSERT_THAT(GetCodesWithSystem(concept, "http://sysg.org"),
              ElementsAre("gcode1", "gcode2"));
}

TEST(CodeableConceptsTest, GetCodesWithSystemFixedSystem) {
  const auto concept = GetConcept();
  ASSERT_THAT(GetCodesWithSystem(concept, "http://sysb.org"),
              ElementsAre("bcode1", "bcode2"));
}

TEST(CodeableConceptsTest, GetCodesWithSystemFixedCode) {
  const auto concept = GetConcept();
  ASSERT_THAT(GetCodesWithSystem(concept, "http://sysc.org"),
              ElementsAre("8472"));
}

TEST(CodeableConceptsTest, GetOnlyCodeWithSystemUnprofiled) {
  const auto concept = GetConcept();
  EXPECT_EQ(GetOnlyCodeWithSystem(concept, "http://sysf.org").ValueOrDie(),
            "fcode");
}

TEST(CodeableConceptsTest, GetOnlyCodeWithSystemFixedSystem) {
  const auto concept = GetConcept();
  EXPECT_EQ(GetOnlyCodeWithSystem(concept, "http://sysa.org").ValueOrDie(),
            "acode");
}

TEST(CodeableConceptsTest, GetOnlyCodeWithSystemFixedCode) {
  const auto concept = GetConcept();
  EXPECT_EQ(GetOnlyCodeWithSystem(concept, "http://sysc.org").ValueOrDie(),
            "8472");
}

TEST(CodeableConceptsTest, GetOnlyCodeWithSystemUnprofiledTooMany) {
  const auto concept = GetConcept();
  ASSERT_EQ(GetOnlyCodeWithSystem(concept, "http://sysg.org").status().code(),
            ::tensorflow::error::Code::ALREADY_EXISTS);
}

TEST(CodeableConceptsTest, GetOnlyCodeWithSystemProfiledTooMany) {
  const auto concept = GetConcept();
  ASSERT_EQ(GetOnlyCodeWithSystem(concept, "http://sysb.org").status().code(),
            ::tensorflow::error::Code::ALREADY_EXISTS);
}

TEST(CodeableConceptsTest, GetOnlyCodeWithSystemNone) {
  const auto concept = GetConcept();
  ASSERT_EQ(GetOnlyCodeWithSystem(concept, "http://sysq.org").status().code(),
            ::tensorflow::error::Code::NOT_FOUND);
}

Coding MakeCoding(const std::string& sys, const std::string& code,
                  const std::string& display) {
  Coding coding;
  coding.mutable_system()->set_value(sys);
  coding.mutable_code()->set_value(code);
  coding.mutable_display()->set_value(display);
  return coding;
}

TEST(CodeableConceptsTest, AddCodingUnprofiled) {
  TestObservation::CodeableConceptForCode concept;

  TF_CHECK_OK(AddCoding(&concept,
                        MakeCoding("http://sysq.org", "qcode1", "Q display1")));
  TF_CHECK_OK(AddCoding(&concept,
                        MakeCoding("http://sysq.org", "qcode2", "Q display2")));
  TF_CHECK_OK(
      AddCoding(&concept, MakeCoding("http://sysr.org", "rcode", "R display")));

  EXPECT_EQ(concept.coding_size(), 3);
  std::string display_accum = "";
  ForEachCoding(concept, [&display_accum](const Coding& coding) {
    display_accum = absl::StrCat(display_accum, coding.display().value(), ",");
  });
  EXPECT_EQ(display_accum, "Q display1,Q display2,R display,");
}

TEST(CodeableConceptsTest, AddCodingFixedSystem) {
  TestObservation::CodeableConceptForCode concept;

  TF_CHECK_OK(
      AddCoding(&concept, MakeCoding("http://sysa.org", "acode", "A display")));
  TF_CHECK_OK(AddCoding(&concept,
                        MakeCoding("http://sysb.org", "bcode1", "B display1")));
  TF_CHECK_OK(AddCoding(&concept,
                        MakeCoding("http://sysb.org", "bcode", "B display2")));

  EXPECT_EQ(concept.coding_size(), 0);
  EXPECT_EQ(concept.sys_b_size(), 2);
  std::string display_accum = "";
  ForEachCoding(concept, [&display_accum](const Coding& coding) {
    display_accum = absl::StrCat(display_accum, coding.display().value(), ",");
  });
  EXPECT_EQ(display_accum, "A display,B display1,B display2,");
}

TEST(CodeableConceptsTest, AddCodingFixedCode) {
  TestObservation::CodeableConceptForCode concept;

  TF_CHECK_OK(
      AddCoding(&concept, MakeCoding("http://sysc.org", "8472", "C display")));
  TF_CHECK_OK(AddCoding(&concept,
                        MakeCoding("http://sysd.org", "8675329", "D display")));

  EXPECT_EQ(concept.coding_size(), 0);
  std::string display_accum = "";
  ForEachCoding(concept, [&display_accum](const Coding& coding) {
    display_accum = absl::StrCat(display_accum, coding.display().value(), ",");
  });
  EXPECT_EQ(display_accum, "C display,D display,");
}

TEST(CodeableConceptsTest, AddCodingFixedSystemSingularAlreadyExists) {
  TestObservation::CodeableConceptForCode concept;
  // sysa is inlined in a non-repeated field, meaning it can only have one code
  // from that system.
  TF_CHECK_OK(AddCoding(&concept,
                        MakeCoding("http://sysa.org", "acode1", "A display1")));
  EXPECT_EQ(
      AddCoding(&concept, MakeCoding("http://sysa.org", "acode2", "A display2"))
          .code(),
      ::tensorflow::error::Code::ALREADY_EXISTS);
}

TEST(CodeableConceptsTest, AddCodingFixedCodeAlreadyExists) {
  TestObservation::CodeableConceptForCode concept;
  // sysc is inlined in a non-repeated fixed-code field, meaning it can only
  // have one with the fixed code and system.
  TF_CHECK_OK(
      AddCoding(&concept, MakeCoding("http://sysc.org", "8472", "C display")));
  EXPECT_EQ(AddCoding(&concept,
                      MakeCoding("http://sysc.org", "8472", "C display other"))
                .code(),
            ::tensorflow::error::Code::ALREADY_EXISTS);
}

TEST(CodeableConceptsTest, AddCodingToSameSystemAsFixedCodeOk) {
  TestObservation::CodeableConceptForCode concept;
  // sysc is inlined in a fixed-code field.
  // If we add a coding from that system that doesn't match the expected code,
  // it should just go in as an unprofiled coding.
  TF_CHECK_OK(
      AddCoding(&concept, MakeCoding("http://sysc.org", "8471", "normal 1")));
  TF_CHECK_OK(AddCoding(
      &concept, MakeCoding("http://sysc.org", "8472", "magic inlined")));
  TF_CHECK_OK(
      AddCoding(&concept, MakeCoding("http://sysc.org", "8473", "normal 2")));

  EXPECT_EQ(concept.coding_size(), 2);
  EXPECT_EQ(concept.coding(0).display().value(), "normal 1");
  EXPECT_EQ(concept.coding(1).display().value(), "normal 2");
  EXPECT_EQ(concept.sys_c().display().value(), "magic inlined");
}

TEST(CodeableConceptsTest, ClearAllCodingsWithSystemUnprofiled) {
  auto concept = GetConcept();
  TF_CHECK_OK(ClearAllCodingsWithSystem(&concept, "http://sysg.org"));
  std::string display_accum = "";
  ForEachCoding(concept, [&display_accum](const Coding& coding) {
    display_accum = absl::StrCat(
        display_accum, coding.has_display() ? coding.display().value() : "NONE",
        ",");
  });
  EXPECT_EQ(display_accum,
            "FDisplay,A Display,BDisplay1,BDisplay2,NONE,display d,");
}

TEST(CodeableConceptsTest, ClearAllCodingsWithSystemFixedSystem) {
  auto concept = GetConcept();
  TF_CHECK_OK(ClearAllCodingsWithSystem(&concept, "http://sysb.org"));
  std::string display_accum = "";
  ForEachCoding(concept, [&display_accum](const Coding& coding) {
    display_accum = absl::StrCat(
        display_accum, coding.has_display() ? coding.display().value() : "NONE",
        ",");
  });
  EXPECT_EQ(display_accum,
            "FDisplay,GDisplay1,GDisplay2,A Display,NONE,display d,");
}

TEST(CodeableConceptsTest, ClearAllCodingsWithSystemFixedCode) {
  auto concept = GetConcept();
  EXPECT_FALSE(ClearAllCodingsWithSystem(&concept, "http://sysc.org").ok());
}

TEST(CodeableConceptsTest, CopyCodeableConcept) {
  CodeableConcept concept = PARSE_STU3_PROTO(R"proto(
    coding {
      system { value: "foo" },
      code { value: "bar" }
    },
    coding {
      system { value: "http://catA.org" },
      code { value: "bar" }
    }
    coding {
      system { value: "http://sysa.org" }
      code { value: "acode" }
      display { value: "A Display" }
    },
    coding {
      system { value: "http://sysc.org" },
      code { value: "8472" }
    },
    text { value: "some text to copy" }
    id { value: "2134" }
    extension {
      url { value: "foo" }
      value { string_value { value: "bar" } }
    }
    extension {
      url { value: "baz" }
      value { integer { value: 5 } }
    }
  )proto");
  TestObservation::CodeableConceptForCode concept_for_code =
      PARSE_STU3_PROTO(R"proto(
        # inlined system
        sys_a {
          code { value: "acode" },
          display { value: "A Display" }
        }
        # inlined system and code
        sys_c {}
        coding {
          system { value: "foo" },
          code { value: "bar" }
        }
        coding {
          system { value: "http://catA.org" },
          code { value: "bar" }
        }
        text { value: "some text to copy" }
        id { value: "2134" }
        extension {
          url { value: "foo" }
          value { string_value { value: "bar" } }
        }
        extension {
          url { value: "baz" }
          value { integer { value: 5 } }
        }
      )proto");
  TestObservation::CodeableConceptForCategory concept_for_cat =
      PARSE_STU3_PROTO(R"proto(
        coding {
          system { value: "http://sysa.org" }
          code { value: "acode" }
          display { value: "A Display" }
        },
        coding {
          system { value: "http://sysc.org" },
          code { value: "8472" }
        },
        coding {
          system { value: "foo" },
          code { value: "bar" }
        },
        # inlined system
        cat_a { code { value: "bar" } }
        text { value: "some text to copy" }
        id { value: "2134" }
        extension {
          url { value: "foo" }
          value { string_value { value: "bar" } }
        }
        extension {
          url { value: "baz" }
          value { integer { value: 5 } }
        }
      )proto");

  CodeableConcept profiled_to_unprofiled;
  TF_ASSERT_OK(CopyCodeableConcept(concept_for_code, &profiled_to_unprofiled));
  ASSERT_THAT(concept,
              testutil::EqualsProtoIgnoringReordering(profiled_to_unprofiled));

  TestObservation::CodeableConceptForCode unprofiled_to_profiled;
  TF_ASSERT_OK(CopyCodeableConcept(concept, &unprofiled_to_profiled));
  ASSERT_THAT(concept_for_code,
              testutil::EqualsProtoIgnoringReordering(unprofiled_to_profiled));

  TestObservation::CodeableConceptForCategory profiled_to_profiled;
  TF_ASSERT_OK(CopyCodeableConcept(concept_for_code, &profiled_to_profiled));
  ASSERT_THAT(concept_for_cat,
              testutil::EqualsProtoIgnoringReordering(profiled_to_profiled));
}

TEST(CodeableConceptsTest, AddCodingFromStrings) {
  stu3::proto::CodeableConcept concept;

  TF_CHECK_OK(AddCoding(&concept, "http://sysq.org", "qcode1"));
  TF_CHECK_OK(AddCoding(&concept, "http://sysq.org", "qcode2"));
  TF_CHECK_OK(AddCoding(&concept, "http://sysr.org", "rcode"));

  EXPECT_EQ(concept.coding_size(), 3);
  std::string code_accum = "";
  ForEachCoding(concept, [&code_accum](const Coding& coding) {
    absl::StrAppend(&code_accum, coding.code().value(), ",");
  });
  EXPECT_EQ(code_accum, "qcode1,qcode2,rcode,");
}

}  // namespace

}  // namespace stu3
}  // namespace fhir
}  // namespace google
