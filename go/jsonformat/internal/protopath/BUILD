package(default_visibility = ["//visibility:public"])

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_test")
load("@io_bazel_rules_go//proto:def.bzl", "go_proto_library")

proto_library(
    name = "protopathtest_proto",
    srcs = [
        "protopathtest.proto",
    ],
)

go_proto_library(
    name = "protopathtest_go_proto",
    importpath = "github.com/google/fhir/go/jsonformat/internal/protopath/protopathtest_go_proto",
    proto = ":protopathtest_proto",
)

go_library(
    name = "protopath",
    srcs = [
        "proto_path.go",
        "proto_path_to_json.go",
    ],
    importpath = "github.com/google/fhir/go/jsonformat/internal/protopath",
    deps = [
        "@com_github_golang_protobuf//proto:go_default_library",
        "@com_github_serenize_snaker//:go_default_library",
        "@org_golang_google_protobuf//proto:go_default_library",
        "@org_golang_google_protobuf//reflect/protoreflect:go_default_library",
        "@org_golang_google_protobuf//reflect/protoregistry:go_default_library",
    ],
)

go_test(
    name = "protopath_test",
    size = "small",
    srcs = [
        "proto_path_test.go",
        "proto_path_to_json_test.go",
    ],
    embed = [":protopath"],
    deps = [
        ":protopathtest_go_proto",
        "@com_github_google_go_cmp//cmp:go_default_library",
        "@com_github_google_go_cmp//cmp/cmpopts:go_default_library",
        "@org_golang_google_protobuf//proto:go_default_library",
        "@org_golang_google_protobuf//testing/protocmp:go_default_library",
    ],
)
