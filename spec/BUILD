# Data files containing the base FHIR specification.

load("//bazel:protogen.bzl", "fhir_package")

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

# STU3 core types.
fhir_package(
    package_name = "fhir_stu3_package",
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
    ]),
    package_info = "fhir_stu3_package_info.prototxt",
)

# Current R4 package
filegroup(
    name = "r4",
    srcs = glob([
        "hl7.fhir.core/4.0.0/package/*.json",
    ]),
)

fhir_package(
    package_name = "fhir_r4_package",
    definitions = glob(
        ["hl7.fhir.core/4.0.0/package/StructureDefinition-*.json"],
        exclude = [
            "hl7.fhir.core/4.0.0/package/StructureDefinition-example-*.json",
            "hl7.fhir.core/4.0.0/package/StructureDefinition-*.*.json",
        ],
    ) + [
        "hl7.fhir.core/4.0.0/package/Bundle-v2-valuesets.json",
        "hl7.fhir.core/4.0.0/package/Bundle-v3-valuesets.json",
        "hl7.fhir.core/4.0.0/package/Bundle-valuesets.json",
    ],
    package_info = "fhir_r4_package_info.prototxt",
)

# US-Core profiles for FHIR STU3.
fhir_package(
    package_name = "uscore_stu3_package",
    definitions = glob([
        "hl7.fhir.us.core/2.0.0/package/StructureDefinition-*.json",
    ]),
    package_info = "uscore_stu3_package_info.prototxt",
)

# US-Core Profiles for FHIR R4

fhir_package(
    package_name = "uscore_r4_package",
    definitions = glob([
        "hl7.fhir.us.core/3.0.0/package/StructureDefinition-*.json",
    ]) + glob(["hl7.fhir.us.core/3.0.0/package/ValueSet-*.json"]) + glob([
        "hl7.fhir.us.core/3.0.0/package/CodeSystem-*.json",
    ]),
    package_info = "uscore_r4_package_info.prototxt",
)

# US QI core profiles.
fhir_package(
    package_name = "qicore_3_2_0_package",
    definitions = glob([
        "hl7.fhir.us.qicore/3.2.0/*.json",
    ]),
    package_info = "qicore_stu3_package_info.prototxt",
)

# US QI core profiles.
fhir_package(
    package_name = "qicore_3_3_0_package",
    definitions = glob([
        "hl7.fhir.us.qicore/3.3.0/*.json",
    ]),
    package_info = "qicore_r4_package_info.prototxt",
)
