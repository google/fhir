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

#include <algorithm>
#include <any>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include "google/protobuf/any.pb.h"
#include "google/protobuf/util/message_differencer.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_set.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/strip.h"
#include "absl/time/civil_time.h"
#include "absl/time/time.h"
#include "absl/types/optional.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codes.h"
#include "google/fhir/fhir_path/FhirPathBaseVisitor.h"
#include "google/fhir/fhir_path/FhirPathLexer.h"
#include "google/fhir/fhir_path/FhirPathParser.h"
#include "google/fhir/fhir_path/fhir_path_types.h"
#include "google/fhir/fhir_path/utils.h"
#include "google/fhir/fhir_types.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "icu4c/source/common/unicode/unistr.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"

namespace google {
namespace fhir {
namespace fhir_path {

using ::absl::InternalError;
using ::absl::InvalidArgumentError;
using ::absl::NotFoundError;
using ::absl::UnimplementedError;
using ::antlr4::ANTLRInputStream;
using ::antlr4::BaseErrorListener;
using ::antlr4::CommonTokenStream;
using ::antlr4::tree::TerminalNode;
using ::antlr_parser::FhirPathBaseVisitor;
using ::antlr_parser::FhirPathLexer;
using ::antlr_parser::FhirPathParser;
using ::google::fhir::AreSameMessageType;
using ::google::fhir::JsonPrimitive;
using ::google::fhir::r4::core::Boolean;
using ::google::fhir::r4::core::Integer;
using ::google::fhir::r4::core::String;
using internal::ExpressionNode;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::util::MessageDifferencer;

namespace {

// TODO(b/216654862): Remove after open source builds are using ANTLR greater
// than 4.9.3
//
// ANTLR versions greater than 4.9.3 removed ANTLR's custom implementation of
// antlrcpp::Any and switched to making antlrcpp::Any a type alias of std::any.
// The following functions act as a shim for handling either type.

template <typename T>
inline constexpr bool IsAntlrAnyOrStdAny =
    (std::is_same_v<T, antlrcpp::Any> || std::is_same_v<T, std::any>);

template <typename F>
std::enable_if_t<IsAntlrAnyOrStdAny<F>, bool> AnyHasValue(const F& any) {
  if constexpr (std::is_same_v<antlrcpp::Any, std::any>) {
    return any.has_value() && any.type() != typeid(nullptr);
  } else {
    return !any.isNull();
  }
}

template <typename T, typename F>
std::enable_if_t<IsAntlrAnyOrStdAny<F>, T> AnyCast(const F& any) {
  if constexpr (std::is_same_v<antlrcpp::Any, std::any>) {
    return std::any_cast<T>(any);
  } else {
    return any.template as<T>();
  }
}

template <typename T, typename F>
std::enable_if_t<IsAntlrAnyOrStdAny<F>, T> AnyCast(F& any) {
  if constexpr (std::is_same_v<antlrcpp::Any, std::any>) {
    return std::any_cast<T>(any);
  } else {
    return any.template as<T>();
  }
}

template <typename T, typename F>
std::enable_if_t<IsAntlrAnyOrStdAny<F>, T> AnyCast(F&& any) {
  if constexpr (std::is_same_v<antlrcpp::Any, std::any>) {
    return std::any_cast<T>(std::forward<F>(any));
  } else {
    return std::forward<F>(any).template as<T>();
  }
}

template <typename T, typename F>
std::enable_if_t<IsAntlrAnyOrStdAny<F>, bool> AnyIs(const F& any) {
  if constexpr (std::is_same_v<antlrcpp::Any, std::any>) {
    return std::any_cast<T>(&any) != nullptr;
  } else {
    return any.template is<T>();
  }
}

}  // namespace

namespace internal {

// Default number of buckets to create when constructing a std::unordered_set.
constexpr int kDefaultSetBucketCount = 10;

// Returns the integer value of the FHIR message if it can be converted to
// a System.Integer
//
// See https://www.hl7.org/fhir/fhirpath.html#types
absl::StatusOr<int32_t> ToSystemInteger(
    const PrimitiveHandler* primitive_handler, const Message& message) {
  // It isn't necessary to widen the values from 32 to 64 bits when converting
  // a UnsignedInt or PositiveInt to an int32_t because FHIR restricts the
  // values of those types to 31 bits.
  if (IsInteger(message)) {
    return primitive_handler->GetIntegerValue(message).value();
  }

  if (IsUnsignedInt(message)) {
    return primitive_handler->GetUnsignedIntValue(message).value();
  }

  if (IsPositiveInt(message)) {
    return primitive_handler->GetPositiveIntValue(message).value();
  }

  return InvalidArgumentError(
      absl::StrCat(message.GetTypeName(), " cannot be cast to an integer."));
}

absl::StatusOr<absl::optional<int32_t>> IntegerOrEmpty(
    const PrimitiveHandler* primitive_handler,
    const std::vector<WorkspaceMessage>& messages) {
  if (messages.empty()) {
    return absl::optional<int>();
  }

  if (messages.size() > 1 || !IsSystemInteger(*messages[0].Message())) {
    return InvalidArgumentError(
        "Expression must be empty or represent a single primitive value.");
  }

  FHIR_ASSIGN_OR_RETURN(int32_t value, ToSystemInteger(primitive_handler,
                                                       *messages[0].Message()));
  return absl::optional<int32_t>(value);
}

// See http://hl7.org/fhirpath/N1/#singleton-evaluation-of-collections
absl::StatusOr<absl::optional<bool>> BooleanOrEmpty(
    const PrimitiveHandler* primitive_handler,
    const std::vector<WorkspaceMessage>& messages) {
  if (messages.empty()) {
    return absl::optional<bool>();
  }

  if (messages.size() > 1) {
    return InvalidArgumentError(
        "Expression must be empty or contain a single value.");
  }

  if (!IsBoolean(*messages[0].Message())) {
    return absl::optional<bool>(true);
  }

  FHIR_ASSIGN_OR_RETURN(
      bool value, primitive_handler->GetBooleanValue(*messages[0].Message()));
  return absl::optional<bool>(value);
}

// Returns the string representation of the provided message if the message is a
// System.String or a FHIR primitive that implicitly converts to System.String.
// Otherwise a status other than OK will be returned.
absl::StatusOr<std::string> MessageToString(const Message& message) {
  if (!IsSystemString(message)) {
    return InvalidArgumentError("Expression is not a string.");
  }

  if (HasValueset(message.GetDescriptor())) {
    return GetCodeAsString(message);
  }

  std::string value;
  return GetPrimitiveStringValue(message, &value);
}

// Convenient wrapper of MessageToString for WorkspaceMessage input types.
absl::StatusOr<std::string> MessageToString(const WorkspaceMessage& message) {
  return MessageToString(*message.Message());
}

// Returns the string representation of the provided messages if there is
// exactly one message in the collection and that message is a System.String or
// a FHIR primitive that implicitly converts to System.String. Otherwise a
// absl::Status other than OK will be returned.
absl::StatusOr<std::string> MessagesToString(
    const std::vector<WorkspaceMessage>& messages) {
  if (messages.size() != 1) {
    return InvalidArgumentError("Expression must represent a single value.");
  }

  return MessageToString(messages[0]);
}

// Converts decimal or integer container messages to a double value.
static absl::Status MessageToDouble(const PrimitiveHandler* primitive_handler,
                                    const Message& message, double* value) {
  if (IsDecimal(message)) {
    FHIR_ASSIGN_OR_RETURN(std::string string_value,
                          primitive_handler->GetDecimalValue(message));

    if (!absl::SimpleAtod(string_value, value)) {
      return InvalidArgumentError(
          "Could not convert decimal to numeric double.");
    }

    return absl::OkStatus();

  } else if (IsSystemInteger(message)) {
    FHIR_ASSIGN_OR_RETURN(*value, ToSystemInteger(primitive_handler, message));
    return absl::OkStatus();
  }

  return InvalidArgumentError(
      absl::StrCat("Message type cannot be converted to double: ",
                   message.GetDescriptor()->full_name()));
}

// Returns a function that creates a new message of the provided descriptor type
// that is flagged to be deleted when the workspace is destroyed.
std::function<Message*(const Descriptor*)> MakeWorkSpaceMessageFactory(
    WorkSpace* work_space) {
  return [=](const Descriptor* descriptor) -> Message* {
    const Message* prototype =
        ::google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
    if (prototype == nullptr) {
      return nullptr;
    }

    Message* message = prototype->New();
    work_space->DeleteWhenFinished(message);
    return message;
  };
}

absl::StatusOr<WorkspaceMessage> WorkspaceMessage::NearestResource() const {
  if (IsResource(result_->GetDescriptor())) {
    return *this;
  }

  auto it = std::find_if(ancestry_stack_.rbegin(), ancestry_stack_.rend(),
                         [](const google::protobuf::Message* message) {
                           return IsResource(message->GetDescriptor());
                         });

  if (it == ancestry_stack_.rend()) {
    return NotFoundError("No Resource found in ancestry.");
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

// Produces a shared pointer explicitly of ExpressionNode rather
// than a subclass to work well with ANTLR's "Any" semantics.
inline std::shared_ptr<ExpressionNode> ToAny(
    std::shared_ptr<ExpressionNode> node) {
  return node;
}

// Expression node that returns literals wrapped in the corresponding
// protobuf wrapper
class Literal : public ExpressionNode {
 public:
  Literal(const Descriptor* descriptor,
          std::function<absl::StatusOr<Message*>()> factory)
      : descriptor_(descriptor), factory_(factory) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    FHIR_ASSIGN_OR_RETURN(Message * value, factory_());
    work_space->DeleteWhenFinished(value);
    results->push_back(WorkspaceMessage(value));

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return descriptor_; }

  std::vector<std::shared_ptr<ExpressionNode>> GetChildren()
      const override {
    return {};
  }

 private:
  const Descriptor* descriptor_;
  std::function<absl::StatusOr<Message*>()> factory_;
};

// Expression node for the empty literal.
class EmptyLiteral : public ExpressionNode {
 public:
  EmptyLiteral() {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    return absl::OkStatus();
  }

  // The return type of the empty literal is undefined. If this causes problems,
  // it is likely we could arbitrarily pick one of the primitive types without
  // ill-effect.
  const Descriptor* ReturnType() const override { return nullptr; }

  std::vector<std::shared_ptr<ExpressionNode>> GetChildren()
      const override {
    return {};
  }
};

// Expression node for a reference to $this.
class ThisReference : public ExpressionNode {
 public:
  explicit ThisReference(const Descriptor* descriptor)
      : descriptor_(descriptor) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    results->push_back(work_space->MessageContext());
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return descriptor_; }

  std::vector<std::shared_ptr<ExpressionNode>> GetChildren()
      const override {
    return {};
  }

 private:
  const Descriptor* descriptor_;
};

// Expression node for a reference to %context.
class ContextReference : public ExpressionNode {
 public:
  explicit ContextReference(const Descriptor* descriptor)
      : descriptor_(descriptor) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    results->push_back(work_space->BottomMessageContext());
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return descriptor_; }

  std::vector<std::shared_ptr<ExpressionNode>> GetChildren()
      const override {
    return {};
  }

 private:
  const Descriptor* descriptor_;
};

// Expression node for a reference to %resource.
class ResourceReference : public ExpressionNode {
 public:
  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    FHIR_ASSIGN_OR_RETURN(WorkspaceMessage result,
                          work_space->MessageContext().NearestResource());
    results->push_back(result);
    return absl::OkStatus();
  }

  // TODO(b/244184211): Track %resource type during compilation.
  const Descriptor* ReturnType() const override { return nullptr; }

  std::vector<std::shared_ptr<ExpressionNode>> GetChildren()
      const override {
    return {};
  }
};

// Implements the InvocationTerm from the FHIRPath grammar,
// producing a term from the root context message.
class InvokeTermNode : public ExpressionNode {
 public:
  explicit InvokeTermNode(const FieldDescriptor* field,
                          const std::string& field_name)
      : field_(field), field_name_(field_name) {}

  absl::Status Evaluate(WorkSpace* work_space,
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
      return absl::OkStatus();
    }

    std::vector<const Message*> result_protos;
    FHIR_RETURN_IF_ERROR(RetrieveField(*message.Message(), *field,
                                       MakeWorkSpaceMessageFactory(work_space),
                                       &result_protos));
    for (const Message* result : result_protos) {
      results->push_back(WorkspaceMessage(message, result));
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return field_ != nullptr ? field_->message_type() : nullptr;
  }

