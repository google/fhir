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

#include <stddef.h>

#include <string>
#include <type_traits>
#include <utility>


#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/reflection.h"
#include "absl/base/macros.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/time/time.h"
#include "google/fhir/annotations.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "proto/annotations.pb.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "re2/re2.h"

// Macro for specifying the ContainedResources type within a bundle-like object,
// given the Bundle-like type
#define BUNDLE_CONTAINED_RESOURCE(b)                                  \
  typename std::remove_const<typename std::remove_reference<decltype( \
      std::declval<b>().entry(0).resource())>::type>::type

// Macro for specifying a type contained within a bundle-like object, given
// the Bundle-like type, and a field on the contained resource from that bundle.
// E.g., in a template with a BundleLike type,
// BUNDLE_TYPE(BundleLike, observation)
// will return the Observation type from that bundle.
#define BUNDLE_TYPE(b, r)                                             \
  typename std::remove_const<typename std::remove_reference<decltype( \
      std::declval<b>().entry(0).resource().r())>::type>::type

// Macro for specifying the extension type associated with a FHIR type.
#define EXTENSION_TYPE(t)                                             \
  typename std::remove_const<typename std::remove_reference<decltype( \
      std::declval<t>().id().extension(0))>::type>::type

// Given a FHIR type, returns the datatype with a given name assosiated with
// the input type's version of fhir
#define FHIR_DATATYPE(t, d)                                           \
  typename std::remove_const<typename std::remove_reference<decltype( \
      std::declval<t>().id().extension(0).value().d())>::type>::type

// Given a FHIR Reference type, gets the corresponding ReferenceId type.
#define REFERENCE_ID_TYPE(r)                                          \
  typename std::remove_const<typename std::remove_reference<decltype( \
      std::declval<r>().patient_id())>::type>::type

