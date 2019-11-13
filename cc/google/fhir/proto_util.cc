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

#include "google/fhir/proto_util.h"

#include <cstddef>
#include <iterator>
#include <string>
#include <vector>

#include "absl/strings/str_split.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "re2/re2.h"

namespace google {
namespace fhir {


using ::google::fhir::StatusOr;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::tensorflow::Status;
using ::tensorflow::errors::InvalidArgument;

namespace {

enum class EmptyFieldsBehavior { ADD_DEFAULT, RETURN_NOT_FOUND };

// Internal version accepts a third argument specifying how to handle empty
// fields.  This allows a common implementation between the mutable and
// non-mutable versions.
// If RETURN_NOT_FOUND, this will return a NOT_FOUND if any part of the path is
// not set.
// If ADD_DEFAULT, this will return a default message.
StatusOr<Message*> GetSubmessageByPathInternal(
    Message* message, const std::string& field_path,
    const EmptyFieldsBehavior empty_fields_behavior) {
  const std::string& message_name = message->GetDescriptor()->name();
  std::vector<std::string> tokens = absl::StrSplit(field_path, '.');
  if (message_name != tokens[0]) {
    return InvalidArgument(absl::StrCat("Cannot find ", field_path, " in ",
                                        message_name,
                                        ": invalid top-level resource."));
  }

  Message* submessage = message;
  // Skip first token, as that just describes the top-level resource type
  for (auto token_iter = std::next(tokens.begin()); token_iter != tokens.end();
       token_iter++) {
    const auto* subfield =
        submessage->GetDescriptor()->FindFieldByCamelcaseName(*token_iter);
    if (subfield != nullptr) {
      // We found a field with this name.
      if (subfield->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
        return InvalidArgument(
            absl::StrCat("Cannot resolve field path ", field_path, ": field ",
                         subfield->full_name(), " is not a message."));
      }
      if (subfield->is_repeated()) {
        return InvalidArgument(
            absl::StrCat("Found repeated field with no index: ", field_path));
      }

      // If we're not allowing empty fields, make sure the field not only
      // exists, but is populated.
      if (empty_fields_behavior == EmptyFieldsBehavior::RETURN_NOT_FOUND &&
          !submessage->GetReflection()->HasField(*submessage, subfield)) {
        return ::tensorflow::errors::NotFound(
            absl::StrCat("Field ", field_path, " is empty"));
      }
      submessage =
          submessage->GetReflection()->MutableMessage(submessage, subfield);
    } else {
      // No field by that name.
      // Check if there's an index into a repeated field.
      // We don't do this off the bat, to avoid doing the regex check unless we
      // have to.
      int index;
      if (!EndsInIndex(*token_iter, &index)) {
        // The field was not found, either as a singular or indexed repeated
        // field.
        return InvalidArgument(
            absl::StrCat("Cannot find field ", *token_iter, " in ",
                         submessage->GetDescriptor()->full_name()));
      }
      subfield = submessage->GetDescriptor()->FindFieldByCamelcaseName(
          StripIndex(*token_iter));
      if (subfield == nullptr) {
        return InvalidArgument(
            absl::StrCat("Invalid field path: ", field_path));
      }
      if (!subfield->is_repeated()) {
        return InvalidArgument(absl::StrCat(
            "Tried to index into non-repeated field: ", field_path));
      }
      const auto* submessage_reflection = submessage->GetReflection();
      int field_size = submessage_reflection->FieldSize(*submessage, subfield);
      if (field_size <= index) {
        return ::tensorflow::errors::OutOfRange(absl::StrCat(
            "Out of range index on repeated field.  Field: ", field_path,
            "  Size: ", field_size, "  Index: ", index));
      }
      submessage = submessage->GetReflection()->MutableRepeatedMessage(
          submessage, subfield, index);
    }
  }
  return submessage;
}

}  //  namespace

bool EndsInIndex(const std::string& field_path, int* index) {
  static LazyRE2 re{R"(\[([0-9]+)]$)"};
  return RE2::PartialMatch(field_path, *re, index);
}

bool EndsInIndex(const std::string& field_path) {
  int index;
  return EndsInIndex(field_path, &index);
}

std::string StripIndex(const std::string& field_path) {
  return field_path.substr(0, field_path.find_last_of('['));
}
StatusOr<const bool> HasSubmessageByPath(const Message& message,
                                         const std::string& field_path) {
  const Status& status = GetSubmessageByPath(message, field_path).status();
  if (status.code() == ::tensorflow::error::Code::INVALID_ARGUMENT) {
    return status;
  }
  return status.ok();
}

StatusOr<Message*> GetMutableSubmessageByPath(Message* message,
                                              const std::string& field_path) {
  return GetSubmessageByPathInternal(message, field_path,
                                     EmptyFieldsBehavior::ADD_DEFAULT);
}

StatusOr<const Message*> GetSubmessageByPath(const Message& message,
                                             const std::string& field_path) {
  auto got =
      GetSubmessageByPathInternal(&(const_cast<Message&>(message)), field_path,
                                  EmptyFieldsBehavior::RETURN_NOT_FOUND);
  TF_RETURN_IF_ERROR(got.status());
  return const_cast<const Message*>(got.ValueOrDie());
}

Status ClearFieldByPath(Message* message, const std::string& field_path) {
  if (EndsInIndex(field_path)) {
    return InvalidArgument(
        absl::StrCat("Cannot clear indexed repeated field: ", field_path));
  }
  // Get parent message, so we can clear the leaf field from it.
  const std::size_t last_dot_index = field_path.find_last_of('.');
  const std::string parent_path = field_path.substr(0, last_dot_index);
  const std::string field_name = field_path.substr(last_dot_index + 1);
  // First check if the parent message exists, to avoid adding an empty
  // parent message.
  auto got = HasSubmessageByPath(*message, parent_path);
  TF_RETURN_IF_ERROR(got.status());
  const bool has_submessage = got.ValueOrDie();
  if (!has_submessage) {
    return Status::OK();
  }
  auto parent_got = GetMutableSubmessageByPath(message, parent_path);
  TF_RETURN_IF_ERROR(parent_got.status());
  Message* parent_message = parent_got.ValueOrDie();
  const auto* parent_reflection = parent_message->GetReflection();
  const auto* field_descriptor =
      parent_message->GetDescriptor()->FindFieldByCamelcaseName(field_name);
  if (field_descriptor == nullptr) {
    return InvalidArgument(absl::StrCat("Invalid field path: ", field_path));
  }
  parent_reflection->ClearField(parent_message, field_descriptor);
  return Status::OK();
}

Message* MutableOrAddMessage(Message* message, const FieldDescriptor* field) {
  if (field->is_repeated()) {
    return message->GetReflection()->AddMessage(message, field);
  }
  return message->GetReflection()->MutableMessage(message, field);
}

bool FieldHasValue(const Message& message, const FieldDescriptor* field) {
  return PotentiallyRepeatedFieldSize(message, field) > 0;
}

bool FieldHasValue(const Message& message, const std::string& field_name) {
  return FieldHasValue(message,
                       message.GetDescriptor()->FindFieldByName(field_name));
}

int PotentiallyRepeatedFieldSize(const Message& message,
                                 const FieldDescriptor* field) {
  if (field->is_repeated()) {
    return message.GetReflection()->FieldSize(message, field);
  }
  return message.GetReflection()->HasField(message, field) ? 1 : 0;
}

const Message& GetPotentiallyRepeatedMessage(const Message& message,
                                             const FieldDescriptor* field,
                                             const int index) {
  if (field->is_repeated()) {
    return message.GetReflection()->GetRepeatedMessage(message, field, index);
  }
  DCHECK_EQ(index, 0) << "GetPotentiallyRepeatedMessage called on singular "
                         "field with index not equal to 0";
  return message.GetReflection()->GetMessage(message, field);
}

Message* MutablePotentiallyRepeatedMessage(Message* message,
                                           const FieldDescriptor* field,
                                           const int index) {
  if (field->is_repeated()) {
    return message->GetReflection()->MutableRepeatedMessage(message, field,
                                                            index);
  }
  DCHECK_EQ(index, 0) << "MutablePotentiallyRepeatedMessage called on singular "
                         "field with index > 0";
  return message->GetReflection()->MutableMessage(message, field);
}

bool AreSameMessageType(const Message& a, const Message& b) {
  return AreSameMessageType(a.GetDescriptor(), b.GetDescriptor());
}

bool AreSameMessageType(const Descriptor* a, const Descriptor* b) {
  return a == b || a->full_name() == b->full_name();
}

Status CopyCommonField(const Message& source, Message* target,
                       const std::string& field_name) {
  const Descriptor* source_descriptor = source.GetDescriptor();
  const Descriptor* target_descriptor = target->GetDescriptor();

  const FieldDescriptor* source_field =
      source_descriptor->FindFieldByName(field_name);
  const FieldDescriptor* target_field =
      target_descriptor->FindFieldByName(field_name);

  if (!source_field || !target_field ||
      source_field->message_type()->full_name() !=
          target_field->message_type()->full_name() ||
      (source_field->is_repeated() != target_field->is_repeated())) {
    return InvalidArgument("Error in CopyCommonField: Field ", field_name,
                           " is not present in both ",
                           source_descriptor->full_name(), " and ",
                           target_descriptor->full_name(),
                           ", or they are not the same type and size.");
  }

  ForEachMessage<Message>(source, source_field, [&](const Message& message) {
    MutableOrAddMessage(target, target_field)->CopyFrom(message);
  });
  return Status::OK();
}

Status ClearField(Message* message, const std::string& field_name) {
  const Descriptor* descriptor = message->GetDescriptor();
  const FieldDescriptor* field = descriptor->FindFieldByName(field_name);

  if (!field) {
    return InvalidArgument("Error in ClearField:  ", descriptor->full_name(),
                           " has no field ", field_name);
  }
  message->GetReflection()->ClearField(message, field);
  return Status::OK();
}

}  // namespace fhir
}  // namespace google
