load("//bazel:proto.bzl", "fhir_proto_library")
load("//bazel:protogen.bzl", "gen_fhir_definitions_and_protos")

licenses(["notice"])

package(
    default_applicable_licenses = [""],
    
    default_visibility = ["//visibility:public"],
)

gen_fhir_definitions_and_protos(
    name = "test",
    # Regression test disabled because generation of STU3 is frozen.
    disable_test = True,
    extensions = [
        "test_extensions.prototxt",
    ],
    package_deps = [
        "//spec:uscore_stu3_package",
    ],
    package_info = "test_package_info.prototxt",
    package_json = "package.json",
    profiles = [
        "test_profiles.prototxt",
    ],
    terminologies = ["test_terminologies.prototxt"],
)

fhir_proto_library(
    srcs = [
        "test.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/stu3:codes_proto",
        "//proto/google/fhir/proto/stu3:datatypes_proto",
        "//proto/google/fhir/proto/stu3:metadatatypes_proto",
        "//testdata/stu3/profiles:test_extensions_proto",
    ],
    proto_library_prefix = "test",
)

fhir_proto_library(
    srcs = [
        "test_extensions.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/stu3:codes_proto",
        "//proto/google/fhir/proto/stu3:datatypes_proto",
    ],
    proto_library_prefix = "test_extensions",
)

filegroup(
    name = "testdata",
    srcs = glob([
        "*.prototxt",
        "*.json",
    ]),
)
