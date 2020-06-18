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

#include "google/fhir/seqex/resource_to_example.h"

#include <map>
#include <memory>
#include <string>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "absl/algorithm/container.h"
#include "absl/flags/flag.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/time/time.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codeable_concepts.h"
#include "google/fhir/codes.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/seqex/text_tokenizer.h"
#include "google/fhir/status/status.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "tensorflow/core/example/example.pb.h"
#include "tensorflow/core/example/feature.pb.h"
#include "tensorflow/core/platform/logging.h"
#include "re2/re2.h"

ABSL_FLAG(std::string, tokenize_feature_list, "Composition.section.text.div",
          "Comma separated feature name list for tokenizing string values. "
          "Doesn't include the original value as a feature. "
          "If a feature name is in this list, "
          "it can't be in add_tokenize_feature_list");
ABSL_FLAG(std::string, add_tokenize_feature_list, "",
          "Comma separated feature name list for tokenizing string values. "
          "Includes the original value as a feature as well. "
          "If a feature name is in this list, "
          "it can't be in tokenize_feature_list");
ABSL_FLAG(bool, tokenize_code_text_features, true,
          "Tokenize all the Coding.display and CodeableConcept.text fields. "
          "Doesn't include the original value as a feature unless it's in "
          "add_tokenize_feature_list.");

namespace google {
namespace fhir {
namespace seqex {

using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

namespace {

class Converter {
 public:
  explicit Converter(const PrimitiveHandler* primitive_handler)
      : primitive_handler_(primitive_handler) {}

  void MessageToExample(const Message& message, const std::string& prefix,
                        const TextTokenizer& tokenizer,
                        ::tensorflow::Example* example,
                        bool enable_attribution);

 private:
  void AddTokensToExample(const std::string& name, const std::string& value,
                          const TextTokenizer& tokenizer,
                          ::tensorflow::Example* example,
                          bool enable_attribution) {
    auto tokens = tokenizer.Tokenize(value);
    auto* token_list =
        (*example->mutable_features()
              ->mutable_feature())[absl::StrCat(name, ".tokenized")]
            .mutable_bytes_list();
    ::tensorflow::Int64List* start_list = nullptr;
    ::tensorflow::Int64List* end_list = nullptr;
    if (enable_attribution) {
      start_list =
          (*example->mutable_features()
                ->mutable_feature())[absl::StrCat(name, ".token_start")]
              .mutable_int64_list();
      end_list = (*example->mutable_features()
                       ->mutable_feature())[absl::StrCat(name, ".token_end")]
                     .mutable_int64_list();
    }
    for (const auto& token : tokens) {
      token_list->add_value(token.text);
      if (enable_attribution) {
        start_list->add_value(token.char_start);
        end_list->add_value(token.char_end);
      }
    }
  }

  // This function does not support the case where the intersection of
  // tokenize_feature_set and add_tokenize_feature_set is not empty, because
  // that case is ambiguous.
  void AddValueAndOrTokensToExample(
      const std::set<std::string>& tokenize_feature_set,
      const std::set<std::string>& add_tokenize_feature_set,
      const TextTokenizer& tokenizer, const std::string& name,
      const std::string& value, ::tensorflow::Example* example,
      bool enable_attribution) {
    if (tokenize_feature_set.count(name) != 0) {
      AddTokensToExample(name, value, tokenizer, example, enable_attribution);
      return;
    }

    if (add_tokenize_feature_set.count(name) != 0) {
      AddTokensToExample(name, value, tokenizer, example, enable_attribution);
    }

    (*example->mutable_features()->mutable_feature())[name]
        .mutable_bytes_list()
        ->add_value(value);
  }

  absl::Status AddLegacyFixedSystemCodingToExample(
      const std::string& original_system,
      const ::google::protobuf::Message& fixed_system_coding, const std::string& name,
      bool append_system_to_feature_name, ::tensorflow::Example* example,
      std::set<std::string>* tokenize_feature_set,
      const std::set<std::string>& add_tokenize_feature_set,
      const TextTokenizer& tokenizer, bool enable_attribution) {
    const Descriptor* descriptor = fixed_system_coding.GetDescriptor();
    const FieldDescriptor* code_field = descriptor->FindFieldByName("code");
    const FieldDescriptor* display_field =
        descriptor->FindFieldByName("display");

    if (!code_field || !code_field->message_type() || !display_field ||
        !display_field->message_type()) {
      return absl::InvalidArgumentError(absl::StrCat(
          "Invalid coding for AddLegacyFixedSystemCodingToExample: ",
          descriptor->full_name()));
    }

    const Reflection* reflection = fixed_system_coding.GetReflection();
    std::string code_scratch;
    FHIR_ASSIGN_OR_RETURN(
        const std::string& original_code,
        GetPrimitiveStringValue(
            reflection->GetMessage(fixed_system_coding, code_field),
            &code_scratch));
    std::string display_scratch;
    FHIR_ASSIGN_OR_RETURN(
        const std::string& original_display,
        GetPrimitiveStringValue(
            reflection->GetMessage(fixed_system_coding, display_field),
            &display_scratch));

    return AddCodingToExample(original_system, original_code, original_display,
                              name, append_system_to_feature_name, example,
                              tokenize_feature_set, add_tokenize_feature_set,
                              tokenizer, enable_attribution);
  }

