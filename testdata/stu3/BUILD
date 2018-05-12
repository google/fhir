# Testdata for the parsers

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

# These files are sample resources from the FHIR spec.
filegroup(
    name = "examples",
    srcs = glob([
        "examples/*.json",
        "examples/*.prototxt",
    ]),
)

# These files contain testdata for generating and parsing extensions.
filegroup(
    name = "extensions",
    srcs = glob([
        "extensions/*.json",
        "extensions/*.descriptor.prototxt",
        "extensions/*.extension.prototxt",
        "extensions/*.message.prototxt",
    ]),
)

# These files contain StructureDefinitions used to generate the protos.
filegroup(
    name = "structure_definitions",
    srcs = glob([
        "structure_definitions/*.profile.json",
        "structure_definitions/*.descriptor.prototxt",
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
