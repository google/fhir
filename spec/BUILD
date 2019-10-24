# Data files containing the base FHIR specification.

load("//bazel:protogen.bzl", "fhir_package", "zip_file")

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

# Structure Definitions for the DSTU2 core types.
filegroup(
    name = "fhir_dstu2_structure_definitions_filegroup",
    srcs = glob(
        ["hl7.fhir.core/1.0.2/package/StructureDefinition-*.json"],
        exclude = ["hl7.fhir.core/1.0.2/package/StructureDefinition-example.json"],
    ),
)

zip_file(
    name = "fhir_dstu2_structure_definitions.zip",
    filegroup = ":fhir_dstu2_structure_definitions_filegroup",
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
    name = "fhir_stu3_definitions_filegroup",
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
    name = "fhir_stu3_definitions.zip",
    filegroup = ":fhir_stu3_definitions_filegroup",
)

fhir_package(
    package_name = "fhir_stu3_package",
    definitions_zip = "fhir_stu3_definitions.zip",
    package_info = "fhir_stu3_package_info.prototxt",
)

# Current R4 package
filegroup(
    name = "r4",
    srcs = glob([
        "hl7.fhir.core/4.0.0/package/*.json",
    ]),
)

filegroup(
    name = "fhir_r4_definitions_filegroup",
    srcs = glob(
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
)

# Structure Definitions for the R4 core types.
zip_file(
    name = "fhir_r4_definitions.zip",
    filegroup = ":fhir_r4_definitions_filegroup",
)

fhir_package(
    package_name = "fhir_r4_package",
    definitions_zip = "fhir_r4_definitions.zip",
    package_info = "fhir_r4_package_info.prototxt",
)

# Structure Definitions for the STU3 US-Core profiles.
filegroup(
    name = "uscore_stu3_definitions_filegroup",
    srcs = glob([
        "hl7.fhir.us.core/2.0.0/package/StructureDefinition-*.json",
    ]),
)

zip_file(
    name = "uscore_stu3_definitions.zip",
    filegroup = ":uscore_stu3_definitions_filegroup",
)

fhir_package(
    package_name = "uscore-stu3-package",
    definitions_zip = "uscore_stu3_definitions.zip",
    package_info = "uscore_stu3_package_info.prototxt",
)

# Structure Definitions for the R4 US-Core Profiles
filegroup(
    name = "uscore_r4_definitions_filegroup",
    srcs = glob([
        "hl7.fhir.us.core/3.0.0/package/StructureDefinition-*.json",
    ]) + glob(["hl7.fhir.us.core/3.0.0/package/ValueSet-*.json"]) + glob([
        "hl7.fhir.us.core/3.0.0/package/CodeSystem-*.json",
    ]),
)

zip_file(
    name = "uscore_r4_definitions.zip",
    filegroup = ":uscore_r4_definitions_filegroup",
)

fhir_package(
    package_name = "uscore-r4-package",
    definitions_zip = "uscore_r4_definitions.zip",
    package_info = "uscore_r4_package_info.prototxt",
)

# Structure Definitions for the US QI core profiles.
filegroup(
    name = "qicore_stu3_definitions_filegroup",
    srcs = glob([
        "hl7.fhir.us.qicore/3.2.0/StructureDefinition-*.json",
    ]),
)

zip_file(
    name = "qicore_stu3_definitions.zip",
    filegroup = ":qicore_stu3_definitions_filegroup",
)

fhir_package(
    package_name = "qicore-package",
    definitions_zip = "qicore_stu3_definitions.zip",
    package_info = "qicore_stu3_package_info.prototxt",
)
