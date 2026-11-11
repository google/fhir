load("//bazel:proto.bzl", "fhir_proto_library")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

fhir_proto_library(
    name = "datatypes",
    srcs = ["datatypes.proto"],
    proto_deps = [
        ":codes_proto",
        ":valuesets_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
)

fhir_proto_library(
    name = "codes",
    srcs = ["codes.proto"],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
    ],
)

fhir_proto_library(
    name = "valuesets",
    srcs = ["valuesets.proto"],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
    ],
)

# We also make the .proto files readable for testing.
filegroup(
    name = "testdata",
    srcs = glob([
        "*.proto",
    ]),
)
