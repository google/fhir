load("//bazel:proto.bzl", "fhir_proto_library")
load("//bazel:protogen.bzl", "gen_fhir_protos")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

exports_files(["version_config.textproto"])

fhir_proto_library(
    srcs = ["datatypes.proto"],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "datatypes",
)

fhir_proto_library(
    srcs = ["codes.proto"],
    proto_deps = [
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "codes",
)

fhir_proto_library(
    srcs = ["metadatatypes.proto"],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "metadatatypes",
)

fhir_proto_library(
    srcs = [
        "extensions.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "extensions",
)

fhir_proto_library(
    srcs = [
        "fhirproto_extensions.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "fhirproto_extensions",
)

fhir_proto_library(
    srcs = [
        "ml_extensions.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "ml_extensions",
)

fhir_proto_library(
    srcs = [
        "resources.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        ":metadatatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
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
        ":metadatatypes_proto",
        ":resources_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "profiles",
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
    proto_library_prefix = "uscore",
)

fhir_proto_library(
    srcs = [
        "uscore_codes.proto",
    ],
    proto_deps = [
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "uscore_codes",
)

fhir_proto_library(
    srcs = ["primitive_test_suite.proto"],
    proto_deps = [
        "//proto/google/fhir/proto/stu3:datatypes_proto",
    ],
    proto_library_prefix = "primitive_test_suite",
)

# We also make the .proto files readable for testing.
filegroup(
    name = "testdata",
    srcs = glob([
        "*.proto",
    ]),
)
