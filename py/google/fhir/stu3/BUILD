load(
    "@rules_python//python:python.bzl",
    "py_library",
    "py_test",
)

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

py_library(
    name = "util",
    srcs = [
        "util.py",
    ],
    srcs_version = "PY3",
    deps = [
        "//proto/stu3:resources_py_pb2",
    ],
)

py_test(
    name = "util_test",
    srcs = [
        "util_test.py",
    ],
    python_version = "PY3",
    deps = [
        ":util",
        "//proto/stu3:resources_py_pb2",
        "@absl_py//absl/testing:absltest",
        "@com_google_protobuf//:protobuf_python",
    ],
)
