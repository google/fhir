load("@io_bazel_rules_go//go:def.bzl", "go_library")
load("//tools/build_defs/go:go_library.bzl", "go_library")
load("//tools/build_defs/go:go_wrap_cc.bzl", "go_wrap_cc")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

go_library(
    name = "fhirpath",
    srcs = ["visitor.go"],
    importpath = "github.com/google/fhir/go/fhirpath",
    deps = [
        "//go/fhirpath/gen",
        "@com_github_antrl4_go_antlr//:go_default_library",
    ],
)
