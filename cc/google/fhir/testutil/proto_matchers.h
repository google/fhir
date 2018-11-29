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

namespace google {
namespace fhir {
namespace testutil {

MATCHER_P(EqualsProto, other, "") {
  return ::google::protobuf::util::MessageDifferencer::Equals(arg, other);
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
