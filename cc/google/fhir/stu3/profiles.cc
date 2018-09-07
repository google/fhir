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

#include "google/fhir/stu3/profiles.h"

#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/status/status.h"
#include "google/fhir/stu3/extensions.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/logging.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;
using ::google::fhir::stu3::proto::Extension;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

namespace {

// Finds any extensions on base_resource that correspond to inlined fields on
// profiled_resource, and fills the inlined fields with the extension.
Status PerformSlicing(const Message& base_resource,
                      Message* profiled_resource) {
  const Descriptor* profiled_descriptor = profiled_resource->GetDescriptor();
  const Descriptor* base_descriptor = base_resource.GetDescriptor();
  const Reflection* profiled_reflection = profiled_resource->GetReflection();
  const Reflection* base_reflection = base_resource.GetReflection();

  // Clear the extensions from the profiled_resource message, and then process
  // all the extensions in the base_resource message into the profiled_resource
  // message, either as profiled_resource extensions where possible, or else raw
  // extensions.
  profiled_reflection->ClearField(
      profiled_resource, profiled_descriptor->FindFieldByName("extension"));

  std::unordered_map<string, const FieldDescriptor*> extension_map;
  for (int i = 0; i < profiled_descriptor->field_count(); i++) {
    const FieldDescriptor* field = profiled_descriptor->field(i);
    if (field->options().HasExtension(proto::fhir_inlined_extension_url)) {
      extension_map[field->options().GetExtension(
          proto::fhir_inlined_extension_url)] = field;
    }
  }
  const FieldDescriptor* base_extension_field =
      base_descriptor->FindFieldByName("extension");
  const FieldDescriptor* profiled_extension_field =
      profiled_descriptor->FindFieldByName("extension");
  for (int i = 0;
       i < base_reflection->FieldSize(base_resource, base_extension_field);
       i++) {
    const Extension& raw_extension =
        dynamic_cast<const Extension&>(base_reflection->GetRepeatedMessage(
            base_resource, base_extension_field, i));
    const auto extension_entry_iter =
        extension_map.find(raw_extension.url().value());
    if (extension_entry_iter != extension_map.end()) {
      // This extension can be sliced into an inlined field.
      const FieldDescriptor* inlined_field = extension_entry_iter->second;
      if (raw_extension.extension_size() == 0) {
        // This is a simple extension
        // Note that we cannot use ExtensionToMessage from extensions.h
        // here, because there is no top-level extension message we're merging
        // into, it's just a single primitive inlined field.
        const Descriptor* destination_type = inlined_field->message_type();
        const Extension::Value& value = raw_extension.value();
        const FieldDescriptor* value_field =
            value.GetReflection()->GetOneofFieldDescriptor(
                value, value.GetDescriptor()->FindOneofByName("value"));
        if (value_field == nullptr) {
          return ::tensorflow::errors::InvalidArgument(
              absl::StrCat("Invalid extension: ",
                           "neither value nor extensions set on extension ",
                           raw_extension.url().value()));
        }
        if (destination_type != value_field->message_type()) {
          return ::tensorflow::errors::InvalidArgument(
              "Profiled extension slice is incorrect type: ",
              raw_extension.url().value(), "should be ",
              destination_type->full_name(), "but is ",
              value_field->message_type()->full_name());
        }
        Message* typed_extension = inlined_field->is_repeated()
                                       ? profiled_reflection->AddMessage(
                                             profiled_resource, inlined_field)
                                       : profiled_reflection->MutableMessage(
                                             profiled_resource, inlined_field);
        typed_extension->CopyFrom(
            value.GetReflection()->GetMessage(value, value_field));
      } else {
        // This is a complex extension
        Message* typed_extension = inlined_field->is_repeated()
                                       ? profiled_reflection->AddMessage(
                                             profiled_resource, inlined_field)
                                       : profiled_reflection->MutableMessage(
                                             profiled_resource, inlined_field);
        TF_RETURN_IF_ERROR(ExtensionToMessage(raw_extension, typed_extension));
      }
    } else {
      // There is no inlined field for this extension
      profiled_reflection
          ->AddMessage(profiled_resource, profiled_extension_field)
          ->CopyFrom(raw_extension);
    }
  }
  // TODO(nickgeorge): add value-based slicing (e.g., codeable concept)
  // TODO(nickgeorge): Perform slicing on submessages.
  return Status::OK();
}

}  // namespace

Status ConvertToProfile(const Message& base_resource,
                        Message* profiled_resource) {
  // TODO(nickgeorge): this should operate on structure definition url,
  // not name.
  string profile_base_type =
      profiled_resource->GetDescriptor()->options().GetExtension(
          proto::fhir_profile_base);
  if (profile_base_type != base_resource.GetDescriptor()->name()) {
    return ::tensorflow::errors::InvalidArgument(
        "Unable to convert ", base_resource.GetDescriptor()->full_name(),
        " to profile ", profiled_resource->GetDescriptor()->full_name(),
        ".  The profile has an incorrect base type of ", profile_base_type);
  }

  // Copy over the base_resource message into the profiled_resource container.
  // We can't use CopyFrom because the messages are technically different,
  // but since all the fields from the base_resource have matching fields
  // in profiled_resource, we can serialize the base_resource message to bytes,
  // and then deserialise it into the profiled_resource container.
  if (!profiled_resource->ParseFromString(base_resource.SerializeAsString())) {
    return ::tensorflow::errors::InvalidArgument(
        "Unable to convert ", base_resource.GetDescriptor()->full_name(),
        " to ", profiled_resource->GetDescriptor()->full_name(),
        ".  They are not binary compatible.  This could mean the profile ",
        "applies constraints that the resource does not meet.");
  }

  FHIR_RETURN_IF_ERROR(PerformSlicing(base_resource, profiled_resource));

  return Status::OK();
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
