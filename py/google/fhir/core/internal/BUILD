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
    name = "primitive_time_utils",
    srcs = [
        "__init__.py",
        "_primitive_time_utils.py",
    ],
    srcs_version = "PY3",
    visibility = ["//py/google/fhir:__subpackages__"],
    deps = [
        "@com_google_protobuf//:protobuf_python",
        "//py",
        "//py/google/fhir/core/utils:proto_utils",
        requirement("backports.zoneinfo"),
        requirement("python-dateutil"),
        requirement("six"),
    ],
)

py_library(
    name = "primitive_handler",
    srcs = [
        "__init__.py",
        ":primitive_handler.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":primitive_time_utils",
        "//py",
        "//py/google/fhir/core/internal/json_format/wrappers:primitive_wrappers",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_library(
    name = "primitive_handler_test",
    testonly = 1,
    srcs = [
        "__init__.py",
        ":primitive_handler_test.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":primitive_handler",
        ":primitive_time_utils",
        "@com_google_protobuf//:protobuf_python",
        "//py",
        "//py/google/fhir/core:extensions",
        "//py/google/fhir/core:fhir_errors",
        "//py/google/fhir/core/testing:testdata_utils",
        "//py/google/fhir/core/utils:path_utils",
        "//py/google/fhir/core/utils:proto_utils",
        requirement("absl-py"),
        requirement("six"),
    ],
)

py_library(
    name = "resource_validation",
    srcs = [
        "__init__.py",
        "resource_validation.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":primitive_handler",
        ":primitive_time_utils",
        "//py",
        "//proto/google/fhir/proto:annotations_py_pb2",
        "//py/google/fhir/core:fhir_errors",
        "//py/google/fhir/core/utils:annotation_utils",
        "//py/google/fhir/core/utils:fhir_types",
        "//py/google/fhir/core/utils:proto_utils",
        "@com_google_protobuf//:protobuf_python",
    ],
)