namespace google {
namespace fhir {


template <typename R>
stu3::proto::Meta* MutableMetadataFromResource(R* resource) {
  return resource->mutable_meta();
}

// Splits relative references into their components, for example, "Patient/ABCD"
// will result in the patientId field getting the value "ABCD".
Status SplitIfRelativeReference(::google::protobuf::Message* reference);

StatusOr<stu3::proto::Reference> ReferenceStringToProtoStu3(
    const std::string& input);

StatusOr<r4::core::Reference> ReferenceStringToProtoR4(
    const std::string& input);

// Return the full string representation of a reference.
StatusOr<std::string> ReferenceProtoToString(
    const stu3::proto::Reference& reference);

// Return the full string representation of a reference.
StatusOr<std::string> ReferenceProtoToString(
    const r4::core::Reference& reference);

// When a message is not of a known type at compile time, this overload can
// be used to cast to a reference and then call ReferenceProtoToString.
// This is provided as a separate API rather than relying on the caller to cast
// so that version-agnostic libraries don't need to link in multiple versions
// of FHIR.
StatusOr<std::string> ReferenceMessageToString(
    const ::google::protobuf::Message& reference);

// Builds an absl::Time from a time-like fhir Element.
// Must have a value_us field.
template <class T>
absl::Time GetTimeFromTimelikeElement(const T& timelike) {
  return absl::FromUnixMicros(timelike.value_us());
}

absl::Duration GetDurationFromTimelikeElement(
    const ::google::fhir::stu3::proto::DateTime& datetime);

absl::Duration GetDurationFromTimelikeElement(
    const ::google::fhir::r4::core::DateTime& datetime);

// Builds a absl::Time from a time-like fhir Element, corresponding to the
// smallest time greater than this time element. For elements with DAY
// precision, for example, this will be 86400 seconds past value_us of this
// field.
template <class T>
absl::Time GetUpperBoundFromTimelikeElement(const T& timelike) {
  return absl::FromUnixMicros(timelike.value_us()) +
         GetDurationFromTimelikeElement(timelike);
}

ABSL_DEPRECATED("Use BuildTimeZoneFromString instead.")
Status GetTimezone(const std::string& timezone_str, absl::TimeZone* tz);

// Converts a time zone string of the forms found in time-like primitive types
// into an absl::TimeZone
StatusOr<absl::TimeZone> BuildTimeZoneFromString(
    const std::string& time_zone_string);

// Populates the resource oneof on ContainedResource with the passed-in
// resource.
template <typename ContainedResourceLike>
Status SetContainedResource(const ::google::protobuf::Message& resource,
                            ContainedResourceLike* contained) {
  const ::google::protobuf::OneofDescriptor* resource_oneof =
      ContainedResourceLike::descriptor()->FindOneofByName("oneof_resource");
  const ::google::protobuf::FieldDescriptor* resource_field = nullptr;
  for (int i = 0; i < resource_oneof->field_count(); i++) {
    const ::google::protobuf::FieldDescriptor* field = resource_oneof->field(i);
    if (field->cpp_type() != ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      return ::tensorflow::errors::InvalidArgument(
          absl::StrCat("Field ", field->full_name(), "is not a message"));
    }
    if (field->message_type()->name() == resource.GetDescriptor()->name()) {
      resource_field = field;
    }
  }
  if (resource_field == nullptr) {
    return ::tensorflow::errors::InvalidArgument(
        absl::StrCat("Resource type ", resource.GetDescriptor()->name(),
                     " not found in fhir::Bundle::Entry::resource"));
  }
  const ::google::protobuf::Reflection* ref = contained->GetReflection();
  ref->MutableMessage(contained, resource_field)->CopyFrom(resource);
  return Status::OK();
}

template <typename ContainedResourceLike>
StatusOr<const ::google::protobuf::Message*> GetContainedResource(
    const ContainedResourceLike& contained) {
  const ::google::protobuf::Reflection* ref = contained.GetReflection();
  // Get the resource field corresponding to this resource.
  const ::google::protobuf::OneofDescriptor* resource_oneof =
      contained.GetDescriptor()->FindOneofByName("oneof_resource");
  const ::google::protobuf::FieldDescriptor* field =
      contained.GetReflection()->GetOneofFieldDescriptor(contained,
                                                         resource_oneof);
  if (!field) {
    return ::tensorflow::errors::NotFound("No Bundle Resource found");
  }
  return &(ref->GetMessage(contained, field));
}

// Returns the input resource, wrapped in a ContainedResource
template <typename ContainedResourceLike>
StatusOr<ContainedResourceLike> WrapContainedResource(
    const ::google::protobuf::Message& resource) {
  ContainedResourceLike contained_resource;
  TF_RETURN_IF_ERROR(SetContainedResource(resource, &contained_resource));
  return contained_resource;
}

StatusOr<std::string> GetResourceId(const ::google::protobuf::Message& message);

template <typename BundleLike, typename PatientLike>
Status GetPatient(const BundleLike& bundle, const PatientLike** patient) {
  bool found = false;
  for (const auto& entry : bundle.entry()) {
    if (entry.resource().has_patient()) {
      if (found) {
        return ::tensorflow::errors::AlreadyExists(
            "Found more than one patient in bundle");
      }
      *patient = &entry.resource().patient();
      found = true;
    }
  }
  if (found) {
    return Status::OK();
  } else {
    return ::tensorflow::errors::NotFound("No patient in bundle.");
  }
}

template <typename BundleLike,
          typename PatientLike = BUNDLE_TYPE(BundleLike, patient)>
StatusOr<const PatientLike*> GetPatient(const BundleLike& bundle) {
  const PatientLike* patient;
  FHIR_RETURN_IF_ERROR(GetPatient(bundle, &patient));
  return patient;
}

// Returns a reference, e.g. "Encounter/1234" for a FHIR resource.
std::string GetReferenceToResource(const ::google::protobuf::Message& message);

// Given a resource and a reference, populates the correct typed reference field
// with a reference to that resource. If the message is not a FHIR
// resource, an error will be returned.
Status PopulatedTypedReferenceToResource(const ::google::protobuf::Message& resource,
                                         stu3::proto::Reference* reference);

// Returns a typed Reference for a FHIR resource.  If the message is not a FHIR
// resource, an error will be returned.
// TODO: Split version-specific functionality into separate files.
StatusOr<stu3::proto::Reference> GetTypedReferenceToResourceStu3(
    const ::google::protobuf::Message& resource);

StatusOr<r4::core::Reference> GetTypedReferenceToResourceR4(
    const ::google::protobuf::Message& resource);

// Extract the value of a Decimal field as a double.
Status GetDecimalValue(const stu3::proto::Decimal& decimal, double* value);

// Extracts and returns the FHIR metadata from a resource
template <typename R>
const stu3::proto::Meta& GetMetadataFromResource(const R& resource) {
  return resource.meta();
}

// Extracts and returns the FHIR resource from a bundle entry.
template <typename EntryLike>
Status GetResourceFromBundleEntry(const EntryLike& entry,
                                  const ::google::protobuf::Message** result) {
  auto got_value = GetContainedResource(entry.resource());
  TF_RETURN_IF_ERROR(got_value.status());
  *result = got_value.ValueOrDie();
  return Status::OK();
}

// Extracts and returns the FHIR extension list from the resource field in
// a bundle entry.
template <typename EntryLike,
          typename ExtensionLike = EXTENSION_TYPE(EntryLike)>
StatusOr<const ::google::protobuf::RepeatedFieldRef<ExtensionLike>>
GetResourceExtensionsFromBundleEntry(const EntryLike& entry) {
  const ::google::protobuf::Message* resource;
  TF_RETURN_IF_ERROR(GetResourceFromBundleEntry(entry, &resource));
  const ::google::protobuf::Reflection* ref = resource->GetReflection();
  // Get the bundle field corresponding to this resource.
  const ::google::protobuf::FieldDescriptor* field =
      resource->GetDescriptor()->FindFieldByName("extension");
  if (field == nullptr) {
    return ::tensorflow::errors::NotFound("No extension field.");
  }
  return ref->GetRepeatedFieldRef<ExtensionLike>(*resource, field);
}

Status SetPrimitiveStringValue(::google::protobuf::Message* primitive,
                               const std::string& value);
StatusOr<std::string> GetPrimitiveStringValue(
    const ::google::protobuf::Message& primitive, std::string* scratch);

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_UTIL_H_
