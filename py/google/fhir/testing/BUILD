load(
    "@rules_python//python:python.bzl",
    "py_library",
)

package(
    default_testonly = 1,
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

py_library(
    name = "protobuf_compare",
    srcs = ["protobuf_compare.py"],
    srcs_version = "PY3",
    deps = [
        "@absl_py//absl/testing:absltest",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_library(
    name = "testdata_utils",
    srcs = ["testdata_utils.py"],
    srcs_version = "PY3",
    deps = [
        "@absl_py//absl/flags",
        "@com_google_protobuf//:protobuf_python",
    ],
)
