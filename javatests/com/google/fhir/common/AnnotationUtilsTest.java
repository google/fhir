//    Copyright 2018 Google Inc.
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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.google.fhir.stu3.proto.Base64Binary;
import com.google.fhir.stu3.proto.Boolean;
import com.google.fhir.stu3.proto.Code;
import com.google.fhir.stu3.proto.Observation;
import com.google.fhir.stu3.proto.Patient;
import com.google.fhir.stu3.proto.Reference;
import com.google.fhir.stu3.uscore.UsCorePatient;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link AnnotationUtils}. */
@RunWith(JUnit4.class)
public final class AnnotationUtilsTest {

  @Test
  public void isPrimitiveType() {
    assertTrue(AnnotationUtils.isPrimitiveType(Boolean.getDescriptor()));
    assertTrue(AnnotationUtils.isPrimitiveType(Boolean.getDescriptor().toProto()));
    assertTrue(AnnotationUtils.isPrimitiveType(Base64Binary.getDescriptor()));
    assertTrue(AnnotationUtils.isPrimitiveType(Base64Binary.getDescriptor().toProto()));
    assertTrue(AnnotationUtils.isPrimitiveType(Code.getDescriptor()));
    assertTrue(AnnotationUtils.isPrimitiveType(Code.getDescriptor().toProto()));
    assertFalse(AnnotationUtils.isPrimitiveType(Patient.getDescriptor()));
    assertFalse(AnnotationUtils.isPrimitiveType(Patient.getDescriptor().toProto()));
    assertFalse(AnnotationUtils.isPrimitiveType(DescriptorProto.getDescriptor().toProto()));

    assertTrue(AnnotationUtils.isPrimitiveType(Code.getDefaultInstance()));
    assertFalse(AnnotationUtils.isPrimitiveType(Patient.getDefaultInstance()));
  }

  @Test
  public void isResource() {
    assertFalse(AnnotationUtils.isResource(Boolean.getDescriptor()));
    assertFalse(AnnotationUtils.isResource(Boolean.getDescriptor().toProto()));
    assertTrue(AnnotationUtils.isResource(Patient.getDescriptor()));
    assertTrue(AnnotationUtils.isResource(Patient.getDescriptor().toProto()));
    assertFalse(AnnotationUtils.isResource(DescriptorProto.getDescriptor().toProto()));

    assertFalse(AnnotationUtils.isResource(Code.getDefaultInstance()));
    assertTrue(AnnotationUtils.isResource(Patient.getDefaultInstance()));
  }

  @Test
  public void isReference() {
    assertFalse(AnnotationUtils.isReference(Boolean.getDescriptor()));
    assertFalse(AnnotationUtils.isReference(Boolean.getDescriptor().toProto()));
    assertTrue(AnnotationUtils.isReference(Reference.getDescriptor()));
    assertTrue(AnnotationUtils.isReference(Reference.getDescriptor().toProto()));

    assertFalse(AnnotationUtils.isReference(Code.getDefaultInstance()));
    assertFalse(AnnotationUtils.isReference(Code.getDescriptor().toProto()));
    assertTrue(AnnotationUtils.isReference(Reference.getDefaultInstance()));
    assertTrue(AnnotationUtils.isReference(Reference.getDescriptor().toProto()));
  }

  @Test
  public void isProfileOf() {
    assertTrue(AnnotationUtils.isProfileOf(Patient.getDescriptor(), UsCorePatient.getDescriptor()));
    assertFalse(
        AnnotationUtils.isProfileOf(Observation.getDescriptor(), UsCorePatient.getDescriptor()));
  }
}
