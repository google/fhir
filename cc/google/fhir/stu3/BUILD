licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "codeable_concepts",
    srcs = ["codeable_concepts.cc"],
    hdrs = ["codeable_concepts.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:proto_util",
        "//cc/google/fhir/status:statusor",
        "//net/proto2/proto:descriptor_cc_proto",
        "//proto:annotations_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@org_tensorflow//tensorflow/core:lib",
        "@protobuf_archive//:protobuf",
    ],
)

cc_test(
    name = "codeable_concepts_test",
    srcs = ["codeable_concepts_test.cc"],
    data = [
        "//testdata/stu3/profiles:testdata",
    ],
    deps = [
        ":codeable_concepts",
        "//cc/google/fhir:test_helper",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/stu3:datatypes_cc_proto",
        "//testdata/stu3/profiles:test_cc_proto",
        "@com_google_googletest//:gtest_main",
        "@org_tensorflow//tensorflow/core:test",
    ],
)
