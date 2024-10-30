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

#include <ctype.h>

#include <cctype>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "google/protobuf/any.pb.h"
#include "absl/base/attributes.h"
#include "absl/base/const_init.h"
#include "absl/container/flat_hash_map.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "google/fhir/annotations.h"
#include "google/fhir/core_resource_registry.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/extensions.h"
#include "google/fhir/json/fhir_json.h"
#include "google/fhir/json/json_sax_handler.h"
#include "google/fhir/json_format.h"
#include "google/fhir/json_format_results.h"
#include "google/fhir/json/json_util.h"
#include "google/fhir/primitive_wrapper.h"
#include "google/fhir/r4/profiles.h"
#include "google/fhir/references.h"
#include "google/fhir/resource_validation.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/google/fhir/proto/annotations.pb.h"

namespace google {
namespace fhir {

using ::absl::InvalidArgumentError;
using ::google::fhir::IsChoiceType;
using ::google::fhir::IsPrimitive;
using ::google::fhir::IsReference;
using ::google::fhir::IsResource;
using ::google::protobuf::Any;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

namespace internal {

// Given a proto descriptor, constructs a map from FHIR JSON field name to
// FieldDescriptor.
// Since FHIR represents extensions to primitives as separate JSON fields,
// prepended by underscore, we add that as a separate mapping to the primitive
// field.
std::unique_ptr<const std::unordered_map<std::string, const FieldDescriptor*>>
MakeFieldMap(const Descriptor* descriptor) {
  auto field_map = std::make_unique<
      std::unordered_map<std::string, const FieldDescriptor*>>();
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    absl::string_view json_name = FhirJsonName(field);
    if (IsChoiceType(field)) {
      std::unique_ptr<
          const std::unordered_map<std::string, const FieldDescriptor*>>
          inner_map = MakeFieldMap(field->message_type());
      for (auto iter = inner_map->begin(); iter != inner_map->end(); iter++) {
        std::string child_field_name = iter->first;
        if (child_field_name[0] == '_') {
          // Convert primitive extension field name to field on choice type,
          // e.g., value + _boolean -> _valueBoolean for Extension.value.
          child_field_name[1] = std::toupper(child_field_name[1]);
          (*field_map)[absl::StrCat("_", json_name,
                                    child_field_name.substr(1))] = field;
        } else {
          // For non-primitive, just append them together as camelcase, e.g.,
          // value + boolean = valueBoolean
          child_field_name[0] = std::toupper(child_field_name[0]);
          (*field_map)[absl::StrCat(json_name, child_field_name)] = field;
        }
      }
    } else {
      (*field_map)[std::string(json_name)] = field;

      if (field->type() == FieldDescriptor::TYPE_MESSAGE &&
          IsPrimitive(field->message_type())) {
        // Fhir JSON represents extensions to primitive fields as separate
        // standalone JSON objects, keyed by the "_" + field name.
        (*field_map)[absl::StrCat("_", json_name)] = field;
      }
    }
  }
  return field_map;
}

// Gets a field map for a given descriptor.
// This memoizes the results of MakeFieldMap.
const std::unordered_map<std::string, const FieldDescriptor*>& GetFieldMap(
    const Descriptor* descriptor) {
  // Note that we memoize on descriptor address, since the values include
  // FieldDescriptor addresses, which will only be valid for a given address
  // of input descriptor
  static auto* memos =
      new absl::flat_hash_map<intptr_t,
                              std::unique_ptr<const std::unordered_map<
                                  std::string, const FieldDescriptor*>>>();
  ABSL_CONST_INIT static absl::Mutex memos_mutex(absl::kConstInit);

  const intptr_t memo_key = reinterpret_cast<intptr_t>(descriptor);

  {
    absl::ReaderMutexLock reader_lock(&memos_mutex);
    const auto iter = memos->find(memo_key);
    if (iter != memos->end()) {
      return *iter->second;
    }
  }

  absl::MutexLock lock(&memos_mutex);

  // Check if anything created and wrote the new entry while we were waiting
  // on the lock
  const auto inside_lock_iter = memos->find(memo_key);
  if (inside_lock_iter != memos->end()) {
    return *inside_lock_iter->second;
  }

  // There's still no memo, and we're holding the lock.  Write a new entry.
  (*memos)[memo_key] = MakeFieldMap(descriptor);
  return *(*memos)[memo_key];
}

typedef std::unordered_map<std::string, const FieldDescriptor*> FieldMap;

// Builds a map from ContainedResource field type to FieldDescriptor for that
// field.
std::unique_ptr<FieldMap> BuildResourceTypeMap(const Descriptor* descriptor) {
  auto map = std::make_unique<FieldMap>();
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    (*map)[std::string(field->message_type()->name())] = field;
  }
  return map;
}

absl::StatusOr<const FieldDescriptor*> GetContainedResourceField(
    const Descriptor* contained_resource_desc,
    const std::string& resource_type) {
  static absl::Mutex field_table_mutex(absl::kConstInit);
  static std::unordered_map<std::string, std::unique_ptr<FieldMap>>*
      field_table =
          new std::unordered_map<std::string, std::unique_ptr<FieldMap>>
              ABSL_GUARDED_BY(field_table_mutex);

  const std::string contained_resource_name(
      contained_resource_desc->full_name());
  {
    absl::ReaderMutexLock reader_lock(&field_table_mutex);
    auto field_table_iter = field_table->find(contained_resource_name);
    if (field_table_iter != field_table->end()) {
      const FieldDescriptor* field = (*field_table_iter->second)[resource_type];
      if (!field) {
        return InvalidArgumentError(absl::StrCat("No field on ",
                                                 contained_resource_name,
                                                 " with type ", resource_type));
      }
      return field;
    }
  }

  absl::MutexLock lock(&field_table_mutex);
  auto field_map = BuildResourceTypeMap(contained_resource_desc);
  const FieldDescriptor* field = (*field_map)[resource_type];
  (*field_table)[contained_resource_name] = std::move(field_map);

  if (!field) {
    return InvalidArgumentError(absl::StrCat(
        "No field on ", contained_resource_name, " with type ", resource_type));
  }
  return field;
}

class Parser {
 public:
  explicit Parser(const PrimitiveHandler* primitive_handler,
                  absl::TimeZone default_timezone)
      : primitive_handler_(primitive_handler),
        default_timezone_(default_timezone) {}

