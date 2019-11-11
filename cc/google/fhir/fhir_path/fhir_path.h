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

#ifndef GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_H_
#define GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_H_

#include <unordered_map>

#include "google/protobuf/message.h"
#include "absl/synchronization/mutex.h"
#include "google/fhir/status/statusor.h"

namespace google {
namespace fhir {
namespace fhir_path {


namespace internal {

// Represents working memory needed to evaluate the expression aginst
// a given message. All temporary structures are destroyed when
// the workspace goes out of scope.
class WorkSpace {
 public:
  // Creates the workspace with the given message context against
  // which the FHIRPath expression is evaluated. The workspace contains
  // temporary data used for a single evaluation, e.g. a call to
  // ExpressionNode::Evaluate. It contains the context message
  // (generally the resource against which the expression is run),
  // the accumulated results, and tracks temporary data to be deleted
  // when the evaluation result is destroyed.
  explicit WorkSpace(const ::google::protobuf::Message* message_context)
      : message_context_(message_context) {}

  // Gets the message context the FHIRPath expression is evaluated against.
  const ::google::protobuf::Message* MessageContext() { return message_context_; }

  // Sets the results to be returned to the caller.
  void SetResultMessages(std::vector<const ::google::protobuf::Message*> messages) {
    messages_ = messages;
  }

  // Gets the results to return to the caller.
  const std::vector<const ::google::protobuf::Message*>& GetResultMessages() {
    return messages_;
  }

  // Mark the message to be deleted when the workspace goes out of scope.
  // This is necessary because some messages are created on the fly,
  // while others simply return nested messages in the user-provided
  // protocol buffers, so we need to explicitly track which we need
  // to delete.
  void DeleteWhenFinished(::google::protobuf::Message* message) {
    to_delete_.push_back(std::unique_ptr<::google::protobuf::Message>(message));
  }

 private:
  std::vector<const ::google::protobuf::Message*> messages_;

  const ::google::protobuf::Message* message_context_;

  std::vector<std::unique_ptr<::google::protobuf::Message>> to_delete_;
};

// Abstract base class of "compiled" FHIRPath expressions. In this
// context, a "compiled" expression consists of ExpressionNode objects,
// each of which implements the logic for the corresponding FHIRPath
// sub-expression. We use this term to differentiate it from the
// expression string itself.
class ExpressionNode {
 public:
  virtual ~ExpressionNode() {}

  // Evaluate the FHIRPath expression. If successful, the implementation
  // should return Status::OK and place the resulting values in
  // the results vector.
  //
  // Note that the results vector itself may contain other messages
  // from other evalutations of this same node since any point in the tree
  // can be multi-valued. So evaluating node foo->bar->baz would
  // see distinct invocations if there are multiple values of bar,
  // but FHIRPath dictates all of these land in the same results collection.
  //
  // The work_space parameter is used to access information about the
  // root message and any temporary data needed for this specific evaluation,
  // so a new work_space is provided on each call.
  virtual Status Evaluate(
      WorkSpace* work_space,
      std::vector<const ::google::protobuf::Message*>* results) const = 0;

  // The descriptor of the message type returned by the expression.
  virtual const ::google::protobuf::Descriptor* ReturnType() const = 0;
};

}  // namespace internal

// The result of a successful evaluation of a CompiledExpression,
// defined below.
//
// FHIRPath expressions always return a collection of objects, by definition.
// Those objects may be a set of data elements or primitives. Even simple
// boolean FHIRPath expressions produce a collectiong with a single,
// boolean value.
//
// For instance, a FHIRPath expression of "address.city" evaluated on
// a Patient record will return a collection of all cities that person has.
// In contrast, an expression of "address.exists()" returns a single boolean
// value indicating at least one address exists for that person.
//
// The EvaluationResult class models this by representing the result of
// all expressions as a collection of Protocol Buffer Messages, where primitives
// are represented with message wrappers of primitive types. This class
// also offers the GetBoolean() method as a convenient way to handle the
// frequent case where expression evaluates to a single booolean.
//
// Depending on the FHIRPath expression, the result could either be children
// of the original Message, or temporary objects. The EvaluationResult
// itself maintains ownership of those objects and will clean them up
// when it goes out of scope. See the AsMessages() method for deails.
//
// This class is immutable and thread safe as long as the Message used
// in the evaluation is in scope and unmodified.
class EvaluationResult {
 public:
  EvaluationResult(EvaluationResult&& result);

