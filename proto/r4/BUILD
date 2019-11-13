load("//bazel:proto.bzl", "fhir_proto_library")
load("//bazel:protogen.bzl", "gen_fhir_protos")

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

fhir_proto_library(
    srcs = [
        "google_extensions.proto",
    ],
    proto_deps = [
        "//proto:annotations_proto",
        "//proto/r4/core:codes_proto",
        "//proto/r4/core:datatypes_proto",
    ],
    proto_library_prefix = "google_extensions",
)

gen_fhir_protos(
    name = "uscore",
    additional_proto_imports = [
        "proto/r4/uscore_codes.proto",
    ],
    package = "//spec:uscore_r4_package",
)

fhir_proto_library(
    srcs = [
        "uscore.proto",
    ],
    proto_deps = [
        ":uscore_codes_proto",
        "@com_google_protobuf//:any_proto",
        "//proto:annotations_proto",
        "//proto/r4/core:codes_proto",
        "//proto/r4/core:datatypes_proto",
        "//proto/r4/core:valuesets_proto",
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
