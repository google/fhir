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

#include "google/fhir/fhir_path/fhir_path.h"

#include <utility>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/util/message_differencer.h"
#include "absl/memory/memory.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/time/civil_time.h"
#include "absl/time/time.h"
#include "absl/types/optional.h"
#include "google/fhir/annotations.h"
#include "google/fhir/fhir_path/FhirPathBaseVisitor.h"
#include "google/fhir/fhir_path/FhirPathLexer.h"
#include "google/fhir/fhir_path/FhirPathParser.h"
#include "google/fhir/fhir_path/utils.h"
#include "google/fhir/primitive_wrapper.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/r4/core/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace fhir_path {

using ::antlr4::ANTLRInputStream;
using ::antlr4::BaseErrorListener;
using ::antlr4::CommonTokenStream;
using ::antlr4::tree::TerminalNode;
using ::antlr_parser::FhirPathBaseVisitor;
using ::antlr_parser::FhirPathLexer;
using ::antlr_parser::FhirPathParser;
using ::google::fhir::AreSameMessageType;
using ::google::fhir::JsonPrimitive;
using ::google::fhir::StatusOr;
using ::google::fhir::r4::core::Boolean;
using ::google::fhir::r4::core::Integer;
using ::google::fhir::r4::core::String;
using internal::ExpressionNode;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::util::MessageDifferencer;
using ::tensorflow::errors::InvalidArgument;
using ::tensorflow::errors::NotFound;

namespace internal {

// Default number of buckets to create when constructing a std::unordered_set.
constexpr int kDefaultSetBucketCount = 10;

// Returns true if the provided FHIR message converts to System.Integer.
//
// See https://www.hl7.org/fhir/fhirpath.html#types
bool IsSystemInteger(const Message& message) {
  return IsInteger(message) || IsUnsignedInt(message) || IsPositiveInt(message);
}

// Returns the integer value of the FHIR message if it can be converted to
// a System.Integer
//
// See https://www.hl7.org/fhir/fhirpath.html#types
StatusOr<int32_t> ToSystemInteger(
    const PrimitiveHandler* primitive_handler, const Message& message) {
  // It isn't necessary to widen the values from 32 to 64 bits when converting
  // a UnsignedInt or PositiveInt to an int32_t because FHIR restricts the
  // values of those types to 31 bits.
  if (IsInteger(message)) {
    return primitive_handler->GetIntegerValue(message).ValueOrDie();
  }

  if (IsUnsignedInt(message)) {
    return primitive_handler->GetUnsignedIntValue(message).ValueOrDie();
  }

  if (IsPositiveInt(message)) {
    return primitive_handler->GetPositiveIntValue(message).ValueOrDie();
  }

  return InvalidArgument(message.GetTypeName(),
                         " cannot be cast to an integer.");
}

StatusOr<absl::optional<int32_t>> IntegerOrEmpty(
    const PrimitiveHandler* primitive_handler,
    const std::vector<WorkspaceMessage>& messages) {
  if (messages.empty()) {
    return absl::optional<int>();
  }

  if (messages.size() > 1 ||
      !IsSystemInteger(*messages[0].Message())) {
    return InvalidArgument(
        "Expression must be empty or represent a single primitive value.");
  }

  FHIR_ASSIGN_OR_RETURN(int32_t value, ToSystemInteger(primitive_handler,
                                                       *messages[0].Message()));
  return absl::optional<int32_t>(value);
}

StatusOr<absl::optional<bool>> BooleanOrEmpty(
    const PrimitiveHandler* primitive_handler,
    const std::vector<WorkspaceMessage>& messages) {
  if (messages.empty()) {
    return absl::optional<bool>();
  }

  if (messages.size() > 1 ||
      !IsPrimitive(messages[0].Message()->GetDescriptor())) {
    return InvalidArgument(
        "Expression must be empty or represent a single primitive value.");
  }

  FHIR_ASSIGN_OR_RETURN(
      bool value, primitive_handler->GetBooleanValue(*messages[0].Message()));
  return absl::optional<bool>(value);
}

// Returns the string representation of the provided message for messages that
// are represented in JSON as strings. For primitive messages that are not
// represented as a string in JSON a status other than OK will be returned.
StatusOr<std::string> MessageToString(const PrimitiveHandler* primitive_handler,
                                      const WorkspaceMessage& message) {
  if (IsString(*message.Message())) {
    return primitive_handler->GetStringValue(*message.Message());
  }

  if (!IsPrimitive(message.Message()->GetDescriptor())) {
    return InvalidArgument("Expression must be a primitive.");
  }

  StatusOr<JsonPrimitive> json_primitive =
      primitive_handler->WrapPrimitiveProto(*message.Message());
  std::string json_string = json_primitive.ValueOrDie().value;

  if (!absl::StartsWith(json_string, "\"")) {
    return InvalidArgument("Expression must evaluate to a string.");
  }

  // Trim the starting and ending double quotation marks from the string (added
  // by JsonPrimitive.)
  return json_string.substr(1, json_string.size() - 2);
}

// Returns the string representation of the provided message for messages that
// are represented in JSON as strings. Requires the presence of exactly one
// message in the provided collection. For primitive messages that are not
// represented as a string in JSON a status other than OK will be returned.
StatusOr<std::string> MessagesToString(
    const PrimitiveHandler* primitive_handler,
    const std::vector<WorkspaceMessage>& messages) {
  if (messages.size() != 1) {
    return InvalidArgument("Expression must represent a single value.");
  }

  return MessageToString(primitive_handler, messages[0]);
}

StatusOr<WorkspaceMessage> WorkspaceMessage::NearestResource() const {
  if (IsResource(result_->GetDescriptor())) {
    return *this;
  }

  auto it = std::find_if(ancestry_stack_.rbegin(), ancestry_stack_.rend(),
                         [](const google::protobuf::Message* message) {
                           return IsResource(message->GetDescriptor());
                         });

  if (it == ancestry_stack_.rend()) {
    return ::tensorflow::errors::NotFound("No Resource found in ancestry.");
  }

  std::vector<const google::protobuf::Message*> resource_ancestry(
      std::make_reverse_iterator(ancestry_stack_.rend()),
      std::make_reverse_iterator(std::next(it)));
  return WorkspaceMessage(resource_ancestry, *it);
}

std::vector<const google::protobuf::Message*> WorkspaceMessage::Ancestry() const {
  std::vector<const google::protobuf::Message*> stack = ancestry_stack_;
  stack.push_back(result_);
  return stack;
}

// Expression node that returns literals wrapped in the corresponding
// protbuf wrapper
template <typename PrimitiveType>
class Literal : public ExpressionNode {
 public:
  Literal(PrimitiveType value, const Descriptor* descriptor,
                   std::function<Message*(PrimitiveType)> factory)
      : value_(value), descriptor_(descriptor), factory_(factory) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    Message* value = factory_(value_);
    work_space->DeleteWhenFinished(value);
    results->push_back(WorkspaceMessage(value));

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return descriptor_;
  }

 private:
  const PrimitiveType value_;
  const Descriptor* descriptor_;
  std::function<Message*(PrimitiveType)> factory_;
};

// Expression node for the empty literal.
class EmptyLiteral : public ExpressionNode {
 public:
  EmptyLiteral() {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    return Status::OK();
  }

  // The return type of the empty literal is undefined. If this causes problems,
  // it is likely we could arbitrarily pick one of the primitive types without
  // ill-effect.
  const Descriptor* ReturnType() const override {
    return nullptr;
  }
};

// Implements the InvocationTerm from the FHIRPath grammar,
// producing a term from the root context message.
class InvokeTermNode : public ExpressionNode {
 public:
  explicit InvokeTermNode(const FieldDescriptor* field,
                          const std::string& field_name)
      : field_(field), field_name_(field_name) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    const WorkspaceMessage& message = work_space->MessageContext();
    const FieldDescriptor* field =
        field_ != nullptr
            ? field_
            : FindFieldByJsonName(message.Message()->GetDescriptor(),
                                  field_name_);

    // If the field cannot be found an empty collection is returned. This
    // matches the behavior of https://github.com/HL7/fhirpath.js and is
    // empirically necessitated by expressions such as "children().element"
    // where not every child necessarily has an "element" field (see FHIRPath
    // constraints on Bundle for a full example.)
    if (field == nullptr) {
      return Status::OK();
    }

    std::vector<const Message*> result_protos;
    FHIR_RETURN_IF_ERROR(
        RetrieveField(*message.Message(), *field, &result_protos));
    for (const Message* result : result_protos) {
      results->push_back(WorkspaceMessage(message, result));
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return field_ != nullptr ? field_->message_type() : nullptr;
  }

 private:
  const FieldDescriptor* field_;
  const std::string field_name_;
};

// Handles the InvocationExpression from the FHIRPath grammar,
// which can be a member of function called on the results of
// another expression.
class InvokeExpressionNode : public ExpressionNode {
 public:
  InvokeExpressionNode(std::shared_ptr<ExpressionNode> child_expression,
                       const FieldDescriptor* field,
                       const std::string& field_name)
      : child_expression_(std::move(child_expression)),
        field_(field),
        field_name_(field_name) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;

    FHIR_RETURN_IF_ERROR(
        child_expression_->Evaluate(work_space, &child_results));

    // Iterate through the results of the child expression and invoke
    // the appropriate field.
    for (const WorkspaceMessage& child_message : child_results) {
      // In the case where the field descriptor was not known at compile time
      // (because ExpressionNode.ReturnType() currently doesn't support
      // collections with mixed types) we attempt to find it at evaluation time.
      const FieldDescriptor* field =
          field_ != nullptr
              ? field_
              : FindFieldByJsonName(child_message.Message()->GetDescriptor(),
                                    field_name_);

      // If the field cannot be found the result is an empty collection. This
      // matches the behavior of https://github.com/HL7/fhirpath.js and is
      // empirically necessitated by expressions such as "children().element"
      // where not every child necessarily has an "element" field (see FHIRPath
      // constraints on Bundle for a full example.)
      if (field == nullptr) {
        continue;
      }

      std::vector<const Message*> result_protos;
      FHIR_RETURN_IF_ERROR(
          RetrieveField(*child_message.Message(), *field, &result_protos));
      for (const Message* result : result_protos) {
        results->push_back(WorkspaceMessage(child_message, result));
      }
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return field_ != nullptr ? field_->message_type() : nullptr;
  }

 private:
  const std::shared_ptr<ExpressionNode> child_expression_;
  // Null if the child_expression_ may evaluate to a collection that contains
  // multiple types.
  const FieldDescriptor* field_;
  const std::string field_name_;
};

class FunctionNode : public ExpressionNode {
 public:
  template <class T>
  StatusOr<T*> static Create(
      const std::shared_ptr<ExpressionNode>& child_expression,
      const std::vector<FhirPathParser::ExpressionContext*>& params,
      FhirPathBaseVisitor* base_context_visitor,
      FhirPathBaseVisitor* child_context_visitor) {
    FHIR_ASSIGN_OR_RETURN(
        std::vector<std::shared_ptr<ExpressionNode>> compiled_params,
        T::CompileParams(params, base_context_visitor, child_context_visitor));
    FHIR_RETURN_IF_ERROR(T::ValidateParams(compiled_params));
    return new T(child_expression, compiled_params);
  }

  static StatusOr<std::vector<std::shared_ptr<ExpressionNode>>> CompileParams(
      const std::vector<FhirPathParser::ExpressionContext*>& params,
      FhirPathBaseVisitor* base_context_visitor,
      FhirPathBaseVisitor*) {
    return CompileParams(params, base_context_visitor);
  }

  static StatusOr<std::vector<std::shared_ptr<ExpressionNode>>> CompileParams(
      const std::vector<FhirPathParser::ExpressionContext*>& params,
      FhirPathBaseVisitor* visitor) {
    std::vector<std::shared_ptr<ExpressionNode>> compiled_params;

    for (auto it = params.begin(); it != params.end(); ++it) {
      antlrcpp::Any param_any = (*it)->accept(visitor);
      if (param_any.isNull()) {
        return InvalidArgument("Failed to compile parameter.");
      }
      compiled_params.push_back(
          param_any.as<std::shared_ptr<ExpressionNode>>());
    }

    return compiled_params;
  }

