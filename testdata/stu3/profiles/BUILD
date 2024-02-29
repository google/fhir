load("//bazel:proto.bzl", "fhir_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

fhir_proto_library(
    srcs = [
        "test.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/stu3:codes_proto",
        "//proto/google/fhir/proto/stu3:datatypes_proto",
        "//proto/google/fhir/proto/stu3:metadatatypes_proto",
        "//testdata/stu3/profiles:test_extensions_proto",
    ],
    proto_library_prefix = "test",
)

fhir_proto_library(
    srcs = [
        "test_extensions.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/stu3:codes_proto",
        "//proto/google/fhir/proto/stu3:datatypes_proto",
    ],
    proto_library_prefix = "test_extensions",
)

filegroup(
    name = "testdata",
    srcs = glob([
        "*.prototxt",
        "*.json",
    ]),
)
