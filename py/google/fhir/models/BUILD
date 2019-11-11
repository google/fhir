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
    name = "test_utils",
    srcs = [
        "test_utils.py",
    ],
    deps = [
        "@com_google_protobuf//:protobuf_python",
        requirement("tensorflow"),
    ],
)

py_library(
    name = "model",
    srcs = ["model.py"],
    deps = [":model_lib"],
)

py_library(
    name = "model_lib",
    srcs = [
        "model.py",
    ],
    deps = [
        requirement("tensorflow"),
    ],
)

py_binary(
    name = "mle_task",
    srcs = ["mle_task.py"],
    python_version = "PY2",
    deps = [":mle_task_main_lib"],
)

py_library(
    name = "mle_task_main_lib",
    srcs = [
        "mle_task.py",
    ],
    deps = [
        ":model",
        requirement("tensorflow"),
    ],
)

py_binary(
    name = "run_locally",
    srcs = [
        "run_locally.py",
    ],
    python_version = "PY2",
    deps = [
        ":model",
        "@absl_py//absl/flags",
        requirement("tensorflow"),
    ],
)

py_test(
    name = "model_test",
    size = "large",
    srcs = [
        "model_test.py",
    ],
    data = [
        "//testdata/stu3:tensorflow",
    ],
    python_version = "PY2",
    deps = [
        ":model_lib",
        ":test_utils",
        "@absl_py//absl/testing:absltest",
        "@absl_py//absl/testing:parameterized",
        requirement("tensorflow"),
    ],
)

py_test(
    name = "mle_task_test",
    size = "large",
    srcs = [
        "mle_task_test.py",
    ],
    data = [
        "//testdata/stu3:tensorflow",
    ],
    python_version = "PY2",
    deps = [
        ":mle_task_main_lib",
        ":test_utils",
        "@absl_py//absl/testing:absltest",
        "@absl_py//absl/testing:parameterized",
        requirement("tensorflow"),
    ],
)
