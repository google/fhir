// Copyright 2019 Google LLC
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

#include "google/fhir/testutil/proto_matchers.h"

#include "google/fhir/proto_util.h"

namespace google {
namespace fhir {
namespace testutil {

inline google::protobuf::Message* CloneProto2(const google::protobuf::Message& src) {
  google::protobuf::Message* clone = src.New();
  clone->CopyFrom(src);
  return clone;
}

EqualsProtoMatcher::EqualsProtoMatcher(const google::protobuf::Message& expected)
    : expected_(CloneProto2(expected)) {}

bool EqualsProtoMatcher::MatchAndExplain(
    const google::protobuf::Message& m,
    testing::MatchResultListener* /* listener */) const {
  return google::fhir::AreSameMessageType(*expected_, m) &&
         ::google::protobuf::util::MessageDifferencer::Equals(*expected_, m);
}

bool EqualsProtoMatcher::MatchAndExplain(
    const google::protobuf::Message* m,
    testing::MatchResultListener* /* listener */) const {
  return google::fhir::AreSameMessageType(*expected_, *m) &&
         ::google::protobuf::util::MessageDifferencer::Equals(*expected_, *m);
}

void EqualsProtoMatcher::DescribeTo(::std::ostream* os) const {
  *os << "is equal to " << expected_->GetTypeName() << " <"
      << expected_->DebugString() << ">";
}

void EqualsProtoMatcher::DescribeNegationTo(::std::ostream* os) const {
  *os << "is NOT equal to " << expected_->GetTypeName() << " <"
      << expected_->DebugString() << ">";
}

}  // namespace testutil
}  // namespace fhir
}  // namespace google
