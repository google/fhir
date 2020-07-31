load(
    "@rules_python//python:python.bzl",
    "py_binary",
    "py_library",
    "py_test",
)

load("@fhir_bazel_pip_dependencies//:requirements.bzl", "requirement")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

# @rules_python may not be able to pull in transitive dependency correctly
# (related issues: https://github.com/bazelbuild/rules_python/pull/90,
#  https://github.com/bazelbuild/rules_python/pull/108). One would still need to run
# pip install -r bazel/requirements.txt before running bazel on these targets.
# At the momemt, requirement() serves more like a tag.

py_library(
    name = "encounter",
    srcs = [
        "encounter.py",
    ],
    srcs_version = "PY3",
    deps = [
        "//proto/r4/core:codes_py_pb2",
        "//proto/r4/core:datatypes_py_pb2",
        "//proto/r4/core/resources:bundle_and_contained_resource_py_pb2",
        "//proto/r4/core/resources:encounter_py_pb2",
        "//proto/r4/core/resources:patient_py_pb2",
    ],
)

py_test(
    name = "encounter_test",
    srcs = [
        "encounter_test.py",
    ],
    data = [
        "//testdata/r4:labels",
    ],
    python_version = "PY3",
    deps = [
        ":encounter",
        "//proto/r4/core/resources:bundle_and_contained_resource_py_pb2",
        "//proto/r4/core/resources:encounter_py_pb2",
        "@absl_py//absl/testing:absltest",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_library(
    name = "label",
    srcs = [
        "label.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":encounter",
        "//proto/r4:ml_extensions_py_pb2",
        "//proto/r4/core:datatypes_py_pb2",
        "//proto/r4/core/resources:encounter_py_pb2",
        "//proto/r4/core/resources:patient_py_pb2",
    ],
)

py_test(
    name = "label_test",
    srcs = [
        "label_test.py",
    ],
    data = [
        "//testdata/r4:labels",
    ],
    python_version = "PY3",
    deps = [
        ":label",
        "//proto/r4:ml_extensions_py_pb2",
        "//proto/r4/core:datatypes_py_pb2",
        "//proto/r4/core/resources:encounter_py_pb2",
        "@absl_py//absl/testing:absltest",
        "@absl_py//absl/testing:parameterized",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_binary(
    name = "bundle_to_label",
    srcs = ["bundle_to_label.py"],
    python_version = "PY3",
    deps = [":bundle_to_label_lib"],
)

py_library(
    name = "bundle_to_label_lib",
    srcs = [
        "bundle_to_label.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":encounter",
        ":label",
        "//proto/r4/core/resources:bundle_and_contained_resource_py_pb2",
        "//proto/r4:ml_extensions_py_pb2",
        "@absl_py//absl:app",
        "@absl_py//absl/flags",
        requirement("apache_beam"),
    ],
)

py_test(
    name = "bundle_to_label_test",
    srcs = [
        "bundle_to_label_test.py",
    ],
    data = [
        "//testdata/r4:labels",
    ],
    python_version = "PY3",
    deps = [
        ":bundle_to_label_lib",
        ":label",
        "@com_google_protobuf//:protobuf_python",
        "//proto/r4/core/resources:bundle_and_contained_resource_py_pb2",
        "@absl_py//absl/testing:absltest",
        requirement("apache_beam"),
        requirement("nose"),
    ],
)
