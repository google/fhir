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

#include "google/protobuf/util/message_differencer.h"
#include "absl/strings/str_cat.h"

// Include the ANTLR-generated visitor, lexer and parser files.
#include "absl/memory/memory.h"
#include "absl/strings/numbers.h"
#include "google/fhir/annotations.h"
#include "google/fhir/fhir_path/FhirPathBaseVisitor.h"
#include "google/fhir/fhir_path/FhirPathLexer.h"
#include "google/fhir/fhir_path/FhirPathParser.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "proto/r4/core/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace fhir_path {

using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::MessageOptions;
using ::google::protobuf::Reflection;
using ::google::protobuf::util::MessageDifferencer;

// Used to wrap primitives in protobuf messages, and
// can be used against multiple versions of FHIR, not just R4.
using ::google::fhir::r4::core::Boolean;
using ::google::fhir::r4::core::DateTime;
using ::google::fhir::r4::core::Decimal;
using ::google::fhir::r4::core::Integer;
using ::google::fhir::r4::core::String;

using antlr4::ANTLRInputStream;
using antlr4::BaseErrorListener;
using antlr4::CommonTokenStream;
using antlr4::tree::TerminalNode;

using antlr_parser::FhirPathBaseVisitor;
using antlr_parser::FhirPathLexer;
using antlr_parser::FhirPathParser;

using ::google::fhir::ForEachMessage;
using ::google::fhir::ForEachMessageHalting;
using ::google::fhir::IsMessageType;
using ::google::fhir::StatusOr;

using internal::ExpressionNode;

using ::tensorflow::errors::InvalidArgument;


namespace internal {

// Returns true if the collection of messages represents
// a boolean value per FHIRPath conventions; that is it
// has exactly one item that is boolean.
bool IsSingleBoolean(const std::vector<const Message*>& messages) {
  return messages.size() == 1 && IsMessageType<Boolean>(*messages[0]);
}

// Returns success with a boolean value if the message collection
// represents a single boolean, or a failure status otherwise.
StatusOr<bool> MessagesToBoolean(const std::vector<const Message*>& messages) {
  if (IsSingleBoolean(messages)) {
    return dynamic_cast<const Boolean*>(messages[0])->value();
  }

  return InvalidArgument("Expression did not evaluate to boolean");
}

// Supported functions.
constexpr char kExistsFunction[] = "exists";
constexpr char kNotFunction[] = "not";
constexpr char kHasValueFunction[] = "hasValue";

// Expression node that returns literals wrapped in the corresponding
// protbuf wrapper
template <typename ProtoType, typename PrimitiveType>
class Literal : public ExpressionNode {
 public:
  explicit Literal(PrimitiveType value) : value_(value) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<const Message*>* results) const override {
    auto value = new ProtoType();
    value->set_value(value_);
    work_space->DeleteWhenFinished(value);
    results->push_back(value);

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return ProtoType::descriptor();
  }

 private:
  const PrimitiveType value_;
};

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

    Boolean* result = new Boolean();
    work_space->DeleteWhenFinished(result);
    result->set_value(!child_results.empty());
    results->push_back(result);

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
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

      Boolean* result = new Boolean();
      work_space->DeleteWhenFinished(result);
      result->set_value(!child_result);

      results->push_back(result);
    }

    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }

 private:
  const std::shared_ptr<ExpressionNode> child_;
};

// Implements the FHIRPath .hasValue() function, which returns true
// if and only if the child is a single primitive value.
class HasValueFunction : public ExpressionNode {
 public:
  explicit HasValueFunction(const std::shared_ptr<ExpressionNode>& child)
      : child_(child) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<const Message*>* results) const override {
    std::vector<const Message*> child_results;

    FHIR_RETURN_IF_ERROR(child_->Evaluate(work_space, &child_results));

    Boolean* result = new Boolean();
    work_space->DeleteWhenFinished(result);

    if (child_results.size() != 1) {
      result->set_value(false);
    } else {
      const MessageOptions& options =
          child_results[0]->GetDescriptor()->options();
      result->set_value(
          options.HasExtension(proto::structure_definition_kind) &&
          (options.GetExtension(proto::structure_definition_kind) ==
           proto::StructureDefinitionKindValue::KIND_PRIMITIVE_TYPE));
    }

    results->push_back(result);
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }

 private:
  const std::shared_ptr<ExpressionNode> child_;
};

