// Copyright 2020 Google LLC
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

#ifndef GOOGLE_FHIR_FHIR_PATH_UTILS_H_
#define GOOGLE_FHIR_FHIR_PATH_UTILS_H_

#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"
#include "google/fhir/status/status.h"

namespace google {
namespace fhir {
namespace fhir_path {
namespace internal {

// Retrieves a field from a FHIR proto and places the resulting message(s) in
// results vector.
//
// This function abstracts away FHIR proto implementation details. For example,
// if one wanted to retrieve the value of a value[X] field or a contained
// resource there is an intermediate message that acts solely as a container for
// the resource or value. This method handles traversing over these abstractions
// and returning the desired message.
//
// NOTE: The messages in the results vector will no longer be accessible when
// the root message is deleted.
absl::Status RetrieveField(const google::protobuf::Message& root,
                           const google::protobuf::FieldDescriptor& field,
                           std::vector<const google::protobuf::Message*>* results);

// Returns true if the message descriptor contains a field whose JSON name
// matches the provided json_name. In the case that the descriptor describes a
// proto wrapper used for ValueX or contained resources, a search for a matching
// field is preformed within the possible values/resources that the proto wraps.
// Returns false if a matching field is not found.
bool HasFieldWithJsonName(const google::protobuf::Descriptor* descriptor,
                          absl::string_view json_name);

// Finds a field in the message descriptor whose JSON name matches the provided
// name or nullptr if one is not found.
//
// Neither Descriptor::FindFieldByName or Descriptor::FindFieldByCamelcaseName
// will suffice as some FHIR fields are renamed in the FHIR protos (e.g.
// "assert" becomes "assert_value" and "class" becomes "class_value").
const google::protobuf::FieldDescriptor* FindFieldByJsonName(
    const google::protobuf::Descriptor* descriptor, absl::string_view json_name);

}  // namespace internal
}  // namespace fhir_path
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_FHIR_PATH_UTILS_H_
