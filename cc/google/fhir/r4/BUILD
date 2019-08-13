licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "profiles",
    srcs = [
        "profiles.cc",
    ],
    hdrs = ["profiles.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:codes",
        "//cc/google/fhir:extensions",
        "//cc/google/fhir:profiles",
        "//cc/google/fhir:profiles_lib",
        "//cc/google/fhir:proto_util",
        "//cc/google/fhir:resource_validation",
        "//cc/google/fhir:util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto:annotations_cc_proto",
        "//proto/r4:resources_cc_proto",
        "@com_google_absl//absl/strings",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_test(
    name = "profiles_test",
    srcs = [
        "profiles_test.cc",
    ],
    data = [
        "//testdata/r4:examples",
        "//testdata/r4/profiles:testdata",
    ],
    deps = [
        ":profiles",
        "//cc/google/fhir:test_helper",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/r4:datatypes_cc_proto",
        "//proto/r4:extensions_cc_proto",
        "//proto/r4:resources_cc_proto",
        "//proto/r4:uscore_cc_proto",
        "//testdata/r4/profiles:test_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)
