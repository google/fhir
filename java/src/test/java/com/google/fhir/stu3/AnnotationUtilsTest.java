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

package com.google.fhir.stu3;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.google.fhir.stu3.proto.Base64Binary;
import com.google.fhir.stu3.proto.Boolean;
import com.google.fhir.stu3.proto.Code;
import com.google.fhir.stu3.proto.Patient;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link AnnotationUtils}. */
@RunWith(JUnit4.class)
public final class AnnotationUtilsTest {

  @Test
  public void isPrimitiveType() {
    assertTrue(AnnotationUtils.isPrimitiveType(Boolean.getDescriptor()));
    assertTrue(AnnotationUtils.isPrimitiveType(Base64Binary.getDescriptor()));
    assertTrue(AnnotationUtils.isPrimitiveType(Code.getDescriptor()));
    assertFalse(AnnotationUtils.isPrimitiveType(Patient.getDescriptor()));
  }

  @Test
  public void isResource() {
    assertFalse(AnnotationUtils.isResource(Boolean.getDescriptor()));
    assertTrue(AnnotationUtils.isResource(Patient.getDescriptor()));
  }
}
