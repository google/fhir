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

#include "google/fhir/stu3/extensions.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/stu3/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "tensorflow/core/lib/core/status.h"

namespace google {
namespace fhir {
namespace stu3 {

namespace {

using ::google::fhir::stu3::google::EventTrigger;
using ::google::fhir::stu3::google::PrimitiveHasNoValue;
using ::google::fhir::stu3::proto::Extension;
using ::google::fhir::testutil::EqualsProto;

template <class T>
void ReadTestData(const string& type, T* message, Extension* extension) {
  *message = ReadProto<T>(absl::StrCat("google/", type, ".message.prototxt"));
  *extension = ReadProto<Extension>(
      absl::StrCat("google/", type, ".extension.prototxt"));
}

TEST(ExtensionsTest, ParseEventTrigger) {
  EventTrigger message;
  Extension extension;
  ReadTestData("trigger", &message, &extension);

  EventTrigger output;
  ASSERT_TRUE(ExtensionToMessage(extension, &output).ok());
  EXPECT_THAT(output, EqualsProto(message));
}

TEST(ExtensionsTest, PrintEventTrigger) {
  EventTrigger message;
  Extension extension;
  ReadTestData("trigger", &message, &extension);

  Extension output;
  ASSERT_TRUE(ConvertToExtension(message, &output).ok());
  EXPECT_THAT(output, EqualsProto(extension));
}

TEST(ExtensionsTest, ParsePrimitiveHasNoValue) {
  PrimitiveHasNoValue message;
  Extension extension;
  ReadTestData("primitive_has_no_value", &message, &extension);

  PrimitiveHasNoValue output;
  ASSERT_TRUE(ExtensionToMessage(extension, &output).ok());
  EXPECT_THAT(output, EqualsProto(message));
}

TEST(ExtensionsTest, ParsePrimitiveHasNoValue_Empty) {
  PrimitiveHasNoValue message;
  Extension extension;
  ReadTestData("empty", &message, &extension);

  PrimitiveHasNoValue output;
  ASSERT_TRUE(ExtensionToMessage(extension, &output).ok());
  EXPECT_THAT(output, EqualsProto(message));
}

TEST(ExtensionsTest, PrintPrimitiveHasNoValue) {
  PrimitiveHasNoValue message;
  Extension extension;
  ReadTestData("primitive_has_no_value", &message, &extension);

  Extension output;
  TF_CHECK_OK(ConvertToExtension(message, &output));
  EXPECT_THAT(output, EqualsProto(extension));
}

TEST(ExtensionsTest, PrintPrimitiveHasNoValue_Empty) {
  PrimitiveHasNoValue message;
  Extension extension;
  ReadTestData("empty", &message, &extension);

  Extension output;
  TF_CHECK_OK(ConvertToExtension(message, &output));
  EXPECT_THAT(output, EqualsProto(extension));
}

}  // namespace

}  // namespace stu3
}  // namespace fhir
}  // namespace google
