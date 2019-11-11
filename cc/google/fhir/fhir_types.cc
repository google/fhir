// Copyright 2019 Google LLC
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

#include "google/fhir/fhir_types.h"

#include "google/protobuf/message.h"
#include "google/fhir/annotations.h"

namespace google {
namespace fhir {

using ::google::protobuf::Descriptor;
using ::google::protobuf::Message;

namespace {

bool IsType(const std::string& url, const Descriptor* descriptor) {
  return GetStructureDefinitionUrl(descriptor) == url;
}

bool IsProfileOf(const std::string& url, const Descriptor* descriptor) {
  for (int i = 0;
       i < descriptor->options().ExtensionSize(proto::fhir_profile_base); i++) {
    if (descriptor->options().GetExtension(proto::fhir_profile_base, i) ==
        url) {
      return true;
    }
  }
  return false;
}

bool IsTypeOrProfileOf(const std::string& url, const Descriptor* descriptor) {
  return IsType(url, descriptor) || IsProfileOf(url, descriptor);
}

bool IsType(const std::string& url, const Message& message) {
  return IsType(url, message.GetDescriptor());
}

bool IsProfileOf(const std::string& url, const Message& message) {
  return IsProfileOf(url, message.GetDescriptor());
}

bool IsTypeOrProfileOf(const std::string& url, const Message& message) {
  return IsTypeOrProfileOf(url, message.GetDescriptor());
}

}  // namespace

#define FHIR_TYPE_CHECK(type, url)                                       \
  bool Is##type(const Message& message) { return IsType(url, message); } \
                                                                         \
  bool IsProfileOf##type(const Message& message) {                       \
    return IsProfileOf(url, message);                                    \
  }                                                                      \
                                                                         \
  bool IsTypeOrProfileOf##type(const Message& message) {                 \
    return IsTypeOrProfileOf(url, message);                              \
  }                                                                      \
                                                                         \
  bool Is##type(const Descriptor* descriptor) {                          \
    return IsType(url, descriptor);                                      \
  }                                                                      \
                                                                         \
  bool IsProfileOf##type(const Descriptor* descriptor) {                 \
    return IsProfileOf(url, descriptor);                                 \
  }                                                                      \
                                                                         \
  bool IsTypeOrProfileOf##type(const Descriptor* descriptor) {           \
    return IsTypeOrProfileOf(url, descriptor);                           \
  }

FHIR_TYPE_CHECK(Bundle, "http://hl7.org/fhir/StructureDefinition/Bundle");
FHIR_TYPE_CHECK(Code, "http://hl7.org/fhir/StructureDefinition/code");
FHIR_TYPE_CHECK(Coding, "http://hl7.org/fhir/StructureDefinition/Coding");
FHIR_TYPE_CHECK(CodeableConcept,
                "http://hl7.org/fhir/StructureDefinition/CodeableConcept");

#undef FHIR_TYPE_CHECK

}  // namespace fhir
}  // namespace google
