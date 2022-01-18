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
    name = "proto_utils",
    srcs = [
        "__init__.py",
        "proto_utils.py",
    ],
    srcs_version = "PY3",
    deps = [
        "//py",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_library(
    name = "fhir_types",
    srcs = [
        "__init__.py",
        "fhir_types.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":annotation_utils",
        "//py",
        "//proto/google/fhir/proto:annotations_py_pb2",
    ],
)

py_library(
    name = "annotation_utils",
    srcs = [
        "__init__.py",
        "annotation_utils.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":proto_utils",
        "//py",
        "//proto/google/fhir/proto:annotations_py_pb2",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_library(
    name = "path_utils",
    srcs = [
        "__init__.py",
        "path_utils.py",
    ],
    srcs_version = "PY3",
    deps = ["//py"],
)

py_library(
    name = "resource_utils",
    srcs = [
        "__init__.py",
        "resource_utils.py",
    ],
    srcs_version = "PY3",
    deps = [
        ":fhir_types",
        ":path_utils",
        ":proto_utils",
        "//py",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_library(
    name = "fhir_package",
    srcs = [
        "__init__.py",
        "fhir_package.py",
    ],
    srcs_version = "PY3",
    deps = [
        "//py",
        "//proto/google/fhir/proto:annotations_py_pb2",
        "//proto/google/fhir/proto:profile_config_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:code_system_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:search_parameter_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:value_set_py_pb2",
        "//py/google/fhir/r4:json_format",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_test(
    name = "proto_utils_test",
    size = "small",
    srcs = [
        "proto_utils_test.py",
    ],
    legacy_create_init = 0,
    python_version = "PY3",
    deps = [
        "//py",
        "//proto/google/fhir/proto/r4:uscore_py_pb2",
        "//proto/google/fhir/proto/r4/core:codes_py_pb2",
        "//proto/google/fhir/proto/r4/core:datatypes_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:patient_py_pb2",
        "//py/google/fhir/utils:proto_utils",
        requirement("absl-py"),
        requirement("six"),
    ],
)

py_test(
    name = "fhir_types_test",
    srcs = [
        "fhir_types_test.py",
    ],
    legacy_create_init = 0,
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        "//py",
        "//proto/google/fhir/proto/r4:fhirproto_extensions_py_pb2",
        "//proto/google/fhir/proto/r4:uscore_py_pb2",
        "//proto/google/fhir/proto/r4/core:datatypes_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:patient_py_pb2",
        "//py/google/fhir/utils:fhir_types",
        requirement("absl-py"),
        requirement("six"),
    ],
)

py_test(
    name = "annotation_utils_test",
    srcs = [
        "annotation_utils_test.py",
    ],
    legacy_create_init = 0,
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        "//py",
        "//proto/google/fhir/proto/r4:uscore_py_pb2",
        "//proto/google/fhir/proto/r4/core:datatypes_py_pb2",
        "//proto/google/fhir/proto/r4/core:valuesets_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:observation_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:patient_py_pb2",
        "//py/google/fhir/utils:annotation_utils",
        "//testdata/r4/profiles:test_py_pb2",
        requirement("absl-py"),
        requirement("six"),
    ],
)

py_test(
    name = "path_utils_test",
    srcs = [
        "path_utils_test.py",
    ],
    legacy_create_init = 0,
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        "//py",
        "//py/google/fhir/utils:path_utils",
        requirement("absl-py"),
        requirement("six"),
    ],
)

py_test(
    name = "resource_utils_test",
    srcs = [
        "resource_utils_test.py",
    ],
    legacy_create_init = 0,
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":resource_utils",
        "//py",
        "//proto/google/fhir/proto/r4/core/resources:account_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:activity_definition_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_py_pb2",
        "//proto/google/fhir/proto/r4/core:datatypes_py_pb2",
        requirement("absl-py"),
        requirement("six"),
    ],
)

py_test(
    name = "fhir_package_test",
    size = "medium",
    srcs = [
        "fhir_package_test.py",
    ],
    data = [
        "//spec:fhir_r4_package.zip",
    ],
    legacy_create_init = 0,
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":fhir_package",
        ":proto_utils",
        "//py",
        "//proto/google/fhir/proto:profile_config_py_pb2",
        "//proto/google/fhir/proto/r4/core/resources:value_set_py_pb2",
        requirement("absl-py"),
        requirement("six"),
    ],
)
