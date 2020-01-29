/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GOOGLE_FHIR_PRIMITIVE_WRAPPER_H_
#define GOOGLE_FHIR_PRIMITIVE_WRAPPER_H_

#include <memory>
#include <string>

#include "absl/strings/str_cat.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "absl/time/time.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "proto/annotations.pb.h"
#include "include/json/json.h"
#include "tensorflow/core/lib/core/status.h"

namespace google {
namespace fhir {


Status BuildHasNoValueExtension(::google::protobuf::Message* extension);

struct JsonPrimitive {
  std::string value;
  std::unique_ptr<::google::protobuf::Message> element;

  const bool is_non_null() const { return value != "null"; }
};

::google::fhir::Status ParseInto(const Json::Value& json,
                                 const proto::FhirVersion fhir_version,
                                 const absl::TimeZone tz,
                                 ::google::protobuf::Message* target);

::google::fhir::StatusOr<JsonPrimitive> WrapPrimitiveProto(
    const ::google::protobuf::Message& proto);

::google::fhir::Status ValidatePrimitive(const ::google::protobuf::Message& primitive);

namespace primitives_internal {

class PrimitiveWrapper {
 public:
  virtual ~PrimitiveWrapper() {}
  virtual Status MergeInto(::google::protobuf::Message* target) const = 0;
  virtual Status Parse(const Json::Value& json,
                       const absl::TimeZone& default_time_zone) = 0;
  virtual Status Wrap(const ::google::protobuf::Message&) = 0;
  virtual bool HasElement() const = 0;
  virtual StatusOr<std::unique_ptr<::google::protobuf::Message>> GetElement() const = 0;

  virtual Status ValidateProto() const = 0;

  StatusOr<std::string> ToValueString() const {
    static const char* kNullString = "null";
    if (HasValue()) {
      return ToNonNullValueString();
    }
    return absl::StrCat(kNullString);
  }

 protected:
  virtual bool HasValue() const = 0;
  virtual StatusOr<std::string> ToNonNullValueString() const = 0;
};

}  // namespace primitives_internal

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_PRIMITIVE_WRAPPER_H_
