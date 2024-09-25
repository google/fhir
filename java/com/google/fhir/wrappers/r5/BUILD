load("@rules_java//java:defs.bzl", "java_library")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

java_library(
    name = "primitive_wrappers",
    srcs = glob(["*.java"]),
    deps = [
        "//java/com/google/fhir/common:annotation_utils",
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:fhir_types",
        "//java/com/google/fhir/common:proto_utils",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto/r5:fhirproto_extensions_java_proto",
        "//proto/google/fhir/proto/r5/core:datatypes_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_code_gson_gson",
        "@maven//:com_google_errorprone_error_prone_annotations",
        "@maven//:com_google_guava_guava",
    ],
)
