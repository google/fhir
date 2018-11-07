# Data files containing the base FHIR specification.

package(default_visibility = ["//visibility:public"])

load("//bazel:protogen.bzl", "zip_file")
load("//bazel:protogen.bzl", "structure_definition_package")

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

# Current R4 package
filegroup(
    name = "r4",
    srcs = glob([
        "hl7.fhir.core/3.5.0/package/*.json",
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
    srcs = glob(["hl7.fhir.us.core/1.0.1/package/StructureDefinition*.json"]),
)

structure_definition_package(
    package_name = "uscore-package",
    package_info = "uscore_package_info.prototxt",
    structure_definitions_zip = "uscore_structure_definitions.zip",
)
