# Testdata for the parsers

package(default_visibility = ["//visibility:public"])

load("//bazel:protogen.bzl", "zip_file")

licenses(["notice"])

exports_files(["fhir_package_info.prototxt"])

# These files are used to help test bigquery integration.
filegroup(
    name = "bigquery",
    srcs = glob([
        "bigquery/*.json",
    ]),
)

# These files are sample resources from the FHIR spec.
filegroup(
    name = "examples",
    srcs = glob([
        "examples/*.json",
        "examples/*.prototxt",
    ]),
)

# These files contain testdata for generating protos.
filegroup(
    name = "descriptors",
    srcs = glob([
        "extensions/*.descriptor.prototxt",
        "structure_definitions/*.descriptor.prototxt",
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

# These files contain valid and invalid examples for various types.
filegroup(
    name = "validation",
    srcs = glob([
        "validation/*.valid.ndjson",
        "validation/*.invalid.ndjson",
    ]),
)
