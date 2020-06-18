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

#define FHIR_SIMPLE_TYPE_CHECK(type, url)                                \
  bool Is##type(const Message& message) { return IsType(url, message); } \
                                                                         \
  bool Is##type(const Descriptor* descriptor) {                          \
    return IsType(url, descriptor);                                      \
  }

#define FHIR_TYPE_CHECK(type, url)                             \
  FHIR_SIMPLE_TYPE_CHECK(type, url)                            \
  bool IsProfileOf##type(const Message& message) {             \
    return IsProfileOf(url, message);                          \
  }                                                            \
                                                               \
  bool IsTypeOrProfileOf##type(const Message& message) {       \
    return IsTypeOrProfileOf(url, message);                    \
  }                                                            \
                                                               \
  bool IsProfileOf##type(const Descriptor* descriptor) {       \
    return IsProfileOf(url, descriptor);                       \
  }                                                            \
                                                               \
  bool IsTypeOrProfileOf##type(const Descriptor* descriptor) { \
    return IsTypeOrProfileOf(url, descriptor);                 \
  }

FHIR_TYPE_CHECK(Bundle, "http://hl7.org/fhir/StructureDefinition/Bundle");
FHIR_TYPE_CHECK(Coding, "http://hl7.org/fhir/StructureDefinition/Coding");
FHIR_TYPE_CHECK(CodeableConcept,
                "http://hl7.org/fhir/StructureDefinition/CodeableConcept");
FHIR_TYPE_CHECK(Extension, "http://hl7.org/fhir/StructureDefinition/Extension");
FHIR_SIMPLE_TYPE_CHECK(Boolean,
                       "http://hl7.org/fhir/StructureDefinition/boolean");
FHIR_SIMPLE_TYPE_CHECK(String,
                       "http://hl7.org/fhir/StructureDefinition/string");
FHIR_SIMPLE_TYPE_CHECK(Integer,
                       "http://hl7.org/fhir/StructureDefinition/integer");
FHIR_SIMPLE_TYPE_CHECK(UnsignedInt,
                       "http://hl7.org/fhir/StructureDefinition/unsignedInt");
FHIR_SIMPLE_TYPE_CHECK(PositiveInt,
                       "http://hl7.org/fhir/StructureDefinition/positiveInt");
FHIR_SIMPLE_TYPE_CHECK(Decimal,
                       "http://hl7.org/fhir/StructureDefinition/decimal");
FHIR_SIMPLE_TYPE_CHECK(DateTime,
                       "http://hl7.org/fhir/StructureDefinition/dateTime");
FHIR_SIMPLE_TYPE_CHECK(Date, "http://hl7.org/fhir/StructureDefinition/date");
FHIR_SIMPLE_TYPE_CHECK(Time, "http://hl7.org/fhir/StructureDefinition/time");
FHIR_SIMPLE_TYPE_CHECK(Instant,
                       "http://hl7.org/fhir/StructureDefinition/instant");
FHIR_SIMPLE_TYPE_CHECK(Xhtml, "http://hl7.org/fhir/StructureDefinition/xhtml");
FHIR_SIMPLE_TYPE_CHECK(Base64Binary,
                       "http://hl7.org/fhir/StructureDefinition/base64Binary");
FHIR_SIMPLE_TYPE_CHECK(Uri, "http://hl7.org/fhir/StructureDefinition/uri");
FHIR_SIMPLE_TYPE_CHECK(Identifier,
                       "http://hl7.org/fhir/StructureDefinition/Identifier");

FHIR_SIMPLE_TYPE_CHECK(Quantity,
                       "http://hl7.org/fhir/StructureDefinition/Quantity");
FHIR_SIMPLE_TYPE_CHECK(
    SimpleQuantity, "http://hl7.org/fhir/StructureDefinition/SimpleQuantity");

#undef FHIR_SIMPLE_TYPE_CHECK
#undef FHIR_TYPE_CHECK

bool IsCode(const ::google::protobuf::Message& message) {
  return IsCode(message.GetDescriptor());
}

bool IsProfileOfCode(const ::google::protobuf::Message& message) {
  return IsProfileOfCode(message.GetDescriptor());
}

bool IsTypeOrProfileOfCode(const ::google::protobuf::Message& message) {
  return IsTypeOrProfileOfCode(message.GetDescriptor());
}

bool IsCode(const ::google::protobuf::Descriptor* descriptor) {
  return IsType("http://hl7.org/fhir/StructureDefinition/code", descriptor);
}

bool IsProfileOfCode(const ::google::protobuf::Descriptor* descriptor) {
  return IsProfileOf("http://hl7.org/fhir/StructureDefinition/code",
                     descriptor) ||
         descriptor->options().HasExtension(proto::fhir_valueset_url);
}

bool IsTypeOrProfileOfCode(const ::google::protobuf::Descriptor* descriptor) {
  return IsCode(descriptor) || IsProfileOfCode(descriptor);
}

}  // namespace fhir
}  // namespace google
