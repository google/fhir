package(default_visibility = ["//visibility:private"],)

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_test")

# Does nothing other than tie Go compilation into `bazel test ...``
go_test(
    name = "go_default_test",
    srcs = ["compilecheck_test.go"],
    deps = [
        "//proto/stu3:go_default_library",
    ],
)
