package(default_visibility = ["//visibility:public"])

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_test")

go_test(
    name = "perf_test",
    size = "small",
    srcs = ["perf_test.go"],
    data = [
        "//go/jsonformat/test:testdata",
    ],
    deps = [
        "//go/jsonformat",
        "@io_bazel_rules_go//go/tools/bazel:go_default_library",
    ],
)
