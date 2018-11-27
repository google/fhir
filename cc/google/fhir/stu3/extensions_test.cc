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
#include "proto/stu3/extensions.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/lib/core/status_test_util.h"

namespace google {
namespace fhir {
namespace stu3 {

namespace {

using ::google::fhir::stu3::google::EventLabel;
using ::google::fhir::stu3::google::EventTrigger;
using ::google::fhir::stu3::google::PrimitiveHasNoValue;
using ::google::fhir::stu3::proto::
    CapabilityStatementSearchParameterCombination;
using ::google::fhir::stu3::proto::Extension;
using ::google::fhir::testutil::EqualsProto;

template <class T>
void ReadTestData(const string& type, T* message, Extension* extension) {
  *message = ReadProto<T>(absl::StrCat("google/", type, ".message.prototxt"));
  *extension = ReadProto<Extension>(
      absl::StrCat("google/", type, ".extension.prototxt"));
}

template <class T>
void TestExtension(const string& name) {
  T message;
  Extension extension;
  ReadTestData(name, &message, &extension);

  T output;
  TF_ASSERT_OK(ExtensionToMessage(extension, &output));
  EXPECT_THAT(output, EqualsProto(message));
}

TEST(ExtensionsTest, ParseEventTrigger) {
  TestExtension<EventTrigger>("trigger");
}

TEST(ExtensionsTest, PrintEventTrigger) {
  TestExtension<EventTrigger>("trigger");
}

TEST(ExtensionsTest, ParseEventLabel) { TestExtension<EventLabel>("label"); }

TEST(ExtensionsTest, PrintEventLabel) { TestExtension<EventLabel>("label"); }

TEST(ExtensionsTest, ParsePrimitiveHasNoValue) {
  TestExtension<PrimitiveHasNoValue>("primitive_has_no_value");
}

TEST(ExtensionsTest, ParsePrimitiveHasNoValue_Empty) {
  TestExtension<PrimitiveHasNoValue>("empty");
}

TEST(ExtensionsTest, PrintPrimitiveHasNoValue) {
  TestExtension<PrimitiveHasNoValue>("primitive_has_no_value");
}

TEST(ExtensionsTest, PrintPrimitiveHasNoValue_Empty) {
  TestExtension<PrimitiveHasNoValue>("empty");
}

TEST(ExtensionsTest, CapabilityStatementSearchParameterCombination) {
  TestExtension<CapabilityStatementSearchParameterCombination>("capability");
}

}  // namespace

}  // namespace stu3
}  // namespace fhir
}  // namespace google
