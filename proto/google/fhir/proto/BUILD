load("//bazel:proto.bzl", "fhir_proto_library")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

fhir_proto_library(
    name = "annotations",
    srcs = ["annotations.proto"],
    proto_deps = [
        "@com_google_protobuf//:descriptor_proto",
    ],
)

fhir_proto_library(
    name = "protogenerator_annotations",
    srcs = ["protogenerator_annotations.proto"],
    proto_deps = [
        "@com_google_protobuf//:descriptor_proto",
    ],
)

fhir_proto_library(
    name = "profile_config",
    srcs = [
        "profile_config.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
    ],
)

fhir_proto_library(
    name = "fhirpath_replacement_list",
    srcs = ["fhirpath_replacement_list.proto"],
)
