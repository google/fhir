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
