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

# Structure Definitions for the STU3 core types.
filegroup(
    name = "fhir_stu3_structure_definitions_filegroup",
    srcs = glob(
        ["hl7.fhir.core/3.0.1/package/StructureDefinition-*.json"],
        exclude = [
            "hl7.fhir.core/3.0.1/package/StructureDefinition-StructureDefinition.json",
            "hl7.fhir.core/3.0.1/package/StructureDefinition-cqif-condition.json",
            "hl7.fhir.core/3.0.1/package/StructureDefinition-cqif-cqlExpression.json",
            "hl7.fhir.core/3.0.1/package/StructureDefinition-cqif-fhirPathExpression.json",
            "hl7.fhir.core/3.0.1/package/StructureDefinition-cqif-library.json",
            "hl7.fhir.core/3.0.1/package/StructureDefinition-familymemberhistory-genetic.json",
        ],
    ) + glob([
        "hl7.fhir.core/3.0.1/modified/StructureDefinition-*.json",
    ]),
)

zip_file(
    name = "fhir_stu3_structure_definitions.zip",
    filegroup = ":fhir_stu3_structure_definitions_filegroup",
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

# Structure Definitions for the US-Core profiles.
filegroup(
    name = "uscore_stu3_structure_definitions_filegroup",
    srcs = glob([
        "hl7.fhir.us.core/1.0.1/package/StructureDefinition-*.json",
    ]),
)

zip_file(
    name = "uscore_stu3_structure_definitions.zip",
    filegroup = ":uscore_stu3_structure_definitions_filegroup",
)

structure_definition_package(
    package_name = "uscore-package",
    package_info = "uscore_stu3_package_info.prototxt",
    structure_definitions_zip = "uscore_stu3_structure_definitions.zip",
)
