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

#ifndef GOOGLE_FHIR_R4_EXTENSIONS_H_
#define GOOGLE_FHIR_R4_EXTENSIONS_H_

#include "google/protobuf/message.h"
#include "absl/status/status.h"
#include "google/fhir/extensions.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"

namespace google {
namespace fhir {
namespace r4 {

// TODO: Deprecate ane eliminate this file in favor of unversioned
// extension util.

absl::Status ExtensionToMessage(const core::Extension& extension,
                                ::google::protobuf::Message* message);

absl::Status ConvertToExtension(const ::google::protobuf::Message& message,
                                core::Extension* extension);

// Given a datatype message (E.g., String, Code, Boolean, etc.),
// finds the appropriate field on the target extension and sets it.
// Returns InvalidArgument if there's no matching oneof type on the extension
// for the message.
absl::Status SetDatatypeOnExtension(const ::google::protobuf::Message& message,
                                    core::Extension* extension);

template <class T, class C>
inline absl::StatusOr<T> ExtractOnlyMatchingExtension(const C& entity) {
  return google::fhir::ExtractOnlyMatchingExtension<T>(entity);
}

}  // namespace r4
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_R4_EXTENSIONS_H_
