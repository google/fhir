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

#include "google/fhir/stu3/json_format.h"

#include <ctype.h>
#include <iosfwd>
#include <memory>
#include <unordered_map>
#include <utility>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/stu3/extensions.h"
#include "google/fhir/stu3/primitive_wrapper.h"
#include "google/fhir/stu3/util.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/google_extensions.pb.h"
#include "proto/stu3/resources.pb.h"
#include "include/json/json.h"
#include "tensorflow/core/lib/core/errors.h"
#include "re2/re2.h"

namespace google {
namespace fhir {
namespace stu3 {

namespace {

using ::google::fhir::Status;
using ::google::fhir::StatusOr;
using ::google::fhir::stu3::ClearTypedExtensions;
using ::google::fhir::stu3::IsChoiceType;
using ::google::fhir::stu3::IsPrimitive;
using ::google::fhir::stu3::IsReference;
using ::google::fhir::stu3::IsResource;
using ::google::fhir::stu3::ReferenceProtoToString;
using ::google::fhir::stu3::ReferenceStringToProto;
using ::google::fhir::stu3::proto::ContainedResource;
using ::google::fhir::stu3::proto::Extension;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using ::tensorflow::errors::InvalidArgument;

// Given a proto descriptor, constructs a map from FHIR JSON field name to
// FieldDescriptor.
// Since FHIR represents extensions to primitives as separate JSON fields,
// prepended by underscore, we add that as a separate mapping to the primitive
// field.
// TODO: memoize
std::unordered_map<string, const FieldDescriptor*> GetFieldMap(
    const Descriptor* descriptor) {
  std::unordered_map<string, const FieldDescriptor*> field_map;

  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    if (IsChoiceType(field)) {
      std::unordered_map<string, const FieldDescriptor*> inner_map =
          GetFieldMap(field->message_type());
      for (auto iter = inner_map.begin(); iter != inner_map.end(); iter++) {
        string inner_field_name = iter->first;
        inner_field_name[0] = std::toupper(inner_field_name[0]);
        field_map[absl::StrCat(field->json_name(), inner_field_name)] = field;
      }
    } else {
      field_map[field->json_name()] = field;
      if (field->type() == FieldDescriptor::TYPE_MESSAGE &&
          IsPrimitive(field->message_type())) {
        // Fhir JSON represents extensions to primitive fields as separate
        // standalone JSON objects, keyed by the "_" + field name.
        field_map["_" + field->json_name()] = field;
      }
    }
  }
  return field_map;
}

// Builds a map from ContainedResource field name to FieldDescriptor for that
// field.
std::unordered_map<string, const FieldDescriptor*>* BuildResourceTypeMap() {
  std::unordered_map<string, const FieldDescriptor*>* map =
      new std::unordered_map<string, const FieldDescriptor*>;
  for (int i = 0; i < ContainedResource::descriptor()->field_count(); i++) {
    const FieldDescriptor* field = ContainedResource::descriptor()->field(i);
    (*map)[field->message_type()->name()] = field;
  }
  return map;
}

static const std::unordered_map<string, const FieldDescriptor*>*
    kResourceTypeMap = BuildResourceTypeMap();

class Parser {
 public:
  explicit Parser(absl::TimeZone default_timezone)
      : default_timezone_(default_timezone) {}

  Status MergeMessage(const Json::Value& value, Message* target) {
    const Descriptor* target_descriptor = target->GetDescriptor();
    if (target_descriptor->full_name() ==
        ContainedResource::descriptor()->full_name()) {
      // We handle contained resources in a special way, because despite
      // internally being a Oneof, it is not acually a choice-type in FHIR. The
      // JSON field name is just "resource", which doesn't give us any clues
      // about which field in the Oneof to set.  Instead, we need to inspect
      // the JSON input to determine its type.  Then, merge into that specific
      // field in the resource Oneof.
      string resource_type =
          value.get("resourceType", Json::Value::null).asString();
      auto resource_field_iter = kResourceTypeMap->find(resource_type);
      if (resource_field_iter == kResourceTypeMap->end()) {
        return InvalidArgument("Unsupported resource type: ", resource_type);
      }
      return MergeMessage(value, target->GetReflection()->MutableMessage(
                                     target, resource_field_iter->second));
    }

    const std::unordered_map<string, const FieldDescriptor*> field_map =
        GetFieldMap(target_descriptor);

    for (auto sub_value_iter = value.begin(); sub_value_iter != value.end();
         ++sub_value_iter) {
      const auto& field_entry = field_map.find(sub_value_iter.key().asString());
      if (field_entry != field_map.end()) {
        if (field_entry->second->options().GetExtension(
                ::google::fhir::stu3::proto::is_choice_type)) {
          FHIR_RETURN_IF_ERROR(MergeChoiceField(*sub_value_iter,
                                                field_entry->second,
                                                field_entry->first, target));
        } else {
          FHIR_RETURN_IF_ERROR(
              MergeField(*sub_value_iter, field_entry->second, target));
        }
      } else if (sub_value_iter.key().asString() == "resourceType") {
        string resource_type = sub_value_iter->asString();
        if (!IsResource(target_descriptor) ||
            target_descriptor->name() != resource_type) {
          return InvalidArgument("Error merging json resource of type ",
                                 resource_type, " into message of type",
                                 target_descriptor->name());
        }
      } else {
        return InvalidArgument("Unable to merge field ",
                               absl::StrCat(sub_value_iter.key().asString()),
                               " into resource of type ",
                               target_descriptor->full_name());
      }
    }
    return Status::OK();
  }

