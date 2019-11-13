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

#include "google/fhir/extensions.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/r4/google_extensions.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/extensions.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "proto/stu3/resources.pb.h"
#include "testdata/stu3/profiles/test_extensions.pb.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/lib/core/status_test_util.h"

namespace google {
namespace fhir {

namespace {

using ::google::fhir::stu3::google::Base64BinarySeparatorStride;
using ::google::fhir::stu3::google::EventLabel;
using ::google::fhir::stu3::google::EventTrigger;
using ::google::fhir::stu3::google::PrimitiveHasNoValue;
using ::google::fhir::stu3::proto::
    CapabilityStatementSearchParameterCombination;
using ::google::fhir::stu3::proto::Composition;
using ::google::fhir::stu3::testing::DigitalMediaType;
using ::google::fhir::testutil::EqualsProto;

template <class T>
void ReadStu3TestData(const std::string& type, T* message,
                      stu3::proto::Extension* extension) {
  *message =
      ReadStu3Proto<T>(absl::StrCat("google/", type, ".message.prototxt"));
  *extension = ReadStu3Proto<stu3::proto::Extension>(
      absl::StrCat("google/", type, ".extension.prototxt"));
}

template <class T>
void ReadR4TestData(const std::string& type, T* message,
                    r4::core::Extension* extension) {
  *message = ReadR4Proto<T>(absl::StrCat("google/", type, ".message.prototxt"));
  *extension = ReadR4Proto<r4::core::Extension>(
      absl::StrCat("google/", type, ".extension.prototxt"));
}

template <class T>
void TestExtensionToMessage(const std::string& name) {
  T message;
  stu3::proto::Extension extension;
  ReadStu3TestData(name, &message, &extension);

  T output;
  TF_ASSERT_OK(ExtensionToMessage(extension, &output));
  EXPECT_THAT(output, EqualsProto(message));
}

template <class T>
void TestConvertToExtension(const std::string& name) {
  T message;
  stu3::proto::Extension extension;
  ReadStu3TestData(name, &message, &extension);

  stu3::proto::Extension output;
  TF_ASSERT_OK(ConvertToExtension(message, &output));
  EXPECT_THAT(output, EqualsProto(extension));
}

template <class T>
void TestExtensionToMessageR4(const std::string& name) {
  T message;
  r4::core::Extension extension;
  ReadR4TestData(name, &message, &extension);

  T output;
  TF_ASSERT_OK(ExtensionToMessage(extension, &output));
  EXPECT_THAT(output, EqualsProto(message));
}

template <class T>
void TestConvertToExtensionR4(const std::string& name) {
  T message;
  r4::core::Extension extension;
  ReadR4TestData(name, &message, &extension);

  r4::core::Extension output;
  TF_ASSERT_OK(ConvertToExtension(message, &output));
  EXPECT_THAT(output, EqualsProto(extension));
}

TEST(ExtensionsTest, ParseEventTrigger) {
  TestExtensionToMessage<EventTrigger>("trigger");
}

TEST(ExtensionsTest, PrintEventTrigger) {
  TestConvertToExtension<EventTrigger>("trigger");
}

TEST(ExtensionsTest, ParseEventLabel) {
  TestExtensionToMessage<EventLabel>("label");
}

TEST(ExtensionsTest, PrintEventLabel) {
  TestConvertToExtension<EventLabel>("label");
}

TEST(ExtensionsTest, ParsePrimitiveHasNoValue) {
  TestExtensionToMessage<PrimitiveHasNoValue>("primitive_has_no_value");
}

TEST(ExtensionsTest, PrintPrimitiveHasNoValue) {
  TestConvertToExtension<PrimitiveHasNoValue>("primitive_has_no_value");
}

TEST(ExtensionsTestR4, ParsePrimitiveHasNoValue) {
  TestExtensionToMessageR4<r4::google::PrimitiveHasNoValue>(
      "primitive_has_no_value");
}

TEST(ExtensionsTestR4, PrintPrimitiveHasNoValue) {
  TestConvertToExtensionR4<r4::google::PrimitiveHasNoValue>(
      "primitive_has_no_value");
}

TEST(ExtensionsTest, ParsePrimitiveHasNoValue_Empty) {
  TestExtensionToMessage<PrimitiveHasNoValue>("empty");
}

TEST(ExtensionsTest, PrintPrimitiveHasNoValue_Empty) {
  TestConvertToExtension<PrimitiveHasNoValue>("empty");
}

TEST(ExtensionsTest, ParseCapabilityStatementSearchParameterCombination) {
  TestExtensionToMessage<CapabilityStatementSearchParameterCombination>(
      "capability");
}

TEST(ExtensionsTest, PrintCapabilityStatementSearchParameterCombination) {
  TestConvertToExtension<CapabilityStatementSearchParameterCombination>(
      "capability");
}

TEST(ExtensionsTest, ParseBoundCodeExtension) {
  TestExtensionToMessage<DigitalMediaType>("digital_media_type");
}

TEST(ExtensionsTest, PrintBoundCodeExtension) {
  TestConvertToExtension<DigitalMediaType>("digital_media_type");
}

TEST(ExtensionsTest, ExtractOnlyMatchingExtensionOneFound) {
  Composition composition = PARSE_VALID_STU3_PROTO(R"pb(
    id { value: "1" }
    status { value: FINAL }
    subject { patient_id { value: "P0" } }
    encounter { encounter_id { value: "2" } }
    author { practitioner_id { value: "3" } }
    title { value: "Note" }
    type {
      coding {
        system { value: "type-system" }
        code { value: "RADIOLOGY" }
      }
    }
    date {
      value_us: 4608099660000000
      timezone: "America/New_York"
      precision: SECOND
    }
    extension {
      url { value: "https://g.co/fhir/StructureDefinition/random" }
      extension {
        url { value: "foo" }
        value { string_value { value: "barr" } }
      }
    }
    extension {
      url {
        value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride"
      }
      extension {
        url { value: "separator" }
        value { string_value { value: "!" } }
      }
      extension {
        url { value: "stride" }
        value { positive_int { value: 5 } }
      }
    }
    extension {
      url { value: "https://g.co/fhir/StructureDefinition/more-random" }
      extension {
        url { value: "baz" }
        value { string_value { value: "quux" } }
      }
    }
  )pb");

  StatusOr<Base64BinarySeparatorStride> extracted =
      ExtractOnlyMatchingExtension<Base64BinarySeparatorStride>(composition);

  Base64BinarySeparatorStride expected;
  ASSERT_TRUE(::google::protobuf::TextFormat::ParseFromString(R"pb(
                                                      separator { value: "!" }
                                                      stride { value: 5 }
                                                    )pb",
                                                    &expected));

