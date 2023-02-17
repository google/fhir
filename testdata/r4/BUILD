# Testdata for the parsers

package(
    default_applicable_licenses = [""],
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

# These files contain testdata for generating protos.
filegroup(
    name = "descriptors",
    srcs = glob([
        "descriptors/*",
    ]),
)

# These files are prototxt representations of sample resources provided by FHIR.
filegroup(
    name = "examples",
    srcs = glob([
        "examples/*",
    ]),
)

# These files are used to help test bigquery integration.
filegroup(
    name = "bigquery",
    srcs = glob([
        "bigquery/*.json",
    ]),
)

# These files are used to help test resource validation.
filegroup(
    name = "validation",
    srcs = glob([
        "validation/*",
    ]),
)

# These files contain extensions in both profiled and unprofiled form.
filegroup(
    name = "extensions",
    srcs = glob([
        "extensions/*",
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
    actual = "//testdata/r4/profiles:testdata",
)
