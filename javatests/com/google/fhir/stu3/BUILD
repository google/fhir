load("@rules_java//java:defs.bzl", "java_test")

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