  absl::StatusOr<ParseResult> MergeMessage(
      const internal::FhirJson& value, Message* target,
      const ScopedErrorReporter& error_reporter) const {
    const Descriptor* target_descriptor = target->GetDescriptor();
    // TODO(b/244184211): handle this with an annotation
    if (target_descriptor->name() == "ContainedResource") {
      return MergeContainedResource(value, target, error_reporter);
    }

    const std::unordered_map<std::string, const FieldDescriptor*>& field_map =
        GetFieldMap(target_descriptor);

    FHIR_ASSIGN_OR_RETURN(auto object_map, value.objectMap());

    bool all_values_parsed_successfully = true;
    for (auto sub_value_iter = object_map->begin();
         sub_value_iter != object_map->end(); ++sub_value_iter) {
      const auto& field_entry = field_map.find(sub_value_iter->first);
      if (field_entry != field_map.end()) {
        if (IsChoiceType(field_entry->second)) {
          FHIR_ASSIGN_OR_RETURN(
              ParseResult result,
              MergeChoiceField(*sub_value_iter->second, field_entry->second,
                               field_entry->first, target, error_reporter));
          if (result == ParseResult::kFailed) {
            all_values_parsed_successfully = false;
          }
        } else {
          FHIR_ASSIGN_OR_RETURN(
              ParseResult result,
              MergeField(*sub_value_iter->second, field_entry->second, target,
                         error_reporter));
          if (result == ParseResult::kFailed) {
            all_values_parsed_successfully = false;
          }
        }
      } else if (sub_value_iter->first == "resourceType") {
        FHIR_ASSIGN_OR_RETURN(std::string resource_type,
                              sub_value_iter->second->asString());
        if (!IsResource(target_descriptor) ||
            target_descriptor->name() != resource_type) {
          FHIR_RETURN_IF_ERROR(
              error_reporter.ReportFhirFatal(InvalidArgumentError(absl::StrCat(
                  "Error merging json resource of type ", resource_type,
                  " into message of type", target_descriptor->name()))));
          all_values_parsed_successfully = false;
        }
      } else {
        if (sub_value_iter->first == "fhir_comments") {
          // fhir_comments can exist in a valid FHIR json, however,
          // it is not supported in the current FHIR protos.
          // Hence, we simply ignore it.
          continue;
        }

        FHIR_RETURN_IF_ERROR(
            error_reporter.ReportFhirFatal(InvalidArgumentError(
                absl::Substitute("No field `$0` in $1", sub_value_iter->first,
                                 target_descriptor->name()))));
        all_values_parsed_successfully = false;
      }
    }
    return all_values_parsed_successfully ? ParseResult::kSucceeded
                                          : ParseResult::kFailed;
  }

