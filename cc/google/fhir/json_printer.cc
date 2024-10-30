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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>


#include "google/protobuf/any.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "google/fhir/annotations.h"
#include "google/fhir/core_resource_registry.h"
#include "google/fhir/extensions.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/json_format.h"
#include "google/fhir/json/json_util.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/r4/codeable_concepts.h"
#include "google/fhir/r4/profiles.h"
#include "google/fhir/references.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/google/fhir/proto/annotations.pb.h"
#include "google/protobuf/message.h"

namespace google {
namespace fhir {

using ::absl::InvalidArgumentError;
using ::google::fhir::IsChoiceType;
using ::google::fhir::IsPrimitive;
using ::google::fhir::IsReference;
using ::google::protobuf::Any;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

namespace internal {

// Format in which the printer will represent the FHIR proto in JSON form.
enum FhirJsonFormat {
  // Lossless JSON representation of FHIR proto.
  kFormatPure = 0,

  // Lossy JSON representation with specified maximum recursive depth and
  // limited support for Extensions.
  kFormatAnalytic = 1
};

class Printer {
 public:
  Printer(const PrimitiveHandler* primitive_handler, int indent_size,
          bool add_newlines, FhirJsonFormat json_format)
      : primitive_handler_(primitive_handler),
        indent_size_(indent_size),
        add_newlines_(add_newlines),
        json_format_(json_format) {}

