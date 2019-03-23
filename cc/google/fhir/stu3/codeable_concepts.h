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

#ifndef GOOGLE_FHIR_STU3_CODEABLE_CONCEPTS_H_
#define GOOGLE_FHIR_STU3_CODEABLE_CONCEPTS_H_

#include <functional>
#include <memory>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/message.h"
#include "absl/types/optional.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/stu3/annotations.h"
#include "google/fhir/stu3/codeable_concepts.h"
#include "google/fhir/stu3/proto_util.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace stu3 {

// Utility functions for working with potentially profiled CodeableConcepts.
// These use a visitor pattern to allow operations on all Codings within a
// CodeableConcept, regardless of how they are internally stored.

typedef std::function<void(const string& system, const string& code)> CodeFunc;
typedef std::function<bool(const string& system, const string& code)>
    CodeBoolFunc;
typedef std::function<void(const proto::Coding&)> CodingFunc;
typedef std::function<bool(const proto::Coding&)> CodingBoolFunc;

namespace internal {

typedef std::function<bool(const ::google::protobuf::FieldDescriptor*,
                           const proto::CodingWithFixedSystem&)>
    FixedSystemFieldBoolFunc;
typedef std::function<bool(const ::google::protobuf::FieldDescriptor*,
                           const proto::CodingWithFixedCode&)>
    FixedCodeFieldBoolFunc;

// Performs functions on Profiled codings, stoping once any function returns
// true.
// Accepts three functions:
//   1) For raw (unprofiled) codings, a function that operates on Coding.
//   2) A function that operates on CodingWithFixedSystem fields.
//   3) A function that operates on CodingWithFixedCode fields.
// This is internal-only, as outside callers shouldn't care about profiled vs
// unprofiled fields, and should only care about Codings and Codes.
template <typename CodeableConceptLike>
const bool ForEachInternalCodingHalting(
    const CodeableConceptLike& concept, const CodingBoolFunc& coding_func,
    const FixedSystemFieldBoolFunc& fixed_system_func,
    const FixedCodeFieldBoolFunc& fixed_code_func) {
  // Check base Coding fields.
  for (int i = 0; i < concept.coding_size(); i++) {
    const proto::Coding& coding = concept.coding(i);
    if (coding_func(coding)) return true;
  }
  // If there are no profiled fields to check, return.
  if (IsMessageType<proto::CodeableConcept>(concept)) return false;

  // Check for profiled fields.
  const ::google::protobuf::Descriptor* descriptor = concept.GetDescriptor();
  for (int i = 0; i < descriptor->field_count(); i++) {
    const ::google::protobuf::FieldDescriptor* field = descriptor->field(i);
    if (field->type() != ::google::protobuf::FieldDescriptor::TYPE_MESSAGE) continue;

    if (IsMessageType<proto::CodingWithFixedSystem>(field->message_type())) {
      const bool stop = ForEachMessageHalting<proto::CodingWithFixedSystem>(
          &concept, field,
          [&fixed_system_func,
           &field](const proto::CodingWithFixedSystem& fixed_system_coding) {
            return fixed_system_func(field, fixed_system_coding);
          });
      if (stop) return true;
    } else if (IsMessageType<proto::CodingWithFixedCode>(
                   field->message_type())) {
      const bool stop = ForEachMessageHalting<proto::CodingWithFixedCode>(
          &concept, field,
          [&fixed_code_func,
           &field](const proto::CodingWithFixedCode& fixed_code_coding) {
            return fixed_code_func(field, fixed_code_coding);
          });
      if (stop) return true;
    }
  }
  return false;
}

// Gets a profiled field for a given system, or nullptr if none is found.
// This is internal, since outside callers shouldn't care about profiled vs
// unprofiled.
template <typename CodeableConceptLike>
const ::google::protobuf::FieldDescriptor* ProfiledFieldForSystem(
    const CodeableConceptLike& concept, const string& system) {
  if (IsMessageType<proto::CodeableConcept>(concept)) return nullptr;

  const ::google::protobuf::Descriptor* descriptor = concept.GetDescriptor();
  for (int i = 0; i < descriptor->field_count(); i++) {
    const ::google::protobuf::FieldDescriptor* field = descriptor->field(i);
    if (field->options().GetExtension(
            stu3::proto::fhir_inlined_coding_system) == system) {
      return field;
    }
  }
  return nullptr;
}

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
std::unique_ptr<proto::Coding> CodingFromFixedSystemCoding(
    const ::google::protobuf::FieldDescriptor* field,
    const proto::CodingWithFixedSystem fixed_system_coding);

std::unique_ptr<proto::Coding> CodingFromFixedCodeCoding(
    const ::google::protobuf::FieldDescriptor* field,
    const proto::CodingWithFixedCode fixed_code_coding);

}  // namespace internal

