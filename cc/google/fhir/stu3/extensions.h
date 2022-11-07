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

#ifndef GOOGLE_FHIR_STU3_EXTENSIONS_H_
#define GOOGLE_FHIR_STU3_EXTENSIONS_H_

#include "google/fhir/extensions.h"
#include "google/fhir/status/statusor.h"
#include "proto/google/fhir/proto/stu3/datatypes.pb.h"

namespace google {
namespace fhir {
namespace stu3 {

// This adds all functions on extensions_lib, which work on any version, so that
// users don't need to care which functions are defined on the core extensions.h
// and can just import the r4 version
using namespace extensions_lib;  // NOLINT

absl::Status ExtensionToMessage(const proto::Extension& extension,
                                ::google::protobuf::Message* message);

absl::Status ConvertToExtension(const ::google::protobuf::Message& message,
                                proto::Extension* extension);

// Given a datatype message (E.g., String, Code, Boolean, etc.),
// finds the appropriate field on the target extension and sets it.
// Returns InvalidArgument if there's no matching oneof type on the extension
// for the message.
absl::Status SetDatatypeOnExtension(const ::google::protobuf::Message& message,
                                    proto::Extension* extension);

}  // namespace stu3
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_EXTENSIONS_H_