  // This is the default implementation. FunctionNodes's that need to validate
  // params at compile time should overwrite this definition with their own.
  static Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    return Status::OK();
  }

 protected:
  FunctionNode(const std::shared_ptr<ExpressionNode>& child,
               const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : child_(child), params_(params) {}

  const std::shared_ptr<ExpressionNode> child_;
  const std::vector<std::shared_ptr<ExpressionNode>> params_;
};

class ZeroParameterFunctionNode : public FunctionNode {
 public:
  static Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    if (!params.empty()) {
      return InvalidArgument("Function does not accept any arguments.");
    }

    return Status::OK();
  }

 protected:
  ZeroParameterFunctionNode(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {
    TF_DCHECK_OK(ValidateParams(params));
  }
};

class SingleParameterFunctionNode : public FunctionNode {
 private:
  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    //  requires a single parameter
    if (params_.size() != 1) {
      return InvalidArgument("this function requires a single parameter.");
    }

    std::vector<WorkspaceMessage> first_param;
    FHIR_RETURN_IF_ERROR(params_[0]->Evaluate(work_space, &first_param));

    return Evaluate(work_space, first_param, results);
  }

 public:
  static Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    if (params.size() != 1) {
      return InvalidArgument("Function requires exactly one argument.");
    }

    return Status::OK();
  }

 protected:
  SingleParameterFunctionNode(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {
    TF_DCHECK_OK(ValidateParams(params));
  }

  virtual Status Evaluate(WorkSpace* work_space,
                          const std::vector<WorkspaceMessage>& first_param,
                          std::vector<WorkspaceMessage>* results) const = 0;
};

class SingleValueFunctionNode : public SingleParameterFunctionNode {
 private:
  Status Evaluate(WorkSpace* work_space,
                  const std::vector<WorkspaceMessage>& first_param,
                  std::vector<WorkspaceMessage>* results) const override {
    //  requires a single parameter
    if (first_param.size() != 1) {
      return InvalidArgument(
          "this function requires a single value parameter.");
    }

    return EvaluateWithParam(work_space, first_param[0], results);
  }

 protected:
  SingleValueFunctionNode(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleParameterFunctionNode(child, params) {}

  virtual Status EvaluateWithParam(
      WorkSpace* work_space, const WorkspaceMessage& param,
      std::vector<WorkspaceMessage>* results) const = 0;
};

// Implements the FHIRPath .exists() function
class ExistsFunction : public ZeroParameterFunctionNode {
 public:
  ExistsFunction(const std::shared_ptr<ExpressionNode>& child,
                 const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    Message* result =
        work_space->GetPrimitiveHandler()->NewBoolean(!child_results.empty());
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }
};

// Implements the FHIRPath .not() function.
class NotFunction : public ZeroParameterFunctionNode {
 public:
  explicit NotFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params = {})
      : ZeroParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;

    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    // Per the FHIRPath spec, boolean operations on empty collection
    // propagate the empty collection.
    if (child_results.empty()) {
      return Status::OK();
    }

    if (child_results.size() != 1) {
      return InvalidArgument("not() must be invoked on a singleton collection");
    }

    // Per the FHIR spec, the not() function produces a value
    // IFF it is given a boolean input, and returns an empty result
    // otherwise.
    FHIR_ASSIGN_OR_RETURN(bool child_result,
                          work_space->GetPrimitiveHandler()->GetBooleanValue(
                              *child_results[0].Message()));

    Message* result =
        work_space->GetPrimitiveHandler()->NewBoolean(!child_result);
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }
};

// Implements the FHIRPath .hasValue() function, which returns true
// if and only if the child is a single primitive value.
class HasValueFunction : public ZeroParameterFunctionNode {
 public:
  HasValueFunction(const std::shared_ptr<ExpressionNode>& child,
                   const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;

    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    Message* result = work_space->GetPrimitiveHandler()->NewBoolean(
        child_results.size() == 1 &&
        IsPrimitive(child_results[0].Message()->GetDescriptor()));
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }
};

// Implements the FHIRPath .startsWith() function, which returns true if and
// only if the child string starts with the given string. When the given string
// is the empty string .startsWith() returns true.
//
// Missing or incorrect parameters will end evaluation and cause Evaluate to
// return a status other than OK. See
// http://hl7.org/fhirpath/2018Sep/index.html#functions-2.
//
// Please note that execution will proceed on any String-like type.
// Specifically, any type for which its JsonPrimitive value is a string. This
// differs from the allowed implicit conversions defined in
// https://hl7.org/fhirpath/2018Sep/index.html#conversion.
class StartsWithFunction : public SingleValueFunctionNode {
 public:
  explicit StartsWithFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleValueFunctionNode(child, params) {}

  Status EvaluateWithParam(
      WorkSpace* work_space, const WorkspaceMessage& param,
      std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() != 1) {
      return InvalidArgument(kInvalidArgumentMessage);
    }

    FHIR_ASSIGN_OR_RETURN(
        std::string item,
        MessagesToString(work_space->GetPrimitiveHandler(), child_results));
    FHIR_ASSIGN_OR_RETURN(
        std::string prefix,
        MessageToString(work_space->GetPrimitiveHandler(), param));

    Message* result = work_space->GetPrimitiveHandler()->NewBoolean(
        absl::StartsWith(item, prefix));
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }

 private:
  static constexpr char kInvalidArgumentMessage[] =
      "startsWith must be invoked on a string with a single string "
      "argument";
};
constexpr char StartsWithFunction::kInvalidArgumentMessage[];

// Implements the FHIRPath .contains() function.
class ContainsFunction : public SingleValueFunctionNode {
 public:
  explicit ContainsFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleValueFunctionNode(child, params) {}

  Status EvaluateWithParam(
      WorkSpace* work_space, const WorkspaceMessage& param,
      std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.empty()) {
      return Status::OK();
    }

    if (child_results.size() > 1) {
      return InvalidArgument("contains() must be invoked on a single string.");
    }

    FHIR_ASSIGN_OR_RETURN(
        std::string haystack,
        MessagesToString(work_space->GetPrimitiveHandler(), child_results));
    FHIR_ASSIGN_OR_RETURN(
        std::string needle,
        MessageToString(work_space->GetPrimitiveHandler(), param));

    Message* result = work_space->GetPrimitiveHandler()->NewBoolean(
        absl::StrContains(haystack, needle));
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }
};

class MatchesFunction : public SingleValueFunctionNode {
 public:
  explicit MatchesFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleValueFunctionNode(child, params) {}

  Status EvaluateWithParam(
      WorkSpace* work_space, const WorkspaceMessage& param,
      std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.empty()) {
      return Status::OK();
    }

    FHIR_ASSIGN_OR_RETURN(
        std::string item,
        MessagesToString(work_space->GetPrimitiveHandler(), child_results));
    FHIR_ASSIGN_OR_RETURN(
        std::string re_string,
        MessageToString(work_space->GetPrimitiveHandler(), param));

    RE2 re(re_string);

    if (!re.ok()) {
      return InvalidArgument(
          absl::StrCat("Unable to parse regular expression, '", re_string,
                       "'. ", re.error()));
    }

    Message* result =
        work_space->GetPrimitiveHandler()->NewBoolean(RE2::FullMatch(item, re));
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }
};

class ReplaceMatchesFunction : public FunctionNode {
 public:
  explicit ReplaceMatchesFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> pattern_param;
    FHIR_RETURN_IF_ERROR(params_[0]->Evaluate(work_space, &pattern_param));

    std::vector<WorkspaceMessage> replacement_param;
    FHIR_RETURN_IF_ERROR(params_[1]->Evaluate(work_space, &replacement_param));

    return EvaluateWithParam(work_space, pattern_param, replacement_param,
                             results);
  }

  Status EvaluateWithParam(
      WorkSpace* work_space, const std::vector<WorkspaceMessage>& pattern_param,
      const std::vector<WorkspaceMessage>& replacement_param,
      std::vector<WorkspaceMessage>* results) const {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.empty()) {
      return Status::OK();
    }

    FHIR_ASSIGN_OR_RETURN(
        std::string item,
        MessagesToString(work_space->GetPrimitiveHandler(), child_results));
    FHIR_ASSIGN_OR_RETURN(
        std::string re_string,
        MessagesToString(work_space->GetPrimitiveHandler(), pattern_param));
    FHIR_ASSIGN_OR_RETURN(
        std::string replacement_string,
        MessagesToString(work_space->GetPrimitiveHandler(), replacement_param));

    RE2 re(re_string);

    if (!re.ok()) {
      return InvalidArgument(
          absl::StrCat("Unable to parse regular expression '", re_string,
                       "'. ", re.error()));
    }

    RE2::Replace(&item, re, replacement_string);

    Message* result = work_space->GetPrimitiveHandler()->NewString(item);
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return Status::OK();
  }

  const Descriptor* ReturnType() const override { return String::descriptor(); }

  static Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    if (params.size() != 2) {
      return InvalidArgument(
          "replaceMatches requires exactly two parameters. Got ",
          params.size(), ".");
    }

    return Status::OK();
  }
};

class ToStringFunction : public ZeroParameterFunctionNode {
 public:
  ToStringFunction(const std::shared_ptr<ExpressionNode>& child,
                   const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return InvalidArgument(
          "Input collection must not contain multiple items");
    }

    if (child_results.empty()) {
      return Status::OK();
    }

    const WorkspaceMessage& child = child_results[0];

    if (IsString(*child.Message())) {
      results->push_back(child);
      return Status::OK();
    }

    if (!IsPrimitive(child.Message()->GetDescriptor())) {
      return Status::OK();
    }

    FHIR_ASSIGN_OR_RETURN(JsonPrimitive json_primitive,
                          work_space->GetPrimitiveHandler()->WrapPrimitiveProto(
                              *child.Message()));
    std::string json_string = json_primitive.value;

    if (absl::StartsWith(json_string, "\"")) {
      json_string = json_string.substr(1, json_string.size() - 2);
    }

    Message* result = work_space->GetPrimitiveHandler()->NewString(json_string);
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return Status::OK();
  }

  const Descriptor* ReturnType() const override { return String::descriptor(); }
};

// Implements the FHIRPath .length() function.
class LengthFunction : public ZeroParameterFunctionNode {
 public:
  LengthFunction(const std::shared_ptr<ExpressionNode>& child,
                 const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.empty()) {
      return Status::OK();
    }

    FHIR_ASSIGN_OR_RETURN(
        std::string item,
        MessagesToString(work_space->GetPrimitiveHandler(), child_results));

    Message* result =
        work_space->GetPrimitiveHandler()->NewInteger(item.length());
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Integer::descriptor();
  }
};

// Implements the FHIRPath .empty() function.
//
// Returns true if the input collection is empty and false otherwise.
class EmptyFunction : public ZeroParameterFunctionNode {
 public:
  EmptyFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    Message* result =
        work_space->GetPrimitiveHandler()->NewBoolean(child_results.empty());
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }
};

// Implements the FHIRPath .count() function.
//
// Returns the size of the input collection as an integer.
class CountFunction : public ZeroParameterFunctionNode {
 public:
  CountFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    Message* result =
        work_space->GetPrimitiveHandler()->NewInteger(child_results.size());
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Integer::descriptor();
  }
};

// Implements the FHIRPath .first() function.
//
// Returns the first element of the input collection. Or an empty collection if
// if the input collection is empty.
class FirstFunction : public ZeroParameterFunctionNode {
 public:
  FirstFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (!child_results.empty()) {
      results->push_back(child_results[0]);
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return child_->ReturnType();
  }
};

// Implements the FHIRPath .tail() function.
class TailFunction : public ZeroParameterFunctionNode {
 public:
  TailFunction(const std::shared_ptr<ExpressionNode>& child,
               const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      results->insert(results->begin(), std::next(child_results.begin()),
                      child_results.end());
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override { return child_->ReturnType(); }
};

// Implements the FHIRPath .trace() function.
class TraceFunction : public SingleValueFunctionNode {
 public:
  TraceFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleValueFunctionNode(child, params) {}

