licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

load("//bazel:proto.bzl", "fhir_proto_library")
load("//bazel:protogen.bzl", "gen_fhir_definitions_and_protos")

gen_fhir_definitions_and_protos(
    name = "test",
    extensions = [
        "test_extensions.prototxt",
    ],
    package_deps = [
        "//spec:uscore_r4_package",
    ],
    package_info = "test_package_info.prototxt",
    profiles = [
        "test_profiles.prototxt",
    ],
    separate_extensions = True,
)

fhir_proto_library(
    srcs = [
        "test.proto",
        "test_extensions.proto",
    ],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto:annotations_proto",
        "//proto/r4/core:codes_proto",
        "//proto/r4/core:datatypes_proto",
        "//proto/r4/core:extensions_proto",
    ],
    proto_library_prefix = "test",
)

filegroup(
    name = "testdata",
    srcs = glob([
        "*.prototxt",
        "*.json",
    ]),
)
