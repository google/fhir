/*
 * Copyright 2018 Google LLC
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

#ifndef GOOGLE_FHIR_STU3_TEST_HELPER_H_
#define GOOGLE_FHIR_STU3_TEST_HELPER_H_

#include <fstream>
#include <ostream>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/fhir/annotations.h"
#include "google/fhir/r4/primitive_handler.h"
#include "google/fhir/r4/profiles.h"
#include "google/fhir/r4/resource_validation.h"
#include "google/fhir/status/status.h"
#include "google/fhir/stu3/primitive_handler.h"
#include "google/fhir/stu3/profiles.h"
#include "google/fhir/stu3/resource_validation.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"

// When comparing converted FHIR resources to their expected value, you should
// also check whether that resource is considered valid. Invalid resources are
// dropped early during the conversion process, and are not included in the
// final patient bundles. On the other hand, there are cases where the input
// data is missing critical information and should indeed be dropped. These
// macros let you specify which situation applies to any given test case.
// TODO(b/244184211): We temporarily disabled FHIR validation in tests to avoid
// having to fix all tests with missing fields.  Long term, tests where
// validation is important should be fixed, and tests where validation is not
// important should be switched to PARSE_STU3_PROTO.
// TODO(b/244184211): This is not stu3 specific - these can be eliminated
// in favor of PARSE_*_FHIR_PROTO versions.
#define PARSE_VALID_STU3_PROTO(asciipb) \
  ::google::fhir::FhirProtoParseHelper( \
      asciipb, ::google::fhir::NO_EXPECTATION, __FILE__, __LINE__)
#define PARSE_INVALID_STU3_PROTO(asciipb)                                \
  ::google::fhir::FhirProtoParseHelper(asciipb, ::google::fhir::INVALID, \
                                       __FILE__, __LINE__)
#define PARSE_STU3_PROTO(asciipb)       \
  ::google::fhir::FhirProtoParseHelper( \
      asciipb, ::google::fhir::NO_EXPECTATION, __FILE__, __LINE__)

#define PARSE_VALID_FHIR_PROTO(asciipb)                                \
  ::google::fhir::FhirProtoParseHelper(asciipb, ::google::fhir::VALID, \
                                       __FILE__, __LINE__)
#define PARSE_INVALID_FHIR_PROTO(asciipb)                                \
  ::google::fhir::FhirProtoParseHelper(asciipb, ::google::fhir::INVALID, \
                                       __FILE__, __LINE__)
#define PARSE_FHIR_PROTO(asciipb)       \
  ::google::fhir::FhirProtoParseHelper( \
      asciipb, ::google::fhir::NO_EXPECTATION, __FILE__, __LINE__)

namespace google {
namespace fhir {

enum ValidityExpectation { VALID, INVALID, NO_EXPECTATION };

class FhirProtoParseHelper {
 public:
  FhirProtoParseHelper(absl::string_view asciipb, ValidityExpectation validity,
                       absl::string_view file, int line)
      : asciipb_(asciipb), validity_(validity), file_(file), line_(line) {}

  template <class T>
  operator T() {
    T tmp;
    const bool parsed_ok = google::protobuf::TextFormat::ParseFromString(asciipb_, &tmp);
    if (!parsed_ok) {
      EXPECT_TRUE(false) << "Unable to parse FHIR proto of type "
                         << T::descriptor()->name() << " on line " << line_
                         << " in file " << file_;
      return T();
    }
    absl::Status valid_status = absl::OkStatus();
    const bool is_profile =
        IsProfile(T::descriptor()) && IsResource(T::descriptor());
    switch (GetFhirVersion(tmp)) {
      case proto::STU3: {
        valid_status = stu3::ValidateWithoutFhirPath(tmp);
        if (is_profile) {
          auto status_or_normalized = NormalizeStu3(tmp);
          EXPECT_TRUE(status_or_normalized.ok());
          tmp = status_or_normalized.value();
        }
        break;
      }
      case proto::R4: {
        valid_status = r4::ValidateWithoutFhirPath(tmp);
        if (is_profile) {
          auto status_or_normalized = NormalizeR4(tmp);
          EXPECT_TRUE(status_or_normalized.ok());
          tmp = status_or_normalized.value();
        }
        break;
      }
    }
    if (validity_ == VALID) {
      EXPECT_TRUE(valid_status.ok())
          << "Invalid FHIR resource of type " << T::descriptor()->full_name()
          << " on line " << line_ << " in file " << file_ << " : "
          << valid_status.message();
    } else if (validity_ == INVALID) {
      EXPECT_FALSE(valid_status.ok())
          << "Unexpected valid FHIR resource of type "
          << T::descriptor()->name() << " on line " << line_ << " in file "
          << file_ << " : " << valid_status.message();
    }
    return tmp;
  }

 private:
  std::string asciipb_;
  ValidityExpectation validity_;
  std::string file_;
  int line_;
};

inline std::string ReadFile(absl::string_view filename) {
  std::ifstream infile;
  infile.open(absl::StrCat(::testing::SrcDir(), "/com_google_fhir/",
                           filename));

  std::ostringstream out;
  out << infile.rdbuf();
  return out.str();
}

template <class T>
T ReadProto(absl::string_view filename) {
  T result;
  CHECK(google::protobuf::TextFormat::ParseFromString(ReadFile(filename), &result))
      << "Failed to parse proto in file " << filename;
  return result;
}

template <class T>
T ReadStu3Proto(absl::string_view filename) {
  return ReadProto<T>(absl::StrCat("testdata/stu3/", filename));
}

template <class T>
T ReadR4Proto(absl::string_view filename) {
  return ReadProto<T>(absl::StrCat("testdata/r4/", filename));
}

#define HANDLER_TYPE_TEST(type, primitive_value, handlerType)                 \
  TEST(HandlerTypeTest##handlerType, type) {                                  \
    ::google::protobuf::Message* msg =                                                  \
        handlerType::GetInstance()->New##type(primitive_value);               \
    ASSERT_EQ(msg->GetDescriptor()->name(), #type);                           \
    auto extracted_status =                                                   \
        handlerType::GetInstance()->Get##type##Value(*msg);                   \
    FHIR_ASSERT_OK(extracted_status.status());                                \
    ASSERT_EQ(extracted_status.value(), primitive_value);                     \
    ASSERT_EQ(handlerType::GetInstance()->type##Descriptor()->name(), #type); \
    delete msg;                                                               \
                                                                              \
    ::google::protobuf::Any any;                                              \
    ASSERT_NE(::absl::OkStatus(),                                             \
              handlerType::GetInstance()->Get##type##Value(any).status());    \
  }

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_TEST_HELPER_H_
