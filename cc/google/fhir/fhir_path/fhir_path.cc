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

#include "google/protobuf/wrappers.pb.h"
#include "absl/strings/str_cat.h"

// Include the ANTLR-generated visitor, lexer and parser files.
#include "absl/memory/memory.h"
#include "google/fhir/annotations.h"
#include "google/fhir/fhir_path/FhirPathBaseVisitor.h"
#include "google/fhir/fhir_path/FhirPathLexer.h"
#include "google/fhir/fhir_path/FhirPathParser.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace fhir_path {

using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;

using ::google::protobuf::BoolValue;

using antlr4::ANTLRInputStream;
using antlr4::CommonTokenStream;

using antlr_parser::FhirPathBaseVisitor;
using antlr_parser::FhirPathLexer;
using antlr_parser::FhirPathParser;

using ::google::fhir::ForEachMessage;
using ::google::fhir::ForEachMessageHalting;
using ::google::fhir::IsMessageType;
using ::google::fhir::StatusOr;

using internal::ExpressionNode;

using ::tensorflow::errors::InvalidArgument;

using std::string;

namespace internal {

// Returns true if the collection of messages represents
// a boolean value per FHIRPath conventions; that is it
// has exactly one item that is boolean.
bool IsSingleBoolean(const std::vector<const Message*>& messages) {
  return messages.size() == 1 && IsMessageType<BoolValue>(*messages[0]);
}

// Returns success with a boolean value if the message collection
// represents a single boolean, or a failure status otherwise.
StatusOr<bool> MessagesToBoolean(const std::vector<const Message*>& messages) {
  if (IsSingleBoolean(messages)) {
    return dynamic_cast<const BoolValue*>(messages[0])->value();
  }

  return InvalidArgument("Expression did not evaluate to boolean");
}

// Supported funtions.
const char kExistsFunction[] = "exists";
const char kNotFunction[] = "not";

// Implements the InvocationTerm from the FHIRPath grammar,
// producing a term from the root context message.
class InvokeTermNode : public ExpressionNode {
 public:
  explicit InvokeTermNode(const FieldDescriptor* field) : field_(field) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<const Message*>* results) const override {
    const Message& message = *work_space->MessageContext();
    const Reflection* refl = message.GetReflection();

    if (field_->is_repeated()) {
      int field_size = refl->FieldSize(message, field_);

      for (int i = 0; i < field_size; ++i) {
        const Message& child = refl->GetRepeatedMessage(message, field_, i);

        results->push_back(&child);
      }

    } else {
      if (refl->HasField(message, field_)) {
        const Message& child = refl->GetMessage(message, field_);

        results->push_back(&child);
      }
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return field_->message_type();
  }

 private:
  const FieldDescriptor* field_;
};

// Handles the InvocationExpression from the FHIRPath grammar,
// which can be a member of function called on the results of
// another expression.
class InvokeExpressionNode : public ExpressionNode {
 public:
  InvokeExpressionNode(std::shared_ptr<ExpressionNode> child_expression,
                       const FieldDescriptor* field)
      : child_expression_(child_expression), field_(field) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<const Message*>* results) const override {
    std::vector<const Message*> child_results;

    FHIR_RETURN_IF_ERROR(
        child_expression_->Evaluate(work_space, &child_results));

    // Iterate through the results of the child expression and invoke
    // the appropriate field.
    for (const Message* child_message : child_results) {
      ForEachMessage<Message>(
          *child_message, field_,
          [&](const Message& result) { results->push_back(&result); });
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return field_->message_type();
  }

 private:
  const std::shared_ptr<ExpressionNode> child_expression_;
  const FieldDescriptor* field_;
};

// Implements the FHIRPath .exists() function
class ExistsFunction : public ExpressionNode {
 public:
  explicit ExistsFunction(const std::shared_ptr<ExpressionNode>& child)
      : child_(child) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<const Message*>* results) const override {
    std::vector<const Message*> child_results;
    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    BoolValue* result = new BoolValue();
    work_space->DeleteWhenFinished(result);
    result->set_value(!child_results.empty());
    results->push_back(result);

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return BoolValue::descriptor();
  }

 private:
  const std::shared_ptr<ExpressionNode> child_;
};

// Implements the FHIRPath .not() function.
class NotFunction : public ExpressionNode {
 public:
  explicit NotFunction(const std::shared_ptr<ExpressionNode>& child)
      : child_(child) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<const Message*>* results) const override {
    std::vector<const Message*> child_results;

    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    // Per the FHIRPath spec, boolean operations on empty collection
    // propagate the empty collection.
    if (child_results.empty()) {
      return Status::OK();
    }

    // Per the FHIR spec, the not() function produces a value
    // IFF it is given a boolean input, and returns an empty result
    // otherwise.
    if (IsSingleBoolean(child_results)) {
      FHIR_ASSIGN_OR_RETURN(bool child_result,
                            MessagesToBoolean(child_results));

      BoolValue* result = new BoolValue();
      work_space->DeleteWhenFinished(result);
      result->set_value(!child_result);

      results->push_back(result);
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return BoolValue::descriptor();
  }

 private:
  const std::shared_ptr<ExpressionNode> child_;
};