// Base class for FHIRPath binary operators.
class BinaryOperator : public ExpressionNode {
 public:
  Status Evaluate(WorkSpace* work_space,
                  std::vector<const Message*>* results) const override {
    std::vector<const Message*> left_results;
    FHIR_RETURN_IF_ERROR(left_->Evaluate(work_space, &left_results));

    std::vector<const Message*> right_results;
    FHIR_RETURN_IF_ERROR(right_->Evaluate(work_space, &right_results));

    return EvaluateOperator(left_results, right_results, work_space, results);
  }

  BinaryOperator(std::shared_ptr<ExpressionNode> left,
                 std::shared_ptr<ExpressionNode> right)
      : left_(left), right_(right) {}

  // Perform the actual boolean evaluation.
  virtual Status EvaluateOperator(
      const std::vector<const Message*>& left_results,
      const std::vector<const Message*>& right_results, WorkSpace* work_space,
      std::vector<const Message*>* out_results) const = 0;

 private:
  const std::shared_ptr<ExpressionNode> left_;

  const std::shared_ptr<ExpressionNode> right_;
};

class EqualsOperator : public BinaryOperator {
 public:
  Status EvaluateOperator(
      const std::vector<const Message*>& left_results,
      const std::vector<const Message*>& right_results, WorkSpace* work_space,
      std::vector<const Message*>* out_results) const override {
    Boolean* result = new Boolean();
    work_space->DeleteWhenFinished(result);

    if (left_results.size() != right_results.size()) {
      result->set_value(false);
    } else {
      // Scan for unequal messages.
      result->set_value(true);
      for (int i = 0; i < left_results.size(); ++i) {
        if (!MessageDifferencer::Equals(*left_results.at(i),
                                        *right_results.at(i))) {
          result->set_value(false);
          break;
        }
      }
    }

    out_results->push_back(result);
    return Status::OK();
  }

  const Descriptor* ReturnType() const override {
    return Boolean::descriptor();
  }

  EqualsOperator(std::shared_ptr<ExpressionNode> left,
                 std::shared_ptr<ExpressionNode> right)
      : BinaryOperator(left, right) {}
};