  Status EvaluateWithParam(
      WorkSpace* work_space, const WorkspaceMessage& param,
      std::vector<WorkspaceMessage>* results) const override {
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, results));
    FHIR_ASSIGN_OR_RETURN(
        std::string name,
        MessageToString(work_space->GetPrimitiveHandler(), param));

    DVLOG(1) << "trace(" << name << "):";
    for (auto it = results->begin(); it != results->end(); it++) {
      DVLOG(1) << (*it).Message()->DebugString();
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return child_->ReturnType();
  }
};

// Implements the FHIRPath .toInteger() function.
class ToIntegerFunction : public ZeroParameterFunctionNode {
 public:
  ToIntegerFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return InvalidArgument(
          "toInterger() requires a collection with no more than 1 item.");
    }

    if (child_results.empty()) {
      return Status::OK();
    }

    const WorkspaceMessage& child_result = child_results[0];

    if (!IsPrimitive(child_result.Message()->GetDescriptor())) {
      return Status::OK();
    }

    if (IsInteger(*child_result.Message())) {
      results->push_back(child_result);
      return Status::OK();
    }

    if (IsBoolean(*child_result.Message())) {
      FHIR_ASSIGN_OR_RETURN(bool value,
                            work_space->GetPrimitiveHandler()->GetBooleanValue(
                                *child_result.Message()));
      Message* result = work_space->GetPrimitiveHandler()->NewInteger(value);
      work_space->DeleteWhenFinished(result);
      results->push_back(WorkspaceMessage(result));
      return Status::OK();
    }

    auto child_as_string =
        MessagesToString(work_space->GetPrimitiveHandler(), child_results);
    if (child_as_string.ok()) {
      int32_t value;
      if (absl::SimpleAtoi(child_as_string.ValueOrDie(), &value)) {
        Message* result = work_space->GetPrimitiveHandler()->NewInteger(value);
        work_space->DeleteWhenFinished(result);
        results->push_back(WorkspaceMessage(result));
        return Status::OK();
      }
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Integer::descriptor();
  }
};

// Base class for FHIRPath binary operators.
class BinaryOperator : public ExpressionNode {
 public:
  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> left_results;
    FHIR_RETURN_IF_ERROR(left_->Evaluate(work_space, &left_results));

    std::vector<WorkspaceMessage> right_results;
    FHIR_RETURN_IF_ERROR(right_->Evaluate(work_space, &right_results));

    return EvaluateOperator(left_results, right_results, work_space, results);
  }

  BinaryOperator(std::shared_ptr<ExpressionNode> left,
                 std::shared_ptr<ExpressionNode> right)
      : left_(left), right_(right) {}

  // Perform the actual boolean evaluation.
  virtual Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* out_results) const = 0;

 protected:
  const std::shared_ptr<ExpressionNode> left_;

  const std::shared_ptr<ExpressionNode> right_;
};

class IndexerExpression : public BinaryOperator {
 public:
  IndexerExpression(const PrimitiveHandler* primitive_handler,
                    std::shared_ptr<ExpressionNode> left,
                    std::shared_ptr<ExpressionNode> right)
      : BinaryOperator(left, right), primitive_handler_(primitive_handler) {}

  Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* out_results) const override {
    FHIR_ASSIGN_OR_RETURN(auto index,
                          (IntegerOrEmpty(primitive_handler_, right_results)));
    if (!index.has_value()) {
      return InvalidArgument("Index must be present.");
    }

    if (left_results.empty() || left_results.size() <= index.value()) {
      return Status::OK();
    }

    out_results->push_back(left_results[index.value()]);
    return Status::OK();
  }

  const Descriptor* ReturnType() const override { return left_->ReturnType(); }

 private:
  const PrimitiveHandler* primitive_handler_;
};

class EqualsOperator : public BinaryOperator {
 public:
  Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* out_results) const override {
    if (left_results.empty() || right_results.empty()) {
      return Status::OK();
    }

    Message* result = work_space->GetPrimitiveHandler()->NewBoolean(AreEqual(
        work_space->GetPrimitiveHandler(), left_results, right_results));
    work_space->DeleteWhenFinished(result);
    out_results->push_back(WorkspaceMessage(result));
    return Status::OK();
  }

  static bool AreEqual(const PrimitiveHandler* primitive_handler,
                       const std::vector<WorkspaceMessage>& left_results,
                       const std::vector<WorkspaceMessage>& right_results) {
    if (left_results.size() != right_results.size()) {
      return false;
    }

    for (int i = 0; i < left_results.size(); ++i) {
        const WorkspaceMessage& left = left_results.at(i);
        const WorkspaceMessage& right = right_results.at(i);
        if (!AreEqual(primitive_handler, *left.Message(), *right.Message())) {
          return false;
        }
    }
    return true;
  }

  static bool AreEqual(const PrimitiveHandler* primitive_handler,
                       const Message& left, const Message& right) {
    if (AreSameMessageType(left, right)) {
      return MessageDifferencer::Equals(left, right);
    } else {
      // When dealing with different types we might be comparing a
      // primitive type (like an enum) to a literal string, which is
      // supported. Therefore we simply convert both to string form
      // and consider them unequal if either is not a string.
      StatusOr<JsonPrimitive> left_primitive =
          primitive_handler->WrapPrimitiveProto(left);
      StatusOr<JsonPrimitive> right_primitive =
          primitive_handler->WrapPrimitiveProto(right);

      // Comparisons between primitives and non-primitives are valid
      // in FHIRPath and should simply return false rather than an error.
      return left_primitive.ok() && right_primitive.ok() &&
             left_primitive.ValueOrDie().value ==
                 right_primitive.ValueOrDie().value;
    }
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }

  EqualsOperator(std::shared_ptr<ExpressionNode> left,
                 std::shared_ptr<ExpressionNode> right)
      : BinaryOperator(left, right) {}
};

struct ProtoPtrSameTypeAndEqual {
  explicit ProtoPtrSameTypeAndEqual(const PrimitiveHandler* primitive_handler)
      : primitive_handler(primitive_handler) {}
  const PrimitiveHandler* primitive_handler;

  bool operator()(const WorkspaceMessage& lhs,
                  const WorkspaceMessage& rhs) const {
    return (lhs.Message() == rhs.Message()) ||
           ((lhs.Message() != nullptr && rhs.Message() != nullptr) &&
            EqualsOperator::AreEqual(primitive_handler, *lhs.Message(),
                                     *rhs.Message()));
  }
};

struct ProtoPtrHash {
  explicit ProtoPtrHash(const PrimitiveHandler* primitive_handler)
      : primitive_handler(primitive_handler) {}
  const PrimitiveHandler* primitive_handler;

  size_t operator()(const WorkspaceMessage& result) const {
    const google::protobuf::Message* message = result.Message();
    if (message == nullptr) {
      return 0;
    }

    // TODO: This will crash on a non-STU3 or R4 primitive.
    // That's probably ok for now but we should fix this to never crash ASAP.
    if (IsPrimitive(message->GetDescriptor())) {
      return std::hash<std::string>{}(
          primitive_handler->WrapPrimitiveProto(*message).ValueOrDie().value);
    }

    return std::hash<std::string>{}(message->SerializeAsString());
  }
};

class UnionOperator : public BinaryOperator {
 public:
  UnionOperator(std::shared_ptr<ExpressionNode> left,
                std::shared_ptr<ExpressionNode> right)
      : BinaryOperator(std::move(left), std::move(right)) {}

  Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* out_results) const override {
    std::unordered_set<WorkspaceMessage, ProtoPtrHash, ProtoPtrSameTypeAndEqual>
        results(kDefaultSetBucketCount,
                ProtoPtrHash(work_space->GetPrimitiveHandler()),
                ProtoPtrSameTypeAndEqual(work_space->GetPrimitiveHandler()));
    results.insert(left_results.begin(), left_results.end());
    results.insert(right_results.begin(), right_results.end());
    out_results->insert(out_results->begin(), results.begin(), results.end());
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    // If the return type of one of the operands is unknown, the return type of
    // the union operator is unknown.
    if (left_->ReturnType() == nullptr || right_->ReturnType() == nullptr) {
      return nullptr;
    }

    if (AreSameMessageType(left_->ReturnType(), right_->ReturnType())) {
      return left_->ReturnType();
    }

    // TODO: Consider refactoring ReturnType to return a set of all types
    // in the collection.
    return nullptr;
  }
};

// Implements the FHIRPath .isDistinct() function.
class IsDistinctFunction : public ZeroParameterFunctionNode {
 public:
  IsDistinctFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    std::unordered_set<WorkspaceMessage, ProtoPtrHash, ProtoPtrSameTypeAndEqual>
        child_results_set(
            child_results.begin(), child_results.end(), kDefaultSetBucketCount,
            ProtoPtrHash(work_space->GetPrimitiveHandler()),
            ProtoPtrSameTypeAndEqual(work_space->GetPrimitiveHandler()));

    Message* result = work_space->GetPrimitiveHandler()->NewBoolean(
        child_results_set.size() == child_results.size());
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::GetDescriptor();
  }
};

// Implements the FHIRPath .distinct() function.
class DistinctFunction : public ZeroParameterFunctionNode {
 public:
  DistinctFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    std::unordered_set<WorkspaceMessage, ProtoPtrHash, ProtoPtrSameTypeAndEqual>
        result_set(child_results.begin(), child_results.end(),
                   kDefaultSetBucketCount,
                   ProtoPtrHash(work_space->GetPrimitiveHandler()),
                   ProtoPtrSameTypeAndEqual(work_space->GetPrimitiveHandler()));
    results->insert(results->begin(), result_set.begin(), result_set.end());
    return Status::OK();
  }

  const Descriptor* ReturnType() const override { return child_->ReturnType(); }
};

// Implements the FHIRPath .combine() function.
class CombineFunction : public SingleParameterFunctionNode {
 public:
  CombineFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  const std::vector<WorkspaceMessage>& first_param,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    results->insert(results->end(), child_results.begin(), child_results.end());
    results->insert(results->end(), first_param.begin(), first_param.end());
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    DCHECK_EQ(params_.size(), 1);

    if (child_->ReturnType() == nullptr ||
        params_[0]->ReturnType() == nullptr) {
      return nullptr;
    }

    if (AreSameMessageType(child_->ReturnType(), params_[0]->ReturnType())) {
      return child_->ReturnType();
    }

    // TODO: Consider refactoring ReturnType to return a set of all types
    // in the collection.
    return nullptr;
  }
};

// Implements the FHIRPath .where() function.
class WhereFunction : public FunctionNode {
 public:
  static Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    if (params.size() != 1) {
      return InvalidArgument("Function requires exactly one argument.");
    }

    return Status::OK();
  }

  static StatusOr<std::vector<std::shared_ptr<ExpressionNode>>> CompileParams(
      const std::vector<FhirPathParser::ExpressionContext*>& params,
      FhirPathBaseVisitor*,
      FhirPathBaseVisitor* child_context_visitor) {
    return FunctionNode::CompileParams(params, child_context_visitor);
  }

  WhereFunction(const std::shared_ptr<ExpressionNode>& child,
                const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {
    TF_DCHECK_OK(ValidateParams(params));
  }

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    for (const WorkspaceMessage& message : child_results) {
      std::vector<WorkspaceMessage> param_results;
      WorkSpace expression_work_space(work_space->GetPrimitiveHandler(),
                                      work_space->MessageContextStack(),
                                      message);
      FHIR_RETURN_IF_ERROR(
          params_[0]->Evaluate(&expression_work_space, &param_results));
      FHIR_ASSIGN_OR_RETURN(
          StatusOr<absl::optional<bool>> allowed,
          (BooleanOrEmpty(work_space->GetPrimitiveHandler(), param_results)));
      if (allowed.ValueOrDie().value_or(false)) {
        results->push_back(message);
      }
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override { return child_->ReturnType(); }
};

// Implements the FHIRPath .all() function.
class AllFunction : public FunctionNode {
 public:
  static Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    if (params.size() != 1) {
      return InvalidArgument("Function requires exactly one argument.");
    }

    return Status::OK();
  }

  static StatusOr<std::vector<std::shared_ptr<ExpressionNode>>> CompileParams(
      const std::vector<FhirPathParser::ExpressionContext*>& params,
      FhirPathBaseVisitor*,
      FhirPathBaseVisitor* child_context_visitor) {
    return FunctionNode::CompileParams(params, child_context_visitor);
  }

  AllFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {
    TF_DCHECK_OK(ValidateParams(params));
  }

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));
    FHIR_ASSIGN_OR_RETURN(bool result, Evaluate(work_space, child_results));

    Message* result_message =
        work_space->GetPrimitiveHandler()->NewBoolean(result);
    work_space->DeleteWhenFinished(result_message);
    results->push_back(WorkspaceMessage(result_message));
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::GetDescriptor();
  }

 private:
  StatusOr<bool> Evaluate(
      WorkSpace* work_space,
      const std::vector<WorkspaceMessage>& child_results) const {
    for (const WorkspaceMessage& message : child_results) {
      std::vector<WorkspaceMessage> param_results;
      WorkSpace expression_work_space(work_space->GetPrimitiveHandler(),
                                      work_space->MessageContextStack(),
                                      message);
      FHIR_RETURN_IF_ERROR(
          params_[0]->Evaluate(&expression_work_space, &param_results));
      FHIR_ASSIGN_OR_RETURN(
          StatusOr<absl::optional<bool>> criteria_met,
          (BooleanOrEmpty(work_space->GetPrimitiveHandler(), param_results)));
      if (!criteria_met.ValueOrDie().value_or(false)) {
        return false;
      }
    }

    return true;
  }
};

