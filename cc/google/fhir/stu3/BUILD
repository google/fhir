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
        "//proto:annotations_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
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

cc_library(
    name = "profiles",
    srcs = [
        "profiles.cc",
    ],
    hdrs = ["profiles.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir:profiles_lib",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/stu3:datatypes_cc_proto",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_test(
    name = "profiles_test",
    srcs = [
        "profiles_test.cc",
    ],
    data = [
        "//testdata/stu3:examples",
        "//testdata/stu3/profiles:testdata",
    ],
    deps = [
        ":profiles",
        "//cc/google/fhir:test_helper",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:extensions_cc_proto",
        "//proto/stu3:profiles_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "//proto/stu3:uscore_cc_proto",
        "//testdata/stu3/profiles:test_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "primitive_handler",
    srcs = ["primitive_handler.cc"],
    hdrs = ["primitive_handler.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:codes",
        "//cc/google/fhir:extensions",
        "//cc/google/fhir:fhir_types",
        "//cc/google/fhir:primitive_handler",
        "//cc/google/fhir:primitive_wrapper",
        "//cc/google/fhir:proto_util",
        "//cc/google/fhir:util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto:annotations_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:extensions_cc_proto",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
        "@com_googlesource_code_re2//:re2",
        "@jsoncpp_git//:jsoncpp",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_test(
    name = "primitive_handler_test",
    srcs = ["primitive_handler_test.cc"],
    data = [
        "//testdata/stu3:validation",
    ],
    deps = [
        ":primitive_handler",
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:json_format",
        "//cc/google/fhir:test_helper",
        "//proto/stu3:codes_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:extensions_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:test",
    ],
)
