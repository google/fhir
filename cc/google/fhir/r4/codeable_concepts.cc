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

#include "google/fhir/r4/codeable_concepts.h"

#include <string>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "absl/memory/memory.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codes.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/statusor.h"
#include "proto/r4/core/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace r4 {

namespace {

using ::google::fhir::r4::core::Coding;
using ::google::fhir::r4::core::CodingWithFixedCode;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::tensorflow::errors::AlreadyExists;
using ::tensorflow::errors::InvalidArgument;
using ::tensorflow::errors::NotFound;

// Utility functions for working with potentially profiled CodeableConcepts.
// These use a visitor pattern to allow operations on all Codings within a
// CodeableConcept, regardless of how they are internally stored.
typedef std::function<bool(const Message&)> FixedSystemFieldBoolFunc;
typedef std::function<bool(const FieldDescriptor*, const CodingWithFixedCode&)>
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

std::unique_ptr<Coding> CodingFromFixedCodeCoding(
    const google::protobuf::FieldDescriptor* field,
    const CodingWithFixedCode& fixed_code_coding) {
  std::unique_ptr<Coding> coding = absl::make_unique<Coding>();
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
  if (ForEachMessageHalting<Coding>(concept, coding_field, coding_func)) {
    return true;
  }

  // If there are no profiled fields to check, return.
  if (IsMessageType<r4::core::CodeableConcept>(concept)) return false;

  // Check for profiled fields.
  const ::google::protobuf::Descriptor* descriptor = concept.GetDescriptor();
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    if (field->type() != FieldDescriptor::TYPE_MESSAGE) continue;

    if (IsMessageType<CodingWithFixedCode>(field->message_type())) {
      const bool stop = ForEachMessageHalting<CodingWithFixedCode>(
          concept, field,
          [&fixed_code_func,
           &field](const CodingWithFixedCode& fixed_code_coding) {
            return fixed_code_func(field, fixed_code_coding);
          });
      if (stop) return true;
    } else if (IsProfileOf<Coding>(field->message_type())) {
      const bool stop = ForEachMessageHalting<Message>(
          concept, field,
          [&fixed_system_func](const Message& fixed_system_coding) {
            return fixed_system_func(fixed_system_coding);
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

bool HasCodeBoundToSystem(const Descriptor* coding_descriptor,
                          const std::string& system) {
  const FieldDescriptor* code_field =
      coding_descriptor->FindFieldByName("code");
  return code_field && GetFixedSystem(code_field->message_type()) == system;
}

}  // namespace

namespace internal {

// Gets a profiled field for a given system, or nullptr if none is found.
// This is internal, since outside callers shouldn't care about profiled vs
// unprofiled.
const FieldDescriptor* ProfiledFieldForSystem(const Message& concept,
                                              const std::string& system) {
  if (IsMessageType<r4::core::CodeableConcept>(concept)) return nullptr;

  const ::google::protobuf::Descriptor* descriptor = concept.GetDescriptor();
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    if (IsProfileOf<Coding>(field->message_type()) &&
        (GetInlinedCodingSystem(field) == system ||
         HasCodeBoundToSystem(field->message_type(), system))) {
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
      [&func](const Coding& coding) {
        const std::string& system = coding.system().value();
        const std::string& code = coding.code().value();
        if (func(system, code)) {
          return true;
        }
        return false;
      },
      [&func](const Message& fixed_system_coding) {
        Coding generic_coding;
        auto status =
            ConvertToGenericCoding(fixed_system_coding, &generic_coding);
        if (!status.ok()) {
          LOG(WARNING) << "Encountered malformed Coding with fixed system "
                       << fixed_system_coding.GetDescriptor()->full_name()
                       << ": " << status.error_message();
          return false;
        }
        const std::string& system = generic_coding.system().value();
        const std::string& code = generic_coding.code().value();
        if (func(system, code)) {
          return true;
        }
        return false;
      },
      [&func](const FieldDescriptor* field, const CodingWithFixedCode& coding) {
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

Status AddCoding(Message* concept, const Coding& coding) {
  if (!IsTypeOrProfileOf<r4::core::CodeableConcept>(*concept)) {
    return InvalidArgument(
        "Error adding coding: ", concept->GetDescriptor()->full_name(),
        " is not CodeableConcept-like.");
  }
  const std::string& system = coding.system().value();
  if (IsProfileOf<r4::core::CodeableConcept>(*concept)) {
    const FieldDescriptor* profiled_field =
        internal::ProfiledFieldForSystem(*concept, system);
    if (profiled_field != nullptr) {
      if (IsMessageType<CodingWithFixedCode>(profiled_field->message_type())) {
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
          CodingWithFixedCode* fixed_system_code =
              static_cast<CodingWithFixedCode*>(target_coding);
          CopyCommonCodingFields(coding, fixed_system_code);
          return Status::OK();
        }
      } else if (IsProfileOf<Coding>(profiled_field->message_type())) {
        if (!profiled_field->is_repeated() &&
            FieldHasValue(*concept, profiled_field)) {
          return ::tensorflow::errors::AlreadyExists(
              "Attempted to add a System to a non-repeated slice that is "
              "already populated.  Field: ",
              profiled_field->full_name(), ", System: ", system);
        }
        return ConvertToTypedCoding(
            coding, MutableOrAddMessage(concept, profiled_field));
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
  const google::protobuf::Descriptor* descriptor = field_descriptor->message_type();
  if (descriptor->full_name() == Coding::descriptor()->full_name()) {
    Coding coding;
    coding.mutable_system()->set_value(system);
    coding.mutable_code()->set_value(code);
    return AddCoding(concept, coding);
  } else if (GetFhirVersion(descriptor) ==
             ::google::fhir::proto::FhirVersion::STU3) {
    return InvalidArgument(
        "Invoked R4 version of AddCoding with an STU3 coding");
  } else {
    return InvalidArgument("Error adding coding: ", descriptor->full_name(),
                           " is not a supported Coding type.");
  }
}

std::shared_ptr<const Coding> FindCoding(const Message& concept,
                                         const CodingBoolFunc& func) {
  std::shared_ptr<const Coding> found_coding = nullptr;
  ForEachInternalCodingHalting(
      concept,
      [&func, &found_coding](const Coding& coding) {
        if (func(coding)) {
          // Use a shared_ptr with a no-op Deleter, since the lifecyle of the
          // Coding should be controlled by the CodeableConcept it lives in.
          found_coding =
              std::shared_ptr<const Coding>(&coding, [](const Coding*) {});
          return true;
        }
        return false;
      },
      [&func, &found_coding](const Message& fixed_system_coding) {
        std::unique_ptr<Coding> synth_coding = absl::make_unique<Coding>();
        auto status =
            ConvertToGenericCoding(fixed_system_coding, synth_coding.get());
        if (!status.ok()) {
          LOG(WARNING) << "Encountered malformed Coding with fixed system "
                       << fixed_system_coding.GetDescriptor()->full_name()
                       << ": " << status.error_message();
          return false;
        }
        if (func(*synth_coding)) {
          found_coding = std::move(synth_coding);
          return true;
        }
        return false;
      },
      [&func, &found_coding](const FieldDescriptor* field,
                             const CodingWithFixedCode& coding) {
        std::shared_ptr<Coding> synth_coding =
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
  FindCoding(concept, [&func](const Coding& coding) {
    func(coding);
    // Return false for all codings, to ensure this iterates over all codings
    // without "finding" anything.
    return false;
  });
}

Status ForEachCodingWithStatus(const Message& concept,
                               const CodingStatusFunc& func) {
  Status return_status = Status::OK();
  FindCoding(concept, [&func, &return_status](const Coding& coding) {
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
  return ForEachCodingWithStatus(source, [&target](const Coding& coding) {
    return AddCoding(target, coding);
  });
}

bool IsCodeableConceptLike(const ::google::protobuf::Descriptor* descriptor) {
  return IsTypeOrProfileOf<r4::core::CodeableConcept>(descriptor);
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

}  // namespace r4
}  // namespace fhir
}  // namespace google
