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

#ifndef GOOGLE_FHIR_UTIL_H_
#define GOOGLE_FHIR_UTIL_H_

#include <math.h>
#include <stddef.h>

#include <string>
#include <type_traits>
#include <utility>


#include "google/protobuf/any.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "absl/base/macros.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/time/time.h"
#include "google/fhir/annotations.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/type_macros.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/reflection.h"
#include "re2/re2.h"

namespace google {
namespace fhir {

using ::google::protobuf::Descriptor;
using ::google::protobuf::Message;

// Builds an absl::Time from a time-like fhir Element.
// Must have a value_us field.
template <class T>
absl::Time GetTimeFromTimelikeElement(const T& timelike) {
  return absl::FromUnixMicros(timelike.value_us());
}

absl::StatusOr<absl::Time> GetTimeFromTimelikeElement(const Message& timelike);

// Converts a time zone string of the forms found in time-like primitive types
// into an absl::TimeZone
absl::StatusOr<absl::TimeZone> BuildTimeZoneFromString(
    const std::string& time_zone_string);

// Populates the resource oneof on ContainedResource with the passed-in
// resource.
template <typename ContainedResourceLike>
absl::Status SetContainedResource(const Message& resource,
                                  ContainedResourceLike* contained) {
  const ::google::protobuf::OneofDescriptor* resource_oneof =
      ContainedResourceLike::descriptor()->FindOneofByName("oneof_resource");
  if (!resource_oneof) {
    return ::absl::InvalidArgumentError(::absl::StrCat(
        "Template type '", ContainedResourceLike::descriptor()->full_name(),
        "' is not a contained resource."));
  }
  const ::google::protobuf::FieldDescriptor* resource_field = nullptr;
  for (int i = 0; i < resource_oneof->field_count(); i++) {
    const ::google::protobuf::FieldDescriptor* field = resource_oneof->field(i);
    if (field->cpp_type() != ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      return ::absl::InvalidArgumentError(
          absl::StrCat("Field ", field->full_name(), "is not a message"));
    }
    if (field->message_type()->name() == resource.GetDescriptor()->name()) {
      resource_field = field;
    }
  }
  if (resource_field == nullptr) {
    return ::absl::InvalidArgumentError(
        absl::StrCat("Resource type ", resource.GetDescriptor()->name(),
                     " not found in fhir::Bundle::Entry::resource"));
  }
  const ::google::protobuf::Reflection* ref = contained->GetReflection();
  ref->MutableMessage(contained, resource_field)->CopyFrom(resource);
  return absl::OkStatus();
}

// Returns whether or not passed in descriptor is a oneof resource in the
// contained resource.
template <typename ContainedResourceType>
absl::StatusOr<bool> IsResourceFromContainedResourceType(
    const Descriptor* descriptor) {
  // Get oneof descriptor from `ContainedResourceType` if it exists.
  const ::google::protobuf::OneofDescriptor* resource_oneof =
      ContainedResourceType::descriptor()->FindOneofByName("oneof_resource");
  if (!resource_oneof) {
    return ::absl::NotFoundError(::absl::StrCat(
        "Template type '", ContainedResourceType::descriptor()->full_name(),
        "' is not a contained resource."));
  }
  static const absl::flat_hash_set<std::string>* resource_types =
      [resource_oneof] {
        auto* resource_types = new absl::flat_hash_set<std::string>();
        // Iterate through all `oneof` fields to get names of resources.
        for (int i = 0; i < resource_oneof->field_count(); i++) {
          resource_types->emplace(
              resource_oneof->field(i)->message_type()->full_name());
        }
        return resource_types;
      }();

  return resource_types->contains(descriptor->full_name());
}

template <typename ContainedResourceLike>
absl::StatusOr<const Message*> GetContainedResource(
    const ContainedResourceLike& contained) {
  const ::google::protobuf::Reflection* ref = contained.GetReflection();
  // Get the resource field corresponding to this resource.
  const ::google::protobuf::OneofDescriptor* resource_oneof =
      contained.GetDescriptor()->FindOneofByName("oneof_resource");
  if (!resource_oneof) {
    return ::absl::InvalidArgumentError(::absl::StrCat(
        "Template type '", contained.GetDescriptor()->full_name(),
        "' is not a contained resource."));
  }
  const ::google::protobuf::FieldDescriptor* field =
      contained.GetReflection()->GetOneofFieldDescriptor(contained,
                                                         resource_oneof);
  if (!field) {
    return ::absl::NotFoundError("ContainedResource is empty.");
  }
  return &(ref->GetMessage(contained, field));
}

// Returns the input resource, wrapped in a ContainedResource
template <typename ContainedResourceLike>
absl::StatusOr<ContainedResourceLike> WrapContainedResource(
    const Message& resource) {
  ContainedResourceLike contained_resource;
  FHIR_RETURN_IF_ERROR(SetContainedResource(resource, &contained_resource));
  return contained_resource;
}

absl::StatusOr<std::string> GetResourceId(const Message& message);

bool ResourceHasId(const Message& message);

template <typename BundleLike, typename PatientLike>
absl::Status GetPatient(const BundleLike& bundle, const PatientLike** patient) {
  bool found = false;
  for (const auto& entry : bundle.entry()) {
    if (entry.resource().has_patient()) {
      if (found) {
        return ::absl::AlreadyExistsError(
            "Found more than one patient in bundle");
      }
      *patient = &entry.resource().patient();
      found = true;
    }
  }
  if (found) {
    return absl::OkStatus();
  } else {
    return ::absl::NotFoundError("No patient in bundle.");
  }
}

template <typename BundleLike,
          typename PatientLike = BUNDLE_TYPE(BundleLike, patient)>
absl::StatusOr<const PatientLike*> GetPatient(const BundleLike& bundle) {
  const PatientLike* patient;
  FHIR_RETURN_IF_ERROR(GetPatient(bundle, &patient));
  return patient;
}

template <typename BundleLike,
          typename PatientLike = BUNDLE_TYPE(BundleLike, patient)>
absl::StatusOr<PatientLike*> GetMutablePatient(BundleLike* bundle) {
  const PatientLike* const_patient;
  auto status = GetPatient(*bundle, &const_patient);
  if (status.ok()) {
    return const_cast<PatientLike*>(const_patient);
  }
  return status;
}

// Extract the value of a Decimal field as a double.
template <class DecimalLike>
absl::StatusOr<double> GetDecimalValue(const DecimalLike& decimal) {
  double value;
  if (!absl::SimpleAtod(decimal.value(), &value) || isinf(value) ||
      isnan(value)) {
    return ::absl::InvalidArgumentError(
        absl::StrCat("Invalid decimal: '", decimal.value(), "'"));
  }
  return value;
}

// Extracts and returns the FHIR resource from a bundle entry.
template <typename EntryLike>
absl::StatusOr<const Message*> GetResourceFromBundleEntry(
    const EntryLike& entry) {
  return GetContainedResource(entry.resource());
}

// Extracts and returns the FHIR extension list from the resource field in
// a bundle entry.
template <typename EntryLike,
          typename ExtensionLike = EXTENSION_TYPE(EntryLike)>
absl::StatusOr<const ::google::protobuf::RepeatedFieldRef<ExtensionLike>>
GetResourceExtensionsFromBundleEntry(const EntryLike& entry) {
  FHIR_ASSIGN_OR_RETURN(const Message* resource,
                        GetResourceFromBundleEntry(entry));
  const ::google::protobuf::Reflection* ref = resource->GetReflection();
  // Get the bundle field corresponding to this resource.
  const ::google::protobuf::FieldDescriptor* field =
      resource->GetDescriptor()->FindFieldByName("extension");
  if (field == nullptr) {
    return ::absl::NotFoundError("No extension field.");
  }
  return ref->GetRepeatedFieldRef<ExtensionLike>(*resource, field);
}

absl::Status SetPrimitiveStringValue(Message* primitive,
                                     const std::string& value);
absl::StatusOr<std::string> GetPrimitiveStringValue(const Message& primitive,
                                                    std::string* scratch);
absl::StatusOr<std::string> GetPrimitiveStringValue(
    const Message& parent, const std::string& field_name, std::string* scratch);
absl::StatusOr<int> GetPrimitiveIntValue(const Message& primitive);

// Finds a resource of a templatized type within a bundle, by reference id.
template <typename R, typename BundleLike, typename ReferenceIdLike>
absl::Status GetResourceByReferenceId(const BundleLike& bundle,
                                      const ReferenceIdLike& reference_id,
                                      const R** output) {
  // First, find the correct oneof field to check.
  const ::google::protobuf::OneofDescriptor* resource_oneof =
      BundleLike::Entry::descriptor()
          ->FindFieldByName("resource")
          ->message_type()
          ->FindOneofByName("oneof_resource");

  const ::google::protobuf::FieldDescriptor* resource_field = nullptr;
  for (int i = 0; i < resource_oneof->field_count(); i++) {
    const ::google::protobuf::FieldDescriptor* field = resource_oneof->field(i);
    if (field->message_type()->full_name() == R::descriptor()->full_name()) {
      resource_field = field;
    }
  }
  if (resource_field == nullptr) {
    return ::absl::InvalidArgumentError(absl::StrCat(
        "No resource oneof option for type ", R::descriptor()->full_name()));
  }

  // For each bundle entry, check if that field is populated with a resource
  // with the correct reference id.
  for (const auto& entry : bundle.entry()) {
    const Message& contained_resource = entry.resource();
    const ::google::protobuf::Reflection* contained_reflection =
        contained_resource.GetReflection();
    if (contained_reflection->HasField(contained_resource, resource_field)) {
      const R& resource = dynamic_cast<const R&>(
          contained_reflection->GetMessage(contained_resource, resource_field));
      if (resource.id().value() == reference_id.value()) {
        *output = &resource;
        return absl::OkStatus();
      }
    }
  }

  return ::absl::NotFoundError(absl::StrCat(
      "No matching resource in bundle.\nReference:", reference_id.value(),
      "\nBundle:\n", bundle.DebugString()));
}

absl::StatusOr<Message*> MutableContainedResource(google::protobuf::Message* contained);

template <typename R, typename ContainedResourceLike>
absl::StatusOr<const R*> GetTypedContainedResource(
    const ContainedResourceLike& contained) {
  const ::google::protobuf::OneofDescriptor* contained_oneof =
      ContainedResourceLike::descriptor()->FindOneofByName("oneof_resource");
  if (!contained_oneof) {
    return ::absl::InvalidArgumentError(::absl::StrCat(
        "Template type '", ContainedResourceLike::descriptor()->full_name(),
        "' is not a contained resource."));
  }
  for (int i = 0; i < contained_oneof->field_count(); i++) {
    const ::google::protobuf::FieldDescriptor* field = contained_oneof->field(i);
    if (field->message_type()->full_name() == R::descriptor()->full_name()) {
      if (!contained.GetReflection()->HasField(contained, field)) {
        return ::absl::NotFoundError(absl::StrCat(
            "Contained resource does not have set resource of type ",
            R::descriptor()->name()));
      }
      return dynamic_cast<const R*>(
          &contained.GetReflection()->GetMessage(contained, field));
    }
  }
  return ::absl::InvalidArgumentError(absl::StrCat(
      "No resource field found for type ", R::descriptor()->name()));
}

std::string ToSnakeCase(absl::string_view input);

// Given an Any representing a packed ContainedResource, returns a pointer to
// a newly-created ContainedResource message of the correct type.  Ownership
// of the object is passed to the call site.
// Returns a status failure if the Any is not an encoded ContainedResource,
// or if the correct ContainedResource is not known to the default descriptor
// pool.
// Note that this method has no application for Implementation Guide protos that
// do not use Any to represent contained resources (e.g., STU3).
absl::StatusOr<std::unique_ptr<Message>> UnpackAnyAsContainedResource(
    const google::protobuf::Any& any);

// Allows calling UnpackAnyAsContainedResource with a custom message factory,
// e.g., to allow more sophisticated memory management of the created object.
absl::StatusOr<Message*> UnpackAnyAsContainedResource(
    const google::protobuf::Any& any,
    std::function<absl::StatusOr<google::protobuf::Message*>(const Descriptor*)>
        message_factory);

// Given a descriptor for a ContainedResource and the name of a resource,
// (e.g., "Patient") searches the resource for a field whose type has that name.
// Returns an InvalidArgumentError if the descriptor is not a valid
// ContainedResource, and a NotFoundError if there is no field whose type has
// the provided name.
absl::StatusOr<const Descriptor*> GetResourceDescriptorByName(
    const Descriptor* contained_resource_type, absl::string_view name);

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_UTIL_H_
