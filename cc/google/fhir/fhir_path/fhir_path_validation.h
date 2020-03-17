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

#ifndef GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_VALIDATION_H_
#define GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_VALIDATION_H_

#include <unordered_map>

#include "google/protobuf/message.h"
#include "absl/synchronization/mutex.h"
#include "google/fhir/annotations.h"
#include "google/fhir/fhir_path/fhir_path.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/status/statusor.h"

namespace google {
namespace fhir {
namespace fhir_path {

// Handler callback function invoked for each FHIRPath constraint violation.
// Users may accumulate these or write them to an appropriate
// reporting mechanism.
//
// Users should return true to halt validation of the resource,
// or false to continue the evaluation.
//
// The handler is provided with:
// * The message on which the FHIRPath violation occurred. This
//   may be the root message or a child structure.
// * The field in the above message that caused the violation, or
//   nullptr if the error occured on the root of the message.
//   This is the field on which the FHIRPath constraint is defined.
// * The FHIRPath constraint expression that triggered the violation.
typedef std::function<bool(const ::google::protobuf::Message& message,
                           const ::google::protobuf::FieldDescriptor* field,
                           const std::string& constraint)>
    ViolationHandlerFunc;

// This class validates that all fhir_path_constraint annotations on
// the given messages are valid. It will compile and cache the
// constraint expressions as it encounters them, so users are encouraged
// to create a single instance of this for the lifetime of the process.
// This class is thread safe.
class MessageValidator {
 public:
  MessageValidator();
  ~MessageValidator();

  // Validates the fhir_path_constraint annotations on the given message.
  // Returns Status::OK or the status of the first constraint violation
  // encountered. Users needing more details should use the overloaded
  // version of with a callback handler for each violation.
  Status Validate(const ::google::protobuf::Message& message);

  // Validates the fhir_path_constraint annotations on the given message.
  // Returns Status::OK or the status of the first constraint failure
  // encountered. Details on all failures are passed to the given handler
  // function.
  Status Validate(const ::google::protobuf::Message& message,
                  ViolationHandlerFunc handler);

 private:
  // A cache of constraints for a given message definition
  struct MessageConstraints {
    // FHIRPath constraints at the "root" FHIR element, which is just the
    // protobuf message.
    std::vector<CompiledExpression> message_expressions;

    // FHIRPath constraints on fields
    std::vector<
        std::pair<const ::google::protobuf::FieldDescriptor*, const CompiledExpression>>
        field_expressions;

    // Nested messages that have constraints, so the evaluation logic
    // knows to check them.
    std::vector<const ::google::protobuf::FieldDescriptor*> nested_with_constraints;
  };

  // Loads constraints for the given descriptor.
  StatusOr<MessageConstraints*> ConstraintsFor(
      const ::google::protobuf::Descriptor* descriptor,
      const PrimitiveHandler* primitive_handler);

  // Recursively called validation method that can terminate
  // validation based on the callback.
  Status Validate(const internal::WorkspaceMessage& message,
                  const PrimitiveHandler* primitive_handler,
                  ViolationHandlerFunc handler, bool* halt_validation);

  absl::Mutex mutex_;
  std::unordered_map<std::string, std::unique_ptr<MessageConstraints>>
      constraints_cache_;
};

// Validates the fhir_path_constraint annotations on the given message.
// Returns Status::OK or the status of the first constraint violation
// encountered. Users needing more details should use the overloaded
// version of with a callback handler for each violation.
Status ValidateMessage(const ::google::protobuf::Message& message);

// Validates the fhir_path_constraint annotations on the given message.
// Returns Status::OK or the status of the first constraint failure
// encountered. Details on all failures are passed to the given handler
// function.
Status ValidateMessage(const ::google::protobuf::Message& message,
                       ViolationHandlerFunc handler);

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_VALIDATION_H_