// Implements the FHIRPath .select() function.
class SelectFunction : public FunctionNode {
 public:
  static Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    if (params.size() != 1) {
      return InvalidArgument("Function requires exactly one argument.");
    }

    return Status::OK();
  }

  static StatusOr<std::vector<std::shared_ptr<ExpressionNode>>> CompileParams(
      const std::vector<FhirPathParser::ExpressionContext*>& params,
      FhirPathBaseVisitor*,
      FhirPathBaseVisitor* child_context_visitor) {
    return FunctionNode::CompileParams(params, child_context_visitor);
  }

  SelectFunction(const std::shared_ptr<ExpressionNode>& child,
                 const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {
    TF_DCHECK_OK(ValidateParams(params));
  }

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    for (const WorkspaceMessage& message : child_results) {
      work_space->PushMessageContext(message);
      Status status = params_[0]->Evaluate(work_space, results);
      work_space->PopMessageContext();
      FHIR_RETURN_IF_ERROR(status);
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return params_[0]->ReturnType();
  }
};

// Implements the FHIRPath .iif() function.
class IifFunction : public FunctionNode {
 public:
  static StatusOr<std::vector<std::shared_ptr<ExpressionNode>>> CompileParams(
      const std::vector<FhirPathParser::ExpressionContext*>& params,
      FhirPathBaseVisitor* base_context_visitor,
      FhirPathBaseVisitor* child_context_visitor) {
    if (params.size() < 2 || params.size() > 3) {
      return InvalidArgument("iif() requires 2 or 3 arugments.");
    }

    std::vector<std::shared_ptr<ExpressionNode>> compiled_params;

    antlrcpp::Any criterion = params[0]->accept(child_context_visitor);
    if (criterion.isNull()) {
      return InvalidArgument("Failed to compile parameter.");
    }
    compiled_params.push_back(criterion.as<std::shared_ptr<ExpressionNode>>());

    antlrcpp::Any true_result = params[1]->accept(base_context_visitor);
    if (true_result.isNull()) {
      return InvalidArgument("Failed to compile parameter.");
    }
    compiled_params.push_back(
        true_result.as<std::shared_ptr<ExpressionNode>>());

    if (params.size() > 2) {
      antlrcpp::Any otherwise_result = params[2]->accept(base_context_visitor);
      if (otherwise_result.isNull()) {
        return InvalidArgument("Failed to compile parameter.");
      }
      compiled_params.push_back(
          otherwise_result.as<std::shared_ptr<ExpressionNode>>());
    }

    return compiled_params;
  }

  IifFunction(const std::shared_ptr<ExpressionNode>& child,
              const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {
    TF_DCHECK_OK(ValidateParams(params));
  }

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return InvalidArgument(
          "iif() requires a collection with no more than 1 item.");
    }

    if (child_results.empty()) {
      return Status::OK();
    }

    const WorkspaceMessage& child = child_results[0];

    std::vector<WorkspaceMessage> param_results;
    WorkSpace expression_work_space(work_space->GetPrimitiveHandler(),
                                    work_space->MessageContextStack(), child);
    FHIR_RETURN_IF_ERROR(
        params_[0]->Evaluate(&expression_work_space, &param_results));
    FHIR_ASSIGN_OR_RETURN(
        StatusOr<absl::optional<bool>> criterion_met,
        (BooleanOrEmpty(work_space->GetPrimitiveHandler(), param_results)));
    if (criterion_met.ValueOrDie().value_or(false)) {
      FHIR_RETURN_IF_ERROR(params_[1]->Evaluate(work_space, results));
    } else if (params_.size() > 2) {
      FHIR_RETURN_IF_ERROR(params_[2]->Evaluate(work_space, results));
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override { return child_->ReturnType(); }
};

// Implements the FHIRPath .is() function.
//
// TODO: This does not currently validate that the tested type exists.
// According to the FHIRPath spec, if the type does not exist the expression
// should throw an error instead of returning false.
//
// TODO: Handle type namespaces (i.e. FHIR.* and System.*)
//
// TODO: Handle type inheritance correctly. For example, a Patient
// resource is a DomainResource, but this function, as is, will return false.
class IsFunction : public ExpressionNode {
 public:
  StatusOr<IsFunction*> static Create(
      const std::shared_ptr<ExpressionNode>& child_expression,
      const std::vector<FhirPathParser::ExpressionContext*>& params,
      FhirPathBaseVisitor* base_context_visitor,
      FhirPathBaseVisitor* child_context_visitor) {
    if (params.size() != 1) {
      return InvalidArgument("is() requires a single argument.");
    }

    return new IsFunction(child_expression, params[0]->getText());
  }

  IsFunction(const std::shared_ptr<ExpressionNode>& child,
             std::string type_name)
      : child_(child), type_name_(type_name) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return InvalidArgument(
          "is() requires a collection with no more than 1 item.");
    }

    if (child_results.empty()) {
      return Status::OK();
    }

    Message* result =
        work_space->GetPrimitiveHandler()->NewBoolean(absl::EqualsIgnoreCase(
            child_results[0].Message()->GetDescriptor()->name(), type_name_));
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::GetDescriptor();
  }

 private:
  const std::shared_ptr<ExpressionNode> child_;
  const std::string type_name_;
};

// Implements the FHIRPath .as() function.
//
// TODO: This does not currently validate that the tested type exists.
// According to the FHIRPath spec, if the type does not exist the expression
// should throw an error.
//
// TODO: Handle type namespaces (i.e. FHIR.* and System.*)
//
// TODO: Handle type inheritance correctly. For example, a Patient
// resource is a DomainResource, but this function, as is, will behave as if
// a Patient is not a DomainResource and return an empty collection.
class AsFunction : public ExpressionNode {
 public:
  StatusOr<AsFunction*> static Create(
      const std::shared_ptr<ExpressionNode>& child_expression,
      const std::vector<FhirPathParser::ExpressionContext*>& params,
      FhirPathBaseVisitor* base_context_visitor,
      FhirPathBaseVisitor* child_context_visitor) {
    if (params.size() != 1) {
      return InvalidArgument("as() requires a single argument.");
    }

    return new AsFunction(child_expression, params[0]->getText());
  }

  AsFunction(const std::shared_ptr<ExpressionNode>& child,
             std::string type_name)
      : child_(child), type_name_(type_name) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return InvalidArgument(
          "as() requires a collection with no more than 1 item.");
    }

    if (!child_results.empty() &&
        absl::EqualsIgnoreCase(
            child_results[0].Message()->GetDescriptor()->name(), type_name_)) {
      results->push_back(child_results[0]);
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    // TODO: Fetch the descriptor based on this->type_name_.
    return nullptr;
  }

 private:
  const std::shared_ptr<ExpressionNode> child_;
  const std::string type_name_;
};

class ChildrenFunction : public ZeroParameterFunctionNode {
 public:
  ChildrenFunction(const std::shared_ptr<ExpressionNode>& child,
                 const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    for (const WorkspaceMessage& child : child_results) {
      const Descriptor* descriptor = child.Message()->GetDescriptor();
      for (int i = 0; i < descriptor->field_count(); i++) {
        std::vector<const Message*> messages;
        FHIR_RETURN_IF_ERROR(
            RetrieveField(*child.Message(), *descriptor->field(i), &messages));
        for (const Message* message : messages) {
            results->push_back(WorkspaceMessage(child, message));
        }
      }
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return nullptr;
  }
};

class DescendantsFunction : public ZeroParameterFunctionNode {
 public:
  DescendantsFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    for (const WorkspaceMessage& child : child_results) {
      FHIR_RETURN_IF_ERROR(AppendDescendants(child, work_space, results));
    }

    return Status::OK();
  }

