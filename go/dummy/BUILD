package(default_visibility = ["//visibility:public"])

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_test")

go_test(
    name = "go_default_test",
    srcs = ["dummy_test.go"],
    deps = [
        "//proto/stu3:go_default_library",
    ],
)
