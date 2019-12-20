/*
 * Copyright 2018 Google LLC
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

#ifndef GOOGLE_FHIR_TESTUTIL_PROTO_MATCHERS_H_
#define GOOGLE_FHIR_TESTUTIL_PROTO_MATCHERS_H_

#include "google/protobuf/util/message_differencer.h"
#include "gmock/gmock.h"
#include "google/fhir/proto_util.h"

namespace google {
namespace fhir {
namespace testutil {

// Matcher that compares google::protobuf::Message objects for equality.
class EqualsProtoMatcher {
 public:
  EqualsProtoMatcher(const google::protobuf::Message& expected);

  bool MatchAndExplain(const google::protobuf::Message& m,
                       testing::MatchResultListener* /* listener */) const;

  bool MatchAndExplain(const google::protobuf::Message* m,
                       testing::MatchResultListener* /* listener */) const;

  void DescribeTo(::std::ostream* os) const;

  void DescribeNegationTo(::std::ostream* os) const;

 private:
  const std::shared_ptr<google::protobuf::Message> expected_;
};

inline testing::PolymorphicMatcher<EqualsProtoMatcher> EqualsProto(
    const google::protobuf::Message& expected) {
  return testing::MakePolymorphicMatcher(EqualsProtoMatcher(expected));
}

MATCHER_P(EqualsProtoIgnoringReordering, other, "") {
  ::google::protobuf::util::MessageDifferencer differencer;
  differencer.set_repeated_field_comparison(
      ::google::protobuf::util::MessageDifferencer::AS_SET);
  return differencer.Compare(arg, other);
}

}  // namespace testutil
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_TESTUTIL_PROTO_MATCHERS_H_
