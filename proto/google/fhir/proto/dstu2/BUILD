load("//bazel:proto.bzl", "fhir_proto_library")

package(
    
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

exports_files(["version_config.textproto"])

fhir_proto_library(
    srcs = ["datatypes.proto"],
    proto_deps = [
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "datatypes",
)

fhir_proto_library(
    srcs = ["codes.proto"],
    proto_deps = [
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "codes",
)

fhir_proto_library(
    srcs = [
        "fhirproto_extensions.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "fhirproto_extensions",
)

fhir_proto_library(
    srcs = ["metadatatypes.proto"],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "metadatatypes",
)

fhir_proto_library(
    srcs = [
        "resources.proto",
    ],
    proto_deps = [
        ":codes_proto",
        ":datatypes_proto",
        ":metadatatypes_proto",
        "//proto/google/fhir/proto:annotations_proto",
    ],
    proto_library_prefix = "resources",
)
