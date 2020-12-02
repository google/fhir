//    Copyright 2020 Google LLC
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

package com.google.fhir.protogen;

import static com.google.common.truth.Truth.assertThat;
import static junit.framework.Assert.assertTrue;
import static org.junit.Assert.assertFalse;

import com.google.common.testing.EqualsTester;
import com.google.fhir.proto.PackageInfo;
import java.io.IOException;
import java.util.ArrayList;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Tests for FhirPackage.
 */
@RunWith(JUnit4.class)
public final class FhirPackageTest {

  @Test
  public void equalityTest() throws Exception {
    new EqualsTester()
        .addEqualityGroup(
            new FhirPackage(
                PackageInfo.newBuilder().setProtoPackage("foo").build(),
                new ArrayList<>(),
                new ArrayList<>(),
                new ArrayList<>()),
            new FhirPackage(
                PackageInfo.newBuilder().setProtoPackage("foo").build(),
                new ArrayList<>(),
                new ArrayList<>(),
                new ArrayList<>()))
        .addEqualityGroup(
            new FhirPackage(
                PackageInfo.newBuilder().setProtoPackage("bar").build(),
                new ArrayList<>(),
                new ArrayList<>(),
                new ArrayList<>()),
            new FhirPackage(
                PackageInfo.newBuilder().setProtoPackage("bar").build(),
                new ArrayList<>(),
                new ArrayList<>(),
                new ArrayList<>()))
        .testEquals();
  }

  private static final int R4_DEFINITIONS_COUNT = 653;
  private static final int R4_CODESYSTEMS_COUNT = 1062;
  private static final int R4_VALUESETS_COUNT = 1316;

  @Test
  public void loadTest() throws IOException {
    FhirPackage fhirPackage = FhirPackage.load("spec/fhir_r4_package.zip");
    assertThat(fhirPackage.packageInfo.getProtoPackage()).isEqualTo("google.fhir.r4.core");
    assertThat(fhirPackage.structureDefinitions).hasSize(R4_DEFINITIONS_COUNT);
    assertThat(fhirPackage.codeSystems).hasSize(R4_CODESYSTEMS_COUNT);
    assertThat(fhirPackage.valueSets).hasSize(R4_VALUESETS_COUNT);
  }

  @Test
  public void filterTest() throws IOException {
    FhirPackage fhirPackage = FhirPackage.load("spec/fhir_r4_package.zip");
    assertTrue(
        fhirPackage.structureDefinitions.stream()
            .anyMatch(def -> def.getId().getValue().equals("Patient")));

    FhirPackage filteredPackage =
        fhirPackage.filterResources(def -> !def.getId().getValue().equals("Patient"));
    assertFalse(
        filteredPackage.structureDefinitions.stream()
            .anyMatch(def -> def.getId().getValue().equals("Patient")));
    assertThat(filteredPackage.structureDefinitions).hasSize(R4_DEFINITIONS_COUNT - 1);
  }
}
