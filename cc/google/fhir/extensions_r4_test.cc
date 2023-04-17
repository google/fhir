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

#include <vector>

#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "google/fhir/extensions.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/binary.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/composition.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/patient.pb.h"
#include "proto/google/fhir/proto/stu3/datatypes.pb.h"

namespace google {
namespace fhir {

namespace {

using ::google::fhir::r4::core::Composition;
using ::google::fhir::r4::core::Extension;
using ::google::fhir::testutil::EqualsProto;

TEST(ExtensionsR4Test, GetOnlyMatchingExtensionSucceeds) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "random_url" }
          value { string_value { value: "*" } }
        }
        extension {
          url { value: "test_url" }
          value { string_value { value: "!" } }
        }
      )pb",
      &composition));

  auto result = GetOnlyMatchingExtension<Extension>("test_url", composition);

  Extension expected;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        url { value: "test_url" }
        value { string_value { value: "!" } }
      )pb",
      &expected));

  EXPECT_TRUE(result.ok());
  EXPECT_THAT(result.value(), EqualsProto(expected));
}

TEST(ExtensionsR4Test, GetAllMatchingExtensionsSucceeds) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "test_url" }
          value { string_value { value: "%" } }
        }
        extension {
          url { value: "random_url" }
          value { string_value { value: "*" } }
        }
        extension {
          url { value: "test_url" }
          value { string_value { value: "!" } }
        }
      )pb",
      &composition));

  auto result = GetAllMatchingExtensions<Extension>("test_url", composition);

  EXPECT_TRUE(result.ok());
  EXPECT_THAT(*result, testing::ElementsAre(&composition.extension(0),
                                            &composition.extension(2)));
}

TEST(
    ExtensionsR4Test,
    GetAllMatchingExtensionsReturnsEmptyArrayForElementsWithoutExtensionField) {
  r4::core::Binary binary;

  auto result = GetAllMatchingExtensions<Extension>("test_url", binary);

  EXPECT_TRUE(result.ok());
  EXPECT_THAT(*result, testing::ElementsAre());
}

TEST(ExtensionsR4Test, GetAllMatchingExtensionsFailsWithWrongType) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "test_url" }
          value { string_value { value: "%" } }
        }
        extension {
          url { value: "random_url" }
          value { string_value { value: "*" } }
        }
        extension {
          url { value: "test_url" }
          value { string_value { value: "!" } }
        }
      )pb",
      &composition));

  // This requests with incorrect extension type template - the proto contains
  // R4 extensions, not STU3.
  auto result =
      GetAllMatchingExtensions<stu3::proto::Extension>("test_url", composition);

  EXPECT_FALSE(result.ok());
}

TEST(ExtensionsR4Test,
     GetOnlyMatchingExtensionReturnsNullptrForElementsWithoutExtensionField) {
  r4::core::Binary binary;

  auto result = GetOnlyMatchingExtension<Extension>("test_url", binary);

  EXPECT_TRUE(result.ok());
  EXPECT_THAT(result.value(), nullptr);
}

TEST(ExtensionsR4Test, GetOnlyMatchingExtensionFailsIfMultipleFound) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "test_url" }
          value { string_value { value: "!" } }
        }
        extension {
          url { value: "random_url" }
          value { string_value { value: "*" } }
        }
        extension {
          url { value: "test_url" }
          value { string_value { value: "?" } }
        }
      )pb",
      &composition));

  auto result = GetOnlyMatchingExtension<Extension>("test_url", composition);

  EXPECT_FALSE(result.ok());
}

TEST(ExtensionsR4Test, GetOnlyMatchingExtensionReturnsNullptrIfMissing) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "random_url" }
          value { string_value { value: "*" } }
        }
      )pb",
      &composition));

  auto result = GetOnlyMatchingExtension<Extension>("test_url", composition);

  EXPECT_TRUE(result.ok());
  EXPECT_EQ(result.value(), nullptr);
}

TEST(ExtensionsR4Test, GetOnlySimpleExtensionValueSucceedsWithPrimitive) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "random_url" }
          value { string_value { value: "*" } }
        }
        extension {
          url { value: "test_url" }
          value { string_value { value: "!" } }
        }
      )pb",
      &composition));

  auto result = GetOnlySimpleExtensionValue<google::fhir::r4::core::String>(
      "test_url", composition);

  google::fhir::r4::core::String expected;
  ASSERT_TRUE(
      google::protobuf::TextFormat::ParseFromString(R"pb(value: "!")pb", &expected));

  EXPECT_TRUE(result.ok());
  EXPECT_THAT(result.value(), EqualsProto(expected));
}

TEST(ExtensionsR4Test, GetOnlySimpleExtensionValueFailsIfMultipleFound) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "test_url" }
          value { string_value { value: "?" } }
        }
        extension {
          url { value: "random_url" }
          value { string_value { value: "*" } }
        }
        extension {
          url { value: "test_url" }
          value { string_value { value: "!" } }
        }
      )pb",
      &composition));

  auto result = GetOnlySimpleExtensionValue<google::fhir::r4::core::String>(
      "test_url", composition);

  EXPECT_FALSE(result.ok());
}

