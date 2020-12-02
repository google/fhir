licenses(["notice"])

java_test(
    name = "JsonFormatStu3Test",
    size = "large",
    srcs = ["JsonFormatTest.java"],
    data = [
        "//spec:stu3",
        "//testdata/stu3:bigquery",
        "//testdata/stu3:examples",
        "//testdata/stu3:profiles",
    ],
    shard_count = 10,
    test_class = "com.google.fhir.stu3.JsonFormatTest",
    deps = [
        "//java/com/google/fhir/testing:json_format_test_base",
        "//proto/google/fhir/proto/stu3:resources_java_proto",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "ValidationStu3Test",
    size = "medium",
    srcs = ["ValidationTest.java"],
    data = [
        "//testdata/stu3:validation",
    ],
    test_class = "com.google.fhir.stu3.ValidationTest",
    deps = [
        "//java/com/google/fhir/common:json_format",
        "//java/com/google/fhir/testing:validation_test_base",
        "//proto/google/fhir/proto/stu3:codes_java_proto",
        "//proto/google/fhir/proto/stu3:datatypes_java_proto",
        "//proto/google/fhir/proto/stu3:fhirproto_extensions_java_proto",
        "//proto/google/fhir/proto/stu3:uscore_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:junit_junit",
    ],
)
