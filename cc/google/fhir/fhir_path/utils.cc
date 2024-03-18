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
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "google/fhir/annotations.h"
#include "google/fhir/json/json_util.h"
#include "google/fhir/references.h"
#include "google/fhir/util.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"

namespace google {
namespace fhir {
namespace fhir_path {
namespace internal {

using ::absl::NotFoundError;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
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

absl::Status OneofMessageFromContainer(const Message& container_message,
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

namespace {

// Returns true if `message` is a Reference and field_descriptor is the `uri`
// field.
bool IsReferenceUri(const Message& message,
                    const google::protobuf::FieldDescriptor& field_descriptor) {
  return message.GetDescriptor()->options().GetExtension(
             ::google::fhir::proto::fhir_structure_definition_url) ==
             "http://hl7.org/fhir/StructureDefinition/Reference" &&
         field_descriptor.name() == "uri";
}

}  // namespace

absl::Status RetrieveField(
    const Message& root, const FieldDescriptor& field,
    std::function<google::protobuf::Message*(const Descriptor*)> message_factory,
    std::vector<const Message*>* results) {
  // FHIR .value invocations on primitive types should return the FHIR
  // primitive message type rather than the underlying native primitive.
  if (IsFhirPrimitiveValue(field)) {
    results->push_back(&root);
    return absl::OkStatus();
  }

  // If asked to retrieve a reference, retrieve a String value with a standard
  // representation (i.e. ResourceType/id) regardless of how the reference is
  // stored in the source message.
  if (IsReferenceUri(root, field)) {
    Message* string_message = message_factory(field.message_type());
    if (string_message == nullptr) {
      return absl::InternalError(
          absl::StrFormat("Unable to create message with same type as '%s'",
                          field.full_name()));
    }
    absl::StatusOr<absl::optional<std::string>> reference_uri =
        ReferenceProtoToString(root);
    if (!reference_uri.ok()) {
      return absl::Status(
          reference_uri.status().code(),
          absl::StrCat(reference_uri.status().message(),
                       " while extracting reference from message ",
                       root.GetDescriptor()->full_name()));
    }
    if (!reference_uri.value()) {  // is nullopt
      return absl::OkStatus();
    }
    string_message->GetReflection()->SetString(
        string_message,
        string_message->GetDescriptor()->FindFieldByName(kPrimitiveValueField),
        reference_uri.value().value());
    results->push_back(string_message);
    return absl::OkStatus();
  }

  return ForEachMessageWithStatus<Message>(
      root, &field, [&](const Message& child) {
        // R4+ packs contained resources in Any protos.
        if (IsMessageType<google::protobuf::Any>(child)) {
          auto any = google::protobuf::DownCastToGenerated<google::protobuf::Any>(child);

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

const FieldDescriptor* FindFieldByJsonName(const Descriptor* descriptor,
                                           absl::string_view json_name) {
  for (int i = 0; i < descriptor->field_count(); ++i) {
    if (json_name == FhirJsonName(descriptor->field(i))) {
      return descriptor->field(i);
    }
  }
  return nullptr;
}

}  // namespace internal
}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
