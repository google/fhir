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
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/time/time.h"
#include "google/fhir/codeable_concepts.h"
#include "google/fhir/codes.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/seqex/text_tokenizer.h"
#include "google/fhir/status/status.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
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

using ::google::fhir::stu3::proto::Base64Binary;
using ::google::fhir::stu3::proto::Boolean;
using ::google::fhir::stu3::proto::Coding;
using ::google::fhir::stu3::proto::Date;
using ::google::fhir::stu3::proto::DateTime;
using ::google::fhir::stu3::proto::Decimal;
using ::google::fhir::stu3::proto::Extension;
using ::google::fhir::stu3::proto::Id;
using ::google::fhir::stu3::proto::Identifier;
using ::google::fhir::stu3::proto::Instant;
using ::google::fhir::stu3::proto::Integer;
using ::google::fhir::stu3::proto::PositiveInt;
using ::google::fhir::stu3::proto::Reference;
using ::google::fhir::stu3::proto::String;
using ::google::fhir::stu3::proto::Time;
using ::google::fhir::stu3::proto::Uri;
using ::google::fhir::stu3::proto::Xhtml;
using ::google::protobuf::Message;

namespace {

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
    start_list = (*example->mutable_features()
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
// tokenize_feature_set and add_tokenize_feature_set is not empty, because that
// case is ambiguous.
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

}  // namespace

void AddCodingToExample(const Coding& coding, const std::string& name,
                        bool append_system_to_feature_name,
                        ::tensorflow::Example* example,
                        std::set<std::string>* tokenize_feature_set,
                        const std::set<std::string>& add_tokenize_feature_set,
                        const TextTokenizer& tokenizer,
                        bool enable_attribution) {
  CHECK(coding.has_system() && coding.has_code());
  const std::string system =
      absl::StrReplaceAll(coding.system().value(),
                          {{"://", "-"}, {":", "-"}, {"/", "-"}, {".", "-"}});
  const std::string feature_name =
      append_system_to_feature_name ? absl::StrCat(name, ".", system) : name;
  const std::string code =
      append_system_to_feature_name
          ? coding.code().value()
          : absl::StrCat(system, ":", coding.code().value());
  (*example->mutable_features()->mutable_feature())[feature_name]
      .mutable_bytes_list()
      ->add_value(code);
  if (coding.has_display()) {
    const std::string full_name = absl::StrCat(feature_name, ".display");
    if (absl::GetFlag(FLAGS_tokenize_code_text_features)) {
      tokenize_feature_set->insert(full_name);
    }
    AddValueAndOrTokensToExample(
        *tokenize_feature_set, add_tokenize_feature_set, tokenizer,
        absl::StrCat(name, ".", system, ".display"), coding.display().value(),
        example, enable_attribution);
  }
}

void MessageToExample(const google::protobuf::Message& message, const std::string& prefix,
                      const TextTokenizer& tokenizer,
                      ::tensorflow::Example* example, bool enable_attribution) {
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
    for (int i = 0; i < count; i++) {
      if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        const google::protobuf::Message& child =
            field->is_repeated()
                ? reflection->GetRepeatedMessage(message, field, i)
                : reflection->GetMessage(message, field);

        // Handle some fhir types explicitly, including primitive STU3 types.
        if (IsTypeOrProfileOfCode(child)) {
          // Codes are emitted as-is, without tokenization.
          stu3::proto::Code code;
          CHECK(CopyCode(child, &code).ok());
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_bytes_list()
              ->add_value(code.value());
        } else if (field->message_type()->full_name() ==
                   Date::descriptor()->full_name()) {
          Date date;
          date.CopyFrom(child);
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_int64_list()
              ->add_value(
                  absl::ToUnixSeconds(absl::FromUnixMicros(date.value_us())));
        } else if (field->message_type()->full_name() ==
                   DateTime::descriptor()->full_name()) {
          DateTime datetime;
          datetime.CopyFrom(child);
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_int64_list()
              ->add_value(absl::ToUnixSeconds(
                  absl::FromUnixMicros(datetime.value_us())));
        } else if (field->message_type()->full_name() ==
                   Time::descriptor()->full_name()) {
          Time time;
          time.CopyFrom(child);
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_int64_list()
              ->add_value(
                  absl::ToUnixSeconds(absl::FromUnixMicros(time.value_us())));
        } else if (field->message_type()->full_name() ==
                   Instant::descriptor()->full_name()) {
          Instant instant;
          instant.CopyFrom(child);
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_int64_list()
              ->add_value(absl::ToUnixSeconds(
                  absl::FromUnixMicros(instant.value_us())));
        } else if (field->message_type()->full_name() ==
                   String::descriptor()->full_name()) {
          String s;
          s.CopyFrom(child);
          if (absl::GetFlag(FLAGS_tokenize_code_text_features) &&
              field->name() == "text" &&
              IsTypeOrProfileOfCodeableConcept(message)) {
            AddTokensToExample(name, s.value(), tokenizer, example,
                               enable_attribution);
          } else {
            AddValueAndOrTokensToExample(
                tokenize_feature_set, add_tokenize_feature_set, tokenizer, name,
                s.value(), example, enable_attribution);
          }
        } else if (field->message_type()->full_name() ==
                   Xhtml::descriptor()->full_name()) {
          Xhtml xhtml;
          xhtml.CopyFrom(child);
          AddValueAndOrTokensToExample(
              tokenize_feature_set, add_tokenize_feature_set, tokenizer, name,
              xhtml.value(), example, enable_attribution);
        } else if (field->message_type()->full_name() ==
                   Boolean::descriptor()->full_name()) {
          Boolean boolean;
          boolean.CopyFrom(child);
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_bytes_list()
              ->add_value(boolean.value() ? "true" : "false");
        } else if (field->message_type()->full_name() ==
                   Integer::descriptor()->full_name()) {
          Integer integer;
          integer.CopyFrom(child);
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_int64_list()
              ->add_value(integer.value());
        } else if (field->message_type()->full_name() ==
                   PositiveInt::descriptor()->full_name()) {
          PositiveInt positive_int;
          positive_int.CopyFrom(child);
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_int64_list()
              ->add_value(positive_int.value());
        } else if (field->message_type()->full_name() ==
                   Decimal::descriptor()->full_name()) {
          Decimal decimal;
          decimal.CopyFrom(child);
          double value_as_double;
          CHECK(absl::SimpleAtod(decimal.value(), &value_as_double));
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_float_list()
              ->add_value(value_as_double);
        } else if (IsTypeOrProfileOfCoding(field->message_type())) {
          // Codings are emitted with the system if we know it, as raw codes
          // otherwise.
          bool use_name_in_feature_name =
              IsTypeOrProfileOfCodeableConcept(message);
          Coding coding;
          auto status = CopyCoding(child, &coding);
          if (!status.ok() &&
              field->message_type()->full_name() ==
                  stu3::proto::CodingWithFixedSystem::descriptor()
                      ->full_name()) {
            // TODO: remove this code path.
            stu3::proto::CodingWithFixedSystem fixed_coding;
            fixed_coding.CopyFrom(child);
            *coding.mutable_code() = fixed_coding.code();
            *coding.mutable_system()->mutable_value() =
                GetInlinedCodingSystem(field);
            if (fixed_coding.has_display()) {
              *coding.mutable_display() = fixed_coding.display();
            }
            AddCodingToExample(coding, prefix, true, example,
                               &tokenize_feature_set, add_tokenize_feature_set,
                               tokenizer, enable_attribution);
          } else {
            CHECK(status.ok()) << status;
            AddCodingToExample(coding, use_name_in_feature_name ? prefix : name,
                               use_name_in_feature_name, example,
                               &tokenize_feature_set, add_tokenize_feature_set,
                               tokenizer, enable_attribution);
          }
        } else if (field->message_type()->full_name() ==
                   Extension::descriptor()->full_name()) {
          // We do not emit extensions. To include data encoded in extensions
          // in generated examples, please use profiled bundles as input.
        } else if (field->message_type()->full_name() ==
                       Identifier::descriptor()->full_name() ||
                   field->message_type()->full_name() ==
                       Id::descriptor()->full_name()) {
          // We don't emit identifiers.
          // TODO: are there situations where we should?
        } else if (field->message_type()->full_name() ==
                   Base64Binary::descriptor()->full_name()) {
          // We don't emit Base64Binary.
          // TODO: are there situations where we should?
        } else if (field->message_type()->full_name() ==
                       Reference::descriptor()->full_name() ||
                   field->message_type()->options().ExtensionSize(
                       proto::fhir_reference_type) > 0 ||
                   field->message_type()->full_name() ==
                       Uri::descriptor()->full_name()) {
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

void ResourceToExample(const google::protobuf::Message& message,
                       const TextTokenizer& tokenizer,
                       ::tensorflow::Example* example,
                       bool enable_attribution) {
  const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
  MessageToExample(message, descriptor->name(), tokenizer, example,
                   enable_attribution);
}

}  // namespace seqex
}  // namespace fhir
}  // namespace google
