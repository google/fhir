package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

load("//bazel:proto.bzl", "fhir_proto_library")

fhir_proto_library(
    srcs = ["annotations.proto"],
    proto_deps = [
        "@protobuf_archive//:descriptor_proto",
    ],
    proto_library_prefix = "annotations",
)

fhir_proto_library(
    srcs = ["protogenerator_annotations.proto"],
    proto_deps = [
        "@protobuf_archive//:descriptor_proto",
    ],
    proto_library_prefix = "protogenerator_annotations",
)
