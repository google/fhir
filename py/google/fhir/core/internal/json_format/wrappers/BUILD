load(
    "@rules_python//python:python.bzl",
    "py_library",
)

package(default_visibility = [
    "//py/google/fhir:__subpackages__",
])

licenses(["notice"])

py_library(
    name = "base64_binary",
    srcs = [
        "__init__.py",
        "_base64_binary.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":primitive_wrappers",
        "//py",
        "//py/google/fhir/core:extensions",
        "//py/google/fhir/core:fhir_errors",
        "//py/google/fhir/core/utils:proto_utils",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_library(
    name = "date",
    srcs = [
        "__init__.py",
        "_date.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":primitive_wrappers",
        "//py",
        "//py/google/fhir/core:fhir_errors",
        "//py/google/fhir/core/internal:primitive_time_utils",
        "//py/google/fhir/core/utils:proto_utils",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_library(
    name = "date_time",
    srcs = [
        "__init__.py",
        "_date_time.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":primitive_wrappers",
        "//py",
        "//py/google/fhir/core:fhir_errors",
        "//py/google/fhir/core/internal:primitive_time_utils",
        "//py/google/fhir/core/utils:proto_utils",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_library(
    name = "decimal",
    srcs = [
        "__init__.py",
        "_decimal.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":primitive_wrappers",
        "//py",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_library(
    name = "instant",
    srcs = [
        "__init__.py",
        "_instant.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":primitive_wrappers",
        "//py",
        "//py/google/fhir/core:fhir_errors",
        "//py/google/fhir/core/internal:primitive_time_utils",
        "//py/google/fhir/core/utils:proto_utils",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_library(
    name = "primitive_wrappers",
    srcs = [
        "__init__.py",
        "_primitive_wrappers.py",
    ],
    srcs_version = "PY3",
    deps = [
        "//py",
        "//py/google/fhir/core:codes",
        "//py/google/fhir/core:extensions",
        "//py/google/fhir/core:fhir_errors",
        "//py/google/fhir/core/utils:annotation_utils",
        "//py/google/fhir/core/utils:proto_utils",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_library(
    name = "time",
    srcs = [
        "__init__.py",
        "_time.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":primitive_wrappers",
        "//py",
        "//py/google/fhir/core:fhir_errors",
        "//py/google/fhir/core/internal:primitive_time_utils",
        "//py/google/fhir/core/utils:proto_utils",
        "@com_google_protobuf//:protobuf_python",
    ],
)
