package(default_visibility = ["//visibility:public"])

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "fhirvalidate",
    srcs = ["fhirvalidate.go"],
    importpath = "github.com/google/fhir/go/jsonformat/fhirvalidate",
    deps = [
        "//go/jsonformat/errorreporter",
        "//go/jsonformat/internal/jsonpbhelper",
        "//proto/google/fhir/proto:annotations_go_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_go_proto",
        "//proto/google/fhir/proto/stu3:datatypes_go_proto",
        "@com_bitbucket_creachadair_stringset//:go_default_library",
        "@org_golang_google_protobuf//proto:go_default_library",
        "@org_golang_google_protobuf//reflect/protoreflect:go_default_library",
    ],
)

go_test(
    name = "fhirvalidate_test",
    size = "small",
    srcs = ["fhirvalidate_test.go"],
    embed = [":fhirvalidate"],
    deps = [
        "//go/fhirversion",
        "//go/jsonformat/errorreporter",
        "//proto/google/fhir/proto/dstu2:codes_go_proto",
        "//proto/google/fhir/proto/dstu2:datatypes_go_proto",
        "//proto/google/fhir/proto/dstu2:resources_go_proto",
        "//proto/google/fhir/proto/r4/core:codes_go_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:operation_outcome_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_go_proto",
        "//proto/google/fhir/proto/stu3:codes_go_proto",
        "//proto/google/fhir/proto/stu3:datatypes_go_proto",
        "//proto/google/fhir/proto/stu3:resources_go_proto",
        "@com_github_google_go_cmp//cmp:go_default_library",
        "@org_golang_google_protobuf//proto:go_default_library",
        "@org_golang_google_protobuf//testing/protocmp:go_default_library",
    ],
)
