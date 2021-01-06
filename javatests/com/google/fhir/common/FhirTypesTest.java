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

import static com.google.common.truth.Truth.assertThat;

import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableList;
import com.google.protobuf.Descriptors.Descriptor;
import java.util.Optional;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

@RunWith(Parameterized.class)
@SuppressWarnings("UnnecessarilyFullyQualified")
public final class FhirTypesTest {
  private final Descriptor rawPatient;
  private final Descriptor profiledPatient;
  private final Descriptor rawExtensionType;
  private final Descriptor typedExtensionType;
  private final Descriptor enumTypedCode;
  private final Descriptor stringTypedCode;

  // TODO: There are no profiled Codings in STU3.
  private final Optional<Descriptor> profiledCoding;

  private final Descriptor profiledCodeableConcept;

  public FhirTypesTest(
      Descriptor rawPatient,
      Descriptor profiledPatient,
      Descriptor rawExtensionType,
      Descriptor typedExtensionType,
      Descriptor enumTypedCode,
      Descriptor stringTypedCode,
      Optional<Descriptor> profiledCoding,
      Descriptor profiledCodeableConcept) {
    this.rawPatient = rawPatient;
    this.profiledPatient = profiledPatient;
    this.rawExtensionType = rawExtensionType;
    this.typedExtensionType = typedExtensionType;
    this.enumTypedCode = enumTypedCode;
    this.stringTypedCode = stringTypedCode;
    this.profiledCoding = profiledCoding;
    this.profiledCodeableConcept = profiledCodeableConcept;
  }

  @Parameterized.Parameters
  public static ImmutableCollection<Object[]> params() {
    return ImmutableList.of(
        new Object[] {
          com.google.fhir.r4.core.Patient.getDescriptor(),
          com.google.fhir.r4.uscore.USCorePatientProfile.getDescriptor(),
          com.google.fhir.r4.core.Extension.getDescriptor(),
          com.google.fhir.r4.core.AddressGeolocation.getDescriptor(),
          com.google.fhir.r4.core.Patient.GenderCode.getDescriptor(),
          com.google.fhir.r4.core.Binary.ContentTypeCode.getDescriptor(),
          Optional.of(
              com.google.fhir.r4.uscore.PatientUSCoreRaceExtension.OmbCategoryCoding
                  .getDescriptor()),
          com.google.fhir.r4.testing.TestObservation.CodeableConceptForCategory.getDescriptor(),
        },
        new Object[] {
          com.google.fhir.stu3.proto.Patient.getDescriptor(),
          com.google.fhir.stu3.uscore.UsCorePatient.getDescriptor(),
          com.google.fhir.stu3.proto.Extension.getDescriptor(),
          com.google.fhir.stu3.proto.AddressGeolocation.getDescriptor(),
          com.google.fhir.stu3.proto.AdministrativeGenderCode.getDescriptor(),
          com.google.fhir.stu3.proto.MimeTypeCode.getDescriptor(),
          Optional.empty(), // TODO: There are no profiled Codings in STU3.
          com.google.fhir.stu3.testing.TestObservation.CodeableConceptForCategory.getDescriptor(),
        });
  }

  private Descriptor getRawType(String fieldName) {
    return rawExtensionType
        .findFieldByName("value")
        .getMessageType()
        .findFieldByName(fieldName)
        .getMessageType();
  }

  @Test
  public void testIsExtension() {
    assertThat(FhirTypes.isExtension(rawExtensionType)).isTrue();
    assertThat(FhirTypes.isExtension(typedExtensionType)).isFalse();
    assertThat(FhirTypes.isExtension(getRawType("code"))).isFalse();
    assertThat(FhirTypes.isExtension(profiledPatient)).isFalse();
  }

  @Test
  public void testIsProfileOfExtension() {
    assertThat(FhirTypes.isProfileOfExtension(rawExtensionType)).isFalse();
    assertThat(FhirTypes.isProfileOfExtension(typedExtensionType)).isTrue();
    assertThat(FhirTypes.isProfileOfExtension(getRawType("code"))).isFalse();
    assertThat(FhirTypes.isProfileOfExtension(profiledPatient)).isFalse();
  }

  @Test
  public void testIsTypeOrProfileOfExtension() {
    assertThat(FhirTypes.isTypeOrProfileOfExtension(rawExtensionType)).isTrue();
    assertThat(FhirTypes.isTypeOrProfileOfExtension(typedExtensionType)).isTrue();
    assertThat(FhirTypes.isTypeOrProfileOfExtension(getRawType("code"))).isFalse();
    assertThat(FhirTypes.isTypeOrProfileOfExtension(profiledPatient)).isFalse();
  }

