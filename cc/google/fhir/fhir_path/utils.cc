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

#include "google/fhir/util.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace fhir_path {
namespace internal {

using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using ::tensorflow::errors::NotFound;

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
    return NotFound("Oneof field not found in ",
                    container_message.GetTypeName(), ".");
  }

  const FieldDescriptor* oneof_field =
      container_reflection->GetOneofFieldDescriptor(container_message,
                                                    oneof_descriptor);
  if (oneof_field == nullptr) {
    return NotFound("Oneof field not set in ", container_message.GetTypeName(),
                    ".");
  }

  const Message& oneof_message =
      container_reflection->GetMessage(container_message, oneof_field);
  results->push_back(&oneof_message);
  return Status::OK();
}

Status RetrieveField(const Message& root, const FieldDescriptor& field,
                     std::vector<const Message*>* results) {
  // FHIR .value invocations on primitive types should return the FHIR
  // primitive message type rather than the underlying native primitive.
  if (IsFhirPrimitiveValue(field)) {
    results->push_back(&root);
    return Status::OK();
  }

  return ForEachMessageWithStatus<Message>(
      root, &field, [&](const Message& child) {
        if (IsChoiceType(&field) || IsContainedResource(field.message_type())) {
          return OneofMessageFromContainer(child, results);
        } else {
          results->push_back(&child);
          return Status::OK();
        }
      });
}

}  // namespace internal
}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
