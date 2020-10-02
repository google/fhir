/*
 * Copyright 2020 Google LLC
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

#include "google/fhir/stu3/extensions.h"

#include "google/fhir/status/statusor.h"
#include "proto/google/fhir/proto/stu3/datatypes.pb.h"

namespace google {
namespace fhir {
namespace stu3 {

absl::Status ExtensionToMessage(const proto::Extension& extension,
                                ::google::protobuf::Message* message) {
  return extensions_templates::ExtensionToMessage<proto::Extension>(extension,
                                                                    message);
}

absl::Status ConvertToExtension(const ::google::protobuf::Message& message,
                                proto::Extension* extension) {
  return extensions_templates::ConvertToExtension(message, extension);
}

absl::Status SetDatatypeOnExtension(const ::google::protobuf::Message& message,
                                    proto::Extension* extension) {
  return extensions_templates::SetDatatypeOnExtension(message, extension);
}

absl::Status ValueToMessage(
    const ::google::fhir::stu3::proto::Extension& extension,
    ::google::protobuf::Message* message, const ::google::protobuf::FieldDescriptor* field) {
  return extensions_templates::ValueToMessage(extension, message, field);
}

}  // namespace stu3
}  // namespace fhir
}  // namespace google
