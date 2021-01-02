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
  private final Descriptor rawCodeType;
  private final Descriptor enumTypedCode;
  private final Descriptor stringTypedCode;

  public FhirTypesTest(
      Descriptor rawPatient,
      Descriptor profiledPatient,
      Descriptor rawExtensionType,
      Descriptor typedExtensionType,
      Descriptor rawCodeType,
      Descriptor enumTypedCode,
      Descriptor stringTypedCode) {
    this.rawPatient = rawPatient;
    this.profiledPatient = profiledPatient;
    this.rawExtensionType = rawExtensionType;
    this.typedExtensionType = typedExtensionType;
    this.rawCodeType = rawCodeType;
    this.enumTypedCode = enumTypedCode;
    this.stringTypedCode = stringTypedCode;
  }

  @Parameterized.Parameters
  public static ImmutableCollection<Object[]> params() {
    return ImmutableList.of(
        new Object[] {
          com.google.fhir.r4.core.Patient.getDescriptor(),
          com.google.fhir.r4.uscore.USCorePatientProfile.getDescriptor(),
          com.google.fhir.r4.core.Extension.getDescriptor(),
          com.google.fhir.r4.core.AddressGeolocation.getDescriptor(),
          com.google.fhir.r4.core.Code.getDescriptor(),
          com.google.fhir.r4.core.Patient.GenderCode.getDescriptor(),
          com.google.fhir.r4.core.Binary.ContentTypeCode.getDescriptor()
        },
        new Object[] {
          com.google.fhir.stu3.proto.Patient.getDescriptor(),
          com.google.fhir.stu3.uscore.UsCorePatient.getDescriptor(),
          com.google.fhir.stu3.proto.Extension.getDescriptor(),
          com.google.fhir.stu3.proto.AddressGeolocation.getDescriptor(),
          com.google.fhir.stu3.proto.Code.getDescriptor(),
          com.google.fhir.stu3.proto.AdministrativeGenderCode.getDescriptor(),
          com.google.fhir.stu3.proto.MimeTypeCode.getDescriptor()
        });
  }

  @Test
  public void isExtension() {
    assertThat(FhirTypes.isExtension(rawExtensionType)).isTrue();
    assertThat(FhirTypes.isExtension(typedExtensionType)).isFalse();
    assertThat(FhirTypes.isExtension(rawCodeType)).isFalse();
    assertThat(FhirTypes.isExtension(profiledPatient)).isFalse();
  }

  @Test
  public void isProfileOfExtension() {
    assertThat(FhirTypes.isProfileOfExtension(rawExtensionType)).isFalse();
    assertThat(FhirTypes.isProfileOfExtension(typedExtensionType)).isTrue();
    assertThat(FhirTypes.isProfileOfExtension(rawCodeType)).isFalse();
    assertThat(FhirTypes.isProfileOfExtension(profiledPatient)).isFalse();
  }

  @Test
  public void isTypeOrProfileOfExtension() {
    assertThat(FhirTypes.isTypeOrProfileOfExtension(rawExtensionType)).isTrue();
    assertThat(FhirTypes.isTypeOrProfileOfExtension(typedExtensionType)).isTrue();
    assertThat(FhirTypes.isTypeOrProfileOfExtension(rawCodeType)).isFalse();
    assertThat(FhirTypes.isTypeOrProfileOfExtension(profiledPatient)).isFalse();
  }

  @Test
  public void isCode() {
    assertThat(FhirTypes.isCode(rawCodeType)).isTrue();
    assertThat(FhirTypes.isCode(enumTypedCode)).isFalse();
    assertThat(FhirTypes.isCode(stringTypedCode)).isFalse();
    assertThat(FhirTypes.isCode(profiledPatient)).isFalse();
  }

  @Test
  public void isProfileOfCode() {
    assertThat(FhirTypes.isProfileOfCode(rawCodeType)).isFalse();
    assertThat(FhirTypes.isProfileOfCode(enumTypedCode)).isTrue();
    assertThat(FhirTypes.isProfileOfCode(stringTypedCode)).isTrue();
    assertThat(FhirTypes.isProfileOfCode(profiledPatient)).isFalse();
  }

  @Test
  public void isTypeOrProfileOfCode() {
    assertThat(FhirTypes.isTypeOrProfileOfCode(rawCodeType)).isTrue();
    assertThat(FhirTypes.isTypeOrProfileOfCode(enumTypedCode)).isTrue();
    assertThat(FhirTypes.isTypeOrProfileOfCode(stringTypedCode)).isTrue();
    assertThat(FhirTypes.isTypeOrProfileOfCode(profiledPatient)).isFalse();
  }

  @Test
  public void isProfileOf() {
    assertThat(FhirTypes.isProfileOf(rawPatient, profiledPatient)).isTrue();
    assertThat(FhirTypes.isProfileOf(rawPatient, rawPatient)).isFalse();
    assertThat(FhirTypes.isProfileOf(rawCodeType, profiledPatient)).isFalse();

    assertThat(FhirTypes.isProfileOf(rawExtensionType, typedExtensionType)).isTrue();

    assertThat(FhirTypes.isProfileOf(rawCodeType, rawCodeType)).isFalse();
    assertThat(FhirTypes.isProfileOf(rawCodeType, enumTypedCode)).isTrue();
    assertThat(FhirTypes.isProfileOf(rawCodeType, stringTypedCode)).isTrue();
  }
}