  std::vector<std::shared_ptr<ExpressionNode>> GetChildren()
      const override {
    return {};
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

  absl::Status Evaluate(WorkSpace* work_space,
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
      FHIR_RETURN_IF_ERROR(RetrieveField(
          *child_message.Message(), *field,
          MakeWorkSpaceMessageFactory(work_space), &result_protos));
      for (const Message* result : result_protos) {
        results->push_back(WorkspaceMessage(child_message, result));
      }
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return field_ != nullptr ? field_->message_type() : nullptr;
  }

  std::vector<std::shared_ptr<ExpressionNode>> GetChildren()
      const override {
    return {child_expression_};
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
  absl::StatusOr<T*> static Create(
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

  static absl::StatusOr<std::vector<std::shared_ptr<ExpressionNode>>>
  CompileParams(const std::vector<FhirPathParser::ExpressionContext*>& params,
                FhirPathBaseVisitor* base_context_visitor,
                FhirPathBaseVisitor*) {
    return CompileParams(params, base_context_visitor);
  }

  static absl::StatusOr<std::vector<std::shared_ptr<ExpressionNode>>>
  CompileParams(const std::vector<FhirPathParser::ExpressionContext*>& params,
                FhirPathBaseVisitor* visitor) {
    std::vector<std::shared_ptr<ExpressionNode>> compiled_params;

    for (auto it = params.begin(); it != params.end(); ++it) {
      antlrcpp::Any param_any = (*it)->accept(visitor);
      // Ensure param_any is an ExpressionNode and not, say, a nullptr, before
      // attempting to cast it to an ExpressionNode below. AnyIs guards against
      // nullptrs but AnyHasValue does not.
      if (!AnyHasValue(param_any) ||
          !AnyIs<std::shared_ptr<ExpressionNode>>(param_any)) {
        return InvalidArgumentError("Failed to compile parameter.");
      }
      compiled_params.push_back(
          AnyCast<std::shared_ptr<ExpressionNode>>(param_any));
    }

    return compiled_params;
  }

  // This is the default implementation. FunctionNodes's that need to validate
  // params at compile time should overwrite this definition with their own.
  static absl::Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    return absl::OkStatus();
  }

  std::vector<std::shared_ptr<ExpressionNode>> GetChildren()
      const override {
    std::vector<std::shared_ptr<ExpressionNode>> children = params_;
    children.insert(children.begin(), child_);
    return children;
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
  static absl::Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    if (!params.empty()) {
      return InvalidArgumentError("Function does not accept any arguments.");
    }

    return absl::OkStatus();
  }

 protected:
  ZeroParameterFunctionNode(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {
    FHIR_DCHECK_OK(ValidateParams(params));
  }
};

class SingleParameterFunctionNode : public FunctionNode {
 private:
  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    //  requires a single parameter
    if (params_.size() != 1) {
      return InvalidArgumentError("this function requires a single parameter.");
    }

    std::vector<WorkspaceMessage> first_param;
    FHIR_RETURN_IF_ERROR(params_[0]->Evaluate(work_space, &first_param));

    return Evaluate(work_space, first_param, results);
  }

 public:
  static absl::Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    if (params.size() != 1) {
      return InvalidArgumentError("Function requires exactly one argument.");
    }

    return absl::OkStatus();
  }

 protected:
  SingleParameterFunctionNode(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {
    FHIR_DCHECK_OK(ValidateParams(params));
  }

  virtual absl::Status Evaluate(
      WorkSpace* work_space, const std::vector<WorkspaceMessage>& first_param,
      std::vector<WorkspaceMessage>* results) const = 0;
};

class SingleValueFunctionNode : public SingleParameterFunctionNode {
 private:
  absl::Status Evaluate(WorkSpace* work_space,
                        const std::vector<WorkspaceMessage>& first_param,
                        std::vector<WorkspaceMessage>* results) const override {
    //  requires a single parameter
    if (first_param.size() != 1) {
      return InvalidArgumentError(
          "this function requires a single value parameter.");
    }

    return EvaluateWithParam(work_space, first_param[0], results);
  }

 protected:
  SingleValueFunctionNode(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleParameterFunctionNode(child, params) {}

  virtual absl::Status EvaluateWithParam(
      WorkSpace* work_space, const WorkspaceMessage& param,
      std::vector<WorkspaceMessage>* results) const = 0;
};

// Implements the FHIRPath .exists() function
class ExistsFunction : public FunctionNode {
 public:
  static absl::Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    if (params.size() > 1) {
      return InvalidArgumentError("Function requires 0 or 1 arguments.");
    }

    return absl::OkStatus();
  }

  static absl::StatusOr<std::vector<std::shared_ptr<ExpressionNode>>>
  CompileParams(const std::vector<FhirPathParser::ExpressionContext*>& params,
                FhirPathBaseVisitor*,
                FhirPathBaseVisitor* child_context_visitor) {
    return FunctionNode::CompileParams(params, child_context_visitor);
  }

  ExistsFunction(const std::shared_ptr<ExpressionNode>& child,
                 const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {
    FHIR_DCHECK_OK(ValidateParams(params));
  }

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (params_.empty()) {
      Message* result =
          work_space->GetPrimitiveHandler()->NewBoolean(!child_results.empty());
      work_space->DeleteWhenFinished(result);
      results->push_back(WorkspaceMessage(result));
      return absl::OkStatus();
    }

    for (const WorkspaceMessage& message : child_results) {
      std::vector<WorkspaceMessage> param_results;
      WorkSpace expression_work_space(work_space->GetPrimitiveHandler(),
                                      work_space->MessageContextStack(),
                                      message);
      FHIR_RETURN_IF_ERROR(
          params_[0]->Evaluate(&expression_work_space, &param_results));
      FHIR_ASSIGN_OR_RETURN(
          absl::StatusOr<std::optional<bool>> allowed,
          (BooleanOrEmpty(work_space->GetPrimitiveHandler(), param_results)));
      if (allowed.value().value_or(false)) {
        Message* result =
            work_space->GetPrimitiveHandler()->NewBoolean(true);
        work_space->DeleteWhenFinished(result);
        results->push_back(WorkspaceMessage(result));
        return absl::OkStatus();
      }
    }

    Message* result =
        work_space->GetPrimitiveHandler()->NewBoolean(false);
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
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

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;

    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    // Per the FHIRPath spec, boolean operations on empty collection
    // propagate the empty collection.
    if (child_results.empty()) {
      return absl::OkStatus();
    }

    if (child_results.size() != 1) {
      return InvalidArgumentError(
          "not() must be invoked on a singleton collection");
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

    return absl::OkStatus();
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

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;

    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    Message* result = work_space->GetPrimitiveHandler()->NewBoolean(
        child_results.size() == 1 &&
        IsPrimitive(child_results[0].Message()->GetDescriptor()));
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }
};

class IndexOfFunction : public SingleParameterFunctionNode {
 public:
  IndexOfFunction(const std::shared_ptr<ExpressionNode>& child,
                  const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        const std::vector<WorkspaceMessage>& first_param,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1 || first_param.size() > 1) {
      return InvalidArgumentError(
          "indexOf() must be invoked on a string with a string argument.");
    }

    if (child_results.empty() || first_param.empty()) {
      return absl::OkStatus();
    }

    FHIR_ASSIGN_OR_RETURN(std::string haystack,
                          MessagesToString(child_results));
    FHIR_ASSIGN_OR_RETURN(std::string needle, MessageToString(first_param[0]));

    size_t position = haystack.find(needle);
    Message* result = work_space->GetPrimitiveHandler()->NewInteger(
        position == std::string::npos ? -1 : position);
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return Integer::descriptor();
  }
};

class SubstringFunction : public FunctionNode {
 public:
  static absl::StatusOr<std::vector<std::shared_ptr<ExpressionNode>>>
  CompileParams(const std::vector<FhirPathParser::ExpressionContext*>& params,
                FhirPathBaseVisitor* base_context_visitor,
                FhirPathBaseVisitor* child_context_visitor) {
    if (params.empty() || params.size() > 2) {
      return InvalidArgumentError(
          "substring(start [, length]) requires 1 or 2 arugments.");
    }

    std::vector<std::shared_ptr<ExpressionNode>> compiled_params;

    antlrcpp::Any start = params[0]->accept(base_context_visitor);
    if (!AnyHasValue(start)) {
      return InvalidArgumentError("Failed to compile `start` parameter.");
    }
    compiled_params.push_back(AnyCast<std::shared_ptr<ExpressionNode>>(start));

    if (params.size() > 1) {
      antlrcpp::Any length = params[1]->accept(base_context_visitor);
      if (!AnyHasValue(length)) {
        return InvalidArgumentError("Failed to compile `length` parameter.");
      }
      compiled_params.push_back(
          AnyCast<std::shared_ptr<ExpressionNode>>(length));
    }

    return compiled_params;
  }

  SubstringFunction(const std::shared_ptr<ExpressionNode>& child,
                    const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {
    FHIR_DCHECK_OK(ValidateParams(params));
  }

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return InvalidArgumentError(
          "substring() requires a collection with no more than 1 item.");
    }

    if (child_results.empty()) {
      return absl::OkStatus();
    }

    FHIR_ASSIGN_OR_RETURN(std::string full_string,
                          MessagesToString(child_results));

    std::vector<WorkspaceMessage> start_param, length_param;
    FHIR_RETURN_IF_ERROR(params_[0]->Evaluate(work_space, &start_param));
    if (start_param.size() > 1) {
      return InvalidArgumentError(
          "substring() `start` evaluated to more than 1 output.");
    }

    // Parameter `start` can be empty according to FHIR spec.
    FHIR_ASSIGN_OR_RETURN(
        absl::optional<int> start,
        IntegerOrEmpty(work_space->GetPrimitiveHandler(), start_param));
    if (!start.has_value() || start.value() < 0 ||
        start.value() >= full_string.length()) {
      return absl::OkStatus();
    }

    int length_value = full_string.length() - start.value();
    if (params_.size() > 1) {
      FHIR_RETURN_IF_ERROR(params_[1]->Evaluate(work_space, &length_param));
      FHIR_ASSIGN_OR_RETURN(
          absl::optional<int> length,
          IntegerOrEmpty(work_space->GetPrimitiveHandler(), length_param));
      if (length.has_value()) {
        if (length.value() < 0) {
          return InvalidArgumentError(
              "substring() `length` cannot be negative.");
        } else if (length.value() < length_value) {
          length_value = length.value();
        }
      }
    }

    icu::UnicodeString unicode_string =
        icu::UnicodeString::fromUTF8(full_string);
    int start_index = unicode_string.moveIndex32(0, start.value());
    int end_index = unicode_string.moveIndex32(start_index, length_value);

    std::string substring;
    unicode_string.tempSubStringBetween(start_index, end_index)
        .toUTF8String(substring);

    Message* result = work_space->GetPrimitiveHandler()->NewString(substring);
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return String::descriptor(); }
};

class StringTestFunction : public SingleValueFunctionNode {
 public:
  StringTestFunction(const std::shared_ptr<ExpressionNode>& child,
                     const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleValueFunctionNode(child, params) {}

  absl::Status EvaluateWithParam(
      WorkSpace* work_space, const WorkspaceMessage& param,
      std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return InvalidArgumentError("Function must be invoked on a string.");
    }

    if (child_results.empty()) {
      return absl::OkStatus();
    }

    FHIR_ASSIGN_OR_RETURN(std::string item, MessagesToString(child_results));
    FHIR_ASSIGN_OR_RETURN(std::string test_string, MessageToString(param));

    Message* result =
        work_space->GetPrimitiveHandler()->NewBoolean(Test(item, test_string));
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
  }

  virtual bool Test(absl::string_view input,
                    absl::string_view test_string) const = 0;

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
class StartsWithFunction : public StringTestFunction {
 public:
  StartsWithFunction(const std::shared_ptr<ExpressionNode>& child,
                     const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : StringTestFunction(child, params) {}
  bool Test(absl::string_view input,
            absl::string_view test_string) const override {
    return absl::StartsWith(input, test_string);
  }
};

// Implements the FHIRPath .endsWith() function.
class EndsWithFunction : public StringTestFunction {
 public:
  EndsWithFunction(const std::shared_ptr<ExpressionNode>& child,
                   const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : StringTestFunction(child, params) {}
  bool Test(absl::string_view input,
            absl::string_view test_string) const override {
    return absl::EndsWith(input, test_string);
  }
};

class StringTransformationFunction : public ZeroParameterFunctionNode {
 public:
  StringTransformationFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return InvalidArgumentError("Function must be invoked on a string.");
    }

    if (child_results.empty()) {
      return absl::OkStatus();
    }

    FHIR_ASSIGN_OR_RETURN(std::string item, MessagesToString(child_results));

    Message* result =
        work_space->GetPrimitiveHandler()->NewString(Transform(item));
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
  }

  virtual std::string Transform(const std::string& input) const = 0;

  const Descriptor* ReturnType() const override { return String::descriptor(); }
};

class LowerFunction : public StringTransformationFunction {
 public:
  LowerFunction(const std::shared_ptr<ExpressionNode>& child,
                const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : StringTransformationFunction(child, params) {}

  std::string Transform(const std::string& input) const override {
    std::string uppercase_string;
    icu::UnicodeString::fromUTF8(input).toLower().toUTF8String(
        uppercase_string);
    return uppercase_string;
  }
};

class UpperFunction : public StringTransformationFunction {
 public:
  UpperFunction(const std::shared_ptr<ExpressionNode>& child,
                const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : StringTransformationFunction(child, params) {}

  std::string Transform(const std::string& input) const override {
    std::string uppercase_string;
    icu::UnicodeString::fromUTF8(input).toUpper().toUTF8String(
        uppercase_string);
    return uppercase_string;
  }
};

// Implements the FHIRPath .contains() function.
class ContainsFunction : public StringTestFunction {
 public:
  explicit ContainsFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : StringTestFunction(child, params) {}

  bool Test(absl::string_view input,
            absl::string_view test_string) const override {
    return absl::StrContains(input, test_string);
  }
};

class MatchesFunction : public SingleValueFunctionNode {
 public:
  explicit MatchesFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleValueFunctionNode(child, params) {}

  absl::Status EvaluateWithParam(
      WorkSpace* work_space, const WorkspaceMessage& param,
      std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.empty()) {
      return absl::OkStatus();
    }

    FHIR_ASSIGN_OR_RETURN(std::string item, MessagesToString(child_results));
    FHIR_ASSIGN_OR_RETURN(std::string re_string, MessageToString(param));

    RE2 re(re_string);

    if (!re.ok()) {
      return InvalidArgumentError(
          absl::StrCat("Unable to parse regular expression, '", re_string,
                       "'. ", re.error()));
    }

    Message* result =
        work_space->GetPrimitiveHandler()->NewBoolean(RE2::FullMatch(item, re));
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }
};

class ReplaceFunction : public FunctionNode {
 public:
  ReplaceFunction(const std::shared_ptr<ExpressionNode>& child,
                  const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> pattern_param;
    FHIR_RETURN_IF_ERROR(params_[0]->Evaluate(work_space, &pattern_param));

    std::vector<WorkspaceMessage> replacement_param;
    FHIR_RETURN_IF_ERROR(params_[1]->Evaluate(work_space, &replacement_param));

    return EvaluateWithParam(work_space, pattern_param, replacement_param,
                             results);
  }

  absl::Status EvaluateWithParam(
      WorkSpace* work_space, const std::vector<WorkspaceMessage>& pattern_param,
      const std::vector<WorkspaceMessage>& replacement_param,
      std::vector<WorkspaceMessage>* results) const {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    // If the input collection, pattern, or substitution are empty, the result
    // is empty ({ }).
    // http://hl7.org/fhirpath/N1/#replacepattern-string-substitution-string-string
    if (child_results.empty() || pattern_param.empty() ||
        replacement_param.empty()) {
      return absl::OkStatus();
    }

    FHIR_ASSIGN_OR_RETURN(std::string item, MessagesToString(child_results));
    FHIR_ASSIGN_OR_RETURN(std::string pattern, MessagesToString(pattern_param));
    FHIR_ASSIGN_OR_RETURN(std::string replacement,
                          MessagesToString(replacement_param));

    if (!item.empty() && pattern.empty()) {
      // "If pattern is the empty string (''), every character in the input
      // string is surrounded by the substitution, e.g. 'abc'.replace('','x')
      // becomes 'xaxbxcx'."
      std::string replaced(replacement);
      replaced.reserve(replacement.length() * (item.length() + 1) +
                       item.length());
      icu::UnicodeString original = icu::UnicodeString::fromUTF8(item);
      for (int i = 0; i < original.length(); i++) {
        original.tempSubStringBetween(i, i + 1).toUTF8String(replaced);
        replaced.append(replacement);
      }
      item = std::move(replaced);
    } else {
      item = absl::StrReplaceAll(item, {{pattern, replacement}});
    }

    Message* result = work_space->GetPrimitiveHandler()->NewString(item);
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return String::descriptor(); }

  static absl::Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    return params.size() == 2
               ? absl::OkStatus()
               : InvalidArgumentError(absl::StrCat(
                     "replace() requires exactly two parameters. Got ",
                     params.size(), "."));
  }
};

class ReplaceMatchesFunction : public FunctionNode {
 public:
  explicit ReplaceMatchesFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> pattern_param;
    FHIR_RETURN_IF_ERROR(params_[0]->Evaluate(work_space, &pattern_param));

    std::vector<WorkspaceMessage> replacement_param;
    FHIR_RETURN_IF_ERROR(params_[1]->Evaluate(work_space, &replacement_param));

    return EvaluateWithParam(work_space, pattern_param, replacement_param,
                             results);
  }

  absl::Status EvaluateWithParam(
      WorkSpace* work_space, const std::vector<WorkspaceMessage>& pattern_param,
      const std::vector<WorkspaceMessage>& replacement_param,
      std::vector<WorkspaceMessage>* results) const {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.empty()) {
      return absl::OkStatus();
    }

    FHIR_ASSIGN_OR_RETURN(std::string item, MessagesToString(child_results));
    FHIR_ASSIGN_OR_RETURN(std::string re_string,
                          MessagesToString(pattern_param));
    FHIR_ASSIGN_OR_RETURN(std::string replacement_string,
                          MessagesToString(replacement_param));

    RE2 re(re_string);

    if (!re.ok()) {
      return InvalidArgumentError(
          absl::StrCat("Unable to parse regular expression '", re_string, "'. ",
                       re.error()));
    }

    RE2::Replace(&item, re, replacement_string);

    Message* result = work_space->GetPrimitiveHandler()->NewString(item);
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return String::descriptor(); }

  static absl::Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    if (params.size() != 2) {
      return InvalidArgumentError(
          absl::StrCat("replaceMatches requires exactly two parameters. Got ",
                       params.size(), "."));
    }

