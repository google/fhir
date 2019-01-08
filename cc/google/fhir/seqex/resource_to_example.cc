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

#include "gflags/gflags.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "absl/algorithm/container.h"
#include "absl/memory/memory.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/time/time.h"
#include "google/fhir/seqex/text_tokenizer.h"
#include "google/fhir/status/status.h"
#include "google/fhir/stu3/util.h"
#include "google/fhir/systems/systems.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/example/example.pb.h"
#include "tensorflow/core/example/feature.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/platform/logging.h"
#include "re2/re2.h"

DEFINE_string(tokenize_feature_list, "Composition.section.text.div",
              "Comma separated feature name list for tokenizing string values. "
              "Doesn't include the original value as a feature. "
              "If a feature name is in this list, "
              "it can't be in add_tokenize_feature_list");
DEFINE_string(add_tokenize_feature_list, "",
              "Comma separated feature name list for tokenizing string values. "
              "Includes the original value as a feature as well. "
              "If a feature name is in this list, "
              "it can't be in tokenize_feature_list");
DEFINE_bool(tokenize_code_text_features, true,
            "Tokenize all the Coding.display and CodeableConcept.text fields. "
            "Doesn't include the original value as a feature unless it's in "
            "add_tokenize_feature_list.");
DEFINE_string(tokenizer, "simple", "Which tokenizer to use.");

