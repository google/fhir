load("//bazel:proto.bzl", "fhir_proto_library")

package(default_visibility = ["//:__subpackages__"])

licenses(["notice"])

fhir_proto_library(
    name = "generic_proto",
    srcs = ["generic_proto.proto"],
)

filegroup(
    name = "resource_utils_testdata",
    srcs = glob([
        "resource_utils-*",
    ]),
)
