// Copyright 2020 Google LLC
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

#include "google/fhir/fhir_path/utils.h"

#include "google/protobuf/any.pb.h"
#include "absl/status/status.h"
#include "google/fhir/util.h"

namespace google {
namespace fhir {
namespace fhir_path {
namespace internal {

using ::absl::NotFoundError;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Descriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

// Logical field in primitives representing the underlying value.
constexpr char kPrimitiveValueField[] = "value";

// Returns true if the given field is accessing the logical "value"
// field on FHIR primitives.
bool IsFhirPrimitiveValue(const FieldDescriptor& field) {
  return field.name() == kPrimitiveValueField &&
         IsPrimitive(field.containing_type());
}

Status OneofMessageFromContainer(const Message& container_message,
                                 std::vector<const Message*>* results) {
  const Reflection* container_reflection = container_message.GetReflection();
  const google::protobuf::OneofDescriptor* oneof_descriptor =
      container_message.GetDescriptor()->oneof_decl(0);
  if (oneof_descriptor == nullptr) {
    return NotFoundError(absl::StrCat("Oneof field not found in ",
                                      container_message.GetTypeName(), "."));
  }

  const FieldDescriptor* oneof_field =
      container_reflection->GetOneofFieldDescriptor(container_message,
                                                    oneof_descriptor);
  if (oneof_field == nullptr) {
    return NotFoundError(absl::StrCat("Oneof field not set in ",
                                      container_message.GetTypeName(), "."));
  }

  const Message& oneof_message =
      container_reflection->GetMessage(container_message, oneof_field);
  results->push_back(&oneof_message);
  return absl::OkStatus();
}

Status RetrieveField(
    const Message& root, const FieldDescriptor& field,
    std::function<google::protobuf::Message*(const Descriptor*)> message_factory,
    std::vector<const Message*>* results) {
  // FHIR .value invocations on primitive types should return the FHIR
  // primitive message type rather than the underlying native primitive.
  if (IsFhirPrimitiveValue(field)) {
    results->push_back(&root);
    return absl::OkStatus();
  }

  return ForEachMessageWithStatus<Message>(
      root, &field, [&](const Message& child) {
        // R4+ packs contained resources in Any protos.
        if (IsMessageType<google::protobuf::Any>(child)) {
          auto any = dynamic_cast<const google::protobuf::Any&>(child);

          FHIR_ASSIGN_OR_RETURN(
              Message * unpacked_message,
              UnpackAnyAsContainedResource(any, message_factory));

          return OneofMessageFromContainer(*unpacked_message, results);
        }

        if (IsChoiceType(&field) || IsContainedResource(field.message_type())) {
          return OneofMessageFromContainer(child, results);
        } else {
          results->push_back(&child);
          return absl::OkStatus();
        }
      });
}

bool HasFieldWithJsonName(const Descriptor* descriptor,
                          absl::string_view json_name) {
  if (IsContainedResource(descriptor) || IsChoiceTypeContainer(descriptor)) {
    for (int i = 0; i < descriptor->field_count(); i++) {
      if (FindFieldByJsonName(descriptor->field(i)->message_type(),
                              json_name)) {
        return true;
      }
    }
    return false;
  }

  return FindFieldByJsonName(descriptor, json_name);
}

const FieldDescriptor* FindFieldByJsonName(
    const Descriptor* descriptor, absl::string_view json_name) {
  for (int i = 0; i < descriptor->field_count(); ++i) {
    if (json_name == descriptor->field(i)->json_name()) {
      return descriptor->field(i);
    }
  }
  return nullptr;
}

}  // namespace internal
}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