  Status MergeChoiceField(const Json::Value& json,
                          const FieldDescriptor* choice_field,
                          const string& field_name, Message* parent) {
    const Descriptor* choice_type_descriptor = choice_field->message_type();
    string value_suffix = field_name.substr(choice_field->json_name().length());
    value_suffix[0] = std::tolower(value_suffix[0]);
    const auto choice_type_field_map = GetFieldMap(choice_type_descriptor);
    auto value_field_iter = choice_type_field_map.find(value_suffix);
    if (value_field_iter == choice_type_field_map.end()) {
      return InvalidArgument("Can't find ", value_suffix, " on ",
                             choice_field->full_name());
    }
    Message* choice_msg =
        parent->GetReflection()->MutableMessage(parent, choice_field);
    return MergeField(json, value_field_iter->second, choice_msg);
  }

  // Given a JSON value, field, and parent message, merges the FHIR JSON into
  // the given field on the parent.
  // Note that we cannot just pass the field message, as this behaves
  // differently if the field has been previously set or not.
  Status MergeField(const Json::Value& json, const FieldDescriptor* field,
                    Message* parent) {
    const Reflection* parent_reflection = parent->GetReflection();
    // If the field is non-primitive make sure it hasn't been set yet.
    // Note that we allow primitive types to be set already, because FHIR
    // represents extensions to primitives as separate, subsequent JSON
    // elements, with the field prepended by an underscore.  In #GetFieldMap
    // above, these were mapped to the same fields.
    if (!IsPrimitive(field->message_type()) &&
        !(field->is_repeated() &&
          parent_reflection->FieldSize(*parent, field) == 0) &&
        !(!field->is_repeated() &&
          !parent_reflection->HasField(*parent, field))) {
      return InvalidArgument("Target field already set: ", field->full_name(),
                             "\n", parent->DebugString(), "\n",
                             field->full_name(), "\n",
                             absl::StrCat(json.toStyledString()), "\n done");
    }

    if (field->containing_oneof()) {
      const ::google::protobuf::FieldDescriptor* oneof_field =
          parent_reflection->GetOneofFieldDescriptor(*parent,
                                                     field->containing_oneof());
      if (oneof_field) {
        return InvalidArgument(
            "Cannot set field ", field->full_name(), " because another field ",
            oneof_field->full_name(), " of the same oneof is already set.");
      }
    }

    if (field->is_repeated()) {
      if (!json.isArray()) {
        return InvalidArgument(
            "Attempted to set repeated field ", field->full_name(),
            " using non-array JSON: ", absl::StrCat(json.toStyledString()));
      }
      size_t existing_field_size = parent_reflection->FieldSize(*parent, field);
      if (existing_field_size != 0 && existing_field_size != json.size()) {
        return InvalidArgument(
            "Repeated primitive list length does not match extension list ",
            "for field: ", field->full_name());
      }
      for (Json::ArrayIndex i = 0; i < json.size(); i++) {
        FHIR_ASSIGN_OR_RETURN(
            std::unique_ptr<Message> parsed_value,
            ParseFieldValue(field, json.get(i, Json::Value::null), parent));
        if (existing_field_size > 0) {
          Message* field_value =
              parent_reflection->MutableRepeatedMessage(parent, field, i);
          field_value->MergeFrom(*parsed_value);
          // This is the second time we've visited this field - once for
          // extensions, and once for value.  So, make sure to clear the
          // PrimitiveHasNoValue extension.
          FHIR_RETURN_IF_ERROR(ClearPrimitiveHasNoValue(field_value));
        } else {
          parent_reflection->AddAllocatedMessage(parent, field,
                                                 parsed_value.release());
        }
      }
    } else {
      FHIR_ASSIGN_OR_RETURN(std::unique_ptr<Message> parsed_value,
                            ParseFieldValue(field, json, parent));
      if (parent_reflection->HasField(*parent, field)) {
        Message* field_value = parent_reflection->MutableMessage(parent, field);
        field_value->MergeFrom(*parsed_value);
        // This is the second time we've visited this field - once for
        // extensions, and once for value.  So, make sure to clear the
        // PrimitiveHasNoValue extension.
        FHIR_RETURN_IF_ERROR(ClearPrimitiveHasNoValue(field_value));
      } else {
        parent_reflection->SetAllocatedMessage(parent, parsed_value.release(),
                                               field);
      }
    }
    return Status::OK();
  }

  Status ClearPrimitiveHasNoValue(Message* message) {
    return ClearTypedExtensions(
        ::google::fhir::stu3::google::PrimitiveHasNoValue::descriptor(),
        message);
  }

