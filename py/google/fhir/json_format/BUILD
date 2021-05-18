load(
    "@rules_python//python:python.bzl",
    "py_test",
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

py_test(
    name = "json_format_primitives_test",
    srcs = ["json_format_primitives_test.py"],
    data = ["//testdata/primitives"],
    legacy_create_init = 0,
    python_version = "PY3",
    deps = [
        "//py",
        "//proto/google/fhir/proto/r4:primitive_test_suite_py_pb2",
        "//proto/google/fhir/proto/r4/core:datatypes_py_pb2",
        "//py/google/fhir:extensions",
        "//py/google/fhir:fhir_errors",
        "//py/google/fhir/r4:json_format",
        "//py/google/fhir/testing:protobuf_compare",
        "//py/google/fhir/testing:testdata_utils",
        "//py/google/fhir/utils:annotation_utils",
        "//py/google/fhir/utils:fhir_types",
        "//py/google/fhir/utils:path_utils",
        "//py/google/fhir/utils:proto_utils",
        "@com_google_protobuf//:protobuf_python",
    ],
)
