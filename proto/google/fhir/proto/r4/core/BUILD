load("//bazel:proto.bzl", "fhir_proto_library")

package(
    allow_incoming_cc_edges_to_proto = True,
    
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

fhir_proto_library(
    srcs = ["datatypes.proto"],
    proto_deps = [
        ":codes_proto",
        ":valuesets_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "datatypes",
)

fhir_proto_library(
    srcs = ["codes.proto"],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "codes",
)

fhir_proto_library(
    srcs = ["valuesets.proto"],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "valuesets",
)

fhir_proto_library(
    srcs = [
        "extensions.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        ":valuesets_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "extensions",
)

# We also make the .proto files readable for testing.
filegroup(
    name = "testdata",
    srcs = glob([
        "*.proto",
    ]),
)
