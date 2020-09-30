load(
    "@rules_python//python:python.bzl",
    "py_library",
)
load("@fhir_bazel_pip_dependencies//:requirements.bzl", "requirement")

package(
    default_testonly = 1,
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

py_library(
    name = "protobuf_compare",
    srcs = [
        "__init__.py",
        "protobuf_compare.py",
    ],
    srcs_version = "PY3",
    deps = [
        "@com_google_protobuf//:protobuf_python",
        "//py",
        requirement("absl-py"),
        requirement("six"),
    ],
)

py_library(
    name = "testdata_utils",
    srcs = [
        "__init__.py",
        "testdata_utils.py",
    ],
    srcs_version = "PY3",
    deps = [
        "@com_google_protobuf//:protobuf_python",
        "//py",
        requirement("absl-py"),
        requirement("six"),
    ],
)
