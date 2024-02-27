load("@io_bazel_rules_go//go:def.bzl", "go_library")

licenses(["notice"])

package(
    
    default_visibility = ["//visibility:public"],
)

go_library(
    name = "fhirpath",
    srcs = [
        "visitor.go",
    ],
    deps = [
        "//go/fhir_path/gen",
        "@com_github_antrl4_go_antlr//:go_default_library",
    ],
)