    return absl::OkStatus();
  }
};

class ToStringFunction : public ZeroParameterFunctionNode {
 public:
  ToStringFunction(const std::shared_ptr<ExpressionNode>& child,
                   const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return InvalidArgumentError(
          "Input collection must not contain multiple items");
    }

    if (child_results.empty()) {
      return absl::OkStatus();
    }

    const WorkspaceMessage& child = child_results[0];

    if (IsSystemString(*child.Message())) {
      FHIR_ASSIGN_OR_RETURN(std::string value, MessageToString(child));
      Message* result = work_space->GetPrimitiveHandler()->NewString(value);
      work_space->DeleteWhenFinished(result);
      results->push_back(WorkspaceMessage(result));
      return absl::OkStatus();
    }

    if (!IsPrimitive(child.Message()->GetDescriptor())) {
      return absl::OkStatus();
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
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return String::descriptor(); }
};

// Implements the FHIRPath .length() function.
class LengthFunction : public ZeroParameterFunctionNode {
 public:
  LengthFunction(const std::shared_ptr<ExpressionNode>& child,
                 const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.empty()) {
      return absl::OkStatus();
    }

    FHIR_ASSIGN_OR_RETURN(std::string item, MessagesToString(child_results));

    Message* result =
        work_space->GetPrimitiveHandler()->NewInteger(item.length());
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
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
  EmptyFunction(const std::shared_ptr<ExpressionNode>& child,
                const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    Message* result =
        work_space->GetPrimitiveHandler()->NewBoolean(child_results.empty());
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
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
  CountFunction(const std::shared_ptr<ExpressionNode>& child,
                const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    Message* result =
        work_space->GetPrimitiveHandler()->NewInteger(child_results.size());
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return Integer::descriptor();
  }
};

// Implements the FHIRPath .single() function.
//
// Returns the sole element of the input collection. Signals an error if
// there is more than one element in the input collection.
class SingleFunction : public ZeroParameterFunctionNode {
 public:
  SingleFunction(const std::shared_ptr<ExpressionNode>& child,
                 const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return absl::FailedPreconditionError(
          "single() may not be called on a collection with size greater than "
          "1.");
    }

    if (!child_results.empty()) {
      results->push_back(child_results.back());
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return child_->ReturnType(); }
};

// Implements the FHIRPath .first() function.
//
// Returns the first element of the input collection. Or an empty collection if
// if the input collection is empty.
class FirstFunction : public ZeroParameterFunctionNode {
 public:
  FirstFunction(const std::shared_ptr<ExpressionNode>& child,
                const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (!child_results.empty()) {
      results->push_back(child_results[0]);
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return child_->ReturnType(); }
};

// Implements the FHIRPath .last() function.
//
// Returns the last element of the input collection. Or an empty collection if
// if the input collection is empty.
class LastFunction : public ZeroParameterFunctionNode {
 public:
  LastFunction(const std::shared_ptr<ExpressionNode>& child,
               const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (!child_results.empty()) {
      results->push_back(child_results.back());
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return child_->ReturnType(); }
};

// Implements the FHIRPath .tail() function.
class TailFunction : public ZeroParameterFunctionNode {
 public:
  TailFunction(const std::shared_ptr<ExpressionNode>& child,
               const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      results->insert(results->begin(), std::next(child_results.begin()),
                      child_results.end());
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return child_->ReturnType(); }
};

class SkipFunction : public SingleValueFunctionNode {
 public:
  explicit SkipFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleValueFunctionNode(child, params) {}

  absl::Status EvaluateWithParam(
      WorkSpace* work_space, const WorkspaceMessage& param,
      std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));
    FHIR_ASSIGN_OR_RETURN(
        int num,
        ToSystemInteger(work_space->GetPrimitiveHandler(), *param.Message()));

    auto start = child_results.begin();
    std::advance(start, std::min(num < 0 ? 0 : static_cast<size_t>(num),
                                 child_results.size()));
    results->insert(results->begin(), start, child_results.end());

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return child_->ReturnType(); }
};

class TakeFunction : public SingleValueFunctionNode {
 public:
  explicit TakeFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleValueFunctionNode(child, params) {}

  absl::Status EvaluateWithParam(
      WorkSpace* work_space, const WorkspaceMessage& param,
      std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));
    FHIR_ASSIGN_OR_RETURN(
        int num,
        ToSystemInteger(work_space->GetPrimitiveHandler(), *param.Message()));

    auto end = child_results.begin();
    std::advance(end, std::min(num < 0 ? 0 : static_cast<size_t>(num),
                               child_results.size()));
    results->insert(results->begin(), child_results.begin(), end);

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return child_->ReturnType(); }
};

// Implements the FHIRPath .trace() function.
class TraceFunction : public SingleValueFunctionNode {
 public:
  TraceFunction(const std::shared_ptr<ExpressionNode>& child,
                const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleValueFunctionNode(child, params) {}

  absl::Status EvaluateWithParam(
      WorkSpace* work_space, const WorkspaceMessage& param,
      std::vector<WorkspaceMessage>* results) const override {
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, results));
    FHIR_ASSIGN_OR_RETURN(std::string name, MessageToString(param));

    DVLOG(1) << "trace(" << name << "):";
    for (auto it = results->begin(); it != results->end(); it++) {
      DVLOG(1) << (*it).Message()->DebugString();
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return child_->ReturnType(); }
};

// Implements the FHIRPath .toInteger() function.
class ToIntegerFunction : public ZeroParameterFunctionNode {
 public:
  ToIntegerFunction(const std::shared_ptr<ExpressionNode>& child,
                    const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return InvalidArgumentError(
          "toInteger() requires a collection with no more than 1 item.");
    }

    if (child_results.empty()) {
      return absl::OkStatus();
    }

    const WorkspaceMessage& child_result = child_results[0];

    if (!IsPrimitive(child_result.Message()->GetDescriptor())) {
      return absl::OkStatus();
    }

    if (IsInteger(*child_result.Message())) {
      results->push_back(child_result);
      return absl::OkStatus();
    }

    if (IsBoolean(*child_result.Message())) {
      FHIR_ASSIGN_OR_RETURN(bool value,
                            work_space->GetPrimitiveHandler()->GetBooleanValue(
                                *child_result.Message()));
      Message* result = work_space->GetPrimitiveHandler()->NewInteger(value);
      work_space->DeleteWhenFinished(result);
      results->push_back(WorkspaceMessage(result));
      return absl::OkStatus();
    }

    auto child_as_string = MessagesToString(child_results);
    if (child_as_string.ok()) {
      int32_t value;
      if (absl::SimpleAtoi(child_as_string.value(), &value)) {
        Message* result = work_space->GetPrimitiveHandler()->NewInteger(value);
        work_space->DeleteWhenFinished(result);
        results->push_back(WorkspaceMessage(result));
        return absl::OkStatus();
      }
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return Integer::descriptor();
  }
};

// Implements the FHIRPath .toBoolean() function.
class ToBooleanFunction : public ZeroParameterFunctionNode {
 public:
  ToBooleanFunction(const std::shared_ptr<ExpressionNode>& child,
                    const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return InvalidArgumentError(
          "toBoolean() requires a collection with no more than 1 item.");
    }

    if (child_results.empty()) {
      return absl::OkStatus();
    }

    const WorkspaceMessage& child_result = child_results[0];

    if (!IsPrimitive(child_result.Message()->GetDescriptor())) {
      return absl::OkStatus();
    }

    if (IsBoolean(*child_result.Message())) {
      results->push_back(child_result);
      return absl::OkStatus();
    }

    if (IsSystemInteger(*child_result.Message())) {
      FHIR_ASSIGN_OR_RETURN(int32_t value,
                            ToSystemInteger(work_space->GetPrimitiveHandler(),
                                            *child_result.Message()));
      if (value != 0 && value != 1) {
        return absl::OkStatus();
      }
      Message* result = work_space->GetPrimitiveHandler()->NewBoolean(value);
      work_space->DeleteWhenFinished(result);
      results->push_back(WorkspaceMessage(result));
      return absl::OkStatus();
    }

    if (IsSystemString(*child_result.Message())) {
      FHIR_ASSIGN_OR_RETURN(std::string value, MessageToString(child_result));
      bool is_true = value == "true" || value == "t" || value == "yes" ||
                     value == "y" || value == "1" || value == "1.0";
      bool is_false = value == "false" || value == "f" || value == "no" ||
                      value == "n" || value == "0" || value == "0.0";
      if (is_true == is_false) {
        return absl::OkStatus();
      }

      Message* result = work_space->GetPrimitiveHandler()->NewBoolean(is_true);
      work_space->DeleteWhenFinished(result);
      results->push_back(WorkspaceMessage(result));
      return absl::OkStatus();
    }

    if (IsSystemDecimal(*child_result.Message())) {
      double value;
      FHIR_RETURN_IF_ERROR(MessageToDouble(work_space->GetPrimitiveHandler(),
                                           *child_result.Message(), &value));
      bool is_true = value == 1.0;
      bool is_false = value == 0.0;
      if (is_true == is_false) {
        return absl::OkStatus();
      }

      Message* result = work_space->GetPrimitiveHandler()->NewBoolean(is_true);
      work_space->DeleteWhenFinished(result);
      results->push_back(WorkspaceMessage(result));
      return absl::OkStatus();
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }
};

// Base class for FHIRPath binary operators.
class BinaryOperator : public ExpressionNode {
 public:
  absl::Status Evaluate(WorkSpace* work_space,
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
  virtual absl::Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* out_results) const = 0;

  std::vector<std::shared_ptr<ExpressionNode>> GetChildren()
      const override {
    return {left_, right_};
  }

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

  absl::Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* out_results) const override {
    FHIR_ASSIGN_OR_RETURN(auto index,
                          (IntegerOrEmpty(primitive_handler_, right_results)));
    if (!index.has_value()) {
      return InvalidArgumentError("Index must be present.");
    }

    if (left_results.empty() || left_results.size() <= index.value()) {
      return absl::OkStatus();
    }

    out_results->push_back(left_results[index.value()]);
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return left_->ReturnType(); }

 private:
  const PrimitiveHandler* primitive_handler_;
};

class EqualsOperator : public BinaryOperator {
 public:
  absl::Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* out_results) const override {
    if (left_results.empty() || right_results.empty()) {
      return absl::OkStatus();
    }

    Message* result = work_space->GetPrimitiveHandler()->NewBoolean(AreEqual(
        work_space->GetPrimitiveHandler(), left_results, right_results));
    work_space->DeleteWhenFinished(result);
    out_results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
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
      absl::StatusOr<JsonPrimitive> left_primitive =
          primitive_handler->WrapPrimitiveProto(left);
      absl::StatusOr<JsonPrimitive> right_primitive =
          primitive_handler->WrapPrimitiveProto(right);

      // Comparisons between primitives and non-primitives are valid
      // in FHIRPath and should simply return false rather than an error.
      return left_primitive.ok() && right_primitive.ok() &&
             left_primitive.value().value == right_primitive.value().value;
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

    // TODO(b/148992850): This will crash on a non-STU3 or R4 primitive.
    // That's probably ok for now but we should fix this to never crash ASAP.
    if (IsPrimitive(message->GetDescriptor())) {
      return std::hash<std::string>{}(
          primitive_handler->WrapPrimitiveProto(*message).value().value);
    }

    return std::hash<std::string>{}(message->SerializeAsString());
  }
};

class UnionOperator : public BinaryOperator {
 public:
  UnionOperator(std::shared_ptr<ExpressionNode> left,
                std::shared_ptr<ExpressionNode> right)
      : BinaryOperator(std::move(left), std::move(right)) {}

  absl::Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* out_results) const override {
    std::unordered_set<WorkspaceMessage, ProtoPtrHash, ProtoPtrSameTypeAndEqual>
        seen_results(
            kDefaultSetBucketCount,
            ProtoPtrHash(work_space->GetPrimitiveHandler()),
            ProtoPtrSameTypeAndEqual(work_space->GetPrimitiveHandler()));
    for (const WorkspaceMessage& item : left_results) {
      if (seen_results.find(item) == seen_results.end()) {
        out_results->push_back(item);
        seen_results.insert(item);
      }
    }
    for (const WorkspaceMessage& item : right_results) {
      if (seen_results.find(item) == seen_results.end()) {
        out_results->push_back(item);
        seen_results.insert(item);
      }
    }
    return absl::OkStatus();
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

    // TODO(b/244184211): Consider refactoring ReturnType to return a set of all
    // types in the collection.
    return nullptr;
  }
};

// Implements the FHIRPath .isDistinct() function.
class IsDistinctFunction : public ZeroParameterFunctionNode {
 public:
  IsDistinctFunction(const std::shared_ptr<ExpressionNode>& child,
                     const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    absl::node_hash_set<WorkspaceMessage, ProtoPtrHash,
                        ProtoPtrSameTypeAndEqual>
        child_results_set(
            child_results.begin(), child_results.end(), kDefaultSetBucketCount,
            ProtoPtrHash(work_space->GetPrimitiveHandler()),
            ProtoPtrSameTypeAndEqual(work_space->GetPrimitiveHandler()));

    Message* result = work_space->GetPrimitiveHandler()->NewBoolean(
        child_results_set.size() == child_results.size());
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::GetDescriptor();
  }
};

// Implements the FHIRPath .distinct() function.
class DistinctFunction : public ZeroParameterFunctionNode {
 public:
  DistinctFunction(const std::shared_ptr<ExpressionNode>& child,
                   const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    std::unordered_set<WorkspaceMessage, ProtoPtrHash, ProtoPtrSameTypeAndEqual>
        result_set(child_results.begin(), child_results.end(),
                   kDefaultSetBucketCount,
                   ProtoPtrHash(work_space->GetPrimitiveHandler()),
                   ProtoPtrSameTypeAndEqual(work_space->GetPrimitiveHandler()));
    results->insert(results->begin(), result_set.begin(), result_set.end());
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return child_->ReturnType(); }
};

// Implements the FHIRPath .combine() function.
class CombineFunction : public SingleParameterFunctionNode {
 public:
  CombineFunction(const std::shared_ptr<ExpressionNode>& child,
                  const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        const std::vector<WorkspaceMessage>& first_param,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    results->insert(results->end(), child_results.begin(), child_results.end());
    results->insert(results->end(), first_param.begin(), first_param.end());
    return absl::OkStatus();
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

    // TODO(b/244184211): Consider refactoring ReturnType to return a set of all
    // types in the collection.
    return nullptr;
  }
};

// Factory method for creating FHIRPath's union() function.
absl::StatusOr<ExpressionNode*> static CreateUnionFunction(
    const std::shared_ptr<ExpressionNode>& child_expression,
    const std::vector<FhirPathParser::ExpressionContext*>& params,
    FhirPathBaseVisitor* base_context_visitor,
    FhirPathBaseVisitor* child_context_visitor) {
  if (params.size() != 1) {
    return InvalidArgumentError("union() requires exactly one argument.");
  }

  FHIR_ASSIGN_OR_RETURN(
      std::vector<std::shared_ptr<ExpressionNode>> compiled_params,
      FunctionNode::CompileParams(params, base_context_visitor));

  return new UnionOperator(child_expression, compiled_params[0]);
}

// Implements the FHIRPath .where() function.
class WhereFunction : public FunctionNode {
 public:
  static absl::Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    if (params.size() != 1) {
      return InvalidArgumentError("Function requires exactly one argument.");
    }

