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

#ifndef GOOGLE_FHIR_STU3_EXTENSIONS_H_
#define GOOGLE_FHIR_STU3_EXTENSIONS_H_

#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/reflection.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/status/status.h"
#include "proto/stu3/annotations.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;

tensorflow::Status ExtensionToMessage(const stu3::proto::Extension& extension,
                                      ::google::protobuf::Message* message);

tensorflow::Status ConvertToExtension(const ::google::protobuf::Message& message,
                                      stu3::proto::Extension* extension);

// Given a datatype message (E.g., String, Code, Boolean, etc.),
// finds the appropriate field on the target extension and sets it.
// Returns InvalidArgument if there's no matching oneof type on the extension
// for the message.
tensorflow::Status SetDatatypeOnExtension(const ::google::protobuf::Message& message,
                                          stu3::proto::Extension* extension);

tensorflow::Status ValidateExtension(const ::google::protobuf::Descriptor* descriptor);

// Extract all matching extensions from a container into a vector, and parse
// them into protos. Example usage:
// Patient patient = ...
// std::vector<MyExtension> my_extensions;
// auto status = GetRepeatedFromExtension(patient.extension(), &my_extension);
template <class C, class T>
tensorflow::Status GetRepeatedFromExtension(const C& extension_container,
                                            std::vector<T>* result) {
  const ::google::protobuf::Descriptor* descriptor = T::descriptor();
  TF_RETURN_IF_ERROR(ValidateExtension(descriptor));
  const string url = descriptor->options().GetExtension(
      stu3::proto::fhir_structure_definition_url);
  for (const auto& extension : extension_container) {
    if (extension.url().value() == url) {
      T message;
      TF_RETURN_IF_ERROR(ExtensionToMessage(extension, &message));
      result->emplace_back(message);
    }
  }
  return tensorflow::Status::OK();
}

tensorflow::Status ClearTypedExtensions(const ::google::protobuf::Descriptor* descriptor,
                                        ::google::protobuf::Message* message);

string GetInlinedExtensionUrl(const ::google::protobuf::FieldDescriptor* field);

}  // namespace stu3
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_EXTENSIONS_H_
