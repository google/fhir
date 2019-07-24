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

#include "google/fhir/profiles.h"

#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codes.h"
#include "google/fhir/extensions.h"
#include "google/fhir/profiles_lib.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/resource_validation.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "proto/r4/resources.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/logging.h"

namespace google {
namespace fhir {

Status ConvertToProfileR4(const ::google::protobuf::Message& source,
                          ::google::protobuf::Message* target) {
  return profiles_internal::ConvertToProfileInternal<r4::proto::Bundle>(source,
                                                                        target);
}

// Identical to ConvertToProfile, except does not run the validation step.
Status ConvertToProfileLenientR4(const ::google::protobuf::Message& source,
                                 ::google::protobuf::Message* target) {
  return profiles_internal::ConvertToProfileLenientInternal<r4::proto::Bundle>(
      source, target);
}

}  // namespace fhir
}  // namespace google
