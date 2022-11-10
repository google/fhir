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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/extensions.h"
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

namespace {

using ::google::fhir::r4::core::CapabilityStatementSearchParameterCombination;
using ::google::fhir::r4::core::Composition;
using ::google::fhir::r4::core::Extension;
using ::google::fhir::r4::fhirproto::Base64BinarySeparatorStride;
using ::google::fhir::r4::fhirproto::PrimitiveHasNoValue;
using ::google::fhir::r4::ml::EventLabel;
using ::google::fhir::r4::ml::EventTrigger;
using ::google::fhir::r4::testing::DigitalMediaType;
using ::google::fhir::testutil::EqualsProto;

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

TEST(ExtensionsR4Test, ConvertToProfiledEventTrigger) {
  TestExtensionToMessage<EventTrigger>("trigger");
}

TEST(ExtensionsR4Test, ConvertToUnprofiledEventTrigger) {
  TestConvertToExtension<EventTrigger>("trigger");
}

TEST(ExtensionsR4Test, ConvertToProfiledEventLabel) {
  TestExtensionToMessage<EventLabel>("label");
}

TEST(ExtensionsR4Test, ConvertToUnprofiledEventLabel) {
  TestConvertToExtension<EventLabel>("label");
}

TEST(ExtensionsR4Test, ConvertToProfiledPrimitiveHasNoValue) {
  TestExtensionToMessage<PrimitiveHasNoValue>("primitive_has_no_value");
}

TEST(ExtensionsR4Test, ConvertToUnprofiledPrimitiveHasNoValue) {
  TestConvertToExtension<PrimitiveHasNoValue>("primitive_has_no_value");
}

TEST(ExtensionsR4Test, ConvertToProfiledPrimitiveHasNoValue_Empty) {
  TestExtensionToMessage<PrimitiveHasNoValue>("empty");
}

TEST(ExtensionsR4Test, ConvertToUnprofiledPrimitiveHasNoValue_Empty) {
  TestConvertToExtension<PrimitiveHasNoValue>("empty");
}

TEST(ExtensionsR4Test,
     ConvertToProfiledCapabilityStatementSearchParameterCombination) {
  TestExtensionToMessage<CapabilityStatementSearchParameterCombination>(
      "capability");
}

TEST(ExtensionsR4Test,
     ConvertToUnprofiledCapabilityStatementSearchParameterCombination) {
  TestConvertToExtension<CapabilityStatementSearchParameterCombination>(
      "capability");
}

TEST(ExtensionsR4Test, ConvertToProfiledBoundCodeExtension) {
  TestExtensionToMessage<DigitalMediaType>("digital_media_type");
}

TEST(ExtensionsR4Test, ConvertToUnprofiledBoundCodeExtension) {
  TestConvertToExtension<DigitalMediaType>("digital_media_type");
}

TEST(ExtensionsR4Test, ConvertToProfiledSingleValueComplexExtension) {
  TestExtensionToMessage<r4::testing::SingleValueComplexExtension>(
      "single_value_complex");
}

TEST(ExtensionsR4Test, ConvertToUnprofiledSingleValueComplexExtension) {
  TestConvertToExtension<r4::testing::SingleValueComplexExtension>(
      "single_value_complex");
}

TEST(ExtensionsR4Test, ExtractOnlyMatchingExtensionOneFound) {
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

TEST(ExtensionsR4Test, ExtractOnlyMatchingExtensionNoneFound) {
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

TEST(ExtensionsR4Test, ExtractOnlyMatchingExtensionMultipleFound) {
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

TEST(ExtensionsR4Test, IsSimpleExtensionTrue) {
  EXPECT_TRUE(IsSimpleExtension(r4::testing::SimpleDecimalExt::descriptor()));
}

TEST(ExtensionsR4Test, IsSimpleExtensionFalseManyFields) {
  EXPECT_FALSE(IsSimpleExtension(r4::testing::ComplexExt::descriptor()));
}

TEST(ExtensionsR4Test, IsSimpleExtensionFalseOneFieldWithComplexAnnotation) {
  EXPECT_FALSE(IsSimpleExtension(
      r4::testing::SingleValueComplexExtension::descriptor()));
}

TEST(ExtensionsR4Test, IsSimpleExtensionFalseOneRepeatedField) {
  EXPECT_FALSE(IsSimpleExtension(
      r4::testing::SingleValueRepeatedComplexExtension::descriptor()));
}

}  // namespace

}  // namespace fhir
}  // namespace google
