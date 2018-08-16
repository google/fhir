licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "text_tokenizer",
    srcs = ["text_tokenizer.cc"],
    hdrs = ["text_tokenizer.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "@com_google_absl//absl/strings",
        "@com_googlesource_code_re2//:re2",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_test(
    name = "text_tokenizer_test",
    srcs = ["text_tokenizer_test.cc"],
    deps = [
        ":text_tokenizer",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)
