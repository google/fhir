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

#ifndef GOOGLE_FHIR_PRIMITIVE_HANDLER_H_
#define GOOGLE_FHIR_PRIMITIVE_HANDLER_H_

#include <memory>
#include <string>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "absl/time/time.h"
#include "absl/types/optional.h"
#include "google/fhir/primitive_wrapper.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "include/json/json.h"
#include "tensorflow/core/lib/core/errors.h"

namespace google {
namespace fhir {

struct JsonPrimitive {
  std::string value;
  std::unique_ptr<::google::protobuf::Message> element;

  const bool is_non_null() const { return value != "null"; }
};

// Class that abstracts direct interaction with primitives.  By delegating
// primitive handling to an instance of this class, core libraries can be
// written without depending on any specific version of FHIR. This allows for
// creating primitives of a given FHIR type (e.g, Decimal), extracting the value
// from a known type, validating primitives, and Parsing and Wrapping functions
// for use by json_format.h
class PrimitiveHandler {
 public:
  virtual ~PrimitiveHandler() {}

  Status ParseInto(const Json::Value& json, const absl::TimeZone tz,
                   ::google::protobuf::Message* target) const;

  Status ParseInto(const Json::Value& json, ::google::protobuf::Message* target) const;

  StatusOr<JsonPrimitive> WrapPrimitiveProto(
      const ::google::protobuf::Message& proto) const;

  Status ValidatePrimitive(const ::google::protobuf::Message& primitive) const;

  // Validates that a reference field conforms to spec.
  // The types of resources that can be referenced is controlled via annotations
  // on the proto, e.g., R4 Patient.general_practitioner must be a reference to
  // an Organization, Practitioner, or PractitionerRole, or it must be an
  // untyped reference, like uri.
  virtual Status ValidateReferenceField(
      const ::google::protobuf::Message& parent,
      const ::google::protobuf::FieldDescriptor* field) const = 0;

  virtual ::google::protobuf::Message* NewContainedResource() const = 0;

  virtual const ::google::protobuf::Descriptor* ContainedResourceDescriptor() const = 0;

  virtual StatusOr<std::string> GetStringValue(
      const ::google::protobuf::Message& primitive) const = 0;

  virtual ::google::protobuf::Message* NewString(const std::string& str) const = 0;

  virtual const ::google::protobuf::Descriptor* StringDescriptor() const = 0;

  virtual StatusOr<bool> GetBooleanValue(
      const ::google::protobuf::Message& primitive) const = 0;

  virtual ::google::protobuf::Message* NewBoolean(const bool value) const = 0;

  virtual const ::google::protobuf::Descriptor* BooleanDescriptor() const = 0;

  virtual StatusOr<int> GetIntegerValue(
      const ::google::protobuf::Message& primitive) const = 0;

  virtual ::google::protobuf::Message* NewInteger(const int value) const = 0;

  virtual const ::google::protobuf::Descriptor* IntegerDescriptor() const = 0;

  virtual StatusOr<std::string> GetDecimalValue(
      const ::google::protobuf::Message& primitive) const = 0;

  virtual ::google::protobuf::Message* NewDecimal(const std::string value) const = 0;

  virtual const ::google::protobuf::Descriptor* DecimalDescriptor() const = 0;

 protected:
  explicit PrimitiveHandler(proto::FhirVersion version) : version_(version) {}

  virtual StatusOr<std::unique_ptr<primitives_internal::PrimitiveWrapper>>
  GetWrapper(const ::google::protobuf::Descriptor* target_descriptor) const = 0;

  Status CheckVersion(const ::google::protobuf::Message& message) const;
  Status CheckVersion(const ::google::protobuf::Descriptor* descriptor) const;

