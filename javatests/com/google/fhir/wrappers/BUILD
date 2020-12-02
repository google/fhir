package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

java_test(
    name = "DateTimeWrapperTest",
    size = "small",
    srcs = ["DateTimeWrapperTest.java"],
    test_class = "com.google.fhir.wrappers.DateTimeWrapperTest",
    deps = [
        "//java/com/google/fhir/wrappers:primitive_wrappers",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "DateWrapperTest",
    size = "small",
    srcs = ["DateWrapperTest.java"],
    test_class = "com.google.fhir.wrappers.DateWrapperTest",
    deps = [
        "//java/com/google/fhir/wrappers:primitive_wrappers",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "DecimalWrapperTest",
    size = "small",
    srcs = ["DecimalWrapperTest.java"],
    test_class = "com.google.fhir.wrappers.DecimalWrapperTest",
    deps = [
        "//java/com/google/fhir/wrappers:primitive_wrappers",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

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
    name = "InstantWrapperTest",
    size = "small",
    srcs = ["InstantWrapperTest.java"],
    test_class = "com.google.fhir.wrappers.InstantWrapperTest",
    deps = [
        "//java/com/google/fhir/wrappers:primitive_wrappers",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "TimeWrapperTest",
    size = "small",
    srcs = ["TimeWrapperTest.java"],
    test_class = "com.google.fhir.wrappers.TimeWrapperTest",
    deps = [
        "//java/com/google/fhir/wrappers:primitive_wrappers",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)
