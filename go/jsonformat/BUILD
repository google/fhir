package(default_visibility = ["//visibility:public"])

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "jsonformat",
    srcs = [
        "date_time.go",
        "marshaller.go",
        "primitive.go",
        "r3_utils.go",
        "r4_utils.go",
        "reference.go",
        "unmarshaller.go",
        "version_config.go",
    ],
    importpath = "github.com/google/fhir/go/jsonformat",
    deps = [
        "//go/fhirversion",
        "//go/jsonformat/errorreporter",
        "//go/jsonformat/fhirvalidate",
        "//go/jsonformat/internal/accessor",
        "//go/jsonformat/internal/jsonpbhelper",
        "//proto/google/fhir/proto:annotations_go_proto",
        "//proto/google/fhir/proto/r4:fhirproto_extensions_go_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_go_proto",
        "//proto/google/fhir/proto/stu3:datatypes_go_proto",
        "//proto/google/fhir/proto/stu3:fhirproto_extensions_go_proto",
        "//proto/google/fhir/proto/stu3:resources_go_proto",
        "@com_bitbucket_creachadair_stringset//:go_default_library",
        "@com_github_json_iterator_go//:go_default_library",
        "@org_golang_google_protobuf//proto:go_default_library",
        "@org_golang_google_protobuf//reflect/protoreflect:go_default_library",
        "@org_golang_google_protobuf//types/known/anypb:go_default_library",
    ],
)

go_test(
    name = "jsonformat_test",
    size = "small",
    srcs = [
        "date_time_test.go",
        "primitive_test.go",
        "reference_test.go",
    ],
    embed = [":jsonformat"],
    deps = [
        "//go/fhirversion",
        "//go/jsonformat/internal/accessor",
        "//go/jsonformat/internal/jsonpbhelper",
        "//proto/google/fhir/proto:annotations_go_proto",
        "//proto/google/fhir/proto/r4:fhirproto_extensions_go_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_go_proto",
        "//proto/google/fhir/proto/stu3:datatypes_go_proto",
        "//proto/google/fhir/proto/stu3:fhirproto_extensions_go_proto",
        "@com_github_google_go_cmp//cmp:go_default_library",
        "@org_golang_google_protobuf//proto:go_default_library",
        "@org_golang_google_protobuf//reflect/protoreflect:go_default_library",
        "@org_golang_google_protobuf//testing/protocmp:go_default_library",
    ],
)

go_test(
    name = "marshaller_unmarshaller_test",
    size = "small",
    srcs = [
        "marshaller_test.go",
        "unmarshaller_test.go",
    ],
    data = [
        "//testdata/jsonformat",
    ],
    embed = [":jsonformat"],
    deps = [
        "//go/fhirversion",
        "//go/jsonformat/errorreporter",
        "//go/jsonformat/internal/jsonpbhelper",
        "//proto/google/fhir/proto/dstu2:codes_go_proto",
        "//proto/google/fhir/proto/dstu2:datatypes_go_proto",
        "//proto/google/fhir/proto/dstu2:metadatatypes_go_proto",
        "//proto/google/fhir/proto/dstu2:resources_go_proto",
        "//proto/google/fhir/proto/r4/core:codes_go_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:code_system_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:condition_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:device_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:observation_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:operation_outcome_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:research_study_go_proto",
        "//proto/google/fhir/proto/r4/core/resources:search_parameter_go_proto",
        "//proto/google/fhir/proto/stu3:codes_go_proto",
        "//proto/google/fhir/proto/stu3:datatypes_go_proto",
        "//proto/google/fhir/proto/stu3:metadatatypes_go_proto",
        "//proto/google/fhir/proto/stu3:resources_go_proto",
        "@com_github_google_go_cmp//cmp:go_default_library",
        "@com_github_google_go_cmp//cmp/cmpopts:go_default_library",
        "@com_github_pkg_errors//:go_default_library",
        "@io_bazel_rules_go//go/tools/bazel:go_default_library",
        "@org_golang_google_protobuf//proto:go_default_library",
        "@org_golang_google_protobuf//reflect/protoreflect:go_default_library",
        "@org_golang_google_protobuf//testing/protocmp:go_default_library",
        "@org_golang_google_protobuf//types/known/anypb:go_default_library",
    ],
)
