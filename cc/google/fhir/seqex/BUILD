licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "bundle_to_seqex_converter",
    srcs = ["bundle_to_seqex_converter.cc"],
    hdrs = ["bundle_to_seqex_converter.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":converter_types",
        ":example_key",
        ":feature_keys",
        ":resource_to_example",
        "//cc/google/fhir:bundle_to_versioned_resources_converter",
        "//cc/google/fhir:references",
        "//cc/google/fhir:type_macros",
        "//cc/google/fhir:util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto:version_config_cc_proto",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
        "@org_tensorflow//tensorflow/core:protos_all_cc",
    ],
)

cc_test(
    name = "bundle_to_seqex_converter_test",
    srcs = ["bundle_to_seqex_converter_test.cc"],
    data = [
        "//proto/r4:version_config.textproto",
        "//proto/stu3:version_config.textproto",
    ],
    deps = [
        ":bundle_to_seqex_converter",
        ":converter_types",
        ":example_key",
        ":r4",
        ":stu3",
        ":text_tokenizer",
        "//cc/google/fhir:test_helper",
        "//cc/google/fhir:type_macros",
        "//cc/google/fhir/r4:primitive_handler",
        "//cc/google/fhir/testutil:fhir_test_env",
        "//cc/google/fhir/testutil:proto_matchers",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
        "@org_tensorflow//tensorflow/core:protos_all_cc",
    ],
)

cc_library(
    name = "bundle_to_seqex_util",
    hdrs = ["bundle_to_seqex_util.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir:extensions",
        "//cc/google/fhir:util",
        "//cc/google/fhir/status:statusor",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "bundle_to_seqex_util_test",
    srcs = ["bundle_to_seqex_util_test.cc"],
    deps = [
        ":bundle_to_seqex_util",
        ":r4",
        ":stu3",
        "//cc/google/fhir/testutil:fhir_test_env",
        "//cc/google/fhir/testutil:proto_matchers",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "example_key",
    srcs = ["example_key.cc"],
    hdrs = ["example_key.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_googlesource_code_re2//:re2",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_test(
    name = "example_key_test",
    srcs = ["example_key_test.cc"],
    deps = [
        ":example_key",
        "@com_google_googletest//:gtest_main",
    ],
)

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
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:codeable_concepts",
        "//cc/google/fhir:codes",
        "//cc/google/fhir:fhir_types",
        "//cc/google/fhir:primitive_handler",
        "//cc/google/fhir:proto_util",
        "//cc/google/fhir:util",
        "//cc/google/fhir/status",
        "//proto:annotations_cc_proto",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
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
        ":text_tokenizer",
        "//cc/google/fhir:test_helper",
        "//cc/google/fhir/r4:primitive_handler",
        "//cc/google/fhir/testutil:fhir_test_env",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/r4/core/resources:binary_cc_proto",
        "//proto/r4/core/resources:encounter_cc_proto",
        "//proto/r4/core/resources:observation_cc_proto",
        "//proto/r4/core/resources:patient_cc_proto",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/memory",
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
        "@com_google_absl//absl/flags:flag",
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
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "converter_types",
    hdrs = ["converter_types.h"],
    strip_include_prefix = "//cc/",
)

cc_library(
    name = "stu3",
    hdrs = ["stu3.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":bundle_to_seqex_converter",
        ":bundle_to_seqex_util",
        ":converter_types",
        ":resource_to_example",
        ":text_tokenizer",
        "//cc/google/fhir/stu3:primitive_handler",
        "//proto/stu3:fhirproto_extensions_cc_proto",
        "//proto/stu3:ml_extensions_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/status",
    ],
)

cc_library(
    name = "r4",
    hdrs = ["r4.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":bundle_to_seqex_converter",
        ":bundle_to_seqex_util",
        ":converter_types",
        ":resource_to_example",
        ":text_tokenizer",
        "//cc/google/fhir/r4:primitive_handler",
        "//proto/r4:ml_extensions_cc_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/r4/core/resources:claim_cc_proto",
        "//proto/r4/core/resources:composition_cc_proto",
        "//proto/r4/core/resources:condition_cc_proto",
        "//proto/r4/core/resources:encounter_cc_proto",
        "//proto/r4/core/resources:medication_administration_cc_proto",
        "//proto/r4/core/resources:medication_cc_proto",
        "//proto/r4/core/resources:medication_request_cc_proto",
        "//proto/r4/core/resources:observation_cc_proto",
        "//proto/r4/core/resources:patient_cc_proto",
        "//proto/r4/core/resources:procedure_cc_proto",
        "//proto/r4/core/resources:service_request_cc_proto",
    ],
)
