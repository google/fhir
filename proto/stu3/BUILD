package(default_visibility = ["//visibility:public"])

licenses(["notice"])

exports_files(["version_config.textproto"])

load("//bazel:proto.bzl", "fhir_proto_library")
load("//bazel:protogen.bzl", "gen_fhir_protos")

fhir_proto_library(
    srcs = ["annotations.proto"],
    proto_deps = [
        "@protobuf_archive//:descriptor_proto",
    ],
    proto_library_prefix = "annotations",
)

fhir_proto_library(
    srcs = ["datatypes.proto"],
    proto_deps = [
        ":annotations_proto",
    ],
    proto_library_prefix = "datatypes",
)

fhir_proto_library(
    srcs = ["codes.proto"],
    proto_deps = [
        ":annotations_proto",
        ":datatypes_proto",
    ],
    proto_library_prefix = "codes",
)

fhir_proto_library(
    srcs = ["metadatatypes.proto"],
    proto_deps = [
        ":annotations_proto",
        ":codes_proto",
        ":datatypes_proto",
    ],
    proto_library_prefix = "metadatatypes",
)

fhir_proto_library(
    srcs = [
        "extensions.proto",
        "google_extensions.proto",
    ],
    proto_deps = [
        ":annotations_proto",
        ":codes_proto",
        ":datatypes_proto",
    ],
    proto_library_prefix = "extensions",
)

fhir_proto_library(
    srcs = [
        "resources.proto",
    ],
    proto_deps = [
        ":annotations_proto",
        ":codes_proto",
        ":datatypes_proto",
        ":metadatatypes_proto",
    ],
    proto_library_prefix = "resources",
)

fhir_proto_library(
    srcs = [
        "profiles.proto",
    ],
    proto_deps = [
        ":annotations_proto",
        ":codes_proto",
        ":datatypes_proto",
        ":extensions_proto",
        ":metadatatypes_proto",
        ":resources_proto",
    ],
    proto_library_prefix = "profiles",
)

gen_fhir_protos(
    name = "uscore",
    add_apache_license = True,
    additional_proto_imports = [
        "proto/stu3/uscore_codes.proto",
    ],
    package = "//spec:uscore-package",
)

fhir_proto_library(
    srcs = [
        "uscore.proto",
    ],
    proto_deps = [
        ":annotations_proto",
        ":codes_proto",
        ":datatypes_proto",
        ":metadatatypes_proto",
        ":resources_proto",
        ":uscore_codes_proto",
    ],
    proto_library_prefix = "uscore",
)

fhir_proto_library(
    srcs = [
        "uscore_codes.proto",
    ],
    proto_deps = [
        ":annotations_proto",
        ":datatypes_proto",
    ],
    proto_library_prefix = "uscore_codes",
)

fhir_proto_library(
    srcs = [
        "version_config.proto",
    ],
    proto_library_prefix = "version_config",
)

fhir_proto_library(
    srcs = [
        "profile_config.proto",
    ],
    proto_deps = [
        ":codes_proto",
    ],
    proto_library_prefix = "profile_config",
)

# We also make the .proto files readable for testing.
filegroup(
    name = "testdata",
    srcs = glob([
        "*.proto",
    ]),
)