  EvaluationResult& operator=(EvaluationResult&& result);

  ~EvaluationResult();

  // Returns the results of the evaluation in message form.
  //
  // Depending on the expression, these messages may be children
  // of the original Message against which the evaluation was performed,
  // or temporary messages produced by the evaluation that will be
  // deleted when the EvaluationResult goes out of scope.
  const std::vector<const ::google::protobuf::Message*>& GetMessages() const;

  // Returns success with a boolean value if the EvaluationResult represents
  // a boolean value per FHIRPath. That is, if it has a single message
  // that contains a boolean. A failure status is returned if the expression
  // did not resolve to a boolean value.
  StatusOr<bool> GetBoolean() const;

  // Returns success with an integer value if the EvaluationResult represents
  // a integer value per FHIRPath. That is, if it has a single message
  // that contains an integer. A failure status is returned if the expression
  // did not resolve to an integer value.
  StatusOr<int32_t> GetInteger() const;

  // Returns success with a decimal value if the EvaluationResult represents
  // a decimal value per FHIRPath. That is, if it has a single message
  // that contains a decimal. A failure status is returned if the expression
  // did not resolve to a decimal value.
  StatusOr<std::string> GetDecimal() const;

  // Returns success with a string value if the EvaluationResult represents
  // a string value per FHIRPath. That is, if it has a single message
  // that contains a string. A failure status is returned if the expression
  // did not resolve to a string value.
  StatusOr<std::string> GetString() const;

 private:
  friend class CompiledExpression;

  explicit EvaluationResult(std::unique_ptr<internal::WorkSpace> work_space);

  std::unique_ptr<internal::WorkSpace> work_space_;
};

// Represents a FHIRPath expression that has been "compiled" to run efficiently
// against a given protobuf message type.
//
// This class is immutable and thread safe. Users are encouraged to create
// long-lived instances of this class for each FHIRPath expression, and reuse
// those instances to evaluate many records.
class CompiledExpression {
 public:
  CompiledExpression(CompiledExpression&& other);
  CompiledExpression& operator=(CompiledExpression&& other);

  CompiledExpression(const CompiledExpression& other);
  CompiledExpression& operator=(const CompiledExpression& other);

  // Returns the FHIRPath string used to compile this expression.
  const std::string& fhir_path() const;

  // Compiles a FHIRPath expression into a structure that will efficiently
  // execute that expression.
  static StatusOr<CompiledExpression> Compile(
      const ::google::protobuf::Descriptor* descriptor, const std::string& fhir_path);

  // Evaluates the compiled expression against the given message.
  StatusOr<EvaluationResult> Evaluate(const ::google::protobuf::Message& message) const;

 private:
  explicit CompiledExpression(
      const std::string& fhir_path,
      std::shared_ptr<internal::ExpressionNode> root_expression);

  std::string fhir_path_;
  std::shared_ptr<const internal::ExpressionNode> root_expression_;
};

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
    std::vector<CompiledExpression> message_expressions_;

    // FHIRPath constraints on fields
    std::vector<
        std::pair<const ::google::protobuf::FieldDescriptor*, const CompiledExpression>>
        field_expressions_;

    // Nested messages that have constraints, so the evaluation logic
    // knows to check them.
    std::vector<const ::google::protobuf::FieldDescriptor*> nested_with_constraints_;
  };

  // Loads constraints for the given descriptor.
  StatusOr<MessageConstraints*> ConstraintsFor(
      const ::google::protobuf::Descriptor* descriptor);

  // Recursively called validation method that can terminate
  // validation based on the callback.
  Status Validate(const ::google::protobuf::Message& message,
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

#endif  // GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_H_