// Converts decimal or integer container messages to a double value
static Status MessageToDouble(const Message& message, double* value) {
  if (IsMessageType<Decimal>(message)) {
    const Decimal* decimal = dynamic_cast<const Decimal*>(&message);

    if (!absl::SimpleAtod(decimal->value(), value)) {
      return InvalidArgument(
          absl::StrCat("Could not convert to numeric: ", decimal->value()));
    }

    return Status::OK();

  } else if (IsMessageType<Integer>(message)) {
    const Integer* integer = dynamic_cast<const Integer*>(&message);

    *value = integer->value();
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
      const std::vector<const Message*>& left_results,
      const std::vector<const Message*>& right_results, WorkSpace* work_space,
      std::vector<const Message*>* out_results) const override {
    // Per the FHIRPath spec, comparison operators propagate empty results.
    if (left_results.empty() || right_results.empty()) {
      return Status::OK();
    }

    if (left_results.size() > 1 || right_results.size() > 1) {
      return InvalidArgument(
          "Comparison operators must have one element on each side.");
    }

    const Message* left_result = left_results[0];
    const Message* right_result = right_results[0];

    Boolean* result = new Boolean();
    work_space->DeleteWhenFinished(result);

    if (IsMessageType<Integer>(*left_result) &&
        IsMessageType<Integer>(*right_result)) {
      EvalIntegerComparison(dynamic_cast<const Integer*>(left_result),
                            dynamic_cast<const Integer*>(right_result), result);

    } else if (IsMessageType<Decimal>(*left_result) ||
               IsMessageType<Decimal>(*right_result)) {
      FHIR_RETURN_IF_ERROR(
          EvalDecimalComparison(left_result, right_result, result));

    } else if (IsMessageType<String>(*left_result) &&
               IsMessageType<String>(*right_result)) {
      EvalStringComparison(dynamic_cast<const String*>(left_result),
                           dynamic_cast<const String*>(right_result), result);
    } else if (IsMessageType<DateTime>(*left_result) &&
               IsMessageType<DateTime>(*right_result)) {
      FHIR_RETURN_IF_ERROR(EvalDateTimeComparison(
          dynamic_cast<const DateTime*>(left_result),
          dynamic_cast<const DateTime*>(right_result), result));
    } else {
      return InvalidArgument("Unsupported comparison value types");
    }

    out_results->push_back(result);
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
  void EvalIntegerComparison(const Integer* left_wrapper,
                             const Integer* right_wrapper,
                             Boolean* result) const {
    const int32_t left = left_wrapper->value();
    const int32_t right = right_wrapper->value();

    switch (comparison_type_) {
      case kLessThan:
        result->set_value(left < right);
        break;
      case kGreaterThan:
        result->set_value(left > right);
        break;
      case kLessThanEqualTo:
        result->set_value(left <= right);
        break;
      case kGreaterThanEqualTo:
        result->set_value(left >= right);
        break;
    }
  }

  Status EvalDecimalComparison(const Message* left_message,
                               const Message* right_message,
                               Boolean* result) const {
    // Handle decimal comparisons, converting integer types
    // if necessary.
    double left;
    FHIR_RETURN_IF_ERROR(MessageToDouble(*left_message, &left));
    double right;
    FHIR_RETURN_IF_ERROR(MessageToDouble(*right_message, &right));

    switch (comparison_type_) {
      case kLessThan:
        result->set_value(left < right);
        break;
      case kGreaterThan:
        result->set_value(left > right);
        break;
      case kLessThanEqualTo:
        // Fallback to literal comparison for equality to avoid
        // rounding errors.
        result->set_value(
            left <= right ||
            (left_message->GetDescriptor() == right_message->GetDescriptor() &&
             MessageDifferencer::Equals(*left_message, *right_message)));
        break;
      case kGreaterThanEqualTo:
        // Fallback to literal comparison for equality to avoid
        // rounding errors.
        result->set_value(
            left >= right ||
            (left_message->GetDescriptor() == right_message->GetDescriptor() &&
             MessageDifferencer::Equals(*left_message, *right_message)));
        break;
    }

    return Status::OK();
  }

  void EvalStringComparison(const String* left_message,
                            const String* right_message,
                            Boolean* result) const {
    const std::string& left = left_message->value();
    const std::string& right = right_message->value();

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
        result->set_value(compare_result < 0);
        break;
      case kGreaterThan:
        result->set_value(compare_result > 0);
        break;
      case kLessThanEqualTo:
        result->set_value(compare_result <= 0);
        break;
      case kGreaterThanEqualTo:
        result->set_value(compare_result >= 0);
        break;
    }
  }

  Status EvalDateTimeComparison(const DateTime* left_message,
                                const DateTime* right_message,
                                Boolean* result) const {
    // TODO: consider support for cross-timezone comparisons.
    if (left_message->timezone() != right_message->timezone()) {
      return InvalidArgument(
          "Date comparisons only supported in same timezone");
    }

    const int64_t left = left_message->value_us();
    const int64_t right = right_message->value_us();

    switch (comparison_type_) {
      case kLessThan:
        result->set_value(left < right);
        break;
      case kGreaterThan:
        result->set_value(left > right);
        break;
      case kLessThanEqualTo:
        result->set_value(left <= right);
        break;
      case kGreaterThanEqualTo:
        result->set_value(left >= right);
        break;
    }

    return Status::OK();
  }

  ComparisonType comparison_type_;
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
                 std::vector<const Message*>* results) const {
    Boolean* result = new Boolean();
    work_space->DeleteWhenFinished(result);
    result->set_value(eval_result);
    results->push_back(result);
  }

  const std::shared_ptr<ExpressionNode> left_;
  const std::shared_ptr<ExpressionNode> right_;
};

class OrOperator : public BooleanOperator {
 public:
  OrOperator(std::shared_ptr<ExpressionNode> left,
             std::shared_ptr<ExpressionNode> right)
      : BooleanOperator(left, right) {}

