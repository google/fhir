# STU3 Testdata

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

# These files are used to help test bigquery integration.
filegroup(
    name = "bigquery",
    srcs = glob([
        "bigquery/*.json",
    ]),
)

# These files are representations of sample resources provided by FHIR.
filegroup(
    name = "examples",
    srcs = glob([
        "examples/*",
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

# These files contain extensions in both profiled and unprofiled form.
filegroup(
    name = "extensions",
    srcs = glob([
        "extensions/*",
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
    name = "profilegenerator",
    srcs = glob([
        "profilegenerator/*",
    ]),
)

filegroup(
    name = "footprint",
    srcs = glob([
        "footprint/*",
    ]),
)

alias(
    name = "profiles",
    actual = "//testdata/stu3/profiles:testdata",
)
