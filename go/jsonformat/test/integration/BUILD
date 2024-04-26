load("@io_bazel_rules_go//go:def.bzl", "go_test")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

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
        "//go/fhirversion",
        "//go/jsonformat",
        "//go/jsonformat/internal/jsonpbhelper",
        "//go/protopath",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:composition_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:observation_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_go_proto",
        "//proto/google/fhir/proto/stu3:resources_go_proto",
        "@com_github_google_go_cmp//cmp:go_default_library",
        "@io_bazel_rules_go//go/tools/bazel:go_default_library",
        "@org_bitbucket_creachadair_stringset//:go_default_library",
        "@org_golang_google_protobuf//encoding/prototext:go_default_library",
        "@org_golang_google_protobuf//proto:go_default_library",
        "@org_golang_google_protobuf//testing/protocmp:go_default_library",
    ],
)
