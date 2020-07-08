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
        "//cc/google/fhir:proto_util",
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
        "//cc/google/fhir:primitive_handler",
        "//cc/google/fhir:util",
        "//cc/google/fhir/r4:primitive_handler",
        "//cc/google/fhir/stu3:primitive_handler",
        "//proto/stu3:resources_cc_proto",
    ],
)

cc_library(
    name = "generator",
    testonly = 1,
    srcs = ["generator.cc"],
    hdrs = ["generator.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:fhir_types",
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
        "@jsoncpp_git//:jsoncpp",
    ],
)

cc_test(
    name = "generator_test",
    srcs = ["generator_test.cc"],
    deps = [
        ":fhir_test_env",
        ":generator",
        ":proto_matchers",
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:primitive_handler",
        "//cc/google/fhir:proto_util",
        "//cc/google/fhir:test_helper",
        "//cc/google/fhir:util",
        "//proto/r4/core:datatypes_cc_proto",
        "//proto/r4/core/resources:patient_cc_proto",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)
