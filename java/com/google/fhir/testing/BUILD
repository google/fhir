package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

java_library(
    name = "json_format_test_base",
    testonly = 1,
    srcs = ["JsonFormatTestBase.java"],
    deps = [
        "//java/com/google/fhir/common:json_format",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_code_gson_gson",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_library(
    name = "validation_test_base",
    testonly = 1,
    srcs = ["ValidationTestBase.java"],
    deps = [
        "//java/com/google/fhir/common:json_format",
        "//java/com/google/fhir/wrappers:primitive_wrappers",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)
