licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fhir_json",
    srcs = ["fhir_json.cc"],
    hdrs = ["fhir_json.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "fhir_json_test",
    srcs = ["fhir_json_test.cc"],
    deps = [
        ":fhir_json",
        ":test_matchers",
        "//cc/google/fhir/status",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "json_sax_handler",
    srcs = ["json_sax_handler.cc"],
    hdrs = ["json_sax_handler.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":fhir_json",
        "@com_google_absl//absl/cleanup",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:optional",
        "@nlohmann_json//:json",
    ],
)

cc_library(
    name = "test_matchers",
    testonly = 1,
    hdrs = ["test_matchers.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
    ],
)

cc_test(
    name = "json_sax_handler_test",
    srcs = ["json_sax_handler_test.cc"],
    deps = [
        ":json_sax_handler",
        ":test_matchers",
        "@com_google_absl//absl/status",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "json_util",
    srcs = ["json_util.cc"],
    hdrs = ["json_util.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir:annotations",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "json_util_test",
    srcs = ["json_util_test.cc"],
    deps = [
        ":json_util",
        "//cc/google/fhir/status",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:observation_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)