  const proto::FhirVersion version_;
};

namespace primitives_internal {

using ::google::protobuf::OneofDescriptor;

template <typename Expected>
ABSL_MUST_USE_RESULT Status CheckType(const ::google::protobuf::Descriptor* descriptor) {
  return IsMessageType<Expected>(descriptor)
             ? Status::OK()
             : InvalidArgument("Expected ", Expected::descriptor()->full_name(),
                               ", but message was of type ",
                               descriptor->full_name());
}

template <typename Expected>
ABSL_MUST_USE_RESULT Status CheckType(const ::google::protobuf::Message& message) {
  return CheckType<Expected>(message.GetDescriptor());
}

template <class TypedReference,
          class TypedReferenceId = REFERENCE_ID_TYPE(TypedReference)>
Status ValidateReferenceField(const Message& parent,
                              const FieldDescriptor* field) {
  static const Descriptor* descriptor = TypedReference::descriptor();
  static const OneofDescriptor* oneof = descriptor->oneof_decl(0);

  for (int i = 0; i < PotentiallyRepeatedFieldSize(parent, field); i++) {
    const TypedReference& reference =
        GetPotentiallyRepeatedMessage<TypedReference>(parent, field, i);
    const Reflection* reflection = reference.GetReflection();
    const FieldDescriptor* reference_field =
        reflection->GetOneofFieldDescriptor(reference, oneof);

    if (!reference_field) {
      if (reference.extension_size() == 0 && !reference.has_identifier() &&
          !reference.has_display()) {
        return FailedPrecondition("empty-reference");
      }
      // There's no reference field, but there is other data.  That's valid.
      return Status::OK();
    }
    if (field->options().ExtensionSize(proto::valid_reference_type) == 0) {
      // The reference field does not have restrictions, so any value is fine.
      return Status::OK();
    }
    if (reference.has_uri() || reference.has_fragment()) {
      // Uri and Fragment references are untyped.
      return Status::OK();
    }

    if (IsMessageType<TypedReferenceId>(reference_field->message_type())) {
      const std::string& reference_type =
          reference_field->options().GetExtension(
              ::google::fhir::proto::referenced_fhir_type);
      bool is_allowed = false;
      for (int i = 0;
           i < field->options().ExtensionSize(proto::valid_reference_type);
           i++) {
        const std::string& valid_type =
            field->options().GetExtension(proto::valid_reference_type, i);
        if (valid_type == reference_type || valid_type == "Resource") {
          is_allowed = true;
          break;
        }
      }
      if (!is_allowed) {
        return FailedPrecondition("invalid-reference-disallowed-type-",
                                  reference_type);
      }
    }
  }

  return Status::OK();
}

// Template for a PrimitiveHandler tied to a single version of FHIR.
// This provides much of the functionality that is common between versions.
// The templating is kept separate from the main interface, in order to provide
// a version-agnostic type that libraries can use without needing to template
// themselves.
template <typename BundleType,
          typename ContainedResourceType =
              BUNDLE_CONTAINED_RESOURCE(BundleType),
          typename ExtensionType = EXTENSION_TYPE(BundleType),
          typename StringType = FHIR_DATATYPE(BundleType, string_value),
          typename IntegerType = FHIR_DATATYPE(BundleType, integer),
          typename DecimalType = FHIR_DATATYPE(BundleType, decimal),
          typename BooleanType = FHIR_DATATYPE(BundleType, boolean),
          typename ReferenceType = FHIR_DATATYPE(BundleType, reference)>
class PrimitiveHandlerTemplate : public PrimitiveHandler {
 public:
  typedef BundleType Bundle;
  typedef ContainedResourceType ContainedResource;
  typedef ExtensionType Extension;
  typedef StringType String;
  typedef BooleanType Boolean;
  typedef IntegerType Integer;
  typedef DecimalType Decimal;
  typedef ReferenceType Reference;

  Status ValidateReferenceField(const Message& parent,
                                const FieldDescriptor* field) const override {
    FHIR_RETURN_IF_ERROR(CheckType<Reference>(field->message_type()));
    return primitives_internal::ValidateReferenceField<Reference>(parent,
                                                                  field);
  }

  ::google::protobuf::Message* NewContainedResource() const override {
    return new ContainedResource();
  }

  const ::google::protobuf::Descriptor* ContainedResourceDescriptor() const override {
    return ContainedResource::GetDescriptor();
  }

  StatusOr<std::string> GetStringValue(
      const ::google::protobuf::Message& primitive) const override {
    FHIR_RETURN_IF_ERROR(CheckType<String>(primitive));
    return dynamic_cast<const String&>(primitive).value();
  }

  ::google::protobuf::Message* NewString(const std::string& str) const override {
    String* msg = new String();
    msg->set_value(str);
    return msg;
  }

  const ::google::protobuf::Descriptor* StringDescriptor() const override {
    return String::GetDescriptor();
  }

  StatusOr<bool> GetBooleanValue(
      const ::google::protobuf::Message& primitive) const override {
    FHIR_RETURN_IF_ERROR(CheckType<Boolean>(primitive));
    return dynamic_cast<const Boolean&>(primitive).value();
  }

  ::google::protobuf::Message* NewBoolean(const bool value) const override {
    Boolean* msg = new Boolean();
    msg->set_value(value);
    return msg;
  }

  const ::google::protobuf::Descriptor* BooleanDescriptor() const override {
    return Boolean::GetDescriptor();
  }

  StatusOr<int> GetIntegerValue(
      const ::google::protobuf::Message& primitive) const override {
    FHIR_RETURN_IF_ERROR(CheckType<Integer>(primitive));
    return dynamic_cast<const Integer&>(primitive).value();
  }

  ::google::protobuf::Message* NewInteger(const int value) const override {
    Integer* msg = new Integer();
    msg->set_value(value);
    return msg;
  }

