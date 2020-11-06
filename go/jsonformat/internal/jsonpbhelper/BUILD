package(default_visibility = ["//visibility:public"])

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "jsonpbhelper",
    srcs = [
        "fhirutil.go",
        "json_format.go",
    ],
    importpath = "github.com/google/fhir/go/jsonformat/internal/jsonpbhelper",
    deps = [
        "//proto/google/fhir/proto:annotations_go_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_go_proto",
        "//proto/google/fhir/proto/stu3:datatypes_go_proto",
        "//proto/google/fhir/proto/stu3:resources_go_proto",
        "@com_bitbucket_creachadair_stringset//:go_default_library",
        "@com_github_golang_glog//:go_default_library",
        "@com_github_json_iterator_go//:go_default_library",
        "@org_golang_google_protobuf//proto:go_default_library",
        "@org_golang_google_protobuf//reflect/protoreflect:go_default_library",
    ],
)

go_test(
    name = "jsonpbhelper_test",
    size = "small",
    srcs = [
        "fhirutil_test.go",
        "json_format_test.go",
    ],
    embed = [":jsonpbhelper"],
    deps = [
        "//proto/google/fhir/proto/r4/core:datatypes_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:basic_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_go_proto",
        "//proto/google/fhir/proto/stu3:codes_go_proto",
        "//proto/google/fhir/proto/stu3:datatypes_go_proto",
        "//proto/google/fhir/proto/stu3:fhirproto_extensions_go_proto",
        "//proto/google/fhir/proto/stu3:resources_go_proto",
        "@com_github_google_go_cmp//cmp:go_default_library",
        "@org_golang_google_protobuf//proto:go_default_library",
        "@org_golang_google_protobuf//reflect/protoreflect:go_default_library",
        "@org_golang_google_protobuf//testing/protocmp:go_default_library",
    ],
)
