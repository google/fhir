load("//bazel:protogen.bzl", "gen_fhir_definitions_and_protos")

licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

gen_fhir_definitions_and_protos(
    name = "split_test",
    extensions = [
        "//testdata/r4/profiles:test_extensions.prototxt",
    ],
    package_deps = [
        "//spec:uscore_r4_package",
    ],
    package_info = "package_info.prototxt",
    profiles = [
        "//testdata/r4/profiles:test_profiles.prototxt",
    ],
    terminologies = [
        "//testdata/r4/profiles:test_terminologies.prototxt",
    ],
)

proto_library(
    name = "all_proto",
    srcs = glob(["*.proto"]),
    deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
    ],
)