// Performs a function on all System/Code pairs, where all codes are treated
// like strings, until the function returns true.
// If the function returns true on a Coding, ceases visiting Codings and returns
// true.  If the function returned false for all Codings, returns false.
template <typename CodeableConceptLike>
const bool FindSystemCodeStringPair(const CodeableConceptLike& concept,
                                    const CodeBoolFunc& func,
                                    const string** found_system,
                                    const string** found_code) {
  return internal::ForEachInternalCodingHalting(
      concept,
      [&func, &found_system, &found_code](const proto::Coding& coding) {
        const string& system = coding.system().value();
        const string& code = coding.code().value();
        if (func(system, code)) {
          *found_system = &system;
          *found_code = &code;
          return true;
        }
        return false;
      },
      [&func, &found_system, &found_code](
          const ::google::protobuf::FieldDescriptor* field,
          const proto::CodingWithFixedSystem& coding) {
        const string& system = GetInlinedCodingSystem(field);
        const string& code = coding.code().value();
        if (func(system, code)) {
          *found_system = &system;
          *found_code = &code;
          return true;
        }
        return false;
      },
      [&func, &found_system, &found_code](
          const ::google::protobuf::FieldDescriptor* field,
          const proto::CodingWithFixedCode& coding) {
        const string& system = GetInlinedCodingSystem(field);
        const string& code = GetInlinedCodingCode(field);
        if (func(system, code)) {
          *found_system = &system;
          *found_code = &code;
          return true;
        }
        return false;
      });
}

// Variant of FindSystemCodeStringPair that does not explicitly set return
// pointers.
// This can be used when the actual result is unused, or when the coding is used
// to set a return pointers in the function closure itself.
template <typename CodeableConceptLike>
const bool FindSystemCodeStringPair(const CodeableConceptLike& concept,
                                    const CodeBoolFunc& func) {
  const string* found_system;
  const string* found_code;
  return FindSystemCodeStringPair(concept, func, &found_system, &found_code);
}

// Performs a function on all System/Code pairs, where all codes are treated
// like strings.  Visits all codings.
template <typename CodeableConceptLike>
void ForEachSystemCodeStringPair(const CodeableConceptLike& concept,
                                 const CodeFunc& func) {
  FindSystemCodeStringPair(
      concept,
      [&func](const string& system, const string& code) {
        func(system, code);
        return false;
      },
      nullptr, nullptr);  // nullptrs for return values, because there will
                          // never be "found" codes, since this function always
                          // returns false.
}

