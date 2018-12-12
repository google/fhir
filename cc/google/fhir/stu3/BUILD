licenses(["notice"])
package(default_visibility = ["//visibility:public"],)

cc_library(
    name = "codes",
    srcs = ["codes.cc"],
    hdrs = ["codes.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":proto_util",
        ":util",
        "//cc/google/fhir/status",
        "//proto/stu3:annotations_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "@protobuf_archive//:protobuf",
    ],
)

cc_library(
    name = "extensions",
    srcs = ["extensions.cc"],
    hdrs = ["extensions.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":codes",
        ":proto_util",
        ":util",
        "//cc/google/fhir/status",
        "//proto/stu3:annotations_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "@com_google_absl//absl/strings",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_test(
    name = "extensions_test",
    srcs = ["extensions_test.cc"],
    data = ["//testdata/stu3:google"],
    deps = [
        ":extensions",
        ":test_helper",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:extensions_cc_proto",
        "//testdata/stu3/profiles:test_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@org_tensorflow//tensorflow/core:lib",
        "@org_tensorflow//tensorflow/core:test",
    ],
)

cc_library(
    name = "profiles",
    srcs = ["profiles.cc"],
    hdrs = ["profiles.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":codes",
        ":extensions",
        ":proto_util",
        ":resource_validation",
        ":util",
        "//cc/google/fhir/status",
        "//proto/stu3:annotations_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "@com_google_absl//absl/strings",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_test(
    name = "profiles_test",
    srcs = ["profiles_test.cc"],
    data = [
        "//testdata/stu3:examples",
        "//testdata/stu3/profiles:testdata",
    ],
    deps = [
        ":profiles",
        ":test_helper",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:extensions_cc_proto",
        "//proto/stu3:profiles_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "//testdata/stu3/profiles:test_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "util",
    srcs = ["util.cc"],
    hdrs = ["util.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//cc/google/fhir/systems",
        "//proto/stu3:annotations_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_test(
    name = "util_test",
    srcs = ["util_test.cc"],
    deps = [
        ":util",
        "//cc/google/fhir/systems",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/stu3:codes_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:metadatatypes_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
        "@org_tensorflow//tensorflow/core:lib",
        "@protobuf_archive//:protobuf",
    ],
)

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
        "@org_tensorflow//tensorflow/core:lib",
        "@protobuf_archive//:protobuf",
    ],
)

cc_library(
    name = "proto_util",
    srcs = ["proto_util.cc"],
    hdrs = ["proto_util.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_googlesource_code_re2//:re2",
        "@org_tensorflow//tensorflow/core:lib",
        "@protobuf_archive//:protobuf",
    ],
)

cc_test(
    name = "proto_util_test",
    srcs = ["proto_util_test.cc"],
    deps = [
        ":proto_util",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "@com_google_googletest//:gtest_main",
        "@protobuf_archive//:protobuf",
    ],
)

cc_library(
    name = "test_helper",
    testonly = 1,
    hdrs = ["test_helper.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":resource_validation",
        "//cc/google/fhir/status",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@org_tensorflow//tensorflow/core:lib",
        "@protobuf_archive//:protobuf",
    ],
)

cc_library(
    name = "bundle_to_versioned_resources_converter",
    srcs = ["bundle_to_versioned_resources_converter.cc"],
    hdrs = ["bundle_to_versioned_resources_converter.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":proto_util",
        ":util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "//proto/stu3:version_config_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@org_tensorflow//tensorflow/core:lib",
        "@protobuf_archive//:protobuf",
    ],
)

cc_test(
    name = "bundle_to_versioned_resources_converter_test",
    srcs = ["bundle_to_versioned_resources_converter_test.cc"],
    data = ["//proto/stu3:version_config.textproto"],
    deps = [
        ":bundle_to_versioned_resources_converter",
        "//cc/google/fhir/testutil:proto_matchers",
        "@com_google_googletest//:gtest_main",
        "@org_tensorflow//tensorflow/core:lib",
        "@protobuf_archive//:protobuf",
    ],
)

cc_library(
    name = "json_format",
    srcs = [
        "json_parser.cc",
        "json_printer.cc",
        "primitive_wrapper.cc",
        "primitive_wrapper.h",
    ],
    hdrs = [
        "json_format.h",
        "primitive_wrapper.h",
    ],
    strip_include_prefix = "//cc/",
    deps = [
        ":codes",
        ":extensions",
        ":util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/stu3:annotations_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:extensions_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_googlesource_code_re2//:re2",
        "@jsoncpp_git//:jsoncpp",
        "@org_tensorflow//tensorflow/core:lib",
        "@protobuf_archive//:protobuf",
    ],
)

cc_test(
    name = "json_format_test",
    srcs = ["json_format_test.cc"],
    data = [
        "//spec:stu3",
        "//testdata/stu3:bigquery",
        "//testdata/stu3:examples",
        "//testdata/stu3:ndjson",
    ],
    deps = [
        ":json_format",
        ":test_helper",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@jsoncpp_git//:jsoncpp",
        "@protobuf_archive//:protobuf",
    ],
)

cc_test(
    name = "primitive_wrapper_test",
    srcs = ["primitive_wrapper_test.cc"],
    data = [
        "//testdata/stu3:validation",
    ],
    deps = [
        ":json_format",
        ":test_helper",
        "//proto/stu3:datatypes_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@org_tensorflow//tensorflow/core:test",
    ],
)
