package(default_visibility = ["//visibility:public"])

licenses(["notice"])

proto_library(
    name = "annotations_proto",
    srcs = [
        "annotations.proto",
    ],
    deps = [
        "@com_google_protobuf//:descriptor_proto",
    ],
)

java_proto_library(
    name = "annotations_java_proto",
    deps = [":annotations_proto"],
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

java_proto_library(
    name = "codes_java_proto",
    deps = [":codes_proto"],
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

java_proto_library(
    name = "datatypes_java_proto",
    deps = [":datatypes_proto"],
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

java_proto_library(
    name = "metadatatypes_java_proto",
    deps = [":metadatatypes_proto"],
)

proto_library(
    name = "extensions_proto",
    srcs = [
        "base64_separator_stride.proto",
        "elementdefinition_binding_name.proto",
        "primitive_has_no_value.proto",
        "structuredefinition_explicit_type_name.proto",
    ],
    deps = [
        ":annotations_proto",
        ":datatypes_proto",
    ],
)

java_proto_library(
    name = "extensions_java_proto",
    deps = [":extensions_proto"],
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
