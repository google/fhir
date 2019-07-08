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
        "resources.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        "//proto:annotations_proto",
    ],
    proto_library_prefix = "resources",
)

fhir_proto_library(
    srcs = [
        "extensions.proto",
        "google_extensions.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        "//proto:annotations_proto",
    ],
    proto_library_prefix = "extensions",
)

gen_fhir_protos(
    name = "uscore",
    additional_proto_imports = [
        "proto/r4/uscore_codes.proto",
    ],
    package = "//spec:uscore-r4-package",
)

fhir_proto_library(
    srcs = [
        "uscore.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        ":resources_proto",
        ":uscore_codes_proto",
        "//proto:annotations_proto",
    ],
    proto_library_prefix = "uscore",
)

fhir_proto_library(
    srcs = [
        "uscore_codes.proto",
    ],
    proto_deps = [
        ":datatypes_proto",
        "//proto:annotations_proto",
    ],
    proto_library_prefix = "uscore_codes",
)
