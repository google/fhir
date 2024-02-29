load("//bazel:proto.bzl", "fhir_proto_library")
load("//bazel:protogen.bzl", "gen_fhir_protos")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

exports_files(["version_config.textproto"])

fhir_proto_library(
    srcs = [
        "fhirproto.proto",
    ],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
    ],
    proto_library_prefix = "fhirproto",
)

fhir_proto_library(
    srcs = [
        "fhirproto_extensions.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
    ],
    proto_library_prefix = "fhirproto_extensions",
)

fhir_proto_library(
    srcs = [
        "ml_extensions.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
    ],
    proto_library_prefix = "ml_extensions",
)

gen_fhir_protos(
    name = "uscore",
    golden_java_proto_rules = [":uscore_java_proto"],
    package = "//spec:uscore_r4_package",
)

fhir_proto_library(
    srcs = [
        "uscore.proto",
    ],
    proto_deps = [
        ":uscore_codes_proto",
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
    proto_library_prefix = "uscore",
)

fhir_proto_library(
    srcs = [
        "uscore_codes.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
    ],
    proto_library_prefix = "uscore_codes",
)

fhir_proto_library(
    srcs = ["primitive_test_suite.proto"],
    proto_deps = [
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
    ],
    proto_library_prefix = "primitive_test_suite",
)