// Base class for FHIRPath binary boolean operators.
class BooleanOperator : public ExpressionNode {
 public:
  Status Evaluate(WorkSpace* work_space,
                  std::vector<const Message*>* results) const override {
    std::vector<const Message*> left_results;
    FHIR_RETURN_IF_ERROR(left_->Evaluate(work_space, &left_results));

    std::vector<const Message*> right_results;
    FHIR_RETURN_IF_ERROR(right_->Evaluate(work_space, &right_results));

    // Per the FHIRPath spec, boolean operators propagate empty results.
    if (left_results.empty() || right_results.empty()) {
      return Status::OK();
    }

    FHIR_ASSIGN_OR_RETURN(bool left_result, MessagesToBoolean(left_results));
    FHIR_ASSIGN_OR_RETURN(bool right_result, MessagesToBoolean(right_results));
    bool eval_result = EvaluateBool(left_result, right_result);

    BoolValue* result = new BoolValue();
    work_space->DeleteWhenFinished(result);
    result->set_value(eval_result);

    results->push_back(result);

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return BoolValue::descriptor();
  }

  BooleanOperator(std::shared_ptr<ExpressionNode> left,
                  std::shared_ptr<ExpressionNode> right)
      : left_(left), right_(right) {}

  // Perform the actual boolean evaluation.
  virtual bool EvaluateBool(bool left, bool right) const = 0;

 private:
  const std::shared_ptr<ExpressionNode> left_;

  const std::shared_ptr<ExpressionNode> right_;
};

class OrOperator : public BooleanOperator {
 public:
  OrOperator(std::shared_ptr<ExpressionNode> left,
             std::shared_ptr<ExpressionNode> right)
      : BooleanOperator(left, right) {}

  bool EvaluateBool(bool left, bool right) const override {
    return left || right;
  }
};

class AndOperator : public BooleanOperator {
 public:
  AndOperator(std::shared_ptr<ExpressionNode> left,
              std::shared_ptr<ExpressionNode> right)
      : BooleanOperator(left, right) {}

  bool EvaluateBool(bool left, bool right) const override {
    return left && right;
  }
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
  InvocationDefinition(const string& name, const bool is_function)
      : name(name), is_function(is_function) {}

  const string name;

  // Indicates it is a function invocation rather than a member lookup.
  const bool is_function;

  const std::vector<std::unique_ptr<ExpressionNode>> params;
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
  explicit FhirPathCompilerVisitor(const Descriptor* descriptor)
      : descriptor_(descriptor) {}

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
      auto function_node = createFunction(definition->name, expr);

