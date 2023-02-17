load("//bazel:proto.bzl", "fhir_proto_library")

package(
    default_applicable_licenses = [""],
    default_visibility = ["//:__subpackages__"],
)

licenses(["notice"])

fhir_proto_library(
    srcs = ["generic_proto.proto"],
    proto_library_prefix = "generic_proto",
)

filegroup(
    name = "resource_utils_testdata",
    srcs = glob([
        "resource_utils-*",
    ]),
)
