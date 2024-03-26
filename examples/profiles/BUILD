load("//bazel:proto.bzl", "fhir_proto_library")
load("//bazel:protogen.bzl", "gen_fhir_definitions_and_protos")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

gen_fhir_definitions_and_protos(
    name = "demo",
    additional_proto_imports = [
        "proto/google/fhir/proto/r4/uscore.proto",
        "proto/google/fhir/proto/r4/uscore_codes.proto",
    ],
    extensions = [],
    package_deps = [
        "//spec:uscore_r4_package",
    ],
    package_info = "demo_package_info.prototxt",
    profiles = ["demo_profiles.prototxt"],
)

fhir_proto_library(
    srcs = [
        "demo.proto",
    ],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4:uscore_codes_proto",
        "//proto/google/fhir/proto/r4:uscore_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:extensions_proto",
    ],
    proto_library_prefix = "demo",
)
