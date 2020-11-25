package(default_visibility = ["//visibility:public"])

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_test")
load("@io_bazel_rules_go//proto:def.bzl", "go_proto_library")

go_library(
    name = "accessor",
    srcs = ["getter_setter.go"],
    importpath = "github.com/google/fhir/go/jsonformat/internal/accessor",
    deps = [
        "@com_github_golang_protobuf//proto:go_default_library",
        "@com_github_pkg_errors//:go_default_library",
        "@org_golang_google_protobuf//reflect/protoreflect:go_default_library",
    ],
)

go_test(
    name = "accessor_test",
    size = "small",
    srcs = ["getter_setter_test.go"],
    embed = [":accessor"],
    deps = [
        ":accessor_test_go_proto",
        "//proto/google/fhir/proto/stu3:datatypes_go_proto",
        "//proto/google/fhir/proto/stu3:resources_go_proto",
        "@com_github_golang_protobuf//proto:go_default_library",
        "@com_github_google_go_cmp//cmp:go_default_library",
        "@org_golang_google_protobuf//testing/protocmp:go_default_library",
    ],
)

proto_library(
    name = "accessor_test_proto",
    srcs = ["accessor_test.proto"],
)

go_proto_library(
    name = "accessor_test_go_proto",
    importpath = "github.com/google/fhir/go/jsonformat/internal/accessor/accessor_test_go_proto",
    proto = ":accessor_test_proto",
)