    return absl::OkStatus();
  }

  static absl::StatusOr<std::vector<std::shared_ptr<ExpressionNode>>>
  CompileParams(const std::vector<FhirPathParser::ExpressionContext*>& params,
                FhirPathBaseVisitor*,
                FhirPathBaseVisitor* child_context_visitor) {
    return FunctionNode::CompileParams(params, child_context_visitor);
  }

  WhereFunction(const std::shared_ptr<ExpressionNode>& child,
                const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {
    FHIR_DCHECK_OK(ValidateParams(params));
  }

  absl::Status Evaluate(WorkSpace* work_space,
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
          absl::StatusOr<absl::optional<bool>> allowed,
          (BooleanOrEmpty(work_space->GetPrimitiveHandler(), param_results)));
      if (allowed.value().value_or(false)) {
        results->push_back(message);
      }
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return child_->ReturnType(); }
};

// Factory method for creating FHIRPath's anyTrue() function.
absl::StatusOr<FunctionNode*> static CreateAnyTrueFunction(
    const std::shared_ptr<ExpressionNode>& child_expression,
    const std::vector<FhirPathParser::ExpressionContext*>& params,
    FhirPathBaseVisitor* base_context_visitor,
    FhirPathBaseVisitor* child_context_visitor) {
  if (!params.empty()) {
    return InvalidArgumentError("anyTrue() requires zero arguments.");
  }

  std::vector<std::shared_ptr<ExpressionNode>> where_params = {
      std::make_shared<ThisReference>(nullptr)};

  return new ExistsFunction(
      std::make_shared<WhereFunction>(child_expression, where_params), {});
}

// Factory method for creating FHIRPath's anyFalse() function.
absl::StatusOr<FunctionNode*> static CreateAnyFalseFunction(
    const std::shared_ptr<ExpressionNode>& child_expression,
    const std::vector<FhirPathParser::ExpressionContext*>& params,
    FhirPathBaseVisitor* base_context_visitor,
    FhirPathBaseVisitor* child_context_visitor) {
  if (!params.empty()) {
    return InvalidArgumentError("anyFalse() requires zero arguments.");
  }

  std::vector<std::shared_ptr<ExpressionNode>> where_params = {
      std::make_shared<NotFunction>(std::make_shared<ThisReference>(nullptr))};

  return new ExistsFunction(
      std::make_shared<WhereFunction>(child_expression, where_params), {});
}

// Implements the FHIRPath .all() function.
class AllFunction : public FunctionNode {
 public:
  static absl::Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    if (params.size() != 1) {
      return InvalidArgumentError("Function requires exactly one argument.");
    }

    return absl::OkStatus();
  }

  static absl::StatusOr<std::vector<std::shared_ptr<ExpressionNode>>>
  CompileParams(const std::vector<FhirPathParser::ExpressionContext*>& params,
                FhirPathBaseVisitor*,
                FhirPathBaseVisitor* child_context_visitor) {
    return FunctionNode::CompileParams(params, child_context_visitor);
  }

  AllFunction(const std::shared_ptr<ExpressionNode>& child,
              const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {
    FHIR_DCHECK_OK(ValidateParams(params));
  }

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));
    FHIR_ASSIGN_OR_RETURN(bool result, Evaluate(work_space, child_results));

    Message* result_message =
        work_space->GetPrimitiveHandler()->NewBoolean(result);
    work_space->DeleteWhenFinished(result_message);
    results->push_back(WorkspaceMessage(result_message));
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::GetDescriptor();
  }

 private:
  absl::StatusOr<bool> Evaluate(
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
          absl::StatusOr<absl::optional<bool>> criteria_met,
          (BooleanOrEmpty(work_space->GetPrimitiveHandler(), param_results)));
      if (!criteria_met.value().value_or(false)) {
        return false;
      }
    }

    return true;
  }
};

// Implements the FHIRPath .allTrue() function.
class AllTrueFunction : public AllFunction {
 public:
  static absl::Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    return params.empty()
               ? absl::OkStatus()
               : InvalidArgumentError("Function requires zero arguments.");
  }

  AllTrueFunction(const std::shared_ptr<ExpressionNode>& child,
                  const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : AllFunction(child, {std::make_shared<ThisReference>(nullptr)}) {}
};

// Implements the FHIRPath .allFalse() function.
class AllFalseFunction : public AllFunction {
 public:
  static absl::Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    return params.empty()
               ? absl::OkStatus()
               : InvalidArgumentError("Function requires zero arguments.");
  }

  AllFalseFunction(const std::shared_ptr<ExpressionNode>& child,
                   const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : AllFunction(child, {std::make_shared<NotFunction>(
                               std::make_shared<ThisReference>(nullptr))}) {}
};

// Implements the FHIRPath .select() function.
class SelectFunction : public FunctionNode {
 public:
  static absl::Status ValidateParams(
      const std::vector<std::shared_ptr<ExpressionNode>>& params) {
    if (params.size() != 1) {
      return InvalidArgumentError("Function requires exactly one argument.");
    }

    return absl::OkStatus();
  }

  static absl::StatusOr<std::vector<std::shared_ptr<ExpressionNode>>>
  CompileParams(const std::vector<FhirPathParser::ExpressionContext*>& params,
                FhirPathBaseVisitor*,
                FhirPathBaseVisitor* child_context_visitor) {
    return FunctionNode::CompileParams(params, child_context_visitor);
  }

  SelectFunction(const std::shared_ptr<ExpressionNode>& child,
                 const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {
    FHIR_DCHECK_OK(ValidateParams(params));
  }

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    for (const WorkspaceMessage& message : child_results) {
      work_space->PushMessageContext(message);
      absl::Status status = params_[0]->Evaluate(work_space, results);
      work_space->PopMessageContext();
      FHIR_RETURN_IF_ERROR(status);
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return params_[0]->ReturnType();
  }
};

// Implements the FHIRPath .iif() function.
class IifFunction : public FunctionNode {
 public:
  static absl::StatusOr<std::vector<std::shared_ptr<ExpressionNode>>>
  CompileParams(const std::vector<FhirPathParser::ExpressionContext*>& params,
                FhirPathBaseVisitor* base_context_visitor,
                FhirPathBaseVisitor* child_context_visitor) {
    if (params.size() < 2 || params.size() > 3) {
      return InvalidArgumentError("iif() requires 2 or 3 arguments.");
    }

    std::vector<std::shared_ptr<ExpressionNode>> compiled_params;

    antlrcpp::Any criterion = params[0]->accept(child_context_visitor);
    if (!AnyHasValue(criterion)) {
      return InvalidArgumentError("Failed to compile parameter.");
    }
    compiled_params.push_back(
        AnyCast<std::shared_ptr<ExpressionNode>>(criterion));

    antlrcpp::Any true_result = params[1]->accept(base_context_visitor);
    if (!AnyHasValue(true_result)) {
      return InvalidArgumentError("Failed to compile parameter.");
    }
    compiled_params.push_back(
        AnyCast<std::shared_ptr<ExpressionNode>>(true_result));

    if (params.size() > 2) {
      antlrcpp::Any otherwise_result = params[2]->accept(base_context_visitor);
      if (!AnyHasValue(otherwise_result)) {
        return InvalidArgumentError("Failed to compile parameter.");
      }
      compiled_params.push_back(
          AnyCast<std::shared_ptr<ExpressionNode>>(otherwise_result));
    }

    return compiled_params;
  }

  IifFunction(const std::shared_ptr<ExpressionNode>& child,
              const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : FunctionNode(child, params) {
    FHIR_DCHECK_OK(ValidateParams(params));
  }

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return InvalidArgumentError(
          "iif() requires a collection with no more than 1 item.");
    }

    if (child_results.empty()) {
      return absl::OkStatus();
    }

    const WorkspaceMessage& child = child_results[0];

    std::vector<WorkspaceMessage> param_results;
    WorkSpace expression_work_space(work_space->GetPrimitiveHandler(),
                                    work_space->MessageContextStack(), child);
    FHIR_RETURN_IF_ERROR(
        params_[0]->Evaluate(&expression_work_space, &param_results));
    FHIR_ASSIGN_OR_RETURN(
        absl::StatusOr<absl::optional<bool>> criterion_met,
        (BooleanOrEmpty(work_space->GetPrimitiveHandler(), param_results)));
    if (criterion_met.value().value_or(false)) {
      FHIR_RETURN_IF_ERROR(params_[1]->Evaluate(work_space, results));
    } else if (params_.size() > 2) {
      FHIR_RETURN_IF_ERROR(params_[2]->Evaluate(work_space, results));
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return child_->ReturnType(); }
};

// Factory method for creating FHIRPath's convertsTo*() function. Type parameter
// T should be the ExpressionNode type that converts to the type in question.
template <typename T>
absl::StatusOr<ExpressionNode*> static CreateConvertsToFunction(
    const std::shared_ptr<ExpressionNode>& child_expression,
    const std::vector<FhirPathParser::ExpressionContext*>& params,
    FhirPathBaseVisitor* base_context_visitor,
    FhirPathBaseVisitor* child_context_visitor) {
  if (!params.empty()) {
    return InvalidArgumentError("convertsTo*() requires zero arguments.");
  }

  // .convertsTo*() is equivalent to .single().select($this.to*().exists()).
  std::vector<std::shared_ptr<ExpressionNode>> empty_params = {};
  return new SelectFunction(
      std::make_shared<SingleFunction>(child_expression, empty_params),
      {std::make_shared<ExistsFunction>(
          std::make_shared<T>(
              std::make_shared<ThisReference>(child_expression->ReturnType()),
              empty_params),
          empty_params)});
}

// Implements the FHIRPath .ofType() function.
//
// TODO(b/244184211): This does not currently validate that the tested type
// exists. According to the FHIRPath spec, if the type does not exist the
// expression should throw an error instead of returning false.
//
// TODO(b/244184211): Handle type namespaces (i.e. FHIR.* and System.*)
//
// TODO(b/244184211): Handle type inheritance correctly. For example, a Patient
// resource is a DomainResource, but this function, as is, will filter out the
// Patient if ofType(DomainResource) is used.
class OfTypeFunction : public ExpressionNode {
 public:
  absl::StatusOr<OfTypeFunction*> static Create(
      const std::shared_ptr<ExpressionNode>& child_expression,
      const std::vector<FhirPathParser::ExpressionContext*>& params,
      FhirPathBaseVisitor* base_context_visitor,
      FhirPathBaseVisitor* child_context_visitor) {
    if (params.size() != 1) {
      return InvalidArgumentError("ofType() requires a single argument.");
    }

    return new OfTypeFunction(child_expression, params[0]->getText());
  }

  OfTypeFunction(const std::shared_ptr<ExpressionNode>& child,
                 std::string type_name)
      : child_(child), type_name_(type_name) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    for (const WorkspaceMessage& message : child_results) {
      if (absl::EqualsIgnoreCase(message.Message()->GetDescriptor()->name(),
                                 type_name_)) {
        results->push_back(message);
      }
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    // TODO(b/244184211): Fetch the descriptor based on this->type_name_.
    return nullptr;
  }

  std::vector<std::shared_ptr<ExpressionNode>> GetChildren()
      const override {
    return {child_};
  }

 private:
  const std::shared_ptr<ExpressionNode> child_;
  const std::string type_name_;
};

// Implements the FHIRPath .is() function.
//
// TODO(b/244184211): This does not currently validate that the tested type
// exists. According to the FHIRPath spec, if the type does not exist the
// expression should throw an error instead of returning false.
//
// TODO(b/244184211): Handle type namespaces (i.e. FHIR.* and System.*)
//
// TODO(b/244184211): Handle type inheritance correctly. For example, a Patient
// resource is a DomainResource, but this function, as is, will return false.
class IsFunction : public ExpressionNode {
 public:
  absl::StatusOr<IsFunction*> static Create(
      const std::shared_ptr<ExpressionNode>& child_expression,
      const std::vector<FhirPathParser::ExpressionContext*>& params,
      FhirPathBaseVisitor* base_context_visitor,
      FhirPathBaseVisitor* child_context_visitor) {
    if (params.size() != 1) {
      return InvalidArgumentError("is() requires a single argument.");
    }

    return new IsFunction(child_expression, params[0]->getText());
  }

  IsFunction(const std::shared_ptr<ExpressionNode>& child,
             std::string type_name)
      : child_(child), type_name_(type_name) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return InvalidArgumentError(
          "is() requires a collection with no more than 1 item.");
    }

    if (child_results.empty()) {
      return absl::OkStatus();
    }

    Message* result =
        work_space->GetPrimitiveHandler()->NewBoolean(absl::EqualsIgnoreCase(
            child_results[0].Message()->GetDescriptor()->name(), type_name_));
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::GetDescriptor();
  }

  std::vector<std::shared_ptr<ExpressionNode>> GetChildren()
      const override {
    return {child_};
  }

 private:
  const std::shared_ptr<ExpressionNode> child_;
  const std::string type_name_;
};

// Implements the FHIRPath .as() function.
//
// TODO(b/244184211): This does not currently validate that the tested type
// exists. According to the FHIRPath spec, if the type does not exist the
// expression should throw an error.
//
// TODO(b/244184211): Handle type namespaces (i.e. FHIR.* and System.*)
//
// TODO(b/244184211): Handle type inheritance correctly. For example, a Patient
// resource is a DomainResource, but this function, as is, will behave as if
// a Patient is not a DomainResource and return an empty collection.
class AsFunction : public ExpressionNode {
 public:
  absl::StatusOr<AsFunction*> static Create(
      const std::shared_ptr<ExpressionNode>& child_expression,
      const std::vector<FhirPathParser::ExpressionContext*>& params,
      FhirPathBaseVisitor* base_context_visitor,
      FhirPathBaseVisitor* child_context_visitor) {
    if (params.size() != 1) {
      return InvalidArgumentError("as() requires a single argument.");
    }

    return new AsFunction(child_expression, params[0]->getText());
  }

  AsFunction(const std::shared_ptr<ExpressionNode>& child,
             std::string type_name)
      : child_(child), type_name_(type_name) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    if (child_results.size() > 1) {
      return InvalidArgumentError(
          "as() requires a collection with no more than 1 item.");
    }

    if (!child_results.empty() &&
        absl::EqualsIgnoreCase(
            child_results[0].Message()->GetDescriptor()->name(), type_name_)) {
      results->push_back(child_results[0]);
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    // TODO(b/244184211): Fetch the descriptor based on this->type_name_.
    return nullptr;
  }

  std::vector<std::shared_ptr<ExpressionNode>> GetChildren()
      const override {
    return {child_};
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

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    for (const WorkspaceMessage& child : child_results) {
      const Descriptor* descriptor = child.Message()->GetDescriptor();
      if (IsPrimitive(descriptor)) {
        continue;
      }
      for (int i = 0; i < descriptor->field_count(); i++) {
        std::vector<const Message*> messages;
        FHIR_RETURN_IF_ERROR(
            RetrieveField(*child.Message(), *descriptor->field(i),
                          MakeWorkSpaceMessageFactory(work_space), &messages));
        for (const Message* message : messages) {
          results->push_back(WorkspaceMessage(child, message));
        }
      }
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return nullptr; }
};

class DescendantsFunction : public ZeroParameterFunctionNode {
 public:
  DescendantsFunction(
      const std::shared_ptr<ExpressionNode>& child,
      const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : ZeroParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    for (const WorkspaceMessage& child : child_results) {
      FHIR_RETURN_IF_ERROR(AppendDescendants(child, work_space, results));
    }

    return absl::OkStatus();
  }

