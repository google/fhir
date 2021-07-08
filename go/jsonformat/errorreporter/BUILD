package(default_visibility = ["//visibility:public"])

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "errorreporter",
    srcs = ["errorreporter.go"],
    importpath = "github.com/google/fhir/go/jsonformat/errorreporter",
    deps = [
        "//go/fhirversion",
        "//proto/google/fhir/proto/r4/core:codes_go_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:operation_outcome_go_proto",
        "//proto/google/fhir/proto/stu3:codes_go_proto",
        "//proto/google/fhir/proto/stu3:datatypes_go_proto",
        "//proto/google/fhir/proto/stu3:resources_go_proto",
    ],
)

go_test(
    name = "errorreporter_test",
    size = "small",
    srcs = ["errorreporter_test.go"],
    embed = [":errorreporter"],
    deps = [
        "//go/fhirversion",
        "//go/jsonformat/internal/jsonpbhelper",
        "//proto/google/fhir/proto/r4/core:codes_go_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:operation_outcome_go_proto",
        "//proto/google/fhir/proto/stu3:codes_go_proto",
        "//proto/google/fhir/proto/stu3:datatypes_go_proto",
        "//proto/google/fhir/proto/stu3:resources_go_proto",
        "@com_github_google_go_cmp//cmp:go_default_library",
        "@org_golang_google_protobuf//testing/protocmp:go_default_library",
    ],
)
