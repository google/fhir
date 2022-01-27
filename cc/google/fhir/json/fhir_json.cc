// Copyright 2022 Google LLC
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

#include "google/fhir/json/fhir_json.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"

namespace google {
namespace fhir {
namespace internal {

FhirJson::FhirJson() : type_(FhirJson::nullValue) {}

FhirJson::FhirJson(ValueType type) : type_(type) {}

std::unique_ptr<FhirJson> FhirJson::CreateNull() {
  return absl::WrapUnique(new FhirJson(FhirJson::nullValue));
}

std::unique_ptr<FhirJson> FhirJson::CreateBoolean(bool val) {
  auto json = absl::WrapUnique(new FhirJson(FhirJson::booleanValue));
  json->int_value_ = val;
  return json;
}

std::unique_ptr<FhirJson> FhirJson::CreateInteger(int64_t val) {
  auto json = absl::WrapUnique(new FhirJson(FhirJson::intValue));
  json->int_value_ = val;
  return json;
}

std::unique_ptr<FhirJson> FhirJson::CreateUnsigned(uint64_t val) {
  auto json = absl::WrapUnique(new FhirJson(FhirJson::uintValue));
  json->uint_value_ = val;
  return json;
}

std::unique_ptr<FhirJson> FhirJson::CreateDecimal(const std::string& val) {
  auto json = absl::WrapUnique(new FhirJson(FhirJson::realValue));
  json->string_value_ = val;
  return json;
}

std::unique_ptr<FhirJson> FhirJson::CreateString(const std::string& val) {
  auto json = absl::WrapUnique(new FhirJson(FhirJson::stringValue));
  json->string_value_ = val;
  return json;
}

std::unique_ptr<FhirJson> FhirJson::CreateObject() {
  return absl::WrapUnique(new FhirJson(FhirJson::objectValue));
}

std::unique_ptr<FhirJson> FhirJson::CreateArray() {
  return absl::WrapUnique(new FhirJson(FhirJson::arrayValue));
}

absl::Status FhirJson::MoveFrom(std::unique_ptr<FhirJson> other) {
  if (type_ != FhirJson::nullValue) {
    return absl::FailedPreconditionError(
        "MoveFrom() can only be invoke on nullValue");
  }
  type_ = other->type_;
  int_value_ = other->int_value_;
  uint_value_ = other->uint_value_;
  string_value_ = other->string_value_;
  for (auto iter = other->object_value_.begin();
       iter != other->object_value_.end(); ++iter) {
    object_value_.emplace(iter->first, std::move(iter->second));
  }
  for (auto& val : other->array_value_) {
    array_value_.emplace_back(std::move(val));
  }
  return absl::OkStatus();
}

std::string FhirJson::toString() const {
  std::string output;
  writeToString(output, 0, false);
  return output;
}

namespace {

void appendToString(std::string& s, const std::string& to_append,
                    int num_spaces) {
  for (int i = 0; i < num_spaces; ++i) {
    absl::StrAppend(&s, " ");
  }
  absl::StrAppend(&s, to_append);
}

}  // namespace

void FhirJson::writeToString(std::string& s, int depth,
                             bool is_on_new_line) const {
  static constexpr int kIndentation = 2;
  const int num_spaces_first_token = is_on_new_line ? depth * kIndentation : 0;
  const int num_spaces_other_tokens = depth * kIndentation;

  switch (type_) {
    case FhirJson::nullValue:
      appendToString(s, "null", num_spaces_first_token);
      break;
    case FhirJson::booleanValue:
      appendToString(s, int_value_ ? "true" : "false", num_spaces_first_token);
      break;
    case FhirJson::intValue:
      appendToString(s, absl::StrCat(int_value_), num_spaces_first_token);
      break;
    case FhirJson::uintValue:
      appendToString(s, absl::StrCat(uint_value_), num_spaces_first_token);
      break;
    case FhirJson::realValue:
      appendToString(s, string_value_, num_spaces_first_token);
      break;
    case FhirJson::stringValue:
      appendToString(s, absl::StrCat("\"", string_value_, "\""),
                     num_spaces_first_token);
      break;
    case FhirJson::objectValue: {
      appendToString(s, "{", num_spaces_first_token);
      absl::StrAppend(&s, "\n");
      int items_remaining = object_value_.size();
      for (auto iter = object_value_.begin(); iter != object_value_.end();
           ++iter) {
        appendToString(s, absl::StrCat("\"", iter->first, "\": "),
                       num_spaces_other_tokens + kIndentation);
        iter->second->writeToString(s, depth + 1, false);
        if (--items_remaining) {
          absl::StrAppend(&s, ",");
        }
        absl::StrAppend(&s, "\n");
      }
      appendToString(s, "}", num_spaces_other_tokens);
      break;
    }
    case FhirJson::arrayValue: {
      appendToString(s, "[", num_spaces_first_token);
      absl::StrAppend(&s, "\n");
      int items_remaining = array_value_.size();
      for (const auto& value : array_value_) {
        value->writeToString(s, depth + 1, true);
        if (--items_remaining) {
          absl::StrAppend(&s, ",");
        }
        absl::StrAppend(&s, "\n");
      }
      appendToString(s, "]", num_spaces_other_tokens);
      break;
    }
  }
}

bool FhirJson::isNull() const { return type_ == FhirJson::nullValue; }

bool FhirJson::isBool() const { return type_ == FhirJson::booleanValue; }

bool FhirJson::isInt() const {
  return type_ == FhirJson::intValue || type_ == FhirJson::uintValue;
}

bool FhirJson::isString() const {
  return type_ == FhirJson::stringValue || type_ == FhirJson::realValue;
}

bool FhirJson::isObject() const { return type_ == FhirJson::objectValue; }

bool FhirJson::isArray() const { return type_ == FhirJson::arrayValue; }

absl::StatusOr<bool> FhirJson::asBool() const {
  switch (type_) {
    case FhirJson::booleanValue:
    case FhirJson::intValue:
      return static_cast<bool>(int_value_);
    case FhirJson::uintValue:
      return static_cast<bool>(uint_value_);
    default:
      return absl::FailedPreconditionError(
          absl::StrCat(typeString(), " cannot be coerced into bool"));
  }
}

absl::StatusOr<int64_t> FhirJson::asInt() const {
  switch (type_) {
    case FhirJson::booleanValue:
    case FhirJson::intValue:
      return static_cast<int64_t>(int_value_);
    case FhirJson::uintValue:
      return static_cast<int64_t>(uint_value_);
    default:
      return absl::FailedPreconditionError(
          absl::StrCat(typeString(), " cannot be coerced into int"));
  }
}

absl::StatusOr<std::string> FhirJson::asString() const {
  switch (type_) {
    case FhirJson::realValue:
    case FhirJson::stringValue:
      return string_value_;
    default:
      return absl::FailedPreconditionError(
          absl::StrCat(typeString(), " cannot be coerced into string"));
  }
}

absl::StatusOr<int> FhirJson::arraySize() const {
  if (type_ != FhirJson::arrayValue) {
    return absl::FailedPreconditionError(
        absl::StrCat(typeString(), " does not have arraySize()"));
  }
  return array_value_.size();
}

absl::StatusOr<FhirJson*> FhirJson::mutableValueToAppend() {
  if (type_ != FhirJson::arrayValue) {
    return absl::FailedPreconditionError(
        absl::StrCat(typeString(), " does not have mutableValueToAppend()"));
  }
  array_value_.emplace_back(CreateNull());
  return array_value_.back().get();
}

absl::StatusOr<const FhirJson*> FhirJson::get(int index) const {
  if (type_ != FhirJson::arrayValue) {
    return absl::FailedPreconditionError(
        absl::StrCat(typeString(), " does not have get(index)"));
  }
  if (index < 0 || index >= array_value_.size()) {
    return absl::OutOfRangeError(absl::StrFormat(
        "Array index %d out of range [0..%d]", index, array_value_.size() - 1));
  }
  return array_value_[index].get();
}

absl::StatusOr<FhirJson*> FhirJson::mutableValueForKey(const std::string& key) {
  if (type_ != FhirJson::objectValue) {
    return absl::FailedPreconditionError(
        absl::StrCat(typeString(), " does not have mutableValueForKey()"));
  }
  if (object_value_.find(key) != object_value_.end()) {
    return absl::AlreadyExistsError(
        absl::StrFormat("Object already contain key %s", key));
  }
  return object_value_.emplace(key, CreateNull()).first->second.get();
}

absl::StatusOr<const FhirJson*> FhirJson::get(const std::string& key) const {
  if (type_ != FhirJson::objectValue) {
    return absl::FailedPreconditionError(
        absl::StrCat(typeString(), " does not have get(key)"));
  }
  if (object_value_.find(key) == object_value_.end()) {
    return absl::NotFoundError(
        absl::StrFormat("Object does not contain key %s", key));
  }
  return object_value_.at(key).get();
}

absl::StatusOr<const std::map<std::string, std::unique_ptr<FhirJson>>*>
FhirJson::objectMap() const {
  if (type_ != FhirJson::objectValue) {
    return absl::FailedPreconditionError(
        absl::StrCat(typeString(), " does not have objectMap()"));
  }
  return &object_value_;
}

std::string FhirJson::typeString() const {
  switch (type_) {
    case FhirJson::nullValue:
      return "nullValue";
    case FhirJson::intValue:
      return "intValue";
    case FhirJson::uintValue:
      return "uintValue";
    case FhirJson::realValue:
      return "realValue";
    case FhirJson::stringValue:
      return "stringValue";
    case FhirJson::booleanValue:
      return "booleanValue";
    case FhirJson::arrayValue:
      return "arrayValue";
    case FhirJson::objectValue:
      return "objectValue";
  }
}

bool FhirJson::operator==(const FhirJson& other) const {
  if (type_ != other.type_) {
    return false;
  }
  switch (type_) {
    case FhirJson::nullValue:
      return true;
    case FhirJson::intValue:
    case FhirJson::booleanValue:
      return int_value_ == other.int_value_;
    case FhirJson::uintValue:
      return uint_value_ == other.uint_value_;
    case FhirJson::realValue:
    case FhirJson::stringValue:
      return string_value_ == other.string_value_;
    case FhirJson::arrayValue: {
      if (array_value_.size() != other.array_value_.size()) {
        return false;
      }
      for (int i = 0; i < array_value_.size(); ++i) {
        if (*array_value_[i] != *other.array_value_[i]) {
          return false;
        }
      }
      return true;
    }
    case FhirJson::objectValue: {
      if (object_value_.size() != other.object_value_.size()) {
        return false;
      }
      for (auto iter = object_value_.begin(); iter != object_value_.end();
           ++iter) {
        auto other_iter = other.object_value_.find(iter->first);
        if (other_iter == other.object_value_.end()) {
          return false;
        }
        if (*iter->second != *other_iter->second) {
          return false;
        }
      }
      return true;
    }
  }
}

bool FhirJson::operator!=(const FhirJson& other) const {
  return !this->operator==(other);
}

}  // namespace internal
}  // namespace fhir
}  // namespace google