TEST(ExtensionsR4Test, GetOnlySimpleExtensionValueSucceedsWithDatatype) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "test_url" }
          value {
            coding {
              system { value: "asdf" }
              code { value: "!" }
            }
          }
        }
      )pb",
      &composition));

  auto result = GetOnlySimpleExtensionValue<google::fhir::r4::core::Coding>(
      "test_url", composition);
  EXPECT_TRUE(result.ok());
  EXPECT_THAT(result.value(), EqualsProto(R"pb(
                system { value: "asdf" }
                code { value: "!" }
              )pb"));
}

TEST(ExtensionsR4Test, GetOnlySimpleExtensionValueWrongDatatypeFails) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "test_url" }
          value { string_value { value: "!" } }
        }
      )pb",
      &composition));

  auto result = GetOnlySimpleExtensionValue<google::fhir::r4::core::Coding>(
      "test_url", composition);

  EXPECT_FALSE(result.ok());
}

TEST(ExtensionsR4Test, GetOnlySimpleExtensionValueNotFoundReturnsNullptr) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "garbage" }
          value { string_value { value: "!" } }
        }
      )pb",
      &composition));

  auto result = GetOnlySimpleExtensionValue<google::fhir::r4::core::Coding>(
      "test_url", composition);

  EXPECT_TRUE(result.ok());
  EXPECT_EQ(result.value(), nullptr);
}

TEST(ExtensionsR4Test, GetOnlySimpleExtensionValueFailsForComplexExtension) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "test_url" }
          extension {
            url { value: "separator" }
            value { string_value { value: "*" } }
          }
          extension {
            url { value: "stride" }
            value { positive_int { value: 6 } }
          }
        }
      )pb",
      &composition));

  auto result = GetOnlySimpleExtensionValue<google::fhir::r4::core::Coding>(
      "test_url", composition);

  EXPECT_FALSE(result.ok());
}

TEST(ExtensionsR4Test, GetAllSimpleExtensionValuesSucceedsWithPrimitive) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "test_url" }
          value { string_value { value: "+" } }
        }
        extension {
          url { value: "random_url" }
          value { string_value { value: "*" } }
        }
        extension {
          url { value: "test_url" }
          value { string_value { value: "!" } }
        }
      )pb",
      &composition));

  auto result = GetAllSimpleExtensionValues<google::fhir::r4::core::String>(
      "test_url", composition);

  EXPECT_TRUE(result.ok());
  EXPECT_THAT(result.value(), ElementsAre(EqualsProto(R"pb(value: "+")pb"),
                                          EqualsProto(R"pb(value: "!")pb")));
}

TEST(ExtensionsR4Test, GetAllSimpleExtensionValuesSucceedsWithDatatypes) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "test_url" }
          value {
            coding {
              system { value: "asdf" }
              code { value: "!" }
            }
          }
        }
      )pb",
      &composition));

  auto result = GetAllSimpleExtensionValues<google::fhir::r4::core::Coding>(
      "test_url", composition);

  EXPECT_TRUE(result.ok());
  EXPECT_THAT(result.value(), ElementsAre(EqualsProto(R"pb(
                system { value: "asdf" }
                code { value: "!" }
              )pb")));
}

TEST(ExtensionsR4Test, GetAllSimpleExtensionValuesWrongDatatypeFails) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "test_url" }
          value { string_value { value: "!" } }
        },
        extension {
          url { value: "test_url" }
          value { code { value: "!" } }
        }
      )pb",
      &composition));

  auto result = GetAllSimpleExtensionValues<google::fhir::r4::core::String>(
      "test_url", composition);

  EXPECT_FALSE(result.ok());
}

TEST(ExtensionsR4Test, GetAllSimpleExtensionValuesNotFoundReturnsNullptrs) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "garbage" }
          value { string_value { value: "!" } }
        }
      )pb",
      &composition));

  auto result = GetAllSimpleExtensionValues<google::fhir::r4::core::Coding>(
      "test_url", composition);

  EXPECT_TRUE(result.ok());
  EXPECT_THAT(result.value(), testing::IsEmpty());
}

TEST(ExtensionsR4Test, GetAllSimpleExtensionValuesForComplexExtensionsFails) {
  Composition composition;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      R"pb(
        extension {
          url { value: "test_url" }
          extension {
            url { value: "separator" }
            value { string_value { value: "*" } }
          }
          extension {
            url { value: "stride" }
            value { positive_int { value: 6 } }
          }
        }
      )pb",
      &composition));

  auto result = GetAllSimpleExtensionValues<google::fhir::r4::core::Coding>(
      "test_url", composition);

  EXPECT_FALSE(result.ok());
}

TEST(ExtensionsR4Test, AddExtensionSucceedsOnComposition) {
  Composition composition;
  absl::StatusOr<Extension*> extension = AddExtension<Extension>(&composition);
  EXPECT_TRUE(extension.ok());

  (*extension)->mutable_url()->set_value("test_url");

  EXPECT_THAT(composition, EqualsProto(R"pb(
                extension { url { value: "test_url" } }
              )pb"));
}

TEST(ExtensionsR4Test, AddExtensionFailsOnMessageWithoutExtensions) {
  r4::core::ContainedResource contained;
  absl::StatusOr<Extension*> extension = AddExtension<Extension>(&contained);
  EXPECT_FALSE(extension.ok());
}

TEST(ExtensionsR4Test, AddExtensionFailsWithWrongExtensionType) {
  Composition composition;
  absl::StatusOr<stu3::proto::Extension*> extension =
      AddExtension<stu3::proto::Extension>(&composition);
  EXPECT_FALSE(extension.ok());
}

}  // namespace

}  // namespace fhir
}  // namespace google
