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
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/stu3/primitive_wrapper.h"
#include "google/fhir/stu3/util.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "include/json/json.h"

namespace google {
namespace fhir {
namespace stu3 {

namespace {

using ::google::fhir::Status;
using ::google::fhir::StatusOr;
using ::google::fhir::stu3::IsChoiceType;
using ::google::fhir::stu3::IsPrimitive;
using ::google::fhir::stu3::IsReference;
using ::google::fhir::stu3::ReferenceProtoToString;
using ::google::fhir::stu3::proto::ContainedResource;
using ::google::fhir::stu3::proto::Reference;
using ::google::fhir::stu3::proto::ReferenceId;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using ::tensorflow::errors::InvalidArgument;

class Printer {
 public:
  Printer(absl::TimeZone default_timezone, int indent_size, bool add_newlines,
          bool for_analytics)
      : default_timezone_(default_timezone),
        indent_size_(indent_size),
        add_newlines_(add_newlines),
        for_analytics_(for_analytics) {}

  StatusOr<string> WriteMessage(const google::protobuf::Message& message) {
    output_.clear();
    current_indent_ = 0;
    FHIR_RETURN_IF_ERROR(PrintNonPrimitive(message));
    return output_;
  }

 private:
  void OpenJsonObject() {
    output_ += "{";
    Indent();
    AddNewline();
  }
  void CloseJsonObject() {
    Outdent();
    AddNewline();
    output_ += "}";
  }

  void Indent() { current_indent_ += indent_size_; }

  void Outdent() { current_indent_ -= indent_size_; }

  void AddNewline() {
    if (add_newlines_) {
      output_ += "\n";
      output_.append(current_indent_, ' ');
    }
  }

  void PrintFieldPreamble(const string& name) {
    absl::StrAppend(&output_, "\"", name, "\": ");
  }

  Status PrintNonPrimitive(const google::protobuf::Message& proto) {
    if (IsReference(proto.GetDescriptor()) && !for_analytics_) {
      FHIR_ASSIGN_OR_RETURN(
          const Reference& reference,
          StandardizeReference(dynamic_cast<const Reference&>(proto)));
      FHIR_RETURN_IF_ERROR(PrintStandardNonPrimitive(reference));
    } else {
      FHIR_RETURN_IF_ERROR(PrintStandardNonPrimitive(proto));
    }
    return Status::OK();
  }

  Status PrintStandardNonPrimitive(const google::protobuf::Message& proto) {
    const Descriptor* descriptor = proto.GetDescriptor();
    const Reflection* reflection = proto.GetReflection();

    std::vector<const FieldDescriptor*> set_fields;
    reflection->ListFields(proto, &set_fields);

    if (descriptor->full_name() ==
        ContainedResource::descriptor()->full_name()) {
      for (const FieldDescriptor* field : set_fields) {
        const Message& field_value = reflection->GetMessage(proto, field);
        if (for_analytics_) {
          // Only print resource url if in analytic mode.
          absl::StrAppend(&output_, "\"",
                          field_value.GetDescriptor()->options().GetExtension(
                              stu3::proto::fhir_structure_definition_url),
                          "\"");
        } else {
          FHIR_RETURN_IF_ERROR(PrintNonPrimitive(field_value));
        }
      }
      return Status::OK();
    }

    if (for_analytics_ &&
        descriptor->full_name() == Extension::descriptor()->full_name()) {
      // Only print extension url when in analytic mode.
      absl::StrAppend(&output_, "\"",
                      dynamic_cast<const Extension&>(proto).url().value(), "\"");
      return Status::OK();
    }

    OpenJsonObject();
    if (IsResource(descriptor) && !for_analytics_) {
      absl::StrAppend(&output_, "\"resourceType\": \"", descriptor->name(),
                      "\",");
      AddNewline();
    }
    for (size_t i = 0; i < set_fields.size(); i++) {
      const FieldDescriptor* field = set_fields[i];
      // We don't unroll choice types when in analytic mode, so that we can
      // look up the choice type in a single query.
      if (IsChoiceType(field) && !for_analytics_) {
        FHIR_RETURN_IF_ERROR(PrintChoiceTypeField(
            reflection->GetMessage(proto, field), field->json_name()));
      } else {
        FHIR_RETURN_IF_ERROR(PrintField(proto, field));
      }
      if (i != set_fields.size() - 1) {
        output_ += ",";
        AddNewline();
      }
    }
    CloseJsonObject();
    return Status::OK();
  }

