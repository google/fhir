package(default_visibility = ["//visibility:public"])

cc_library(
    name = "primitive_handler",
    srcs = ["primitive_handler.cc"],
    hdrs = ["primitive_handler.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir:primitive_handler",
        "//cc/google/fhir:primitive_wrapper",
        "//proto/google/fhir/proto/r5:fhirproto_extensions_cc_proto",
        "//proto/google/fhir/proto/r5/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:bundle_and_contained_resource_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "primitive_handler_test",
    srcs = ["primitive_handler_test.cc"],
    deps = [
        ":primitive_handler",
        "//cc/google/fhir:error_reporter",
        "//cc/google/fhir:json_format_results",
        "//cc/google/fhir:operation_error_reporter",
        "//cc/google/fhir:primitive_handler",
        "//cc/google/fhir/json:fhir_json",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/r5:fhirproto_extensions_cc_proto",
        "//proto/google/fhir/proto/r5/core:codes_cc_proto",
        "//proto/google/fhir/proto/r5/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:binary_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:observation_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:operation_outcome_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:patient_cc_proto",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)
