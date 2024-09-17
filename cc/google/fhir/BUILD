licenses(["notice"])

package(default_visibility = ["//visibility:public"])

package_group(
    name = "internal",
    packages = [
        "//cc/google/fhir/...",
    ],
)

cc_library(
    name = "codes",
    srcs = ["codes.cc"],
    hdrs = ["codes.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        ":fhir_types",
        ":proto_util",
        ":util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/google/fhir/proto:annotations_cc_proto",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "codes_test",
    srcs = ["codes_test.cc"],
    data = [
        "//testdata/r4/codes:testdata",
    ],
    deps = [
        ":codes",
        ":proto_util",
        ":test_helper",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/r4:uscore_cc_proto",
        "//proto/google/fhir/proto/r4/core:codes_cc_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:encounter_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:family_member_history_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:metadata_resource_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_cc_proto",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
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
        ":util",
        "//cc/google/fhir/status:statusor",
        "//proto/google/fhir/proto:annotations_cc_proto",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "references",
    srcs = ["references.cc"],
    hdrs = ["references.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        ":core_resource_registry",
        ":proto_util",
        ":util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@com_google_protobuf//:protobuf",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_test(
    name = "references_test",
    srcs = ["references_test.cc"],
    deps = [
        ":references",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/stu3:codes_cc_proto",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
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
        ":codes",
        ":error_reporter",
        ":extensions",
        ":fhir_types",
        ":profiled_extensions",
        ":proto_util",
        ":resource_validation",
        ":util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/google/fhir/proto:annotations_cc_proto",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
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
        ":type_macros",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/google/fhir/proto:annotations_cc_proto",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_test(
    name = "util_test",
    srcs = ["util_test.cc"],
    deps = [
        ":test_helper",
        ":util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:medication_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:medication_request_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_cc_proto",
        "//proto/google/fhir/proto/stu3:codes_cc_proto",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "//testdata/r4/profiles:test_cc_proto",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "resource_validation",
    srcs = ["resource_validation.cc"],
    hdrs = ["resource_validation.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        ":error_reporter",
        ":primitive_handler",
        ":proto_util",
        ":util",
        "//cc/google/fhir/fhir_path:fhir_path_validation",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/google/fhir/proto:annotations_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
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
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_test(
    name = "proto_util_test",
    srcs = ["proto_util_test.cc"],
    deps = [
        ":proto_util",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "test_helper",
    testonly = 1,
    hdrs = ["test_helper.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        "//cc/google/fhir/r4:primitive_handler",
        "//cc/google/fhir/r4:profiles",
        "//cc/google/fhir/r4:resource_validation",
        "//cc/google/fhir/status",
        "//cc/google/fhir/stu3:primitive_handler",
        "//cc/google/fhir/stu3:profiles",
        "//cc/google/fhir/stu3:resource_validation",
        "//proto/google/fhir/proto:annotations_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
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
        ":error_reporter",
        ":extensions",
        ":fhir_types",
        ":json_format_results",
        ":proto_util",
        ":type_macros",
        ":util",
        "//cc/google/fhir/json:fhir_json",
        "//cc/google/fhir/json:json_util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/google/fhir/proto:annotations_cc_proto",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_library(
    name = "json_format_results",
    hdrs = ["json_format_results.h"],
    strip_include_prefix = "//cc/",
)

# TODO(b/244184211): eliminate version-specific deps
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
        ":core_resource_registry",
        ":error_reporter",
        ":extensions",
        ":fhir_types",
        ":json_format_results",
        ":primitive_handler",
        ":primitive_wrapper",
        ":proto_util",
        ":references",
        ":resource_validation",
        ":util",
        "//cc/google/fhir/json:fhir_json",
        "//cc/google/fhir/json:json_sax_handler",
        "//cc/google/fhir/json:json_util",
        "//cc/google/fhir/r4:codeable_concepts",
        "//cc/google/fhir/r4:profiles",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/google/fhir/proto:annotations_cc_proto",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "annotations",
    srcs = ["annotations.cc"],
    hdrs = ["annotations.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//proto/google/fhir/proto:annotations_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "annotations_test",
    srcs = ["annotations_test.cc"],
    deps = [
        ":annotations",
        "//cc/google/fhir/status",
        "//proto/google/fhir/proto:annotations_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "//testdata/r4/profiles:test_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "core_resource_registry",
    srcs = ["core_resource_registry.cc"],
    hdrs = ["core_resource_registry.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        "//cc/google/fhir/status:statusor",
        "//proto/google/fhir/proto:annotations_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "core_resource_registry_test",
    srcs = ["core_resource_registry_test.cc"],
    deps = [
        ":core_resource_registry",
        ":proto_util",
        "//proto/google/fhir/proto:profile_config_cc_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:observation_cc_proto",
        "//testdata/r4/profiles:test_cc_proto",
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
        "//proto/google/fhir/proto/r4:uscore_cc_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core:extensions_cc_proto",
        "//proto/google/fhir/proto/r4/core/profiles:observation_bodyheight_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:extensions_cc_proto",
        "//proto/google/fhir/proto/stu3:ml_extensions_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
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
        ":error_reporter",
        ":json_format_results",
        ":primitive_wrapper",
        ":proto_util",
        ":type_macros",
        ":util",
        "//cc/google/fhir/json:fhir_json",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_absl//absl/types:optional",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "type_macros",
    hdrs = ["type_macros.h"],
    strip_include_prefix = "//cc/",
)

cc_library(
    name = "error_reporter",
    srcs = ["error_reporter.cc"],
    hdrs = ["error_reporter.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//proto/google/fhir/proto:annotations_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "error_reporter_test",
    srcs = ["error_reporter_test.cc"],
    deps = [
        ":error_reporter",
        "//cc/google/fhir/status",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:observation_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fhir_package_test",
    srcs = ["fhir_package_test.cc"],
    data = [
        "@hl7.fhir.r4.core_4.0.1//file",
    ],
    deps = [
        ":fhir_package",
        "//cc/google/fhir/json:fhir_json",
        "//cc/google/fhir/json:json_sax_handler",
        "//cc/google/fhir/status:statusor",
        "//cc/google/fhir/testutil:archive",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:code_system_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:search_parameter_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:value_set_cc_proto",
        "@com_google_absl//absl/cleanup",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_googletest//:gtest_main",
        "@libarchive",
    ],
)

cc_test(
    name = "extensions_stu3_test",
    srcs = ["extensions_stu3_test.cc"],
    data = ["//testdata/stu3:extensions"],
    deps = [
        ":extensions",
        ":profiled_extensions",
        ":test_helper",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:extensions_cc_proto",
        "//proto/google/fhir/proto/stu3:fhirproto_extensions_cc_proto",
        "//proto/google/fhir/proto/stu3:ml_extensions_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "//testdata/stu3/profiles:test_extensions_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "extensions_r4_test",
    srcs = ["extensions_r4_test.cc"],
    deps = [
        ":extensions",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:binary_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:composition_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_cc_proto",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "operation_error_reporter",
    hdrs = ["operation_error_reporter.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":error_reporter",
        "//proto/google/fhir/proto/r4:fhirproto_cc_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:operation_outcome_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "fhir_package",
    srcs = ["fhir_package.cc"],
    hdrs = ["fhir_package.h"],
    strip_include_prefix = "//cc/",
    deps = [
        ":error_reporter",
        "//cc/google/fhir/json:fhir_json",
        "//cc/google/fhir/json:json_sax_handler",
        "//cc/google/fhir/r4:json_format",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:code_system_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:implementation_guide_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:search_parameter_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:value_set_cc_proto",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@libarchive",
    ],
)

cc_library(
    name = "profiled_extensions",
    srcs = ["profiled_extensions.cc"],
    hdrs = ["profiled_extensions.h"],
    data = ["//testdata/r4:extensions"],
    strip_include_prefix = "//cc/",
    deps = [
        ":annotations",
        ":codes",
        ":extensions",
        ":fhir_types",
        ":proto_util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings:string_view",
    ],
)

cc_test(
    name = "profiled_extensions_test",
    srcs = ["profiled_extensions_test.cc"],
    deps = [
        ":profiled_extensions",
        ":test_helper",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/r4:fhirproto_extensions_cc_proto",
        "//proto/google/fhir/proto/r4:ml_extensions_cc_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core:extensions_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:composition_cc_proto",
        "//testdata/r4/profiles:test_extensions_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "json_format_test",
    srcs = ["json_format_test.cc"],
    data = [
        "@hl7.fhir.r4.core_4.0.1//file",
        "@hl7.fhir.r5.core_5.0.0//file",
    ],
    shard_count = 16,
    deps = [
        ":error_reporter",
        ":json_format",
        ":json_format_results",
        ":primitive_handler",
        "//cc/google/fhir/json:fhir_json",
        "//cc/google/fhir/json:json_sax_handler",
        "//cc/google/fhir/json:test_matchers",
        "//cc/google/fhir/r4:primitive_handler",
        "//cc/google/fhir/r5:primitive_handler",
        "//cc/google/fhir/status",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:bundle_and_contained_resource_cc_proto",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
        "@libarchive",
    ],
)

cc_test(
    name = "type_macros_test",
    srcs = ["type_macros_test.cc"],
    deps = [
        ":type_macros",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:observation_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_cc_proto",
        "//proto/google/fhir/proto/r5/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:observation_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:structure_definition_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)
