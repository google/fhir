licenses(["notice"])

package(default_visibility = ["//visibility:public"],)

cc_library(
    name = "status",
    hdrs = ["status.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_library(
    name = "statusor",
    srcs = ["statusor.cc"],
    hdrs = [
        "statusor.h",
        "statusor_internals.h",
    ],
    strip_include_prefix = "//cc/",
    deps = [
        ":status",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_test(
    name = "statusor_test",
    size = "small",
    srcs = ["statusor_test.cc"],
    deps = [
        ":statusor",
        "@com_google_googletest//:gtest_main",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)