  absl::Status AppendDescendants(const WorkspaceMessage& parent,
                                 WorkSpace* work_space,
                                 std::vector<WorkspaceMessage>* results) const {
    const Descriptor* descriptor = parent.Message()->GetDescriptor();
    if (IsPrimitive(descriptor)) {
      return absl::OkStatus();
    }

    for (int i = 0; i < descriptor->field_count(); i++) {
      std::vector<const Message*> messages;
      FHIR_RETURN_IF_ERROR(
          RetrieveField(*parent.Message(), *descriptor->field(i),
                        MakeWorkSpaceMessageFactory(work_space), &messages));
      for (const Message* message : messages) {
        WorkspaceMessage child(parent, message);
        results->push_back(child);
        FHIR_RETURN_IF_ERROR(AppendDescendants(child, work_space, results));
      }
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return nullptr; }
};

// Implements the FHIRPath .intersect() function.
class IntersectFunction : public SingleParameterFunctionNode {
 public:
  IntersectFunction(const std::shared_ptr<ExpressionNode>& child,
                    const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleParameterFunctionNode(child, params) {}

  absl::Status Evaluate(WorkSpace* work_space,
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

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    if (child_->ReturnType() == nullptr ||
        params_[0]->ReturnType() == nullptr) {
      return nullptr;
    }

    if (AreSameMessageType(child_->ReturnType(), params_[0]->ReturnType())) {
      return child_->ReturnType();
    }

    // TODO(b/244184211): Consider refactoring ReturnType to return a set of all
    // types in the collection.
    return nullptr;
  }
};

// memberOf(valueset : string) : Boolean
// When invoked on a code-valued element, returns true if the code is a member
// of the given valueset.
// When invoked on a concept-valued element, returns true if any code in the
// concept is a member of the given valueset.
// When invoked on a string, returns true if the string is equal to a code
// in the valueset, so long as the valueset only contains one codesystem.
// If the valueset in this case contains more than one codesystem, an error
// is thrown.
//
// If the valueset cannot be resolved as a uri to a value set, an error
// is thrown.
class MemberOfFunction : public SingleValueFunctionNode {
 public:
  MemberOfFunction(const std::shared_ptr<ExpressionNode>& child,
                   const std::vector<std::shared_ptr<ExpressionNode>>& params)
      : SingleValueFunctionNode(child, params) {}

  static absl::StatusOr<bool> StringIsMember(const std::string& code_str,
                                             const std::string& value_set,
                                             WorkSpace* work_space) {
    FHIR_ASSIGN_OR_RETURN(
        const bool has_multiple_code_systems,
        work_space->GetTerminologyResolver()->ValueSetHasMultipleCodeSystems(
            value_set));
    if (has_multiple_code_systems) {
      return absl::FailedPreconditionError(
          absl::StrFormat("Value set %s contains more than one code system, "
                          "hence cannot be evaluated on a string value.",
                          value_set));
    }
    return work_space->GetTerminologyResolver()->IsCodeInValueSet(code_str,
                                                                  value_set);
  }

  static absl::StatusOr<bool> CodeIsMember(
      const Message& code_object,
      const absl::optional<std::string>& code_system,
      const std::string& value_set, WorkSpace* work_space) {
    FHIR_ASSIGN_OR_RETURN(std::string code_str, GetCodeAsString(code_object));
    if (code_system.has_value()) {
      return work_space->GetTerminologyResolver()->IsCodeInValueSet(
          code_str, *code_system, value_set);
    } else {
      return work_space->GetTerminologyResolver()->IsCodeInValueSet(code_str,
                                                                    value_set);
    }
  }

  static absl::StatusOr<bool> ConceptIsMember(const Message& concept_object,
                                              const std::string& value_set,
                                              WorkSpace* work_space) {
    const ::google::protobuf::FieldDescriptor* coding_field =
        concept_object.GetDescriptor()->FindFieldByName("coding");
    const ::google::protobuf::FieldDescriptor* code_field = nullptr;
    const ::google::protobuf::FieldDescriptor* system_field = nullptr;
    if (coding_field == nullptr || !coding_field->is_repeated()) {
      return absl::FailedPreconditionError(
          "`concept_object` is not a valid CodeableConcept");
    }
    auto reflection = concept_object.GetReflection();
    for (int i = 0; i < reflection->FieldSize(concept_object, coding_field);
         ++i) {
      const Message& coding =
          reflection->GetRepeatedMessage(concept_object, coding_field, i);
      if (code_field == nullptr) {
        code_field = coding.GetDescriptor()->FindFieldByName("code");
        if (code_field == nullptr || code_field->is_repeated()) {
          return absl::FailedPreconditionError(
              "`concept_object.coding` is not a valid Coding");
        }
      }
      if (system_field == nullptr) {
        system_field = coding.GetDescriptor()->FindFieldByName("system");
      }
      absl::optional<std::string> system = absl::nullopt;
      if (system_field != nullptr &&
          coding.GetReflection()->HasField(coding, system_field)) {
        FHIR_ASSIGN_OR_RETURN(
            system, MessageToString(coding.GetReflection()->GetMessage(
                        coding, system_field)));
      }
      const Message& code =
          coding.GetReflection()->GetMessage(coding, code_field);
      FHIR_ASSIGN_OR_RETURN(bool is_member,
                            CodeIsMember(code, system, value_set, work_space));
      if (is_member) {
        return true;
      }
    }
    return false;
  }

  absl::Status EvaluateWithParam(
      WorkSpace* work_space, const WorkspaceMessage& param,
      std::vector<WorkspaceMessage>* results) const override {
    if (work_space->GetTerminologyResolver() == nullptr) {
      return absl::FailedPreconditionError(
          "memberOf() can only be called when `terminology_resolver` has been "
          "supplied to CompiledExpression::Compile()");
    }

    FHIR_ASSIGN_OR_RETURN(std::string value_set_id, MessageToString(param));
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    std::vector<bool> membership_results;
    for (const auto& element : child_results) {
      if (IsCodeableConcept(*element.Message())) {
        FHIR_ASSIGN_OR_RETURN(
            bool is_member,
            ConceptIsMember(*element.Message(), value_set_id, work_space));
        membership_results.push_back(is_member);
      } else if (IsCode(*element.Message())) {
        FHIR_ASSIGN_OR_RETURN(
            bool is_member,
            CodeIsMember(*element.Message(), /*code_system=*/absl::nullopt,
                         value_set_id, work_space));
        membership_results.push_back(is_member);
      } else if (IsString(*element.Message())) {
        FHIR_ASSIGN_OR_RETURN(std::string code_str, MessageToString(element));
        FHIR_ASSIGN_OR_RETURN(
            bool is_member, StringIsMember(code_str, value_set_id, work_space));
        membership_results.push_back(is_member);
      } else {
        return absl::InvalidArgumentError(absl::StrFormat(
            "`memberOf(valueset : string)` is not supported on `%s` types.",
            element.Message()->GetDescriptor()->full_name()));
      }
    }
    for (bool is_member : membership_results) {
      Message* result =
          work_space->GetPrimitiveHandler()->NewBoolean(is_member);
      work_space->DeleteWhenFinished(result);
      results->push_back(WorkspaceMessage(result));
    }
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }
};

// Implements a custom user-defined function.
class CustomFunction : public FunctionNode {
 public:
  absl::StatusOr<CustomFunction*> static Create(
      const std::shared_ptr<ExpressionNode>& child_expression,
      const std::vector<FhirPathParser::ExpressionContext*>& params,
      const UserDefinedFunction* user_defined_function, const int32_t id,
      FhirPathBaseVisitor* base_context_visitor,
      FhirPathBaseVisitor* child_context_visitor,
      const PrimitiveHandler* primitive_handler) {
    FHIR_ASSIGN_OR_RETURN(
        std::vector<std::shared_ptr<ExpressionNode>> compiled_params,
        CompileParams(user_defined_function, params, base_context_visitor,
                      child_context_visitor, primitive_handler));

    FHIR_RETURN_IF_ERROR(
        user_defined_function->ValidateParams(compiled_params));
    return new CustomFunction(child_expression, compiled_params,
                              user_defined_function, id);
  }

  static absl::StatusOr<std::vector<std::shared_ptr<ExpressionNode>>>
  CompileParams(const UserDefinedFunction* function,
                const std::vector<FhirPathParser::ExpressionContext*>& params,
                FhirPathBaseVisitor* base_context_visitor,
                FhirPathBaseVisitor* child_context_visitor,
                const PrimitiveHandler* primitive_handler) {
    if (function->GetParamTypes().size() != params.size()) {
      return InvalidArgumentError(
          absl::StrFormat("%s() requires %d arguments.", function->GetName(),
                          function->GetParamTypes().size()));
    }
    std::vector<std::shared_ptr<ExpressionNode>> compiled_params;

    for (int i = 0; i < params.size(); ++i) {
      antlrcpp::Any param_any;
      switch (function->GetParamTypes()[i]) {
        case UserDefinedFunction::ParameterVisitorType::kBase:
          param_any = params[i]->accept(base_context_visitor);
          break;
        case UserDefinedFunction::ParameterVisitorType::kChild:
          param_any = params[i]->accept(child_context_visitor);
          break;
        case UserDefinedFunction::ParameterVisitorType::kIdentifier:
          std::string identifier = params[i]->getText();
          param_any = ToAny(std::make_shared<Literal>(
              primitive_handler->StringDescriptor(),
              [primitive_handler, identifier]() {
                return primitive_handler->NewString(identifier);
              }));
      }
      if (!AnyHasValue(param_any)) {
        return InvalidArgumentError("Failed to compile parameter.");
      }
      compiled_params.push_back(
          AnyCast<std::shared_ptr<ExpressionNode>>(param_any));
    }

    return compiled_params;
  }

  CustomFunction(const std::shared_ptr<ExpressionNode>& child,
                 const std::vector<std::shared_ptr<ExpressionNode>>& params,
                 const UserDefinedFunction* function, int32_t id)
      : FunctionNode(child, params), function_(function), id_(id) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    if (GetCallbackFunctionName().has_value()) {
      auto cached_results = work_space->GetResultsCache().find(id_);
      if (cached_results != work_space->GetResultsCache().end()) {
        results->insert(results->end(), cached_results->second.begin(),
                        cached_results->second.end());
        return absl::OkStatus();
      } else {
        work_space->SetNextCacheNodeId(id_);
      }
    }
    std::vector<WorkspaceMessage> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));
    return function_->Evaluate(work_space, child_results, results, params_);
  }

  const Descriptor* ReturnType() const override { return nullptr; }


  // If the node represents a callback function, it's name is returned.
  // Otherwise, an empty response is returned.
  std::optional<std::string> GetCallbackFunctionName() const {
    if (function_->IsCallbackFunction()) {
      return function_->GetName();
    }
    return std::nullopt;
  }

  int32_t GetId() const { return id_; }

 private:
  const UserDefinedFunction* function_;
  int32_t id_;
};

class ComparisonOperator : public BinaryOperator {
 public:
  // Types of comparisons supported by this operator.
  enum ComparisonType {
    kLessThan,
    kGreaterThan,
    kLessThanEqualTo,
    kGreaterThanEqualTo
  };

