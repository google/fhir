load("//bazel:proto.bzl", "fhir_proto_library")
load("//bazel:protogen.bzl", "gen_fhir_protos")

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

fhir_proto_library(
    srcs = ["datatypes.proto"],
    proto_deps = [
        "//proto:annotations_proto",
    ],
    proto_library_prefix = "datatypes",
)

fhir_proto_library(
    srcs = ["codes.proto"],
    proto_deps = [
        ":datatypes_proto",
        "//proto:annotations_proto",
    ],
    proto_library_prefix = "codes",
)

fhir_proto_library(
    srcs = [
        "extensions.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        "//proto:annotations_proto",
    ],
    proto_library_prefix = "extensions",
)
