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

#ifndef GOOGLE_FHIR_STU3_PROTO_UTIL_H_
#define GOOGLE_FHIR_STU3_PROTO_UTIL_H_

#include <string>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;

// Finds a message subfield at the path specified by a string in the format
// used by a ResourceConfig (camel-case, prefixed by Resource type), e.g.:
// MedicationRequest.dispenseRequest.validityPeriod.start
//
// Mutable versions return mutable results, and add default protos if any steps
// are unset.
//
// Const versions return const results, and return NOT_FOUND status if any
// submessage along the way are empty.
//
// All versions return an INVALID_ARGUMENT status if the field_path does not
// resolve to a message
StatusOr<google::protobuf::Message*> GetMutableSubmessageByPath(google::protobuf::Message* message,
                                                      const string& field_path);

StatusOr<const google::protobuf::Message*> GetSubmessageByPath(
    const google::protobuf::Message& message, const string& field_path);

// Typed variants that return the requested type, or INVALID_ARGUMENT if
// the located message is not of the expected type.
template <typename T>
StatusOr<T*> GetMutableSubmessageByPathAndCheckType(google::protobuf::Message* message,
                                                    const string& field_path) {
  const string& message_name = message->GetDescriptor()->name();
  auto got = GetMutableSubmessageByPath(message, field_path);
  TF_RETURN_IF_ERROR(got.status());
  google::protobuf::Message* submessage = got.ValueOrDie();
  if (T::descriptor()->full_name() !=
      submessage->GetDescriptor()->full_name()) {
    return ::tensorflow::errors::InvalidArgument(
        ::absl::StrCat("Cannot find ", field_path, " in ", message_name,
                       " of type ", T::descriptor()->full_name(),
                       ".  Found: ", submessage->GetDescriptor()->full_name()));
  }

  return dynamic_cast<T*>(submessage);
}

template <typename T>
StatusOr<const T*> GetSubmessageByPathAndCheckType(
    const google::protobuf::Message& message, const string& field_path) {
  const string& message_name = message.GetDescriptor()->name();
  auto got = GetSubmessageByPath(message, field_path);
  TF_RETURN_IF_ERROR(got.status());
  const google::protobuf::Message* submessage = got.ValueOrDie();
  if (T::descriptor()->full_name() !=
      submessage->GetDescriptor()->full_name()) {
    return ::tensorflow::errors::InvalidArgument(
        ::absl::StrCat("Cannot find ", field_path, " in ", message_name,
                       " of type ", T::descriptor()->full_name(),
                       ".  Found: ", submessage->GetDescriptor()->full_name()));
  }

  return dynamic_cast<const T*>(submessage);
}

// Returns true if the field specified by field_path is set on a message,
// false if the field is unset, and InvalidArgument if the field_path doesn't
// resolve to a field, or the field is an unindexed repeated.
StatusOr<const bool> HasSubmessageByPath(const google::protobuf::Message& message,
                                         const string& field_path);

// Clears a field at the location specified by field path.
// If this points to a singular field, will just delete that element.
// If this points to a repeated field (with no index), will delete the entire
// contents of that field.
// If the path is invalid, returns an InvalidArgument status.
// Note that this operates on fields, not a submessage, so a field_path ending
// in an index is considered invalid.
tensorflow::Status ClearFieldByPath(google::protobuf::Message* message,
                                    const string& field_path);

// Returns true if a field_path ends in a repeated index, e.g.,
// Resource.repeatedSubfield[5].
// If true, populates index param with the index.
// Note that this is only true if the LEAF FIELD is repeated, so
// EndsInIndex("Resource.repeatedSubfield[5].id", &index); // False
bool EndsInIndex(const string& field_path, int* index);

// Variant that only returns boolean, without extracting index.
bool EndsInIndex(const string& field_path);

// Strips repeated index from a field where EndsInIndex is true, e.g.:
// StripIndex("repeatedSubfield[5]"); // "repeatedSubfield"
// Is a no-op on fields for which EndsInIndex is false.
string StripIndex(const string& field_path);

// Convenience method for adding a message to a field.
// If the field is singular, returns the mutable message.
// If the field is repeated, returns a newly added message.
google::protobuf::Message* MutableOrAddMessage(google::protobuf::Message* message,
                                     const google::protobuf::FieldDescriptor* field);

// Convenience method for checking if a message has a field set.
// If the field is singular, returns HasField.
// If the field is repeated, returns FieldSize > 0.
bool FieldHasValue(const google::protobuf::Message& message,
                   const google::protobuf::FieldDescriptor* field);

int PotentiallyRepeatedFieldSize(const google::protobuf::Message& message,
                                 const google::protobuf::FieldDescriptor* field);

const google::protobuf::Message& GetPotentiallyRepeatedMessage(
    const google::protobuf::Message& message, const google::protobuf::FieldDescriptor* field,
    const int index);

google::protobuf::Message* MutablePotentiallyRepeatedMessage(
    google::protobuf::Message* message, const google::protobuf::FieldDescriptor* field,
    const int index);

template <typename T>
bool IsMessageType(const google::protobuf::Descriptor* descriptor) {
  return descriptor->full_name() == T::descriptor()->full_name();
}

template <typename T>
bool IsMessageType(const google::protobuf::Message& message) {
  return IsMessageType<T>(message.GetDescriptor());
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_PROTO_UTIL_H_