  absl::Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* out_results) const override {
    // Per the FHIRPath spec, comparison operators propagate empty results.
    if (left_results.empty() || right_results.empty()) {
      return absl::OkStatus();
    }

    if (left_results.size() > 1 || right_results.size() > 1) {
      return InvalidArgumentError(
          "Comparison operators must have one element on each side.");
    }

    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> result,
                          EvalComparison(work_space->GetPrimitiveHandler(),
                                         left_results[0], right_results[0]));

    if (result.has_value()) {
      Message* result_message =
          work_space->GetPrimitiveHandler()->NewBoolean(result.value());
      work_space->DeleteWhenFinished(result_message);
      out_results->push_back(WorkspaceMessage(result_message));
    }
    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }

  ComparisonOperator(std::shared_ptr<ExpressionNode> left,
                     std::shared_ptr<ExpressionNode> right,
                     ComparisonType comparison_type)
      : BinaryOperator(left, right), comparison_type_(comparison_type) {}

 private:
  absl::StatusOr<absl::optional<bool>> EvalComparison(
      const PrimitiveHandler* primitive_handler, const WorkspaceMessage& left,
      const WorkspaceMessage& right) const {
    const Message* left_result = left.Message();
    const Message* right_result = right.Message();

    if (IsSystemInteger(*left_result) && IsSystemInteger(*right_result)) {
      return EvalIntegerComparison(
          ToSystemInteger(primitive_handler, *left_result).value(),
          ToSystemInteger(primitive_handler, *right_result).value());
    } else if (IsDecimal(*left_result) || IsDecimal(*right_result)) {
      return EvalDecimalComparison(primitive_handler, left_result,
                                   right_result);

    } else if (IsSystemString(*left_result) && IsSystemString(*right_result)) {
      return EvalStringComparison(primitive_handler, left, right);
    } else if ((IsInstant(*left_result) || IsDateTime(*left_result)) &&
               (IsInstant(*right_result) || IsDateTime(*right_result))) {
      return EvalDateTimeComparison(primitive_handler, *left_result,
                                    *right_result);
    } else if (IsSimpleQuantity(*left_result) &&
               IsSimpleQuantity(*right_result)) {
      return EvalSimpleQuantityComparison(primitive_handler, *left_result,
                                          *right_result);
    } else {
      return InvalidArgumentError(absl::StrCat(
          "Unsupported comparison value types: ", left_result->GetTypeName(),
          " and ", right_result->GetTypeName()));
    }
  }

  absl::optional<bool> EvalIntegerComparison(int32_t left,
                                             int32_t right) const {
    switch (comparison_type_) {
      case kLessThan:
        return absl::make_optional(left < right);
      case kGreaterThan:
        return absl::make_optional(left > right);
      case kLessThanEqualTo:
        return absl::make_optional(left <= right);
      case kGreaterThanEqualTo:
        return absl::make_optional(left >= right);
    }
  }

  absl::StatusOr<absl::optional<bool>> EvalDecimalComparison(
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
        return absl::make_optional(left < right);
      case kGreaterThan:
        return absl::make_optional(left > right);
      case kLessThanEqualTo:
        // Fallback to literal comparison for equality to avoid
        // rounding errors.
        return absl::make_optional(
            left <= right ||
            (left_message->GetDescriptor() == right_message->GetDescriptor() &&
             MessageDifferencer::Equals(*left_message, *right_message)));
      case kGreaterThanEqualTo:
        // Fallback to literal comparison for equality to avoid
        // rounding errors.
        return absl::make_optional(
            left >= right ||
            (left_message->GetDescriptor() == right_message->GetDescriptor() &&
             MessageDifferencer::Equals(*left_message, *right_message)));
    }

    return absl::OkStatus();
  }

  absl::StatusOr<absl::optional<bool>> EvalStringComparison(
      const PrimitiveHandler* primitive_handler,
      const WorkspaceMessage& left_message,
      const WorkspaceMessage& right_message) const {
    FHIR_ASSIGN_OR_RETURN(const std::string left,
                          MessageToString(left_message));
    FHIR_ASSIGN_OR_RETURN(const std::string right,
                          MessageToString(right_message));

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
        return absl::make_optional(compare_result < 0);
      case kGreaterThan:
        return absl::make_optional(compare_result > 0);
      case kLessThanEqualTo:
        return absl::make_optional(compare_result <= 0);
      case kGreaterThanEqualTo:
        return absl::make_optional(compare_result >= 0);
    }
  }

  static DateTimePrecision NormalizePrecisionForComparison(
      DateTimePrecision original) {
    if (original == DateTimePrecision::kMillisecond ||
        original == DateTimePrecision::kMicrosecond) {
      return DateTimePrecision::kSecond;
    }
    return original;
  }

  absl::StatusOr<absl::optional<bool>> EvalDateTimeComparison(
      const PrimitiveHandler* primitive_handler, const Message& left_message,
      const Message& right_message) const {
    const Message* left = &left_message;
    const Message* right = &right_message;
    std::vector<std::unique_ptr<const Message>> to_delete;
    if (IsInstant(left_message)) {
      FHIR_ASSIGN_OR_RETURN(
          JsonPrimitive json_primitive,
          primitive_handler->WrapPrimitiveProto(left_message));
      json_primitive.value = absl::StripPrefix(json_primitive.value, "\"");
      json_primitive.value = absl::StripSuffix(json_primitive.value, "\"");
      FHIR_ASSIGN_OR_RETURN(
          left, primitive_handler->NewDateTime(json_primitive.value));
      to_delete.push_back(std::unique_ptr<const Message>(left));
    }
    if (IsInstant(right_message)) {
      FHIR_ASSIGN_OR_RETURN(
          JsonPrimitive json_primitive,
          primitive_handler->WrapPrimitiveProto(right_message));
      json_primitive.value = absl::StripPrefix(json_primitive.value, "\"");
      json_primitive.value = absl::StripSuffix(json_primitive.value, "\"");
      FHIR_ASSIGN_OR_RETURN(
          right, primitive_handler->NewDateTime(json_primitive.value));
      to_delete.push_back(std::unique_ptr<const Message>(right));
    }
    FHIR_ASSIGN_OR_RETURN(DateTimePrecision left_precision,
                          primitive_handler->GetDateTimePrecision(*left));
    FHIR_ASSIGN_OR_RETURN(DateTimePrecision right_precision,
                          primitive_handler->GetDateTimePrecision(*right));

    // The FHIRPath spec (http://hl7.org/fhirpath/#comparison) states that "If
    // one value is specified to a different level of precision than the other,
    // the result is empty ({ }) to indicate that the result of the comparison
    // is unknown."
    if (NormalizePrecisionForComparison(left_precision) !=
        NormalizePrecisionForComparison(right_precision)) {
      return absl::optional<bool>();
    }

    FHIR_ASSIGN_OR_RETURN(absl::Time left_time,
                          primitive_handler->GetDateTimeValue(*left));
    FHIR_ASSIGN_OR_RETURN(absl::Time right_time,
                          primitive_handler->GetDateTimeValue(*right));

    // negative if left < right, positive if left > right, 0 if equal
    absl::civil_diff_t time_difference =
        absl::ToInt64Microseconds(left_time - right_time);

    switch (comparison_type_) {
      case kLessThan:
        return absl::make_optional(time_difference < 0);
      case kGreaterThan:
        return absl::make_optional(time_difference > 0);
      case kLessThanEqualTo:
        return absl::make_optional(time_difference <= 0);
      case kGreaterThanEqualTo:
        return absl::make_optional(time_difference >= 0);
    }
  }

  absl::StatusOr<absl::optional<bool>> EvalSimpleQuantityComparison(
      const PrimitiveHandler* primitive_handler, const Message& left_wrapper,
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
      return InvalidArgumentError(
          absl::StrCat("Compared quantities must have the same units. Got ",
                       "[", left_code, ", ", left_system, "] and ", "[",
                       right_code, ", ", right_system, "]"));
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
  absl::Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* out_results) const override {
    // Per the FHIRPath spec, comparison operators propagate empty results.
    if (left_results.empty() || right_results.empty()) {
      return absl::OkStatus();
    }

    if (left_results.size() > 1 || right_results.size() > 1) {
      return InvalidArgumentError(
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
    } else if (IsSystemString(*left_result) && IsSystemString(*right_result)) {
      FHIR_ASSIGN_OR_RETURN(
          std::string value,
          EvalStringAddition(left_results[0], right_results[0]));
      Message* result = work_space->GetPrimitiveHandler()->NewString(value);
      work_space->DeleteWhenFinished(result);
      out_results->push_back(WorkspaceMessage(result));
    } else {
      // TODO(b/244184211): Add implementation for Date, DateTime, Time, and
      // Decimal addition.
      return InvalidArgumentError(absl::StrCat(
          "Addition not supported for ", left_result->GetTypeName(), " and ",
          right_result->GetTypeName()));
    }

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override { return left_->ReturnType(); }

  AdditionOperator(std::shared_ptr<ExpressionNode> left,
                   std::shared_ptr<ExpressionNode> right)
      : BinaryOperator(std::move(left), std::move(right)) {}

 private:
  absl::StatusOr<int32_t> EvalIntegerAddition(
      const PrimitiveHandler* primitive_handler, const Message& left_wrapper,
      const Message& right_wrapper) const {
    FHIR_ASSIGN_OR_RETURN(int32_t left,
                          ToSystemInteger(primitive_handler, left_wrapper));
    FHIR_ASSIGN_OR_RETURN(int32_t right,
                          ToSystemInteger(primitive_handler, right_wrapper));
    return left + right;
  }

  absl::StatusOr<std::string> EvalStringAddition(
      const WorkspaceMessage& left_message,
      const WorkspaceMessage& right_message) const {
    FHIR_ASSIGN_OR_RETURN(std::string left, MessageToString(left_message));
    FHIR_ASSIGN_OR_RETURN(std::string right, MessageToString(right_message));

    return absl::StrCat(left, right);
  }
};

// Implementation for FHIRPath's string concatenation operator (&).
class StrCatOperator : public BinaryOperator {
 public:
  absl::Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* out_results) const override {
    if (left_results.size() > 1 || right_results.size() > 1) {
      return InvalidArgumentError(
          "String concatenation operators must have one element "
          "on each side.");
    }

    std::string left;
    std::string right;

    if (!left_results.empty()) {
      FHIR_ASSIGN_OR_RETURN(left, MessageToString(left_results[0]));
    }
    if (!right_results.empty()) {
      FHIR_ASSIGN_OR_RETURN(right, MessageToString(right_results[0]));
    }

    Message* result =
        work_space->GetPrimitiveHandler()->NewString(absl::StrCat(left, right));
    work_space->DeleteWhenFinished(result);
    out_results->push_back(WorkspaceMessage(result));
    return absl::OkStatus();
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

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    std::vector<WorkspaceMessage> operand_result;
    FHIR_RETURN_IF_ERROR(operand_->Evaluate(work_space, &operand_result));

    if (operand_result.size() > 1) {
      return InvalidArgumentError(
          "Polarity operators must operate on a single element.");
    }

    if (operand_result.empty()) {
      return absl::OkStatus();
    }

    const WorkspaceMessage& operand_value = operand_result[0];

    if (operation_ == kPositive) {
      results->push_back(operand_value);
      return absl::OkStatus();
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
      return absl::OkStatus();
    }

    if (IsSystemInteger(*operand_value.Message())) {
      FHIR_ASSIGN_OR_RETURN(int32_t value,
                            ToSystemInteger(work_space->GetPrimitiveHandler(),
                                            *operand_value.Message()));
      Message* result =
          work_space->GetPrimitiveHandler()->NewInteger(value * -1);
      work_space->DeleteWhenFinished(result);
      results->push_back(WorkspaceMessage(result));
      return absl::OkStatus();
    }

    return InvalidArgumentError(
        "Polarity operators must operate on a decimal or integer type.");
  }

  const Descriptor* ReturnType() const override {
    return operand_->ReturnType();
  }

  std::vector<std::shared_ptr<ExpressionNode>> GetChildren() const override {
    return {operand_};
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

  std::vector<std::shared_ptr<ExpressionNode>> GetChildren() const override {
    return {left_, right_};
  }

 protected:
  void SetResult(bool eval_result, WorkSpace* work_space,
                 std::vector<WorkspaceMessage>* results) const {
    Message* result =
        work_space->GetPrimitiveHandler()->NewBoolean(eval_result);
    work_space->DeleteWhenFinished(result);
    results->push_back(WorkspaceMessage(result));
  }

  absl::StatusOr<absl::optional<bool>> EvaluateBooleanNode(
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

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> left_result,
                          EvaluateBooleanNode(left_, work_space));

    // Short circuit evaluation when left_result == "false"
    if (left_result.has_value() && !left_result.value()) {
      SetResult(true, work_space, results);
      return absl::OkStatus();
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

    return absl::OkStatus();
  }
};

class XorOperator : public BooleanOperator {
 public:
  XorOperator(std::shared_ptr<ExpressionNode> left,
              std::shared_ptr<ExpressionNode> right)
      : BooleanOperator(left, right) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    // Logic from truth table spec: http://hl7.org/fhirpath/#boolean-logic
    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> left_result,
                          EvaluateBooleanNode(left_, work_space));
    if (!left_result.has_value()) {
      return absl::OkStatus();
    }

    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> right_result,
                          EvaluateBooleanNode(right_, work_space));
    if (!right_result.has_value()) {
      return absl::OkStatus();
    }

    SetResult(left_result.value() != right_result.value(), work_space, results);
    return absl::OkStatus();
  }
};

class OrOperator : public BooleanOperator {
 public:
  OrOperator(std::shared_ptr<ExpressionNode> left,
             std::shared_ptr<ExpressionNode> right)
      : BooleanOperator(left, right) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    // Logic from truth table spec: http://hl7.org/fhirpath/#boolean-logic
    // Short circuit and return true on the first true result.
    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> left_result,
                          EvaluateBooleanNode(left_, work_space));
    if (left_result.has_value() && left_result.value()) {
      SetResult(true, work_space, results);
      return absl::OkStatus();
    }

    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> right_result,
                          EvaluateBooleanNode(right_, work_space));
    if (right_result.has_value() && right_result.value()) {
      SetResult(true, work_space, results);
      return absl::OkStatus();
    }

    if (left_result.has_value() && right_result.has_value()) {
      // Both children must be false to get here, so return false.
      SetResult(false, work_space, results);
      return absl::OkStatus();
    }

    // Neither child is true and at least one is empty, so propagate
    // empty per the FHIRPath spec.
    return absl::OkStatus();
  }
};

class AndOperator : public BooleanOperator {
 public:
  AndOperator(std::shared_ptr<ExpressionNode> left,
              std::shared_ptr<ExpressionNode> right)
      : BooleanOperator(left, right) {}

  absl::Status Evaluate(WorkSpace* work_space,
                        std::vector<WorkspaceMessage>* results) const override {
    // Logic from truth table spec: http://hl7.org/fhirpath/#boolean-logic
    // Short circuit and return false on the first false result.
    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> left_result,
                          EvaluateBooleanNode(left_, work_space));
    if (left_result.has_value() && !left_result.value()) {
      SetResult(false, work_space, results);
      return absl::OkStatus();
    }

    FHIR_ASSIGN_OR_RETURN(absl::optional<bool> right_result,
                          EvaluateBooleanNode(right_, work_space));
    if (right_result.has_value() && !right_result.value()) {
      SetResult(false, work_space, results);
      return absl::OkStatus();
    }

    if (left_result.has_value() && right_result.has_value()) {
      // Both children must be true to get here, so return true.
      SetResult(true, work_space, results);
      return absl::OkStatus();
    }

    // Neither child is false and at least one is empty, so propagate
    // empty per the FHIRPath spec.
    return absl::OkStatus();
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

  absl::Status EvaluateOperator(
      const std::vector<WorkspaceMessage>& left_results,
      const std::vector<WorkspaceMessage>& right_results, WorkSpace* work_space,
      std::vector<WorkspaceMessage>* results) const override {
    if (right_results.empty()) {
      return absl::OkStatus();
    }

    if (right_results.size() > 1) {
      return InvalidArgumentError(
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

    return absl::OkStatus();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }
};

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

absl::StatusOr<ExpressionNode*> UnimplementedFunction(
    std::shared_ptr<ExpressionNode>,
    const std::vector<FhirPathParser::ExpressionContext*>&,
    FhirPathBaseVisitor*, FhirPathBaseVisitor*) {
  return UnimplementedError("Function is not yet supported.");
}

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
  FhirPathCompilerVisitor(
      const Descriptor* descriptor, const PrimitiveHandler* primitive_handler,
      const std::map<std::string, UserDefinedFunction*>& user_defined_functions)
      : error_listener_(this),
        descriptor_stack_({descriptor}),
        primitive_handler_(primitive_handler),
        user_defined_functions_(user_defined_functions) {}

  FhirPathCompilerVisitor(
      const std::vector<const Descriptor*>& descriptor_stack_history,
      const Descriptor* descriptor, const PrimitiveHandler* primitive_handler,
      const std::map<std::string, UserDefinedFunction*>& user_defined_functions)
      : error_listener_(this),
        descriptor_stack_(descriptor_stack_history),
        primitive_handler_(primitive_handler),
        user_defined_functions_(user_defined_functions) {
    descriptor_stack_.push_back(descriptor);
  }

  antlrcpp::Any visitInvocationExpression(
      FhirPathParser::InvocationExpressionContext* node) override {
    antlrcpp::Any expression = node->children[0]->accept(this);

    // This could be a simple member name or a parameterized function...
    antlrcpp::Any invocation = node->children[2]->accept(this);

    if (!GetError().ok()) {
      return nullptr;
    }

    auto definition =
        AnyCast<std::shared_ptr<InvocationDefinition>>(invocation);

    std::shared_ptr<ExpressionNode> expr =
        AnyCast<std::shared_ptr<ExpressionNode>>(expression);

    if (definition->is_function) {
      auto function_node = createFunction(
          definition->name, expr, definition->params, primitive_handler_);

      if (function_node == nullptr || !GetError().ok()) {
        return nullptr;
      } else {
        return ToAny(function_node);
      }
    } else {
      const Descriptor* descriptor = expr->ReturnType();

      // If we know the return type of the expression, and the return type
      // doesn't have the referenced field, set an error and return.
      if (descriptor != nullptr &&
          !HasFieldWithJsonName(descriptor, definition->name)) {
        SetError(NotFoundError(
            absl::StrFormat("Unable to find field `%s` in `%s`",
                            definition->name, descriptor->full_name())));
        return nullptr;
      }

      const FieldDescriptor* field =
          descriptor != nullptr &&
                  !IsMessageType<google::protobuf::Any>(descriptor)
              ? FindFieldByJsonName(descriptor, definition->name)
              : nullptr;
      return ToAny(std::make_shared<InvokeExpressionNode>(
          AnyCast<std::shared_ptr<ExpressionNode>>(expression), field,
          definition->name));
    }
  }

  antlrcpp::Any visitInvocationTerm(
      FhirPathParser::InvocationTermContext* ctx) override {
    antlrcpp::Any invocation = visitChildren(ctx);

    if (!GetError().ok()) {
      return nullptr;
    }

    if (AnyIs<std::shared_ptr<ExpressionNode>>(invocation)) {
      return invocation;
    }

    auto definition =
        AnyCast<std::shared_ptr<InvocationDefinition>>(invocation);

    if (definition->is_function) {
      auto function_node = createFunction(
          definition->name,
          std::make_shared<ThisReference>(descriptor_stack_.back()),
          definition->params, primitive_handler_);

      return function_node == nullptr || !GetError().ok()
                 ? nullptr
                 : ToAny(function_node);
    }

    const Descriptor* descriptor = descriptor_stack_.back();

    // Handle base types:
    //   Resource -> DomainResource -> resourceType
    //   Element -> complexType/primitiveType
    if (descriptor != nullptr &&
        ((IsResource(descriptor) && (definition->name == "Resource" ||
                                     definition->name == "DomainResource" ||
                                     definition->name == descriptor->name())) ||
         ((IsComplex(descriptor) || IsPrimitive(descriptor)) &&
          definition->name == "Element"))) {
      return visitThisInvocation(nullptr);
    }

    // If we know the return type of the expression, and the return type
    // doesn't have the referenced field, set an error and return.
    if (descriptor != nullptr &&
        !HasFieldWithJsonName(descriptor, definition->name)) {
      SetError(NotFoundError(
          absl::StrFormat("Unable to find field `%s` in `%s`", definition->name,
                          descriptor->full_name())));
      return nullptr;
    }

    const FieldDescriptor* field =
        descriptor != nullptr &&
                !IsMessageType<google::protobuf::Any>(descriptor)
            ? FindFieldByJsonName(descriptor, definition->name)
            : nullptr;
    return ToAny(std::make_shared<InvokeTermNode>(field, definition->name));
  }

  antlrcpp::Any visitIndexerExpression(
      FhirPathParser::IndexerExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!GetError().ok()) {
      return nullptr;
    }

    auto left = AnyCast<std::shared_ptr<ExpressionNode>>(left_any);
    auto right = AnyCast<std::shared_ptr<ExpressionNode>>(right_any);

    return ToAny(
        std::make_shared<IndexerExpression>(primitive_handler_, left, right));
  }

