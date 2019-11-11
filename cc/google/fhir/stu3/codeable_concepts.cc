// Copyright 2019 Google LLC
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

#include "google/fhir/stu3/codeable_concepts.h"

#include <string>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "absl/memory/memory.h"
#include "absl/strings/match.h"
#include "google/fhir/annotations.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/statusor.h"
#include "proto/annotations.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace stu3 {

namespace {

using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::tensorflow::errors::AlreadyExists;
using ::tensorflow::errors::InvalidArgument;
using ::tensorflow::errors::NotFound;

// Utility functions for working with potentially profiled CodeableConcepts.
// These use a visitor pattern to allow operations on all Codings within a
// CodeableConcept, regardless of how they are internally stored.

typedef std::function<bool(const FieldDescriptor*,
                           const stu3::proto::CodingWithFixedSystem&)>
    FixedSystemFieldBoolFunc;
typedef std::function<bool(const FieldDescriptor*,
                           const stu3::proto::CodingWithFixedCode&)>
    FixedCodeFieldBoolFunc;

template <typename SrcCodingLike, typename DestCodingLike>
void CopyCommonCodingFields(const SrcCodingLike& src, DestCodingLike* dest) {
  dest->mutable_extension()->CopyFrom(src.extension());
  if (src.has_id()) *dest->mutable_id() = src.id();
  if (src.has_display()) *dest->mutable_display() = src.display();
  if (src.has_version()) *dest->mutable_version() = src.version();
  if (src.has_user_selected()) {
    *dest->mutable_user_selected() = src.user_selected();
  }
}

// These use unique_ptr so that they can be heap-allocated.  This allows
// callers to work with references and pointers to codings without caring
// if the underlying objects are synthetic, or owned by the CodeableConcept
// proto.
std::unique_ptr<stu3::proto::Coding> CodingFromFixedSystemCoding(
    const google::protobuf::FieldDescriptor* field,
    const stu3::proto::CodingWithFixedSystem& fixed_system_coding) {
  std::unique_ptr<stu3::proto::Coding> coding =
      absl::make_unique<stu3::proto::Coding>();
  CopyCommonCodingFields(fixed_system_coding, coding.get());
  coding->mutable_system()->set_value(GetInlinedCodingSystem(field));
  *coding->mutable_code() = fixed_system_coding.code();
  return coding;
}

std::unique_ptr<stu3::proto::Coding> CodingFromFixedCodeCoding(
    const google::protobuf::FieldDescriptor* field,
    const stu3::proto::CodingWithFixedCode& fixed_code_coding) {
  std::unique_ptr<stu3::proto::Coding> coding =
      absl::make_unique<stu3::proto::Coding>();
  CopyCommonCodingFields(fixed_code_coding, coding.get());
  coding->mutable_system()->set_value(GetInlinedCodingSystem(field));
  coding->mutable_code()->set_value(GetInlinedCodingCode(field));
  return coding;
}

// Performs functions on Profiled codings, stoping once any function returns
// true.
// Accepts three functions:
//   1) For raw (unprofiled) codings, a function that operates on Coding.
//   2) A function that operates on CodingWithFixedSystem fields.
//   3) A function that operates on CodingWithFixedCode fields.
// This is internal-only, as outside callers shouldn't care about profiled vs
// unprofiled fields, and should only care about Codings and Codes.
const bool ForEachInternalCodingHalting(
    const Message& concept, const CodingBoolFunc& coding_func,
    const FixedSystemFieldBoolFunc& fixed_system_func,
    const FixedCodeFieldBoolFunc& fixed_code_func) {
  // Check base Coding fields.
  const FieldDescriptor* coding_field =
      concept.GetDescriptor()->FindFieldByName("coding");
  if (ForEachMessageHalting<stu3::proto::Coding>(concept, coding_field,
                                                 coding_func)) {
    return true;
  }

  // If there are no profiled fields to check, return.
  if (IsMessageType<stu3::proto::CodeableConcept>(concept)) return false;

  // Check for profiled fields.
  const ::google::protobuf::Descriptor* descriptor = concept.GetDescriptor();
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    if (field->type() != FieldDescriptor::TYPE_MESSAGE) continue;

    if (IsMessageType<stu3::proto::CodingWithFixedSystem>(
            field->message_type())) {
      const bool stop =
          ForEachMessageHalting<stu3::proto::CodingWithFixedSystem>(
              concept, field,
              [&fixed_system_func,
               &field](const stu3::proto::CodingWithFixedSystem&
                           fixed_system_coding) {
                return fixed_system_func(field, fixed_system_coding);
              });
      if (stop) return true;
    } else if (IsMessageType<stu3::proto::CodingWithFixedCode>(
                   field->message_type())) {
      const bool stop = ForEachMessageHalting<stu3::proto::CodingWithFixedCode>(
          concept, field,
          [&fixed_code_func,
           &field](const stu3::proto::CodingWithFixedCode& fixed_code_coding) {
            return fixed_code_func(field, fixed_code_coding);
          });
      if (stop) return true;
    }
  }
  return false;
}

