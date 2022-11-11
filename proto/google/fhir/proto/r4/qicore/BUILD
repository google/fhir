load("//bazel:proto.bzl", "fhir_proto_library")
load("//bazel:protogen.bzl", "gen_fhir_protos")

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

fhir_proto_library(
    srcs = [
        "qicore.proto",
    ],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4:uscore_codes_proto",
        "//proto/google/fhir/proto/r4:uscore_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:extensions_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
        "//proto/google/fhir/proto/r4/qicore:codes_proto",
    ],
    proto_library_prefix = "qicore",
)

fhir_proto_library(
    srcs = [
        "codes.proto",
    ],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "codes",
)

gen_fhir_protos(
    name = "qicore",
    additional_proto_imports = [
        "proto/google/fhir/proto/r4/uscore_codes.proto",
        "proto/google/fhir/proto/r4/uscore.proto",
        "proto/google/fhir/proto/r4/qicore/codes.proto",
    ],
    # This is an old version of the QI core that has a dependency on a
    # US-core resource that doesn't exist anymore.
    # TODO(b/152362881): Bump this up to QICore 4.0.0 and re-enable
    disable_test = True,
    package = "//spec:qicore_3_3_0_package",
    package_deps = [
        "//spec:uscore_r4_package",
    ],
)
