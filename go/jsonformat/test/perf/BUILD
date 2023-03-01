package(
    default_applicable_licenses = [""],
    default_visibility = ["//visibility:public"],
)

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
        "//go/fhirversion",
        "//go/jsonformat",
        "//proto/google/fhir/proto/r4/core:datatypes_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:binary_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_go_proto",
        "@io_bazel_rules_go//go/tools/bazel:go_default_library",
    ],
)
