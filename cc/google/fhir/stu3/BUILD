licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "resource_validation",
    srcs = ["resource_validation.cc"],
    hdrs = ["resource_validation.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/stu3:annotations_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_test(
    name = "resource_validation_test",
    srcs = ["resource_validation_test.cc"],
    deps = [
        ":resource_validation",
        "//proto/stu3:resources_cc_proto",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)
