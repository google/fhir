package(default_visibility = ["//visibility:public"])

licenses(["notice"])

# gMock matchers for Protocol Buffers.
cc_library(
    name = "proto_matchers",
    testonly = 1,
    srcs = ["proto_matchers.cc"],
    hdrs = ["proto_matchers.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "fhir_test_env",
    testonly = 1,
    hdrs = ["fhir_test_env.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir:type_macros",
        "//cc/google/fhir/r4:primitive_handler",
        "//cc/google/fhir/r5:primitive_handler",
        "//cc/google/fhir/stu3:primitive_handler",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:claim_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:composition_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:condition_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:encounter_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:medication_administration_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:medication_request_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:observation_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:organization_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:parameters_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:procedure_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:value_set_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
    ],
)

cc_library(
    name = "generator",
    srcs = ["generator.cc"],
    hdrs = ["generator.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:error_reporter",
        "//cc/google/fhir:fhir_types",
        "//cc/google/fhir:json_format_results",
        "//cc/google/fhir:primitive_handler",
        "//cc/google/fhir:proto_util",
        "//cc/google/fhir:references",
        "//cc/google/fhir:util",
        "//cc/google/fhir/status",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/random:distributions",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "generator_test",
    srcs = ["generator_test.cc"],
    deps = [
        ":fhir_test_env",
        ":generator",
        ":proto_matchers",
        "//cc/google/fhir:proto_util",
        "//cc/google/fhir/r4:primitive_handler",
        "//cc/google/fhir/status",
        "//proto/google/fhir/proto/r4/core/resources:observation_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:plan_definition_cc_proto",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "archive",
    srcs = ["archive.cc"],
    hdrs = ["archive.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "@com_google_absl//absl/cleanup",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@libarchive",
    ],
)