  TF_ASSERT_OK(extracted.status());
  EXPECT_THAT(extracted.ValueOrDie(), EqualsProto(expected));
}

TEST(ExtensionsTest, ExtractOnlyMatchingExtensionNoneFound) {
  Composition composition = PARSE_VALID_STU3_PROTO(R"pb(
    id { value: "1" }
    status { value: FINAL }
    subject { patient_id { value: "P0" } }
    encounter { encounter_id { value: "2" } }
    author { practitioner_id { value: "3" } }
    title { value: "Note" }
    type {
      coding {
        system { value: "type-system" }
        code { value: "RADIOLOGY" }
      }
    }
    date {
      value_us: 4608099660000000
      timezone: "America/New_York"
      precision: SECOND
    }
    extension {
      url { value: "https://g.co/fhir/StructureDefinition/random" }
      extension {
        url { value: "foo" }
        value { string_value { value: "barr" } }
      }
    }
    extension {
      url { value: "https://g.co/fhir/StructureDefinition/more-random" }
      extension {
        url { value: "baz" }
        value { string_value { value: "quux" } }
      }
    }
  )pb");

  StatusOr<Base64BinarySeparatorStride> extracted =
      ExtractOnlyMatchingExtension<Base64BinarySeparatorStride>(composition);

  EXPECT_FALSE(extracted.status().ok());
  EXPECT_EQ(tensorflow::error::NOT_FOUND, extracted.status().code());
}

TEST(ExtensionsTest, ExtractOnlyMatchingExtensionMultipleFound) {
  Composition composition = PARSE_VALID_STU3_PROTO(R"pb(
    id { value: "1" }
    status { value: FINAL }
    subject { patient_id { value: "P0" } }
    encounter { encounter_id { value: "2" } }
    author { practitioner_id { value: "3" } }
    title { value: "Note" }
    type {
      coding {
        system { value: "type:system" }
        code { value: "RADIOLOGY" }
      }
    }
    date {
      value_us: 4608099660000000
      timezone: "America/New_York"
      precision: SECOND
    }
    extension {
      url {
        value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride"
      }
      extension {
        url { value: "separator" }
        value { string_value { value: "!" } }
      }
      extension {
        url { value: "stride" }
        value { positive_int { value: 5 } }
      }
    }
    extension {
      url {
        value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride"
      }
      extension {
        url { value: "separator" }
        value { string_value { value: "*" } }
      }
      extension {
        url { value: "stride" }
        value { positive_int { value: 6 } }
      }
    }
  )pb");

  StatusOr<Base64BinarySeparatorStride> extracted =
      ExtractOnlyMatchingExtension<Base64BinarySeparatorStride>(composition);

  EXPECT_FALSE(extracted.status().ok());
  EXPECT_EQ(tensorflow::error::INVALID_ARGUMENT, extracted.status().code());
}

}  // namespace

}  // namespace fhir
}  // namespace google
