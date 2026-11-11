load("//bazel:proto.bzl", "fhir_proto_library")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

fhir_proto_library(
    name = "fhirproto_extensions",
    srcs = [
        "fhirproto_extensions.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r5/core:codes_proto",
        "//proto/google/fhir/proto/r5/core:datatypes_proto",
    ],
)

fhir_proto_library(
    name = "ml_extensions",
    srcs = [
        "ml_extensions.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r5/core:codes_proto",
        "//proto/google/fhir/proto/r5/core:datatypes_proto",
    ],
)

fhir_proto_library(
    name = "primitive_test_suite",
    srcs = ["primitive_test_suite.proto"],
    proto_deps = [
        "//proto/google/fhir/proto/r5/core:datatypes_proto",
    ],
)
