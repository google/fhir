licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "systems",
    srcs = ["systems.cc"],
    hdrs = ["systems.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "systems_test",
    srcs = ["systems_test.cc"],
    deps = [
        ":systems",
        "@com_google_googletest//:gtest_main",
    ],
)
