licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "feature_keys",
    srcs = ["feature_keys.cc"],
    hdrs = ["feature_keys.h"],
    strip_include_prefix = "//cc/",
)

cc_library(
    name = "resource_to_example",
    srcs = ["resource_to_example.cc"],
    hdrs = ["resource_to_example.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":text_tokenizer",
        "//cc/google/fhir/status",
        "//cc/google/fhir/stu3:util",
        "//cc/google/fhir/systems",
        "//proto/stu3:annotations_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
        "@com_googlesource_code_re2//:re2",
        "@org_tensorflow//tensorflow/core:lib",
        "@org_tensorflow//tensorflow/core:protos_all_cc",
    ],
)

cc_test(
    name = "resource_to_example_test",
    srcs = ["resource_to_example_test.cc"],
    deps = [
        ":resource_to_example",
        "//cc/google/fhir/stu3:test_helper",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/stu3:resources_cc_proto",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
        "@org_tensorflow//tensorflow/core:protos_all_cc",
    ],
)

cc_library(
    name = "text_tokenizer",
    srcs = ["text_tokenizer.cc"],
    hdrs = ["text_tokenizer.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "@com_google_absl//absl/strings",
        "@com_googlesource_code_re2//:re2",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_test(
    name = "text_tokenizer_test",
    srcs = ["text_tokenizer_test.cc"],
    deps = [
        ":text_tokenizer",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)
