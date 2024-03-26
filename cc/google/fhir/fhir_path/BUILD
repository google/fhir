load("//bazel:antlr4_cc.bzl", "antlr4_cc_lexer", "antlr4_cc_parser")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

exports_files(["FhirPath.g4"])

antlr4_cc_lexer(
    name = "fhir_path_lexer",
    src = "FhirPath.g4",
    namespaces = ["antlr_parser"],
)

antlr4_cc_parser(
    name = "fhir_path_grammar",
    src = "FhirPath.g4",
    listener = True,
    namespaces = ["antlr_parser"],
    visitor = True,
)

cc_library(
    name = "fhir_path",
    srcs = [
        "fhir_path.cc",
    ],
    hdrs = [
        # TODO(b/244184211): These may not be necessary as Bazel approaches 1.0
        "FhirPathBaseVisitor.h",
        "FhirPathLexer.h",
        "FhirPathParser.h",
        "FhirPathVisitor.h",
        "fhir_path.h",
    ],
    copts = [
        "-fexceptions",
    ],
    features = ["-use_header_modules"],  # Incompatible with -fexception
    strip_include_prefix = "//cc/",
    deps = [
        ":fhir_path_grammar",
        ":fhir_path_lexer",
        ":fhir_path_types",
        ":utils",
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:codes",
        "//cc/google/fhir:fhir_types",
        "//cc/google/fhir:primitive_handler",
        "//cc/google/fhir:proto_util",
        "//cc/google/fhir:util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//cc/google/fhir/terminology:terminology_resolver",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_absl//absl/types:optional",
        "@com_google_protobuf//:protobuf",
        "@icu//:common",
    ],
)

cc_library(
    name = "fhir_path_validation",
    srcs = [
        "fhir_path_validation.cc",
    ],
    hdrs = [
        "fhir_path_validation.h",
    ],
    strip_include_prefix = "//cc/",
    deps = [
        ":fhir_path",
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:error_reporter",
        "//cc/google/fhir:primitive_handler",
        "//cc/google/fhir:proto_util",
        "//cc/google/fhir:util",
        "//cc/google/fhir/json:json_util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//proto/google/fhir/proto:annotations_cc_proto",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "fhir_path_validation_rule",
    srcs = [
        "fhir_path_validation_rule.cc",
    ],
    hdrs = [
        "fhir_path_validation_rule.h",
    ],
    strip_include_prefix = "//cc/",
    deps = [
        ":fhir_path_validation",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/hash",
    ],
)

cc_library(
    name = "fhir_path_types",
    srcs = [
        "fhir_path_types.cc",
    ],
    hdrs = [
        "fhir_path_types.h",
    ],
    strip_include_prefix = "//cc/",
    visibility = ["//visibility:private"],
    deps = [
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:fhir_types",
        "//cc/google/fhir/status:statusor",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "utils",
    srcs = [
        "utils.cc",
    ],
    hdrs = [
        "utils.h",
    ],
    strip_include_prefix = "//cc/",
    visibility = ["//visibility:private"],
    deps = [
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:references",
        "//cc/google/fhir:util",
        "//cc/google/fhir/json:json_util",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "fhir_path_test",
    size = "small",
    srcs = [
        "fhir_path_test.cc",
    ],
    deps = [
        ":fhir_path",
        ":utils",
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:proto_util",
        "//cc/google/fhir:references",
        "//cc/google/fhir:util",
        "//cc/google/fhir/r4:primitive_handler",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//cc/google/fhir/stu3:primitive_handler",
        "//cc/google/fhir/terminology:terminology_resolver",
        "//cc/google/fhir/testutil:fhir_test_env",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/r4/core:codes_cc_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:encounter_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:medication_knowledge_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:observation_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:parameters_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:value_set_cc_proto",
        "//proto/google/fhir/proto/r5/core:codes_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:encounter_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:observation_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:parameters_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:patient_cc_proto",
        "//proto/google/fhir/proto/r5/core/resources:structure_definition_cc_proto",
        "//proto/google/fhir/proto/stu3:codes_cc_proto",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:metadatatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "//proto/google/fhir/proto/stu3:uscore_cc_proto",
        "//proto/google/fhir/proto/stu3:uscore_codes_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "fhir_path_validation_test",
    size = "small",
    srcs = [
        "fhir_path_validation_test.cc",
    ],
    deps = [
        ":fhir_path_validation",
        "//cc/google/fhir:error_reporter",
        "//cc/google/fhir/r4:primitive_handler",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "//cc/google/fhir/stu3:primitive_handler",
        "//cc/google/fhir/testutil:fhir_test_env",
        "//proto/google/fhir/proto/r4:uscore_cc_proto",
        "//proto/google/fhir/proto/r4:uscore_codes_cc_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:encounter_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:medication_knowledge_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:observation_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:organization_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:value_set_cc_proto",
        "//proto/google/fhir/proto/stu3:codes_cc_proto",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:metadatatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "//proto/google/fhir/proto/stu3:uscore_cc_proto",
        "//proto/google/fhir/proto/stu3:uscore_codes_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "fhir_path_validation_rule_test",
    size = "small",
    srcs = [
        "fhir_path_validation_rule_test.cc",
    ],
    deps = [
        ":fhir_path_validation_rule",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fhir_path_types_test",
    size = "small",
    srcs = [
        "fhir_path_types_test.cc",
    ],
    deps = [
        ":fhir_path_types",
        "//cc/google/fhir:type_macros",
        "//cc/google/fhir:util",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/stu3:codes_cc_proto",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/status:statusor",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "utils_test",
    size = "small",
    srcs = [
        "utils_test.cc",
    ],
    deps = [
        ":utils",
        "//cc/google/fhir/status",
        "//cc/google/fhir/testutil:proto_matchers",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:patient_cc_proto",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)
