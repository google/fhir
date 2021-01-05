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
import static com.google.fhir.common.ProtoUtils.findField;
import static org.junit.Assert.assertThrows;

import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableList;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Message;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

// TODO: Add test coverage for older functions.
@RunWith(Parameterized.class)
public final class ProtoUtilsTest {
  private final Message patient;

  public ProtoUtilsTest(Message patient) {
    this.patient = patient;
  }

  @Parameterized.Parameters
  public static ImmutableCollection<Object[]> params() {
    return ImmutableList.of(
        new Object[] {com.google.fhir.r4.core.Patient.getDefaultInstance()},
        new Object[] {com.google.fhir.stu3.proto.Patient.getDefaultInstance()});
  }

  @Test
  public void findField_descriptor_success() {
    Descriptor descriptor = patient.getDescriptorForType();
    assertThat(findField(descriptor, "id")).isEqualTo(descriptor.findFieldByName("id"));
  }

  @Test
  public void findField_message_success() {
    Descriptor descriptor = patient.getDescriptorForType();
    assertThat(findField(patient, "id")).isEqualTo(descriptor.findFieldByName("id"));
  }

  @Test
  public void findField_descriptor_failure() {
    Descriptor descriptor = patient.getDescriptorForType();
    assertThrows(IllegalArgumentException.class, () -> findField(descriptor, "wizbang"));
  }

  @Test
  public void findField_message_failure() {
    assertThrows(IllegalArgumentException.class, () -> findField(patient, "wizbang"));
  }
}
