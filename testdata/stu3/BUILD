# Testdata for the parsers

package(default_visibility = ["//visibility:public"])

load("//bazel:protogen.bzl", "zip_file")
load("//bazel:protogen.bzl", "structure_definition_package")

licenses(["notice"])

# These files are used to help test bigquery integration.
filegroup(
    name = "bigquery",
    srcs = glob([
        "bigquery/*.json",
    ]),
)

# These files are prototxt representations of sample resources provided by FHIR.
filegroup(
    name = "examples",
    srcs = glob([
        "examples/*.prototxt",
    ]),
)

# These files are ndjson representations of some sample resources provided by FHIR.
filegroup(
    name = "ndjson",
    srcs = glob([
        "ndjson/*.ndjson",
    ]),
)

# These files contain testdata for generating protos.
filegroup(
    name = "descriptors",
    srcs = glob([
        "descriptors/*",
    ]),
)

# These files contain google extensions.
filegroup(
    name = "google",
    srcs = glob([
        "google/*.json",
        "google/*.descriptor.prototxt",
        "google/*.extension.prototxt",
        "google/*.message.prototxt",
    ]),
)

# These files contain StructureDefinitions used to generate the protos.
filegroup(
    name = "fhir_structure_definitions",
    srcs = glob([
        "structure_definitions/*.profile.json",
        "extensions/*.json",
    ]),
)

zip_file(
    name = "fhir_structure_definitions.zip",
    srcs = glob([
        "structure_definitions/*.profile.json",
        "extensions/*.json",
    ]),
)

structure_definition_package(
    package_name = "fhir",
    package_info = "fhir_package_info.prototxt",
    structure_definitions_zip = "fhir_structure_definitions.zip",
)

# These files contain valid and invalid examples for various types.
filegroup(
    name = "validation",
    srcs = glob([
        "validation/*.valid.ndjson",
        "validation/*.invalid.ndjson",
    ]),
)

filegroup(
    name = "labels",
    srcs = glob([
        "labels/*",
    ]),
)

filegroup(
    name = "tensorflow",
    srcs = glob([
        "tensorflow/*",
    ]),
)
