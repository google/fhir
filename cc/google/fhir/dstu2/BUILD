licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "primitive_handler",
    srcs = ["primitive_handler.cc"],
    hdrs = ["primitive_handler.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:json_format_results",
        "//cc/google/fhir:primitive_handler",
        "//cc/google/fhir:primitive_wrapper",
        "//cc/google/fhir:proto_util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/google/fhir/proto/dstu2:codes_cc_proto",
        "//proto/google/fhir/proto/dstu2:datatypes_cc_proto",
        "//proto/google/fhir/proto/dstu2:fhirproto_extensions_cc_proto",
        "//proto/google/fhir/proto/dstu2:resources_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "primitive_handler_test",
    size = "large",
    srcs = ["primitive_handler_test.cc"],
    data = [
        "//testdata/dstu2:validation",
    ],
    deps = [
        ":json_format",
        ":primitive_handler",
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:error_reporter",
        "//cc/google/fhir:test_helper",
        "//cc/google/fhir/status",
        "//proto/google/fhir/proto/dstu2:datatypes_cc_proto",
        "//proto/google/fhir/proto/dstu2:fhirproto_extensions_cc_proto",
        "//proto/google/fhir/proto/dstu2:resources_cc_proto",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
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
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "json_format_test",
    size = "large",
    srcs = ["json_format_test.cc"],
    data = [
        "//spec:dstu2",
        "//testdata/dstu2:examples",
        "//testdata/jsonformat",
    ],
    shard_count = 10,
    deps = [
        ":json_format",
        ":primitive_handler",
        "//cc/google/fhir:test_helper",
        "//cc/google/fhir/json:fhir_json",
        "//cc/google/fhir/json:json_sax_handler",
        "//cc/google/fhir/json:test_matchers",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto:annotations_cc_proto",
        "//proto/google/fhir/proto/dstu2:codes_cc_proto",
        "//proto/google/fhir/proto/dstu2:datatypes_cc_proto",
        "//proto/google/fhir/proto/dstu2:resources_cc_proto",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "fhir_test_env",
    testonly = 1,
    hdrs = ["fhir_test_env.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":primitive_handler",
        "//cc/google/fhir:primitive_handler",
        "//cc/google/fhir:type_macros",
        "//cc/google/fhir:util",
        "//proto/google/fhir/proto/dstu2:resources_cc_proto",
    ],
)