  Status PrintField(const google::protobuf::Message& containing_proto,
                    const FieldDescriptor* field) {
    if (field->containing_type() != containing_proto.GetDescriptor()) {
      return InvalidArgument("Field ", field->full_name(), " not found on ",
                             containing_proto.GetDescriptor()->full_name());
    }
    const Reflection* reflection = containing_proto.GetReflection();

    if (field->is_repeated()) {
      if (IsPrimitive(field->message_type())) {
        FHIR_RETURN_IF_ERROR(
            PrintRepeatedPrimitiveField(containing_proto, field));
      } else {
        int field_size = reflection->FieldSize(containing_proto, field);

        PrintFieldPreamble(field->json_name());
        output_ += "[";
        Indent();
        AddNewline();

        for (int i = 0; i < field_size; i++) {
          FHIR_RETURN_IF_ERROR(PrintNonPrimitive(
              reflection->GetRepeatedMessage(containing_proto, field, i)));
          if (i != field_size - 1) {
            output_ += ",";
            AddNewline();
          }
        }
        Outdent();
        AddNewline();
        output_ += "]";
      }
    } else {  // Singular Field
      if (IsPrimitive(field->message_type())) {
        FHIR_RETURN_IF_ERROR(
            PrintPrimitiveField(reflection->GetMessage(containing_proto, field),
                                field->json_name()));
      } else {
        PrintFieldPreamble(field->json_name());
        FHIR_RETURN_IF_ERROR(
            PrintNonPrimitive(reflection->GetMessage(containing_proto, field)));
      }
    }
    return Status::OK();
  }

  Status PrintPrimitiveField(const google::protobuf::Message& proto,
                             const string& field_name) {
    if (for_analytics_ && proto.GetDescriptor()->full_name() ==
                              ReferenceId::descriptor()->full_name()) {
      // In analytic mode, print the raw reference id rather than slicing into
      // type subfields, to make it easier to query.
      PrintFieldPreamble(field_name);
      absl::StrAppend(&output_, "\"",
                      dynamic_cast<const ReferenceId&>(proto).value(), "\"");
      return Status::OK();
    }
    FHIR_ASSIGN_OR_RETURN(const JsonPrimitive json_primitive,
                          WrapPrimitiveProto(proto, default_timezone_));

    if (json_primitive.is_non_null()) {
      PrintFieldPreamble(field_name);
      output_ += json_primitive.value;
    }
    if (json_primitive.element && !for_analytics_) {
      if (json_primitive.is_non_null()) {
        output_ += ",";
        AddNewline();
      }
      PrintFieldPreamble(absl::StrCat("_", field_name));
      FHIR_RETURN_IF_ERROR(PrintNonPrimitive(*json_primitive.element));
    }
    return Status::OK();
  }

  Status PrintChoiceTypeField(const google::protobuf::Message& choice_container,
                              const string& json_name) {
    const google::protobuf::Reflection* choice_reflection =
        choice_container.GetReflection();
    const google::protobuf::Descriptor* choice_descriptor =
        choice_container.GetDescriptor();
    if (choice_descriptor->oneof_decl_count() != 1) {
      return InvalidArgument("No oneof field on: ",
                             choice_container.GetDescriptor()->full_name());
    }
    const google::protobuf::OneofDescriptor* oneof =
        choice_container.GetDescriptor()->oneof_decl(0);
    if (!choice_reflection->HasOneof(choice_container, oneof)) {
      return InvalidArgument("Oneof not set on choice type: ",
                             choice_container.GetDescriptor()->full_name());
    }
    const google::protobuf::FieldDescriptor* value_field =
        choice_reflection->GetOneofFieldDescriptor(choice_container, oneof);
    string oneof_field_name = value_field->json_name();
    oneof_field_name[0] = toupper(oneof_field_name[0]);

    if (IsPrimitive(value_field->message_type())) {
      FHIR_RETURN_IF_ERROR(PrintPrimitiveField(
          choice_reflection->GetMessage(choice_container, value_field),
          absl::StrCat(json_name, oneof_field_name)));
    } else {
      PrintFieldPreamble(absl::StrCat(json_name, oneof_field_name));
      FHIR_RETURN_IF_ERROR(PrintNonPrimitive(
          choice_reflection->GetMessage(choice_container, value_field)));
    }
    return Status::OK();
  }

