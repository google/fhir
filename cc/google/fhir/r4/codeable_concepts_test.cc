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

#include "google/fhir/r4/codeable_concepts.h"

#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "testdata/r4/profiles/test.pb.h"

namespace google {
namespace fhir {
namespace r4 {

namespace {

using ::google::fhir::r4::core::CodeableConcept;
using ::google::fhir::r4::core::Coding;
using ::google::fhir::r4::testing::TestObservation;
using ::google::fhir::testutil::EqualsProto;
using ::google::fhir::testutil::IgnoringRepeatedFieldOrdering;
using ::testing::ElementsAre;

const TestObservation::CodeableConceptForCode GetConcept() {
  return ReadProto<TestObservation::CodeableConceptForCode>(
      "testdata/r4/profiles/testobservation_codeableconceptforcode.prototxt");
}

TEST(CodeableConceptsTest, FindSystemCodeStringPairUnprofiled) {
  std::string found_system;
  std::string found_code;
  const auto codeable_concept = GetConcept();
  EXPECT_TRUE(FindSystemCodeStringPair(
      codeable_concept,
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
  const auto codeable_concept = GetConcept();
  EXPECT_TRUE(FindSystemCodeStringPair(
      codeable_concept,
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
  const auto codeable_concept = GetConcept();
  EXPECT_TRUE(FindSystemCodeStringPair(
      codeable_concept,
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
  const auto codeable_concept = GetConcept();
  EXPECT_FALSE(FindSystemCodeStringPair(
      codeable_concept,
      [](const std::string& system, const std::string& code) { return false; },
      &found_system, &found_code));
}

TEST(CodeableConceptsTest, FindCodingUnprofiled) {
  const auto codeable_concept = GetConcept();
  std::shared_ptr<const r4::core::Coding> coding =
      FindCoding(codeable_concept, [](const Coding& test_coding) {
        return test_coding.display().value() == "FDisplay";
      });
  EXPECT_EQ(coding->code().value(), "fcode");
}

TEST(CodeableConceptsTest, FindCodingFixedSystem) {
  const auto codeable_concept = GetConcept();
  std::shared_ptr<const r4::core::Coding> coding =
      FindCoding(codeable_concept, [](const Coding& test_coding) {
        return test_coding.code().value() == "acode";
      });
  EXPECT_EQ(coding->display().value(), "A Display");
}

TEST(CodeableConceptsTest, FindCodingFixedCode) {
  const auto codeable_concept = GetConcept();
  std::shared_ptr<const r4::core::Coding> coding =
      FindCoding(codeable_concept, [](const Coding& test_coding) {
        return test_coding.code().value() == "8675329";
      });
  EXPECT_EQ(coding->display().value(), "display d");
}

TEST(CodeableConceptsTest, FindCodingNotFound) {
  const auto codeable_concept = GetConcept();
  std::shared_ptr<const r4::core::Coding> coding = FindCoding(
      codeable_concept, [](const Coding& test_coding) { return false; });
  EXPECT_FALSE(coding);
}

TEST(CodeableConceptsTest, ForEachSystemCodeStringPair) {
  const auto codeable_concept = GetConcept();
  std::string sys_accum = "";
  std::string code_accum = "";
  ForEachSystemCodeStringPair(
      codeable_concept, [&sys_accum, &code_accum](const std::string& sys,
                                                  const std::string& code) {
        absl::StrAppend(&sys_accum, sys, ",");
        absl::StrAppend(&code_accum, code, ",");
      });
  EXPECT_EQ(
      sys_accum,
      "http://sysf.org,http://sysg.org,http://sysg.org,http://sysa.org,http://"
      "sysb.org,http://sysb.org,http://sysc.org,http://sysd.org,");
  EXPECT_EQ(code_accum,
            "fcode,gcode1,gcode2,acode,bcode1,bcode2,8472,8675329,");
}

TEST(CodeableConceptsTest, ForEachCoding) {
  const auto codeable_concept = GetConcept();
  std::string display_accum = "";
  ForEachCoding(codeable_concept, [&display_accum](const Coding& coding) {
    absl::StrAppend(&display_accum,
                    coding.has_display() ? coding.display().value() : "NONE",
                    ",");
  });
  EXPECT_EQ(display_accum,
            "FDisplay,GDisplay1,GDisplay2,A "
            "Display,BDisplay1,BDisplay2,NONE,display d,");
}

TEST(CodeableConceptsTest, GetCodesWithSystemUnprofiled) {
  const auto codeable_concept = GetConcept();
  ASSERT_THAT(GetCodesWithSystem(codeable_concept, "http://sysg.org"),
              ElementsAre("gcode1", "gcode2"));
}

TEST(CodeableConceptsTest, GetCodesWithSystemFixedSystem) {
  const auto codeable_concept = GetConcept();
  ASSERT_THAT(GetCodesWithSystem(codeable_concept, "http://sysb.org"),
              ElementsAre("bcode1", "bcode2"));
}

TEST(CodeableConceptsTest, GetCodesWithSystemFixedCode) {
  const auto codeable_concept = GetConcept();
  ASSERT_THAT(GetCodesWithSystem(codeable_concept, "http://sysc.org"),
              ElementsAre("8472"));
}

TEST(CodeableConceptsTest, GetOnlyCodeWithSystemUnprofiled) {
  const auto codeable_concept = GetConcept();
  EXPECT_EQ(GetOnlyCodeWithSystem(codeable_concept, "http://sysf.org").value(),
            "fcode");
}

TEST(CodeableConceptsTest, GetOnlyCodeWithSystemFixedSystem) {
  const auto codeable_concept = GetConcept();
  EXPECT_EQ(GetOnlyCodeWithSystem(codeable_concept, "http://sysa.org").value(),
            "acode");
}

TEST(CodeableConceptsTest, GetOnlyCodeWithSystemFixedCode) {
  const auto codeable_concept = GetConcept();
  EXPECT_EQ(GetOnlyCodeWithSystem(codeable_concept, "http://sysc.org").value(),
            "8472");
}

TEST(CodeableConceptsTest, GetOnlyCodeWithSystemUnprofiledTooMany) {
  const auto codeable_concept = GetConcept();
  ASSERT_EQ(GetOnlyCodeWithSystem(codeable_concept, "http://sysg.org")
                .status()
                .code(),
            ::absl::StatusCode::kAlreadyExists);
}

TEST(CodeableConceptsTest, GetOnlyCodeWithSystemProfiledTooMany) {
  const auto codeable_concept = GetConcept();
  ASSERT_EQ(GetOnlyCodeWithSystem(codeable_concept, "http://sysb.org")
                .status()
                .code(),
            ::absl::StatusCode::kAlreadyExists);
}

TEST(CodeableConceptsTest, GetOnlyCodeWithSystemNone) {
  const auto codeable_concept = GetConcept();
  ASSERT_EQ(GetOnlyCodeWithSystem(codeable_concept, "http://sysq.org")
                .status()
                .code(),
            ::absl::StatusCode::kNotFound);
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
  TestObservation::CodeableConceptForCode codeable_concept;

  FHIR_CHECK_OK(
      AddCoding(&codeable_concept,
                MakeCoding("http://sysq.org", "qcode1", "Q display1")));
  FHIR_CHECK_OK(
      AddCoding(&codeable_concept,
                MakeCoding("http://sysq.org", "qcode2", "Q display2")));
  FHIR_CHECK_OK(AddCoding(&codeable_concept,
                          MakeCoding("http://sysr.org", "rcode", "R display")));

  EXPECT_EQ(codeable_concept.coding_size(), 3);
  std::string display_accum = "";
  ForEachCoding(codeable_concept, [&display_accum](const Coding& coding) {
    absl::StrAppend(&display_accum, coding.display().value(), ",");
  });
  EXPECT_EQ(display_accum, "Q display1,Q display2,R display,");
}

TEST(CodeableConceptsTest, AddCodingFixedSystem) {
  TestObservation::CodeableConceptForCode codeable_concept;

  FHIR_CHECK_OK(AddCoding(&codeable_concept,
                          MakeCoding("http://sysa.org", "acode", "A display")));
  FHIR_CHECK_OK(
      AddCoding(&codeable_concept,
                MakeCoding("http://sysb.org", "bcode1", "B display1")));
  FHIR_CHECK_OK(AddCoding(
      &codeable_concept, MakeCoding("http://sysb.org", "bcode", "B display2")));

  EXPECT_EQ(codeable_concept.coding_size(), 0);
  EXPECT_EQ(codeable_concept.sys_b_size(), 2);
  std::string display_accum = "";
  ForEachCoding(codeable_concept, [&display_accum](const Coding& coding) {
    absl::StrAppend(&display_accum, coding.display().value(), ",");
  });
  EXPECT_EQ(display_accum, "A display,B display1,B display2,");
}

TEST(CodeableConceptsTest, AddCodingFixedCode) {
  TestObservation::CodeableConceptForCode codeable_concept;

  FHIR_CHECK_OK(AddCoding(&codeable_concept,
                          MakeCoding("http://sysc.org", "8472", "C display")));
  FHIR_CHECK_OK(
      AddCoding(&codeable_concept,
                MakeCoding("http://sysd.org", "8675329", "D display")));

  EXPECT_EQ(codeable_concept.coding_size(), 0);
  std::string display_accum = "";
  ForEachCoding(codeable_concept, [&display_accum](const Coding& coding) {
    absl::StrAppend(&display_accum, coding.display().value(), ",");
  });
  EXPECT_EQ(display_accum, "C display,D display,");
}

TEST(CodeableConceptsTest, AddCodingFixedSystemSingularAlreadyExists) {
  TestObservation::CodeableConceptForCode codeable_concept;
  // sysa is inlined in a non-repeated field, meaning it can only have one code
  // from that system.
  FHIR_CHECK_OK(
      AddCoding(&codeable_concept,
                MakeCoding("http://sysa.org", "acode1", "A display1")));
  EXPECT_EQ(AddCoding(&codeable_concept,
                      MakeCoding("http://sysa.org", "acode2", "A display2"))
                .code(),
            ::absl::StatusCode::kAlreadyExists);
}

TEST(CodeableConceptsTest, AddCodingFixedCodeAlreadyExists) {
  TestObservation::CodeableConceptForCode codeable_concept;
  // sysc is inlined in a non-repeated fixed-code field, meaning it can only
  // have one with the fixed code and system.
  FHIR_CHECK_OK(AddCoding(&codeable_concept,
                          MakeCoding("http://sysc.org", "8472", "C display")));
  EXPECT_EQ(AddCoding(&codeable_concept,
                      MakeCoding("http://sysc.org", "8472", "C display other"))
                .code(),
            ::absl::StatusCode::kAlreadyExists);
}

TEST(CodeableConceptsTest, AddCodingToSameSystemAsFixedCodeOk) {
  TestObservation::CodeableConceptForCode codeable_concept;
  // sysc is inlined in a fixed-code field.
  // If we add a coding from that system that doesn't match the expected code,
  // it should just go in as an unprofiled coding.
  FHIR_CHECK_OK(AddCoding(&codeable_concept,
                          MakeCoding("http://sysc.org", "8471", "normal 1")));
  FHIR_CHECK_OK(
      AddCoding(&codeable_concept,
                MakeCoding("http://sysc.org", "8472", "magic inlined")));
  FHIR_CHECK_OK(AddCoding(&codeable_concept,
                          MakeCoding("http://sysc.org", "8473", "normal 2")));

  EXPECT_EQ(codeable_concept.coding_size(), 2);
  EXPECT_EQ(codeable_concept.coding(0).display().value(), "normal 1");
  EXPECT_EQ(codeable_concept.coding(1).display().value(), "normal 2");
  EXPECT_EQ(codeable_concept.sys_c().display().value(), "magic inlined");
}

TEST(CodeableConceptsTest, ClearAllCodingsWithSystemUnprofiled) {
  auto codeable_concept = GetConcept();
  FHIR_CHECK_OK(
      ClearAllCodingsWithSystem(&codeable_concept, "http://sysg.org"));
  std::string display_accum = "";
  ForEachCoding(codeable_concept, [&display_accum](const Coding& coding) {
    absl::StrAppend(&display_accum,
                    coding.has_display() ? coding.display().value() : "NONE",
                    ",");
  });
  EXPECT_EQ(display_accum,
            "FDisplay,A Display,BDisplay1,BDisplay2,NONE,display d,");
}

TEST(CodeableConceptsTest, ClearAllCodingsWithSystemFixedSystem) {
  auto codeable_concept = GetConcept();
  FHIR_CHECK_OK(
      ClearAllCodingsWithSystem(&codeable_concept, "http://sysb.org"));
  std::string display_accum = "";
  ForEachCoding(codeable_concept, [&display_accum](const Coding& coding) {
    absl::StrAppend(&display_accum,
                    coding.has_display() ? coding.display().value() : "NONE",
                    ",");
  });
  EXPECT_EQ(display_accum,
            "FDisplay,GDisplay1,GDisplay2,A Display,NONE,display d,");
}

TEST(CodeableConceptsTest, ClearAllCodingsWithSystemFixedCode) {
  auto codeable_concept = GetConcept();
  EXPECT_FALSE(
      ClearAllCodingsWithSystem(&codeable_concept, "http://sysc.org").ok());
}

TEST(CodeableConceptsTest, CopyCodeableConcept) {
  CodeableConcept codeable_concept = PARSE_FHIR_PROTO(R"pb(
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
  )pb");
  TestObservation::CodeableConceptForCode concept_for_code =
      PARSE_FHIR_PROTO(R"pb(
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
      )pb");
  TestObservation::CodeableConceptForCategory concept_for_cat =
      PARSE_FHIR_PROTO(R"pb(
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
      )pb");

  CodeableConcept profiled_to_unprofiled;
  FHIR_ASSERT_OK(
      CopyCodeableConcept(concept_for_code, &profiled_to_unprofiled));
  ASSERT_THAT(codeable_concept, IgnoringRepeatedFieldOrdering(
                                    EqualsProto(profiled_to_unprofiled)));

  TestObservation::CodeableConceptForCode unprofiled_to_profiled;
  FHIR_ASSERT_OK(
      CopyCodeableConcept(codeable_concept, &unprofiled_to_profiled));
  ASSERT_THAT(concept_for_code, IgnoringRepeatedFieldOrdering(
                                    EqualsProto(unprofiled_to_profiled)));

  TestObservation::CodeableConceptForCategory profiled_to_profiled;
  FHIR_ASSERT_OK(CopyCodeableConcept(concept_for_code, &profiled_to_profiled));
  ASSERT_THAT(concept_for_cat,
              IgnoringRepeatedFieldOrdering(EqualsProto(profiled_to_profiled)));
}

TEST(CodeableConceptsTest, AddCodingFromStrings) {
  r4::core::CodeableConcept codeable_concept;

  FHIR_CHECK_OK(AddCoding(&codeable_concept, "http://sysq.org", "qcode1"));
  FHIR_CHECK_OK(AddCoding(&codeable_concept, "http://sysq.org", "qcode2"));
  FHIR_CHECK_OK(AddCoding(&codeable_concept, "http://sysr.org", "rcode"));

  EXPECT_EQ(codeable_concept.coding_size(), 3);
  std::string code_accum = "";
  ForEachCoding(codeable_concept, [&code_accum](const Coding& coding) {
    absl::StrAppend(&code_accum, coding.code().value(), ",");
  });
  EXPECT_EQ(code_accum, "qcode1,qcode2,rcode,");
}

Coding MakeUnprofiledCoding(const std::string& system,
                            const std::string& code) {
  Coding coding;
  coding.mutable_system()->set_value(system);
  coding.mutable_code()->set_value(code);
  return coding;
}

TEST(CodeableConceptsTest, GetAllCodingsWithSystemNoMatchesSucceeds) {
  r4::core::CodeableConcept codeable_concept;

  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysq.org", "qcode1");
  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysq.org", "qcode2");
  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysr.org", "rcode");

  EXPECT_TRUE(
      GetAllCodingsWithSystem(codeable_concept, "not-found-system").empty());
}

TEST(CodeableConceptsTest, GetAllCodingsWithSystemMultipleFoundSucceeds) {
  r4::core::CodeableConcept codeable_concept;

  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysq.org", "qcode1");
  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysr.org", "rcode");
  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysq.org", "qcode2");

  EXPECT_THAT(GetAllCodingsWithSystem(codeable_concept, "http://sysq.org"),
              ElementsAre(EqualsProto(codeable_concept.coding(0)),
                          EqualsProto(codeable_concept.coding(2))));
}

TEST(CodeableConceptsTest, GetOnlyCodingsWithSystemNoMatchesFails) {
  r4::core::CodeableConcept codeable_concept;

  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysq.org", "qcode1");
  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysq.org", "qcode2");
  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysr.org", "rcode");

  EXPECT_EQ(GetOnlyCodingWithSystem(codeable_concept, "not-found-system")
                .status()
                .code(),
            absl::StatusCode::kNotFound);
}

TEST(CodeableConceptsTest, GetOnlyCodingsWithSystemMultipleFoundFails) {
  r4::core::CodeableConcept codeable_concept;

  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysq.org", "qcode1");
  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysr.org", "rcode");
  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysq.org", "qcode2");

  EXPECT_EQ(GetOnlyCodingWithSystem(codeable_concept, "http://sysq.org")
                .status()
                .code(),
            absl::StatusCode::kInvalidArgument);
}

TEST(CodeableConceptsTest, GetOnlyCodingsWithSystemOneFoundSucceeds) {
  r4::core::CodeableConcept codeable_concept;

  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysq.org", "qcode1");
  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysr.org", "rcode");
  *codeable_concept.add_coding() =
      MakeUnprofiledCoding("http://sysq.org", "qcode2");

  auto test_out = GetOnlyCodingWithSystem(codeable_concept, "http://sysr.org");

  EXPECT_TRUE(test_out.ok());
  EXPECT_THAT(*test_out, EqualsProto(codeable_concept.coding(1)));
}

}  // namespace

}  // namespace r4
}  // namespace fhir
}  // namespace google
