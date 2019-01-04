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

import com.google.common.base.Ascii;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.fhir.stu3.proto.AbstractTypeCode;
import com.google.fhir.stu3.proto.AddressTypeCode;
import com.google.fhir.stu3.proto.Decimal;
import com.google.fhir.stu3.proto.ElementDefinition;
import com.google.fhir.stu3.proto.ElementDefinitionBindingName;
import com.google.fhir.stu3.proto.StructureDefinition;
import com.google.fhir.stu3.uscore.UsCoreBirthSexCode;
import com.google.protobuf.Descriptors.FileDescriptor;

/** Enum that represents different FHIR versions and stores their corresponding types. */
public enum FhirVersion {
  STU3(
      "google.fhir.stu3.proto",
      ImmutableList.of(
          AbstractTypeCode.getDescriptor().getFile(),
          AddressTypeCode.getDescriptor().getFile(),
          UsCoreBirthSexCode.getDescriptor().getFile()),
      ImmutableMap.of(
          "datatypes.proto", Decimal.getDescriptor().getFile(),
          "resources.proto", StructureDefinition.getDescriptor().getFile(),
          "metadatatypes.proto", ElementDefinition.getDescriptor().getFile(),
          "extensions.proto", ElementDefinitionBindingName.getDescriptor().getFile(),
          "codes.proto", AbstractTypeCode.getDescriptor().getFile()));

  // The package of the core FHIR structures.
  public final String coreFhirPackage;
  // A list of all the FHIR code files.
  public final ImmutableList<FileDescriptor> codeTypeList;
  // A map of all the FHIR core types to their corresponding files.
  public final ImmutableMap<String, FileDescriptor> coreTypeMap;

  private FhirVersion(
      String coreFhirPackage,
      ImmutableList<FileDescriptor> codeTypeList,
      ImmutableMap<String, FileDescriptor> coreTypeMap) {
    this.coreFhirPackage = coreFhirPackage;
    this.codeTypeList = codeTypeList;
    this.coreTypeMap = coreTypeMap;
  }

  /** Converts from a String value. */
  public static FhirVersion fromString(String code) {
    for (FhirVersion fhirVersion : FhirVersion.values()) {
      if (Ascii.equalsIgnoreCase(fhirVersion.toString(), code)) {
        return fhirVersion;
      }
    }
    return null;
  }
}
