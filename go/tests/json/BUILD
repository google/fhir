package(default_visibility = ["//visibility:private"])

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_test")

go_test(
    name = "go_default_test",
    srcs = [
        "interfaces_test.go",
        "json_test.go",
    ],
    deps = [
        "//proto/stu3:go_default_library",
        "@com_github_golang_protobuf//jsonpb:go_default_library_gen",
        "@com_github_golang_protobuf//proto:go_default_library",
    ],
)