  Status AppendDescendants(const WorkspaceMessage& parent,
                           WorkSpace* work_space,
                           std::vector<WorkspaceMessage>* results) const {
    const Descriptor* descriptor = parent.Message()->GetDescriptor();
    if (IsPrimitive(descriptor)) {
      return Status::OK();
    }

    for (int i = 0; i < descriptor->field_count(); i++) {
      std::vector<const Message*> messages;
      FHIR_RETURN_IF_ERROR(RetrieveField(
          *parent.Message(), *descriptor->field(i), &messages));
      for (const Message* message : messages) {
        WorkspaceMessage child(parent, message);
        results->push_back(child);
        FHIR_RETURN_IF_ERROR(AppendDescendants(child, work_space, results));
      }
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override { return nullptr; }
};

// Implements the FHIRPath .intersect() function.
class IntersectFunction : public SingleParameterFunctionNode {
 public:
  IntersectFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleParameterFunctionNode(child, params) {}

  Status Evaluate(WorkSpace* work_space,
                  const std::vector<WorkspaceMessage>& first_param,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    std::unordered_set<WorkspaceMessage, ProtoPtrHash, ProtoPtrSameTypeAndEqual>
        child_set(child_results.begin(), child_results.end(),
                  kDefaultSetBucketCount,
                  ProtoPtrHash(work_space->GetPrimitiveHandler()),
                  ProtoPtrSameTypeAndEqual(work_space->GetPrimitiveHandler()));

    for (const auto& elem : first_param) {
      if (child_set.count(elem) > 0) {
        child_set.erase(elem);
        results->push_back(elem);
      }
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    if (child_->ReturnType() == nullptr ||
        params_[0]->ReturnType() == nullptr) {
      return nullptr;
    }

    if (AreSameMessageType(child_->ReturnType(), params_[0]->ReturnType())) {
      return child_->ReturnType();
    }

    // TODO: Consider refactoring ReturnType to return a set of all types
    // in the collection.
    return nullptr;
  }
};

// Converts decimal or integer container messages to a double value
static Status MessageToDouble(const PrimitiveHandler* primitive_handler,
                              const Message& message, double* value) {
  if (IsDecimal(message)) {
    FHIR_ASSIGN_OR_RETURN(std::string string_value,
                          primitive_handler->GetDecimalValue(message));

    if (!absl::SimpleAtod(string_value, value)) {
      return InvalidArgument(
          absl::StrCat("Could not convert to numeric: ", string_value));
    }

    return Status::OK();

  } else if (IsSystemInteger(message)) {
    FHIR_ASSIGN_OR_RETURN(*value, ToSystemInteger(primitive_handler, message));
    return Status::OK();
  }

  return InvalidArgument(
      absl::StrCat("Message type cannot be converted to double: ",
                   message.GetDescriptor()->full_name()));
}

class ComparisonOperator : public BinaryOperator {
 public:
  // Types of comparisons supported by this operator.
  enum ComparisonType {
    kLessThan,
    kGreaterThan,
    kLessThanEqualTo,
    kGreaterThanEqualTo
  };

  Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* out_results) const override {
    // Per the FHIRPath spec, comparison operators propagate empty results.
    if (left_results.empty() || right_results.empty()) {
      return Status::OK();
    }

    if (left_results.size() > 1 || right_results.size() > 1) {
      return InvalidArgument(
          "Comparison operators must have one element on each side.");
    }

    FHIR_ASSIGN_OR_RETURN(bool result, EvalComparison(
        work_space->GetPrimitiveHandler(), left_results[0], right_results[0]));

    Message* result_message =
        work_space->GetPrimitiveHandler()->NewBoolean(result);
    work_space->DeleteWhenFinished(result_message);
    out_results->push_back(WorkspaceMessage(result_message));
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }

  ComparisonOperator(std::shared_ptr<ExpressionNode> left,
                     std::shared_ptr<ExpressionNode> right,
                     ComparisonType comparison_type)
      : BinaryOperator(left, right), comparison_type_(comparison_type) {}

 private:
  StatusOr<bool> EvalComparison(const PrimitiveHandler* primitive_handler,
      const WorkspaceMessage& left, const WorkspaceMessage& right) const {
    const Message* left_result = left.Message();
    const Message* right_result = right.Message();

    if (IsSystemInteger(*left_result) && IsSystemInteger(*right_result)) {
      return EvalIntegerComparison(
          ToSystemInteger(primitive_handler, *left_result)
              .ValueOrDie(),
          ToSystemInteger(primitive_handler, *right_result)
              .ValueOrDie());
    } else if (IsDecimal(*left_result) || IsDecimal(*right_result)) {
      return EvalDecimalComparison(primitive_handler, left_result,
                                   right_result);

    } else if (IsString(*left_result) && IsString(*right_result)) {
      return EvalStringComparison(primitive_handler, left_result, right_result);
    } else if (IsDateTime(*left_result) && IsDateTime(*right_result)) {
      return EvalDateTimeComparison(primitive_handler, *left_result,
                                    *right_result);
    } else if (IsSimpleQuantity(*left_result) &&
               IsSimpleQuantity(*right_result)) {
      return EvalSimpleQuantityComparison(primitive_handler, *left_result,
                                          *right_result);
    } else {
      return InvalidArgument(
          "Unsupported comparison value types: ", left_result->GetTypeName(),
          " and ", right_result->GetTypeName());
    }
  }

  bool EvalIntegerComparison(int32_t left, int32_t right) const {
    switch (comparison_type_) {
      case kLessThan:
        return left < right;
      case kGreaterThan:
        return left > right;
      case kLessThanEqualTo:
        return left <= right;
      case kGreaterThanEqualTo:
        return left >= right;
    }
  }

  StatusOr<bool> EvalDecimalComparison(
      const PrimitiveHandler* primitive_handler, const Message* left_message,
      const Message* right_message) const {
    // Handle decimal comparisons, converting integer types
    // if necessary.
    double left;
    FHIR_RETURN_IF_ERROR(
        MessageToDouble(primitive_handler, *left_message, &left));
    double right;
    FHIR_RETURN_IF_ERROR(
        MessageToDouble(primitive_handler, *right_message, &right));

    switch (comparison_type_) {
      case kLessThan:
        return left < right;
      case kGreaterThan:
        return left > right;
      case kLessThanEqualTo:
        // Fallback to literal comparison for equality to avoid
        // rounding errors.
        return
            left <= right ||
            (left_message->GetDescriptor() == right_message->GetDescriptor() &&
             MessageDifferencer::Equals(*left_message, *right_message));
      case kGreaterThanEqualTo:
        // Fallback to literal comparison for equality to avoid
        // rounding errors.
        return
            left >= right ||
            (left_message->GetDescriptor() == right_message->GetDescriptor() &&
             MessageDifferencer::Equals(*left_message, *right_message));
    }

    return Status::OK();
  }

  StatusOr<bool> EvalStringComparison(const PrimitiveHandler* primitive_handler,
                                      const Message* left_message,
                                      const Message* right_message) const {
    FHIR_ASSIGN_OR_RETURN(const std::string left,
                          primitive_handler->GetStringValue(*left_message));
    FHIR_ASSIGN_OR_RETURN(const std::string right,
                          primitive_handler->GetStringValue(*right_message));

    // FHIR defines string comparisons to be based on unicode values,
    // so simply comparison operators are not sufficient.
    static const std::locale locale("en_US.UTF-8");

    static const std::collate<char>& coll =
        std::use_facet<std::collate<char>>(locale);

    int compare_result =
        coll.compare(left.data(), left.data() + left.length(), right.data(),
                     right.data() + right.length());

    switch (comparison_type_) {
      case kLessThan:
        return compare_result < 0;
      case kGreaterThan:
        return compare_result > 0;
      case kLessThanEqualTo:
        return compare_result <= 0;
      case kGreaterThanEqualTo:
        return compare_result >= 0;
    }
  }

  StatusOr<bool> EvalDateTimeComparison(
      const PrimitiveHandler* primitive_handler, const Message& left_message,
      const Message& right_message) const {
    FHIR_ASSIGN_OR_RETURN(absl::Time left_time,
                          primitive_handler->GetDateTimeValue(left_message));
    FHIR_ASSIGN_OR_RETURN(absl::Time right_time,
                          primitive_handler->GetDateTimeValue(right_message));

    FHIR_ASSIGN_OR_RETURN(absl::TimeZone left_zone,
                          primitive_handler->GetDateTimeZone(left_message));
    FHIR_ASSIGN_OR_RETURN(absl::TimeZone right_zone,
                          primitive_handler->GetDateTimeZone(right_message));

    FHIR_ASSIGN_OR_RETURN(
        DateTimePrecision left_precision,
        primitive_handler->GetDateTimePrecision(left_message));
    FHIR_ASSIGN_OR_RETURN(
        DateTimePrecision right_precision,
        primitive_handler->GetDateTimePrecision(right_message));

    // negative if left < right, positive if left > right, 0 if equal
    absl::civil_diff_t time_difference;

    // The FHIRPath spec (http://hl7.org/fhirpath/#comparison) states that
    // datetime comparison is done at the finest precision BOTH
    // dates support. This is equivalent to finding the looser precision
    // between the two and comparing them, which is simpler to implement here.
    if (left_precision == DateTimePrecision::kYear ||
        right_precision == DateTimePrecision::kYear) {
      absl::CivilYear left_year = absl::ToCivilYear(left_time, left_zone);
      absl::CivilYear right_year = absl::ToCivilYear(right_time, right_zone);
      time_difference = left_year - right_year;

    } else if (left_precision == DateTimePrecision::kMonth ||
               right_precision == DateTimePrecision::kMonth) {
      absl::CivilMonth left_month = absl::ToCivilMonth(left_time, left_zone);
      absl::CivilMonth right_month = absl::ToCivilMonth(right_time, right_zone);
      time_difference = left_month - right_month;

    } else if (left_precision == DateTimePrecision::kDay ||
               right_precision == DateTimePrecision::kDay) {
      absl::CivilDay left_day = absl::ToCivilDay(left_time, left_zone);
      absl::CivilDay right_day = absl::ToCivilDay(right_time, right_zone);
      time_difference = left_day - right_day;

    } else if (left_precision == DateTimePrecision::kSecond ||
               right_precision == DateTimePrecision::kSecond) {
      absl::CivilSecond left_second = absl::ToCivilSecond(left_time, left_zone);
      absl::CivilSecond right_second =
          absl::ToCivilSecond(right_time, right_zone);
      time_difference = left_second - right_second;
    } else {
      // Abseil does not support sub-second civil time precision, so we handle
      // them by first comparing seconds (to resolve timezone differences)
      // and then comparing the sub-second component if the seconds are
      // equal.
      absl::CivilSecond left_second = absl::ToCivilSecond(left_time, left_zone);
      absl::CivilSecond right_second =
          absl::ToCivilSecond(right_time, right_zone);
      time_difference = left_second - right_second;

      // In the same second, so check for sub-second differences.
      if (time_difference == 0) {
        time_difference = absl::ToUnixMicros(left_time) % 1000000 -
                          absl::ToUnixMicros(right_time) % 1000000;
      }
    }

    switch (comparison_type_) {
      case kLessThan:
        return time_difference < 0;
      case kGreaterThan:
        return time_difference > 0;
      case kLessThanEqualTo:
        return time_difference <= 0;
      case kGreaterThanEqualTo:
        return time_difference >= 0;
    }
  }

  StatusOr<bool> EvalSimpleQuantityComparison(
      const PrimitiveHandler* primitive_handler,
      const Message& left_wrapper,
      const Message& right_wrapper) const {
    FHIR_ASSIGN_OR_RETURN(
        std::string left_code,
        primitive_handler->GetSimpleQuantityCode(left_wrapper));
    FHIR_ASSIGN_OR_RETURN(
        std::string left_system,
        primitive_handler->GetSimpleQuantitySystem(left_wrapper));
    FHIR_ASSIGN_OR_RETURN(
        std::string right_code,
        primitive_handler->GetSimpleQuantityCode(right_wrapper));
    FHIR_ASSIGN_OR_RETURN(
        std::string right_system,
        primitive_handler->GetSimpleQuantitySystem(right_wrapper));

    if (left_code != right_code || left_system != right_system) {
      // From the FHIRPath spec: "Implementations are not required to fully
      // support operations on units, but they must at least respect units,
      // recognizing when units differ."
      return InvalidArgument(
          "Compared quantities must have the same units. Got ",
          "[", left_code, ", ", left_system, "] and ",
          "[", right_code, ", ", right_system, "]");
    }

    FHIR_ASSIGN_OR_RETURN(
        std::string left_value,
        primitive_handler->GetSimpleQuantityValue(left_wrapper));
    FHIR_ASSIGN_OR_RETURN(
        std::string right_value,
        primitive_handler->GetSimpleQuantityValue(right_wrapper));

    auto left_value_wrapper =
        std::unique_ptr<Message>(primitive_handler->NewDecimal(left_value));
    auto right_value_wrapper =
        std::unique_ptr<Message>(primitive_handler->NewDecimal(right_value));

    return EvalDecimalComparison(primitive_handler, left_value_wrapper.get(),
                                 right_value_wrapper.get());
  }

  ComparisonType comparison_type_;
};

// Implementation for FHIRPath's addition operator.
class AdditionOperator : public BinaryOperator {
 public:
  Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* out_results) const override {
    // Per the FHIRPath spec, comparison operators propagate empty results.
    if (left_results.empty() || right_results.empty()) {
      return Status::OK();
    }

    if (left_results.size() > 1 || right_results.size() > 1) {
      return InvalidArgument(
          "Addition operators must have one element on each side.");
    }

    const Message* left_result = left_results[0].Message();
    const Message* right_result = right_results[0].Message();

    if (IsSystemInteger(*left_result) && IsSystemInteger(*right_result)) {
      FHIR_ASSIGN_OR_RETURN(
          int32_t value, EvalIntegerAddition(work_space->GetPrimitiveHandler(),
                                             *left_result, *right_result));
      Message* result = work_space->GetPrimitiveHandler()->NewInteger(value);
      work_space->DeleteWhenFinished(result);
      out_results->push_back(WorkspaceMessage(result));
    } else if (IsString(*left_result) && IsString(*right_result)) {
      FHIR_ASSIGN_OR_RETURN(
          std::string value,
          EvalStringAddition(work_space->GetPrimitiveHandler(), *left_result,
                             *right_result));
      Message* result = work_space->GetPrimitiveHandler()->NewString(value);
      work_space->DeleteWhenFinished(result);
      out_results->push_back(WorkspaceMessage(result));
    } else {
      // TODO: Add implementation for Date, DateTime, Time, and Decimal
      // addition.
      return InvalidArgument(absl::StrCat("Addition not supported for ",
                                          left_result->GetTypeName(), " and ",
                                          right_result->GetTypeName()));
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override { return left_->ReturnType(); }

  AdditionOperator(std::shared_ptr<ExpressionNode> left,
                   std::shared_ptr<ExpressionNode> right)
      : BinaryOperator(std::move(left), std::move(right)) {}

 private:
  StatusOr<int32_t> EvalIntegerAddition(
      const PrimitiveHandler* primitive_handler, const Message& left_wrapper,
      const Message& right_wrapper) const {
    FHIR_ASSIGN_OR_RETURN(int32_t left,
                          ToSystemInteger(primitive_handler, left_wrapper));
    FHIR_ASSIGN_OR_RETURN(int32_t right,
                          ToSystemInteger(primitive_handler, right_wrapper));
    return left + right;
  }

  StatusOr<std::string> EvalStringAddition(
      const PrimitiveHandler* primitive_handler, const Message& left_message,
      const Message& right_message) const {
    FHIR_ASSIGN_OR_RETURN(std::string left,
                          primitive_handler->GetStringValue(left_message));
    FHIR_ASSIGN_OR_RETURN(std::string right,
                          primitive_handler->GetStringValue(right_message));

    return absl::StrCat(left, right);
  }
};

// Implementation for FHIRPath's string concatenation operator (&).
class StrCatOperator : public BinaryOperator {
 public:
  Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* out_results) const override {
    if (left_results.size() > 1 || right_results.size() > 1) {
      return InvalidArgument(
          "String concatenation operators must have one element on each side.");
    }

    std::string left;
    std::string right;

    if (!left_results.empty()) {
      FHIR_ASSIGN_OR_RETURN(
          left,
          MessageToString(work_space->GetPrimitiveHandler(), left_results[0]));
    }
    if (!right_results.empty()) {
      FHIR_ASSIGN_OR_RETURN(
          right,
          MessageToString(work_space->GetPrimitiveHandler(), right_results[0]));
    }

    Message* result =
        work_space->GetPrimitiveHandler()->NewString(absl::StrCat(left, right));
    work_space->DeleteWhenFinished(result);
    out_results->push_back(WorkspaceMessage(result));
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return String::GetDescriptor();
  }

  StrCatOperator(std::shared_ptr<ExpressionNode> left,
                 std::shared_ptr<ExpressionNode> right)
      : BinaryOperator(std::move(left), std::move(right)) {}
};

class PolarityOperator : public ExpressionNode {
 public:
  // Supported polarity operations.
  enum PolarityOperation {
    kPositive,
    kNegative,
  };

  PolarityOperator(PolarityOperation operation,
                   const std::shared_ptr<ExpressionNode>& operand)
      : operation_(operation), operand_(operand) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> operand_result;
    FHIR_RETURN_IF_ERROR(operand_->Evaluate(work_space, &operand_result));

    if (operand_result.size() > 1) {
      return InvalidArgument(
          "Polarity operators must operate on a single element.");
    }

    if (operand_result.empty()) {
      return Status::OK();
    }

    const WorkspaceMessage& operand_value = operand_result[0];

    if (operation_ == kPositive) {
      results->push_back(operand_value);
      return Status::OK();
    }

    if (IsDecimal(*operand_value.Message())) {
      FHIR_ASSIGN_OR_RETURN(std::string value,
                            work_space->GetPrimitiveHandler()->GetDecimalValue(
                                *operand_value.Message()));
      value = absl::StartsWith(value, "-") ? value.substr(1)
                                           : absl::StrCat("-", value);
      Message* result = work_space->GetPrimitiveHandler()->NewDecimal(value);
      work_space->DeleteWhenFinished(result);
      results->push_back(WorkspaceMessage(result));
      return Status::OK();
    }

    if (IsSystemInteger(*operand_value.Message())) {
      FHIR_ASSIGN_OR_RETURN(int32_t value,
                            ToSystemInteger(work_space->GetPrimitiveHandler(),
                                            *operand_value.Message()));
      Message* result =
          work_space->GetPrimitiveHandler()->NewInteger(value * -1);
      work_space->DeleteWhenFinished(result);
      results->push_back(WorkspaceMessage(result));
      return Status::OK();
    }

    return InvalidArgument(
        "Polarity operators must operate on a decimal or integer type.");
  }

  const Descriptor* ReturnType() const override {
    return operand_->ReturnType();
  }

 private:
  const PolarityOperation operation_;
  const std::shared_ptr<ExpressionNode> operand_;
};

// Base class for FHIRPath binary boolean operators.
class BooleanOperator : public ExpressionNode {
 public:
  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }

