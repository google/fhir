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

#include <cstdint>
#include <map>
#include <vector>

#include "google/protobuf/message.h"
#include "absl/status/statusor.h"
#include "google/fhir/annotations.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/terminology/terminology_resolver.h"

namespace google {
namespace fhir {
namespace fhir_path {

namespace internal {

// Represents a single value encountered during FHIRPath evaluation, including
// necessary context about the value's ancestry to determine the resource
// it was derived from (where possible.)
class WorkspaceMessage {
 public:
  explicit WorkspaceMessage(const ::google::protobuf::Message* message)
      : result_(message) {}

  WorkspaceMessage(const WorkspaceMessage& parent,
                   const ::google::protobuf::Message* message)
      : ancestry_stack_(parent.Ancestry()), result_(message) {}

  WorkspaceMessage(const std::vector<const google::protobuf::Message*>& ancestry,
                   const ::google::protobuf::Message* message)
      : ancestry_stack_(ancestry), result_(message) {}

  WorkspaceMessage(const WorkspaceMessage& copy) = default;
  WorkspaceMessage& operator=(const WorkspaceMessage& copy) = default;
  WorkspaceMessage(WorkspaceMessage&& move) = default;
  WorkspaceMessage& operator=(WorkspaceMessage&& move) = default;

  // Returns the Message wrapped by this class.
  const ::google::protobuf::Message* Message() const { return result_; }

  // Finds the nearest message of type Resource for the message wrapped by this
  // class.
  //
  // The nearest resource may be the message wrapped by this class or one of
  // its ancestors. In the case where this message is derived (i.e. not
  // explicitly a node in a resource) a status of NotFound is returned.
  absl::StatusOr<WorkspaceMessage> NearestResource() const;

 private:
  // Returns the ancestry of this message.
  //
  // The front of the vector is the root and the end of the vector is the
  // message stored by this object. In cases where there is not a clear parent
  // (e.g. the result of Resource.foo.empty() is generated during evaluation and
  // is not clearly owned by any resource) the ancestry stack may contain a
  // single element that points to the message wrapped by this class.
  std::vector<const google::protobuf::Message*> Ancestry() const;

  std::vector<const google::protobuf::Message*> ancestry_stack_;
  const ::google::protobuf::Message* result_;
};

// Represents working memory needed to evaluate the expression against
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
  explicit WorkSpace(const PrimitiveHandler* primitive_handler,
                     const ::google::protobuf::Message* message_context)
      : message_context_stack_({WorkspaceMessage(message_context)}),
        primitive_handler_(primitive_handler),
        terminology_resolver_(nullptr) {}

  // Same as WorkSpace(const ::google::protobuf::Message*) but message_context_stack is
  // added the the bottom of the message context stack and message_context is
  // placed on the top.
  explicit WorkSpace(const PrimitiveHandler* primitive_handler,
                     const std::vector<WorkspaceMessage>& message_context_stack,
                     const WorkspaceMessage& message_context)
      : message_context_stack_(message_context_stack),
        primitive_handler_(primitive_handler),
        terminology_resolver_(nullptr) {
    message_context_stack_.push_back(message_context);
  }

  // Same as above, with added parameter for const
  // terminology::TerminologyResolver* and results_cache.
  //
  // results_cache stores the previously evaluated results for expression nodes
  // of type user-defined callback functions.
  explicit WorkSpace(
      const PrimitiveHandler* primitive_handler,
      const std::vector<WorkspaceMessage>& message_context_stack,
      const WorkspaceMessage& message_context,
      const terminology::TerminologyResolver* terminology_resolver,
      const std::map<int32_t, std::vector<WorkspaceMessage>>& results_cache)
      : message_context_stack_(message_context_stack),
        primitive_handler_(primitive_handler),
        terminology_resolver_(terminology_resolver),
        results_cache_(results_cache) {
    message_context_stack_.push_back(message_context);
  }

  // Gets the message context the FHIRPath expression is evaluated against.
  const WorkspaceMessage MessageContext() {
    return message_context_stack_.back();
  }

  // Gets the message context the FHIRPath expression is evaluated against.
  std::vector<WorkspaceMessage> MessageContextStack() {
    return message_context_stack_;
  }

  // Gets the bottom-most message context of this workspace.
  const WorkspaceMessage BottomMessageContext() {
    return message_context_stack_.front();
  }

  // Pushes a new message to the top of the context stack.
  //
  // This is useful, for example, when evaluating a function's argument of type
  // expression whose evaluation context is the message it was invoked on, not
  // the base context of the workspace.
  const void PushMessageContext(const WorkspaceMessage& message_context) {
    message_context_stack_.push_back(message_context);
  }

  // Pops a message off the top of the context stack.
  //
  // Requires that there is more than one message on the stack.
  const void PopMessageContext() {
    DCHECK_GT(message_context_stack_.size(), 1);
    return message_context_stack_.pop_back();
  }

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

  const PrimitiveHandler* GetPrimitiveHandler() {
    return primitive_handler_;
  }

