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


#include "google/protobuf/any.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codeable_concepts.h"
#include "google/fhir/core_resource_registry.h"
#include "google/fhir/extensions.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/primitive_wrapper.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/r4/profiles.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/stu3/profiles.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "include/json/json.h"

namespace google {
namespace fhir {

namespace {

using ::google::fhir::IsChoiceType;
using ::google::fhir::IsPrimitive;
using ::google::fhir::IsReference;
using ::google::fhir::Status;
using ::google::fhir::StatusOr;
using ::google::protobuf::Descriptor;
using ::google::protobuf::Any;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using ::tensorflow::errors::InvalidArgument;

class Printer {
 public:
  Printer(int indent_size, bool add_newlines, bool for_analytics)
      : indent_size_(indent_size),
        add_newlines_(add_newlines),
        for_analytics_(for_analytics) {}

  StatusOr<std::string> WriteMessage(const Message& message) {
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

  void PrintFieldPreamble(const std::string& name) {
    absl::StrAppend(&output_, "\"", name, "\": ");
  }

  Status PrintNonPrimitive(const Message& proto) {
    if (IsReference(proto.GetDescriptor()) && !for_analytics_) {
      // For printing reference, we don't want typed reference fields,
      // just standard FHIR reference fields.
      // If we have a typed field instead, convert to a "Standard" reference.
      FHIR_ASSIGN_OR_RETURN(std::unique_ptr<Message> standard_reference,
                            StandardizeReference(proto));
      if (standard_reference) {
        return PrintStandardNonPrimitive(*standard_reference);
      }
    }
    if (for_analytics_ && IsProfileOfCodeableConcept(proto.GetDescriptor())) {
      FHIR_ASSIGN_OR_RETURN(std::unique_ptr<Message> analytic_codeable_concept,
                            MakeAnalyticCodeableConcept(proto));
      return PrintStandardNonPrimitive(*analytic_codeable_concept);
    }
    return PrintStandardNonPrimitive(proto);
  }

  Status PrintStandardNonPrimitive(const Message& proto) {
    const Descriptor* descriptor = proto.GetDescriptor();
    const Reflection* reflection = proto.GetReflection();


    // TODO: Use an annotation here.
    if (descriptor->name() == "ContainedResource") {
      return PrintContainedResource(proto);
    }
    if (descriptor->full_name() == Any::descriptor()->full_name()) {
      // TODO: Handle STU3 Any
      r4::core::ContainedResource contained;
      if (!dynamic_cast<const Any&>(proto).UnpackTo(&contained)) {
        return InvalidArgument("Unable to unpack Any to ContainedResource");
      }
      return PrintContainedResource(contained);
    }

    if (for_analytics_ && IsFhirType<stu3::proto::Extension>(proto)) {
      // Only print extension url when in analytic mode.
      std::string scratch;
      absl::StrAppend(&output_, "\"", GetExtensionUrl(proto, &scratch), "\"");
      return Status::OK();
    }

    OpenJsonObject();
    if (IsResource(descriptor) && !for_analytics_) {
      absl::StrAppend(&output_, "\"resourceType\": \"", descriptor->name(),
                      "\",");
      AddNewline();
    }
    std::vector<const FieldDescriptor*> set_fields;
    reflection->ListFields(proto, &set_fields);
    for (size_t i = 0; i < set_fields.size(); i++) {
      const FieldDescriptor* field = set_fields[i];
      // Choice types in proto form have a containing message that is not part
      // of the FHIR spec, so we need a special method to print them as valid
      // fhir.
      // In analytics mode, we print the containing message to make it easier
      // to query all possible choice types in a single query.
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

  Status PrintContainedResource(const Message& proto) {
    std::vector<const FieldDescriptor*> set_fields;
    proto.GetReflection()->ListFields(proto, &set_fields);

    for (const FieldDescriptor* field : set_fields) {
      const Message& field_value =
          proto.GetReflection()->GetMessage(proto, field);
      if (for_analytics_) {
        // Only print resource url if in analytic mode.
        absl::StrAppend(&output_, "\"",
                        GetStructureDefinitionUrl(field_value.GetDescriptor()),
                        "\"");
      } else {
        FHIR_RETURN_IF_ERROR(PrintNonPrimitive(field_value));
      }
    }
    return Status::OK();
  }

  Status PrintField(const Message& containing_proto,
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

  Status PrintPrimitiveField(const Message& proto,
                             const std::string& field_name) {
    // TODO: check for ReferenceId using an annotation.
    if (for_analytics_ && proto.GetDescriptor()->name() == "ReferenceId") {
      // In analytic mode, print the raw reference id rather than slicing into
      // type subfields, to make it easier to query.
      PrintFieldPreamble(field_name);
      std::string scratch;
      FHIR_ASSIGN_OR_RETURN(const std::string& reference_value,
                            GetPrimitiveStringValue(proto, &scratch));
      absl::StrAppend(&output_, "\"", reference_value, "\"");
      return Status::OK();
    }
    FHIR_ASSIGN_OR_RETURN(const JsonPrimitive json_primitive,
                          WrapPrimitiveProto(proto));

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

  Status PrintChoiceTypeField(const Message& choice_container,
                              const std::string& json_name) {
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
    std::string oneof_field_name = value_field->json_name();
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

  Status PrintRepeatedPrimitiveField(const Message& containing_proto,
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
                            WrapPrimitiveProto(field_value));
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

  // Creates a copy of the profiled codeable concept that has ALL codings
  // present in the coding fields, even if they're also present in profiled
  // fields.
  // Does this by making a copy of the original, clearing the coding field,
  // and then coping all codings on the original (profiled and unprofiled)
  // onto the coding field of the copy.
  StatusOr<std::unique_ptr<Message>> MakeAnalyticCodeableConcept(
      const Message& profiled_codeable_concept) {
    auto analytic_codeable_concept =
        absl::WrapUnique(profiled_codeable_concept.New());

    FHIR_RETURN_IF_ERROR(CopyCodeableConcept(profiled_codeable_concept,
                                             analytic_codeable_concept.get()));
    FHIR_RETURN_IF_ERROR(ClearField(analytic_codeable_concept.get(), "coding"));

    const Descriptor* descriptor = analytic_codeable_concept->GetDescriptor();
    const Reflection* reflection = analytic_codeable_concept->GetReflection();
    const FieldDescriptor* coding_field = descriptor->FindFieldByName("coding");
    if (!coding_field || !coding_field->message_type() ||
        !IsCoding(coding_field->message_type())) {
      return InvalidArgument(
          "Invalid or missing coding field on CodeableConcept type ",
          descriptor->full_name());
    }

    switch (GetFhirVersion(profiled_codeable_concept)) {
      case proto::STU3:
        stu3::ForEachCoding(
            profiled_codeable_concept, [&](const stu3::proto::Coding& coding) {
              reflection
                  ->AddMessage(analytic_codeable_concept.get(), coding_field)
                  ->CopyFrom(coding);
            });
        break;
      case proto::R4:
        r4::ForEachCoding(
            profiled_codeable_concept, [&](const r4::core::Coding& coding) {
              reflection
                  ->AddMessage(analytic_codeable_concept.get(), coding_field)
                  ->CopyFrom(coding);
            });
        break;
      default:
        return InvalidArgument(
            "Unsupported FHIR Version for profiling for resource: " +
            profiled_codeable_concept.GetDescriptor()->full_name());
    }

    return analytic_codeable_concept;
  }

  // If reference is typed Returns a unique pointer to a new standardized
  // reference
  // Returns nullptr if reference is alrady standard.
  StatusOr<std::unique_ptr<Message>> StandardizeReference(
      const Message& reference) {
    const Descriptor* descriptor = reference.GetDescriptor();
    const Reflection* reflection = reference.GetReflection();
    const ::google::protobuf::OneofDescriptor* oneof =
        descriptor->FindOneofByName("reference");

    if (!reflection->HasOneof(reference, oneof)) {
      // Nothing we need to do.  Return a null unique ptr to indicate this.
      return std::unique_ptr<Message>();
    }
    const FieldDescriptor* set_oneof =
        reflection->GetOneofFieldDescriptor(reference, oneof);
    if (set_oneof->name() == "uri") {
      // It's already standard
      return std::unique_ptr<Message>();
    }
    // If we're this far, we have a type reference that needs to be standardized
    auto mutable_reference = absl::WrapUnique(reference.New());
    mutable_reference->CopyFrom(reference);
    const FieldDescriptor* uri_field =
        mutable_reference->GetDescriptor()->FindFieldByName("uri");
    Message* uri = mutable_reference->GetReflection()->MutableMessage(
        mutable_reference.get(), uri_field);
    // Note that setting the uri clears the typed references, since they share
    // a oneof
    FHIR_ASSIGN_OR_RETURN(const std::string& reference_string,
                          ReferenceMessageToString(reference));
    FHIR_RETURN_IF_ERROR(SetPrimitiveStringValue(uri, reference_string));
    return mutable_reference;
  }

  const int indent_size_;
  const bool add_newlines_;
  const bool for_analytics_;

  std::string output_;
  int current_indent_;
};

StatusOr<std::string> WriteMessage(Printer printer, const Message& message) {
  if (IsProfile(message.GetDescriptor())) {
    // Unprofile before writing, since JSON should be based on raw proto
    // Note that these are "lenient" profilings, because it doesn't make sense
    // to error out during printing.
    FHIR_ASSIGN_OR_RETURN(std::unique_ptr<Message> core_resource,
                          GetBaseResourceInstance(message));

    // TODO: This is not ideal because it pulls in both stu3 and
    // r4 datatypes.
    switch (GetFhirVersion(message)) {
      case proto::STU3:
        FHIR_RETURN_IF_ERROR(
            ConvertToProfileLenientStu3(message, core_resource.get()));
        break;
      case proto::R4:
        FHIR_RETURN_IF_ERROR(
            ConvertToProfileLenientR4(message, core_resource.get()));
        break;
      default:
        return InvalidArgument(
            "Unsupported FHIR Version for profiling for resource: " +
            message.GetDescriptor()->full_name());
    }
    return printer.WriteMessage(*core_resource);
  } else {
    return printer.WriteMessage(message);
  }
}

}  // namespace

StatusOr<std::string> PrettyPrintFhirToJsonString(const Message& fhir_proto) {
  Printer printer{2, true, false};
  return WriteMessage(printer, fhir_proto);
}

StatusOr<std::string> PrintFhirToJsonString(const Message& fhir_proto) {
  Printer printer{0, false, false};
  return WriteMessage(printer, fhir_proto);
}

StatusOr<std::string> PrintFhirToJsonStringForAnalytics(
    const Message& fhir_proto) {
  Printer printer{0, false, true};
  return printer.WriteMessage(fhir_proto);
}

StatusOr<std::string> PrettyPrintFhirToJsonStringForAnalytics(
    const Message& fhir_proto) {
  Printer printer{2, true, true};
  return printer.WriteMessage(fhir_proto);
}

}  // namespace fhir
}  // namespace google
