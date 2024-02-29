licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "codeable_concepts",
    srcs = ["codeable_concepts.cc"],
    hdrs = ["codeable_concepts.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:fhir_types",
        "//cc/google/fhir:proto_util",
        "//cc/google/fhir/status:statusor",
        "//proto/google/fhir/proto:annotations_cc_proto",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "codeable_concepts_test",
    srcs = ["codeable_concepts_test.cc"],
    data = [
        "//testdata/stu3/profiles:testdata",
    ],
    deps = [
        ":codeable_concepts",
        "//cc/google/fhir:test_helper",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//testdata/stu3/profiles:test_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "profiles",
    srcs = [
        "profiles.cc",
    ],
    hdrs = ["profiles.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":operation_error_reporter",
        ":primitive_handler",
        "//cc/google/fhir:error_reporter",
        "//cc/google/fhir:profiles_lib",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "profiles_test",
    srcs = [
        "profiles_test.cc",
    ],
    data = [
        "//testdata/stu3:examples",
        "//testdata/stu3/profiles:testdata",
    ],
    deps = [
        ":profiles",
        "//cc/google/fhir:test_helper",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/stu3:profiles_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "//proto/google/fhir/proto/stu3:uscore_cc_proto",
        "//testdata/stu3/profiles:test_cc_proto",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "primitive_handler",
    srcs = ["primitive_handler.cc"],
    hdrs = ["primitive_handler.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":codeable_concepts",
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:codes",
        "//cc/google/fhir:extensions",
        "//cc/google/fhir:fhir_types",
        "//cc/google/fhir:primitive_handler",
        "//cc/google/fhir:primitive_wrapper",
        "//cc/google/fhir:proto_util",
        "//cc/google/fhir:util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:fhirproto_extensions_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "json_format",
    srcs = ["json_format.cc"],
    hdrs = ["json_format.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":primitive_handler",
        "//cc/google/fhir:error_reporter",
        "//cc/google/fhir:json_format",
        "//cc/google/fhir:json_format_results",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "json_format_test",
    size = "large",
    srcs = ["json_format_test.cc"],
    data = [
        "//spec:stu3",
        "//testdata/jsonformat",
        "//testdata/stu3:bigquery",
        "//testdata/stu3:examples",
        "//testdata/stu3:ndjson",
        "//testdata/stu3:profiles",
    ],
    shard_count = 10,
    deps = [
        ":json_format",
        ":resource_validation",
        "//cc/google/fhir:error_reporter",
        "//cc/google/fhir:json_format",
        "//cc/google/fhir:test_helper",
        "//cc/google/fhir/json:fhir_json",
        "//cc/google/fhir/json:json_sax_handler",
        "//cc/google/fhir/json:test_matchers",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "//testdata/stu3/profiles:test_cc_proto",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "resource_validation",
    srcs = ["resource_validation.cc"],
    hdrs = ["resource_validation.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":operation_error_reporter",
        ":primitive_handler",
        "//cc/google/fhir:error_reporter",
        "//cc/google/fhir:resource_validation",
        "//cc/google/fhir/fhir_path:fhir_path_validation",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "operation_error_reporter",
    hdrs = ["operation_error_reporter.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir:error_reporter",
        "//cc/google/fhir:operation_error_reporter",
        "//proto/google/fhir/proto/stu3:codes_cc_proto",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/status",
    ],
)

cc_test(
    name = "resource_validation_test",
    srcs = ["resource_validation_test.cc"],
    data = [
        "//testdata/stu3:validation",
    ],
    deps = [
        ":resource_validation",
        "//cc/google/fhir:test_helper",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "primitive_handler_test",
    srcs = ["primitive_handler_test.cc"],
    data = [
        "//testdata/stu3:validation",
    ],
    deps = [
        ":json_format",
        ":primitive_handler",
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:json_format",
        "//cc/google/fhir:test_helper",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/stu3:codes_cc_proto",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:fhirproto_extensions_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)
