package(default_visibility = ["//visibility:public"])

licenses(["notice"])

exports_files(["version_config.textproto"])

load("@com_google_protobuf//:protobuf.bzl", "py_proto_library")

proto_library(
    name = "annotations_proto",
    srcs = [
        "annotations.proto",
    ],
    deps = [
        "@com_google_protobuf//:descriptor_proto",
    ],
)

cc_proto_library(
    name = "annotations_cc_proto",
    deps = [":annotations_proto"],
)

java_proto_library(
    name = "annotations_java_proto",
    deps = [":annotations_proto"],
)

py_proto_library(
    name = "annotations_py_pb2",
    srcs = ["annotations.proto"],
    default_runtime = "@com_google_protobuf//:protobuf_python",
    protoc = "@com_google_protobuf//:protoc",
    deps = [
        "@com_google_protobuf//:protobuf_python",
    ],
)

proto_library(
    name = "codes_proto",
    srcs = [
        "codes.proto",
    ],
    deps = [
        ":annotations_proto",
        ":datatypes_proto",
    ],
)

cc_proto_library(
    name = "codes_cc_proto",
    deps = [":codes_proto"],
)

java_proto_library(
    name = "codes_java_proto",
    deps = [":codes_proto"],
)

py_proto_library(
    name = "codes_py_pb2",
    srcs = ["codes.proto"],
    default_runtime = "@com_google_protobuf//:protobuf_python",
    protoc = "@com_google_protobuf//:protoc",
    deps = [
        ":annotations_py_pb2",
        ":datatypes_py_pb2",
    ],
)

proto_library(
    name = "datatypes_proto",
    srcs = [
        "datatypes.proto",
    ],
    deps = [
        ":annotations_proto",
    ],
)

cc_proto_library(
    name = "datatypes_cc_proto",
    deps = [":datatypes_proto"],
)

java_proto_library(
    name = "datatypes_java_proto",
    deps = [":datatypes_proto"],
)

py_proto_library(
    name = "datatypes_py_pb2",
    srcs = ["datatypes.proto"],
    default_runtime = "@com_google_protobuf//:protobuf_python",
    protoc = "@com_google_protobuf//:protoc",
    deps = [
        ":annotations_py_pb2",
    ],
)

proto_library(
    name = "metadatatypes_proto",
    srcs = [
        "metadatatypes.proto",
    ],
    deps = [
        ":annotations_proto",
        ":codes_proto",
        ":datatypes_proto",
    ],
)

cc_proto_library(
    name = "metadatatypes_cc_proto",
    deps = [":metadatatypes_proto"],
)

java_proto_library(
    name = "metadatatypes_java_proto",
    deps = [":metadatatypes_proto"],
)

py_proto_library(
    name = "metadatatypes_py_pb2",
    srcs = ["metadatatypes.proto"],
    default_runtime = "@com_google_protobuf//:protobuf_python",
    protoc = "@com_google_protobuf//:protoc",
    deps = [
        ":annotations_py_pb2",
        ":codes_py_pb2",
        ":datatypes_py_pb2",
    ],
)

proto_library(
    name = "extensions_proto",
    srcs = [
        "base64_separator_stride.proto",
        "extensions.proto",
        "primitive_has_no_value.proto",
    ],
    deps = [
        ":annotations_proto",
        ":datatypes_proto",
    ],
)

cc_proto_library(
    name = "extensions_cc_proto",
    deps = [":extensions_proto"],
)

java_proto_library(
    name = "extensions_java_proto",
    deps = [":extensions_proto"],
)

py_proto_library(
    name = "extensions_py_pb2",
    srcs = [
        "base64_separator_stride.proto",
        "extensions.proto",
        "primitive_has_no_value.proto",
    ],
    default_runtime = "@com_google_protobuf//:protobuf_python",
    protoc = "@com_google_protobuf//:protoc",
    deps = [
        ":annotations_py_pb2",
        ":datatypes_py_pb2",
    ],
)

proto_library(
    name = "resources_proto",
    srcs = [
        "resources.proto",
    ],
    deps = [
        ":annotations_proto",
        ":codes_proto",
        ":datatypes_proto",
        ":metadatatypes_proto",
    ],
)

cc_proto_library(
    name = "resources_cc_proto",
    deps = [":resources_proto"],
)

java_proto_library(
    name = "resources_java_proto",
    deps = [":resources_proto"],
)

py_proto_library(
    name = "resources_py_pb2",
    srcs = ["resources.proto"],
    default_runtime = "@com_google_protobuf//:protobuf_python",
    protoc = "@com_google_protobuf//:protoc",
    deps = [
        ":annotations_py_pb2",
        ":codes_py_pb2",
        ":datatypes_py_pb2",
        ":metadatatypes_py_pb2",
    ],
)

proto_library(
    name = "version_config_proto",
    srcs = [
        "version_config.proto",
    ],
)

cc_proto_library(
    name = "version_config_cc_proto",
    deps = [":version_config_proto"],
)

py_proto_library(
    name = "version_config_py_pb2",
    srcs = ["version_config.proto"],
    default_runtime = "@com_google_protobuf//:protobuf_python",
    protoc = "@com_google_protobuf//:protoc",
)

# We also make the .proto files readable for testing.
filegroup(
    name = "testdata",
    srcs = glob([
        "*.proto",
    ]),
)
