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

#ifndef GOOGLE_FHIR_EXTENSIONS_H_
#define GOOGLE_FHIR_EXTENSIONS_H_

#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/reflection.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "proto/r4/core/datatypes.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {


Status ExtensionToMessage(const stu3::proto::Extension& extension,
                          ::google::protobuf::Message* message);

Status ExtensionToMessage(const r4::core::Extension& extension,
                          ::google::protobuf::Message* message);

Status ConvertToExtension(const ::google::protobuf::Message& message,
                          stu3::proto::Extension* extension);

Status ConvertToExtension(const ::google::protobuf::Message& message,
                          r4::core::Extension* extension);

// Given a datatype message (E.g., String, Code, Boolean, etc.),
// finds the appropriate field on the target extension and sets it.
// Returns InvalidArgument if there's no matching oneof type on the extension
// for the message.
Status SetDatatypeOnExtension(const ::google::protobuf::Message& message,
                              stu3::proto::Extension* extension);
Status SetDatatypeOnExtension(const ::google::protobuf::Message& message,
                              r4::core::Extension* extension);

Status ValidateExtension(const ::google::protobuf::Descriptor* descriptor);

// Extract all matching extensions from a container into a vector, and parse
// them into protos. Example usage:
// Patient patient = ...
// std::vector<MyExtension> my_extensions;
// auto status = GetRepeatedFromExtension(patient.extension(), &my_extension);
template <class C, class T>
Status GetRepeatedFromExtension(const C& extension_container,
                                std::vector<T>* result) {
  // This function will be called a huge number of times, usually when no
  // extensions are present.  Return early in this case to keep overhead as low
  // as possible.
  if (extension_container.empty()) {
    return Status::OK();
  }
  const ::google::protobuf::Descriptor* descriptor = T::descriptor();
  FHIR_RETURN_IF_ERROR(ValidateExtension(descriptor));
  const std::string& url = descriptor->options().GetExtension(
      ::google::fhir::proto::fhir_structure_definition_url);
  for (const auto& extension : extension_container) {
    if (extension.url().value() == url) {
      T message;
      FHIR_RETURN_IF_ERROR(ExtensionToMessage(extension, &message));
      result->emplace_back(message);
    }
  }
  return Status::OK();
}

// Extracts a single extension of type T from 'entity'. Returns a NotFound error
// if there are zero extensions of that type. Returns an InvalidArgument error
// if there are more than one.
template <class T, class C>
StatusOr<T> ExtractOnlyMatchingExtension(const C& entity) {
  std::vector<T> result;
  FHIR_RETURN_IF_ERROR(GetRepeatedFromExtension(entity.extension(), &result));
  if (result.empty()) {
    return ::tensorflow::errors::NotFound(
        "Did not find any extension with url: ",
        GetStructureDefinitionUrl(T::descriptor()), " on ",
        C::descriptor()->full_name(), ".");
  }
  if (result.size() > 1) {
    return ::tensorflow::errors::InvalidArgument(
        "Expected exactly 1 extension with url: ",
        GetStructureDefinitionUrl(T::descriptor()), " on ",
        C::descriptor()->full_name(), ". Found: ", result.size());
  }
  return result.front();
}

Status ClearTypedExtensions(const ::google::protobuf::Descriptor* descriptor,
                            ::google::protobuf::Message* message);

Status ClearExtensionsWithUrl(const std::string& url,
                              ::google::protobuf::Message* message);

std::string GetInlinedExtensionUrl(const ::google::protobuf::FieldDescriptor* field);

const std::string& GetExtensionUrl(const google::protobuf::Message& extension,
                                   std::string* scratch);

const std::string& GetExtensionSystem(const google::protobuf::Message& extension,
                                      std::string* scratch);

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_EXTENSIONS_H_
