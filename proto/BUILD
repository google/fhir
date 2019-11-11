package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

load("//bazel:proto.bzl", "fhir_proto_library")

fhir_proto_library(
    srcs = ["annotations.proto"],
    proto_deps = [
        "@com_google_protobuf//:descriptor_proto",
    ],
    proto_library_prefix = "annotations",
)

fhir_proto_library(
    srcs = ["protogenerator_annotations.proto"],
    proto_deps = [
        "@com_google_protobuf//:descriptor_proto",
    ],
    proto_library_prefix = "protogenerator_annotations",
)
