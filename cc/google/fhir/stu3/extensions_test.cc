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
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/primitive_has_no_value.pb.h"
#include "tensorflow/core/platform/env.h"


namespace google {
namespace fhir {
namespace stu3 {

namespace {
using ::google::fhir::stu3::proto::Extension;
using ::google::fhir::stu3::proto::PrimitiveHasNoValue;
using ::google::fhir::testutil::EqualsProto;

class ExtensionsTest : public ::testing::Test {
 private:
  template <class T>
  T ReadProto(const string& filename) {
    T result;
    TF_CHECK_OK(::tensorflow::ReadTextProto(
        tensorflow::Env::Default(),
        "testdata/stu3/extensions/" + filename,
        &result));
    return result;
  }

 protected:
  template <class T>
  void ReadTestData(const string& type, T* message, Extension* extension) {
    *message = ReadProto<T>(absl::StrCat(type, ".message.prototxt"));
    *extension =
        ReadProto<Extension>(absl::StrCat(type, ".extension.prototxt"));
  }
};

TEST_F(ExtensionsTest, ParsePrimitiveHasNoValue) {
  PrimitiveHasNoValue message;
  Extension extension;
  ReadTestData("primitive_has_no_value", &message, &extension);

  PrimitiveHasNoValue output;
  ASSERT_TRUE(ExtensionToMessage(extension, &output).ok());
  EXPECT_THAT(output, EqualsProto(message));
}

TEST_F(ExtensionsTest, ParsePrimitiveHasNoValue_Empty) {
  PrimitiveHasNoValue message;
  Extension extension;
  ReadTestData("empty", &message, &extension);

  PrimitiveHasNoValue output;
  ASSERT_TRUE(ExtensionToMessage(extension, &output).ok());
  EXPECT_THAT(output, EqualsProto(message));
}

TEST_F(ExtensionsTest, PrintPrimitiveHasNoValue) {
  PrimitiveHasNoValue message;
  Extension extension;
  ReadTestData("primitive_has_no_value", &message, &extension);

  Extension output;
  ASSERT_TRUE(ConvertToExtension(message, &output).ok());
  EXPECT_THAT(output, EqualsProto(extension));
}

TEST_F(ExtensionsTest, PrintPrimitiveHasNoValue_Empty) {
  PrimitiveHasNoValue message;
  Extension extension;
  ReadTestData("empty", &message, &extension);

  Extension output;
  ASSERT_TRUE(ConvertToExtension(message, &output).ok());
  EXPECT_THAT(output, EqualsProto(extension));
}

}  // namespace

}  // namespace stu3
}  // namespace fhir
}  // namespace google
