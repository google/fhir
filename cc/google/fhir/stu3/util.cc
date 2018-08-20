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

#include "google/fhir/stu3/util.h"

#include <iterator>
#include <string>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/reflection.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/systems/systems.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace stu3 {

using ::google::fhir::stu3::proto::ContainedResource;
using ::google::protobuf::Message;

using std::string;
StatusOr<string> ExtractCodeBySystem(
    const stu3::proto::CodeableConcept& codeable_concept,
    absl::string_view system_value) {
  for (int i = 0; i < codeable_concept.coding_size(); i++) {
    auto coding = codeable_concept.coding(i);
    if (coding.has_system() && coding.has_code() &&
        coding.system().value() == system_value) {
      return coding.code().value();
    }
  }

  return ::tensorflow::errors::NotFound(
      "Cannot find a value for the corresponding system code.");
}

StatusOr<string> ExtractIcdCode(
    const stu3::proto::CodeableConcept& codeable_concept,
    const std::vector<string>& schemes) {
  bool found_response = false;
  StatusOr<string> result;
  for (int i = 0; i < schemes.size(); i++) {
    StatusOr<string> s = ExtractCodeBySystem(codeable_concept, schemes[i]);

    if (s.status().code() == ::tensorflow::errors::Code::ALREADY_EXISTS) {
      // Multiple codes, so we can return an error already.
      return s;
    } else if (s.ok()) {
      if (found_response) {
        // We found _another_ code. That shouldn't have happened.
        return ::tensorflow::errors::AlreadyExists("Found more than one code");
      } else {
        result = s;
        found_response = true;
      }
    }
  }
  if (found_response) {
    return result;
  } else {
    return ::tensorflow::errors::NotFound(
        "No ICD code with the provided schemes in concept.");
  }
}

Status SetContainedResource(const Message& resource,
                            ContainedResource* contained) {
  const google::protobuf::OneofDescriptor* resource_oneof =
      ContainedResource::descriptor()->FindOneofByName("oneof_resource");
  const google::protobuf::FieldDescriptor* resource_field = nullptr;
  for (int i = 0; i < resource_oneof->field_count(); i++) {
    const google::protobuf::FieldDescriptor* field = resource_oneof->field(i);
    if (field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
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
  const google::protobuf::Reflection* ref = contained->GetReflection();
  ref->MutableMessage(contained, resource_field)->CopyFrom(resource);
  return Status::OK();
}

StatusOr<ContainedResource> WrapContainedResource(const Message& resource) {
  ContainedResource contained_resource;
  TF_RETURN_IF_ERROR(SetContainedResource(resource, &contained_resource));
  return contained_resource;
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