  Status PrintRepeatedPrimitiveField(const google::protobuf::Message& containing_proto,
                                     const FieldDescriptor* field) {
    if (field->containing_type() != containing_proto.GetDescriptor()) {
      return InvalidArgument("Field ", field->full_name(), " not found on ",
                             containing_proto.GetDescriptor()->full_name());
    }
    const Reflection* reflection = containing_proto.GetReflection();
    int field_size = reflection->FieldSize(containing_proto, field);

    std::vector<JsonPrimitive> json_primitives(field_size);
    bool any_primitive_extensions_found = false;
    bool non_null_values_found = false;

    for (int i = 0; i < field_size; i++) {
      const Message& field_value =
          reflection->GetRepeatedMessage(containing_proto, field, i);
      FHIR_ASSIGN_OR_RETURN(json_primitives[i],
                            WrapPrimitiveProto(field_value, default_timezone_));
      non_null_values_found =
          non_null_values_found || (json_primitives[i].is_non_null());
      any_primitive_extensions_found =
          any_primitive_extensions_found ||
          (json_primitives[i].element != nullptr);
    }

    if (non_null_values_found) {
      PrintFieldPreamble(field->json_name());
      output_ += "[";
      Indent();
      for (const JsonPrimitive& json_primitive : json_primitives) {
        AddNewline();
        output_ += json_primitive.value;
        output_ += ",";
      }
      output_.pop_back();
      Outdent();
      AddNewline();
      output_ += "]";
    }

    if (any_primitive_extensions_found) {
      if (non_null_values_found) {
        output_ += ",";
        AddNewline();
      }
      PrintFieldPreamble(absl::StrCat("_", field->json_name()));
      output_ += "[";
      Indent();
      for (const JsonPrimitive& json_primitive : json_primitives) {
        AddNewline();
        if (json_primitive.element != nullptr) {
          FHIR_RETURN_IF_ERROR(PrintNonPrimitive(*json_primitive.element));
        } else {
          output_ += "null";
        }
        output_ += ",";
      }
      output_.pop_back();
      Outdent();
      AddNewline();
      output_ += "]";
    }
    return Status::OK();
  }

  StatusOr<const Reference> StandardizeReference(const google::protobuf::Message& proto) {
    const Descriptor* descriptor = proto.GetDescriptor();
    if (Reference::descriptor()->full_name() != descriptor->full_name()) {
      return InvalidArgument(descriptor->full_name(), " is not a reference.");
    }

    const Reference& reference = dynamic_cast<const Reference&>(proto);
    if (reference.has_uri()) {
      return reference;
    }
    StatusOr<string> reference_string_status =
        ReferenceProtoToString(reference);
    if (tensorflow::errors::IsNotFound(reference_string_status.status())) {
      // Indicates a Reference with no reference string - e.g., contains a
      // display text only.
      return reference;
    }
    FHIR_ASSIGN_OR_RETURN(const string& reference_string,
                          reference_string_status);
    Reference new_reference;
    new_reference = reference;
    new_reference.mutable_uri()->set_value(reference_string);
    return new_reference;
  }

  const absl::TimeZone default_timezone_;
  const int indent_size_;
  const bool add_newlines_;
  const bool for_analytics_;

  string output_;
  int current_indent_;
};

}  // namespace

StatusOr<string> PrettyPrintFhirToJsonString(
    const google::protobuf::Message& fhir_proto, const absl::TimeZone default_timezone) {
  Printer printer{default_timezone, 2, true, false};
  return printer.WriteMessage(fhir_proto);
}

StatusOr<string> PrintFhirToJsonString(const google::protobuf::Message& fhir_proto,
                                       const absl::TimeZone default_timezone) {
  Printer printer{default_timezone, 0, false, false};
  return printer.WriteMessage(fhir_proto);
}

StatusOr<string> PrintFhirToJsonStringForAnalytics(
    const google::protobuf::Message& fhir_proto, const absl::TimeZone default_timezone) {
  Printer printer{default_timezone, 0, false, true};
  return printer.WriteMessage(fhir_proto);
}

StatusOr<string> PrettyPrintFhirToJsonStringForAnalytics(
    const google::protobuf::Message& fhir_proto, const absl::TimeZone default_timezone) {
  Printer printer{default_timezone, 2, true, true};
  return printer.WriteMessage(fhir_proto);
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
