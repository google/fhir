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

#ifndef GOOGLE_FHIR_STU3_PROFILES_LIB_H_
#define GOOGLE_FHIR_STU3_PROFILES_LIB_H_

#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/annotations.h"
#include "google/fhir/codeable_concepts.h"
#include "google/fhir/codes.h"
#include "google/fhir/extensions.h"
#include "google/fhir/proto_util.h"
#include "google/fhir/resource_validation.h"
#include "google/fhir/status/status.h"
#include "google/fhir/status/statusor.h"
#include "google/fhir/util.h"
#include "proto/annotations.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status.h"

namespace google {
namespace fhir {

namespace profiles_internal {

using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using std::unordered_map;
using ::tensorflow::errors::InvalidArgument;

const bool SharesCommonAncestor(const ::google::protobuf::Descriptor* first,
                                const ::google::protobuf::Descriptor* second);

// Gets a map from profiled extension urls to the fields that they are profiled
// in on a target message.
const unordered_map<std::string, const FieldDescriptor*>& GetExtensionMap(
    const Descriptor* descriptor);

// Copies the contents of the extension field on source to the target message.
// Any extensions that can be slotted into profiled fields are, and any that
// cannot are put into the extension field on the target.
template <typename ExtensionLike,
          typename CodeLike = FHIR_DATATYPE(ExtensionLike, code)>
Status PerformExtensionSlicing(const Message& source, Message* target) {
  const Descriptor* source_descriptor = source.GetDescriptor();
  const Reflection* source_reflection = source.GetReflection();
  const Descriptor* target_descriptor = target->GetDescriptor();
  const Reflection* target_reflection = target->GetReflection();

  const FieldDescriptor* source_extension_field =
      source_descriptor->FindFieldByName("extension");
  if (!source_extension_field) {
    // Nothing to slice
    return Status::OK();
  }

  // Map from profiled extension urls to the fields that they are profiled in
  // on the target
  std::unordered_map<std::string, const FieldDescriptor*> extension_map =
      GetExtensionMap(target_descriptor);

  for (const ExtensionLike& source_extension :
       source_reflection->GetRepeatedFieldRef<ExtensionLike>(
           source, source_extension_field)) {
    const std::string& url = source_extension.url().value();
    const auto extension_entry_iter = extension_map.find(url);
    if (extension_entry_iter != extension_map.end()) {
      // This extension can be sliced into an inlined field.
      const FieldDescriptor* inlined_field = extension_entry_iter->second;
      if (source_extension.extension_size() == 0) {
        // This is a simple extension
        // Note that we cannot use ExtensionToMessage from extensions.h
        // here, because there is no top-level extension message we're merging
        // into, it's just a single primitive inlined field.
        const Descriptor* destination_type = inlined_field->message_type();
        const auto& value = source_extension.value();
        const FieldDescriptor* src_datatype_field =
            value.GetReflection()->GetOneofFieldDescriptor(
                value, value.GetDescriptor()->FindOneofByName("value"));
        if (src_datatype_field == nullptr) {
          return InvalidArgument(
              absl::StrCat("Invalid extension: neither value nor extensions "
                           "set on extension ",
                           url));
        }
        Message* typed_extension = MutableOrAddMessage(target, inlined_field);
        const Message& src_value =
            value.GetReflection()->GetMessage(value, src_datatype_field);
        if (destination_type == src_datatype_field->message_type()) {
          typed_extension->CopyFrom(src_value);
        } else if (src_datatype_field->message_type()->full_name() ==
                   CodeLike::descriptor()->full_name()) {
          TF_RETURN_IF_ERROR(ConvertToTypedCode(
              dynamic_cast<const CodeLike&>(src_value), typed_extension));
        } else {
          return InvalidArgument(
              "Profiled extension slice is incorrect type: ", url, "should be ",
              destination_type->full_name(), " but is ",
              src_datatype_field->message_type()->full_name());
        }
      } else {
        // This is a complex extension
        Message* typed_extension = MutableOrAddMessage(target, inlined_field);
        TF_RETURN_IF_ERROR(
            ExtensionToMessage(source_extension, typed_extension));
      }
    } else {
      // There is no inlined field for this extension, just copy it over.
      const FieldDescriptor* target_extension_field =
          target_descriptor->FindFieldByName("extension");
      if (!target_extension_field) {
        // Target doesn't have a raw extension field, and doesn't have a typed
        // extension field that can bandle this.
        return InvalidArgument(
            "Cannot Slice extensions from ", source_descriptor->full_name(),
            " to ", target_descriptor->full_name(),
            ": target does not have an extension field that can handle url: ",
            url);
      }
      target_reflection->AddMessage(target, target_extension_field)
          ->CopyFrom(source_extension);
    }
  }
  return Status::OK();
}

template <typename ExtensionLike>
Status UnsliceExtension(const Message& typed_extension,
                        const FieldDescriptor* source_field,
                        ExtensionLike* target) {
  if (IsProfileOf<ExtensionLike>(typed_extension)) {
    // This a profile on extension, and therefore a complex extension
    return ConvertToExtension(typed_extension, target);
  } else {
    // This just a raw datatype, and therefore a simple extension
    target->mutable_url()->set_value(GetInlinedExtensionUrl(source_field));

    return SetDatatypeOnExtension(typed_extension, target);
  }
}

template <typename ExtensionLike, typename CodeableConceptLike = FHIR_DATATYPE(
                                      ExtensionLike, codeable_concept)>
bool CanHaveSlicing(const FieldDescriptor* field) {
  if (IsChoiceType(field)) {
    return false;
  }
  // There are three kinds of subfields that could potentially have slices:
  // 1) Types that are themselves profiles
  // 2) "Backbone" i.e. nested types defined on this message
  // 3) Contained resources of profiled bundles.  These are basically "profiles"
  //    of the base contained resources, but are not actually fhir elements.
  const Descriptor* field_type = field->message_type();
  // TODO: Use an annotation for this.
  if (field->message_type()->name() == "ContainedResource" &&
      (field->message_type()->full_name() !=
           "google.fhir.stu3.proto.ContainedResource" &&
       field->message_type()->full_name() !=
           "google.fhir.r4.core.ContainedResource" &&
       field->message_type()->full_name() !=
           "google.fhir.r4.proto.ContainedResource")) {
    return true;
  }
  if (IsProfile(field_type)) {
    if (IsProfileOf<CodeableConceptLike>(field_type) ||
        IsProfileOf<ExtensionLike>(field_type)) {
      // Profiles on Extensions and CodeableConcepts are the slices themselves,
      // rather than elements that *have* slices.
      return false;
    }
    return true;
  }

  // The type is a nested message defined on this type if its full name starts
  // with the full name of the containing type.
  return field_type->full_name().rfind(
             absl::StrCat(field->containing_type()->full_name(), "."), 0) == 0;
}

template <typename ExtensionLike,
          typename CodeableConceptLike = FHIR_DATATYPE(ExtensionLike,
                                                       codeable_concept),
          typename CodeLike = FHIR_DATATYPE(ExtensionLike, code)>
Status CopyToProfile(const Message& source, Message* target) {
  target->Clear();
  // Handle all the raw extensions on source.  This slot extensions that have
  // profiled fields, and copy the rest over to the target raw extensions field.
  // Noe that this will only handle raw extensions on source - typed extensions
  // on source will be handled when iterating through the fields.
  FHIR_RETURN_IF_ERROR(PerformExtensionSlicing<ExtensionLike>(source, target));

  const Descriptor* source_descriptor = source.GetDescriptor();
  const Reflection* source_reflection = source.GetReflection();
  const Descriptor* target_descriptor = target->GetDescriptor();

  // Keep a reference to the target extension field - even though we already
  // handled all the raw extensions on the source, we could still hit typed
  // extensions on the source that don't have corresponding fields in the target
  const FieldDescriptor* target_extension_field =
      target_descriptor->FindFieldByName("extension");

  for (int i = 0; i < source_descriptor->field_count(); i++) {
    const FieldDescriptor* source_field = source_descriptor->field(i);
    if (!FieldHasValue(source, source_field)) continue;

    // Skip over extensions field because it was handled by
    // PerformExtensionSlicing
    if (source_field->name() == "extension") continue;

    const FieldDescriptor* target_field =
        target_descriptor->FindFieldByName(source_field->name());
    if (!target_field) {
      // Since CodeableConcepts are handled via a different code path,
      // the only time that a field on source might not exist on target is if
      // the source is a typed extension.  In this case, it should be converted
      // to a raw extension.
      if (!HasInlinedExtensionUrl(source_field) || !target_extension_field) {
        return InvalidArgument("Unable to Profile ",
                               source_descriptor->full_name(), " to ",
                               target_descriptor->full_name(), ": no field ",
                               source_field->name(), " on target.");
      }
      if (!IsMessageType<ExtensionLike>(
              target_extension_field->message_type())) {
        return InvalidArgument(
            "Unexpected type on extension field for ",
            target_descriptor->full_name(),
            ".  Expected: ", ExtensionLike::descriptor()->full_name(),
            " but found: ",
            target_extension_field->message_type()->full_name());
      }

      FHIR_RETURN_IF_ERROR(ForEachMessageWithStatus<Message>(
          source, source_field,
          [&source_field, &target,
           &target_extension_field](const Message& source_message) {
            return UnsliceExtension(
                source_message, source_field,
                dynamic_cast<ExtensionLike*>(
                    MutableOrAddMessage(target, target_extension_field)));
          }));
      continue;
    }

    // Make sure the source data fits into the size restriction of the target
    // field.
    if (source_field->is_repeated() && !target_field->is_repeated() &&
        source_reflection->FieldSize(source, source_field) > 1) {
      return InvalidArgument(
          "Unable to Profile ", source_descriptor->full_name(), " to ",
          target_descriptor->full_name(), ": For field ", source_field->name(),
          ", source has multiple entries but target field is not repeated.");
    }

    const Descriptor* source_field_type = source_field->message_type();

    if (!source_field_type) {
      return InvalidArgument("Encountered unexpected primitive type on field: ",
                             source_field->full_name());
    }
    if (IsTypeOrProfileOf<CodeLike>(target_field->message_type())) {
      FHIR_RETURN_IF_ERROR(ForEachMessageWithStatus<Message>(
          source, source_field,
          [&target, &target_field](const Message& source_message) {
            return CopyCode(source_message,
                            MutableOrAddMessage(target, target_field));
          }));
      continue;
    }

    // TODO:  Handle type-or-profile-of CodingLike

    if (IsTypeOrProfileOf<CodeableConceptLike>(target_field->message_type())) {
      FHIR_RETURN_IF_ERROR(ForEachMessageWithStatus<Message>(
          source, source_field,
          [&target, &target_field](const Message& source_message) {
            return CopyCodeableConcept(
                source_message, MutableOrAddMessage(target, target_field));
          }));
      continue;
    }

    // If the target field can have slicing, we perform a CopyToProfile
    // even if the source field and target field are the same type.
    // This is so that we can guarantee that the output will use profiled fields
    // wherever possible, even if the input proto did not.
    // E.g., if the source message has a raw extension with a url that should
    // have been in a profiled field.
    if (source_field_type->full_name() !=
            target_field->message_type()->full_name() ||
        CanHaveSlicing<ExtensionLike>(target_field)) {
      FHIR_RETURN_IF_ERROR(ForEachMessageWithStatus<Message>(
          source, source_field,
          [&target, &target_field](const Message& source_message) {
            return CopyToProfile<ExtensionLike>(
                source_message, MutableOrAddMessage(target, target_field));
          }));
      continue;
    }

    // The target field is not of any known profilable type.  This means the
    // types must be equal, and we can safely copy over.
    if (source_field_type->full_name() ==
        target_field->message_type()->full_name()) {
      ForEachMessage<Message>(
          source, source_field,
          [&target, &target_field](const Message& source_message) {
            MutableOrAddMessage(target, target_field)->CopyFrom(source_message);
          });
      continue;
    }
    return InvalidArgument("Unable to Profile ", source_descriptor->full_name(),
                           " to ", target_descriptor->full_name(), ": Types ",
                           source_field_type->full_name(), " and ",
                           target_field->message_type()->full_name(),
                           " are incompatible.");
  }
  return Status::OK();
}

template <typename ExtensionLike>
Status ConvertToProfileLenientInternal(const Message& source, Message* target) {
  if (!SharesCommonAncestor(source.GetDescriptor(), target->GetDescriptor())) {
    return tensorflow::errors::InvalidArgument(
        "Incompatible profile types: ", source.GetDescriptor()->full_name(),
        " to ", target->GetDescriptor()->full_name());
  }
  return CopyToProfile<ExtensionLike>(source, target);
}

template <typename ExtensionLike>
Status ConvertToProfileInternal(const Message& source, Message* target) {
  const auto& status =
      ConvertToProfileLenientInternal<ExtensionLike>(source, target);
  FHIR_RETURN_IF_ERROR(status);
  Status validation = ValidateResource(*target);
  if (validation.ok()) {
    return Status::OK();
  }
  return tensorflow::errors::FailedPrecondition(validation.error_message());
}

}  // namespace profiles_internal

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_PROFILES_LIB_H_