namespace google {
namespace fhir {
namespace seqex {

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
using ::tensorflow::Status;

namespace {

void AddTokensToExample(const string& name, const string& value,
                        ::tensorflow::Example* example,
                        bool enable_attribution) {
  std::unique_ptr<TextTokenizer> tokenizer;
  // Strings are tokenized if in the whitelist.
  if (FLAGS_tokenizer == "simple") {
    tokenizer = absl::make_unique<SimpleWordTokenizer>(true /* lowercase */);
  } else if (FLAGS_tokenizer == "single") {
    tokenizer = absl::make_unique<SingleTokenTokenizer>();
  } else {
    LOG(FATAL) << "Unknown tokenizer: " << FLAGS_tokenizer;
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
    const std::set<string>& tokenize_feature_set,
    const std::set<string>& add_tokenize_feature_set, const string& name,
    const string& value, ::tensorflow::Example* example,
    bool enable_attribution) {
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

Status ExtractCodeBySystem(const CodeableConcept& codeable_concept,
                           absl::string_view system_value, string* result) {
  auto status_or_result =
      stu3::ExtractCodeBySystem(codeable_concept, system_value);
  if (status_or_result.ok()) {
    *result = status_or_result.ValueOrDie();
    return Status::OK();
  }
  return status_or_result.status();
}

Status ExtractIcdCode(const CodeableConcept& codeable_concept,
                      const std::vector<string>& schemes, string* result) {
  auto status_or_result = stu3::ExtractIcdCode(codeable_concept, schemes);
  if (status_or_result.ok()) {
    *result = status_or_result.ValueOrDie();
    return Status::OK();
  }
  return status_or_result.status();
}

Status GetPreferredCode(const CodeableConcept& concept, string* result) {
  string code;
  if (ExtractCodeBySystem(concept, systems::kLoinc, &code).ok()) {
    *result = absl::StrCat("loinc:" + code);
  } else if (ExtractIcdCode(concept, *systems::kIcd9Schemes, &code).ok()) {
    *result = absl::StrCat("icd9:" + code);
  } else if (ExtractIcdCode(concept, *systems::kIcd10Schemes, &code).ok()) {
    *result = absl::StrCat("icd10:" + code);
  } else if (ExtractCodeBySystem(concept, systems::kCpt, &code).ok()) {
    *result = absl::StrCat("cpt:" + code);
  } else if (ExtractCodeBySystem(concept, systems::kNdc, &code).ok()) {
    *result = absl::StrCat("ndc:" + code);
  } else if (ExtractCodeBySystem(concept, systems::kRxNorm, &code).ok()) {
    *result = absl::StrCat("rxnorm:" + code);
  } else if (ExtractCodeBySystem(concept, systems::kSnomed, &code).ok()) {
    *result = absl::StrCat("snomed:" + code);
  } else if (ExtractCodeBySystem(concept, systems::kObservationCategory, &code)
                 .ok()) {
    *result = absl::StrCat("observation_category:" + code);
  } else if (ExtractCodeBySystem(concept, systems::kClaimCategory, &code)
                 .ok()) {
    *result = absl::StrCat("claim_category:" + code);
  } else if (ExtractCodeBySystem(concept, systems::kMaritalStatus, &code)
                 .ok()) {
    *result = absl::StrCat("marital_status:" + code);
  } else if (ExtractCodeBySystem(concept, systems::kNUBCDischarge, &code)
                 .ok()) {
    *result = absl::StrCat("nubc_discharge:" + code);
  } else if (ExtractCodeBySystem(concept, systems::kLanguage, &code)
                 .ok()) {
    *result = absl::StrCat("language:" + code);
  } else if (ExtractCodeBySystem(concept, systems::kDischargeDisposition, &code)
                 .ok()) {
    *result = absl::StrCat("discharge_disposition:" + code);
  } else if (ExtractCodeBySystem(concept, systems::kDiagnosisRole, &code)
                 .ok()) {
    *result = absl::StrCat("diagnosis_role:" + code);
  } else if (ExtractCodeBySystem(concept, systems::kEncounterClass, &code)
                 .ok()) {
    *result = absl::StrCat("actcode:" + code);
  } else {
    return ::tensorflow::errors::NotFound(
        absl::StrCat("No known coding system found in CodeableConcept: ",
                     concept.DebugString()));
  }
  return Status::OK();
}

}  // namespace

string GetCode(const Coding& coding) {
  CodeableConcept concept;
  *concept.add_coding() = coding;
  string code;
  if (GetPreferredCode(concept, &code).ok()) {
    return code;
  } else {
    return coding.code().value();
  }
}

void AddCodeableConceptToExample(
    const CodeableConcept& concept, const string& name,
    ::tensorflow::Example* example, std::set<string>* tokenize_feature_set,
    const std::set<string>& add_tokenize_feature_set, bool enable_attribution) {
  string code;
  Status code_status = GetPreferredCode(concept, &code);
  if (!code_status.ok()) {
    // Ignore the code that we do not recognize.
    LOG(INFO) << "Unable to handle the codeable concept: "
              << code_status.error_message();
    return;
  }
  // Codeable concepts are emitted using the preferred coding systems.
  (*example->mutable_features()->mutable_feature())[name]
      .mutable_bytes_list()
      ->add_value(code);
  if (concept.has_text()) {
    const string full_name = absl::StrCat(name, ".text");
    if (FLAGS_tokenize_code_text_features) {
      tokenize_feature_set->insert(full_name);
    }
    AddValueAndOrTokensToExample(
        *tokenize_feature_set, add_tokenize_feature_set, full_name,
        concept.text().value(), example, enable_attribution);
  }
  for (const auto& coding : concept.coding()) {
    CHECK(coding.has_system() && coding.has_code());
    const string system = systems::ToShortSystemName(coding.system().value());
    (*example->mutable_features()
          ->mutable_feature())[absl::StrCat(name, ".", system)]
        .mutable_bytes_list()
        ->add_value(coding.code().value());
    if (coding.has_display()) {
      const string full_name = absl::StrCat(name, ".", system, ".display");
      if (FLAGS_tokenize_code_text_features) {
        tokenize_feature_set->insert(full_name);
      }
      AddValueAndOrTokensToExample(
          *tokenize_feature_set, add_tokenize_feature_set,
          absl::StrCat(name, ".", system, ".display"), coding.display().value(),
          example, enable_attribution);
    }
  }
}

void MessageToExample(const google::protobuf::Message& message, const string& prefix,
                      ::tensorflow::Example* example, bool enable_attribution) {
  std::set<string> tokenize_feature_set;
  if (!FLAGS_tokenize_feature_list.empty()) {
    tokenize_feature_set =
        absl::StrSplit(FLAGS_tokenize_feature_list, ',', absl::SkipEmpty());
  }
  std::set<string> add_tokenize_feature_set;
  if (!FLAGS_add_tokenize_feature_list.empty()) {
    add_tokenize_feature_set =
        absl::StrSplit(FLAGS_add_tokenize_feature_list, ',', absl::SkipEmpty());
  }

  std::set<string> intersection;
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
    const string name = absl::StrCat(prefix, ".", field->json_name());
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
                       stu3::proto::fhir_valueset_url)) {
          // Valueset-constrained codes are emitted without tokenization.
          const google::protobuf::Reflection* reflection = child.GetReflection();
          const google::protobuf::FieldDescriptor* enum_field =
              field->message_type()->FindFieldByName("value");
          const google::protobuf::EnumValueDescriptor* enum_value =
              reflection->GetEnum(child, enum_field);
          (*example->mutable_features()->mutable_feature())[name]
              .mutable_bytes_list()
              ->add_value(absl::AsciiStrToLower(enum_value->name()));
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
        } else if (field->message_type()->full_name() ==
                   CodeableConcept::descriptor()->full_name()) {
          // Codeable concepts are emitted using the preferred coding systems.
          CodeableConcept concept;
          concept.CopyFrom(child);
          AddCodeableConceptToExample(
              concept, name, example, &tokenize_feature_set,
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
                       Identifier::descriptor()->full_name() ||
                   field->message_type()->full_name() ==
                       Id::descriptor()->full_name()) {
          // We don't emit identifiers.
          // TODO: are there situations where we should?
        } else if (field->message_type()->full_name() ==
                       Reference::descriptor()->full_name() ||
                   field->message_type()->options().ExtensionSize(
                       stu3::proto::fhir_reference_type) > 0 ||
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
