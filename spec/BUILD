# Data files containing the base FHIR specification.

load("//bazel:protogen.bzl", "structure_definition_package", "zip_file")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

exports_files(["uscore_package_info.prototxt"])

# Current DSTU2 package
filegroup(
    name = "dstu2",
    srcs = glob([
        "hl7.fhir.core/1.0.2/package/*.json",
    ]),
)

# Current STU3 package
filegroup(
    name = "stu3",
    srcs = glob([
        "hl7.fhir.core/3.0.1/package/*.json",
    ]),
)

filegroup(
    name = "fhir_stu3_structure_definitions",
    srcs = glob(["hl7.fhir.core/3.0.1/package/StructureDefinition-*.json"]) + glob([
        "hl7.fhir.core/3.0.1/modified/StructureDefinition-*.json",
    ]),
)

# Structure Definitions for the STU3 core types.
zip_file(
    name = "fhir_stu3_structure_definitions.zip",
    srcs = glob(["hl7.fhir.core/3.0.1/package/StructureDefinition-*.json"]),
)

structure_definition_package(
    package_name = "fhir_stu3_package",
    package_info = "fhir_stu3_package_info.prototxt",
    structure_definitions_zip = "fhir_stu3_structure_definitions.zip",
)

# Current R4 package
filegroup(
    name = "r4",
    srcs = glob([
        "hl7.fhir.core/4.0.0/package/*.json",
    ]),
)

# US Core package
filegroup(
    name = "uscore",
    srcs = glob([
        "hl7.fhir.us.core/1.0.1/package/StructureDefinition*.json",
    ]),
)

# Structure Definitions for the US-Core profiles.
zip_file(
    name = "uscore_structure_definitions.zip",
    srcs = glob(["hl7.fhir.us.core/1.0.1/package/StructureDefinition-*.json"]),
)

structure_definition_package(
    package_name = "uscore-package",
    package_info = "uscore_package_info.prototxt",
    structure_definitions_zip = "uscore_structure_definitions.zip",
)
