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

#include "google/fhir/testutil/generator.h"

#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/any.pb.h"
#include "absl/random/distributions.h"
#include "absl/status/status.h"
#include "google/fhir/annotations.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/json_format_results.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/references.h"
#include "google/fhir/util.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace fhir {
namespace testutil {

namespace {

// Characters to use in ids.
const absl::string_view kLegalIdCharacters =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";

// Characters to use in strings. Should be compatible
// with the FHIR string regex which is "[ \r\n\t\S]+"
const absl::string_view kLegalStringCharacters = reinterpret_cast<const char*>(
    u8"abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789"
    "!@#$%?^&*()-+=_{}[]|<>,.\\"
    " \n\r\t");

// Returns a randomly generated string consisting of legal_characters
// and of length between the given min and max string lengths, inclusive.
std::string RandomString(absl::string_view legal_characters,
                         int min_string_length, int max_string_length,
                         absl::BitGen& bitgen) {
  int length = absl::Uniform<int>(bitgen, min_string_length, max_string_length);
  std::string str;
  str.reserve(length);
  for (int i = 0; i < length; i++) {
    char c = kLegalStringCharacters[absl::Uniform<size_t>(
        bitgen, 0, legal_characters.length())];
    str += c;
  }
  return str;
}

}  // namespace

bool RandomValueProvider::ShouldFill(const google::protobuf::FieldDescriptor* field,
                                     int recursion_depth) {
  if (recursion_depth >= params_.max_recursion_depth) {
    return false;
  }
  if (field->message_type() && field->message_type()->name() == "Extension" &&
      !params_.fill_extensions) {
    return false;
  }
  return absl::Bernoulli(bitgen_,
                         params_.optional_set_probability *
                             std::pow(params_.optional_set_ratio_per_level,
                                      recursion_depth));  // NOLINT
}

int RandomValueProvider::GetNumRepeated(
    const google::protobuf::FieldDescriptor* descriptor, int recursion_depth) {
  return absl::Uniform(absl::IntervalClosed, bitgen_, params_.min_repeated,
                       params_.max_repeated);
}

const google::protobuf::FieldDescriptor* RandomValueProvider::SelectOneOf(
    const google::protobuf::Message* message,
    const std::vector<const google::protobuf::FieldDescriptor*>& one_of_fields) {
  return one_of_fields[absl::Uniform<int>(bitgen_, 0, one_of_fields.size())];
}

bool RandomValueProvider::GetBoolean(const google::protobuf::FieldDescriptor* field,
                                     int recursion_depth) {
  return absl::Bernoulli(bitgen_, 0.5);
}

std::string RandomValueProvider::GetBase64Binary(
    const google::protobuf::FieldDescriptor* field, int recursion_depth) {
  std::string escaped;
  absl::Base64Escape(RandomString(kLegalStringCharacters, 1,
                                  params_.max_string_length, bitgen_),
                     &escaped);
  return escaped;
}

int RandomValueProvider::GetInteger(const google::protobuf::FieldDescriptor* field,
                                    int recursion_depth) {
  return absl::Uniform<int>(bitgen_, params_.low_value, params_.high_value);
}

std::string RandomValueProvider::GetString(const google::protobuf::FieldDescriptor* field,
                                           int recursion_depth) {
  return RandomString(kLegalStringCharacters, 1, params_.max_string_length,
                      bitgen_);
}

int RandomValueProvider::GetPositiveInt(const google::protobuf::FieldDescriptor* field,
                                        int recursion_depth) {
  return absl::Uniform<int>(bitgen_, 1, params_.high_value);
}

int RandomValueProvider::GetUnsignedInt(const google::protobuf::FieldDescriptor* field,
                                        int recursion_depth) {
  return absl::Uniform<int>(bitgen_, 0, params_.high_value);
}

std::string RandomValueProvider::GetDecimal(
    const google::protobuf::FieldDescriptor* field, int recursion_depth) {
  // TODO(b/160252654): Use a fractional component when the parser properly
  // handles a list of decimals.
  // return absl::StrCat(GetInteger(field, recursion_depth), ".",
  //                    GetUnsignedInt(field, recursion_depth));
  return absl::StrCat(GetInteger(field, recursion_depth));
}

std::string RandomValueProvider::GetDateTime(
    const google::protobuf::FieldDescriptor* field, int recursion_depth) {
  // Only use partial dates for datetime fields some of the time.
  if (absl::Bernoulli(bitgen_, .8)) {
    return GetDate(field, recursion_depth);
  } else {
    std::string offset =
        absl::StrFormat("-%02d:00", absl::Uniform<int>(bitgen_, 1, 12));
    return absl::StrCat(GetFullDate(), "T", GetTime(field, recursion_depth),
                        offset);
  }
}

std::string RandomValueProvider::GetDate(const google::protobuf::FieldDescriptor* field,
                                         int recursion_depth) {
  // Set between one and three date fields, either year, year-month,
  // or year-month-day.
  int fields_set = absl::Uniform<int>(bitgen_, 1, 4);

  return fields_set == 1   ? GetYear()
         : fields_set == 2 ? GetYearMonth()
                           : GetFullDate();
}

std::string RandomValueProvider::GetTime(const google::protobuf::FieldDescriptor* field,
                                         int recursion_depth) {
  return absl::StrFormat("%02d:%02d:%02d", absl::Uniform<int>(bitgen_, 1, 23),
                         absl::Uniform<int>(bitgen_, 0, 60),
                         absl::Uniform<int>(bitgen_, 0, 60));
}

std::string RandomValueProvider::GetInstant(
    const google::protobuf::FieldDescriptor* field, int recursion_depth) {
  return absl::StrCat(GetFullDate(), "T", GetTime(field, recursion_depth), "Z");
}

std::string RandomValueProvider::GetId(const google::protobuf::FieldDescriptor* field,
                                       int recursion_depth) {
  // Ensure ids have a reasonable min and max length to stay readable and with
  // few collisions.
  return RandomString(kLegalIdCharacters, 5, 10, bitgen_);
}

std::string RandomValueProvider::GetUuid(const google::protobuf::FieldDescriptor* field,
                                         int recursion_depth) {
  // TODO(b/154072870): generate UUID at runtime
  return absl::StrCat("urn:uuid:", "2eb090b1-9da8-4549-ba31-bf3c3a0239f3");
}

std::string RandomValueProvider::GetIdentifier(
    const google::protobuf::FieldDescriptor* field, int recursion_depth) {
  return GetId(field, recursion_depth);
}

std::string RandomValueProvider::GetUri(const google::protobuf::FieldDescriptor* field,
                                        int recursion_depth) {
  return GetUrl(field, recursion_depth);
}

std::string RandomValueProvider::GetUrl(const google::protobuf::FieldDescriptor* field,
                                        int recursion_depth) {
  return absl::StrCat("http://www.example.com/", GetId(field, recursion_depth));
}

std::string RandomValueProvider::GetCanonical(
    const google::protobuf::FieldDescriptor* field, int recursion_depth) {
  return GetUrl(field, recursion_depth);
}

std::string RandomValueProvider::GetOid(const google::protobuf::FieldDescriptor* field,
                                        int recursion_depth) {
  // Follows general period-delimited number OID structure with similar
  // numeric ranges.
  return absl::StrCat("urn:oid:", absl::Uniform(bitgen_, 1, 3), ".",
                      absl::Uniform(bitgen_, 10, 10), ".",
                      absl::Uniform(bitgen_, 100000, 200000), ".",
                      absl::Uniform(bitgen_, 100, 1000));
}

std::string RandomValueProvider::GetCode(const google::protobuf::FieldDescriptor* field,
                                         int recursion_depth) {
  return RandomValueProvider::GetId(field, recursion_depth);
}

const google::protobuf::EnumValueDescriptor* RandomValueProvider::GetCodeEnum(
    const google::protobuf::FieldDescriptor* primitive_field,
    const google::protobuf::FieldDescriptor* value_field, int recursion_depth) {
  int index = absl::Uniform<int>(absl::IntervalClosedOpen, bitgen_, 1,
                                 value_field->enum_type()->value_count());
  return value_field->enum_type()->value(index);
}

std::string RandomValueProvider::GetMarkdown(
    const google::protobuf::FieldDescriptor* field, int recursion_depth) {
  return GetString(field, recursion_depth);
}

std::string RandomValueProvider::GetXhtml(const google::protobuf::FieldDescriptor* field,
                                          int recursion_depth) {
  return GetString(field, recursion_depth);
}

std::string RandomValueProvider::GetReferenceType(
    const google::protobuf::FieldDescriptor* field, int recursion_depth) {
  int ref_type_count =
      field->options().ExtensionSize(proto::valid_reference_type);
  if (ref_type_count == 0) {
    return "Resource";
  }

  return field->options().GetExtension(
      proto::valid_reference_type,
      absl::Uniform<int>(bitgen_, 0, ref_type_count));
}

std::string RandomValueProvider::GetReferenceId(
    const google::protobuf::FieldDescriptor* field, int recursion_depth) {
  return GetId(field, recursion_depth);
}

std::string RandomValueProvider::GetYear() {
  return absl::StrFormat("%04d", absl::Uniform<int>(bitgen_, 1900, 2100));
}

std::string RandomValueProvider::GetYearMonth() {
  return absl::StrFormat("%04d-%02d", absl::Uniform<int>(bitgen_, 1900, 2100),
                         absl::Uniform<int>(bitgen_, 1, 13));
}

std::string RandomValueProvider::GetFullDate() {
  return absl::StrFormat(
      "%04d-%02d-%02d", absl::Uniform<int>(bitgen_, 1900, 2100),
      absl::Uniform<int>(bitgen_, 1, 13), absl::Uniform<int>(bitgen_, 1, 29));
}

absl::Status FhirGenerator::FillPrimitive(
    const google::protobuf::FieldDescriptor* field, google::protobuf::Message* message,
    absl::flat_hash_map<const google::protobuf::Descriptor*, int>* recursion_count) {
  int recursion_depth = (*recursion_count)[field->message_type()];
  google::protobuf::Message* fhir_primitive =
      ::google::fhir::MutableOrAddMessage(message, field);
  std::unique_ptr<internal::FhirJson> value;

  if (google::fhir::IsBoolean(*fhir_primitive)) {
    value = internal::FhirJson::CreateBoolean(
        value_provider_->GetBoolean(field, recursion_depth));
  } else if (google::fhir::IsBase64Binary(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetBase64Binary(field, recursion_depth));
  } else if (google::fhir::IsString(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetString(field, recursion_depth));
  } else if (google::fhir::IsInteger(*fhir_primitive)) {
    value = internal::FhirJson::CreateInteger(
        value_provider_->GetInteger(field, recursion_depth));
  } else if (google::fhir::IsPositiveInt(*fhir_primitive)) {
    value = internal::FhirJson::CreateUnsigned(
        value_provider_->GetPositiveInt(field, recursion_depth));
  } else if (google::fhir::IsUnsignedInt(*fhir_primitive)) {
    value = internal::FhirJson::CreateUnsigned(
        value_provider_->GetUnsignedInt(field, recursion_depth));
  } else if (google::fhir::IsDecimal(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetDecimal(field, recursion_depth));
  } else if (google::fhir::IsDateTime(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetDateTime(field, recursion_depth));
  } else if (google::fhir::IsDate(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetDate(field, recursion_depth));
  } else if (google::fhir::IsTime(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetTime(field, recursion_depth));
  } else if (google::fhir::IsInstant(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetInstant(field, recursion_depth));
  } else if (google::fhir::IsId(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetId(field, recursion_depth));
  } else if (google::fhir::IsUuid(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetUuid(field, recursion_depth));
  } else if (google::fhir::IsIdentifier(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetIdentifier(field, recursion_depth));
  } else if (google::fhir::IsUri(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetUri(field, recursion_depth));
  } else if (google::fhir::IsUrl(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetUrl(field, recursion_depth));
  } else if (google::fhir::IsOid(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetOid(field, recursion_depth));
  } else if (google::fhir::IsCanonical(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetCanonical(field, recursion_depth));
  } else if (google::fhir::IsCode(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetCode(field, recursion_depth));
  } else if (google::fhir::IsXhtml(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetXhtml(field, recursion_depth));
  } else if (google::fhir::IsMarkdown(*fhir_primitive)) {
    value = internal::FhirJson::CreateString(
        value_provider_->GetMarkdown(field, recursion_depth));
  } else if (google::fhir::IsProfileOfCode(*fhir_primitive)) {
    // Profile codes use the enumerated value when present.
    const google::protobuf::FieldDescriptor* value_field =
        fhir_primitive->GetDescriptor()->FindFieldByName("value");
    if (value_field->type() == google::protobuf::FieldDescriptor::TYPE_ENUM) {
      const google::protobuf::EnumValueDescriptor* enum_value =
          value_provider_->GetCodeEnum(field, value_field, recursion_depth);
      fhir_primitive->GetReflection()->SetEnum(fhir_primitive, value_field,
                                               enum_value);
    } else {
      value = internal::FhirJson::CreateString(
          value_provider_->GetCode(field, recursion_depth));
    }
  } else {
    return absl::InvalidArgumentError(absl::StrCat(
        "Unrecognized type: ", fhir_primitive->GetTypeName(), " structure: ",
        GetStructureDefinitionUrl(fhir_primitive->GetDescriptor())));
  }

  if (value) {
    ScopedErrorReporter reporter(&FailFastErrorHandler::FailOnErrorOrFatal(),
                                 fhir_primitive->GetDescriptor()->name());
    FHIR_ASSIGN_OR_RETURN(
        ParseResult result,
        primitive_handler_->ParseInto(*value, fhir_primitive, reporter));

    // This should be impossible, since anything that causes succeeded to
    // be false should have been reported as fatal, and short-circuited due to
    // the FailOnErrorOrFatal error handler.
    if (result == ParseResult::kFailed) {
      return absl::InternalError(
          "Unexpected merge failure in JsonFhirStringToProtoWithoutValidating");
    }
  }
  return absl::OkStatus();
}

absl::Status FhirGenerator::FillReference(
    const google::protobuf::FieldDescriptor* field, google::protobuf::Message* message,
    absl::flat_hash_map<const google::protobuf::Descriptor*, int>* recursion_count) {
  // Get the URI field to populate
  google::protobuf::Message* fhir_reference =
      ::google::fhir::MutableOrAddMessage(message, field);

  // Set the URI value then split it into the ID fields per type.
  int recursion_depth = (*recursion_count)[field->message_type()];
  std::string reference_type =
      value_provider_->GetReferenceType(field, recursion_depth);

  // References to the generic Resource type can't be relative URIs, so create
  // an external identifier instead.
  if (reference_type == "Resource") {
    const google::protobuf::FieldDescriptor* identifier_field =
        fhir_reference->GetDescriptor()->FindFieldByName("identifier");

    google::protobuf::Message* identifier =
        fhir_reference->GetReflection()->MutableMessage(fhir_reference,
                                                        identifier_field);
    return Fill(identifier);

  } else {
    const google::protobuf::FieldDescriptor* uri_field =
        fhir_reference->GetDescriptor()->FindFieldByName("uri");
    google::protobuf::Message* uri_message =
        fhir_reference->GetReflection()->MutableMessage(fhir_reference,
                                                        uri_field);
    std::string reference_id =
        value_provider_->GetReferenceId(field, recursion_depth);
    auto fhir_json = internal::FhirJson::CreateString(
        absl::StrCat(reference_type, "/", reference_id));

    ScopedErrorReporter reporter(&FailFastErrorHandler::FailOnErrorOrFatal(),
                                 "");
    FHIR_ASSIGN_OR_RETURN(
        ParseResult result,
        primitive_handler_->ParseInto(*fhir_json, uri_message, reporter));

    // This should be impossible, since anything that causes succeeded to
    // be false should have been reported as fatal, and short-circuited due to
    // the FailOnErrorOrFatal error handler.
    // TODO(b/237300807): Remove this branch once we've deprecated
    // FastFailErrorHandlers.
    if (result == ParseResult::kFailed) {
      return absl::InternalError(
          "Unexpected merge failure in JsonFhirStringToProtoWithoutValidating");
    }
    return SplitIfRelativeReference(fhir_reference);
  }
}

bool FhirGenerator::ShouldFill(
    const google::protobuf::FieldDescriptor* field, google::protobuf::Message* message,
    absl::flat_hash_map<const google::protobuf::Descriptor*, int>* recursion_count) {
  // "Any" types and contained resources not currently supported.
  if (IsMessageType<::google::protobuf::Any>(field->message_type())) {
    return false;
  }

  // Always populate the id fields on FHIR resources so they are usable in
  // FHIR stores and to track down errors.
  if (field->name() == "id" &&
      message->GetDescriptor()->options().HasExtension(
          ::google::fhir::proto::structure_definition_kind) &&
      message->GetDescriptor()->options().GetExtension(
          ::google::fhir::proto::structure_definition_kind) ==
          ::google::fhir::proto::KIND_RESOURCE) {
    return true;
  }

  // Required fields and fields in choice types must always be filled.
  // For other fields we ask the value provider.
  bool is_required_field = field->options().HasExtension(
                               ::google::fhir::proto::validation_requirement) &&
                           field->options().GetExtension(
                               ::google::fhir::proto::validation_requirement) ==
                               ::google::fhir::proto::REQUIRED_BY_FHIR;
  bool is_choice_type = message->GetDescriptor()->options().HasExtension(
      ::google::fhir::proto::is_choice_type);
  return is_required_field || is_choice_type ||
         value_provider_->ShouldFill(field,
                                     (*recursion_count)[field->message_type()]);
}

absl::Status FhirGenerator::Fill(
    google::protobuf::Message* message,
    absl::flat_hash_map<const google::protobuf::Descriptor*, int>* recursion_count) {
  // Increment the recursion count for this message type.
  const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
  (*recursion_count)[descriptor]++;

  // Group oneof fields by descriptor so the value provider can select
  // only one. Non-oneof fields use a null key.
  absl::flat_hash_map<const google::protobuf::OneofDescriptor*,
                      std::vector<const google::protobuf::FieldDescriptor*>>
      one_of_groups;

  for (int i = 0; i < descriptor->field_count(); i++) {
    const google::protobuf::FieldDescriptor* field = descriptor->field(i);
    const google::protobuf::OneofDescriptor* one_of = field->containing_oneof();
    one_of_groups[one_of].push_back(field);
  }

  for (const auto& one_of_group : one_of_groups) {
    std::vector<const google::protobuf::FieldDescriptor*> descriptors =
        one_of_group.second;

    // If the field is part of a oneof group, select only one to use
    // and clear the others.
    if (one_of_group.first != nullptr) {
      const google::protobuf::FieldDescriptor* oneof_to_use =
          value_provider_->SelectOneOf(message, descriptors);
      descriptors.clear();
      descriptors.push_back(oneof_to_use);
    }

    for (const auto& field : descriptors) {
      if (ShouldFill(field, message, recursion_count)) {
        int num_to_add =
            field->is_repeated()
                ? value_provider_->GetNumRepeated(
                      field, (*recursion_count)[field->message_type()])
                : 1;

        for (int i = 0; i < num_to_add; ++i) {
          if (IsPrimitive(field->message_type())) {
            FHIR_RETURN_IF_ERROR(
                FillPrimitive(field, message, recursion_count));
          } else if (google::fhir::IsReference(field->message_type())) {
            FHIR_RETURN_IF_ERROR(
                FillReference(field, message, recursion_count));
          } else {
            google::protobuf::Message* child =
                ::google::fhir::MutableOrAddMessage(message, field);
            FHIR_RETURN_IF_ERROR(Fill(child, recursion_count));
          }
        }
      }
    }
  }
  (*recursion_count)[descriptor]--;
  return absl::OkStatus();
}

}  // namespace testutil
}  // namespace fhir
}  // namespace google
