load(
    "@rules_python//python:python.bzl",
    "py_library",
)
load("@fhir_bazel_pip_dependencies//:requirements.bzl", "requirement")

package(
    default_visibility = [
        "//py/google/fhir:__subpackages__",
    ],
)

licenses(["notice"])

py_library(
    name = "json_format",
    srcs = [
        "__init__.py",
        "_json_parser.py",
        "_json_printer.py",
    ],
    srcs_version = "PY3",
    deps = [
        "//py",
        "//py/google/fhir:extensions",
        "//py/google/fhir:primitive_handler",
        "//py/google/fhir:references",
        "//py/google/fhir:resource_validation",
        "//py/google/fhir/utils:annotation_utils",
        "//py/google/fhir/utils:fhir_types",
        "//py/google/fhir/utils:proto_utils",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_library(
    name = "json_format_test",
    testonly = 1,
    srcs = [
        "__init__.py",
        "json_format_test.py",
    ],
    srcs_version = "PY3",
    deps = [
        "@com_google_protobuf//:protobuf_python",
        "//py",
        "//py/google/fhir/testing:testdata_utils",
        requirement("absl-py"),
        requirement("six"),
    ],
)
