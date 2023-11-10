# Testdata for the parsers

package(default_visibility = ["//visibility:public"])

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
        "examples/*.prototxt",
    ]),
)

filegroup(
    name = "validation",
    srcs = glob([
        "validation/*",
    ]),
)