  absl::StatusOr<ParseResult> MergeContainedResource(
      const internal::FhirJson& value, Message* target,
      const ScopedErrorReporter& error_reporter) const {
    // We handle contained resources in a special way, because despite
    // internally being a Oneof, it is not acually a choice-type in FHIR. The
    // JSON field name is just "resource", which doesn't give us any clues
    // about which field in the Oneof to set.  Instead, we need to inspect
    // the JSON input to determine its type.  Then, merge into that specific
    // field in the resource Oneof.
    FHIR_ASSIGN_OR_RETURN(const internal::FhirJson* resource_type_json,
                          value.get("resourceType"));
    FHIR_ASSIGN_OR_RETURN(const std::string resource_type_str,
                          resource_type_json->asString());
    FHIR_ASSIGN_OR_RETURN(
        const FieldDescriptor* contained_field,
        GetContainedResourceField(target->GetDescriptor(), resource_type_str));
    return MergeMessage(
        value, target->GetReflection()->MutableMessage(target, contained_field),
        error_reporter);
  }

  absl::StatusOr<ParseResult> MergeChoiceField(
      const internal::FhirJson& json, const FieldDescriptor* choice_field,
      absl::string_view field_name, Message* parent,
      const ScopedErrorReporter& error_reporter) const {
    const Descriptor* choice_type_descriptor = choice_field->message_type();
    const auto& choice_type_field_map = GetFieldMap(choice_type_descriptor);
    std::string choice_field_name = std::string(field_name);
    if (field_name[0] == '_') {
      // E.g., _valueBoolean -> boolean
      choice_field_name = absl::StrCat(
          "_",
          choice_field_name.substr(1 + choice_field->json_name().length()));
      choice_field_name[1] = std::tolower(choice_field_name[1]);
    } else {
      // E.g., valueBoolean -> boolean
      choice_field_name =
          choice_field_name.substr(choice_field->json_name().length());
      choice_field_name[0] = std::tolower(choice_field_name[0]);
    }
    auto value_field_iter = choice_type_field_map.find(choice_field_name);
    if (value_field_iter == choice_type_field_map.end()) {
      FHIR_RETURN_IF_ERROR(
          error_reporter.ReportFhirFatal(absl::InvalidArgumentError(
              absl::StrCat("Can't find ", choice_field_name, " on ",
                           choice_field->full_name()))));
      return ParseResult::kFailed;
    }
    Message* choice_msg =
        parent->GetReflection()->MutableMessage(parent, choice_field);
    return MergeField(json, value_field_iter->second, choice_msg,
                      error_reporter);
  }

  // Given a JSON value, field, and parent message, merges the FHIR JSON into
  // the given field on the parent.
  // Note that we cannot just pass the field message, as this behaves
  // differently if the field has been previously set or not.
  absl::StatusOr<ParseResult> MergeField(
      const internal::FhirJson& json, const FieldDescriptor* field,
      Message* parent, const ScopedErrorReporter& error_reporter) const {
    const Reflection* parent_reflection = parent->GetReflection();
    // If the field is non-primitive make sure it hasn't been set yet.
    // Note that we allow primitive types to be set already, because FHIR
    // represents extensions to primitives as separate, subsequent JSON
    // elements, with the field prepended by an underscore.  In #GetFieldMap
    // above, these were mapped to the same fields.
    if (!IsPrimitive(field->message_type())) {
      if (!(field->is_repeated() &&
            parent_reflection->FieldSize(*parent, field) == 0) &&
          !(!field->is_repeated() &&
            !parent_reflection->HasField(*parent, field))) {
        FHIR_RETURN_IF_ERROR(
            error_reporter.ReportFhirFatal(InvalidArgumentError(absl::StrCat(
                "Target field already set: ", field->full_name()))));
        return ParseResult::kFailed;
      }
    }

    if (field->containing_oneof()) {
      const ::google::protobuf::FieldDescriptor* oneof_field =
          parent_reflection->GetOneofFieldDescriptor(*parent,
                                                     field->containing_oneof());
      // Consider it an error to try to set a field in a oneof if one is already
      // set.
      // Exception: When a primitive in a choice type has a value and an
      // extension, it will get set twice, once by the value (e.g.,
      // valueString), and once by an extension (e.g., _valueString).
      if (oneof_field && !(IsPrimitive(field->message_type()) &&
                           oneof_field->full_name() == field->full_name())) {
        FHIR_RETURN_IF_ERROR(
            error_reporter.ReportFhirFatal(InvalidArgumentError(absl::StrCat(
                "Cannot set field ", field->full_name(),
                " because another field ", oneof_field->full_name(),
                " of the same oneof is already set."))));
        return ParseResult::kFailed;
      }
    }

    if (field->is_repeated()) {
      if (!json.isArray()) {
        FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirFatal(
            InvalidArgumentError(
                "Attempted to set repeated field using non-array JSON."),
            field->full_name()));
        return ParseResult::kFailed;
      }
      size_t existing_field_size = parent_reflection->FieldSize(*parent, field);
      if (existing_field_size != 0 &&
          existing_field_size != json.arraySize().value()) {
        LOG(ERROR) << existing_field_size << " : " << json.arraySize().value();
        FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirFatal(
            InvalidArgumentError(
                "Repeated primitive list length does not match extension list"),
            field->full_name()));
        return ParseResult::kFailed;
      }

