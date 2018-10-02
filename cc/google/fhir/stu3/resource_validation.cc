// Copyright 2018 Google LLC
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

#include "google/fhir/stu3/resource_validation.h"

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;

namespace {

// Check if a required field is missing; if so, return true and set name to
// be the fully qualified name of the field that is missing.
bool HasMissingRequiredField(const google::protobuf::Message& message, string* name) {
  const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
  const google::protobuf::Reflection* reflection = message.GetReflection();
  for (int i = 0; i < descriptor->field_count(); i++) {
    const google::protobuf::FieldDescriptor* field = descriptor->field(i);
    if (field->options().HasExtension(stu3::proto::validation_requirement) &&
        field->options().GetExtension(stu3::proto::validation_requirement) ==
            stu3::proto::REQUIRED_BY_FHIR) {
      if ((field->is_repeated() &&
           reflection->FieldSize(message, field) == 0) ||
          (!field->is_repeated() && !reflection->HasField(message, field))) {
        *name = field->json_name();
        return true;
      }
    }
    if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      bool missing = false;
      if (field->is_repeated()) {
        for (int j = 0; j < reflection->FieldSize(message, field) && !missing;
             j++) {
          missing = HasMissingRequiredField(
              reflection->GetRepeatedMessage(message, field, j), name);
        }
      } else {
        missing = reflection->HasField(message, field) &&
                  HasMissingRequiredField(
                      reflection->GetMessage(message, field), name);
      }
      if (missing) {
        *name = absl::StrCat(field->name(), ".", *name);
        return true;
      }
    }
  }
  // Also verify that oneof fields are set.
  // Note that optional choice-types should have the containing message unset -
  // if the containing message is set, it should have a value set as well.
  for (int i = 0; i < descriptor->oneof_decl_count(); i++) {
    const google::protobuf::OneofDescriptor* oneof = descriptor->oneof_decl(i);
    if (!reflection->HasOneof(message, oneof) &&
        !oneof->options().GetExtension(stu3::proto::fhir_oneof_is_optional)) {
      *name = absl::StrCat(oneof->name(), "_oneof");
      return true;
    }
  }

  return false;
}

}  // namespace

Status ValidateFhirConstraints(const google::protobuf::Message& resource) {
  // Make sure all required fields are set.
  string missing_field_name;
  if (HasMissingRequiredField(resource, &missing_field_name)) {
    return ::tensorflow::errors::InvalidArgument(
        absl::StrCat("missing-", missing_field_name));
  }
  return Status::OK();
}
}  // namespace stu3
}  // namespace fhir
}  // namespace google