  absl::Status AddCodingToExample(
      const std::string& original_system, const std::string& original_code,
      const std::string& original_display, const std::string& name,
      bool append_system_to_feature_name, ::tensorflow::Example* example,
      std::set<std::string>* tokenize_feature_set,
      const std::set<std::string>& add_tokenize_feature_set,
      const TextTokenizer& tokenizer, bool enable_attribution) {
    if (original_system.empty() || original_code.empty()) {
      return absl::InvalidArgumentError(
          "Encountered invalid Coding: must have `code` and `system`.");
    }
    const std::string system = absl::StrReplaceAll(
        original_system, {{"://", "-"}, {":", "-"}, {"/", "-"}, {".", "-"}});
    const std::string feature_name =
        append_system_to_feature_name ? absl::StrCat(name, ".", system) : name;
    const std::string code = append_system_to_feature_name
                                 ? original_code
                                 : absl::StrCat(system, ":", original_code);
    (*example->mutable_features()->mutable_feature())[feature_name]
        .mutable_bytes_list()
        ->add_value(code);
    if (!original_display.empty()) {
      const std::string full_name = absl::StrCat(feature_name, ".display");
      if (absl::GetFlag(FLAGS_tokenize_code_text_features)) {
        tokenize_feature_set->insert(full_name);
      }
      AddValueAndOrTokensToExample(
          *tokenize_feature_set, add_tokenize_feature_set, tokenizer,
          absl::StrCat(name, ".", system, ".display"), original_display,
          example, enable_attribution);
    }
    return absl::OkStatus();
  }

