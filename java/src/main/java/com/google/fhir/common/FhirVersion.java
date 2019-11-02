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
import com.google.fhir.r4.core.FHIRVersionCode;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FileDescriptor;

/** Enum that represents different FHIR versions and stores their corresponding types. */
public enum FhirVersion {
  STU3(
      "google.fhir.stu3.proto",
      "proto/stu3",
      ImmutableList.of(
          com.google.fhir.stu3.proto.AbstractTypeCode.getDescriptor().getFile(),
          com.google.fhir.stu3.proto.AddressTypeCode.getDescriptor().getFile()),
      ImmutableMap.of(
          "datatypes.proto", com.google.fhir.stu3.proto.Decimal.getDescriptor().getFile(),
          "resources.proto",
              com.google.fhir.stu3.proto.StructureDefinition.getDescriptor().getFile(),
          "metadatatypes.proto",
              com.google.fhir.stu3.proto.ElementDefinition.getDescriptor().getFile(),
          "extensions.proto",
              com.google.fhir.stu3.proto.ElementDefinitionBindingName.getDescriptor().getFile(),
          "codes.proto", com.google.fhir.stu3.proto.AbstractTypeCode.getDescriptor().getFile()),
      com.google.fhir.stu3.proto.ContainedResource.getDescriptor(),
      FHIRVersionCode.Value.V_3_0_1),
  R4(
      "google.fhir.r4.core",
      "proto/r4/core",
      ImmutableList.of(
          com.google.fhir.r4.core.AccountStatusCode.getDescriptor().getFile(),
          com.google.fhir.r4.core.BodyLengthUnitsValueSet.getDescriptor().getFile()),
      ImmutableMap.of(
          "datatypes.proto", com.google.fhir.r4.core.Decimal.getDescriptor().getFile(),
          "extensions.proto", com.google.fhir.r4.core.MimeType.getDescriptor().getFile(),
          "codes.proto", com.google.fhir.r4.core.AccountStatusCode.getDescriptor().getFile(),
          "valuesets.proto",
              com.google.fhir.r4.core.BodyLengthUnitsValueSet.getDescriptor().getFile()),
      com.google.fhir.r4.core.ContainedResource.getDescriptor(),
      FHIRVersionCode.Value.V_4_0_0);

  // The proto package of the core FHIR structures.
  public final String coreProtoPackage;
  
  // The import location of the core FHIR proto package.
  public final String coreProtoImportRoot;

  // A list of all files that define enums (CodeSystems and ValueSets) from the core FHIR package.
  // This is necessary since the core FHIR package defines too many codes and valuesets to make
  // enums for them all.  Instead, we only generate those that are used by the core resources and
  // profiles.  If other CodeSystems/ValueSets get referenced by non-core profiles, the enum needs
  // to be written into the profile definition itself.
  // This list is used so that the ProtoGenerator can determine if a core enum was defined in the
  // core code files, or needs to be inlined.
  public final ImmutableList<FileDescriptor> codeTypeList;

  // A map of all the FHIR core types to their corresponding files.
  public final ImmutableMap<String, FileDescriptor> coreTypeMap;

  public final Descriptor coreContainedResource;

  public final FHIRVersionCode.Value minorVersion;

  // The path for annotation definition of proto options.
  public static final String ANNOTATION_PATH = "proto";

  private FhirVersion(
      String coreProtoPackage,
      String coreProtoImportRoot,
      ImmutableList<FileDescriptor> codeTypeList,
      ImmutableMap<String, FileDescriptor> coreTypeMap,
      Descriptor coreContainedResource,
      FHIRVersionCode.Value minorVersion) {
    this.coreProtoPackage = coreProtoPackage;
    this.coreProtoImportRoot = coreProtoImportRoot;
    this.codeTypeList = codeTypeList;
    this.coreTypeMap = coreTypeMap;
    this.coreContainedResource = coreContainedResource;
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
