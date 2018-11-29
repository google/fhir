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

#ifndef GOOGLE_FHIR_STU3_UTIL_H_
#define GOOGLE_FHIR_STU3_UTIL_H_

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/reflection.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/time/time.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "re2/re2.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;

using ::google::fhir::stu3::proto::Bundle;
using ::google::fhir::stu3::proto::CodeableConcept;
using ::google::fhir::stu3::proto::ContainedResource;
using ::google::fhir::stu3::proto::Extension;
using ::google::fhir::stu3::proto::Patient;
using ::google::fhir::stu3::proto::Reference;

// Extract code value for a given system code. Return as soon as we find one.
StatusOr<string> ExtractCodeBySystem(const CodeableConcept& codeable_concept,
                                     absl::string_view system_value);

// Extract the icd code for the given schemes.
StatusOr<string> ExtractIcdCode(const CodeableConcept& codeable_concept,
                                const std::vector<string>& schemes);

template <typename R>
stu3::proto::Meta* MutableMetadataFromResource(R* resource) {
  return resource->mutable_meta();
}

StatusOr<Reference> ReferenceStringToProto(const string& input);
// Return the full string representation of a reference.
StatusOr<string> ReferenceProtoToString(const Reference& reference);

// Builds an absl::Time from a time-like fhir Element.
// Must have a value_us field.
template <class T>
absl::Time GetTimeFromTimelikeElement(const T& timelike) {
  return absl::FromUnixMicros(timelike.value_us());
}

absl::Duration GetDurationFromTimelikeElement(
    const stu3::proto::DateTime& datetime);

Status GetTimezone(const string& timezone_str, absl::TimeZone* tz);

// Builds a absl::Time from a time-like fhir Element, corresponding to the
// smallest time greater than this time element. For elements with DAY
// precision, for example, this will be 86400 seconds past value_us of this
// field.
template <class T>
absl::Time GetUpperBoundFromTimelikeElement(const T& timelike) {
  return absl::FromUnixMicros(timelike.value_us()) +
         GetDurationFromTimelikeElement(timelike);
}

// Populates the resource oneof on ContainedResource with the passed-in
// resource.
Status SetContainedResource(const ::google::protobuf::Message& resource,
                            stu3::proto::ContainedResource* contained);

StatusOr<const ::google::protobuf::Message*> GetContainedResource(
    const ContainedResource& contained);

// Returns the input resource, wrapped in a ContainedResource
StatusOr<stu3::proto::ContainedResource> WrapContainedResource(
    const ::google::protobuf::Message& resource);

StatusOr<string> GetResourceId(const ::google::protobuf::Message& message);

Status GetPatient(const Bundle& bundle, const Patient** patient);

bool IsChoiceType(const ::google::protobuf::FieldDescriptor* field);

const string GetFhirProfileBase(const ::google::protobuf::Descriptor* descriptor);

const string GetStructureDefinitionUrl(const ::google::protobuf::Descriptor* descriptor);

bool IsPrimitive(const ::google::protobuf::Descriptor* descriptor);

bool IsResource(const ::google::protobuf::Descriptor* descriptor);

bool IsReference(const ::google::protobuf::Descriptor* descriptor);

bool HasValueset(const ::google::protobuf::Descriptor* descriptor);

// Returns a reference, e.g. "Encounter/1234" for a FHIR resource.
string GetReferenceToResource(const ::google::protobuf::Message& message);

// Extract the value of a Decimal field as a double.
Status GetDecimalValue(const stu3::proto::Decimal& decimal, double* value);

// Extracts and returns the FHIR metadata from a resource
template <typename R>
const stu3::proto::Meta& GetMetadataFromResource(const R& resource) {
  return resource.meta();
}

// Extracts and returns the FHIR resource from a bundle entry.
Status GetResourceFromBundleEntry(const Bundle::Entry& entry,
                                  const ::google::protobuf::Message** result);

// Extracts and returns the FHIR extension list from the resource field in
// a bundle entry.
StatusOr<const ::google::protobuf::RepeatedFieldRef<Extension>>
GetResourceExtensionsFromBundleEntry(const Bundle::Entry& entry);

}  // namespace stu3
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_UTIL_H_