      ParseResult overall_result = ParseResult::kSucceeded;
      for (int i = 0; i < json.arraySize().value(); i++) {
        FHIR_ASSIGN_OR_RETURN(
            std::unique_ptr<Message> parsed_value,
            ParseFieldValue(field, *json.get(i).value(), parent,
                            error_reporter.WithScope(field, i)));
        if (parsed_value == nullptr) {
          overall_result = ParseResult::kFailed;
          continue;
        }
        if (existing_field_size > 0) {
          FHIR_RETURN_IF_ERROR(MergeAndClearPrimitiveWithNoValue(
              *parsed_value,
              parent_reflection->MutableRepeatedMessage(parent, field, i)));
        } else {
          parent_reflection->AddAllocatedMessage(parent, field,
                                                 parsed_value.release());
        }
      }
      return overall_result;
    } else {
      FHIR_ASSIGN_OR_RETURN(std::unique_ptr<Message> parsed_value,
                            ParseFieldValue(field, json, parent,
                                            error_reporter.WithScope(field)));
      if (parsed_value == nullptr) {
        return ParseResult::kFailed;
      }
      if (parent_reflection->HasField(*parent, field)) {
        FHIR_RETURN_IF_ERROR(MergeAndClearPrimitiveWithNoValue(
            *parsed_value, parent_reflection->MutableMessage(parent, field)));
      } else {
        parent_reflection->SetAllocatedMessage(parent, parsed_value.release(),
                                               field);
      }
    }
    return ParseResult::kSucceeded;
  }

  absl::Status AddPrimitiveHasNoValueExtension(Message* message) const {
    Message* extension = message->GetReflection()->AddMessage(
        message, message->GetDescriptor()->FindFieldByName("extension"));
    return BuildHasNoValueExtension(extension);
  }

  absl::Status MergeAndClearPrimitiveWithNoValue(
      const Message& parsed_value, Message* field_to_modify) const {
    field_to_modify->MergeFrom(parsed_value);
    // This is the second time we've visited this field - once for
    // extensions, and once for value.  So, make sure to clear the
    // PrimitiveHasNoValue extension.
    return ClearExtensionsWithUrl(primitives_internal::kPrimitiveHasNoValueUrl,
                                  field_to_modify);
  }

  absl::StatusOr<std::unique_ptr<Message>> ParseFieldValue(
      const FieldDescriptor* field, const internal::FhirJson& json,
      Message* parent, const ScopedErrorReporter& error_reporter) const {
    if (field->type() != FieldDescriptor::Type::TYPE_MESSAGE) {
      FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirFatal(InvalidArgumentError(
          absl::StrCat("Error in FHIR proto definition: Field ",
                       field->full_name(), " is not a message."))));
      return nullptr;
    }
    if (field->message_type()->full_name() == Any::descriptor()->full_name()) {
      std::unique_ptr<Message> contained =
          absl::WrapUnique(primitive_handler_->NewContainedResource());
      FHIR_ASSIGN_OR_RETURN(
          ParseResult result,
          MergeContainedResource(json, contained.get(), error_reporter));
      if (result == ParseResult::kFailed) {
        return nullptr;
      }
      Any* any = new Any;
      any->PackFrom(*contained);
      return absl::WrapUnique<Message>(any);
    } else {
      std::unique_ptr<Message> target =
          absl::WrapUnique(parent->GetReflection()
                               ->GetMessageFactory()
                               ->GetPrototype(field->message_type())
                               ->New());
      FHIR_ASSIGN_OR_RETURN(ParseResult result,
                            MergeValue(json, target.get(), error_reporter));
      if (result == ParseResult::kFailed) {
        return nullptr;
      }
      return std::move(target);
    }
  }

  absl::StatusOr<ParseResult> MergeValue(
      const internal::FhirJson& json, Message* target,
      const ScopedErrorReporter& error_reporter) const {
    if (IsPrimitive(target->GetDescriptor())) {
      if (json.isObject()) {
        // This is a primitive type extension.
        // Merge the extension fields into into the empty target proto,
        // and tag it as having no value.
        FHIR_ASSIGN_OR_RETURN(ParseResult result,
                              MergeMessage(json, target, error_reporter));
        if (result == ParseResult::kFailed) {
          return ParseResult::kFailed;
        }
        FHIR_RETURN_IF_ERROR(
            BuildHasNoValueExtension(target->GetReflection()->AddMessage(
                target,
                target->GetDescriptor()->FindFieldByName("extension"))));
        return ParseResult::kSucceeded;
      } else {
        return primitive_handler_->ParseInto(json, default_timezone_, target,
                                             error_reporter);
      }
    } else if (IsReference(target->GetDescriptor())) {
      FHIR_ASSIGN_OR_RETURN(ParseResult result,
                            MergeMessage(json, target, error_reporter));
      if (result == ParseResult::kFailed) {
        return ParseResult::kFailed;
      }
      FHIR_RETURN_IF_ERROR(SplitIfRelativeReference(target));
      return ParseResult::kSucceeded;
    }
    // Must be another FHIR element.
    if (!json.isObject()) {
      if (json.isArray() && json.arraySize().value() == 1) {
        // The target field is non-repeated, and we're trying to populate it
        // with a single element array.
        // This is considered valid, and occurs when a profiled resource reduces
        // the size of a repeated FHIR field to max of 1.
        return MergeMessage(*json.get(0).value(), target, error_reporter);
      }
      FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirFatal(InvalidArgumentError(
          absl::StrCat("Expected JsonObject for field of type ",
                       target->GetDescriptor()->full_name()))));
      return ParseResult::kFailed;
    }
    return MergeMessage(json, target, error_reporter);
  }

 private:
  const PrimitiveHandler* primitive_handler_;
  const absl::TimeZone default_timezone_;
};

}  // namespace internal

