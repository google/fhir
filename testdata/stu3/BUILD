# Testdata for the parsers

package(default_visibility = ["//visibility:public"])

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

# These files contain valid and invalid examples for various types.
filegroup(
    name = "validation",
    srcs = glob([
        "validation/*",
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

filegroup(
    name = "profilegenerator",
    srcs = glob([
        "profilegenerator/*",
    ]),
)

alias(
    name = "profiles",
    actual = "//testdata/stu3/profiles:testdata",
)
