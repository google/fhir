load("@io_bazel_rules_go//go:def.bzl", "go_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

go_library(
    name = "gen",
    srcs = [
        "doc.go",
        "fhirpath_base_listener.go",
        "fhirpath_base_visitor.go",
        "fhirpath_lexer.go",
        "fhirpath_listener.go",
        "fhirpath_parser.go",
        "fhirpath_visitor.go",
    ],
    importpath = "github.com/google/fhir/go/fhirpath/gen",
    visibility = [
        "//go/fhirpath:__subpackages__",
    ],
    deps = ["@com_github_antrl4_go_antlr//:go_default_library"],
)