  antlrcpp::Any visitUnionExpression(
      FhirPathParser::UnionExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!GetError().ok()) {
      return nullptr;
    }

    auto left = AnyCast<std::shared_ptr<ExpressionNode>>(left_any);
    auto right = AnyCast<std::shared_ptr<ExpressionNode>>(right_any);

    return ToAny(std::make_shared<UnionOperator>(left, right));
  }

  antlrcpp::Any visitAdditiveExpression(
      FhirPathParser::AdditiveExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    std::string op = ctx->children[1]->getText();
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!GetError().ok()) {
      return nullptr;
    }

    auto left = AnyCast<std::shared_ptr<ExpressionNode>>(left_any);
    auto right = AnyCast<std::shared_ptr<ExpressionNode>>(right_any);

    if (op == "+") {
      return ToAny(std::make_shared<AdditionOperator>(left, right));
    }

    if (op == "&") {
      return ToAny(std::make_shared<StrCatOperator>(left, right));
    }

    if (op == "-") {
      // TODO(b/153188056): Support "-"
      SetError(UnimplementedError("'-' operator is not supported yet."));
      return nullptr;
    }

    // FhirPath.g4 does not define any additional additive operators.
    SetError(InternalError(absl::StrCat("Unknown additive operator: ", op)));
    return nullptr;
  }

  antlrcpp::Any visitPolarityExpression(
      FhirPathParser::PolarityExpressionContext* ctx) override {
    std::string op = ctx->children[0]->getText();
    antlrcpp::Any operand_any = ctx->children[1]->accept(this);

    if (!GetError().ok()) {
      return nullptr;
    }

    auto operand = AnyCast<std::shared_ptr<ExpressionNode>>(operand_any);

    if (op == "+") {
      return ToAny(std::make_shared<PolarityOperator>(
          PolarityOperator::kPositive, operand));
    }

    if (op == "-") {
      return ToAny(std::make_shared<PolarityOperator>(
          PolarityOperator::kNegative, operand));
    }

    // FhirPath.g4 does not define any additional polarity operators.
    SetError(InternalError(absl::StrCat("Unknown polarity operator: ", op)));
    return nullptr;
  }

  antlrcpp::Any visitTypeExpression(
      FhirPathParser::TypeExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    std::string op = ctx->children[1]->getText();
    std::string type = ctx->children[2]->getText();

    if (!GetError().ok()) {
      return nullptr;
    }

    auto left = AnyCast<std::shared_ptr<ExpressionNode>>(left_any);

    if (op == "is") {
      return ToAny(std::make_shared<IsFunction>(left, type));
    }

    if (op == "as") {
      return ToAny(std::make_shared<AsFunction>(left, type));
    }

    // FhirPath.g4 does not define any additional type operators.
    SetError(InternalError(absl::StrCat("Unknown type operator: ", op)));
    return nullptr;
  }

  antlrcpp::Any visitEqualityExpression(
      FhirPathParser::EqualityExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    std::string op = ctx->children[1]->getText();
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!GetError().ok()) {
      return nullptr;
    }

    auto left = AnyCast<std::shared_ptr<ExpressionNode>>(left_any);
    auto right = AnyCast<std::shared_ptr<ExpressionNode>>(right_any);

    if (op == "=") {
      return ToAny(std::make_shared<EqualsOperator>(left, right));
    }
    if (op == "!=") {
      // Negate the equals function to implement !=
      auto equals_op = std::make_shared<EqualsOperator>(left, right);
      return ToAny(std::make_shared<NotFunction>(equals_op));
    }

    if (op == "~" || op == "!~") {
      SetError(
          UnimplementedError("'~' and '!~' operators are not yet supported"));
      return nullptr;
    }

    // FhirPath.g4 does not define any additional equality operators.
    SetError(InternalError(absl::StrCat("Unknown equality operator: ", op)));
    return nullptr;
  }

  antlrcpp::Any visitInequalityExpression(
      FhirPathParser::InequalityExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    std::string op = ctx->children[1]->getText();
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!GetError().ok()) {
      return nullptr;
    }

    auto left = AnyCast<std::shared_ptr<ExpressionNode>>(left_any);
    auto right = AnyCast<std::shared_ptr<ExpressionNode>>(right_any);

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
      SetError(
          InternalError(absl::StrCat("Unknown comparison operator: ", op)));
      return nullptr;
    }

    return ToAny(std::make_shared<ComparisonOperator>(left, right, op_type));
  }

  antlrcpp::Any visitMembershipExpression(
      FhirPathParser::MembershipExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    std::string op = ctx->children[1]->getText();
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!GetError().ok()) {
      return nullptr;
    }

    auto left = AnyCast<std::shared_ptr<ExpressionNode>>(left_any);
    auto right = AnyCast<std::shared_ptr<ExpressionNode>>(right_any);

    if (op == "in") {
      return ToAny(std::make_shared<ContainsOperator>(right, left));
    } else if (op == "contains") {
      return ToAny(std::make_shared<ContainsOperator>(left, right));
    }

    SetError(InternalError(absl::StrCat("Unknown membership operator: ", op)));
    return nullptr;
  }

  antlrcpp::Any visitMemberInvocation(
      FhirPathParser::MemberInvocationContext* ctx) override {
    std::string text;
    if (ctx->identifier()->IDENTIFIER()) {
      text = ctx->identifier()->IDENTIFIER()->getSymbol()->getText();
    } else if (ctx->identifier()->DELIMITEDIDENTIFIER()) {
      text = ctx->identifier()->DELIMITEDIDENTIFIER()->getSymbol()->getText();
      if (text.size() < 2 || text.front() != '`' || text.back() != '`') {
        SetError(
            InternalError(absl::StrCat("Invalid DELIMITEDIDENTIFIER ", text)));
      }
      text = text.substr(1, text.size() - 2);
    } else {
      SetError(InternalError(
          "Member visit missing both IDENTIFIER and DELIMITEDIDENTIFIER."));
    }
    return std::make_shared<InvocationDefinition>(text, false);
  }

  antlrcpp::Any visitFunctionInvocation(
      FhirPathParser::FunctionInvocationContext* ctx) override {
    if (!GetError().ok()) {
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

    if (!GetError().ok()) {
      return nullptr;
    }

    auto left = AnyCast<std::shared_ptr<ExpressionNode>>(left_any);
    auto right = AnyCast<std::shared_ptr<ExpressionNode>>(right_any);

    return ToAny(std::make_shared<ImpliesOperator>(left, right));
  }

  antlrcpp::Any visitOrExpression(
      FhirPathParser::OrExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    std::string op = ctx->children[1]->getText();
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!GetError().ok()) {
      return nullptr;
    }

    auto left = AnyCast<std::shared_ptr<ExpressionNode>>(left_any);
    auto right = AnyCast<std::shared_ptr<ExpressionNode>>(right_any);

    return op == "or" ? ToAny(std::make_shared<OrOperator>(left, right))
                      : ToAny(std::make_shared<XorOperator>(left, right));
  }

  antlrcpp::Any visitAndExpression(
      FhirPathParser::AndExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!GetError().ok()) {
      return nullptr;
    }

    auto left = AnyCast<std::shared_ptr<ExpressionNode>>(left_any);
    auto right = AnyCast<std::shared_ptr<ExpressionNode>>(right_any);

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

  antlrcpp::Any visitTotalInvocation(
      FhirPathParser::TotalInvocationContext* ctx) override {
    // TODO(b/154666068): Add support for $total.
    SetError(UnimplementedError("$total is not implemented"));
    return nullptr;
  }

  antlrcpp::Any visitIndexInvocation(
      FhirPathParser::IndexInvocationContext* ctx) override {
    // TODO(b/154665405): Add support for $index.
    SetError(UnimplementedError("$index is not implemented"));
    return nullptr;
  }

  antlrcpp::Any visitExternalConstant(
      FhirPathParser::ExternalConstantContext* ctx) override {
    std::string name = ctx->children[1]->getText();
    const PrimitiveHandler* primitive_handler = primitive_handler_;
    if (name == "ucum") {
      return ToAny(std::make_shared<Literal>(
          primitive_handler_->StringDescriptor(), [primitive_handler]() {
            return primitive_handler->NewString("http://unitsofmeasure.org");
          }));
    } else if (name == "sct") {
      return ToAny(std::make_shared<Literal>(
          primitive_handler_->StringDescriptor(), [primitive_handler]() {
            return primitive_handler->NewString("http://snomed.info/sct");
          }));
    } else if (name == "loinc") {
      return ToAny(std::make_shared<Literal>(
          primitive_handler_->StringDescriptor(), [primitive_handler]() {
            return primitive_handler->NewString("http://loinc.org");
          }));
    } else if (name == "context") {
      return ToAny(
          std::make_shared<ContextReference>(descriptor_stack_.front()));
    } else if (name == "resource") {
      return ToAny(std::make_shared<ResourceReference>());
    } else if (name == "rootResource") {
      // TODO(b/154440191): Add suport for %rootResource
      SetError(UnimplementedError("%rootResource is not implemented"));
    } else if (absl::StartsWith(name, "vs-")) {
      // TODO(b/154443143): Add support for %vs-[name]
      SetError(UnimplementedError("%vs-[name] is not implemented"));
    } else if (absl::StartsWith(name, "ext-")) {
      // TODO(b/154442904): Add support for %ext-[name]
      SetError(UnimplementedError("%ext-[name] is not implemented"));
    } else {
      SetError(
          NotFoundError(absl::StrCat("Unknown external constant: ", name)));
    }

    return nullptr;
  }

  absl::StatusOr<std::shared_ptr<Literal>> ParseDateTime(
      const std::string& text) {
    std::string date_time_str;
    std::string subseconds_str;
    std::string time_zone_str;
    if (!RE2::FullMatch(text, R"(@([\d-]+T[\d:]*)(\.\d+)?(Z|([+-]\d\d:\d\d))?)",
                        &date_time_str, &subseconds_str, &time_zone_str)) {
      return InvalidArgumentError(absl::StrCat(
          "DateTime literal does not match expected pattern: ", text));
    }

    if (date_time_str.size() == 13 || date_time_str.size() == 16) {
      // TODO(b/154874664): Support hour and minute precision
      return UnimplementedError(
          "Hour and minute level precision is not supported yet.");
    }

    bool no_time = date_time_str.back() == 'T';
    std::string normalized_date_time_string =
        absl::StrCat(absl::StripSuffix(date_time_str, "T"), subseconds_str,
                     !no_time && time_zone_str.empty() ? "Z" : time_zone_str);
    return std::make_shared<Literal>(
        primitive_handler_->DateTimeDescriptor(),
        [=, primitive_handler = primitive_handler_]() {
          return primitive_handler->NewDateTime(normalized_date_time_string);
        });
  }

  antlrcpp::Any visitDateTimeLiteral(
      FhirPathParser::DateTimeLiteralContext* ctx) override {
    absl::StatusOr<std::shared_ptr<Literal>> status_or_date_time_literal =
        ParseDateTime(ctx->getText());
    if (status_or_date_time_literal.ok()) {
      return ToAny(status_or_date_time_literal.value());
    } else {
      SetError(status_or_date_time_literal.status());
      return nullptr;
    }
  }

  antlrcpp::Any visitNumberLiteral(
      FhirPathParser::NumberLiteralContext* ctx) override {
    const std::string& text = ctx->getText();
    const PrimitiveHandler* primitive_handler = primitive_handler_;
    // Determine if the number is an integer or decimal, propagating
    // decimal types in string form to preserve precision.
    if (text.find('.') != std::string::npos) {
      return ToAny(std::make_shared<Literal>(
          primitive_handler_->DecimalDescriptor(), [primitive_handler, text]() {
            return primitive_handler->NewDecimal(text);
          }));
    } else {
      int32_t value;
      if (!absl::SimpleAtoi(text, &value)) {
        SetError(
            InvalidArgumentError(absl::StrCat("Malformed integer ", text)));
        return nullptr;
      }

      return ToAny(std::make_shared<Literal>(
          primitive_handler_->IntegerDescriptor(),
          [primitive_handler, value]() {
            return primitive_handler->NewInteger(value);
          }));
    }
  }

  antlrcpp::Any visitStringLiteral(
      FhirPathParser::StringLiteralContext* ctx) override {
    const std::string& text = ctx->getText();
    const PrimitiveHandler* primitive_handler = primitive_handler_;
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
    return ToAny(std::make_shared<Literal>(
        primitive_handler_->StringDescriptor(),
        [primitive_handler, unescaped]() {
          return primitive_handler->NewString(unescaped);
        }));
  }

  antlrcpp::Any visitBooleanLiteral(
      FhirPathParser::BooleanLiteralContext* ctx) override {
    const bool value = ctx->getText() == "true";
    const PrimitiveHandler* primitive_handler = primitive_handler_;

    return ToAny(std::make_shared<Literal>(
        primitive_handler_->BooleanDescriptor(), [primitive_handler, value]() {
          return primitive_handler->NewBoolean(value);
        }));
  }

  antlrcpp::Any visitNullLiteral(
      FhirPathParser::NullLiteralContext* ctx) override {
    return ToAny(std::make_shared<EmptyLiteral>());
  }

  antlrcpp::Any visitTerminal(TerminalNode* node) override {
    switch (node->getSymbol()->getType()) {
      case FhirPathLexer::DATE:
        // TODO(b/153188970): Add support for Date literals.
        SetError(UnimplementedError("Date literals are not yet supported."));
        return nullptr;

      case FhirPathLexer::TIME:
        // TODO(b/153185021): Add spport for time literals.
        SetError(UnimplementedError("Time literals are not yet supported."));
        return nullptr;

      default:
        SetError(InternalError(absl::StrCat("Unknown terminal type: ",
                                            node->getSymbol()->getText())));
        return nullptr;
    }
  }

  antlrcpp::Any visitErrorNode(antlr4::tree::ErrorNode* node) override {
    SetError(InternalError(
        absl::StrCat("Unknown error encountered: ", node->toString())));
    return nullptr;
  }

  antlrcpp::Any defaultResult() override { return nullptr; }

  absl::Status GetError() { return error_; }

  BaseErrorListener* GetErrorListener() { return &error_listener_; }

 private:
  typedef std::function<absl::StatusOr<ExpressionNode*>(
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
      {"ofType", OfTypeFunction::Create},
      {"children", FunctionNode::Create<ChildrenFunction>},
      {"descendants", FunctionNode::Create<DescendantsFunction>},
      {"allTrue", FunctionNode::Create<AllTrueFunction>},
      {"anyTrue", CreateAnyTrueFunction},
      {"allFalse", FunctionNode::Create<AllFalseFunction>},
      {"anyFalse", CreateAnyFalseFunction},
      {"subsetOf", UnimplementedFunction},
      {"supersetOf", UnimplementedFunction},
      {"repeat", UnimplementedFunction},
      {"single", FunctionNode::Create<SingleFunction>},
      {"last", FunctionNode::Create<LastFunction>},
      {"skip", FunctionNode::Create<SkipFunction>},
      {"take", FunctionNode::Create<TakeFunction>},
      {"exclude", UnimplementedFunction},
      {"union", CreateUnionFunction},
      {"convertsToBoolean", CreateConvertsToFunction<ToBooleanFunction>},
      {"toBoolean", FunctionNode::Create<ToBooleanFunction>},
      {"convertsToInteger", CreateConvertsToFunction<ToIntegerFunction>},
      {"convertsToDate", UnimplementedFunction},
      {"toDate", UnimplementedFunction},
      {"convertsToDateTime", UnimplementedFunction},
      {"toDateTime", UnimplementedFunction},
      {"convertsToDecimal", UnimplementedFunction},
      {"toDecimal", UnimplementedFunction},
      {"convertsToQuantity", UnimplementedFunction},
      {"toQuantity", UnimplementedFunction},
      {"convertsToString", CreateConvertsToFunction<ToStringFunction>},
      {"convertsToTime", UnimplementedFunction},
      {"toTime", UnimplementedFunction},
      {"indexOf", FunctionNode::Create<IndexOfFunction>},
      {"substring", FunctionNode::Create<SubstringFunction>},
      {"upper", FunctionNode::Create<UpperFunction>},
      {"lower", FunctionNode::Create<LowerFunction>},
      {"replace", FunctionNode::Create<ReplaceFunction>},
      {"endsWith", FunctionNode::Create<EndsWithFunction>},
      {"toChars", UnimplementedFunction},
      {"today", UnimplementedFunction},
      {"now", UnimplementedFunction},
      {"getValue", UnimplementedFunction},
      {"elementDefinition", UnimplementedFunction},
      {"slice", UnimplementedFunction},
      {"checkModifiers", UnimplementedFunction},
      {"memberOf", FunctionNode::Create<MemberOfFunction>},
      {"subsumes", UnimplementedFunction},
      {"subsumedBy", UnimplementedFunction},
  };

  // Returns an ExpressionNode that implements the specified FHIRPath function.
  std::shared_ptr<ExpressionNode> createFunction(
      const std::string& function_name,
      std::shared_ptr<ExpressionNode> child_expression,
      const std::vector<FhirPathParser::ExpressionContext*>& params,
      const PrimitiveHandler* primitive_handler) {
    // Some functions accept parameters that are expressions evaluated using
    // the child expression's result as context, not the base context of the
    // FHIRPath expression. In order to compile such parameters, we need to
    // visit it with the child expression's type and not the base type of the
    // current visitor. Therefore, both the current visitor and a visitor with
    // the child expression as the context are provided. The function factory
    // will use whichever visitor (or both) is needed to compile the function
    // invocation.
    FhirPathCompilerVisitor child_context_visitor(
        descriptor_stack_, child_expression->ReturnType(), primitive_handler_,
        user_defined_functions_);

    std::map<std::string, FunctionFactory>::iterator function_factory =
        function_map_.find(function_name);
    absl::StatusOr<ExpressionNode*> result;

    if (function_factory != function_map_.end()) {
      if (user_defined_functions_.find(function_name) !=
          user_defined_functions_.end()) {
        SetError(NotFoundError(
            absl::StrCat("The user-defined function ", function_name,
                         "() conflicts with an existing function name.")));
        return std::shared_ptr<ExpressionNode>(nullptr);
      }
      result = function_factory->second(child_expression, params, this,
                                        &child_context_visitor);
    } else if (user_defined_functions_.find(function_name) !=
               user_defined_functions_.end()) {
      result = CustomFunction::Create(
          child_expression, params, user_defined_functions_.at(function_name),
          next_user_defined_function_id_++, this, &child_context_visitor,
          primitive_handler);
    } else {
      SetError(NotFoundError(
          absl::StrCat("The function ", function_name, " does not exist.")));

      return std::shared_ptr<ExpressionNode>(nullptr);
    }

    if (!result.ok()) {
      this->SetError(absl::InvalidArgumentError(absl::StrCat(
          "Failed to compile call to ", function_name,
          "(): ", result.status().message(),
          !child_context_visitor.GetError().ok()
              ? absl::StrCat("; ", child_context_visitor.GetError().message())
              : "")));
      return std::shared_ptr<ExpressionNode>(nullptr);
    }
    if (!child_context_visitor.GetError().ok()) {
      this->SetError(absl::InvalidArgumentError(absl::StrCat(
          "Failed to compile call to ", function_name,
          "(): ", child_context_visitor.GetError().message(),
          !child_context_visitor.GetError().ok()
              ? absl::StrCat("; ", child_context_visitor.GetError().message())
              : "")));

      // Unlike the previous case where we get an error, we free the result
      // object here since the error isn't returned directly from the function
      // call but is a side effect. Thus we delete the unused object before
      // returning.
      delete result.value();
      return std::shared_ptr<ExpressionNode>(nullptr);
    }
    if (!GetError().ok()) {
      this->SetError(absl::InvalidArgumentError(absl::StrCat(
          "Failed to compile call to ", function_name,
          "(): ", GetError().message(),
          !GetError().ok() ? absl::StrCat("; ", GetError().message()) : "")));
      delete result.value();
      return std::shared_ptr<ExpressionNode>(nullptr);
    }

    return std::shared_ptr<ExpressionNode>(result.value());
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
      visitor_->SetError(absl::InvalidArgumentError(message));
    }

   private:
    FhirPathCompilerVisitor* visitor_;
  };

  void SetError(const absl::Status& error) { error_ = error; }

  FhirPathErrorListener error_listener_;
  std::vector<const Descriptor*> descriptor_stack_;
  absl::Status error_;
  const PrimitiveHandler* primitive_handler_;
  const std::map<std::string, UserDefinedFunction*> user_defined_functions_;
  int32_t next_user_defined_function_id_ = 0;
};

