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
        "profiles.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        ":extensions_proto",
        ":resources_proto",
        "//proto:annotations_proto",
    ],
    proto_library_prefix = "profiles",
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

fhir_proto_library(
    srcs = [
        "google_extensions_core.proto",
    ],
    proto_deps = [
        "//proto:annotations_proto",
        "//proto/r4/core:codes_proto",
        "//proto/r4/core:datatypes_proto",
    ],
    proto_library_prefix = "google_extensions_core",
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
        ":uscore_codes_proto",
        "@protobuf_archive//:any_proto",
        "//proto:annotations_proto",
        "//proto/r4/core:codes_proto",
        "//proto/r4/core:datatypes_proto",
    ],
    proto_library_prefix = "uscore",
)

fhir_proto_library(
    srcs = [
        "uscore_codes.proto",
    ],
    proto_deps = [
        "//proto:annotations_proto",
        "//proto/r4/core:datatypes_proto",
    ],
    proto_library_prefix = "uscore_codes",
)
