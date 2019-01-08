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
        ":fhir_common",
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
    srcs = glob(
        ["src/main/java/com/google/fhir/examples/*.java"],
        exclude = [
            "src/main/java/com/google/fhir/examples/BigQuerySchemaGeneratorMain.java",
            "src/main/java/com/google/fhir/examples/StructureDefinitionTransformerMain.java",
        ],
    ),
    deps = [
        ":fhir_common",
        ":structure_definition_transformer",
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
        "@org_apache_beam_beam_sdks_java_core//jar",
    ],
)

java_library(
    name = "beam-runners",
    runtime_deps = [
        "@com_fasterxml_jackson_core_jackson_annotations//jar",
        "@com_fasterxml_jackson_core_jackson_core//jar",
        "@com_fasterxml_jackson_core_jackson_databind//jar",
        "@com_google_api_client_google_api_client//jar",
        "@com_google_apis_google_api_services_clouddebugger//jar",
        "@com_google_apis_google_api_services_dataflow//jar",
        "@com_google_apis_google_api_services_storage//jar",
        "@com_google_auth_google_auth_library_credentials//jar",
        "@com_google_cloud_bigdataoss_util//jar",
        "@joda_time_joda_time//jar",
        "@org_apache_beam_beam_model_pipeline//jar",
        "@org_apache_beam_beam_runners_direct_java//jar",
        "@org_apache_beam_beam_runners_google_cloud_dataflow_java//jar",
        "@org_apache_beam_beam_sdks_java_extensions_google_cloud_platform_core//jar",
        "@org_apache_beam_beam_sdks_java_io_google_cloud_platform//jar",
        "@org_apache_beam_beam_vendor_grpc_v1_13_1//jar",
        "@org_slf4j_slf4j_api//jar",
        "@org_slf4j_slf4j_simple//jar",
        "@org_xerial_snappy_snappy_java//jar",
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

java_binary(
    name = "SyntheaToTFRecords",
    main_class = "com.google.fhir.examples.SyntheaToTFRecordsMain",
    runtime_deps = [
        ":beam-runners",
        ":examples",
    ],
)

java_library(
    name = "structure_definition_transformer",
    srcs = [
        "src/main/java/com/google/fhir/dstu2/StructureDefinitionTransformer.java",
    ],
    deps = [
        "//external:gson",
        "//external:guava",
        "@com_beust_jcommander//jar",
        "@com_google_http_client_google_http_client//jar",
        "@com_google_http_client_google_http_client_gson//jar",
    ],
)

java_library(
    name = "fhir_common",
    srcs = [
        "src/main/java/com/google/fhir/common/FhirVersion.java",
    ],
    deps = [
        "//external:guava",
        "//proto/stu3:codes_java_proto",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:extensions_java_proto",
        "//proto/stu3:metadatatypes_java_proto",
        "//proto/stu3:resources_java_proto",
        "//proto/stu3:uscore_codes_java_proto",
        "@com_google_protobuf//:protobuf_java",
    ],
)

java_test(
    name = "StructureDefinitionTransformerTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/dstu2/StructureDefinitionTransformerTest.java"],
    test_class = "com.google.fhir.dstu2.StructureDefinitionTransformerTest",
    deps = [
        ":structure_definition_transformer",
        "//external:gson",
        "@com_google_http_client_google_http_client//jar",
        "@com_google_http_client_google_http_client_gson//jar",
        "@com_google_truth_truth//jar",
        "@junit_junit//jar",
    ],
)

java_binary(
    name = "StructureDefinitionTransformer",
    srcs = ["src/main/java/com/google/fhir/examples/StructureDefinitionTransformerMain.java"],
    main_class = "com.google.fhir.examples.StructureDefinitionTransformerMain",
    deps = [
        ":structure_definition_transformer",
        "//external:guava",
        "@com_beust_jcommander//jar",
    ],
)

java_library(
    name = "big_query_schema_generator_lib",
    srcs = ["src/main/java/com/google/fhir/examples/BigQuerySchemaGeneratorMain.java"],
    deps = [
        ":stu3",
        "//external:guava",
        "//examples/profiles:demo_java_proto",
        "@com_beust_jcommander//jar",
        "@com_google_apis_google_api_services_bigquery//jar",
        "@com_google_http_client_google_http_client//jar",
        "@com_google_http_client_google_http_client_gson//jar",
        "@com_google_protobuf//:protobuf_java",
    ],
)

java_binary(
    name = "BigQuerySchemaGenerator",
    main_class = "com.google.fhir.examples.BigQuerySchemaGeneratorMain",
    runtime_deps = [
        ":big_query_schema_generator_lib",
    ],
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
        "//spec:stu3",
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
        "//spec:stu3",
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
        "//spec:fhir_stu3_structure_definitions",
        "//testdata/stu3:descriptors",
    ],
    test_class = "com.google.fhir.stu3.ProtoFilePrinterTest",
    deps = [
        ":fhir_common",
        ":stu3",
        "//external:gson",
        "//external:guava",
        "//proto/stu3:annotations_java_proto",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:extensions_java_proto",
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
        "//spec:fhir_stu3_structure_definitions",
        "//spec:uscore",
        "//testdata/stu3:descriptors",
        "//testdata/stu3:google",
    ],
    test_class = "com.google.fhir.stu3.ProtoGeneratorTest",
    deps = [
        ":fhir_common",
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

java_library(
    name = "profile_generator_lib",
    srcs = ["src/main/java/com/google/fhir/profile/ProfileGenerator.java"],
    deps = [
        "//external:gson",
        "//external:guava",
        "//proto/stu3:annotations_java_proto",
        "//proto/stu3:codes_java_proto",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:extensions_java_proto",
        "//proto/stu3:metadatatypes_java_proto",
        "//proto/stu3:profile_config_java_proto",
        "//proto/stu3:profiles_java_proto",
        "//proto/stu3:resources_java_proto",
        "//java:stu3",
        "@com_google_protobuf//:protobuf_java",
    ],
)

java_binary(
    name = "ProfileGenerator",
    srcs = ["src/main/java/com/google/fhir/profile/ProfileGeneratorMain.java"],
    main_class = "com.google.fhir.profile.ProfileGeneratorMain",
    deps = [
        ":profile_generator_lib",
        "//external:guava",
        "//proto/stu3:profile_config_java_proto",
        "//proto/stu3:resources_java_proto",
        "//java:stu3",
        "@com_beust_jcommander//jar",
        "@com_google_protobuf//:protobuf_java",
    ],
)
