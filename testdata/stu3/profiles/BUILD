load("//bazel:proto.bzl", "fhir_proto_library")
load("//bazel:protogen.bzl", "gen_fhir_definitions_and_protos")
load("@io_bazel_rules_go//proto:def.bzl", "go_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

gen_fhir_definitions_and_protos(
    name = "test",
    extensions = [
        "test_extensions.prototxt",
    ],
    package_deps = [
        "//spec:uscore_stu3_package",
    ],
    package_info = "test_package_info.prototxt",
    profiles = [
        "test_profiles.prototxt",
    ],
    separate_extensions = True,
    terminologies = ["test_terminologies.prototxt"],
)

fhir_proto_library(
    srcs = [
        "test.proto",
        "test_extensions.proto",
    ],
    proto_deps = [
        "//proto:annotations_proto",
        "//proto/stu3:codes_proto",
        "//proto/stu3:datatypes_proto",
        "//proto/stu3:extensions_proto",
        "//proto/stu3:metadatatypes_proto",
        "//proto/stu3:resources_proto",
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

go_proto_library(
    name = "go_proto",
    importpath = "github.com/google/fhir/go/testdata/stu3/profiles",
    protos = [
        "//testdata/stu3/profiles:test_proto",
    ],
    deps = [
        "//proto:go_proto",
        "//proto/stu3:go_proto",
    ],
)
