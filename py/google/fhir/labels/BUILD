licenses(["notice"])

package(default_visibility = ["//visibility:public"])

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

py_library(
    name = "encounter",
    srcs = [
        "encounter.py",
    ],
    deps = [
        "//proto/stu3:codes_py_pb2",
        "//proto/stu3:datatypes_py_pb2",
        "//proto/stu3:resources_py_pb2",
    ],
)

py_test(
    name = "encounter_test",
    srcs = [
        "encounter_test.py",
    ],
    data = [
        "//testdata/stu3:labels",
    ],
    deps = [
        ":encounter",
        "//proto/stu3:resources_py_pb2",
        "@absl_py//absl/testing:absltest",
        "@protobuf_archive//:protobuf_python",
    ],
)

py_library(
    name = "label",
    srcs = [
        "label.py",
    ],
    deps = [
        ":encounter",
        "//proto/stu3:datatypes_py_pb2",
        "//proto/stu3:extensions_py_pb2",
        "//proto/stu3:resources_py_pb2",
    ],
)

py_test(
    name = "label_test",
    srcs = [
        "label_test.py",
    ],
    data = [
        "//testdata/stu3:labels",
    ],
    deps = [
        ":label",
        "//proto/stu3:datatypes_py_pb2",
        "//proto/stu3:extensions_py_pb2",
        "//proto/stu3:resources_py_pb2",
        "@absl_py//absl/testing:absltest",
        "@absl_py//absl/testing:parameterized",
        "@protobuf_archive//:protobuf_python",
    ],
)

py_binary(
    name = "bundle_to_label",
    srcs = [
        "bundle_to_label.py",
    ],
    deps = [
        ":encounter",
        ":label",
        "//proto/stu3:extensions_py_pb2",
        "//proto/stu3:resources_py_pb2",
        "@absl_py//absl:app",
        "@absl_py//absl/flags",
        "@absl_py//absl/logging",
        requirement("apache_beam"),
    ],
)

py_test(
    name = "bundle_to_label_test",
    srcs = [
        "bundle_to_label_test.py",
    ],
    data = [
        "//testdata/stu3:labels",
    ],
    deps = [
        ":bundle_to_label",
        ":label",
        "@protobuf_archive//:protobuf_python",
        "//proto/stu3:resources_py_pb2",
        "@absl_py//absl/testing:absltest",
        requirement("apache_beam"),
        requirement("nose"),
    ],
)
