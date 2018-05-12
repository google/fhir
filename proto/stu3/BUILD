package(default_visibility = ["//visibility:public"])

licenses(["notice"])

proto_library(
    name = "extensions",
    srcs = [
        "base64_separator_stride.proto",
        "elementdefinition_binding_name.proto",
        "primitive_has_no_value.proto",
        "structuredefinition_explicit_type_name.proto",
    ],
    deps = [
        ":annotations",
        ":datatypes",
    ],
)

java_proto_library(
    name = "extensions_java_proto",
    deps = [":extensions"],
)

proto_library(
    name = "datatypes",
    srcs = [
        "codes.proto",
        "datatypes.proto",
    ],
    deps = [
        ":annotations",
    ],
)

java_proto_library(
    name = "datatypes_java_proto",
    deps = [":datatypes"],
)

proto_library(
    name = "annotations",
    srcs = [
        "annotations.proto",
    ],
    deps = [
        "@com_google_protobuf//:descriptor_proto",
    ],
)

java_proto_library(
    name = "annotations_java_proto",
    deps = [":annotations"],
)

proto_library(
    name = "resources_proto",
    srcs = [
        "resources.proto",
    ],
    deps = [
        ":annotations",
        ":datatypes",
    ],
)

java_proto_library(
    name = "resources_java_proto",
    deps = [":resources_proto"],
)

# We also make the .proto files readable for testing.
filegroup(
    name = "testdata",
    srcs = glob([
        "*.proto",
    ]),
)
