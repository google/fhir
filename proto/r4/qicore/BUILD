load("//bazel:proto.bzl", "fhir_proto_library")
load("//bazel:protogen.bzl", "gen_fhir_protos")

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

fhir_proto_library(
    srcs = [
        "codes.proto",
        "qicore.proto",
    ],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto:annotations_proto",
        "//proto/r4:uscore_codes_proto",
        "//proto/r4:uscore_proto",
        "//proto/r4/core:codes_proto",
        "//proto/r4/core:datatypes_proto",
        "//proto/r4/core:extensions_proto",
        "//proto/r4/core:valuesets_proto",
    ],
    proto_library_prefix = "qicore",
)

gen_fhir_protos(
    name = "qicore",
    additional_proto_imports = [
        "proto/r4/uscore_codes.proto",
        "proto/r4/uscore.proto",
        "proto/r4/qicore/codes.proto",
    ],
    package = "//spec:qicore_3_3_0_package",
    package_deps = [
        "//spec:uscore_r4_package",
    ],
)
