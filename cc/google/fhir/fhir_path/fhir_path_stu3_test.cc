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

// FHIRPath tests specific to STU3
// TODO: many of these are redundant with fhir_path_test.cc
// and should be removed when those tests start to handle multiple version.
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "google/fhir/fhir_path/fhir_path_validation.h"
#include "proto/stu3/codes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "proto/stu3/uscore.pb.h"
#include "proto/stu3/uscore_codes.pb.h"

namespace google {
namespace fhir {
namespace fhir_path {

namespace {

using stu3::uscore::UsCoreBirthSexCode;
using stu3::uscore::UsCorePatient;

using ::google::protobuf::Message;

template <typename T>
T ParseFromString(const std::string& str) {
  google::protobuf::TextFormat::Parser parser;
  parser.AllowPartialMessage(true);
  T t;
  EXPECT_TRUE(parser.ParseFromString(str, &t));
  return t;
}

UsCorePatient ValidUsCorePatient() {
  return ParseFromString<UsCorePatient>(R"proto(
    identifier {
      system { value: "foo" },
      value: { value: "http://example.com/patient" }
    }
  )proto");
}


TEST(FhirPathTest, ProfiledWithExtensions) {
  UsCorePatient patient = ValidUsCorePatient();
  auto race = new stu3::uscore::PatientUSCoreRaceExtension();

  stu3::proto::Coding* coding = race->add_omb_category();
  coding->mutable_code()->set_value("urn:oid:2.16.840.1.113883.6.238");
  coding->mutable_code()->set_value("1002-5");
  patient.set_allocated_race(race);

  patient.mutable_birthsex()->set_value(UsCoreBirthSexCode::MALE);

  MessageValidator validator;
  FHIR_ASSERT_OK(validator.Validate(patient));
}

}  // namespace

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
