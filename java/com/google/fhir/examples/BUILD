load("@rules_java//java:defs.bzl", "java_binary", "java_library")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

java_library(
    name = "examples",
    srcs = [
        "ConvertNdJsonForBigQueryMain.java",
        "JsonParserArgs.java",
        "SplitBundleMain.java",
    ],
    deps = [
        "//java/com/google/fhir/common:big_query_schema",
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:json_format",
        "//java/com/google/fhir/common:resource_utils",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@com_google_protobuf//:protobuf_java_util",
        "@maven//:com_beust_jcommander",
        "@maven//:com_google_apis_google_api_services_bigquery",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_http_client_google_http_client_gson",
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
    name = "SplitBundle",
    main_class = "com.google.fhir.examples.SplitBundleMain",
    runtime_deps = [":examples"],
)

java_library(
    name = "big_query_schema_generator_lib",
    srcs = ["BigQuerySchemaGeneratorMain.java"],
    deps = [
        "//examples/profiles:demo_java_proto",
        "//java/com/google/fhir/common:big_query_schema",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_apis_google_api_services_bigquery",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_http_client_google_http_client_gson",
    ],
)

java_binary(
    name = "BigQuerySchemaGenerator",
    main_class = "com.google.fhir.examples.BigQuerySchemaGeneratorMain",
    runtime_deps = [
        ":big_query_schema_generator_lib",
    ],
)
