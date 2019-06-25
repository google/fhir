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

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.fhir.proto.Annotations;
import com.google.fhir.r4.proto.FHIRVersionCode;
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
      "proto/stu3",
      ImmutableList.of(
          AbstractTypeCode.getDescriptor().getFile(),
          AddressTypeCode.getDescriptor().getFile(),
          UsCoreBirthSexCode.getDescriptor().getFile()),
      ImmutableMap.of(
          "datatypes.proto", Decimal.getDescriptor().getFile(),
          "resources.proto", StructureDefinition.getDescriptor().getFile(),
          "metadatatypes.proto", ElementDefinition.getDescriptor().getFile(),
          "extensions.proto", ElementDefinitionBindingName.getDescriptor().getFile(),
          "codes.proto", AbstractTypeCode.getDescriptor().getFile()),
      FHIRVersionCode.Value.NUM_3_0_1),
  R4(
      "google.fhir.r4.proto",
      "proto/r4",
      ImmutableList.of(
          com.google.fhir.r4.proto.AccountStatusCode.getDescriptor().getFile(),
          com.google.fhir.r4.proto.AddressTypeCode.getDescriptor().getFile()),
      ImmutableMap.of(
          "datatypes.proto", com.google.fhir.r4.proto.Decimal.getDescriptor().getFile(),
          "codes.proto", com.google.fhir.r4.proto.AccountStatusCode.getDescriptor().getFile()),
      FHIRVersionCode.Value.NUM_4_0_0);

  // The proto package of the core FHIR structures.
  public final String coreProtoPackage;
  // The import location of the core FHIR proto package.
  public final String coreProtoImportRoot;
  // A list of all the FHIR code files.
  public final ImmutableList<FileDescriptor> codeTypeList;
  // A map of all the FHIR core types to their corresponding files.
  public final ImmutableMap<String, FileDescriptor> coreTypeMap;

  public final FHIRVersionCode.Value minorVersion;

  private FhirVersion(
      String coreProtoPackage,
      String coreProtoImportRoot,
      ImmutableList<FileDescriptor> codeTypeList,
      ImmutableMap<String, FileDescriptor> coreTypeMap,
      FHIRVersionCode.Value minorVersion) {
    this.coreProtoPackage = coreProtoPackage;
    this.coreProtoImportRoot = coreProtoImportRoot;
    this.codeTypeList = codeTypeList;
    this.coreTypeMap = coreTypeMap;
    this.minorVersion = minorVersion;
  }

  /** Converts from a proto enum value. */
  public static FhirVersion fromAnnotation(Annotations.FhirVersion protoEnum) {
    switch (protoEnum) {
      case STU3:
        return STU3;
      case R4:
        return R4;
      default:
        throw new IllegalArgumentException("FHIR version unknown or unsupported: " + protoEnum);
    }
  }

  /** Converts to a proto enum value. */
  public Annotations.FhirVersion toAnnotation() {
    switch (this) {
      case STU3:
        return Annotations.FhirVersion.STU3;
      case R4:
        return Annotations.FhirVersion.R4;
    }
    throw new IllegalArgumentException("Unhandled FHIR version: " + this);
  }
}
