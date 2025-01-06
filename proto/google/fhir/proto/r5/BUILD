load("//bazel:proto.bzl", "fhir_proto_library")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

fhir_proto_library(
    srcs = [
        "fhirproto_extensions.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r5/core:codes_proto",
        "//proto/google/fhir/proto/r5/core:datatypes_proto",
    ],
    proto_library_prefix = "fhirproto_extensions",
)

fhir_proto_library(
    srcs = [
        "ml_extensions.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r5/core:codes_proto",
        "//proto/google/fhir/proto/r5/core:datatypes_proto",
    ],
    proto_library_prefix = "ml_extensions",
)

fhir_proto_library(
    srcs = ["primitive_test_suite.proto"],
    proto_deps = [
        "//proto/google/fhir/proto/r5/core:datatypes_proto",
    ],
    proto_library_prefix = "primitive_test_suite",
)
