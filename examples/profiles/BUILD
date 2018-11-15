package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

load("//bazel:protogen.bzl", "gen_fhir_definitions_and_protos")
load("//bazel:proto.bzl", "fhir_proto_library")

gen_fhir_definitions_and_protos(
    name = "demo",
    additional_proto_imports = [
        "proto/stu3/uscore.proto",
        "proto/stu3/uscore_codes.proto",
    ],
    extensions = [],
    package_deps = [
        "//spec:uscore-package",
    ],
    package_info = "demo_package_info.prototxt",
    profiles = ["demo_profiles.prototxt"],
)

fhir_proto_library(
    srcs = [
        "demo.proto",
    ],
    proto_deps = [
        "//proto/stu3:annotations_proto",
        "//proto/stu3:codes_proto",
        "//proto/stu3:datatypes_proto",
        "//proto/stu3:extensions_proto",
        "//proto/stu3:metadatatypes_proto",
        "//proto/stu3:resources_proto",
        "//proto/stu3:uscore_codes_proto",
        "//proto/stu3:uscore_proto",
    ],
    proto_library_prefix = "demo",
)
