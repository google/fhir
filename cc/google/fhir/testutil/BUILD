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