  const ::google::protobuf::Descriptor* IntegerDescriptor() const override {
    return Integer::GetDescriptor();
  }

  StatusOr<std::string> GetDecimalValue(
      const ::google::protobuf::Message& primitive) const override {
    FHIR_RETURN_IF_ERROR(CheckType<Decimal>(primitive));
    return dynamic_cast<const Decimal&>(primitive).value();
  }

  ::google::protobuf::Message* NewDecimal(const std::string value) const override {
    Decimal* msg = new Decimal();
    msg->set_value(value);
    return msg;
  }

  const ::google::protobuf::Descriptor* DecimalDescriptor() const override {
    return Decimal::GetDescriptor();
  }

 protected:
  PrimitiveHandlerTemplate()
      : PrimitiveHandler(GetFhirVersion(ExtensionType::descriptor())) {}
};

// Helper function for handling primitive types that are universally present in
// all FHIR versions >= STU3.
template <typename ExtensionType, typename XhtmlType,
          typename Base64BinarySeparatorStrideType>
absl::optional<std::unique_ptr<PrimitiveWrapper>> GetWrapperForStu3Types(
    const Descriptor* target_descriptor) {
  if (IsTypeOrProfileOfCode(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new CodeWrapper<FHIR_DATATYPE(ExtensionType, code)>());
  }
  if (IsMessageType<FHIR_DATATYPE(ExtensionType, code)>(target_descriptor) ||
      HasValueset(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        (new CodeWrapper<FHIR_DATATYPE(ExtensionType, code)>()));
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, base64_binary)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new Base64BinaryWrapper<FHIR_DATATYPE(ExtensionType, base64_binary),
                                Base64BinarySeparatorStrideType>());
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, boolean)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new BooleanWrapper<FHIR_DATATYPE(ExtensionType, boolean)>());
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, date)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeTypeWrapper<FHIR_DATATYPE(ExtensionType, date)>());
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, date_time)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeTypeWrapper<FHIR_DATATYPE(ExtensionType, date_time)>());
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, decimal)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new DecimalWrapper<FHIR_DATATYPE(ExtensionType, decimal)>());
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, id)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<FHIR_DATATYPE(ExtensionType, id)>());
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, instant)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeTypeWrapper<FHIR_DATATYPE(ExtensionType, instant)>());
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, integer)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new IntegerTypeWrapper<FHIR_DATATYPE(ExtensionType, integer)>());
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, markdown)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<FHIR_DATATYPE(ExtensionType, markdown)>());
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, oid)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<FHIR_DATATYPE(ExtensionType, oid)>());
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, positive_int)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new PositiveIntWrapper<FHIR_DATATYPE(ExtensionType, positive_int)>());
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, string_value)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<FHIR_DATATYPE(ExtensionType, string_value)>());
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, time)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new TimeWrapper<FHIR_DATATYPE(ExtensionType, time)>());
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, unsigned_int)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new UnsignedIntWrapper<FHIR_DATATYPE(ExtensionType, unsigned_int)>());
  } else if (IsMessageType<FHIR_DATATYPE(ExtensionType, uri)>(
                 target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<FHIR_DATATYPE(ExtensionType, uri)>());
  } else if (IsMessageType<XhtmlType>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(new XhtmlWrapper<XhtmlType>());
  }
  return absl::optional<std::unique_ptr<PrimitiveWrapper>>();
}

// Helper function for handling primitive types that are universally present in
// all FHIR versions >= R4.
template <typename ExtensionType, typename XhtmlType,
          typename Base64BinarySeparatorStrideType>
absl::optional<std::unique_ptr<PrimitiveWrapper>> GetWrapperForR4Types(
    const Descriptor* target_descriptor) {
  absl::optional<std::unique_ptr<PrimitiveWrapper>> wrapper_for_stu3_types =
      primitives_internal::GetWrapperForStu3Types<
          ExtensionType, XhtmlType, Base64BinarySeparatorStrideType>(
          target_descriptor);

  if (wrapper_for_stu3_types.has_value()) {
    return std::move(wrapper_for_stu3_types.value());
  }

  if (IsMessageType<FHIR_DATATYPE(ExtensionType, canonical)>(
          target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<FHIR_DATATYPE(ExtensionType, canonical)>());
  }

  if (IsMessageType<FHIR_DATATYPE(ExtensionType, url)>(target_descriptor)) {
    return std::unique_ptr<PrimitiveWrapper>(
        new StringTypeWrapper<FHIR_DATATYPE(ExtensionType, url)>());
  }
  return absl::optional<std::unique_ptr<PrimitiveWrapper>>();
}

}  // namespace primitives_internal

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_PRIMITIVE_HANDLER_H_