  BooleanOperator(std::shared_ptr<ExpressionNode> left,
                  std::shared_ptr<ExpressionNode> right)
      : left_(left), right_(right) {}

 protected:
  void SetResult(bool eval_result, WorkSpace* work_space,
                 std::vector<WorkspaceMessage>* results) const {
    Message* result =
        work_space->GetPrimitiveHandler()->NewBoolean(eval_result);
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
  }

  StatusOr<absl::optional<bool>> EvaluateBooleanNode(
      std::shared_ptr<ExpressionNode> node, WorkSpace* work_space) const {
    std::vector<WorkspaceMessage> results;
    FHIR_RETURN_IF_ERROR(node->Evaluate(work_space, &results));
    FHIR_ASSIGN_OR_RETURN(
        absl::optional<bool> result,
        (BooleanOrEmpty(work_space->GetPrimitiveHandler(), results)));
    return result;
  }

  const std::shared_ptr<ExpressionNode> left_;
  const std::shared_ptr<ExpressionNode> right_;
};

// Implements logic for the "implies" operator. Logic may be found in
// section 6.5.4 at http://hl7.org/fhirpath/#boolean-logic
class ImpliesOperator : public BooleanOperator {
 public:
  ImpliesOperator(std::shared_ptr<ExpressionNode> left,
                  std::shared_ptr<ExpressionNode> right)
      : BooleanOperator(left, right) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> left_result,
                          EvaluateBooleanNode(left_, work_space));

    // Short circuit evaluation when left_result == "false"
    if (left_result.has_value() && !left_result.value()) {
      SetResult(true, work_space, results);
      return Status::OK();
    }

    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> right_result,
                          EvaluateBooleanNode(right_, work_space));

    if (!left_result.has_value()) {
      if (right_result.value_or(false)) {
        SetResult(true, work_space, results);
      }
    } else if (right_result.has_value()) {
      SetResult(right_result.value(), work_space, results);
    }

    return Status::OK();
  }
};

class XorOperator : public BooleanOperator {
 public:
  XorOperator(std::shared_ptr<ExpressionNode> left,
              std::shared_ptr<ExpressionNode> right)
      : BooleanOperator(left, right) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    // Logic from truth table spec: http://hl7.org/fhirpath/#boolean-logic
    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> left_result,
                          EvaluateBooleanNode(left_, work_space));
    if (!left_result.has_value()) {
      return Status::OK();
    }

    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> right_result,
                          EvaluateBooleanNode(right_, work_space));
    if (!right_result.has_value()) {
      return Status::OK();
    }

    SetResult(left_result.value() != right_result.value(), work_space, results);
    return Status::OK();
  }
};

class OrOperator : public BooleanOperator {
 public:
  OrOperator(std::shared_ptr<ExpressionNode> left,
             std::shared_ptr<ExpressionNode> right)
      : BooleanOperator(left, right) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    // Logic from truth table spec: http://hl7.org/fhirpath/#boolean-logic
    // Short circuit and return true on the first true result.
    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> left_result,
                          EvaluateBooleanNode(left_, work_space));
    if (left_result.has_value() && left_result.value()) {
      SetResult(true, work_space, results);
      return Status::OK();
    }

    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> right_result,
                          EvaluateBooleanNode(right_, work_space));
    if (right_result.has_value() && right_result.value()) {
      SetResult(true, work_space, results);
      return Status::OK();
    }

    if (left_result.has_value() && right_result.has_value()) {
      // Both children must be false to get here, so return false.
      SetResult(false, work_space, results);
      return Status::OK();
    }

    // Neither child is true and at least one is empty, so propagate
    // empty per the FHIRPath spec.
    return Status::OK();
  }
};

class AndOperator : public BooleanOperator {
 public:
  AndOperator(std::shared_ptr<ExpressionNode> left,
              std::shared_ptr<ExpressionNode> right)
      : BooleanOperator(left, right) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    // Logic from truth table spec: http://hl7.org/fhirpath/#boolean-logic
    // Short circuit and return false on the first false result.
    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> left_result,
                          EvaluateBooleanNode(left_, work_space));
    if (left_result.has_value() && !left_result.value()) {
      SetResult(false, work_space, results);
      return Status::OK();
    }

    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> right_result,
                          EvaluateBooleanNode(right_, work_space));
    if (right_result.has_value() && !right_result.value()) {
      SetResult(false, work_space, results);
      return Status::OK();
    }

    if (left_result.has_value() && right_result.has_value()) {
      // Both children must be true to get here, so return true.
      SetResult(true, work_space, results);
      return Status::OK();
    }

    // Neither child is false and at least one is empty, so propagate
    // empty per the FHIRPath spec.
    return Status::OK();
  }
};

// Implements the "contain" operator. This may also be used for the "in"
// operator by switching the left and right operands.
//
// See https://hl7.org/fhirpath/#collections-2
class ContainsOperator : public BinaryOperator {
 public:
  ContainsOperator(std::shared_ptr<ExpressionNode> left,
                  std::shared_ptr<ExpressionNode> right)
      : BinaryOperator(std::move(left), std::move(right)) {}

  Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* results) const override {
    if (right_results.empty()) {
      return Status::OK();
    }

    if (right_results.size() > 1) {
      return InvalidArgument(
          "in/contains must have one or fewer items in the left/right "
          "operand.");
    }

    const Message* right_operand = right_results[0].Message();

    bool found = std::any_of(left_results.begin(), left_results.end(),
                             [=](const WorkspaceMessage& message) {
                               return EqualsOperator::AreEqual(
                                   work_space->GetPrimitiveHandler(),
                                   *right_operand, *message.Message());
                             });

    Message* result = work_space->GetPrimitiveHandler()->NewBoolean(found);
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }
};

// Expression node for a reference to $this.
class ThisReference : public ExpressionNode {
 public:
  explicit ThisReference(const Descriptor* descriptor)
      : descriptor_(descriptor){}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    results->push_back(work_space->MessageContext());
    return Status::OK();
  }

  const Descriptor* ReturnType() const override { return descriptor_; }

 private:
  const Descriptor* descriptor_;
};

// Expression node for a reference to %context.
class ContextReference : public ExpressionNode {
 public:
  explicit ContextReference(const Descriptor* descriptor)
      : descriptor_(descriptor) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    results->push_back(work_space->BottomMessageContext());
    return Status::OK();
  }

  const Descriptor* ReturnType() const override { return descriptor_; }

 private:
  const Descriptor* descriptor_;
};

// Expression node for a reference to %resource.
class ResourceReference : public ExpressionNode {
 public:
  Status Evaluate(WorkSpace* work_space,
                  std::vector<WorkspaceMessage>* results) const override {
    FHIR_ASSIGN_OR_RETURN(WorkspaceMessage result,
                          work_space->MessageContext().NearestResource());
    results->push_back(result);
    return Status::OK();
  }

  // TODO: Track %resource type during compilation.
  const Descriptor* ReturnType() const override { return nullptr; }
};

// Produces a shared pointer explicitly of ExpressionNode rather
// than a subclass to work well with ANTLR's "Any" semantics.
inline std::shared_ptr<ExpressionNode> ToAny(
    std::shared_ptr<ExpressionNode> node) {
  return node;
}

// Internal structure that defines an invocation. This is used
// at points when visiting the AST that do not have enough context
// to produce an ExpressionNode (e.g., they do not see the type of
// the calling object), and is a placeholder for a higher-level
// visitor to transform into an ExpressionNode
struct InvocationDefinition {
  InvocationDefinition(const std::string& name, const bool is_function)
      : name(name), is_function(is_function) {}

  InvocationDefinition(
      const std::string& name, const bool is_function,
      const std::vector<FhirPathParser::ExpressionContext*>& params)
      : name(name), is_function(is_function), params(params) {}

  const std::string name;

  // Indicates it is a function invocation rather than a member lookup.
  const bool is_function;

  const std::vector<FhirPathParser::ExpressionContext*> params;
};

// ANTLR Visitor implementation to translate the AST
// into ExpressionNodes that can run the expression over
// given protocol buffers.
//
// Note that the return value of antlrcpp::Any assumes returned values
// have copy constructors, which means we cannot use std::unique_ptr even
// if that's the most logical choice. Therefore we'll use std:shared_ptr
// more frequently here, but the costs in this case are negligible.
class FhirPathCompilerVisitor : public FhirPathBaseVisitor {
 public:
  FhirPathCompilerVisitor(const Descriptor* descriptor,
                          const PrimitiveHandler* primitive_handler)
      : error_listener_(this),
        descriptor_stack_({descriptor}),
        primitive_handler_(primitive_handler) {}

