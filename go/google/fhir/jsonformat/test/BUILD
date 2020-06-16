package(default_visibility = ["//visibility:public"])

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_test")

go_test(
    name = "integration_test",
    size = "large",
    srcs = ["integration_test.go"],
    data = [
        "//spec:r4_examples",
        "//spec:stu3",
        "//testdata/r4:bigquery",
        "//testdata/r4:examples",
        "//testdata/stu3:bigquery",
        "//testdata/stu3:examples",
    ],
    deps = [
        "//go/google/fhir/jsonformat",
        "//go/google/fhir/jsonformat/internal/jsonpbhelper",
        "//go/google/fhir/jsonformat/internal/protopath",
        "@com_bitbucket_creachadair_stringset//:go_default_library",
        "@com_github_golang_protobuf//proto:go_default_library",
        "@com_github_google_go_cmp//cmp:go_default_library",
        "@com_google_fhir//proto/r4/core/resources:bundle_and_contained_resource_go_proto",
        "@com_google_fhir//proto/r4/core/resources:composition_go_proto",
        "@com_google_fhir//proto/r4/core/resources:observation_go_proto",
        "@com_google_fhir//proto/r4/core/resources:patient_go_proto",
        "@com_google_fhir//proto/stu3:resources_go_proto",
        "@io_bazel_rules_go//go/tools/bazel:go_default_library",
        "@org_golang_google_protobuf//testing/protocmp:go_default_library",
    ],
)

filegroup(
    name = "testdata",
    srcs = glob(["testdata/*.json"]),
)

go_test(
    name = "perf_test",
    size = "small",
    srcs = ["perf_test.go"],
    data = [
        ":testdata",
    ],
    deps = [
        "//go/google/fhir/jsonformat",
        "@io_bazel_rules_go//go/tools/bazel:go_default_library",
    ],
)
