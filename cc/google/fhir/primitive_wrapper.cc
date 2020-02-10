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

#include "google/fhir/primitive_wrapper.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "absl/memory/memory.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codes.h"
#include "google/fhir/extensions.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/r4/google_extensions.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "include/json/json.h"
#include "tensorflow/core/lib/core/errors.h"
#include "re2/re2.h"

namespace google {
namespace fhir {

using ::google::fhir::Status;
using ::google::fhir::StatusOr;
using primitives_internal::PrimitiveWrapper;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using ::tensorflow::errors::InvalidArgument;

namespace primitives_internal {

StatusOr<bool> HasPrimitiveHasNoValue(const Message& message) {
  const FieldDescriptor* field =
      message.GetDescriptor()->FindFieldByName("extension");
  std::vector<const Message*> no_value_extensions;
  ForEachMessage<Message>(message, field, [&](const Message& extension) {
    std::string scratch;
    const std::string& url_value =
        extensions_lib::GetExtensionUrl(extension, &scratch);
    if (url_value == kPrimitiveHasNoValueUrl) {
      no_value_extensions.push_back(&extension);
    }
  });
  if (no_value_extensions.size() > 1) {
    return InvalidArgument(
        "Message has more than one PrimitiveHasNoValue extension: ",
        message.GetDescriptor()->full_name());
  }
  if (no_value_extensions.empty()) {
    return false;
  }
  const Message& no_value_extension = *no_value_extensions.front();
  const Message& value_msg = no_value_extension.GetReflection()->GetMessage(
      no_value_extension,
      no_value_extension.GetDescriptor()->FindFieldByName("value"));
  const Message& boolean_msg = value_msg.GetReflection()->GetMessage(
      value_msg, value_msg.GetDescriptor()->FindFieldByName("boolean"));
  return boolean_msg.GetReflection()->GetBool(
      boolean_msg, boolean_msg.GetDescriptor()->FindFieldByName("value"));
}

}  // namespace primitives_internal

Status BuildHasNoValueExtension(Message* extension) {
  const Descriptor* descriptor = extension->GetDescriptor();
  const Reflection* reflection = extension->GetReflection();

  if (!IsExtension(descriptor)) {
    return InvalidArgument("Not a valid extension type: ",
                           descriptor->full_name());
  }

  const FieldDescriptor* url_field = descriptor->FindFieldByName("url");
  FHIR_RETURN_IF_ERROR(
      SetPrimitiveStringValue(reflection->MutableMessage(extension, url_field),
                              primitives_internal::kPrimitiveHasNoValueUrl));

  Message* value = reflection->MutableMessage(
      extension, descriptor->FindFieldByName("value"));

  Message* boolean_message = value->GetReflection()->MutableMessage(
      value, value->GetDescriptor()->FindFieldByName("boolean"));

  boolean_message->GetReflection()->SetBool(
      boolean_message,
      boolean_message->GetDescriptor()->FindFieldByName("value"), true);

  return Status::OK();
}

}  // namespace fhir
}  // namespace google
