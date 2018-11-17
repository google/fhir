package(default_visibility = ["//visibility:public"])

licenses(["notice"])

exports_files(["version_config.textproto"])

load("//bazel:proto.bzl", "fhir_proto_library")
load("@io_bazel_rules_go//proto:def.bzl", "go_proto_library")
load("@io_bazel_rules_go//go:def.bzl", "go_library")

# stu3_proto aggregates all of the proto files for the go_proto_library as Bazel
# conforms to Go convention of having only one package per directory. To use
# Gazelle for automatic Go build-file creation, we need to tag these rules with
# "keep" comments as a signal to Gazelle not to modify them.
# keep
proto_library(
    name = "stu3_proto",
    visibility = ["//visibility:private"],
    srcs = glob([
        "*.proto",
    ]),
    deps = [
        "@protobuf_archive//:descriptor_proto",
    ],
)

# keep
go_proto_library(
    name = "stu3_go_proto",
    importpath = "github.com/google/fhir/proto/stu3",
    proto = ":stu3_proto",
)

# keep
go_library(
    name = "go_default_library",
    srcs = ["json.go"],
    embed = [":stu3_go_proto"],
    importpath = "github.com/google/fhir/proto/stu3",
    deps = [
        "@com_github_golang_protobuf//descriptor:go_default_library_gen",
        "@com_github_golang_protobuf//jsonpb:go_default_library_gen",
    ],
)

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

fhir_proto_library(
    srcs = [
        "uscore.proto",
    ],
    proto_deps = [
        ":annotations_proto",
        ":codes_proto",
        ":datatypes_proto",
        ":extensions_proto",
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
    proto_library_prefix = "profile_config",
)

# We also make the .proto files readable for testing.
filegroup(
    name = "testdata",
    srcs = glob([
        "*.proto",
    ]),
)
