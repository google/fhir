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

package com.google.fhir.common;

import com.google.common.collect.ImmutableSet;
import com.google.fhir.proto.Annotations;
import com.google.fhir.wrappers.DateTimeWrapper;
import com.google.fhir.wrappers.PrimitiveWrappers;
import com.google.protobuf.Any;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import com.google.protobuf.Message;
import com.google.protobuf.MessageOrBuilder;

/** Validator for FHIR resources. */
public final class ResourceValidator {

  public ResourceValidator() {}

  public void validateResource(MessageOrBuilder message) throws InvalidFhirException {
    validateFhirConstraints(message);
  }

  public void validateFhirConstraints(MessageOrBuilder message) throws InvalidFhirException {
    validateFhirConstraints(message, message.getDescriptorForType().getName());
  }

  private static void validateFhirConstraints(MessageOrBuilder message, String baseName)
      throws InvalidFhirException {
    if (AnnotationUtils.isPrimitiveType(message)) {
      try {
        PrimitiveWrappers.validatePrimitive(message);
      } catch (IllegalArgumentException e) {
        throw new InvalidFhirException("invalid-primitive-" + baseName, e);
      }
      return;
    }

    if (message instanceof Any) {
      // We do not validate "Any" contained resources.
      // TODO: maybe we should though... we'd need a registry that
      // allows us to automatically unpack into the correct type based on url.
      return;
    }

    Descriptor descriptor = message.getDescriptorForType();

    for (FieldDescriptor field : descriptor.getFields()) {
      checkField(message, field, baseName);
    }

    // Also verify that oneof fields are set.
    // Note that optional choice-types should have the containing message unset -
    // if the containing message is set, it should have a value set as well.
    for (OneofDescriptor oneof : descriptor.getOneofs()) {
      if (!message.hasOneof(oneof)
          && !oneof.getOptions().getExtension(Annotations.fhirOneofIsOptional)) {
        throw new InvalidFhirException("empty-oneof-" + oneof.getFullName());
      }
    }
  }

  private static void checkField(MessageOrBuilder message, FieldDescriptor field, String baseName)
      throws InvalidFhirException {
    String newBase = baseName + "." + field.getJsonName();
    if (field.getOptions().getExtension(Annotations.validationRequirement)
            == Annotations.Requirement.REQUIRED_BY_FHIR
        && !ProtoUtils.fieldIsSet(message, field)) {
      throw new InvalidFhirException("missing-" + newBase);
    }
    if (field
            .getMessageType()
            .getFullName()
            .equals(com.google.fhir.stu3.proto.Reference.getDescriptor().getFullName())
        || field
            .getMessageType()
            .getFullName()
            .equals(com.google.fhir.r4.core.Reference.getDescriptor().getFullName())) {
      validateReferenceField(message, field, baseName);
      return;
    }

    boolean isPeriodField =
        field
                .getMessageType()
                .getFullName()
                .equals(com.google.fhir.stu3.proto.Period.getDescriptor().getFullName())
            || field
                .getMessageType()
                .getFullName()
                .equals(com.google.fhir.r4.core.Period.getDescriptor().getFullName());
    for (int i = 0; i < ProtoUtils.fieldSize(message, field); i++) {
      MessageOrBuilder submessage = ProtoUtils.getAtIndex(message, field, i);
      validateFhirConstraints(submessage, newBase);

      // Run extra validation for some types, until FHIRPath validation covers
      // these cases as well.
      if (isPeriodField) {
        validatePeriod(submessage, newBase);
        return;
      }
    }
  }

  // If there is no typed reference id on a reference, one of these fields must be set.
  // TODO: remove this once FHIRPath can handle this.
  private static final ImmutableSet<String> OTHER_REFERENCE_FIELDS =
      ImmutableSet.of("extension", "identifier", "display");

  private static void validateReferenceField(
      MessageOrBuilder message, FieldDescriptor field, String baseName)
      throws InvalidFhirException {
    Descriptor descriptor = field.getMessageType();
    OneofDescriptor oneof = descriptor.getOneofs().get(0);

    for (int i = 0; i < ProtoUtils.fieldSize(message, field); i++) {
      MessageOrBuilder reference = ProtoUtils.getAtIndex(message, field, i);
      FieldDescriptor referenceField = reference.getOneofFieldDescriptor(oneof);
      if (referenceField == null) {
        // Note: getAllFields only returns those fields that are set :/
        for (FieldDescriptor setField : reference.getAllFields().keySet()) {
          if (OTHER_REFERENCE_FIELDS.contains(setField.getName())) {
            // There's no reference field, but there is other data.  That's valid.
            return;
          }
          throw new InvalidFhirException("empty-reference-" + baseName + "." + setField.getName());
        }
      }
      if (field.getOptions().getExtensionCount(Annotations.validReferenceType) == 0) {
        // The reference field does not have restrictions, so any value is fine.
        return;
      }
      if (!referenceField.getOptions().hasExtension(Annotations.referencedFhirType)) {
        // This is either a Uri, or a Fragment, which are untyped, and therefore valid.
        return;
      }
      String referenceType =
          referenceField.getOptions().getExtension(Annotations.referencedFhirType);
      boolean isAllowed = false;
      for (String validType : field.getOptions().getExtension(Annotations.validReferenceType)) {
        if (validType.equals(referenceType) || validType.equals("Resource")) {
          isAllowed = true;
        }
      }
      if (!isAllowed) {
        throw new InvalidFhirException(
            "invalid-reference-"
                + baseName
                + "."
                + field.getName()
                + "-disallowed-type-"
                + referenceType);
      }
    }
  }

  private static void validatePeriod(MessageOrBuilder period, String baseName)
      throws InvalidFhirException {
    Descriptor descriptor = period.getDescriptorForType();
    FieldDescriptor startField = descriptor.findFieldByName("start");
    FieldDescriptor endField = descriptor.findFieldByName("end");

    if (period.hasField(startField) && period.hasField(endField)) {
      DateTimeWrapper start = new DateTimeWrapper((Message) period.getField(startField));
      DateTimeWrapper end = new DateTimeWrapper((Message) period.getField(endField));

      if (start.getWrapped().getValueUs() < end.getWrapped().getValueUs()) {
        return;
      }
      // Start time is greater than end time, but that's not necessarily invalid, since the
      // precisions can be different.  So we need to compare the end time at the upper bound of end
      // element.
      // Example: If the start time is "Tuesday at noon", and the end time is "some time Tuesday",
      // this is valid, even thought the timestamp used for "some time Tuesday" is Tuesday 00:00,
      // since the precision for the start is higher than the end.
      //
      // Also note the GetUpperBoundFromTimelikeElement is always greater than
      // the time itself by exactly one time unit, and hence start needs to be strictly less than
      // end upper bound of end, so as to not allow ranges like [Tuesday, Monday] to be valid.
      if (start.getWrapped().getValueUs() >= end.getUpperBound()) {
        throw new InvalidFhirException(baseName + "-start-time-later-than-end-time");
      }
    }
  }
}