  const terminology::TerminologyResolver* GetTerminologyResolver() const {
    return terminology_resolver_;
  }

  std::map<int32_t, std::vector<WorkspaceMessage>>& GetResultsCache() {
    return results_cache_;
  }

  void SetNextCacheNodeId(int32_t next_cache_node_id) {
    next_cache_node_id_ = next_cache_node_id;
  }

  std::optional<int32_t> GetNextCacheNodeId() const {
    return next_cache_node_id_;
  }

 private:
  std::vector<const ::google::protobuf::Message*> messages_;

  std::vector<WorkspaceMessage> message_context_stack_;

  std::vector<std::unique_ptr<::google::protobuf::Message>> to_delete_;

  const PrimitiveHandler* primitive_handler_;

  const terminology::TerminologyResolver* terminology_resolver_;

  std::map<int32_t, std::vector<WorkspaceMessage>> results_cache_;

  std::optional<int32_t> next_cache_node_id_;
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
  virtual absl::Status Evaluate(
      WorkSpace* work_space, std::vector<WorkspaceMessage>* results) const = 0;

  // The descriptor of the message type returned by the expression.
  virtual const ::google::protobuf::Descriptor* ReturnType() const = 0;

  // The children nodes of the current node, that need to be evaluated first.
  virtual std::vector<std::shared_ptr<ExpressionNode>> GetChildren()
      const = 0;
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
// frequent case where expression evaluates to a single boolean.
//
// Depending on the FHIRPath expression, the result could either be children
// of the original Message, or temporary objects. The EvaluationResult
// itself maintains ownership of those objects and will clean them up
// when it goes out of scope. See the AsMessages() method for details.
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
  absl::StatusOr<bool> GetBoolean() const;

  // Returns success with an integer value if the EvaluationResult represents
  // a integer value per FHIRPath. That is, if it has a single message
  // that contains an integer. A failure status is returned if the expression
  // did not resolve to an integer value.
  absl::StatusOr<int32_t> GetInteger() const;

  // Returns success with a decimal value if the EvaluationResult represents
  // a decimal value per FHIRPath. That is, if it has a single message
  // that contains a decimal. A failure status is returned if the expression
  // did not resolve to a decimal value.
  absl::StatusOr<std::string> GetDecimal() const;

  // Returns success with a string value if the EvaluationResult represents
  // a string value per FHIRPath. That is, if it has a single message
  // that contains a string. A failure status is returned if the expression
  // did not resolve to a string value.
  absl::StatusOr<std::string> GetString() const;

  // If empty, this indicates that the result is final. Otherwise, it means that
  // it's an intermediate result. Intermediate results are inputs to a callback
  // function that the caller need to evaluate on their end, then call
  // CompiledExpression::ResumeEvaluation() with the computed output to resume
  // evaluation.
  // For more information, see documentation on the UserDefinedFunction class.
  std::optional<std::string> CallbackFunctionName() const {
    return callback_function_name_;
  }

 private:
  friend class CompiledExpression;

  explicit EvaluationResult(std::unique_ptr<internal::WorkSpace> work_space,
                            const std::string& callback_function_name = "");

  std::unique_ptr<internal::WorkSpace> work_space_;
  std::optional<std::string> callback_function_name_;
};

// Represents a custom user-defined function.
// These are custom functions defined by the caller and passed to the FHIRPath
// compiler. The caller is responsible for defining how these functions are
// evaluated given the results of the child expression and the compiled
// parameters.
//
// User-defined functions can also be defined as "Callback" function.
// This means that some of the execution of the function has to occur outside
// of the FhirPath engine, for instance for expensive calls to external
// services.  When the engine encounters a Callback function, it will pause
// evaluation with the result of the CallbackFunction, and return an
// EvaluationResult.  In this case, the optional
// `EvaluationResult::CallbackFuctionName()` will be present and populated with
// the name of the Callback function that evaluation was paused on, and the
// EvaluationResult should be considered an "intermediate" result that is the
// input to the external part of the function. The user can then determine the
// output of the Callback function and resume evaluation using
// CompiledExpression::ResumeEvaluation.
//
// For example, consider the expression
// `Encounter.serviceProvider.resolve().meta`
// This should return the `meta` field of the Resource referenced by the
// `Encounter.serviceProvider` field, which is a Reference.  This cannot be
// fully evaluated by the FhirPath engine, since it is unable to look up
// arbitrary resources by id.
// Instead, this could be implemented by a UserDefinedFunction with
// `is_callback_function` set to true, that just returns the Reference proto it
// is evaluated on, so the user can externally resolve the reference and supply
// the result.
//
// E.g.,
//
// absl::StatusOr<CompiledExpression>  expr =
//     CompiledExpression::Compile(Encounter::descriptor(),
//                                 r4_primitive_handler,
//                                 `Encounter.serviceProvider.resolve().meta`,
//                                 terminology_resolver,
//                                 udf_definitions);
// absl::StatusOr<EvaluationResult> result = expr->Evaluate(my_encounter);
// // Check for CallbackFunctionName, which indicates we've paused on an
// // intermediate result.
// while (result->CallbackFunctionName().has_value()) {
//   // There is a CallbackFunctionName, so this is an intermediate result
//   if (*result->CallbackFunctionName() == "resolve") {
//     // Evaluation was paused on a "resolve" callback function.
//     // The implementation of "resolve" according to `udf_definitions` just
//     // returns the reference it was invoked on, so that is the value of the
//     // intermediate result, and we must do the resolving ourself.
//     Observation referenced_obs = DoActualResolving(result->value());
//     // Resume evaluation of the FhirPath expression and update the value of
//     // result.
//     result = expr->ResumeEvaluation(*result, {referenced_obs});
//     continue;
//   }
//   {... Handle other potential callback names ...}
// }
// // There are no more Callback functions - the result is final.
class UserDefinedFunction {
 public:
  // Defines how the parameters are compiled.
  // TODO(b/310238380): Extend this to support functions with optional or
  // variable arguments.
  enum class ParameterVisitorType {
    // Parameter will be compiled against the base context of the expression.
    // To be used for parameters of primitive types or expressions that don't
    // depend on the input.
    kBase,
    // Parameter will be compiled against the child context.
    // To be used for parameters that depend on the child expression or input,
    // for example expressions that access fields within the input.
    kChild,
    // Parameter that should not be parsed and are treated as labels.
    // See https://hl7.org/fhirpath/N1/#identifiers
    kIdentifier
  };

