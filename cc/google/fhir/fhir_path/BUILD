licenses(["notice"])

package(default_visibility = ["//visibility:public"])

load("//bazel:antlr4_cc.bzl", "antlr4_cc_lexer", "antlr4_cc_parser")

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
        # TODO: These may not be necessary as Bazel approaches 1.0
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
        "//cc/google/fhir:annotations",
        "//cc/google/fhir:proto_util",
        "//cc/google/fhir/status",
        "//cc/google/fhir/status:statusor",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@org_tensorflow//tensorflow/core:lib",
        "@protobuf_archive//:protobuf",
    ],
)

cc_test(
    name = "fhir_path_test",
    size = "small",
    srcs = ["fhir_path_test.cc"],
    copts = [
        "-fexceptions",
    ],
    features = ["-use_header_modules"],  # Incompatible with -fexceptions.
    deps = [
        ":fhir_path",
        "//cc/google/fhir/status",
        "//proto/stu3:codes_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:extensions_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "//proto/stu3:version_config_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@com_google_googletest//:gtest_main",
        "@org_tensorflow//tensorflow/core:lib",
        "@protobuf_archive//:protobuf",
    ],
)
