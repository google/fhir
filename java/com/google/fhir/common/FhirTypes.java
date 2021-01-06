//    Copyright 2021 Google Inc.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        https://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

package com.google.fhir.common;

import static com.google.fhir.common.AnnotationUtils.getStructureDefinitionUrl;

import com.google.fhir.proto.Annotations;
import com.google.protobuf.Descriptors.Descriptor;

/** Utilities for determining FHIR type in a version-independent way. */
public final class FhirTypes {

  private FhirTypes() {}

  private static final String CODE_URL = "http://hl7.org/fhir/StructureDefinition/code";
  private static final String CODEABLE_CONCEPT_URL =
      "http://hl7.org/fhir/StructureDefinition/CodeableConcept";
  private static final String CODING_URL = "http://hl7.org/fhir/StructureDefinition/Coding";
  private static final String EXTENSION_URL = "http://hl7.org/fhir/StructureDefinition/Extension";
  private static final String PERIOD_URL = "http://hl7.org/fhir/StructureDefinition/Period";
  private static final String REFERENCE_URL = "http://hl7.org/fhir/StructureDefinition/Reference";

  public static boolean isProfileOfType(Descriptor base, Descriptor test) {
    if (isCode(base)) {
      return isProfileOfCode(test);
    }
    return isProfileOf(AnnotationUtils.getStructureDefinitionUrl(base), test);
  }

  // Extension
  public static boolean isExtension(Descriptor type) {
    return isType(EXTENSION_URL, type);
  }

  public static boolean isProfileOfExtension(Descriptor type) {
    return isProfileOf(EXTENSION_URL, type);
  }

  public static boolean isTypeOrProfileOfExtension(Descriptor type) {
    return isTypeOrProfileOf(EXTENSION_URL, type);
  }

  // Period
  public static boolean isPeriod(Descriptor type) {
    return isType(PERIOD_URL, type);
  }

  public static boolean isProfileOfPeriod(Descriptor type) {
    return isProfileOf(PERIOD_URL, type);
  }

  public static boolean isTypeOrProfileOfPeriod(Descriptor type) {
    return isTypeOrProfileOf(PERIOD_URL, type);
  }

  // Reference
  public static boolean isReference(Descriptor type) {
    return isType(REFERENCE_URL, type);
  }

  public static boolean isProfileOfReference(Descriptor type) {
    return isProfileOf(REFERENCE_URL, type);
  }

  public static boolean isTypeOrProfileOfReference(Descriptor type) {
    return isTypeOrProfileOf(REFERENCE_URL, type);
  }

  // Code
  public static boolean isCode(Descriptor type) {
    return isType(CODE_URL, type);
  }

  public static boolean isProfileOfCode(Descriptor type) {
    if (!AnnotationUtils.getFhirValuesetUrl(type).isEmpty()) {
      // TODO: This is an STU3-type code
      return true;
    }
    return isProfileOf(CODE_URL, type);
  }

  public static boolean isTypeOrProfileOfCode(Descriptor type) {
    return isCode(type) || isProfileOfCode(type);
  }

  // Coding
  public static boolean isCoding(Descriptor type) {
    return isType(CODING_URL, type);
  }

  public static boolean isProfileOfCoding(Descriptor type) {
    return isProfileOf(CODING_URL, type);
  }

  public static boolean isTypeOrProfileOfCoding(Descriptor type) {
    return isCoding(type) || isProfileOfCoding(type);
  }

  // CodeableConcept
  public static boolean isCodeableConcept(Descriptor type) {
    return isType(CODEABLE_CONCEPT_URL, type);
  }

  public static boolean isProfileOfCodeableConcept(Descriptor type) {
    return isProfileOf(CODEABLE_CONCEPT_URL, type);
  }

  public static boolean isTypeOrProfileOfCodeableConcept(Descriptor type) {
    return isCodeableConcept(type) || isProfileOfCodeableConcept(type);
  }

  // Implementation
  private static boolean isTypeOrProfileOf(String url, Descriptor type) {
    return isType(url, type) || isProfileOf(url, type);
  }

  private static boolean isType(String url, Descriptor type) {
    return url.equals(getStructureDefinitionUrl(type));
  }

  private static boolean isProfileOf(String url, Descriptor type) {
    for (int i = 0; i < type.getOptions().getExtensionCount(Annotations.fhirProfileBase); i++) {
      if (type.getOptions().getExtension(Annotations.fhirProfileBase, i).equals(url)) {
        // This is an R4 or later type code.
        return true;
      }
    }
    return false;
  }
}
