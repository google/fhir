load("//bazel:proto.bzl", "fhir_proto_library")
load("//bazel:protogen.bzl", "gen_fhir_definitions_and_protos")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

gen_fhir_definitions_and_protos(
    name = "test",
    extensions = [
        "test_extensions.prototxt",
    ],
    golden_java_proto_rules = [":test_java_proto"],
    package_deps = [
        "//spec:uscore_r4_package",
    ],
    package_info = "test_package_info.prototxt",
    package_json = "test_package.json",
    profiles = [
        "test_profiles.prototxt",
    ],
    terminologies = [
        "test_terminologies.prototxt",
    ],
)

fhir_proto_library(
    name = "test",  # buildifier: disable=duplicated-name
    srcs = [
        "test.proto",
    ],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//testdata/r4/profiles:test_extensions_proto",
    ],
)

fhir_proto_library(
    name = "test_extensions",
    srcs = [
        "test_extensions.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//testdata/r4/profiles:test_codes_proto",
    ],
)

fhir_proto_library(
    name = "test_codes",
    srcs = [
        "test_codes.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
    ],
)

filegroup(
    name = "testdata",
    srcs = glob([
        "*.prototxt",
        "*.json",
    ]),
)

# Export config files for use in "split" version of test profile.
exports_files(
    [
        "test_package_info.prototxt",
        "test_extensions.prototxt",
        "test_profiles.prototxt",
        "test_terminologies.prototxt",
        "test_package.json",
    ],
    visibility = ["//testdata/r4/profiles:__subpackages__"],
)