  @Test
  public void testIsCode() {
    assertThat(FhirTypes.isCode(getRawType("code"))).isTrue();
    assertThat(FhirTypes.isCode(enumTypedCode)).isFalse();
    assertThat(FhirTypes.isCode(stringTypedCode)).isFalse();
    assertThat(FhirTypes.isCode(profiledPatient)).isFalse();
  }

  @Test
  public void testIsProfileOfCode() {
    assertThat(FhirTypes.isProfileOfCode(getRawType("code"))).isFalse();
    assertThat(FhirTypes.isProfileOfCode(enumTypedCode)).isTrue();
    assertThat(FhirTypes.isProfileOfCode(stringTypedCode)).isTrue();
    assertThat(FhirTypes.isProfileOfCode(profiledPatient)).isFalse();
  }

  @Test
  public void testIsTypeOrProfileOfCode() {
    assertThat(FhirTypes.isTypeOrProfileOfCode(getRawType("code"))).isTrue();
    assertThat(FhirTypes.isTypeOrProfileOfCode(enumTypedCode)).isTrue();
    assertThat(FhirTypes.isTypeOrProfileOfCode(stringTypedCode)).isTrue();
    assertThat(FhirTypes.isTypeOrProfileOfCode(profiledPatient)).isFalse();
  }

  @Test
  public void testIsCoding() {
    assertThat(FhirTypes.isCoding(getRawType("coding"))).isTrue();
    if (profiledCoding.isPresent()) {
      assertThat(FhirTypes.isCoding(profiledCoding.get())).isFalse();
    }
    assertThat(FhirTypes.isCoding(profiledPatient)).isFalse();
  }

  @Test
  public void testIsProfileOfCoding() {
    assertThat(FhirTypes.isProfileOfCoding(getRawType("coding"))).isFalse();
    if (profiledCoding.isPresent()) {
      assertThat(FhirTypes.isProfileOfCoding(profiledCoding.get())).isTrue();
    }
    assertThat(FhirTypes.isProfileOfCoding(profiledPatient)).isFalse();
  }

  @Test
  public void testIsTypeOrProfileOfCoding() {
    assertThat(FhirTypes.isTypeOrProfileOfCoding(getRawType("coding"))).isTrue();
    if (profiledCoding.isPresent()) {
      assertThat(FhirTypes.isTypeOrProfileOfCoding(profiledCoding.get())).isTrue();
    }
    assertThat(FhirTypes.isTypeOrProfileOfCoding(profiledPatient)).isFalse();
  }

  @Test
  public void testIsCodeableConcept() {
    assertThat(FhirTypes.isCodeableConcept(getRawType("codeable_concept"))).isTrue();
    assertThat(FhirTypes.isCodeableConcept(profiledCodeableConcept)).isFalse();
    assertThat(FhirTypes.isCodeableConcept(profiledPatient)).isFalse();
  }

  @Test
  public void testIsProfileOfCodeableConcept() {
    assertThat(FhirTypes.isProfileOfCodeableConcept(getRawType("codeable_concept"))).isFalse();
    assertThat(FhirTypes.isProfileOfCodeableConcept(profiledCodeableConcept)).isTrue();
    assertThat(FhirTypes.isProfileOfCodeableConcept(profiledPatient)).isFalse();
  }

  @Test
  public void testIsTypeOrProfileOfCodeableConcept() {
    assertThat(FhirTypes.isTypeOrProfileOfCodeableConcept(getRawType("codeable_concept"))).isTrue();
    assertThat(FhirTypes.isTypeOrProfileOfCodeableConcept(profiledCodeableConcept)).isTrue();
    assertThat(FhirTypes.isTypeOrProfileOfCodeableConcept(profiledPatient)).isFalse();
  }

  @Test
  public void testIsProfileOfType() {
    assertThat(FhirTypes.isProfileOfType(rawPatient, profiledPatient)).isTrue();
    assertThat(FhirTypes.isProfileOfType(rawPatient, rawPatient)).isFalse();
    assertThat(FhirTypes.isProfileOfType(getRawType("code"), profiledPatient)).isFalse();

    assertThat(FhirTypes.isProfileOfType(rawExtensionType, typedExtensionType)).isTrue();

    assertThat(FhirTypes.isProfileOfType(getRawType("code"), getRawType("code"))).isFalse();
    assertThat(FhirTypes.isProfileOfType(getRawType("code"), enumTypedCode)).isTrue();
    assertThat(FhirTypes.isProfileOfType(getRawType("code"), stringTypedCode)).isTrue();
  }

  @Test
  public void testIsReference() {
    assertThat(FhirTypes.isReference(getRawType("code"))).isFalse();
    assertThat(FhirTypes.isReference(getRawType("reference"))).isTrue();
  }

  @Test
  public void testIsPeriod() {
    assertThat(FhirTypes.isPeriod(getRawType("code"))).isFalse();
    assertThat(FhirTypes.isPeriod(getRawType("period"))).isTrue();
  }
}
