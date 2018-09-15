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

#include "google/fhir/stu3/profiles.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/stu3/test_helper.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "proto/stu3/profiles.pb.h"
#include "proto/stu3/resources.pb.h"

namespace google {
namespace fhir {
namespace stu3 {

namespace {
using ::google::fhir::testutil::EqualsProto;

template <class B, class P>
void TestProfile(const string& filename) {
  B unprofiled = ReadProto<B>(absl::StrCat(filename, ".prototxt"));
  P profiled;

  auto status = ConvertToProfile(unprofiled, &profiled);
  if (!status.ok()) {
    LOG(ERROR) << status.error_message();
    EXPECT_TRUE(status.ok());
  }
  EXPECT_THAT(
      profiled,
      EqualsProto(ReadProto<P>(absl::StrCat(filename, "-profiled.prototxt"))));
}

TEST(ProfilesTest, SimpleExtensions) {
  TestProfile<proto::Observation, proto::ObservationGenetics>(
      "examples/observation-example-genetics-1");
}

}  // namespace

}  // namespace stu3
}  // namespace fhir
}  // namespace google
