load(
    "@rules_python//python:python.bzl",
    "py_library",
    "py_test",
)
load("@fhir_bazel_pip_dependencies//:requirements.bzl", "requirement")

package(default_visibility = [
    "//py/google/fhir:__subpackages__",
])

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
        "//py/google/fhir/core:extensions",
        "//py/google/fhir/core:references",
        "//py/google/fhir/core/internal:primitive_handler",
        "//py/google/fhir/core/utils:annotation_utils",
        "//py/google/fhir/core/utils:fhir_types",
        "//py/google/fhir/core/utils:proto_utils",
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
        "//py",
        "//py/google/fhir/core/testing:testdata_utils",
        "@com_google_protobuf//:protobuf_python",
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
        "//proto/google/fhir/proto/r4:primitive_test_suite_py_pb2",
        "//proto/google/fhir/proto/r4/core:datatypes_py_pb2",
        "//py",
        "//py/google/fhir/core:extensions",
        "//py/google/fhir/core:fhir_errors",
        "//py/google/fhir/core/testing:protobuf_compare",
        "//py/google/fhir/core/testing:testdata_utils",
        "//py/google/fhir/core/utils:annotation_utils",
        "//py/google/fhir/core/utils:fhir_types",
        "//py/google/fhir/core/utils:path_utils",
        "//py/google/fhir/core/utils:proto_utils",
        "//py/google/fhir/r4:json_format",
        "@com_google_protobuf//:protobuf_python",
    ],
)
