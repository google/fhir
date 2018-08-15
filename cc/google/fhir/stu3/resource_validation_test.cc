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

#include "google/fhir/stu3/resource_validation.h"
#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace stu3 {

namespace {

using stu3::proto::Bundle;

TEST(BundleValidationTest, MissingRequiredField) {
  google::protobuf::TextFormat::Parser parser;
  parser.AllowPartialMessage(true);
  Bundle bundle;
  ASSERT_TRUE(parser.ParseFromString(R"()", &bundle));

  EXPECT_EQ(ValidateResource(bundle),
            ::tensorflow::errors::InvalidArgument("missing-type"));
}

TEST(BundleValidationTest, Valid) {
  google::protobuf::TextFormat::Parser parser;
  parser.AllowPartialMessage(true);
  Bundle bundle;
  ASSERT_TRUE(parser.ParseFromString(R"proto(
    type { value: COLLECTION }
    id { value: "123" }
    entry { resource { patient {} } }
  )proto", &bundle));

  EXPECT_TRUE(ValidateResource(bundle).ok());
}

}  // namespace

}  // namespace stu3
}  // namespace fhir
}  // namespace google
