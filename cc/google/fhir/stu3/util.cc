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
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace stu3 {

using ::google::protobuf::Message;
using ::google::fhir::stu3::proto::ContainedResource;

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
