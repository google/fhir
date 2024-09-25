load("@rules_java//java:defs.bzl", "java_test")

licenses(["notice"])

java_test(
    name = "ProtoGenTransformerTest",
    srcs = ["ProtoGenTransformerTest.java"],
    data = [
        "//spec:fhir_stu3_package.zip",
        "//testdata/protogentransformer:json",
    ],
    test_class = "com.google.fhir.common.ProtoGenTransformerTest",
    deps = [
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:json_format",
        "//java/com/google/fhir/common:proto_gen_transformer",
        "//java/com/google/fhir/common:resource_validator",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "ResourceValidatorTest",
    size = "small",
    srcs = ["ResourceValidatorR4Test.java"],
    data = [
        "//testdata/r4:validation",
    ],
    test_class = "com.google.fhir.common.ResourceValidatorR4Test",
    deps = [
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:resource_validator",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:encounter_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:observation_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "AnnotationUtilsTest",
    size = "small",
    srcs = ["AnnotationUtilsTest.java"],
    test_class = "com.google.fhir.common.AnnotationUtilsTest",
    deps = [
        "//java/com/google/fhir/common:annotation_utils",
        "//proto/google/fhir/proto/stu3:datatypes_java_proto",
        "//proto/google/fhir/proto/stu3:resources_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "FhirTypesTest",
    size = "small",
    srcs = ["FhirTypesTest.java"],
    test_class = "com.google.fhir.common.FhirTypesTest",
    deps = [
        "//java/com/google/fhir/common:fhir_types",
        "//proto/google/fhir/proto/r4:uscore_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core:extensions_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:binary_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_java_proto",
        "//proto/google/fhir/proto/stu3:codes_java_proto",
        "//proto/google/fhir/proto/stu3:datatypes_java_proto",
        "//proto/google/fhir/proto/stu3:extensions_java_proto",
        "//proto/google/fhir/proto/stu3:resources_java_proto",
        "//proto/google/fhir/proto/stu3:uscore_java_proto",
        "//testdata/r4/profiles:test_java_proto",
        "//testdata/stu3/profiles:test_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "ExtensionsTest",
    size = "small",
    srcs = ["ExtensionsTest.java"],
    test_class = "com.google.fhir.common.ExtensionsTest",
    deps = [
        "//java/com/google/fhir/common:extensions",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_java_proto",
        "//proto/google/fhir/proto/stu3:datatypes_java_proto",
        "//proto/google/fhir/proto/stu3:resources_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "ProtoUtilsTest",
    size = "small",
    srcs = ["ProtoUtilsTest.java"],
    test_class = "com.google.fhir.common.ProtoUtilsTest",
    deps = [
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:proto_utils",
        "//testdata:generic_proto_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_truth_extensions_truth_proto_extension",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "BigQuerySchemaTest",
    size = "small",
    srcs = ["BigQuerySchemaTest.java"],
    data = [
        "//spec:stu3",
        "//testdata/stu3:bigquery",
        "//testdata/stu3:examples",
    ],
    test_class = "com.google.fhir.common.BigQuerySchemaTest",
    deps = [
        "//java/com/google/fhir/common:big_query_schema",
        "//proto/google/fhir/proto/stu3:resources_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_apis_google_api_services_bigquery",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_http_client_google_http_client_gson",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "CodesTest",
    size = "small",
    srcs = ["CodesTest.java"],
    test_class = "com.google.fhir.common.CodesTest",
    deps = [
        "//java/com/google/fhir/common:codes",
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:proto_utils",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:binary_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_java_proto",
        "//proto/google/fhir/proto/stu3:codes_java_proto",
        "//proto/google/fhir/proto/stu3:datatypes_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "DescriptorMemosMapTest",
    size = "small",
    srcs = ["DescriptorMemosMapTest.java"],
    test_class = "com.google.fhir.common.DescriptorMemosMapTest",
    deps = [
        "//java/com/google/fhir/common:descriptor_memos_map",
        "//java/com/google/fhir/common:exceptions",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/stu3:datatypes_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "ResourceUtilsTest",
    size = "small",
    srcs = ["ResourceUtilsTest.java"],
    data = [
        "//testdata:resource_utils_testdata",
    ],
    test_class = "com.google.fhir.common.ResourceUtilsTest",
    deps = [
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:proto_utils",
        "//java/com/google/fhir/common:resource_utils",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/google/fhir/proto/stu3:datatypes_java_proto",
        "//proto/google/fhir/proto/stu3:resources_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_extensions_truth_proto_extension",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "TerminologyExpanderTest",
    size = "small",
    srcs = ["TerminologyExpanderTest.java"],
    deps = [
        "//java/com/google/fhir/common:terminology_expander",
        "//java/com/google/fhir/protogen:fhir_package",
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:code_system_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:value_set_java_proto",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)
