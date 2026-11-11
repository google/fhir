load("//bazel:proto.bzl", "fhir_proto_library")
load("//bazel:protogen.bzl", "gen_fhir_protos")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

exports_files(["version_config.textproto"])

fhir_proto_library(
    name = "datatypes",
    srcs = ["datatypes.proto"],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
    ],
)

fhir_proto_library(
    name = "codes",
    srcs = ["codes.proto"],
    proto_deps = [
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
)

fhir_proto_library(
    name = "metadatatypes",
    srcs = ["metadatatypes.proto"],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
)

fhir_proto_library(
    name = "extensions",
    srcs = [
        "extensions.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
)

fhir_proto_library(
    name = "fhirproto_extensions",
    srcs = [
        "fhirproto_extensions.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
)

fhir_proto_library(
    name = "ml_extensions",
    srcs = [
        "ml_extensions.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
)

fhir_proto_library(
    name = "resources",
    srcs = [
        "resources.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        ":metadatatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
)

fhir_proto_library(
    name = "profiles",
    srcs = [
        "profiles.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        ":extensions_proto",
        ":metadatatypes_proto",
        ":resources_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
)

gen_fhir_protos(
    name = "uscore",
    additional_proto_imports = [
        "proto/google/fhir/proto/stu3/uscore_codes.proto",
    ],
    # Regression test disabled because generation of STU3 is frozen.
    disable_test = True,
    package = "//spec:uscore_stu3_package",
)

fhir_proto_library(
    name = "uscore",  # buildifier: disable=duplicated-name
    srcs = [
        "uscore.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        ":metadatatypes_proto",
        ":resources_proto",
        ":uscore_codes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
)

fhir_proto_library(
    name = "uscore_codes",
    srcs = [
        "uscore_codes.proto",
    ],
    proto_deps = [
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
)

fhir_proto_library(
    name = "primitive_test_suite",
    srcs = ["primitive_test_suite.proto"],
    proto_deps = [
        "//proto/google/fhir/proto/stu3:datatypes_proto",
    ],
)

# We also make the .proto files readable for testing.
filegroup(
    name = "testdata",
    srcs = glob([
        "*.proto",
    ]),
)