      if (function_node == nullptr) {
        return nullptr;
      } else {
        return ToAny(function_node);
      }
    } else {
      const Descriptor* descriptor = expr->ReturnType();

      const FieldDescriptor* field =
          descriptor->FindFieldByName(definition->name);

      if (field == nullptr) {
        SetError("Unable to find field" + definition->name);
        return nullptr;
      }

      return ToAny(std::make_shared<InvokeExpressionNode>(expression, field));
    }
  }

  antlrcpp::Any visitInvocationTerm(
      FhirPathParser::InvocationTermContext* ctx) override {
    antlrcpp::Any invocation = visitChildren(ctx);

    if (!CheckOk()) {
      return nullptr;
    }

    auto definition = invocation.as<std::shared_ptr<InvocationDefinition>>();

    const FieldDescriptor* field =
        descriptor_->FindFieldByName(definition->name);

    if (field == nullptr) {
      SetError("Unable to find field" + definition->name);
      return nullptr;

    } else {
      return ToAny(std::make_shared<InvokeTermNode>(field));
    }
  }

  antlrcpp::Any visitEqualityExpression(
      FhirPathParser::EqualityExpressionContext* ctx) override {
    // TODO: Implement this.
    SetError("Equality operator not yet implemented");
    return nullptr;
  }

  antlrcpp::Any visitInequalityExpression(
      FhirPathParser::InequalityExpressionContext* ctx) override {
    // TODO: Implement this.
    SetError("Inequality operator not yet implemented.");
    return nullptr;
  }

  antlrcpp::Any visitMemberInvocation(
      FhirPathParser::MemberInvocationContext* ctx) override {
    string text = ctx->identifier()->IDENTIFIER()->getSymbol()->getText();

    return std::make_shared<InvocationDefinition>(text, false);
  }

  antlrcpp::Any visitFunctionInvocation(
      FhirPathParser::FunctionInvocationContext* ctx) override {
    if (!CheckOk()) {
      return nullptr;
    }

    string text =
        ctx->function()->identifier()->IDENTIFIER()->getSymbol()->getText();

    // TODO: visit and handle parameters
    //  in ctx->function()->paramList()->expression()

    return std::make_shared<InvocationDefinition>(text, true);
  }

  antlrcpp::Any visitOrExpression(
      FhirPathParser::OrExpressionContext* ctx) override {
    antlrcpp::Any left_any = ctx->children[0]->accept(this);
    antlrcpp::Any right_any = ctx->children[2]->accept(this);

    if (!CheckOk()) {
      return nullptr;
    }

    auto left = left_any.as<std::shared_ptr<ExpressionNode>>();
    auto right = right_any.as<std::shared_ptr<ExpressionNode>>();

    return ToAny(std::make_shared<OrOperator>(left, right));
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

  antlrcpp::Any defaultResult() override { return nullptr; }

  bool CheckOk() { return error_message_.empty(); }

  string GetError() { return error_message_; }

 private:
  // Returns an ExpressionNode that implements the
  // specified FHIRPath function.
  std::shared_ptr<ExpressionNode> createFunction(
      const std::string& function_name,
      std::shared_ptr<ExpressionNode> child_expression) {
    if (function_name == kExistsFunction) {
      return std::make_shared<ExistsFunction>(child_expression);
    } else if (function_name == kNotFunction) {
      return std::make_shared<NotFunction>(child_expression);
    } else {
      // TODO: Implement set of functions for initial use cases.
      SetError(absl::StrCat("The function ", function_name,
                            " is not yet implemented"));

      return std::shared_ptr<ExpressionNode>(nullptr);
    }
  }

  void SetError(const string& error_message) { error_message_ = error_message; }

  const Descriptor* descriptor_;
  string error_message_;
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
  if (internal::IsSingleBoolean(work_space_->GetResultMessages())) {
    return internal::MessagesToBoolean(work_space_->GetResultMessages());
  }

  return InvalidArgument("Expression did not evaluate to boolean");
}

CompiledExpression::CompiledExpression(CompiledExpression&& other)
    : fhir_path_(std::move(other.fhir_path_)),
      root_expression_(std::move(other.root_expression_)) {}

CompiledExpression& CompiledExpression::operator=(CompiledExpression&& other) {
  fhir_path_ = std::move(other.fhir_path_);
  root_expression_ = std::move(other.root_expression_);

  return *this;
}

CompiledExpression::CompiledExpression(const CompiledExpression& other)
    : fhir_path_(other.fhir_path_), root_expression_(other.root_expression_) {}

CompiledExpression& CompiledExpression::operator=(
    const CompiledExpression& other) {
  fhir_path_ = other.fhir_path_;
  root_expression_ = other.root_expression_;

  return *this;
}

const string& CompiledExpression::fhir_path() const { return fhir_path_; }

CompiledExpression::CompiledExpression(
    const string& fhir_path,
    std::shared_ptr<internal::ExpressionNode> root_expression)
    : fhir_path_(fhir_path), root_expression_(root_expression) {}

StatusOr<CompiledExpression> CompiledExpression::Compile(
    const Descriptor* descriptor, const string& fhir_path) {
  ANTLRInputStream input(fhir_path);
  FhirPathLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  FhirPathParser parser(&tokens);

  internal::FhirPathCompilerVisitor visitor(descriptor);
  antlrcpp::Any result = visitor.visit(parser.expression());

  if (result.isNotNull()) {
    auto root_node = result.as<std::shared_ptr<internal::ExpressionNode>>();
    return CompiledExpression(fhir_path, root_node);

  } else {
    return InvalidArgument(visitor.GetError());
  }
}

StatusOr<EvaluationResult> CompiledExpression::Evaluate(
    const Message& message) const {
  auto work_space = absl::make_unique<internal::WorkSpace>(&message);

  std::vector<const Message*> results;

  FHIR_RETURN_IF_ERROR(root_expression_->Evaluate(work_space.get(), &results));

  work_space->SetResultMessages(results);

  return EvaluationResult(std::move(work_space));
}

MessageValidator::MessageValidator() {}
MessageValidator::~MessageValidator() {}