// Copies a field from source to target if it is present on source.
// Fails if the field is present and populated on the source, but does not have
// an identical field on the target.
// Notably, this will *not* fail if the field is present on the target but not
// the source, or present on the source but not the target but the field is not
// set, since no information will be lost in these cases.
Status CopyFieldIfPresent(const Message& source, Message* target,
                          const std::string& field_name) {
  const FieldDescriptor* field =
      source.GetDescriptor()->FindFieldByName(field_name);
  if (!field && FieldHasValue(source, field)) {
    return Status::OK();
  }
  return CopyCommonField(source, target, field_name);
}

}  // namespace

namespace internal {

// Gets a profiled field for a given system, or nullptr if none is found.
// This is internal, since outside callers shouldn't care about profiled vs
// unprofiled.
const FieldDescriptor* ProfiledFieldForSystem(const Message& concept,
                                              const std::string& system) {
  if (IsMessageType<stu3::proto::CodeableConcept>(concept)) return nullptr;

  const ::google::protobuf::Descriptor* descriptor = concept.GetDescriptor();
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    if (GetInlinedCodingSystem(field) == system ||
        GetFixedCodingSystem(field->message_type()) == system) {
      return field;
    }
  }
  return nullptr;
}

}  // namespace internal

const bool FindSystemCodeStringPair(const Message& concept,
                                    const CodeBoolFunc& func,
                                    std::string* found_system,
                                    std::string* found_code) {
  return FindSystemCodeStringPair(
      concept, [&func, &found_system, &found_code](const std::string& system,
                                                   const std::string& code) {
        if (func(system, code)) {
          *found_system = system;
          *found_code = code;
          return true;
        }
        return false;
      });
}

const bool FindSystemCodeStringPair(const Message& concept,
                                    const CodeBoolFunc& func) {
  return ForEachInternalCodingHalting(
      concept,
      [&func](const stu3::proto::Coding& coding) {
        const std::string& system = coding.system().value();
        const std::string& code = coding.code().value();
        if (func(system, code)) {
          return true;
        }
        return false;
      },
      [&func](const FieldDescriptor* field,
              const stu3::proto::CodingWithFixedSystem& coding) {
        const std::string& system = GetInlinedCodingSystem(field);
        const std::string& code = coding.code().value();
        if (func(system, code)) {
          return true;
        }
        return false;
      },
      [&func](const FieldDescriptor* field,
              const stu3::proto::CodingWithFixedCode& coding) {
        const std::string& system = GetInlinedCodingSystem(field);
        const std::string& code = GetInlinedCodingCode(field);
        if (func(system, code)) {
          return true;
        }
        return false;
      });
}

void ForEachSystemCodeStringPair(const Message& concept, const CodeFunc& func) {
  FindSystemCodeStringPair(
      concept, [&func](const std::string& system, const std::string& code) {
        func(system, code);
        return false;
      });
}