  StatusOr<std::unique_ptr<Message>> ParseFieldValue(
      const FieldDescriptor* field, const Json::Value& json, Message* parent) {
    if (field->type() != FieldDescriptor::Type::TYPE_MESSAGE) {
      return InvalidArgument("Error in FHIR proto definition: Field ",
                             field->full_name(), " is not a message.");
    }
    std::unique_ptr<Message> target =
        absl::WrapUnique(parent->GetReflection()
                             ->GetMessageFactory()
                             ->GetPrototype(field->message_type())
                             ->New());
    FHIR_RETURN_IF_ERROR(MergeValue(json, target.get()));
    return std::move(target);
  }

  Status MergeValue(const Json::Value& json, Message* target) {
    if (IsPrimitive(target->GetDescriptor())) {
      if (json.isObject()) {
        // This is a primitive type extension.
        // Merge the extension fields into into the empty target proto,
        // and tag it as having no value.
        FHIR_RETURN_IF_ERROR(MergeMessage(json, target));
        target->GetReflection()
            ->AddMessage(target,
                         target->GetDescriptor()->FindFieldByName("extension"))
            ->CopyFrom(*GetPrimitiveHasNoValueExtension());
        return Status::OK();
      } else {
        FHIR_RETURN_IF_ERROR(ParseInto(json, default_timezone_, target));
        return Status::OK();
      }
    } else if (IsReference(target->GetDescriptor())) {
      ::google::fhir::stu3::proto::Reference reference;
      FHIR_RETURN_IF_ERROR(MergeMessage(json, &reference));

      // Proto references support more structure than json references.
      // Serialize and deserialize to get the most structured version
      // possible.
      StatusOr<string> reference_string = ReferenceProtoToString(reference);
      if (!reference_string.ok()) {
        // This code path is taken for References which don't have "reference"
        // set, such as references with only a "display" field.
        target->MergeFrom(reference);
        return Status::OK();
      } else {
        string reference_value = reference_string.ValueOrDie();
        FHIR_ASSIGN_OR_RETURN(
            ::google::fhir::stu3::proto::Reference parsed_reference,
            ReferenceStringToProto(reference_value));
        if (reference.has_display()) {
          parsed_reference.mutable_display()->set_value(
              reference.display().value());
        }
        if (reference.has_id()) {
          parsed_reference.mutable_id()->set_value(reference.id().value());
        }
        for (const Extension& extension : reference.extension()) {
          *(parsed_reference.add_extension()) = extension;
        }
        target->MergeFrom(parsed_reference);
        return Status::OK();
      }
    }
    // Must be another FHIR element.
    if (!json.isObject()) {
      return InvalidArgument("Expected JsonObject for field of type ",
                             target->GetDescriptor()->full_name());
    }
    FHIR_RETURN_IF_ERROR(MergeMessage(json, target));
    return Status::OK();
  }

 private:
  absl::TimeZone default_timezone_;
};

StatusOr<Json::Value> ParseJsonValue(const string& raw_json) {
  Json::Reader reader;
  Json::Value value;
  if (!reader.parse(raw_json, value)) {
    return InvalidArgument("Failed parsing raw json: ", raw_json);
  }
  return value;
}

}  // namespace

Status MergeJsonFhirStringIntoProto(const string& raw_json, Message* target,
                                    const absl::TimeZone default_timezone) {
  string mutable_raw_json = raw_json;
  // FHIR JSON format stores decimals as unquoted rational numbers.  This is
  // problematic, because their representation could change when they are
  // parsed into C++ doubles.  To avoid this, add quotes around any decimal
  // fields to ensure that they are parsed as strings.  Note that any field
  // that is already in quotes will not match this regex, and thus be ignored.
  //
  // This regex is three capture groups:
  // 1: a un-escaped double-quote followed by a colon and arbitrary whitespace,
  //    to ensure this is a field value (and not inside a string).
  // 2: a decimal
  // 3: any field-ending token.
  static const LazyRE2 kDecimalKeyValuePattern{
      "(?m)([^\\\\]\":\\s*)(-?\\d*\\.\\d*?)([\\s,\\}\\]$])"};
  RE2::GlobalReplace(&mutable_raw_json, *kDecimalKeyValuePattern,
                     "\\1\"\\2\"\\3");

  Json::Value value;

  // TODO: Decide if we want to support value-only JSON
  if (target->GetDescriptor()->full_name() ==
          proto::Decimal::descriptor()->full_name() &&
      raw_json != "null") {
    // Similar to above, if this is a standalone decimal, parse it as a string
    // to avoid changing reprentation due to precision.
    FHIR_ASSIGN_OR_RETURN(
        value, ParseJsonValue(absl::StrCat("\"", mutable_raw_json, "\"")));
  } else {
    FHIR_ASSIGN_OR_RETURN(value, ParseJsonValue(mutable_raw_json));
  }

  Parser parser{default_timezone};
  return parser.MergeValue(value, target);
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