absl::StatusOr<ParseResult> Parser::MergeJsonFhirStringIntoProto(
    const absl::string_view raw_json, Message* target,
    const absl::TimeZone default_timezone, const bool validate,
    ErrorHandler& error_handler) const {
  internal::FhirJson json_object;
  FHIR_RETURN_IF_ERROR(internal::ParseJsonValue(raw_json, json_object));

  return MergeJsonFhirObjectIntoProto(json_object, target, default_timezone,
                                      validate, error_handler);
}

absl::StatusOr<ParseResult> Parser::MergeJsonFhirObjectIntoProto(
    const internal::FhirJson& json_object, Message* target,
    const absl::TimeZone default_timezone, const bool validate,
    ErrorHandler& error_handler) const {
  ScopedErrorReporter error_reporter(&error_handler,
                                     target->GetDescriptor()->name());

  internal::Parser parser{primitive_handler_, default_timezone};

  // If the target is a profiled resource, first parse to the base resource,
  // and then profile to the target type.
  // Note that we do not do this for primitive profiled types like Code,
  // since those are handled directly by the primitive wrappers.
  if (IsProfile(target->GetDescriptor()) &&
      !IsPrimitive(target->GetDescriptor())) {
    if (GetFhirVersion(*target) != proto::R4) {
      return InvalidArgumentError(
          absl::StrCat("Unsupported FHIR Version for profiling for resource: ",
                       target->GetDescriptor()->full_name()));
    }
    FHIR_ASSIGN_OR_RETURN(std::unique_ptr<Message> core_resource,
                          GetBaseResourceInstance(*target));

    FHIR_ASSIGN_OR_RETURN(
        ParseResult result,
        parser.MergeValue(json_object, core_resource.get(), error_reporter));
    if (result == ParseResult::kFailed) {
      return ParseResult::kFailed;
    }

    absl::Status profile_status =
        validate ? ConvertToProfileR4(*core_resource, target)
                 : ConvertToProfileLenientR4(*core_resource, target);

    if (profile_status.ok()) {
      return ParseResult::kSucceeded;
    }
    FHIR_RETURN_IF_ERROR(error_reporter.ReportFhirFatal(profile_status));
    return ParseResult::kFailed;
  }

  FHIR_ASSIGN_OR_RETURN(ParseResult result,
                        parser.MergeValue(json_object, target, error_reporter));

  if (validate) {
    // TODO(b/221105249): Use FHIRPath validation here.
    FHIR_RETURN_IF_ERROR(
        ValidateWithoutFhirPath(*target, primitive_handler_, error_handler));
  }
  return result;
}

}  // namespace fhir
}  // namespace google