const std::vector<std::string> GetCodesWithSystem(
    const Message& concept, const absl::string_view target_system) {
  std::vector<std::string> codes;
  ForEachSystemCodeStringPair(
      concept, [&codes, &target_system](const std::string& system,
                                        const std::string& code) {
        if (system == target_system) {
          codes.push_back(code);
        }
      });
  return codes;
}

StatusOr<const std::string> GetOnlyCodeWithSystem(
    const Message& concept, const absl::string_view system) {
  const std::vector<std::string>& codes = GetCodesWithSystem(concept, system);
  if (codes.empty()) {
    return tensorflow::errors::NotFound("No code from system: ", system);
  }
  if (codes.size() > 1) {
    return ::tensorflow::errors::AlreadyExists("Found more than one code");
  }
  return codes.front();
}

Status AddCoding(Message* concept, const stu3::proto::Coding& coding) {
  if (!IsTypeOrProfileOf<stu3::proto::CodeableConcept>(*concept)) {
    return InvalidArgument(
        "Error adding coding: ", concept->GetDescriptor()->full_name(),
        " is not CodeableConcept-like.");
  }
  const std::string& system = coding.system().value();
  if (IsProfileOf<stu3::proto::CodeableConcept>(*concept)) {
    const FieldDescriptor* profiled_field =
        internal::ProfiledFieldForSystem(*concept, system);
    if (profiled_field != nullptr) {
      if (IsMessageType<stu3::proto::CodingWithFixedSystem>(
              profiled_field->message_type())) {
        if (!profiled_field->is_repeated() &&
            FieldHasValue(*concept, profiled_field)) {
          return ::tensorflow::errors::AlreadyExists(
              "Attempted to add a System to a non-repeated slice that is "
              "already populated.  Field: ",
              profiled_field->full_name(), ", System: ", system);
        }
        Message* target_coding = MutableOrAddMessage(concept, profiled_field);
        stu3::proto::CodingWithFixedSystem* fixed_system_coding =
            static_cast<stu3::proto::CodingWithFixedSystem*>(target_coding);
        CopyCommonCodingFields(coding, fixed_system_coding);
        *fixed_system_coding->mutable_code() = coding.code();
        return Status::OK();
      } else if (IsMessageType<stu3::proto::CodingWithFixedCode>(
                     profiled_field->message_type())) {
        const std::string& fixed_code = GetInlinedCodingCode(profiled_field);
        if (fixed_code == coding.code().value()) {
          if (!profiled_field->is_repeated() &&
              FieldHasValue(*concept, profiled_field)) {
            return ::tensorflow::errors::AlreadyExists(
                "Attempted to add a Code-System Pair to a non-repeated slice "
                "that is already populated.  Field: ",
                profiled_field->full_name(), ", System: ", system,
                ", Code:", fixed_code);
          }
          Message* target_coding = MutableOrAddMessage(concept, profiled_field);
          stu3::proto::CodingWithFixedCode* fixed_system_code =
              static_cast<stu3::proto::CodingWithFixedCode*>(target_coding);
          CopyCommonCodingFields(coding, fixed_system_code);
          return Status::OK();
        }
      }
    }
  }
  concept->GetReflection()
      ->AddMessage(concept, concept->GetDescriptor()->FindFieldByName("coding"))
      ->CopyFrom(coding);
  return Status::OK();
}

