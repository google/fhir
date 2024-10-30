/*
 * Copyright 2020 Google LLC
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

#include "google/fhir/references.h"

#include <memory>
#include <string>

#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "re2/re2.h"

namespace google {
namespace fhir {

using ::absl::InvalidArgumentError;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

absl::Status ReferenceStringToProto(const std::string& input,
                                    Message* reference) {
  const Descriptor* descriptor = reference->GetDescriptor();
  const Reflection* reflection = reference->GetReflection();
  const FieldDescriptor* uri_field = descriptor->FindFieldByName("uri");
  FHIR_RETURN_IF_ERROR(SetPrimitiveStringValue(
      reflection->MutableMessage(reference, uri_field), input));
  return SplitIfRelativeReference(reference);
}

std::string GetReferenceStringToResource(const Message& message) {
  return absl::StrCat(message.GetDescriptor()->name(), "/",
                      GetResourceId(message).value());
}

absl::StatusOr<absl::optional<std::string>> ReferenceProtoToString(
    const ::google::protobuf::Message& reference) {
  const ::google::protobuf::Descriptor* descriptor = reference.GetDescriptor();
  const ::google::protobuf::Reflection* reflection = reference.GetReflection();

  std::string uri_value;
  FHIR_ASSIGN_OR_RETURN(uri_value,
                        GetPrimitiveStringValue(reference, "uri", &uri_value));
  if (!uri_value.empty()) {
    return uri_value;
  }

  std::string fragment_value;
  FHIR_ASSIGN_OR_RETURN(
      fragment_value,
      GetPrimitiveStringValue(reference, "fragment", &fragment_value));
  if (!fragment_value.empty()) {
    return absl::StrCat("#", fragment_value);
  }

  static const google::protobuf::OneofDescriptor* oneof =
      descriptor->FindOneofByName("reference");
  if (oneof == nullptr) {
    return absl::InvalidArgumentError(
        absl::StrCat("Invalid Reference type: ", descriptor->full_name()));
  }
  const ::google::protobuf::FieldDescriptor* reference_field =
      reflection->GetOneofFieldDescriptor(reference, oneof);
  if (reference_field == nullptr) {
    return absl::nullopt;
  }
  if (reference_field->name() == "uri" ||
      reference_field->name() == "fragment") {
    return absl::nullopt;
  }

  std::string prefix;
  bool start = true;
  for (const char c : reference_field->name()) {
    if (start) {
      start = false;
      prefix.push_back(c + 'A' - 'a');
    } else if (c == '_') {
      start = true;
    } else {
      prefix.push_back(c);
    }
  }
  static LazyRE2 re = {"Id$"};
  RE2::Replace(&prefix, *re, "");

  const ::google::protobuf::Message& id =
      reflection->GetMessage(reference, reference_field);

  std::string id_value;
  FHIR_ASSIGN_OR_RETURN(id_value, GetPrimitiveStringValue(id, &id_value));
  std::string reference_string = absl::StrCat(prefix, "/", id_value);

  std::string history_value;
  FHIR_ASSIGN_OR_RETURN(history_value,
                        GetPrimitiveStringValue(id, "history", &history_value));

  if (!history_value.empty()) {
    absl::StrAppend(&reference_string, "/_history/", history_value);
  }
  return reference_string;
}

// Splits relative references into their components, for example, "Patient/ABCD"
// will result in the patientId field getting the value "ABCD".
absl::Status SplitIfRelativeReference(Message* reference) {
  const Descriptor* descriptor = reference->GetDescriptor();
  const Reflection* reflection = reference->GetReflection();

  const FieldDescriptor* uri_field = descriptor->FindFieldByName("uri");
  const FieldDescriptor* fragment_field =
      descriptor->FindFieldByName("fragment");
  if (!reflection->HasField(*reference, uri_field)) {
    // There is no uri to split
    return absl::OkStatus();
  }

  const Message& uri = reflection->GetMessage(*reference, uri_field);

  std::string uri_scratch;
  FHIR_ASSIGN_OR_RETURN(const std::string& uri_string,
                        GetPrimitiveStringValue(uri, &uri_scratch));

  static const LazyRE2 kInternalReferenceRegex{
      "([0-9A-Za-z_]+)/([A-Za-z0-9.-]{1,64})(?:/_history/"
      "([A-Za-z0-9.-]{1,64}))?"};
  std::string resource_type;
  std::string resource_id;
  std::string version;
  if (RE2::FullMatch(uri_string, *kInternalReferenceRegex, &resource_type,
                     &resource_id, &version)) {
    const FieldDescriptor* reference_id_field =
        internal::GetReferenceFieldForResource(*reference, resource_type);
    if (reference_id_field == nullptr) {
      // Not a recognized relative reference.
      return absl::OkStatus();
    }

    // Note that we make the reference_id off of the reference before adding it,
    // since adding the reference_id would destroy the uri field, since they are
    // in the same oneof.  This way allows us to copy fields from uri to
    // reference_id without an extra copy.
    std::unique_ptr<Message> reference_id =
        absl::WrapUnique(reflection->GetMessageFactory()
                             ->GetPrototype(reference_id_field->message_type())
                             ->New());
    FHIR_RETURN_IF_ERROR(internal::PopulateTypedReferenceId(
        resource_id, version, reference_id.get()));
    const Message& uri = reflection->GetMessage(*reference, uri_field);
    reference_id->GetTypeName();
    FHIR_RETURN_IF_ERROR(CopyCommonField(uri, reference_id.get(), "id"));
    FHIR_RETURN_IF_ERROR(CopyCommonField(uri, reference_id.get(), "extension"));
    reflection->SetAllocatedMessage(reference, reference_id.release(),
                                    reference_id_field);
    return absl::OkStatus();
  }

  static const LazyRE2 kFragmentReferenceRegex{"#[A-Za-z0-9.-]{1,64}"};
  if (RE2::FullMatch(uri_string, *kFragmentReferenceRegex)) {
    // Note that we make the fragment off of the reference before adding it,
    // since adding the fragment would destroy the uri field, since they are in
    // the same oneof.  This way allows us to copy fields from uri to fragment
    // without an extra copy.
    std::unique_ptr<Message> fragment =
        absl::WrapUnique(reflection->GetMessageFactory()
                             ->GetPrototype(fragment_field->message_type())
                             ->New());
    FHIR_RETURN_IF_ERROR(
        SetPrimitiveStringValue(fragment.get(), uri_string.substr(1)));
    FHIR_RETURN_IF_ERROR(CopyCommonField(uri, fragment.get(), "id"));
    FHIR_RETURN_IF_ERROR(CopyCommonField(uri, fragment.get(), "extension"));
    reflection->SetAllocatedMessage(reference, fragment.release(),
                                    fragment_field);
    return absl::OkStatus();
  }

  // There's no way to rewrite the URI, but it's valid as is.
  return absl::OkStatus();
}

namespace internal {

absl::Status PopulateTypedReferenceId(const std::string& resource_id,
                                      const std::string& version,
                                      Message* reference_id) {
  FHIR_RETURN_IF_ERROR(SetPrimitiveStringValue(reference_id, resource_id));
  if (!version.empty()) {
    const FieldDescriptor* history_field =
        reference_id->GetDescriptor()->FindFieldByName("history");
    if (history_field == nullptr) {
      return InvalidArgumentError(
          absl::StrCat("Not a valid ReferenceId message: ",
                       reference_id->GetDescriptor()->full_name(),
                       ".  Field history does not exist)."));
    }
    FHIR_RETURN_IF_ERROR(
        SetPrimitiveStringValue(reference_id->GetReflection()->MutableMessage(
                                    reference_id, history_field),
                                version));
  }
  return absl::OkStatus();
}

const FieldDescriptor* GetReferenceFieldForResource(
    const Message& reference, absl::string_view resource_type) {
  const std::string field_name =
      absl::StrCat(ToSnakeCase(resource_type), "_id");
  return reference.GetDescriptor()->FindFieldByName(field_name);
}

}  // namespace internal

}  // namespace fhir
}  // namespace google