  const PrimitiveHandler* primitive_handler_;
};

StatusOr<int64_t> GetUnixSeconds(const Message& timelike) {
  const ::google::protobuf::Descriptor* descriptor = timelike.GetDescriptor();
  const ::google::protobuf::FieldDescriptor* value_field =
      descriptor->FindFieldByName("value_us");
  if (!IsPrimitive(descriptor) || !value_field ||
      value_field->type() != ::google::protobuf::FieldDescriptor::Type::TYPE_INT64) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Invalid type for GetAsAbslTime: ", descriptor->full_name()));
  }
  return absl::ToUnixSeconds(absl::FromUnixMicros(
      timelike.GetReflection()->GetInt64(timelike, value_field)));
}

void Converter::MessageToExample(const Message& message,
                                 const std::string& prefix,
                                 const TextTokenizer& tokenizer,
                                 ::tensorflow::Example* example,
                                 bool enable_attribution) {
  std::set<std::string> tokenize_feature_set;
  const std::string& tokenize_feature_list =
      absl::GetFlag(FLAGS_tokenize_feature_list);
  if (!tokenize_feature_list.empty()) {
    tokenize_feature_set =
        absl::StrSplit(tokenize_feature_list, ',', absl::SkipEmpty());
  }
  std::set<std::string> add_tokenize_feature_set;
  if (!absl::GetFlag(FLAGS_add_tokenize_feature_list).empty()) {
    add_tokenize_feature_set = absl::StrSplit(
        absl::GetFlag(FLAGS_add_tokenize_feature_list), ',', absl::SkipEmpty());
  }

  std::set<std::string> intersection;
  absl::c_set_intersection(tokenize_feature_set, add_tokenize_feature_set,
                           std::inserter(intersection, intersection.end()));
  QCHECK(intersection.empty())
      << "Feature can't be both in tokenize_feature_list and "
         "add_tokenize_feature_list";

  const google::protobuf::Reflection* reflection = message.GetReflection();

  std::vector<const google::protobuf::FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  for (const auto* field : fields) {
    int count = 0;
    if (field->is_repeated()) {
      count = reflection->FieldSize(message, field);
    } else if (reflection->HasField(message, field)) {
      count = 1;
    }
    const std::string name = absl::StrCat(prefix, ".", field->json_name());
    const auto* field_type = field->message_type();
    for (int i = 0; i < count; i++) {
      if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        const Message& child =
            field->is_repeated()
                ? reflection->GetRepeatedMessage(message, field, i)
                : reflection->GetMessage(message, field);

        // Handle some fhir types explicitly, including primitive types.
        if (IsTypeOrProfileOfCode(child)) {
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_bytes_list()
              ->add_value(GetCodeAsString(child).ValueOrDie());
        } else if (IsDate(field_type) || IsDateTime(field_type) ||
                   IsTime(field_type) || IsInstant(field_type)) {
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_int64_list()
              ->add_value(GetUnixSeconds(child).ValueOrDie());
        } else if (IsString(child)) {
          const std::string& value =
              primitive_handler_->GetStringValue(child).ValueOrDie();
          if (absl::GetFlag(FLAGS_tokenize_code_text_features) &&
              field->name() == "text" &&
              IsTypeOrProfileOfCodeableConcept(message)) {
            AddTokensToExample(name, value, tokenizer, example,
                               enable_attribution);
          } else {
            AddValueAndOrTokensToExample(
                tokenize_feature_set, add_tokenize_feature_set, tokenizer, name,
                value, example, enable_attribution);
          }
        } else if (IsXhtml(child)) {
          std::string scratch;
          AddValueAndOrTokensToExample(
              tokenize_feature_set, add_tokenize_feature_set, tokenizer, name,
              GetPrimitiveStringValue(child, &scratch).ValueOrDie(), example,
              enable_attribution);
        } else if (IsBoolean(child)) {
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_bytes_list()
              ->add_value(
                  primitive_handler_->GetBooleanValue(child).ValueOrDie()
                      ? "true"
                      : "false");
        } else if (IsInteger(child)) {
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_int64_list()
              ->add_value(
                  primitive_handler_->GetIntegerValue(child).ValueOrDie());
        } else if (IsPositiveInt(child)) {
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_int64_list()
              ->add_value(
                  primitive_handler_->GetPositiveIntValue(child).ValueOrDie());
        } else if (IsDecimal(child)) {
          const std::string& value_as_string =
              primitive_handler_->GetDecimalValue(child).ValueOrDie();
          double value_as_double;
          CHECK(absl::SimpleAtod(value_as_string, &value_as_double));
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_float_list()
              ->add_value(value_as_double);
        } else if (IsTypeOrProfileOfCoding(field_type)) {
          // Codings are emitted with the system if we know it, as raw codes
          // otherwise.
          bool use_name_in_feature_name =
              IsTypeOrProfileOfCodeableConcept(message);
          if (field->message_type()->full_name() ==
              "google.fhir.stu3.proto.CodingWithFixedSystem") {
            // Legacy codepath for Stu3 fixed-system codings
            FHIR_CHECK_OK(AddLegacyFixedSystemCodingToExample(
                GetInlinedCodingSystem(field), child,
                use_name_in_feature_name ? prefix : name,
                use_name_in_feature_name, example, &tokenize_feature_set,
                add_tokenize_feature_set, tokenizer, enable_attribution));
          } else {
            FHIR_CHECK_OK(AddCodingToExample(
                primitive_handler_->GetCodingSystem(child).ValueOrDie(),
                primitive_handler_->GetCodingCode(child).ValueOrDie(),
                primitive_handler_->GetCodingDisplay(child).ValueOrDie(),
                use_name_in_feature_name ? prefix : name,
                use_name_in_feature_name, example, &tokenize_feature_set,
                add_tokenize_feature_set, tokenizer, enable_attribution));
          }
        } else if (IsExtension(child)) {
          // We do not emit extensions. To include data encoded in extensions
          // in generated examples, use profiled bundles as input.
        } else if (IsIdentifier(child)) {
          // We don't emit identifiers.
          // TODO: are there situations where we should?
        } else if (IsBase64Binary(child)) {
          // We don't emit Base64Binary.
          // TODO: are there situations where we should?
        } else if (IsReference(child.GetDescriptor()) || IsUri(child)) {
          // We don't emit any stu3 references or URIs.
        } else {
          // We currently flatten repeated submessages. That could potentially
          // be problematic.
          // TODO: figure out something better to do here.
          MessageToExample(child, name, tokenizer, example, enable_attribution);
        }
      } else {
        LOG(ERROR) << "Unable to handle field " << name
                   << " in message of type "
                   << message.GetDescriptor()->full_name();
      }
    }
  }
}

}  // namespace

void ResourceToExample(const Message& message, const TextTokenizer& tokenizer,
                       ::tensorflow::Example* example, bool enable_attribution,
                       const PrimitiveHandler* primitive_handler) {
  const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
  Converter(primitive_handler)
      .MessageToExample(message, descriptor->name(), tokenizer, example,
                        enable_attribution);
}

}  // namespace seqex
}  // namespace fhir
}  // namespace google