Status AddCoding(Message* concept, const std::string& system,
                 const std::string& code) {
  const google::protobuf::FieldDescriptor* field_descriptor =
      concept->GetDescriptor()->FindFieldByName("coding");
  if (field_descriptor == nullptr ||
      field_descriptor->type() != google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
    return InvalidArgument(
        "Error adding coding: ", concept->GetDescriptor()->full_name(),
        " is not CodeableConcept-like.");
  }
  const google::protobuf::Descriptor* descriptor =
      field_descriptor
          ->message_type();  // Ensure there is a a valid STU3 Coding field.
  if (descriptor->full_name() ==
      stu3::proto::Coding::descriptor()->full_name()) {
    stu3::proto::Coding coding;
    coding.mutable_system()->set_value(system);
    coding.mutable_code()->set_value(code);
    return AddCoding(concept, coding);
  } else if (GetFhirVersion(descriptor) ==
             ::google::fhir::proto::FhirVersion::R4) {
    return InvalidArgument(
        "Invoked STU3 version of AddCoding with an R4 coding");
  } else {
    return InvalidArgument("Error adding coding: ", descriptor->full_name(),
                           " is not a supported Coding type.");
  }
}

std::shared_ptr<const stu3::proto::Coding> FindCoding(
    const Message& concept, const CodingBoolFunc& func) {
  std::shared_ptr<const stu3::proto::Coding> found_coding = nullptr;
  ForEachInternalCodingHalting(
      concept,
      [&func, &found_coding](const stu3::proto::Coding& coding) {
        if (func(coding)) {
          // Use a shared_ptr with a no-op Deleter, since the lifecyle of the
          // Coding should be controlled by the CodeableConcept it lives in.
          found_coding = std::shared_ptr<const stu3::proto::Coding>(
              &coding, [](const stu3::proto::Coding*) {});
          return true;
        }
        return false;
      },
      [&func, &found_coding](const FieldDescriptor* field,
                             const stu3::proto::CodingWithFixedSystem& coding) {
        std::shared_ptr<stu3::proto::Coding> synth_coding =
            CodingFromFixedSystemCoding(field, coding);
        if (func(*synth_coding)) {
          found_coding = std::move(synth_coding);
          return true;
        }
        return false;
      },
      [&func, &found_coding](const FieldDescriptor* field,
                             const stu3::proto::CodingWithFixedCode& coding) {
        std::shared_ptr<stu3::proto::Coding> synth_coding =
            CodingFromFixedCodeCoding(field, coding);
        if (func(*synth_coding)) {
          found_coding = std::move(synth_coding);
          return true;
        }
        return false;
      });
  return found_coding;
}

void ForEachCoding(const Message& concept, const CodingFunc& func) {
  FindCoding(concept, [&func](const stu3::proto::Coding& coding) {
    func(coding);
    // Return false for all codings, to ensure this iterates over all codings
    // without "finding" anything.
    return false;
  });
}

Status ForEachCodingWithStatus(const Message& concept,
                               const CodingStatusFunc& func) {
  Status return_status = Status::OK();
  FindCoding(concept,
             [&func, &return_status](const stu3::proto::Coding& coding) {
               Status status = func(coding);
               if (status.ok()) {
                 return false;
               }
               return_status = status;
               return true;
             });
  return return_status;
}

Status CopyCodeableConcept(const Message& source, Message* target) {
  // Copy common fields.
  // These will fail if the field is present & populated on the source,
  // but does not have an identical field on the target.
  FHIR_RETURN_IF_ERROR(CopyFieldIfPresent(source, target, "id"));
  FHIR_RETURN_IF_ERROR(CopyFieldIfPresent(source, target, "text"));
  FHIR_RETURN_IF_ERROR(CopyFieldIfPresent(source, target, "extension"));

  // Copy all codings.
  return ForEachCodingWithStatus(source,
                                 [&target](const stu3::proto::Coding& coding) {
                                   return AddCoding(target, coding);
                                 });
}

bool IsCodeableConceptLike(const ::google::protobuf::Descriptor* descriptor) {
  return IsTypeOrProfileOf<stu3::proto::CodeableConcept>(descriptor);
}

bool IsCodeableConceptLike(const Message& message) {
  return IsCodeableConceptLike(message.GetDescriptor());
}

int CodingSize(const ::google::protobuf::Message& concept) {
  int size = 0;
  ForEachSystemCodeStringPair(
      concept,
      [&size](const std::string& system, const std::string& code) { size++; });
  return size;
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