absl::StatusOr<std::unique_ptr<internal::WorkSpace>> EvaluateCompiledExpression(
    std::shared_ptr<const ExpressionNode> start_node,
    const PrimitiveHandler* primitive_handler,
    const terminology::TerminologyResolver* terminology_resolver,
    const WorkspaceMessage& message,
    std::vector<internal::WorkspaceMessage> message_context_stack = {},
    std::map<int32_t, std::vector<internal::WorkspaceMessage>> results_cache =
        {}) {
  auto work_space = std::make_unique<internal::WorkSpace>(
      primitive_handler, message_context_stack, message, terminology_resolver,
      results_cache);
  std::vector<internal::WorkspaceMessage> workspace_results;
  FHIR_RETURN_IF_ERROR(
      start_node->Evaluate(work_space.get(), &workspace_results));

  std::vector<const Message*> results;
  results.reserve(workspace_results.size());
  for (internal::WorkspaceMessage& result : workspace_results) {
    results.push_back(result.Message());
  }
  work_space->SetResultMessages(results);
  return std::move(work_space);
}

}  // namespace internal

// Postorder traversal of the AST looking for the first callback function node
// with no precomputed result (entry in results_cache), starting the traversal
// from the `root_expression_` node of the compiled expression.
// Returns nullptr if the AST doesn't contain any callback function nodes that
// have not already been evaluated.
std::shared_ptr<const internal::CustomFunction>
GetFirstCallbackNodeWithNoCachedResult(
    std::shared_ptr<const ExpressionNode> node,
    const std::map<int32_t, std::vector<internal::WorkspaceMessage>>&
        results_cache) {
  for (const std::shared_ptr<ExpressionNode>& child : node->GetChildren()) {
    auto callback_node =
        GetFirstCallbackNodeWithNoCachedResult(child, results_cache);
    if (callback_node != nullptr) return callback_node;
  }
  std::shared_ptr<const internal::CustomFunction> custom_function =
      std::dynamic_pointer_cast<const internal::CustomFunction>(node);
  if (custom_function != nullptr &&
      custom_function->GetCallbackFunctionName().has_value() &&
      results_cache.find(custom_function->GetId()) == results_cache.end()) {
    return custom_function;
  }
  return nullptr;
}

EvaluationResult::EvaluationResult(EvaluationResult&& result)
    : work_space_(std::move(result.work_space_)),
      callback_function_name_(std::move(result.callback_function_name_)) {}

EvaluationResult& EvaluationResult::operator=(EvaluationResult&& result) {
  work_space_ = std::move(result.work_space_);
  callback_function_name_ = result.callback_function_name_;

  return *this;
}

EvaluationResult::EvaluationResult(
    std::unique_ptr<internal::WorkSpace> work_space,
    const std::string& callback_function_name)
    : work_space_(std::move(work_space)) {
  if (!callback_function_name.empty()) {
    callback_function_name_ = callback_function_name;
  }
}

EvaluationResult::~EvaluationResult() {}

const std::vector<const Message*>& EvaluationResult::GetMessages() const {
  return work_space_->GetResultMessages();
}

absl::StatusOr<bool> EvaluationResult::GetBoolean() const {
  auto messages = work_space_->GetResultMessages();
  if (messages.size() != 1) {
    return InvalidArgumentError(
        "Result collection for GetBoolean must contain exactly one element");
  }
  return work_space_->GetPrimitiveHandler()->GetBooleanValue(*messages[0]);
}

absl::StatusOr<int32_t> EvaluationResult::GetInteger() const {
  auto messages = work_space_->GetResultMessages();
  if (messages.size() != 1) {
    return InvalidArgumentError(
        "Result collection for GetInteger must contain exactly one element");
  }
  return work_space_->GetPrimitiveHandler()->GetIntegerValue(*messages[0]);
}

absl::StatusOr<std::string> EvaluationResult::GetDecimal() const {
  auto messages = work_space_->GetResultMessages();
  if (messages.size() != 1) {
    return InvalidArgumentError(
        "Result collection for GetDecimal must contain exactly one element");
  }
  return work_space_->GetPrimitiveHandler()->GetDecimalValue(*messages[0]);
}

absl::StatusOr<std::string> EvaluationResult::GetString() const {
  auto messages = work_space_->GetResultMessages();
  if (messages.size() != 1) {
    return InvalidArgumentError(
        "Result collection for GetString must contain exactly one element");
  }
  return work_space_->GetPrimitiveHandler()->GetStringValue(*messages[0]);
}

CompiledExpression::CompiledExpression(CompiledExpression&& other)
    : fhir_path_(std::move(other.fhir_path_)),
      root_expression_(std::move(other.root_expression_)),
      primitive_handler_(other.primitive_handler_),
      terminology_resolver_(other.terminology_resolver_) {}

CompiledExpression& CompiledExpression::operator=(CompiledExpression&& other) {
  fhir_path_ = std::move(other.fhir_path_);
  root_expression_ = std::move(other.root_expression_);
  primitive_handler_ = other.primitive_handler_;
  terminology_resolver_ = other.terminology_resolver_;

  return *this;
}

CompiledExpression::CompiledExpression(const CompiledExpression& other)
    : fhir_path_(other.fhir_path_),
      root_expression_(other.root_expression_),
      primitive_handler_(other.primitive_handler_),
      terminology_resolver_(other.terminology_resolver_) {}

CompiledExpression& CompiledExpression::operator=(
    const CompiledExpression& other) {
  fhir_path_ = other.fhir_path_;
  root_expression_ = other.root_expression_;
  primitive_handler_ = other.primitive_handler_;
  terminology_resolver_ = other.terminology_resolver_;

  return *this;
}

const std::string& CompiledExpression::fhir_path() const { return fhir_path_; }

CompiledExpression::CompiledExpression(
    const std::string& fhir_path,
    std::shared_ptr<internal::ExpressionNode> root_expression,
    const PrimitiveHandler* primitive_handler,
    const terminology::TerminologyResolver* terminology_resolver)
    : fhir_path_(fhir_path),
      root_expression_(root_expression),
      primitive_handler_(primitive_handler),
      terminology_resolver_(terminology_resolver) {}

absl::StatusOr<CompiledExpression> CompiledExpression::Compile(
    const Descriptor* descriptor, const PrimitiveHandler* primitive_handler,
    const std::string& fhir_path,
    const terminology::TerminologyResolver* terminology_resolver,
    const std::vector<UserDefinedFunction*>& user_defined_functions) {
  std::map<std::string, UserDefinedFunction*> fns;
  for (UserDefinedFunction* fn : user_defined_functions) {
    if (fns.find(fn->GetName()) != fns.end()) {
      return absl::InvalidArgumentError(
          absl::StrCat("Found multiple user-defined functions with same name: ",
                       fn->GetName()));
    }
    fns[fn->GetName()] = fn;
  }

  ANTLRInputStream input(fhir_path);
  FhirPathLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  FhirPathParser parser(&tokens);

  internal::FhirPathCompilerVisitor visitor(descriptor, primitive_handler, fns);
  parser.addErrorListener(visitor.GetErrorListener());
  lexer.addErrorListener(visitor.GetErrorListener());
  antlrcpp::Any result = visitor.visit(parser.expression());

  if (AnyHasValue(result) && visitor.GetError().ok()) {
    auto root_node = AnyCast<std::shared_ptr<internal::ExpressionNode>>(result);
    return CompiledExpression(fhir_path, root_node, primitive_handler,
                              terminology_resolver);
  } else {
    return visitor.GetError();
  }
}

absl::StatusOr<EvaluationResult> CompiledExpression::Evaluate(
    const Message& message) const {
  return Evaluate(internal::WorkspaceMessage(&message));
}

absl::StatusOr<EvaluationResult> CompiledExpression::Evaluate(
    const internal::WorkspaceMessage& message) const {
  auto next_callback_node =
      GetFirstCallbackNodeWithNoCachedResult(root_expression_, {});
  auto work_space = internal::EvaluateCompiledExpression(
      next_callback_node == nullptr ? root_expression_ : next_callback_node,
      primitive_handler_, terminology_resolver_, message);
  if (!work_space.ok()) {
    return work_space.status();
  }
  if (next_callback_node != nullptr) {
    return EvaluationResult(
        std::move(work_space.value()),
        next_callback_node->GetCallbackFunctionName().value());
  }
  return EvaluationResult(std::move(work_space.value()));
}

absl::StatusOr<EvaluationResult> CompiledExpression::ResumeEvaluation(
    const EvaluationResult& prev_result,
    const std::vector<const Message*>& callback_results) const {
  std::vector<internal::WorkspaceMessage> workspace_results;
  workspace_results.reserve(callback_results.size());
  for (const Message* msg : callback_results) {
    workspace_results.push_back(internal::WorkspaceMessage(msg));
  }
  std::map<int32_t, std::vector<internal::WorkspaceMessage>> results_cache =
      prev_result.work_space_->GetResultsCache();
  std::optional<int32_t> next_cache_node_id =
      prev_result.work_space_->GetNextCacheNodeId();
  if (!next_cache_node_id.has_value()) {
    return absl::InvalidArgumentError(
        "Cannot resume evaluation, no remaining callback functions found");
  }
  results_cache[next_cache_node_id.value()] = workspace_results;

  auto next_callback_node =
      GetFirstCallbackNodeWithNoCachedResult(root_expression_, results_cache);

  auto work_space = internal::EvaluateCompiledExpression(
      next_callback_node == nullptr ? root_expression_ : next_callback_node,
      primitive_handler_, terminology_resolver_,
      prev_result.work_space_->MessageContext(),
      prev_result.work_space_->MessageContextStack(), results_cache);
  if (!work_space.ok()) {
    return work_space.status();
  }
  if (next_callback_node != nullptr) {
    return EvaluationResult(
        std::move(work_space.value()),
        next_callback_node->GetCallbackFunctionName().value());
  }
  return EvaluationResult(std::move(work_space.value()));
}

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
