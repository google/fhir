licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "codes",
    srcs = ["codes.cc"],
    hdrs = ["codes.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        ":proto_util",
        ":util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto:annotations_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_test(
    name = "codes_test",
    srcs = ["codes_test.cc"],
    data = [
        "//testdata/r4/codes:testdata",
        "//testdata/stu3:google",
    ],
    deps = [
        ":codes",
        ":proto_util",
        ":test_helper",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/r4:uscore_cc_proto",
        "//proto/r4/core:codes_cc_proto",
        "//proto/r4/core:datatypes_cc_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/r4/core/resources:encounter_cc_proto",
        "//proto/r4/core/resources:family_member_history_cc_proto",
        "//proto/r4/core/resources:metadata_resource_cc_proto",
        "//proto/r4/core/resources:patient_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "extensions",
    srcs = ["extensions.cc"],
    hdrs = ["extensions.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        ":codes",
        ":fhir_types",
        ":proto_util",
        ":util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto:annotations_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_library(
    name = "profiles_lib",
    srcs = ["profiles_lib.cc"],
    hdrs = ["profiles_lib.h"],
    strip_include_prefix = "//cc/",
    visibility = [
        "//cc/google/fhir/r4:__pkg__",
        "//cc/google/fhir/stu3:__pkg__",
    ],
    deps = [
        ":annotations",
        ":codeable_concepts",
        ":codes",
        ":core_resource_registry",
        ":extensions",
        ":proto_util",
        ":resource_validation",
        ":util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto:annotations_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_library(
    name = "util",
    srcs = ["util.cc"],
    hdrs = ["util.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        ":proto_util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto:annotations_cc_proto",
        "//proto/r4/core:datatypes_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
        "@com_googlesource_code_re2//:re2",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_test(
    name = "util_test",
    srcs = ["util_test.cc"],
    deps = [
        ":util",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/stu3:codes_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:metadatatypes_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_library(
    name = "resource_validation",
    srcs = ["resource_validation.cc"],
    hdrs = ["resource_validation.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        ":primitive_handler",
        ":primitive_wrapper",
        ":proto_util",
        ":util",
        "//cc/google/fhir/fhir_path",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto:annotations_cc_proto",
        "//proto/r4/core:datatypes_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
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
        "@com_google_protobuf//:protobuf",
        "@com_googlesource_code_re2//:re2",
        "@org_tensorflow//tensorflow/core:lib",
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
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_library(
    name = "test_helper",
    testonly = 1,
    hdrs = ["test_helper.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        ":resource_validation",
        "//cc/google/fhir/r4:primitive_handler",
        "//cc/google/fhir/r4:profiles",
        "//cc/google/fhir/r4:resource_validation",
        "//cc/google/fhir/status",
        "//cc/google/fhir/stu3:primitive_handler",
        "//cc/google/fhir/stu3:profiles",
        "//cc/google/fhir/stu3:resource_validation",
        "//proto:annotations_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
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
        "//proto:version_config_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
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
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_library(
    name = "primitive_wrapper",
    srcs = [
        "primitive_wrapper.cc",
    ],
    hdrs = [
        "primitive_wrapper.h",
    ],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        ":codes",
        ":extensions",
        ":fhir_types",
        ":proto_util",
        ":util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto:annotations_cc_proto",
        "//proto/r4:google_extensions_cc_proto",
        "//proto/r4/core:datatypes_cc_proto",
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

# TODO: eliminate version-specific deps
cc_library(
    name = "json_format",
    srcs = [
        "json_parser.cc",
        "json_printer.cc",
    ],
    hdrs = [
        "json_format.h",
    ],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        ":codeable_concepts",
        ":core_resource_registry",
        ":extensions",
        ":fhir_types",
        ":primitive_handler",
        ":primitive_wrapper",
        ":proto_util",
        ":resource_validation",
        ":util",
        "//cc/google/fhir/r4:profiles",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//cc/google/fhir/stu3:profiles",
        "//proto:annotations_cc_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:extensions_cc_proto",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
        "@com_googlesource_code_re2//:re2",
        "@jsoncpp_git//:jsoncpp",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_library(
    name = "codeable_concepts",
    srcs = ["codeable_concepts.cc"],
    hdrs = ["codeable_concepts.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        ":proto_util",
        "//cc/google/fhir/r4:codeable_concepts",
        "//cc/google/fhir/status:statusor",
        "//cc/google/fhir/stu3:codeable_concepts",
        "//proto:annotations_cc_proto",
        "//proto/r4/core:datatypes_cc_proto",
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
        "//testdata/r4/profiles:testdata",
        "//testdata/stu3/profiles:testdata",
    ],
    deps = [
        ":codeable_concepts",
        ":test_helper",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/r4/core:datatypes_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "//testdata/r4/profiles:test_cc_proto",
        "//testdata/stu3/profiles:test_cc_proto",
        "@com_google_googletest//:gtest_main",
        "@org_tensorflow//tensorflow/core:test",
    ],
)

cc_library(
    name = "annotations",
    srcs = ["annotations.cc"],
    hdrs = ["annotations.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//proto:annotations_cc_proto",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "core_resource_registry",
    srcs = ["core_resource_registry.cc"],
    hdrs = ["core_resource_registry.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        ":util",
        "//cc/google/fhir/status:statusor",
        "//proto:annotations_cc_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/memory",
        "@com_google_protobuf//:protobuf",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)

cc_test(
    name = "core_resource_registry_test",
    srcs = ["core_resource_registry_test.cc"],
    deps = [
        ":core_resource_registry",
        ":proto_util",
        "//proto:profile_config_cc_proto",
        "//proto/r4/core:datatypes_cc_proto",
        "//proto/r4/core/resources:observation_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "//testdata/r4/profiles:test_cc_proto",
        "//testdata/stu3/profiles:test_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "fhir_types",
    srcs = ["fhir_types.cc"],
    hdrs = ["fhir_types.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "fhir_types_test",
    srcs = ["fhir_types_test.cc"],
    deps = [
        ":fhir_types",
        "//proto/r4:uscore_cc_proto",
        "//proto/r4/core:datatypes_cc_proto",
        "//proto/r4/core:extensions_cc_proto",
        "//proto/r4/core/profiles:observation_bodyheight_cc_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:extensions_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "//testdata/r4/profiles:test_cc_proto",
        "//testdata/stu3/profiles:test_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "primitive_handler",
    srcs = ["primitive_handler.cc"],
    hdrs = ["primitive_handler.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        ":primitive_wrapper",
        ":proto_util",
        ":util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
        "@jsoncpp_git//:jsoncpp",
        "@org_tensorflow//tensorflow/core:lib",
    ],
)
