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

#include "google/fhir/r4/extensions.h"

#include "google/fhir/extensions.h"
#include "proto/r4/core/datatypes.pb.h"

namespace google {
namespace fhir {
namespace r4 {

Status ExtensionToMessage(const core::Extension& extension,
                          ::google::protobuf::Message* message) {
  return extensions_templates::ExtensionToMessage<core::Extension>(extension,
                                                                   message);
}

Status ConvertToExtension(const ::google::protobuf::Message& message,
                          core::Extension* extension) {
  return extensions_templates::ConvertToExtension(message, extension);
}

Status SetDatatypeOnExtension(const ::google::protobuf::Message& message,
                              core::Extension* extension) {
  return extensions_templates::SetDatatypeOnExtension(message, extension);
}

Status ValueToMessage(const ::google::fhir::r4::core::Extension& extension,
                      ::google::protobuf::Message* message,
                      const ::google::protobuf::FieldDescriptor* field) {
  return extensions_templates::ValueToMessage(extension, message, field);
}

}  // namespace r4
}  // namespace fhir
}  // namespace google
