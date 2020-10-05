load(
    "@rules_python//python:python.bzl",
    "py_library",
)

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

# A Python3 library target for targets in //py/google/fhir/** to include
# as a dependency to allow for relative imports, e.g.:
# * Before: from py.google.fhir.jsonformat.core import resource_validation
# * After: from google.fhir.jsonformat.core import resource_validation
#
# See more at: https://github.com/bazelbuild/bazel/issues/6556.
py_library(
    name = "py",
    imports = ["."],
    srcs_version = "PY3",
)
