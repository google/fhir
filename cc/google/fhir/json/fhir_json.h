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

#ifndef GOOGLE_FHIR_JSON_FHIR_JSON_H_
#define GOOGLE_FHIR_JSON_FHIR_JSON_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "absl/status/statusor.h"

namespace google {
namespace fhir {
namespace internal {

// Container of a JSON value. Has specific FHIR implementation logic, such as
// storing decimals as strings.
class FhirJson {
 public:
  // Create a FhirJson representing a null JSON value.
  FhirJson();

  // Other constructors are hidden from the interface in favor of more explicit
  // factory methods. This allows us to also hide other implementation details,
  // such as the type enum.
  static std::unique_ptr<FhirJson> CreateNull();
  static std::unique_ptr<FhirJson> CreateBoolean(bool val);
  static std::unique_ptr<FhirJson> CreateInteger(int64_t val);
  static std::unique_ptr<FhirJson> CreateUnsigned(uint64_t val);
  static std::unique_ptr<FhirJson> CreateDecimal(const std::string& val);
  static std::unique_ptr<FhirJson> CreateString(const std::string& val);
  static std::unique_ptr<FhirJson> CreateObject();
  static std::unique_ptr<FhirJson> CreateArray();

  // Move all content of other to this. This can only be called when this is
  // a "nullValue", so in a way this is an init-function.
  absl::Status MoveFrom(std::unique_ptr<FhirJson> other);

  // Predicates on whether this FhirJson is convertible to a certain type.
  bool isNull() const;
  bool isBool() const;
  bool isInt() const;
  bool isString() const;
  bool isObject() const;
  bool isArray() const;

  // Convert the FhirJson to a commonly used type, or error if conversion is
  // illegal.
  absl::StatusOr<bool> asBool() const;
  absl::StatusOr<int64_t> asInt() const;
  absl::StatusOr<std::string> asString() const;

  // Utilities when this is an arrayValue.
  absl::StatusOr<int> arraySize() const;
  absl::StatusOr<FhirJson*> mutableValueToAppend();
  absl::StatusOr<const FhirJson*> get(int index) const;

  // Utilities when this is an objectValue.
  absl::StatusOr<FhirJson*> mutableValueForKey(const std::string& key);
  absl::StatusOr<const FhirJson*> get(const std::string& key) const;
  absl::StatusOr<const std::map<std::string, std::unique_ptr<FhirJson>>*>
  objectMap() const;

  // Return multi-line, indented representation of the JSON object.
  std::string toString() const;

  bool operator==(const FhirJson& other) const;
  bool operator!=(const FhirJson& other) const;

  // Return a string indicating the "type" of this FhirJson, useful for
  // constructing debug messages.
  std::string typeString() const;

 private:
  enum ValueType {
    nullValue = 0,  ///< 'null' value
    intValue,       ///< signed integer value
    uintValue,      ///< unsigned integer value
    realValue,      ///< double value
    stringValue,    ///< UTF-8 string value
    booleanValue,   ///< bool value
    arrayValue,     ///< array value (ordered list)
    objectValue     ///< object value (collection of name/value pairs).
  };

  explicit FhirJson(ValueType type);

  // Disallow copy and assignment. This avoid potential expensive operations
  // of copying deeply nested structures.
  FhirJson(const FhirJson& other) = delete;
  FhirJson& operator=(const FhirJson& other) = delete;

  // Helper method for toString().
  void writeToString(std::string& s, int depth, bool is_on_new_line) const;

  ValueType type_;
  int64_t int_value_;
  uint64_t uint_value_;
  std::string string_value_;
  std::map<std::string, std::unique_ptr<FhirJson>> object_value_;
  std::vector<std::unique_ptr<FhirJson>> array_value_;
};

}  // namespace internal
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_JSON_FHIR_JSON_H_
