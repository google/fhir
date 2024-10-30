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

#include <optional>
#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/fhir/annotations.h"
#include "google/fhir/core_resource_registry.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"

namespace google {
namespace fhir {

namespace internal {

absl::Status PopulateTypedReferenceId(const std::string& resource_id,
                                      const std::string& version,
                                      ::google::protobuf::Message* reference_id);
const ::google::protobuf::FieldDescriptor* GetReferenceFieldForResource(
    const ::google::protobuf::Message& reference, absl::string_view resource_type);

}  // namespace internal

// If possible, parses a reference `uri` field into more structured fields.
// This is only possible for two forms of reference uris:
// * Relative references of the form $TYPE/$ID, e.g., "Patient/1234"
//    In this case, this will be parsed to a proto of the form:
//    {patient_id: {value: "1234"}}
// * Fragments of the form "#$FRAGMENT", e.g., "#vs1".  In this case, this would
//    be parsed into a proto of the form:
//    {fragment: {value: "vs1"} }
//
// If the reference URI matches one of these schemas, the `uri` field will be
// cleared, and the appropriate structured fields set.
//
// If it does not match any of these schemas, the reference will be unchanged,
// and an OK status will be returned.
//
// If the message is not a valid FHIR Reference proto, this will return a
// failure status.
absl::Status SplitIfRelativeReference(::google::protobuf::Message* reference);

// Return the full string representation of a reference.
// If the message `reference` is a valid reference type, but does not contain
// an actual reference (e.g. empty message), then returns nullopt.
absl::StatusOr<std::optional<std::string>> ReferenceProtoToString(
    const ::google::protobuf::Message& reference);

absl::Status ReferenceStringToProto(const std::string& input,
                                    ::google::protobuf::Message* reference);

template <typename ReferenceType>
absl::StatusOr<ReferenceType> ReferenceStringToProto(const std::string& input) {
  ReferenceType reference;
  FHIR_RETURN_IF_ERROR(ReferenceStringToProto(input, &reference));
  return reference;
}

template <typename ResourceType, typename ReferenceLike>
absl::StatusOr<std::string> GetResourceIdFromReference(
    const ReferenceLike& reference) {
  auto status_or_reference_string = ReferenceProtoToString(reference);
  if (!status_or_reference_string.ok()) {
    return status_or_reference_string.status();
  }
  if (!status_or_reference_string.value()) {  // is nullopt
    return ::absl::NotFoundError("Reference is not populated.");
  }
  ::absl::string_view reference_string(
      status_or_reference_string.value().value());
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
absl::StatusOr<ReferenceType> GetReferenceProtoToResource(
    const ::google::protobuf::Message& resource) {
  // TODO(b/268518264): Clean up profiled proto support.
  if (GetFhirVersion(resource.GetDescriptor()) != proto::R4 &&
      IsProfile(resource.GetDescriptor())) {
    return absl::InvalidArgumentError(
        "Profiled resoures only supported for R4");
  }
  ReferenceType reference;
  FHIR_ASSIGN_OR_RETURN(const std::string resource_id, GetResourceId(resource));

  const google::protobuf::Descriptor* base_resource_type = nullptr;
  if (IsProfile(resource.GetDescriptor())) {
    FHIR_ASSIGN_OR_RETURN(base_resource_type,
                          GetBaseResourceDescriptor(resource.GetDescriptor()));
  } else {
    base_resource_type = resource.GetDescriptor();
  }

  const google::protobuf::FieldDescriptor* reference_id_field =
      internal::GetReferenceFieldForResource(reference,
                                             base_resource_type->name());
  if (reference_id_field == nullptr) {
    return absl::InvalidArgumentError(absl::Substitute(
        "$0 has no reference field for type $1",
        reference.GetDescriptor()->full_name(), base_resource_type->name()));
  }
  ::google::protobuf::Message* reference_id =
      reference.GetReflection()->MutableMessage(&reference, reference_id_field);
  FHIR_RETURN_IF_ERROR(internal::PopulateTypedReferenceId(
      resource_id, "" /* no version */, reference_id));
  return reference;
}

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_REFERENCES_H_