// Performs a function on all Codings, until the function returns true.
// If the function returns true on a Coding, ceases visiting Codings and returns
// a shared_ptr to that coding.  If the function returned false for all Codings,
// returns a null shared_ptr.
// For profiled fields that have no native Coding representation, constructs
// a synthetic Coding to operate on.
// The shared_ptr semantics are used to correctly manage the lifecycle of both
// cases:
//   * For raw codings, the shared_ptr has a no-op deleter, so lifecycle is
//     managed by the CodeableConcept.
//   * For synthetic codings, the returned shared_ptr has the default deleter,
//     so lifecycle is managed by the call site.
// Since the call site has no way of knowing which case it is, the shared_ptr
// semantics accurately convey to the caller that they should never count on
// the Coding living longer than the shared_ptr.
template <typename CodeableConceptLike>
std::shared_ptr<const proto::Coding> FindCoding(
    const CodeableConceptLike& concept, const CodingBoolFunc& func) {
  std::shared_ptr<const proto::Coding> found_coding = nullptr;
  internal::ForEachInternalCodingHalting(
      concept,
      [&func, &found_coding](const proto::Coding& coding) {
        if (func(coding)) {
          // Use a shared_ptr with a no-op Deleter, since the lifecyle of the
          // Coding should be controlled by the CodeableConcept it lives in.
          found_coding = std::shared_ptr<const proto::Coding>(
              &coding, [](const proto::Coding*) {});
          return true;
        }
        return false;
      },
      [&func, &found_coding](const ::google::protobuf::FieldDescriptor* field,
                             const proto::CodingWithFixedSystem& coding) {
        std::shared_ptr<proto::Coding> synth_coding =
            internal::CodingFromFixedSystemCoding(field, coding);
        if (func(*synth_coding)) {
          found_coding = std::move(synth_coding);
          return true;
        }
        return false;
      },
      [&func, &found_coding](const ::google::protobuf::FieldDescriptor* field,
                             const proto::CodingWithFixedCode& coding) {
        std::shared_ptr<proto::Coding> synth_coding =
            internal::CodingFromFixedCodeCoding(field, coding);
        if (func(*synth_coding)) {
          found_coding = std::move(synth_coding);
          return true;
        }
        return false;
      });
  return found_coding;
}

// Performs a function on all Codings.
// For profiled fields that have no native Coding representation, constructs
// a synthetic Coding.
template <typename CodeableConceptLike>
void ForEachCoding(const CodeableConceptLike& concept, const CodingFunc& func) {
  FindCoding(concept, [&func](const proto::Coding& coding) {
    func(coding);
    // Return false for all codings, to ensure this iterates over all codings
    // without "finding" anything.
    return false;
  });
}

// Gets a vector of all codes with a given system, where codes are represented
// as strings.
template <typename CodeableConceptLike>
const std::vector<string> GetCodesWithSystem(
    const CodeableConceptLike& concept, const absl::string_view target_system) {
  std::vector<string> codes;
  ForEachSystemCodeStringPair(
      concept,
      [&codes, &target_system](const string& system, const string& code) {
        if (system == target_system) {
          codes.push_back(code);
        }
      });
  return codes;
}

// Gets the only code with a given system.
// If no code is found with that system, returns a NotFound status.
// If more than one code is found with that system, returns AlreadyExists
// status.
template <typename CodeableConceptLike>
StatusOr<const string> GetOnlyCodeWithSystem(const CodeableConceptLike& concept,
                                             const absl::string_view system) {
  const std::vector<string>& codes = GetCodesWithSystem(concept, system);
  if (codes.size() == 0) {
    return tensorflow::errors::NotFound("No code from system: ", system);
  }
  if (codes.size() > 1) {
    return ::tensorflow::errors::AlreadyExists("Found more than one code");
  }
  return codes.front();
}

