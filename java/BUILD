# Tools for working with FHIR protocol buffers in java.
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

# TODO: remove dependency on uscore once we have json
# definitions of codes.
java_library(
    name = "stu3",
    srcs = glob(["src/main/java/com/google/fhir/stu3/*.java"]),
    deps = [
        "//external:gson",
        "//external:guava",
        "//proto/stu3:annotations_java_proto",
        "//proto/stu3:codes_java_proto",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:extensions_java_proto",
        "//proto/stu3:metadatatypes_java_proto",
        "//proto/stu3:profile_config_java_proto",
        "//proto/stu3:resources_java_proto",
        "//proto/stu3:uscore_codes_java_proto",
        "@com_google_apis_google_api_services_bigquery//jar",
        "@com_google_http_client_google_http_client//jar",
        "@com_google_protobuf//:protobuf_java",
    ],
)

java_library(
    name = "examples",
    srcs = glob(["src/main/java/com/google/fhir/examples/*.java"]),
    deps = [
        ":stu3",
        "//external:guava",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:extensions_java_proto",
        "//proto/stu3:profile_config_java_proto",
        "//proto/stu3:resources_java_proto",
        "@com_beust_jcommander//jar",
        "@com_google_apis_google_api_services_bigquery//jar",
        "@com_google_http_client_google_http_client//jar",
        "@com_google_http_client_google_http_client_gson//jar",
        "@com_google_protobuf//:protobuf_java",
        "@com_google_protobuf//:protobuf_java_util",
    ],
)

java_binary(
    name = "ConvertNdJsonForBigQuery",
    main_class = "com.google.fhir.examples.ConvertNdJsonForBigQueryMain",
    runtime_deps = [":examples"],
)

java_binary(
    name = "JsonToProto",
    main_class = "com.google.fhir.examples.JsonToProtoMain",
    runtime_deps = [":examples"],
)

java_binary(
    name = "ProtoGenerator",
    main_class = "com.google.fhir.examples.ProtoGeneratorMain",
    runtime_deps = [":examples"],
)

java_binary(
    name = "SplitBundle",
    main_class = "com.google.fhir.examples.SplitBundleMain",
    runtime_deps = [":examples"],
)

java_test(
    name = "AnnotationUtilsTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/stu3/AnnotationUtilsTest.java"],
    test_class = "com.google.fhir.stu3.AnnotationUtilsTest",
    deps = [
        ":stu3",
        "//proto/stu3:annotations_java_proto",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:resources_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@com_google_truth_truth//jar",
        "@junit_junit//jar",
    ],
)

java_test(
    name = "BigQuerySchemaTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/stu3/BigQuerySchemaTest.java"],
    data = [
        "//testdata/stu3:bigquery",
        "//testdata/stu3:examples",
    ],
    test_class = "com.google.fhir.stu3.BigQuerySchemaTest",
    deps = [
        ":stu3",
        "//external:guava",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:resources_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_apis_google_api_services_bigquery//jar",
        "@com_google_http_client_google_http_client//jar",
        "@com_google_http_client_google_http_client_gson//jar",
        "@com_google_protobuf//:protobuf_java",
        "@com_google_protobuf//:protobuf_java_util",
        "@com_google_truth_truth//jar",
        "@junit_junit//jar",
    ],
)

java_test(
    name = "DateTimeWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/stu3/DateTimeWrapperTest.java"],
    test_class = "com.google.fhir.stu3.DateTimeWrapperTest",
    deps = [
        ":stu3",
        "//proto/stu3:datatypes_java_proto",
        "@com_google_truth_truth//jar",
        "@junit_junit//jar",
    ],
)

java_test(
    name = "DateWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/stu3/DateWrapperTest.java"],
    test_class = "com.google.fhir.stu3.DateWrapperTest",
    deps = [
        ":stu3",
        "//proto/stu3:datatypes_java_proto",
        "@com_google_truth_truth//jar",
        "@junit_junit//jar",
    ],
)