  FhirPathCompilerVisitor(
      const std::vector<const Descriptor*>& descriptor_stack_history,
      const Descriptor* descriptor, const PrimitiveHandler* primitive_handler)
      : error_listener_(this),
        descriptor_stack_(descriptor_stack_history),
        primitive_handler_(primitive_handler) {
    descriptor_stack_.push_back(descriptor);
  }

  antlrcpp::Any visitInvocationExpression(
      FhirPathParser::InvocationExpressionContext* node) override {
    antlrcpp::Any expression = node->children[0]->accept(this);

    // This could be a simple member name or a parameterized function...
    antlrcpp::Any invocation = node->children[2]->accept(this);

    if (!CheckOk()) {
      return nullptr;
    }

    auto definition = invocation.as<std::shared_ptr<InvocationDefinition>>();

    std::shared_ptr<ExpressionNode> expr =
        expression.as<std::shared_ptr<ExpressionNode>>();

    if (definition->is_function) {
      auto function_node =
          createFunction(definition->name, expr, definition->params);

      if (function_node == nullptr || !CheckOk()) {
        return nullptr;
      } else {
        return ToAny(function_node);
      }
    } else {
      const Descriptor* descriptor = expr->ReturnType();

      // If we know the return type of the expression, and the return type
      // doesn't have the referenced field, set an error and return.
      if (descriptor != nullptr && !HasFieldWithJsonName(descriptor,
                                                         definition->name)) {
        SetError(absl::StrCat("Unable to find field ", definition->name));
        return nullptr;
      }

      const FieldDescriptor* field =
          descriptor != nullptr
              ? FindFieldByJsonName(descriptor, definition->name)
              : nullptr;
      return ToAny(std::make_shared<InvokeExpressionNode>(expression, field,
                                                          definition->name));
    }
  }

  antlrcpp::Any visitInvocationTerm(
      FhirPathParser::InvocationTermContext* ctx) override {
    antlrcpp::Any invocation = visitChildren(ctx);

    if (!CheckOk()) {
      return nullptr;
    }

    if (invocation.is<std::shared_ptr<ExpressionNode>>()) {
      return invocation;
    }

    auto definition = invocation.as<std::shared_ptr<InvocationDefinition>>();

    if (definition->is_function) {
      auto function_node = createFunction(
          definition->name,
          std::make_shared<ThisReference>(descriptor_stack_.back()),
          definition->params);

      return function_node == nullptr || !CheckOk() ? nullptr
                                                    : ToAny(function_node);
    }

    const Descriptor* descriptor = descriptor_stack_.back();

    // If we know the return type of the expression, and the return type
    // doesn't have the referenced field, set an error and return.
    if (descriptor != nullptr && !HasFieldWithJsonName(descriptor,
                                                       definition->name)) {
      SetError(absl::StrCat("Unable to find field ", definition->name));
      return nullptr;
    }

    const FieldDescriptor* field =
        descriptor != nullptr
            ? FindFieldByJsonName(descriptor, definition->name)
            : nullptr;
    return ToAny(std::make_shared<InvokeTermNode>(field, definition->name));
  }

  antlrcpp::Any visitIndexerExpression(
      FhirPathParser::IndexerExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!CheckOk()) {
      return nullptr;
    }

    auto left = left_any.as<std::shared_ptr<ExpressionNode>>();
    auto right = right_any.as<std::shared_ptr<ExpressionNode>>();

    return ToAny(
        std::make_shared<IndexerExpression>(primitive_handler_, left, right));
  }

  antlrcpp::Any visitUnionExpression(
      FhirPathParser::UnionExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!CheckOk()) {
      return nullptr;
    }

    auto left = left_any.as<std::shared_ptr<ExpressionNode>>();
    auto right = right_any.as<std::shared_ptr<ExpressionNode>>();

    return ToAny(std::make_shared<UnionOperator>(left, right));
  }

  antlrcpp::Any visitAdditiveExpression(
      FhirPathParser::AdditiveExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    std::string op = ctx->children[1]->getText();
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!CheckOk()) {
      return nullptr;
    }

    auto left = left_any.as<std::shared_ptr<ExpressionNode>>();
    auto right = right_any.as<std::shared_ptr<ExpressionNode>>();

    if (op == "+") {
      return ToAny(std::make_shared<AdditionOperator>(left, right));
    }

    if (op == "&") {
      return ToAny(std::make_shared<StrCatOperator>(left, right));
    }

    // TODO: Support "-"

    SetError(absl::StrCat("Unsupported additive operator: ", op));
    return nullptr;
  }

  antlrcpp::Any visitPolarityExpression(
      FhirPathParser::PolarityExpressionContext* ctx) override {
    std::string op = ctx->children[0]->getText();
    antlrcpp::Any operand_any = ctx->children[1]->accept(this);

    if (!CheckOk()) {
      return nullptr;
    }

    auto operand = operand_any.as<std::shared_ptr<ExpressionNode>>();

    if (op == "+") {
      return ToAny(std::make_shared<PolarityOperator>(
          PolarityOperator::kPositive, operand));
    }

    if (op == "-") {
      return ToAny(std::make_shared<PolarityOperator>(
          PolarityOperator::kNegative, operand));
    }

    SetError(absl::StrCat("Unsupported polarity operator: ", op));
    return nullptr;
  }

  antlrcpp::Any visitTypeExpression(
      FhirPathParser::TypeExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    std::string op = ctx->children[1]->getText();
    std::string type = ctx->children[2]->getText();

    if (!CheckOk()) {
      return nullptr;
    }

    auto left = left_any.as<std::shared_ptr<ExpressionNode>>();

    if (op == "is") {
      return ToAny(std::make_shared<IsFunction>(left, type));
    }

    if (op == "as") {
      return ToAny(std::make_shared<AsFunction>(left, type));
    }

    SetError(absl::StrCat("Unsupported type operator: ", op));
    return nullptr;
  }

  antlrcpp::Any visitEqualityExpression(
      FhirPathParser::EqualityExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    std::string op = ctx->children[1]->getText();
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!CheckOk()) {
      return nullptr;
    }

    auto left = left_any.as<std::shared_ptr<ExpressionNode>>();
    auto right = right_any.as<std::shared_ptr<ExpressionNode>>();

    if (op == "=") {
      return ToAny(std::make_shared<EqualsOperator>(left, right));
    }
    if (op == "!=") {
      // Negate the equals function to implement !=
      auto equals_op = std::make_shared<EqualsOperator>(left, right);
      return ToAny(std::make_shared<NotFunction>(equals_op));
    }

    SetError(absl::StrCat("Unsupported equality operator: ", op));
    return nullptr;
  }

  antlrcpp::Any visitInequalityExpression(
      FhirPathParser::InequalityExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    std::string op = ctx->children[1]->getText();
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!CheckOk()) {
      return nullptr;
    }

    auto left = left_any.as<std::shared_ptr<ExpressionNode>>();
    auto right = right_any.as<std::shared_ptr<ExpressionNode>>();

    ComparisonOperator::ComparisonType op_type;

    if (op == "<") {
      op_type = ComparisonOperator::kLessThan;
    } else if (op == ">") {
      op_type = ComparisonOperator::kGreaterThan;
    } else if (op == "<=") {
      op_type = ComparisonOperator::kLessThanEqualTo;
    } else if (op == ">=") {
      op_type = ComparisonOperator::kGreaterThanEqualTo;
    } else {
      SetError(absl::StrCat("Unsupported comparison operator: ", op));
      return nullptr;
    }

    return ToAny(std::make_shared<ComparisonOperator>(left, right, op_type));
  }

  antlrcpp::Any visitMembershipExpression(
      FhirPathParser::MembershipExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    std::string op = ctx->children[1]->getText();
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!CheckOk()) {
      return nullptr;
    }

    auto left = left_any.as<std::shared_ptr<ExpressionNode>>();
    auto right = right_any.as<std::shared_ptr<ExpressionNode>>();

    if (op == "in") {
      return ToAny(std::make_shared<ContainsOperator>(right, left));
    } else if (op == "contains") {
      return ToAny(std::make_shared<ContainsOperator>(left, right));
    }

    SetError(absl::StrCat("Unsupported membership operator: ", op));
    return nullptr;
  }

  antlrcpp::Any visitMemberInvocation(
      FhirPathParser::MemberInvocationContext* ctx) override {
    std::string text = ctx->identifier()->IDENTIFIER()->getSymbol()->getText();
    return std::make_shared<InvocationDefinition>(text, false);
  }

  antlrcpp::Any visitFunctionInvocation(
      FhirPathParser::FunctionInvocationContext* ctx) override {
    if (!CheckOk()) {
      return nullptr;
    }

    std::string text = ctx->function()->identifier()->getText();
    std::vector<FhirPathParser::ExpressionContext*> params;
    if (ctx->function()->paramList()) {
      params = ctx->function()->paramList()->expression();
    }

    return std::make_shared<InvocationDefinition>(text, true, params);
  }

  antlrcpp::Any visitImpliesExpression(
      FhirPathParser::ImpliesExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!CheckOk()) {
      return nullptr;
    }

    auto left = left_any.as<std::shared_ptr<ExpressionNode>>();
    auto right = right_any.as<std::shared_ptr<ExpressionNode>>();

    return ToAny(std::make_shared<ImpliesOperator>(left, right));
  }

  antlrcpp::Any visitOrExpression(
      FhirPathParser::OrExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    std::string op = ctx->children[1]->getText();
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!CheckOk()) {
      return nullptr;
    }

    auto left = left_any.as<std::shared_ptr<ExpressionNode>>();
    auto right = right_any.as<std::shared_ptr<ExpressionNode>>();

    return op == "or"
        ? ToAny(std::make_shared<OrOperator>(left, right))
        : ToAny(std::make_shared<XorOperator>(left, right));
  }

  antlrcpp::Any visitAndExpression(
      FhirPathParser::AndExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!CheckOk()) {
      return nullptr;
    }

    auto left = left_any.as<std::shared_ptr<ExpressionNode>>();
    auto right = right_any.as<std::shared_ptr<ExpressionNode>>();

    return ToAny(std::make_shared<AndOperator>(left, right));
  }

  antlrcpp::Any visitParenthesizedTerm(
      FhirPathParser::ParenthesizedTermContext* ctx) override {
    // Simply propagate the value of the parenthesized term.
    return ctx->children[1]->accept(this);
  }

  antlrcpp::Any visitThisInvocation(
      FhirPathParser::ThisInvocationContext* ctx) override {
    return ToAny(std::make_shared<ThisReference>(descriptor_stack_.back()));
  }

  antlrcpp::Any visitExternalConstant(
      FhirPathParser::ExternalConstantContext* ctx) override {
    std::string name = ctx->children[1]->getText();
    const PrimitiveHandler* primitive_handler = primitive_handler_;
    if (name == "ucum") {
      return ToAny(std::make_shared<Literal<std::string>>(
          "http://unitsofmeasure.org", primitive_handler_->StringDescriptor(),
          [primitive_handler](const std::string& value) {
            return primitive_handler->NewString(value);
          }));
    } else if (name == "sct") {
      return ToAny(std::make_shared<Literal<std::string>>(
          "http://snomed.info/sct", primitive_handler_->StringDescriptor(),
          [primitive_handler](const std::string& value) {
            return primitive_handler->NewString(value);
          }));
    } else if (name == "loinc") {
      return ToAny(std::make_shared<Literal<std::string>>(
          "http://loinc.org", primitive_handler_->StringDescriptor(),
          [primitive_handler](const std::string& value) {
            return primitive_handler->NewString(value);
          }));
    } else if (name == "context") {
      return ToAny(
          std::make_shared<ContextReference>(descriptor_stack_.front()));
    } else if (name == "resource") {
      return ToAny(std::make_shared<ResourceReference>());
    }

    SetError(absl::StrCat("Unknown external constant: ", name));
    return nullptr;
  }

  antlrcpp::Any visitTerminal(TerminalNode* node) override {
    const std::string& text = node->getSymbol()->getText();
    const PrimitiveHandler* primitive_handler = primitive_handler_;

    switch (node->getSymbol()->getType()) {
      case FhirPathLexer::NUMBER:
        // Determine if the number is an integer or decimal, propagating
        // decimal types in string form to preserve precision.
        if (text.find(".") != std::string::npos) {
          return ToAny(std::make_shared<Literal<std::string>>(
              text, primitive_handler_->DecimalDescriptor(),
              [primitive_handler](const std::string& value) {
                return primitive_handler->NewDecimal(value);
              }));
        } else {
          int32_t value;
          if (!absl::SimpleAtoi(text, &value)) {
            SetError(absl::StrCat("Malformed integer ", text));
            return nullptr;
          }

          return ToAny(std::make_shared<Literal<int32_t>>(
              value, primitive_handler_->IntegerDescriptor(),
              [primitive_handler](int32_t value) {
                return primitive_handler->NewInteger(value);
              }));
        }

      case FhirPathLexer::STRING: {
        // The lexer keeps the quotes around string literals,
        // so we remove them here. The following assert simply reflects
        // the lexer's guarantees as defined.
        assert(text.length() >= 2);
        const std::string& trimmed = text.substr(1, text.length() - 2);
        std::string unescaped;
        // CUnescape handles additional escape sequences not allowed by
        // FHIRPath. However, these additional sequences are disallowed by the
        // grammar rules (FhirPath.g4) which are enforced by the parser. In
        // addition, CUnescape does not handle escaped forward slashes.
        absl::CUnescape(trimmed, &unescaped);
        return ToAny(std::make_shared<Literal<std::string>>(
            unescaped, primitive_handler_->StringDescriptor(),
            [primitive_handler](const std::string& value) {
              return primitive_handler->NewString(value);
            }));
      }

      case FhirPathLexer::BOOL:
        return ToAny(std::make_shared<Literal<bool>>(
            text == "true", primitive_handler_->BooleanDescriptor(),
            [primitive_handler](bool value) {
              return primitive_handler->NewBoolean(value);
            }));

      case FhirPathLexer::EMPTY:
        return ToAny(std::make_shared<EmptyLiteral>());

      default:

        SetError(absl::StrCat("Unknown terminal type: ", text));
        return nullptr;
    }
  }

  antlrcpp::Any defaultResult() override { return nullptr; }

  bool CheckOk() { return error_message_.empty(); }

  std::string GetError() { return error_message_; }

  BaseErrorListener* GetErrorListener() { return &error_listener_; }

 private:
  typedef std::function<StatusOr<ExpressionNode*>(
      std::shared_ptr<ExpressionNode>,
      const std::vector<FhirPathParser::ExpressionContext*>&,
      FhirPathBaseVisitor*, FhirPathBaseVisitor*)>
      FunctionFactory;

  std::map<std::string, FunctionFactory> function_map_{
      {"exists", FunctionNode::Create<ExistsFunction>},
      {"not", FunctionNode::Create<NotFunction>},
      {"hasValue", FunctionNode::Create<HasValueFunction>},
      {"startsWith", FunctionNode::Create<StartsWithFunction>},
      {"contains", FunctionNode::Create<ContainsFunction>},
      {"empty", FunctionNode::Create<EmptyFunction>},
      {"first", FunctionNode::Create<FirstFunction>},
      {"tail", FunctionNode::Create<TailFunction>},
      {"trace", FunctionNode::Create<TraceFunction>},
      {"toInteger", FunctionNode::Create<ToIntegerFunction>},
      {"count", FunctionNode::Create<CountFunction>},
      {"combine", FunctionNode::Create<CombineFunction>},
      {"distinct", FunctionNode::Create<DistinctFunction>},
      {"matches", FunctionNode::Create<MatchesFunction>},
      {"replaceMatches", FunctionNode::Create<ReplaceMatchesFunction>},
      {"length", FunctionNode::Create<LengthFunction>},
      {"isDistinct", FunctionNode::Create<IsDistinctFunction>},
      {"intersect", FunctionNode::Create<IntersectFunction>},
      {"where", FunctionNode::Create<WhereFunction>},
      {"select", FunctionNode::Create<SelectFunction>},
      {"all", FunctionNode::Create<AllFunction>},
      {"toString", FunctionNode::Create<ToStringFunction>},
      {"iif", FunctionNode::Create<IifFunction>},
      {"is", IsFunction::Create},
      {"as", AsFunction::Create},
      {"children", FunctionNode::Create<ChildrenFunction>},
      {"descendants", FunctionNode::Create<DescendantsFunction>},
  };

  // Returns an ExpressionNode that implements the specified FHIRPath function.
  std::shared_ptr<ExpressionNode> createFunction(
      const std::string& function_name,
      std::shared_ptr<ExpressionNode> child_expression,
      const std::vector<FhirPathParser::ExpressionContext*>& params) {
    std::map<std::string, FunctionFactory>::iterator function_factory =
        function_map_.find(function_name);
    if (function_factory != function_map_.end()) {
      // Some functions accept parameters that are expressions evaluated using
      // the child expression's result as context, not the base context of the
      // FHIRPath expression. In order to compile such parameters, we need to
      // visit it with the child expression's type and not the base type of the
      // current visitor. Therefore, both the current visitor and a visitor with
      // the child expression as the context are provided. The function factory
      // will use whichever visitor (or both) is needed to compile the function
      // invocation.
      FhirPathCompilerVisitor child_context_visitor(
          descriptor_stack_, child_expression->ReturnType(),
          primitive_handler_);
      StatusOr<ExpressionNode*> result = function_factory->second(
          child_expression, params, this, &child_context_visitor);
      if (!result.ok()) {
        this->SetError(absl::StrCat(
            "Failed to compile call to ", function_name,
            "(): ", result.status().error_message(),
            !child_context_visitor.CheckOk()
                ? absl::StrCat("; ", child_context_visitor.GetError())
                : ""));
        return std::shared_ptr<ExpressionNode>(nullptr);
      }

      return std::shared_ptr<ExpressionNode>(result.ValueOrDie());
    } else {
      // TODO: Implement set of functions for initial use cases.
      SetError(absl::StrCat("The function ", function_name,
                            " is not yet implemented"));

      return std::shared_ptr<ExpressionNode>(nullptr);
    }
  }

  // ANTLR listener to report syntax errors.
  class FhirPathErrorListener : public BaseErrorListener {
   public:
    explicit FhirPathErrorListener(FhirPathCompilerVisitor* visitor)
        : visitor_(visitor) {}

    void syntaxError(antlr4::Recognizer* recognizer,
                     antlr4::Token* offending_symbol, size_t line,
                     size_t position_in_line, const std::string& message,
                     std::exception_ptr e) override {
      visitor_->SetError(message);
    }

   private:
    FhirPathCompilerVisitor* visitor_;
  };

  void SetError(const std::string& error_message) {
    error_message_ = error_message;
  }

  FhirPathErrorListener error_listener_;
  std::vector<const Descriptor*> descriptor_stack_;
  std::string error_message_;
  const PrimitiveHandler* primitive_handler_;
};

}  // namespace internal