  absl::StatusOr<std::string> WriteMessage(const Message& message) {
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

  void PrintFieldPreamble(absl::string_view name) {
    absl::StrAppend(&output_, "\"", name, "\": ");
  }

  absl::Status PrintNonPrimitive(const Message& proto,
                                 bool print_as_string = false) {
    if (IsReference(proto.GetDescriptor()) && json_format_ == kFormatPure) {
      // For printing reference, we don't want typed reference fields,
      // just standard FHIR reference fields.
      // If we have a typed field instead, convert to a "Standard" reference.
      FHIR_ASSIGN_OR_RETURN(std::unique_ptr<Message> standard_reference,
                            StandardizeReference(proto));
      if (standard_reference) {
        return PrintStandardNonPrimitive(*standard_reference);
      }
    }
    if (json_format_ == kFormatAnalytic &&
        IsProfileOfCodeableConcept(proto.GetDescriptor())) {
      FHIR_ASSIGN_OR_RETURN(std::unique_ptr<Message> analytic_codeable_concept,
                            MakeAnalyticCodeableConcept(proto));
      return PrintStandardNonPrimitive(*analytic_codeable_concept,
                                       print_as_string);
    }
    return PrintStandardNonPrimitive(proto, print_as_string);
  }

  std::optional<std::unique_ptr<Message>> ExtractConcreteMessage(
      const google::protobuf::Any& any_message) {
    const google::protobuf::Descriptor* descriptor;
    google::protobuf::Message* concrete_msg;
    std::string full_name;
    // Resolve types in generated pool.
    if (!google::protobuf::Any::ParseAnyTypeUrl(any_message.type_url(),
                                                &full_name)) {
      return std::nullopt;
    }

    // Get the descriptor of the embedded message.
    descriptor =
        google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(
            full_name);
    if (descriptor == nullptr) {
      return std::nullopt;
    }

    // Construct a default object of the embedded type.
    const google::protobuf::Message* default_embedded_msg =
        google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
    if (default_embedded_msg == nullptr) {
      return std::nullopt;
    }
    concrete_msg = default_embedded_msg->New();

    if (any_message.UnpackTo(concrete_msg)) {
      return absl::WrapUnique(concrete_msg);
    }

    return std::nullopt;
  }

  absl::Status PrintContainedResourceOneOf(const Message& contained_one_of) {
    std::unique_ptr<Message> contained =
        absl::WrapUnique(primitive_handler_->NewContainedResource());

    const FieldDescriptor* resource_field = nullptr;
    const google::protobuf::OneofDescriptor*  // NOLINT: The direct import breaks kokoro.
        resource_oneof =
            contained->GetDescriptor()->FindOneofByName("oneof_resource");
    for (int i = 0; i < resource_oneof->field_count(); i++) {
      const ::google::protobuf::FieldDescriptor* field = resource_oneof->field(i);

      if (field->message_type()->name() ==
          contained_one_of.GetDescriptor()->name()) {
        resource_field = field;
      }
    }

    if (resource_field == nullptr) {
      // One of resource not found in ContainedResource. This could happen if
      // the proto is not a core FHIR resource. In this case we drop the
      // resource.
      return absl::OkStatus();
    }

    const ::google::protobuf::Reflection* ref = contained->GetReflection();
    ref->MutableMessage(contained.get(), resource_field)
        ->CopyFrom(contained_one_of);

    return PrintContainedResource(*contained);
  }

  absl::Status PrintStandardNonPrimitive(const Message& proto,
                                         bool print_as_string = false) {
    const Descriptor* descriptor = proto.GetDescriptor();
    const Reflection* reflection = proto.GetReflection();

    // TODO(b/154059162): Use an annotation here.
    if (descriptor->name() == "ContainedResource") {
      return PrintContainedResource(proto);
    }
    if (descriptor->full_name() == Any::descriptor()->full_name()) {
      std::unique_ptr<Message> contained =
          absl::WrapUnique(primitive_handler_->NewContainedResource());

      // TODO(b/148916862): Use a registry to determine the correct
      // ContainedResource to unpack to.
      if (google::protobuf::DownCastToGenerated<Any>(proto).UnpackTo(contained.get())) {
        return PrintContainedResource(*contained);
      }

      // If we can't unpack the Any proto as a contained resource, try to unpack
      // it as a contained resource `one of` field.
      std::optional<std::unique_ptr<google::protobuf::Message>> resource_msg =
          ExtractConcreteMessage(google::protobuf::DownCastToGenerated<Any>(proto));
      if (resource_msg == std::nullopt) {
        // Unable to extract Any proto into a concrete message. This could
        // happen if the proto is not a core FHIR resource. In this case we
        // drop the resource.
        return absl::OkStatus();
      }

      return PrintContainedResourceOneOf(**resource_msg);
    }

    if (json_format_ == kFormatAnalytic && IsExtension(proto)) {
      // Only print extension url when in analytic mode.
      std::string scratch;
      absl::StrAppend(&output_, "\"", GetExtensionUrl(proto, &scratch), "\"");
      return absl::OkStatus();
    }

    OpenJsonObject();
    std::vector<const FieldDescriptor*> set_fields;
    reflection->ListFields(proto, &set_fields);
    if (IsResource(descriptor) && json_format_ == kFormatPure) {
      absl::StrAppend(&output_, "\"resourceType\": \"", descriptor->name(),
                      "\"");
      if (!set_fields.empty()) {
        absl::StrAppend(&output_, ",");
        AddNewline();
      }
    }
    for (size_t i = 0; i < set_fields.size(); i++) {
      const FieldDescriptor* field = set_fields[i];
      if (json_format_ == kFormatAnalytic && field->name() == "id" &&
          !google::fhir::IsId(field->message_type())) {
        // Skip over submessage id fields.
        // Resource-level ids use the Id FHIR data type, but ids on sub-messages
        // use Strings.
        continue;
      }
      int64_t size_before_call = output_.size();
      // Choice types in proto form have a containing message that is not part
      // of the FHIR spec, so we need a special method to print them as valid
      // fhir.
      // In analytics mode, we print the containing message to make it easier
      // to query all possible choice types in a single query.
      if (IsChoiceType(field) && json_format_ == kFormatPure) {
        FHIR_RETURN_IF_ERROR(PrintChoiceTypeField(
            reflection->GetMessage(proto, field), FhirJsonName(field)));
      } else {
        FHIR_RETURN_IF_ERROR(PrintField(proto, field, print_as_string));
      }
      bool output_changed = output_.size() > size_before_call;
      if (i < set_fields.size() - 1 && output_changed) {
        output_ += ",";
        AddNewline();
      }
    }
    CloseJsonObject();

    return absl::OkStatus();
  }

  // Escape a section of our output string to allow it to be used as a string
  // literal:
  // E.g. {"name": "test"} -> "{ \"name\": \"test\" }"
  std::string EscapeSubstringForUseAsString(absl::string_view output,
                                            size_t start, size_t end) {
    std::string new_string = output_.substr(start, end - start);
    new_string = absl::StrReplaceAll(new_string, {{"\\", "\\\\"},
                                                  {"\"", "\\\""},
                                                  {"\n", "\\n"},
                                                  {"\r", "\\r"},
                                                  {"\b", "\\b"}});
    return output_.replace(start, end, new_string);
  }

  absl::Status PrintContainedResource(const Message& proto) {
    std::vector<const FieldDescriptor*> set_fields;
    proto.GetReflection()->ListFields(proto, &set_fields);

    for (const FieldDescriptor* field : set_fields) {
      const Message& field_value =
          proto.GetReflection()->GetMessage(proto, field);
      if (json_format_ == kFormatAnalytic) {
        // If print as string is set, wrap the output created here in opening
        // quotes. Use single quotes to avoid being escaped by the double quotes
        // created in the functions below.
        absl::StrAppend(&output_, "\"");

        size_t size_before_call = output_.size();
        // Print a string of the resource in analytic mode.
        FHIR_RETURN_IF_ERROR(
            PrintNonPrimitive(field_value, /*print_as_string=*/true));
        size_t size_after_call = output_.size();
        output_ = EscapeSubstringForUseAsString(output_, size_before_call,
                                                size_after_call);

        // If print as string is set, wrap the output created here in closing
        // quotes.
        absl::StrAppend(&output_, "\"");

      } else {
        FHIR_RETURN_IF_ERROR(PrintNonPrimitive(field_value));
      }
    }
    return absl::OkStatus();
  }

  absl::Status PrintField(const Message& containing_proto,
                          const FieldDescriptor* field,
                          bool print_as_string = false) {
    if (field->containing_type() != containing_proto.GetDescriptor()) {
      return InvalidArgumentError(
          absl::StrCat("Field ", field->full_name(), " not found on ",
                       containing_proto.GetDescriptor()->full_name()));
    }
    const Reflection* reflection = containing_proto.GetReflection();

    if (field->is_repeated()) {
      if (IsPrimitive(field->message_type())) {
        FHIR_RETURN_IF_ERROR(
            PrintRepeatedPrimitiveField(containing_proto, field));
      } else {
        int field_size = reflection->FieldSize(containing_proto, field);

        PrintFieldPreamble(FhirJsonName(field));
        output_ += "[";
        Indent();
        AddNewline();

        for (int i = 0; i < field_size; i++) {
          int64_t size_before_call = output_.size();
          FHIR_RETURN_IF_ERROR(PrintNonPrimitive(
              reflection->GetRepeatedMessage(containing_proto, field, i)));
          bool output_changed = output_.size() > size_before_call;
          if (i != field_size - 1 && output_changed) {
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
                                FhirJsonName(field)));
      } else {
        PrintFieldPreamble(FhirJsonName(field));
        FHIR_RETURN_IF_ERROR(
            PrintNonPrimitive(reflection->GetMessage(containing_proto, field)));
      }
    }
    return absl::OkStatus();
  }

  absl::Status PrintPrimitiveField(const Message& proto,
                                   absl::string_view field_name) {
    // TODO(b/153462178): check for ReferenceId using an annotation.
    if (json_format_ == kFormatAnalytic &&
        proto.GetDescriptor()->name() == "ReferenceId") {
      // In analytic mode, print the raw reference id rather than slicing into
      // type subfields, to make it easier to query.
      PrintFieldPreamble(field_name);
      std::string scratch;
      FHIR_ASSIGN_OR_RETURN(const std::string& reference_value,
                            GetPrimitiveStringValue(proto, &scratch));
      absl::StrAppend(&output_, "\"", reference_value, "\"");
      return absl::OkStatus();
    }
    FHIR_ASSIGN_OR_RETURN(const JsonPrimitive json_primitive,
                          primitive_handler_->WrapPrimitiveProto(proto));

    if (json_primitive.is_non_null() && !json_primitive.value.empty()) {
      PrintFieldPreamble(field_name);
      output_ += json_primitive.value;
    }
    if (json_primitive.element && json_format_ == kFormatPure) {
      if (json_primitive.is_non_null()) {
        output_ += ",";
        AddNewline();
      }
      PrintFieldPreamble(absl::StrCat("_", field_name));
      FHIR_RETURN_IF_ERROR(PrintNonPrimitive(*json_primitive.element));
    }
    return absl::OkStatus();
  }

  absl::Status PrintChoiceTypeField(const Message& choice_container,
                                    absl::string_view json_name) {
    const google::protobuf::Reflection* choice_reflection =
        choice_container.GetReflection();
    const google::protobuf::Descriptor* choice_descriptor =
        choice_container.GetDescriptor();
    if (choice_descriptor->oneof_decl_count() != 1) {
      return InvalidArgumentError(
          absl::StrCat("No oneof field on: ",
                       choice_container.GetDescriptor()->full_name()));
    }
    const google::protobuf::OneofDescriptor* oneof =
        choice_container.GetDescriptor()->oneof_decl(0);
    if (!choice_reflection->HasOneof(choice_container, oneof)) {
      return InvalidArgumentError(
          absl::StrCat("Oneof not set on choice type: ",
                       choice_container.GetDescriptor()->full_name()));
    }
    const google::protobuf::FieldDescriptor* value_field =
        choice_reflection->GetOneofFieldDescriptor(choice_container, oneof);
    std::string oneof_field_name(FhirJsonName(value_field));
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
    return absl::OkStatus();
  }

  absl::Status PrintRepeatedPrimitiveField(const Message& containing_proto,
                                           const FieldDescriptor* field) {
    if (field->containing_type() != containing_proto.GetDescriptor()) {
      return InvalidArgumentError(
          absl::StrCat("Field ", field->full_name(), " not found on ",
                       containing_proto.GetDescriptor()->full_name()));
    }
    const Reflection* reflection = containing_proto.GetReflection();
    int field_size = reflection->FieldSize(containing_proto, field);

    std::vector<JsonPrimitive> json_primitives(field_size);
    bool any_primitive_extensions_found = false;
    bool non_null_values_found = false;

    for (int i = 0; i < field_size; i++) {
      const Message& field_value =
          reflection->GetRepeatedMessage(containing_proto, field, i);
      FHIR_ASSIGN_OR_RETURN(
          json_primitives[i],
          primitive_handler_->WrapPrimitiveProto(field_value));
      non_null_values_found =
          non_null_values_found || (json_primitives[i].is_non_null());
      any_primitive_extensions_found = any_primitive_extensions_found ||
                                       (json_primitives[i].element != nullptr);
    }

    if (non_null_values_found) {
      PrintFieldPreamble(FhirJsonName(field));
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
      PrintFieldPreamble(absl::StrCat("_", FhirJsonName(field)));
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
    return absl::OkStatus();
  }

  // Creates a copy of the profiled codeable concept that has ALL codings
  // present in the coding fields, even if they're also present in profiled
  // fields.
  // Does this by making a copy of the original, clearing the coding field,
  // and then coping all codings on the original (profiled and unprofiled)
  // onto the coding field of the copy.
  absl::StatusOr<std::unique_ptr<Message>> MakeAnalyticCodeableConcept(
      const Message& profiled_codeable_concept) {
    auto analytic_codeable_concept =
        absl::WrapUnique(profiled_codeable_concept.New());

    FHIR_RETURN_IF_ERROR(primitive_handler_->CopyCodeableConcept(
        profiled_codeable_concept, analytic_codeable_concept.get()));
    FHIR_RETURN_IF_ERROR(ClearField(analytic_codeable_concept.get(), "coding"));

    const Descriptor* descriptor = analytic_codeable_concept->GetDescriptor();
    const Reflection* reflection = analytic_codeable_concept->GetReflection();
    const FieldDescriptor* coding_field = descriptor->FindFieldByName("coding");
    if (!coding_field || !coding_field->message_type() ||
        !IsCoding(coding_field->message_type())) {
      return InvalidArgumentError(absl::StrCat(
          "Invalid or missing coding field on CodeableConcept type ",
          descriptor->full_name()));
    }

    switch (GetFhirVersion(profiled_codeable_concept)) {
      case proto::R4:
        r4::ForEachCoding(
            profiled_codeable_concept, [&](const r4::core::Coding& coding) {
              reflection
                  ->AddMessage(analytic_codeable_concept.get(), coding_field)
                  ->CopyFrom(coding);
            });
        break;
      default:
        return InvalidArgumentError(absl::StrCat(
            "Unsupported FHIR Version for profiling for resource: ",
            profiled_codeable_concept.GetDescriptor()->full_name()));
    }

    return std::move(analytic_codeable_concept);
  }

  // If reference is typed Returns a unique pointer to a new standardized
  // reference
  // Returns nullptr if reference is alrady standard.
  absl::StatusOr<std::unique_ptr<Message>> StandardizeReference(
      const Message& reference) {
    const Descriptor* descriptor = reference.GetDescriptor();
    const Reflection* reflection = reference.GetReflection();
    const google::protobuf::OneofDescriptor* oneof =
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
    FHIR_ASSIGN_OR_RETURN(const absl::optional<std::string> reference_string,
                          ReferenceProtoToString(reference));
    if (reference_string) {
      FHIR_RETURN_IF_ERROR(
          SetPrimitiveStringValue(uri, reference_string.value()));
    }
    return std::move(mutable_reference);
  }

  const PrimitiveHandler* primitive_handler_;
  const int indent_size_;
  const bool add_newlines_;
  const FhirJsonFormat json_format_;

  std::string output_;
  int current_indent_;
};

absl::StatusOr<std::string> WriteMessage(Printer printer,
                                         const Message& message) {
  if (IsProfile(message.GetDescriptor())) {
    if (GetFhirVersion(message) != proto::R4) {
      return InvalidArgumentError(
          absl::StrCat("Unsupported FHIR Version for profiling for resource: ",
                       message.GetDescriptor()->full_name()));
    }
    // Unprofile before writing, since JSON should be based on raw proto
    // Note that these are "lenient" profilings, because it doesn't make
    // sense to error out during printing.
    FHIR_ASSIGN_OR_RETURN(std::unique_ptr<Message> core_resource,
                          GetBaseResourceInstance(message));
    FHIR_RETURN_IF_ERROR(
        ConvertToProfileLenientR4(message, core_resource.get()));

    return printer.WriteMessage(*core_resource);
  } else {
    return printer.WriteMessage(message);
  }
}

}  // namespace internal

::absl::StatusOr<std::string> Printer::PrintFhirPrimitive(
    const Message& primitive_message) const {
  FHIR_ASSIGN_OR_RETURN(
      const JsonPrimitive& primitive,
      primitive_handler_->WrapPrimitiveProto(primitive_message));
  return primitive.value;
}

absl::StatusOr<std::string> Printer::PrettyPrintFhirToJsonString(
    const Message& fhir_proto) const {
  internal::Printer printer{primitive_handler_, 2, true, internal::kFormatPure};
  return WriteMessage(printer, fhir_proto);
}

absl::StatusOr<std::string> Printer::PrintFhirToJsonString(
    const Message& fhir_proto) const {
  internal::Printer printer{primitive_handler_, 0, false,
                            internal::kFormatPure};
  return WriteMessage(printer, fhir_proto);
}

absl::StatusOr<std::string> Printer::PrintFhirToJsonStringForAnalytics(
    const Message& fhir_proto) const {
  internal::Printer printer{primitive_handler_, 0, false,
                            internal::kFormatAnalytic};
  return printer.WriteMessage(fhir_proto);
}

absl::StatusOr<std::string> Printer::PrettyPrintFhirToJsonStringForAnalytics(
    const Message& fhir_proto) const {
  internal::Printer printer{primitive_handler_, 2, true,
                            internal::kFormatAnalytic};
  return printer.WriteMessage(fhir_proto);
}

}  // namespace fhir
}  // namespace google