java_test(
    name = "DecimalWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/stu3/DecimalWrapperTest.java"],
    test_class = "com.google.fhir.stu3.DecimalWrapperTest",
    deps = [
        ":stu3",
        "//proto/stu3:datatypes_java_proto",
        "@com_google_truth_truth//jar",
        "@junit_junit//jar",
    ],
)

java_test(
    name = "ExtensionWrapperTest",
    size = "medium",
    srcs = ["src/test/java/com/google/fhir/stu3/ExtensionWrapperTest.java"],
    data = [
        "//testdata/stu3:google",
    ],
    test_class = "com.google.fhir.stu3.ExtensionWrapperTest",
    deps = [
        ":stu3",
        "//external:guava",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:extensions_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@com_google_truth_truth//jar",
        "@junit_junit//jar",
    ],
)

java_test(
    name = "InstantWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/stu3/InstantWrapperTest.java"],
    test_class = "com.google.fhir.stu3.InstantWrapperTest",
    deps = [
        ":stu3",
        "//proto/stu3:datatypes_java_proto",
        "@com_google_truth_truth//jar",
        "@junit_junit//jar",
    ],
)

java_test(
    name = "JsonFormatTest",
    size = "medium",
    srcs = ["src/test/java/com/google/fhir/stu3/JsonFormatTest.java"],
    data = [
        "//testdata/stu3:bigquery",
        "//testdata/stu3:examples",
    ],
    test_class = "com.google.fhir.stu3.JsonFormatTest",
    deps = [
        ":stu3",
        "//external:gson",
        "//external:guava",
        "//proto/stu3:resources_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@com_google_truth_truth//jar",
        "@junit_junit//jar",
    ],
)

java_test(
    name = "ProtoFilePrinterTest",
    size = "medium",
    srcs = ["src/test/java/com/google/fhir/stu3/ProtoFilePrinterTest.java"],
    data = [
        "//proto/stu3:testdata",
        "//testdata/stu3:descriptors",
        "//testdata/stu3:fhir_structure_definitions",
    ],
    test_class = "com.google.fhir.stu3.ProtoFilePrinterTest",
    deps = [
        ":stu3",
        "//external:gson",
        "//external:guava",
        "//proto/stu3:annotations_java_proto",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:metadatatypes_java_proto",
        "//proto/stu3:profile_config_java_proto",
        "//proto/stu3:resources_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@com_google_truth_truth//jar",
        "@junit_junit//jar",
    ],
)

java_test(
    name = "ProtoGeneratorTest",
    size = "medium",
    srcs = ["src/test/java/com/google/fhir/stu3/ProtoGeneratorTest.java"],
    data = [
        "//testdata/stu3:descriptors",
        "//testdata/stu3:fhir_structure_definitions",
        "//testdata/stu3:google",
        "//testdata/stu3/uscore:uscore_structure_definitions",
    ],
    test_class = "com.google.fhir.stu3.ProtoGeneratorTest",
    deps = [
        ":stu3",
        "//external:gson",
        "//external:guava",
        "//proto/stu3:annotations_java_proto",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:profile_config_java_proto",
        "//proto/stu3:resources_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@com_google_truth_truth//jar",
        "@junit_junit//jar",
    ],
)

java_test(
    name = "TimeWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/stu3/TimeWrapperTest.java"],
    test_class = "com.google.fhir.stu3.TimeWrapperTest",
    deps = [
        ":stu3",
        "//proto/stu3:datatypes_java_proto",
        "@com_google_truth_truth//jar",
        "@junit_junit//jar",
    ],
)

java_test(
    name = "ValidationTest",
    size = "medium",
    srcs = ["src/test/java/com/google/fhir/stu3/ValidationTest.java"],
    data = [
        "//testdata/stu3:validation",
    ],
    test_class = "com.google.fhir.stu3.ValidationTest",
    deps = [
        ":stu3",
        "//external:guava",
        "//proto/stu3:datatypes_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@com_google_truth_truth//jar",
        "@junit_junit//jar",
    ],
)
