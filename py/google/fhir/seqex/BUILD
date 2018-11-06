licenses(["notice"])

package(default_visibility = ["//visibility:public"])

load("//bazel:cython.bzl", "pyx_library")

load(
 "@io_bazel_rules_python//python:python.bzl",
 "py_binary", "py_library", "py_test",
)

load("@fhir_bazel_pip_dependencies//:requirements.bzl", "requirement")

# @io_bazel_rules_python may not be able to pull in transitive dependency correctly
# (related issues: https://github.com/bazelbuild/rules_python/pull/90,
#  https://github.com/bazelbuild/rules_python/pull/108). One would still need to run
# pip install -r bazel/requirements.txt before running bazel on these targets.
# At the momemt, requirement() serves more like a tag.

pyx_library(
    name = "bundle_seqex_converter",
    srcs = [
        "bundle_to_seqex_converter.pxd",
        "bundle_to_seqex_converter.pyx",
    ],
    py_deps = [
        "@org_tensorflow//tensorflow/core:protos_all_py",
    ],
    deps = [
        "//cc/google/fhir/seqex:bundle_to_seqex_converter",
        "//proto/stu3:extensions_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "//proto/stu3:version_config_cc_proto",
        "@org_tensorflow//tensorflow/core:protos_all_cc",
    ],
)

py_test(
    name = "bundle_to_seqex_converter_test",
    srcs = ["bundle_to_seqex_converter_test.py"],
    data = [
        "//proto/stu3:version_config.textproto",
    ],
    deps = [
        ":bundle_seqex_converter",
        "//proto/stu3:extensions_py_pb2",
        "//proto/stu3:resources_py_pb2",
        "//proto/stu3:version_config_py_pb2",
        "//py/google/fhir/testutil:protobuf_compare",
        "@absl_py//absl/flags",
        "@absl_py//absl/testing:absltest",
        "@org_tensorflow//tensorflow/core:protos_all_py",
        "@protobuf_archive//:protobuf_python",
    ],
)

pyx_library(
    name = "bundle_to_seqex_util",
    srcs = [
        "bundle_to_seqex_util.pxd",
        "bundle_to_seqex_util.pyx",
    ],
    py_deps = [
        "//proto/stu3:extensions_py_pb2",
    ],
    deps = [
        "//cc/google/fhir/seqex:bundle_to_seqex_util",
        "//proto/stu3:extensions_cc_proto",
        "//proto/stu3:resources_cc_proto",
    ],
)

py_test(
    name = "bundle_to_seqex_util_test",
    srcs = ["bundle_to_seqex_util_test.py"],
    deps = [
        ":bundle_to_seqex_util",
        "//proto/stu3:extensions_py_pb2",
        "//proto/stu3:resources_py_pb2",
        "//py/google/fhir/testutil:protobuf_compare",
        "@absl_py//absl/testing:absltest",
        "@protobuf_archive//:protobuf_python",
    ],
)

py_library(
    name = "bundle_to_seqex",
    srcs = [
        "bundle_to_seqex.py",
    ],
    deps = [
        ":bundle_seqex_converter",
        ":bundle_to_seqex_util",
        "//proto/stu3:extensions_py_pb2",
        "//proto/stu3:resources_py_pb2",
        "//py/google/fhir/stu3:util",
        "@org_tensorflow//tensorflow/core:protos_all_py",
        requirement("apache_beam"),
    ],
)

py_test(
    name = "bundle_to_seqex_test",
    srcs = [
        "bundle_to_seqex_test.py",
    ],
    data = [
        "//proto/stu3:version_config.textproto",
    ],
    deps = [
        ":bundle_to_seqex",
        ":bundle_seqex_converter",
        "@protobuf_archive//:protobuf_python",
        "//proto/stu3:extensions_py_pb2",
        "//proto/stu3:resources_py_pb2",
        "//proto/stu3:version_config_py_pb2",
        "//py/google/fhir/testutil:protobuf_compare",
        "@absl_py//absl/testing:absltest",
        "@org_tensorflow//tensorflow/core:protos_all_py",
        requirement("apache_beam"),
        requirement("nose"),
    ],
)

# Libraries and binaries for building and running tensorflow models on medical records.

py_binary(
    name = "bundle_to_seqex_main",
    srcs = [
        "bundle_to_seqex_main.py",
    ],
    deps = [
        ":bundle_to_seqex",
        "@protobuf_archive//:protobuf_python",
        "//proto/stu3:extensions_py_pb2",
        "//proto/stu3:resources_py_pb2",
        "//proto/stu3:version_config_py_pb2",
        "@absl_py//absl:app",
        "@absl_py//absl/flags",
        "@absl_py//absl/logging",
        "@org_tensorflow//tensorflow/core:protos_all_py",
        requirement("apache_beam"),
    ],
)