  Status Evaluate(WorkSpace* work_space,
                  std::vector<const Message*>* results) const override {
    // Logic from truth table spec: http://hl7.org/fhirpath/#boolean-logic
    // Short circuit and return true on the first true result.
    std::vector<const Message*> left_results;
    FHIR_RETURN_IF_ERROR(left_->Evaluate(work_space, &left_results));
    if (!left_results.empty()) {
      FHIR_ASSIGN_OR_RETURN(bool left_result, MessagesToBoolean(left_results));
      if (left_result) {
        SetResult(true, work_space, results);
        return Status::OK();
      }
    }

    std::vector<const Message*> right_results;
    FHIR_RETURN_IF_ERROR(right_->Evaluate(work_space, &right_results));
    if (!right_results.empty()) {
      FHIR_ASSIGN_OR_RETURN(bool right_result,
                            MessagesToBoolean(right_results));
      if (right_result) {
        SetResult(true, work_space, results);
        return Status::OK();
      }
    }

    if (!left_results.empty() && !right_results.empty()) {
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
                  std::vector<const Message*>* results) const override {
    // Logic from truth table spec: http://hl7.org/fhirpath/#boolean-logic
    // Short circuit and return false on the first false result.
    std::vector<const Message*> left_results;
    FHIR_RETURN_IF_ERROR(left_->Evaluate(work_space, &left_results));
    if (!left_results.empty()) {
      FHIR_ASSIGN_OR_RETURN(bool left_result, MessagesToBoolean(left_results));
      if (!left_result) {
        SetResult(false, work_space, results);
        return Status::OK();
      }
    }

    std::vector<const Message*> right_results;
    FHIR_RETURN_IF_ERROR(right_->Evaluate(work_space, &right_results));
    if (!right_results.empty()) {
      FHIR_ASSIGN_OR_RETURN(bool right_result,
                            MessagesToBoolean(right_results));
      if (!right_result) {
        SetResult(false, work_space, results);
        return Status::OK();
      }
    }

    if (!left_results.empty() && !right_results.empty()) {
      // Both children must be true to get here, so return true.
      SetResult(true, work_space, results);
      return Status::OK();
    }

    // Neither child is false and at least one is empty, so propagate
    // empty per the FHIRPath spec.
    return Status::OK();
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
  InvocationDefinition(const std::string& name, const bool is_function)
      : name(name), is_function(is_function) {}

  const std::string name;

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
      : error_listener_(this), descriptor_(descriptor) {}

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
        SetError(absl::StrCat("Unable to find field", definition->name));
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
      SetError(absl::StrCat("Unable to find field", definition->name));
      return nullptr;

    } else {
      return ToAny(std::make_shared<InvokeTermNode>(field));
    }
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

    std::string text =
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

  antlrcpp::Any visitTerminal(TerminalNode* node) override {
    const std::string& text = node->getSymbol()->getText();

    switch (node->getSymbol()->getType()) {
      case FhirPathLexer::NUMBER:
        // Determine if the number is an integer or decimal, propagating
        // decimal types in string form to preserve precision.
        if (text.find(".") != std::string::npos) {
          return ToAny(std::make_shared<Literal<Decimal, std::string>>(text));
        } else {
          int32_t value;
          if (!absl::SimpleAtoi(text, &value)) {
            SetError(absl::StrCat("Malformed integer ", text));
            return nullptr;
          }

          return ToAny(std::make_shared<Literal<Integer, int32_t>>(value));
        }

      case FhirPathLexer::STRING:
        // The lexer keeps the quotes around string literals,
        // so we remove them here. The following assert simply reflects
        // the lexer's guarantees as defined.
        assert(text.length() >= 2);
        return ToAny(std::make_shared<Literal<String, std::string>>(
            text.substr(1, text.length() - 2)));

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
  // Returns an ExpressionNode that implements the
  // specified FHIRPath function.
  std::shared_ptr<ExpressionNode> createFunction(
      const std::string& function_name,
      std::shared_ptr<ExpressionNode> child_expression) {
    if (function_name == kExistsFunction) {
      return std::make_shared<ExistsFunction>(child_expression);
    } else if (function_name == kNotFunction) {
      return std::make_shared<NotFunction>(child_expression);
    } else if (function_name == kHasValueFunction) {
      return std::make_shared<HasValueFunction>(child_expression);
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
    FhirPathErrorListener(FhirPathCompilerVisitor* visitor)
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
  const Descriptor* descriptor_;
  std::string error_message_;
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

StatusOr<int32_t> EvaluationResult::GetInteger() const {
  auto messages = work_space_->GetResultMessages();

  if (messages.size() == 1 && IsMessageType<Integer>(*messages[0])) {
    return dynamic_cast<const Integer*>(messages[0])->value();
  }

  return InvalidArgument("Expression did not evaluate to integer");
}

StatusOr<std::string> EvaluationResult::GetDecimal() const {
  auto messages = work_space_->GetResultMessages();

  if (messages.size() == 1 && IsMessageType<Decimal>(*messages[0])) {
    return dynamic_cast<const Decimal*>(messages[0])->value();
  }

  return InvalidArgument("Expression did not evaluate to decimal");
}

StatusOr<std::string> EvaluationResult::GetString() const {
  auto messages = work_space_->GetResultMessages();

  if (messages.size() == 1 && IsMessageType<String>(*messages[0])) {
    return dynamic_cast<const String*>(messages[0])->value();
  }

  return InvalidArgument("Expression did not evaluate to string");
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

const std::string& CompiledExpression::fhir_path() const { return fhir_path_; }

CompiledExpression::CompiledExpression(
    const std::string& fhir_path,
    std::shared_ptr<internal::ExpressionNode> root_expression)
    : fhir_path_(fhir_path), root_expression_(root_expression) {}

StatusOr<CompiledExpression> CompiledExpression::Compile(
    const Descriptor* descriptor, const std::string& fhir_path) {
  ANTLRInputStream input(fhir_path);
  FhirPathLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  FhirPathParser parser(&tokens);

  internal::FhirPathCompilerVisitor visitor(descriptor);
  parser.addErrorListener(visitor.GetErrorListener());
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

  int ext_size =
      descriptor->options().ExtensionSize(proto::fhir_path_message_constraint);

  for (int i = 0; i < ext_size; ++i) {
    const std::string& fhir_path = descriptor->options().GetExtension(
        proto::fhir_path_message_constraint, i);
    auto constraint = CompiledExpression::Compile(descriptor, fhir_path);
    if (constraint.ok()) {
      CompiledExpression expression = constraint.ValueOrDie();
      constraints->message_expressions_.push_back(expression);
    }

    // TODO: Unsupported FHIRPath expressions are simply not
    // validated for now; this should produce an error once we support
    // all of FHIRPath.
  }

  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);

    const Descriptor* field_type = field->message_type();

    // Constraints only apply to non-primitives.
    if (field_type != nullptr) {
      int ext_size =
          field->options().ExtensionSize(proto::fhir_path_constraint);

      for (int j = 0; j < ext_size; ++j) {
        const std::string& fhir_path =
            field->options().GetExtension(proto::fhir_path_constraint, j);

        auto constraint = CompiledExpression::Compile(field_type, fhir_path);

        if (constraint.ok()) {
          constraints->field_expressions_.push_back(
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
      if (!child_constraints->message_expressions_.empty() ||
          !child_constraints->field_expressions_.empty() ||
          !child_constraints->nested_with_constraints_.empty()) {
        constraints_local->nested_with_constraints_.push_back(field);
      }
    }
  }

  return constraints_local;
}

// Default handler halts on first error
bool HaltOnErrorHandler(const Message& message, const FieldDescriptor* field,
                        const std::string& constraint) {
  return true;
}

Status MessageValidator::Validate(const Message& message) {
  return Validate(message, HaltOnErrorHandler);
}

// Validates that the given message satisfies the
// the given FHIRPath expression, invoking the handler in case
// of failure.
Status ValidateMessageConstraint(const Message& message,
                                 const CompiledExpression& expression,
                                 const ViolationHandlerFunc handler,
                                 bool* halt_validation) {
  FHIR_ASSIGN_OR_RETURN(const EvaluationResult expr_result,
                        expression.Evaluate(message));

  if (!expr_result.GetBoolean().ValueOrDie()) {
    std::string err_msg = absl::StrCat("fhirpath-constraint-violation-",
                                       message.GetDescriptor()->name());

    *halt_validation = handler(message, nullptr, expression.fhir_path());
    return ::tensorflow::errors::FailedPrecondition(err_msg);
  }

  return Status::OK();
}

// Validates that the given field in the given parent satisfies the
// the given FHIRPath expression, invoking the handler in case
// of failure.
Status ValidateFieldConstraint(const Message& parent,
                               const FieldDescriptor* field,
                               const Message& field_value,
                               const CompiledExpression& expression,
                               const ViolationHandlerFunc handler,
                               bool* halt_validation) {
  FHIR_ASSIGN_OR_RETURN(const EvaluationResult expr_result,
                        expression.Evaluate(field_value));

  if (!expr_result.GetBoolean().ValueOrDie()) {
    std::string err_msg =
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

  // Validate the constraints attached to the message root.
  for (const CompiledExpression& expr : constraints->message_expressions_) {
    UpdateStatus(
        &accumulative_status,
        ValidateMessageConstraint(message, expr, handler, halt_validation));
    if (*halt_validation) {
      return accumulative_status;
    }
  }

  // Validate the constraints attached to the message's fields.
  for (auto expression : constraints->field_expressions_) {
    if (*halt_validation) {
      return accumulative_status;
    }

    const FieldDescriptor* field = expression.first;
    const CompiledExpression& expr = expression.second;

    ForEachMessageHalting<Message>(message, field, [&](const Message& child) {
      UpdateStatus(&accumulative_status,
                   ValidateFieldConstraint(message, field, child, expr, handler,
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

// Common validator instance for the lifetime of the process.
static MessageValidator* validator = new MessageValidator();

Status ValidateMessage(const ::google::protobuf::Message& message) {
  return validator->Validate(message);
}

Status ValidateMessage(const ::google::protobuf::Message& message,
                       ViolationHandlerFunc handler) {
  return validator->Validate(message, handler);
}

}  // namespace fhir_path
}  // namespace fhir
}  // namespace google
