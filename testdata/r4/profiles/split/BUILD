load("//bazel:proto.bzl", "fhir_proto_library")
load("//bazel:protogen.bzl", "gen_fhir_definitions_and_protos")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

gen_fhir_definitions_and_protos(
    name = "split_test",
    extensions = [
        "//testdata/r4/profiles:test_extensions.prototxt",
    ],
    package_deps = [
        "//spec:uscore_r4_package",
    ],
    package_info = "package_info.prototxt",
    package_json = "package.json",
    profiles = [
        "//testdata/r4/profiles:test_profiles.prototxt",
    ],
    terminologies = [
        "//testdata/r4/profiles:test_terminologies.prototxt",
    ],
)

fhir_proto_library(
    srcs = glob(["*.proto"]),
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
    ],
    proto_library_prefix = "all",
)

# Export config files for use in "retagging split" version of test profile.
exports_files(
    [
        "package_info.prototxt",
        "package.json",
    ],
    visibility = ["//testdata/r4/profiles:__subpackages__"],
)