// Build the constraints for the given message type and
// add it to the constraints cache.
StatusOr<MessageValidator::MessageConstraints*>
MessageValidator::ConstraintsFor(const Descriptor* descriptor) {
  // Simply return the cached constraint if it exists.
  auto iter = constraints_cache_.find(descriptor->full_name());

  if (iter != constraints_cache_.end()) {
    return iter->second.get();
  }

  auto constraints = absl::make_unique<MessageConstraints>();

  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);

    const Descriptor* field_type = field->message_type();

    // Constraints only apply to non-primitives.
    if (field_type != nullptr) {
      int ext_size =
          field->options().ExtensionSize(proto::fhir_path_constraint);

      for (int j = 0; j < ext_size; ++j) {
        const string& fhir_path =
            field->options().GetExtension(proto::fhir_path_constraint, j);

        auto constraint = CompiledExpression::Compile(field_type, fhir_path);

        if (constraint.ok()) {
          constraints->expressions_.push_back(
              std::make_pair(field, constraint.ValueOrDie()));
        }

        // TODO: Unsupported FHIRPath expressions are simply not
        // validated for now; this should produce an error once we support
        // all of FHIRPath.
      }
    }
  }

  // Add the successful constraints to the cache while keeping a local
  // reference.
  MessageConstraints* constraints_local = constraints.get();
  constraints_cache_[descriptor->full_name()] = std::move(constraints);

  // Now we recursively look for fields with constraints.
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);

    const Descriptor* field_type = field->message_type();

    // Constraints only apply to non-primitives.
    if (field_type != nullptr) {
      // Validate the field type.
      FHIR_ASSIGN_OR_RETURN(auto child_constraints, ConstraintsFor(field_type));

      // Nested fields that directly or transitively have constraints
      // are retained and used when applying constraints.
      if (!child_constraints->expressions_.empty() ||
          !child_constraints->nested_with_constraints_.empty()) {
        constraints_local->nested_with_constraints_.push_back(field);
      }
    }
  }

  return constraints_local;
}

// Default handler halts on first error
bool HaltOnErrorHandler(const Message& message, const FieldDescriptor* field,
                        const string& constraint) {
  return true;
}

Status MessageValidator::Validate(const Message& message) {
  return Validate(message, HaltOnErrorHandler);
}

// Validates that the given field in the given parent satisfies the
// the given FHIRPath expression, invoking the handler in case
// of failure.
Status ValidateConstraint(const Message& parent, const FieldDescriptor* field,
                          const Message& field_value,
                          const CompiledExpression& expression,
                          const ViolationHandlerFunc handler,
                          bool* halt_validation) {
  FHIR_ASSIGN_OR_RETURN(const EvaluationResult expr_result,
                        expression.Evaluate(field_value));

  if (!expr_result.GetBoolean().ValueOrDie()) {
    string err_msg =
        absl::StrCat("fhirpath-constraint-violation-",
                     field->containing_type()->name(), ".", field->json_name());

    *halt_validation = handler(parent, field, expression.fhir_path());
    return ::tensorflow::errors::FailedPrecondition(err_msg);
  }

  return Status::OK();
}

// Store the first detected failure in the accumulative status.
void UpdateStatus(Status* accumulative_status, const Status& current_status) {
  if (accumulative_status->ok() && !current_status.ok()) {
    *accumulative_status = current_status;
  }
}

Status MessageValidator::Validate(const Message& message,
                                  ViolationHandlerFunc handler) {
  bool halt_validation = false;
  return Validate(message, handler, &halt_validation);
}

Status MessageValidator::Validate(const Message& message,
                                  ViolationHandlerFunc handler,
                                  bool* halt_validation) {
  // ConstraintsFor may recursively build constraints so
  // we lock the mutex here to ensure thread safety.
  mutex_.Lock();
  auto status_or_constraints = ConstraintsFor(message.GetDescriptor());
  mutex_.Unlock();

  if (!status_or_constraints.ok()) {
    return status_or_constraints.status();
  }

  auto constraints = status_or_constraints.ValueOrDie();

  // Keep the first failure to return to the caller.
  Status accumulative_status = Status::OK();

  // Validate the constraints attached to the message's fields.
  for (auto expression : constraints->expressions_) {
    if (*halt_validation) {
      return accumulative_status;
    }

    const FieldDescriptor* field = expression.first;
    const CompiledExpression& expr = expression.second;

    ForEachMessageHalting<Message>(message, field, [&](const Message& child) {
      UpdateStatus(&accumulative_status,
                   ValidateConstraint(message, field, child, expr, handler,
                                      halt_validation));

      return *halt_validation;
    });
  }

  // Recursively validate constraints for nested messages that have them.
  for (const FieldDescriptor* field : constraints->nested_with_constraints_) {
    if (*halt_validation) {
      return accumulative_status;
    }

    ForEachMessageHalting<Message>(message, field, [&](const Message& child) {
      UpdateStatus(&accumulative_status,
                   Validate(child, handler, halt_validation));
      return *halt_validation;
    });
  }

  return accumulative_status;
}

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
