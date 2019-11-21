load("//bazel:proto.bzl", "fhir_proto_library")

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

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
        "//proto:annotations_proto",
        "//proto/r4/core:codes_proto",
        "//proto/r4/core:datatypes_proto",
    ],
    proto_library_prefix = "profile_config",
)