// Adds a Coding to a CodeableConcept.
// TODO: This will have undefined behavior if there is a
// CodingWithFixedCode and CodingWithFixedSystem field with the same system.
// Think about this a bit more.  It might just be illegal since a code might
// fit into two different slices, but might be useful, e.g., if you want
// to specify a required ICD9 code, but make it easy to add other ICD9 codes.
template <typename CodeableConceptLike>
Status AddCoding(CodeableConceptLike* concept, const proto::Coding& coding) {
  const string& system = coding.system().value();
  if (IsProfileOf<proto::CodeableConcept>(*concept)) {
    const ::google::protobuf::FieldDescriptor* profiled_field =
        internal::ProfiledFieldForSystem(*concept, system);
    if (profiled_field != nullptr) {
      if (IsMessageType<proto::CodingWithFixedSystem>(
              profiled_field->message_type())) {
        if (!profiled_field->is_repeated() &&
            FieldHasValue(*concept, profiled_field)) {
          return ::tensorflow::errors::AlreadyExists(
              "Attempted to add a System to a non-repeated slice that is "
              "already populated.  Field: ",
              profiled_field->full_name(), ", System: ", system);
        }
        ::google::protobuf::Message* target_coding =
            MutableOrAddMessage(concept, profiled_field);
        proto::CodingWithFixedSystem* fixed_system_coding =
            static_cast<proto::CodingWithFixedSystem*>(target_coding);
        internal::CopyCommonCodingFields(coding, fixed_system_coding);
        *fixed_system_coding->mutable_code() = coding.code();
        return Status::OK();
      } else if (IsMessageType<proto::CodingWithFixedCode>(
                     profiled_field->message_type())) {
        const string& fixed_code = profiled_field->options().GetExtension(
            stu3::proto::fhir_inlined_coding_code);
        if (fixed_code == coding.code().value()) {
          if (!profiled_field->is_repeated() &&
              FieldHasValue(*concept, profiled_field)) {
            return ::tensorflow::errors::AlreadyExists(
                "Attempted to add a Code-System Pair to a non-repeated slice "
                "that is already populated.  Field: ",
                profiled_field->full_name(), ", System: ", system,
                ", Code:", fixed_code);
          }
          ::google::protobuf::Message* target_coding =
              MutableOrAddMessage(concept, profiled_field);
          proto::CodingWithFixedCode* fixed_system_code =
              static_cast<proto::CodingWithFixedCode*>(target_coding);
          internal::CopyCommonCodingFields(coding, fixed_system_code);
          return Status::OK();
        }
      }
    }
  }
  *concept->add_coding() = coding;
  return Status::OK();
}

template <typename CodeableConceptLike>
Status AddCoding(CodeableConceptLike* concept, const string& system,
                 const string& code) {
  proto::Coding coding;
  coding.mutable_system()->set_value(system);
  coding.mutable_code()->set_value(code);
  return AddCoding(concept, coding);
}

template <typename CodeableConceptLike>
Status ClearAllCodingsWithSystem(CodeableConceptLike* concept,
                                 const string& system) {
  if (IsProfileOf<proto::CodeableConcept>(*concept)) {
    const ::google::protobuf::FieldDescriptor* profiled_field =
        internal::ProfiledFieldForSystem(*concept, system);
    if (profiled_field != nullptr) {
      if (IsMessageType<proto::CodingWithFixedSystem>(
              profiled_field->message_type())) {
        concept->GetReflection()->ClearField(concept, profiled_field);
      } else if (IsMessageType<proto::CodingWithFixedCode>(
                     profiled_field->message_type())) {
        return ::tensorflow::errors::InvalidArgument(
            "Cannot clear coding system: ", system, " from ",
            concept->GetDescriptor()->full_name(),
            ". It is a fixed code on that profile");
      }
    }
  }
  for (auto iter = concept->mutable_coding()->begin();
       iter != concept->mutable_coding()->end();) {
    if (iter->has_system() && iter->system().value() == system) {
      iter = concept->mutable_coding()->erase(iter);
    } else {
      iter++;
    }
  }
  return Status::OK();
}

template <typename CodeableConceptLikeSource,
          typename CodeableConceptLikeTarget>
void CopyCodeableConcept(const CodeableConceptLikeSource& source,
                         CodeableConceptLikeTarget* target) {
  if (IsMessageType<CodeableConceptLikeSource>(*target)) {
    target->CopyFrom(source);
    return;
  }
  ForEachCoding(source, [&target](const proto::Coding& coding) {
    AddCoding(target, coding);
  });
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_CODEABLE_CONCEPTS_H_
