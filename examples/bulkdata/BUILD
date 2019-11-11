# Simple client to download data from FHIR server (using bulk data protocol)
licenses(["notice"])

load("@rules_python//python:python.bzl", "py_binary")
load("@fhir_bazel_pip_dependencies//:requirements.bzl", "requirement")

# @rules_python may not be able to pull in transitive dependency correctly
# (related issues: https://github.com/bazelbuild/rules_python/pull/90,
#  https://github.com/bazelbuild/rules_python/pull/108). One would still need to run
# pip install -r bazel/requirements.txt before running bazel on these targets.
# At the momemt, requirement() serves more like a tag.

py_binary(
    name = "bulkload",
    srcs = ["bulkload.py"],
    python_version = "PY2",
    deps = [
        "@absl_py//absl:app",
        "@absl_py//absl/flags",
        requirement("request"),
    ],
)
