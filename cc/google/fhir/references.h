/*
 * Copyright 2020 Google LLC
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

#ifndef GOOGLE_FHIR_REFERENCES_H_
#define GOOGLE_FHIR_REFERENCES_H_

#include <string>

#include "google/protobuf/message.h"
#include "absl/strings/strip.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/type_macros.h"
#include "google/fhir/util.h"
#include "re2/re2.h"

namespace google {
namespace fhir {

namespace internal {

Status PopulateTypedReferenceId(const std::string& resource_id,
                                const std::string& version,
                                ::google::protobuf::Message* reference_id);
StatusOr<const ::google::protobuf::FieldDescriptor*> GetReferenceFieldForResource(
    const ::google::protobuf::Message& reference, const std::string& resource_type);

}  // namespace internal

// Return the full string representation of a reference.
StatusOr<std::string> ReferenceProtoToString(
    const ::google::protobuf::Message& reference);

Status ReferenceStringToProto(const std::string& input,
                              ::google::protobuf::Message* reference);

template <typename ReferenceType>
StatusOr<ReferenceType> ReferenceStringToProto(const std::string& input) {
  ReferenceType reference;
  FHIR_RETURN_IF_ERROR(ReferenceStringToProto(input, &reference));
  return reference;
}

template <typename ResourceType, typename ReferenceLike>
StatusOr<std::string> GetResourceIdFromReference(
    const ReferenceLike& reference) {
  auto value = ReferenceProtoToString(reference);
  if (!value.ok()) {
    return value;
  }
  ::absl::string_view reference_string(value.ValueOrDie());
  if (!::absl::ConsumePrefix(
          &reference_string,
          absl::StrCat(ResourceType::descriptor()->name(), "/"))) {
    return ::absl::InvalidArgumentError(
        absl::StrCat("Reference `", reference_string,
                     "` does not point to a resource of type `",
                     ResourceType::descriptor()->name(), "`"));
  }
  return std::string(reference_string);
}

// Returns a reference, e.g. "Encounter/1234" for a FHIR resource.
std::string GetReferenceStringToResource(const ::google::protobuf::Message& message);

// Gets a typed reference to a resource.
template <typename ReferenceType>
StatusOr<ReferenceType> GetReferenceProtoToResource(
    const ::google::protobuf::Message& resource) {
  ReferenceType reference;
  FHIR_ASSIGN_OR_RETURN(const std::string resource_id, GetResourceId(resource));
  FHIR_ASSIGN_OR_RETURN(const ::google::protobuf::FieldDescriptor* reference_id_field,
                        internal::GetReferenceFieldForResource(
                            reference, resource.GetDescriptor()->name()));
  ::google::protobuf::Message* reference_id =
      reference.GetReflection()->MutableMessage(&reference, reference_id_field);
  FHIR_RETURN_IF_ERROR(internal::PopulateTypedReferenceId(
      resource_id, "" /* no version */, reference_id));
  return reference;
}

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_REFERENCES_H_
