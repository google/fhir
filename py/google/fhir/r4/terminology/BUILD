load(
    "@rules_python//python:python.bzl",
    "py_library",
    "py_test",
)
load("@fhir_bazel_pip_dependencies//:requirements.bzl", "requirement")

package(
    default_visibility = [
        "//visibility:public",
    ],
)

licenses(["notice"])

py_library(
    name = "terminology_service_client",
    srcs = [
        "terminology_service_client.py",
    ],
    srcs_version = "PY3",
    deps = [
        "//py",
        "//proto/google/fhir/proto/r4/core/resources:value_set_py_pb2",
        "//py/google/fhir/r4:json_format",
        "//py/google/fhir/core/utils:url_utils",
        requirement("absl-py"),
        requirement("requests"),
    ],
)

py_test(
    name = "terminology_service_client_test",
    srcs = ["terminology_service_client_test.py"],
    legacy_create_init = 0,
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":terminology_service_client",
        "//py",
        "//proto/google/fhir/proto/r4/core/resources:value_set_py_pb2",
        requirement("absl-py"),
    ],
)

py_library(
    name = "value_sets",
    srcs = [
        "__init__.py",
        "value_sets.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":terminology_service_client",
        "//py",
        "//proto/google/fhir/proto/r4/core/resources:code_system_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:value_set_py_pb2",
        "//py/google/fhir/core/utils:fhir_package",
        "//py/google/fhir/core/utils:proto_utils",
        "//py/google/fhir/core/utils:url_utils",
        requirement("absl-py"),
    ],
)

py_test(
    name = "value_sets_test",
    srcs = ["value_sets_test.py"],
    data = [
        "//spec:fhir_r4_package.zip",
    ],
    legacy_create_init = 0,
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":terminology_service_client",
        ":value_sets",
        "//py",
        "//proto/google/fhir/proto/r4/core:datatypes_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:code_system_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:value_set_py_pb2",
        "//py/google/fhir/core/utils:fhir_package",
        requirement("absl-py"),
        requirement("six"),
    ],
)

py_library(
    name = "value_set_tables",
    srcs = [
        "__init__.py",
        "value_set_tables.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":terminology_service_client",
        ":value_sets",
        "//py",
        "//proto/google/fhir/proto/r4/core/resources:value_set_py_pb2",
        "//py/google/fhir/core/utils:url_utils",
        requirement("absl-py"),
        requirement("google-cloud-bigquery"),
        requirement("sqlalchemy"),
        requirement("sqlalchemy-bigquery"),
    ],
)