EvaluationResult::EvaluationResult(EvaluationResult&& result)
    : work_space_(std::move(result.work_space_)) {}

EvaluationResult& EvaluationResult::operator=(EvaluationResult&& result) {
  work_space_ = std::move(result.work_space_);

  return *this;
}

EvaluationResult::EvaluationResult(
    std::unique_ptr<internal::WorkSpace> work_space)
    : work_space_(std::move(work_space)) {}

EvaluationResult::~EvaluationResult() {}

const std::vector<const Message*>& EvaluationResult::GetMessages() const {
  return work_space_->GetResultMessages();
}

StatusOr<bool> EvaluationResult::GetBoolean() const {
  auto messages = work_space_->GetResultMessages();
  if (messages.size() != 1) {
    return InvalidArgument(
        "Result collection must contain exactly one element");
  }
  return work_space_->GetPrimitiveHandler()->GetBooleanValue(*messages[0]);
}

StatusOr<int32_t> EvaluationResult::GetInteger() const {
  auto messages = work_space_->GetResultMessages();
  if (messages.size() != 1) {
    return InvalidArgument(
        "Result collection must contain exactly one element");
  }
  return work_space_->GetPrimitiveHandler()->GetIntegerValue(*messages[0]);
}

StatusOr<std::string> EvaluationResult::GetDecimal() const {
  auto messages = work_space_->GetResultMessages();
  if (messages.size() != 1) {
    return InvalidArgument(
        "Result collection must contain exactly one element");
  }
  return work_space_->GetPrimitiveHandler()->GetDecimalValue(*messages[0]);
}

StatusOr<std::string> EvaluationResult::GetString() const {
  auto messages = work_space_->GetResultMessages();
  if (messages.size() != 1) {
    return InvalidArgument(
        "Result collection must contain exactly one element");
  }
  return work_space_->GetPrimitiveHandler()->GetStringValue(*messages[0]);
}

CompiledExpression::CompiledExpression(CompiledExpression&& other)
    : fhir_path_(std::move(other.fhir_path_)),
      root_expression_(std::move(other.root_expression_)),
      primitive_handler_(other.primitive_handler_) {}

CompiledExpression& CompiledExpression::operator=(CompiledExpression&& other) {
  fhir_path_ = std::move(other.fhir_path_);
  root_expression_ = std::move(other.root_expression_);
  primitive_handler_ = other.primitive_handler_;

  return *this;
}

CompiledExpression::CompiledExpression(const CompiledExpression& other)
    : fhir_path_(other.fhir_path_),
      root_expression_(other.root_expression_),
      primitive_handler_(other.primitive_handler_) {}

CompiledExpression& CompiledExpression::operator=(
    const CompiledExpression& other) {
  fhir_path_ = other.fhir_path_;
  root_expression_ = other.root_expression_;
  primitive_handler_ = other.primitive_handler_;

  return *this;
}

const std::string& CompiledExpression::fhir_path() const { return fhir_path_; }

CompiledExpression::CompiledExpression(
    const std::string& fhir_path,
    std::shared_ptr<internal::ExpressionNode> root_expression,
    const PrimitiveHandler* primitive_handler)
    : fhir_path_(fhir_path),
      root_expression_(root_expression),
      primitive_handler_(primitive_handler) {}

StatusOr<CompiledExpression> CompiledExpression::Compile(
    const Descriptor* descriptor, const PrimitiveHandler* primitive_handler,
    const std::string& fhir_path) {
  ANTLRInputStream input(fhir_path);
  FhirPathLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  FhirPathParser parser(&tokens);

  internal::FhirPathCompilerVisitor visitor(descriptor, primitive_handler);
  parser.addErrorListener(visitor.GetErrorListener());
  lexer.addErrorListener(visitor.GetErrorListener());
  antlrcpp::Any result = visitor.visit(parser.expression());

  // TODO: the visitor error check should be redundant
  if (result.isNotNull() && visitor.CheckOk()) {
    auto root_node = result.as<std::shared_ptr<internal::ExpressionNode>>();
    return CompiledExpression(fhir_path, root_node, primitive_handler);
  } else {
    auto status = InvalidArgument(visitor.GetError());
    return InvalidArgument(visitor.GetError());
  }
}

StatusOr<EvaluationResult> CompiledExpression::Evaluate(
    const Message& message) const {
  return Evaluate(internal::WorkspaceMessage(&message));
}

StatusOr<EvaluationResult> CompiledExpression::Evaluate(
    const internal::WorkspaceMessage& message) const {
  std::vector<internal::WorkspaceMessage> message_context_stack;
  auto work_space = absl::make_unique<internal::WorkSpace>(
      primitive_handler_, message_context_stack, message);

  std::vector<internal::WorkspaceMessage> workspace_results;
  FHIR_RETURN_IF_ERROR(
      root_expression_->Evaluate(work_space.get(), &workspace_results));

  std::vector<const Message*> results;
  results.reserve(workspace_results.size());
  for (internal::WorkspaceMessage& result : workspace_results) {
    results.push_back(result.Message());
  }

  work_space->SetResultMessages(results);

  return EvaluationResult(std::move(work_space));
}

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
