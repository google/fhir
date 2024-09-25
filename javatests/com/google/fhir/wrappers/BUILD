load("@rules_java//java:defs.bzl", "java_test")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

java_test(
    name = "ExtensionWrapperTest",
    size = "medium",
    srcs = ["ExtensionWrapperTest.java"],
    data = [
        "//testdata/stu3:extensions",
    ],
    test_class = "com.google.fhir.wrappers.ExtensionWrapperTest",
    deps = [
        "//java/com/google/fhir/wrappers:primitive_wrappers",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/stu3:fhirproto_extensions_java_proto",
        "//proto/google/fhir/proto/stu3:ml_extensions_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "PrimitiveWrappersTest",
    size = "medium",
    srcs = ["PrimitiveWrappersTest.java"],
    data = [
        "//testdata/primitives",
    ],
    test_class = "com.google.fhir.wrappers.PrimitiveWrappersTest",
    deps = [
        "//java/com/google/fhir/common:annotation_utils",
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:extensions",
        "//java/com/google/fhir/common:fhir_types",
        "//java/com/google/fhir/common:proto_utils",
        "//java/com/google/fhir/wrappers:primitive_wrappers",
        "//proto/google/fhir/proto/r4:primitive_test_suite_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/stu3:datatypes_java_proto",
        "//proto/google/fhir/proto/stu3:primitive_test_suite_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_code_gson_gson",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)
