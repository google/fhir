load("//bazel:proto.bzl", "fhir_resource_rules")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

fhir_resource_rules(
    resource_files = glob(["*.proto"]),
    deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r5/core:codes_proto",
        "//proto/google/fhir/proto/r5/core:datatypes_proto",
        "//proto/google/fhir/proto/r5/core:valuesets_proto",
    ],
)
