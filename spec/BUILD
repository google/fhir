# Data files containing the base FHIR specification.

load("//bazel:protogen.bzl", "fhir_package")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

exports_files(["uscore_package_info.prototxt"])

# Technical corrections for 4.0.1.
filegroup(
    name = "core_4_0_1_technical_corections",
    srcs = ["hl7.fhir.core/4.0.1/technical_corrections.textproto"],
)

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

# STU3 core types.
fhir_package(
    name = "fhir_stu3",
    definitions = glob(
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
    ]) + glob(
        [
            "hl7.fhir.core/3.0.1/package/ValueSet-*.json",
        ],
    ) + glob(
        [
            "hl7.fhir.core/3.0.1/package/CodeSystem-*.json",
        ],
    ) + ["hl7.fhir.core/3.0.1/package/package.json"],
    package_info = "fhir_stu3_package_info.prototxt",
)

# Current R4 package
filegroup(
    name = "r4",
    srcs = glob([
        "hl7.fhir.core/4.0.1/package/*.json",
    ]),
)

# Example testdata, e.g., Patient-example.json, was dropped in the 4.0.1 npm
filegroup(
    name = "r4_examples",
    srcs = glob([
        "hl7.fhir.r4.examples/4.0.1/package/*",
    ]),
)

fhir_package(
    name = "fhir_r4",
    definitions =
        glob(
            ["hl7.fhir.core/4.0.1/package/StructureDefinition-*.json"],
            exclude = [
                "hl7.fhir.core/4.0.1/package/StructureDefinition-example-*.json",
                "hl7.fhir.core/4.0.1/package/StructureDefinition-*.*.json",
                "hl7.fhir.core/4.0.1/package/StructureDefinition-structuredefinition-xml-type.json",
                "hl7.fhir.core/4.0.1/package/StructureDefinition-structuredefinition-rdf-type.json",
                "hl7.fhir.core/4.0.1/package/StructureDefinition-structuredefinition-json-type.json",
            ],
        ) + glob(
            [
                "hl7.fhir.core/4.0.1/package/ValueSet-*.json",
            ],
        ) + glob(
            ["hl7.fhir.core/4.0.1/package/CodeSystem-*.json"],
        ) + glob(
            ["hl7.fhir.core/4.0.1/package/SearchParameter-*.json"],
            exclude = [
                "hl7.fhir.core/4.0.1/package/SearchParameter-*-extensions-*.json",
            ],
        ) + ["hl7.fhir.core/4.0.1/package/package.json"],
    package_info = "fhir_r4_package_info.prototxt",
)

# US-Core profiles for FHIR STU3.
fhir_package(
    name = "uscore_stu3_package",
    definitions = glob([
        "hl7.fhir.us.core/2.0.0/package/StructureDefinition-*.json",
    ]),
    package_info = "uscore_stu3_package_info.prototxt",
)

# US-Core Profiles for FHIR R4

fhir_package(
    name = "uscore_r4_package",
    definitions = glob([
        "hl7.fhir.us.core/3.1.0/package/StructureDefinition-*.json",
    ]) + glob(["hl7.fhir.us.core/3.1.0/package/ValueSet-*.json"]) + glob([
        "hl7.fhir.us.core/3.1.0/package/CodeSystem-*.json",
    ]) + ["hl7.fhir.us.core/3.1.0/package/package.json"],
    package_info = "uscore_r4_package_info.prototxt",
)
