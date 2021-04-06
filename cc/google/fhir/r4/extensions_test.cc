/*
 * Copyright 2020 Google LLC
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

#include "google/fhir/r4/extensions.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/extensions.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/composition.pb.h"
#include "proto/google/fhir/proto/r4/fhirproto_extensions.pb.h"
#include "proto/google/fhir/proto/r4/ml_extensions.pb.h"
#include "testdata/r4/profiles/test_extensions.pb.h"

namespace google {
namespace fhir {
namespace r4 {

namespace {

using core::CapabilityStatementSearchParameterCombination;
using core::Composition;
using core::Extension;
using fhirproto::Base64BinarySeparatorStride;
using fhirproto::PrimitiveHasNoValue;
using ::google::fhir::testutil::EqualsProto;
using ml::EventLabel;
using ml::EventTrigger;
using testing::DigitalMediaType;

template <class T>
void ReadR4TestData(const std::string& type, T* message, Extension* extension) {
  *message =
      ReadR4Proto<T>(absl::StrCat("extensions/", type, ".message.prototxt"));
  *extension = ReadR4Proto<Extension>(
      absl::StrCat("extensions/", type, ".extension.prototxt"));
}

template <class T>
void TestExtensionToMessage(const std::string& name) {
  T message;
  Extension extension;
  ReadR4TestData(name, &message, &extension);

  T output;
  FHIR_ASSERT_OK(ExtensionToMessage(extension, &output));
  EXPECT_THAT(output, EqualsProto(message));
}

template <class T>
void TestConvertToExtension(const std::string& name) {
  T message;
  Extension extension;
  ReadR4TestData(name, &message, &extension);

  Extension output;
  FHIR_ASSERT_OK(ConvertToExtension(message, &output));
  EXPECT_THAT(output, EqualsProto(extension));
}

TEST(ExtensionsTest, ConvertToProfiledEventTrigger) {
  TestExtensionToMessage<EventTrigger>("trigger");
}

TEST(ExtensionsTest, ConvertToUnprofiledEventTrigger) {
  TestConvertToExtension<EventTrigger>("trigger");
}

TEST(ExtensionsTest, ConvertToProfiledEventLabel) {
  TestExtensionToMessage<EventLabel>("label");
}

TEST(ExtensionsTest, ConvertToUnprofiledEventLabel) {
  TestConvertToExtension<EventLabel>("label");
}

TEST(ExtensionsTest, ConvertToProfiledPrimitiveHasNoValue) {
  TestExtensionToMessage<PrimitiveHasNoValue>("primitive_has_no_value");
}

TEST(ExtensionsTest, ConvertToUnprofiledPrimitiveHasNoValue) {
  TestConvertToExtension<PrimitiveHasNoValue>("primitive_has_no_value");
}

TEST(ExtensionsTest, ConvertToProfiledPrimitiveHasNoValue_Empty) {
  TestExtensionToMessage<PrimitiveHasNoValue>("empty");
}

TEST(ExtensionsTest, ConvertToUnprofiledPrimitiveHasNoValue_Empty) {
  TestConvertToExtension<PrimitiveHasNoValue>("empty");
}

TEST(ExtensionsTest,
     ConvertToProfiledCapabilityStatementSearchParameterCombination) {
  TestExtensionToMessage<CapabilityStatementSearchParameterCombination>(
      "capability");
}

TEST(ExtensionsTest,
     ConvertToUnprofiledCapabilityStatementSearchParameterCombination) {
  TestConvertToExtension<CapabilityStatementSearchParameterCombination>(
      "capability");
}

TEST(ExtensionsTest, ConvertToProfiledBoundCodeExtension) {
  TestExtensionToMessage<DigitalMediaType>("digital_media_type");
}

TEST(ExtensionsTest, ConvertToUnprofiledBoundCodeExtension) {
  TestConvertToExtension<DigitalMediaType>("digital_media_type");
}

TEST(ExtensionsTest, ConvertToProfiledSingleValueComplexExtension) {
  TestExtensionToMessage<testing::SingleValueComplexExtension>(
      "single_value_complex");
}

TEST(ExtensionsTest, ConvertToUnprofiledSingleValueComplexExtension) {
  TestConvertToExtension<testing::SingleValueComplexExtension>(
      "single_value_complex");
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

  absl::StatusOr<Base64BinarySeparatorStride> extracted =
      ExtractOnlyMatchingExtension<Base64BinarySeparatorStride>(composition);

  Base64BinarySeparatorStride expected;
  ASSERT_TRUE(::google::protobuf::TextFormat::ParseFromString(R"pb(
                                                      separator { value: "!" }
                                                      stride { value: 5 }
                                                    )pb",
                                                    &expected));

  FHIR_ASSERT_OK(extracted.status());
  EXPECT_THAT(extracted.value(), EqualsProto(expected));
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

  absl::StatusOr<Base64BinarySeparatorStride> extracted =
      ExtractOnlyMatchingExtension<Base64BinarySeparatorStride>(composition);

  EXPECT_FALSE(extracted.status().ok());
  EXPECT_EQ(absl::StatusCode::kNotFound, extracted.status().code());
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

  absl::StatusOr<Base64BinarySeparatorStride> extracted =
      ExtractOnlyMatchingExtension<Base64BinarySeparatorStride>(composition);

  EXPECT_FALSE(extracted.status().ok());
  EXPECT_EQ(absl::StatusCode::kInvalidArgument, extracted.status().code());
}

TEST(ExtensionsTest, IsSimpleExtensionTrue) {
  EXPECT_TRUE(IsSimpleExtension(r4::testing::SimpleDecimalExt::descriptor()));
}

TEST(ExtensionsTest, IsSimpleExtensionFalseManyFields) {
  EXPECT_FALSE(IsSimpleExtension(r4::testing::ComplexExt::descriptor()));
}

TEST(ExtensionsTest, IsSimpleExtensionFalseOneFieldWithComplexAnnotation) {
  EXPECT_FALSE(IsSimpleExtension(
      r4::testing::SingleValueComplexExtension::descriptor()));
}

TEST(ExtensionsTest, IsSimpleExtensionFalseOneRepeatedField) {
  EXPECT_FALSE(IsSimpleExtension(
      r4::testing::SingleValueRepeatedComplexExtension::descriptor()));
}

}  // namespace

}  // namespace r4
}  // namespace fhir
}  // namespace google