  explicit UserDefinedFunction(const std::string& name,
                               const std::vector<ParameterVisitorType>& params,
                               bool is_callback_function = false)
      : name_(name),
        param_types_(params),
        is_callback_function_(is_callback_function) {}
  virtual ~UserDefinedFunction() {}

  // Validate the function's parameters.
  virtual absl::Status ValidateParams(
      std::vector<std::shared_ptr<internal::ExpressionNode>>& params) const = 0;

  // Evaluate the function given the work_space containing the context stack,
  // the child results representing the input, and the function's parameter
  // nodes. If successful, the implementation should return Status::OK and place
  // the resulting values in the results vector.
  virtual absl::Status Evaluate(
      internal::WorkSpace* work_space,
      const std::vector<internal::WorkspaceMessage>& child_results,
      std::vector<internal::WorkspaceMessage>* results,
      const std::vector<std::shared_ptr<internal::ExpressionNode>>& params...)
      const = 0;

  // The descriptor of the message type returned by the expression.
  virtual const ::google::protobuf::Descriptor* ReturnType() const = 0;

  // The name of the function that the parser will use.
  std::string GetName() const { return name_; }

  // A list of parameter types indicating how to compile each parameter and the
  // number of them.
  std::vector<ParameterVisitorType> GetParamTypes() const {
    return param_types_;
  }

  // Whether the function is a callback function that will return an
  // intermediate result to the caller.
  bool IsCallbackFunction() const { return is_callback_function_; }

 protected:
  const std::string name_;
  const std::vector<ParameterVisitorType> param_types_;
  const bool is_callback_function_;
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
  static absl::StatusOr<CompiledExpression> Compile(
      const ::google::protobuf::Descriptor* descriptor,
      const PrimitiveHandler* primitive_handler, const std::string& fhir_path,
      // If `terminology_resolver` is specified, it should be alive for as long
      // as the returned CompiledExpression object is alive.
      const terminology::TerminologyResolver* terminology_resolver = nullptr,
      const std::vector<UserDefinedFunction*>& user_defined_functions = {});

  // Evaluates the compiled expression against the given message.
  absl::StatusOr<EvaluationResult> Evaluate(
      const ::google::protobuf::Message& message) const;

  // Evaluates the compiled expression against the given message.
  //
  // Use this over Evaluate(const ::google::protobuf::Message&) when additional metadata
  // needs to be included with the message that the expression is being
  // evaluated against (e.g. the message's ancestry.)
  absl::StatusOr<EvaluationResult> Evaluate(
      const internal::WorkspaceMessage& message) const;

  // Resumes evaluation of the compiled expression.
  //
  // To be used whenever an expression contains a callback function and returns
  // an intermediate result. The caller needs to evaluate the callback function
  // and then call this method providing the output of the callback in
  // callback_results. Note that this function may need to be called multiple
  // times for each callback-type function in the expression.
  absl::StatusOr<EvaluationResult> ResumeEvaluation(
      const EvaluationResult& prev_result,
      const std::vector<const Message*>& callback_results) const;

 private:
  explicit CompiledExpression(
      const std::string& fhir_path,
      std::shared_ptr<internal::ExpressionNode> root_expression,
      const PrimitiveHandler* primitive_handler,
      const terminology::TerminologyResolver* terminology_resolver);

  std::string fhir_path_;
  std::shared_ptr<const internal::ExpressionNode> root_expression_;
  const PrimitiveHandler* primitive_handler_;
  const terminology::TerminologyResolver* terminology_resolver_;
};

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_FHIR_PATH_FHIR_PATH_H_
