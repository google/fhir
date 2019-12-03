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
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/time/time.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/seqex/text_tokenizer.h"
#include "google/fhir/status/status.h"
#include "google/fhir/stu3/codeable_concepts.h"
#include "google/fhir/systems/systems.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/example/example.pb.h"
#include "tensorflow/core/example/feature.pb.h"
#include "tensorflow/core/lib/core/errors.h"
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
ABSL_FLAG(std::string, tokenizer, "simple", "Which tokenizer to use.");

namespace google {
namespace fhir {
namespace seqex {

using ::google::fhir::stu3::proto::Base64Binary;
using ::google::fhir::stu3::proto::Boolean;
using ::google::fhir::stu3::proto::CodeableConcept;
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
using ::tensorflow::Status;

namespace {

void AddTokensToExample(const std::string& name, const std::string& value,
                        ::tensorflow::Example* example,
                        bool enable_attribution) {
  std::unique_ptr<TextTokenizer> tokenizer;
  // Strings are tokenized if in the whitelist.
  if (absl::GetFlag(FLAGS_tokenizer) == "simple") {
    tokenizer = absl::make_unique<SimpleWordTokenizer>(true /* lowercase */);
  } else if (absl::GetFlag(FLAGS_tokenizer) == "single") {
    tokenizer = absl::make_unique<SingleTokenTokenizer>();
  } else {
    LOG(FATAL) << "Unknown tokenizer: " << absl::GetFlag(FLAGS_tokenizer);
  }
  auto tokens = tokenizer->Tokenize(value);
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
    const std::string& name, const std::string& value,
    ::tensorflow::Example* example, bool enable_attribution) {
  if (tokenize_feature_set.count(name) != 0) {
    AddTokensToExample(name, value, example, enable_attribution);
    return;
  }

  if (add_tokenize_feature_set.count(name) != 0) {
    AddTokensToExample(name, value, example, enable_attribution);
  }

  (*example->mutable_features()->mutable_feature())[name]
      .mutable_bytes_list()
      ->add_value(value);
}

// Extract code value for a given system code. Return as soon as we find one.

Status ExtractCodeBySystem(const Message& codeable_concept,
                           absl::string_view system_value,
                           std::string* result) {
  auto status_or_result =
      google::fhir::stu3::ExtractCodeBySystem(codeable_concept, system_value);
  if (status_or_result.ok()) {
    *result = status_or_result.ValueOrDie();
    return Status::OK();
  }
  return status_or_result.status();
}

Status ExtractIcdCode(const Message& codeable_concept,
                      const std::vector<std::string>& schemes,
                      std::string* result) {
  bool found_response = false;
  std::string intermediate_result;
  for (size_t i = 0; i < schemes.size(); i++) {
    StatusOr<std::string> s =
        google::fhir::stu3::ExtractCodeBySystem(codeable_concept, schemes[i]);

    if (s.status().code() == ::tensorflow::errors::Code::ALREADY_EXISTS) {
      // Multiple codes, so we can return an error already.
      return s.status();
    } else if (s.ok()) {
      if (found_response) {
        // We found _another_ code. That shouldn't have happened.
        return ::tensorflow::errors::AlreadyExists("Found more than one code");
      } else {
        intermediate_result = s.ValueOrDie();
        found_response = true;
      }
    }
  }
  if (found_response) {
    *result = intermediate_result;
    return Status::OK();
  } else {
    return ::tensorflow::errors::NotFound(
        "No ICD code with the provided schemes in concept.");
  }
}

absl::optional<std::string> GetCodeFromConceptText(const Message& concept) {
  if (stu3::CodingSize(concept) > 0) {
    return absl::nullopt;
  }
  const google::protobuf::FieldDescriptor* text_field =
      concept.GetDescriptor()->FindFieldByName("text");
  if (!text_field) {
    return absl::nullopt;
  }
  if (!FieldHasValue(concept, text_field)) {
    return absl::nullopt;
  }
  const auto& statusor_text =
      GetMessageInField<stu3::proto::String>(concept, text_field);
  if (!statusor_text.ok()) {
    return absl::nullopt;
  }
  return absl::StrCat("text:" + statusor_text.ValueOrDie().value());
}

StatusOr<std::string> GetPreferredCode(const Message& concept) {
  std::string code;
  if (ExtractCodeBySystem(concept, systems::kLoinc, &code).ok()) {
    return absl::StrCat("loinc:" + code);
  }
  if (ExtractIcdCode(concept, *systems::kIcd9Schemes, &code).ok()) {
    return absl::StrCat("icd9:" + code);
  }
  if (ExtractIcdCode(concept, *systems::kIcd10Schemes, &code).ok()) {
    return absl::StrCat("icd10:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kCpt, &code).ok()) {
    return absl::StrCat("cpt:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kNdc, &code).ok()) {
    return absl::StrCat("ndc:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kRxNorm, &code).ok()) {
    return absl::StrCat("rxnorm:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kSnomed, &code).ok()) {
    return absl::StrCat("snomed:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kObservationCategory, &code).ok()) {
    return absl::StrCat("observation_category:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kClaimCategory, &code).ok()) {
    return absl::StrCat("claim_category:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kMaritalStatus, &code).ok()) {
    return absl::StrCat("marital_status:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kNUBCDischarge, &code).ok()) {
    return absl::StrCat("nubc_discharge:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kLanguage, &code).ok()) {
    return absl::StrCat("language:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kDischargeDisposition, &code)
          .ok()) {
    return absl::StrCat("discharge_disposition:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kDiagnosisRole, &code).ok()) {
    return absl::StrCat("diagnosis_role:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kEncounterClass, &code).ok()) {
    return absl::StrCat("actcode:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kConditionCategory, &code).ok()) {
    return absl::StrCat("condition_category:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kProcedureCategory, &code).ok()) {
    return absl::StrCat("procedure_category:" + code);
  }
  if (ExtractCodeBySystem(concept, systems::kConditionVerStatus, &code).ok()) {
    return absl::StrCat("condition_ver_status:" + code);
  }
  return ::tensorflow::errors::NotFound(
      absl::StrCat("No known coding system found in CodeableConcept: ",
                   concept.DebugString()));
}

}  // namespace

std::string GetCode(const Coding& coding) {
  CodeableConcept concept;
  *concept.add_coding() = coding;
  const auto& statusor_code = GetPreferredCode(concept);
  if (statusor_code.ok()) {
    return statusor_code.ValueOrDie();
  } else {
    return coding.code().value();
  }
}

void AddCodeableConceptToExample(
    const Message& concept, const std::string& name,
    ::tensorflow::Example* example, std::set<std::string>* tokenize_feature_set,
    const std::set<std::string>& add_tokenize_feature_set,
    bool enable_attribution) {
  const auto& statusor_code = GetPreferredCode(concept);
  if (!statusor_code.ok()) {
    // Ignore the code that we do not recognize.
    LOG(INFO) << "Unable to handle the codeable concept: "
              << statusor_code.status().error_message();
    return;
  }
  const std::string& code = statusor_code.ValueOrDie();
  // Codeable concepts are emitted using the preferred coding systems.
  (*example->mutable_features()->mutable_feature())[name]
      .mutable_bytes_list()
      ->add_value(code);
  const StatusOr<String>& statusor_text =
      GetMessageInField<String>(concept, "text");
  if (statusor_text.ok() && !statusor_text.ValueOrDie().value().empty()) {
    const std::string full_name = absl::StrCat(name, ".text");
    if (absl::GetFlag(FLAGS_tokenize_code_text_features)) {
      tokenize_feature_set->insert(full_name);
    }
    AddValueAndOrTokensToExample(
        *tokenize_feature_set, add_tokenize_feature_set, full_name,
        statusor_text.ValueOrDie().value(), example, enable_attribution);
  }
  stu3::ForEachCoding(concept, [&](const Coding& coding) {
    CHECK(coding.has_system() && coding.has_code());
    const std::string system =
        systems::ToShortSystemName(coding.system().value());
    (*example->mutable_features()
          ->mutable_feature())[absl::StrCat(name, ".", system)]
        .mutable_bytes_list()
        ->add_value(coding.code().value());
    if (coding.has_display()) {
      const std::string full_name = absl::StrCat(name, ".", system, ".display");
      if (absl::GetFlag(FLAGS_tokenize_code_text_features)) {
        tokenize_feature_set->insert(full_name);
      }
      AddValueAndOrTokensToExample(
          *tokenize_feature_set, add_tokenize_feature_set,
          absl::StrCat(name, ".", system, ".display"), coding.display().value(),
          example, enable_attribution);
    }
  });
}

void MessageToExample(const google::protobuf::Message& message, const std::string& prefix,
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
        if (field->message_type()->full_name() ==
            // Codes are emitted as-is, without tokenization.
            stu3::proto::Code::descriptor()->full_name()) {
          stu3::proto::Code code;
          code.CopyFrom(child);
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_bytes_list()
              ->add_value(code.value());
        } else if (field->message_type()->options().HasExtension(
                       proto::fhir_valueset_url)) {
          // Valueset-constrained codes are emitted without tokenization.
          // Note: some codes (i.e. MimeTypeCode) are represented as non
          // enumerated strings - handled here.
          const google::protobuf::Reflection* reflection = child.GetReflection();
          const google::protobuf::FieldDescriptor* value_field =
              field->message_type()->FindFieldByName("value");
          auto* bytes_list =
              (*example->mutable_features()->mutable_feature())[name]
                  .mutable_bytes_list();
          switch (value_field->cpp_type()) {
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
              const google::protobuf::EnumValueDescriptor* enum_value =
                  reflection->GetEnum(child, value_field);
              bytes_list->add_value(absl::AsciiStrToLower(enum_value->name()));
              break;
            }
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
              std::string str;
              str = reflection->GetStringReference(child, value_field, &str);
              bytes_list->add_value(absl::AsciiStrToLower(str));
              break;
            }
            default:
              LOG(FATAL)
                  << "Unrecognized code type found in resource_to_example.";
          }
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
          AddValueAndOrTokensToExample(tokenize_feature_set,
                                       add_tokenize_feature_set, name,
                                       s.value(), example, enable_attribution);
        } else if (field->message_type()->full_name() ==
                   Xhtml::descriptor()->full_name()) {
          Xhtml xhtml;
          xhtml.CopyFrom(child);
          AddValueAndOrTokensToExample(
              tokenize_feature_set, add_tokenize_feature_set, name,
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
        } else if (stu3::IsCodeableConceptLike(field->message_type())) {
          // Codeable concepts are emitted using the preferred coding systems.
          AddCodeableConceptToExample(
              child, name, example, &tokenize_feature_set,
              add_tokenize_feature_set, enable_attribution);
        } else if (field->message_type()->full_name() ==
                   Coding::descriptor()->full_name()) {
          // Codings are emitted with the system if we know it, as raw codes
          // otherwise.
          Coding coding;
          coding.CopyFrom(child);
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_bytes_list()
              ->add_value(GetCode(coding));
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
          MessageToExample(child, name, example, enable_attribution);
        }
      } else {
        LOG(INFO) << "Unable to handle field " << name << " in message of type "
                  << message.GetDescriptor()->full_name();
      }
    }
  }
}

void ResourceToExample(const google::protobuf::Message& message,
                       ::tensorflow::Example* example,
                       bool enable_attribution) {
  const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
  MessageToExample(message, descriptor->name(), example, enable_attribution);
}

}  // namespace seqex
}  // namespace fhir
}  // namespace google
