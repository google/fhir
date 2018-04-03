package(default_visibility = ["//visibility:public"])

licenses(["notice"])

proto_library(
    name = "datatypes",
    srcs = [
        "annotations.proto",
        "codes.proto",
        "datatypes.proto",
    ],
    deps = [
        "@com_google_protobuf//:descriptor_proto",
    ],
)

java_proto_library(
    name = "datatypes_java_proto",
    strict_deps = 0,
    deps = [":datatypes"],
)

proto_library(
    name = "resources_proto",
    srcs = [
        "resources.proto",
    ],
    deps = [
        ":datatypes",
    ],
)

java_proto_library(
    name = "resources_java_proto",
    strict_deps = 0,
    deps = [":resources_proto"],
)

proto_library(
    name = "extensions",
    srcs = [
        "base64_separator_stride.proto",
        "elementdefinition_binding_name.proto",
        "primitive_has_no_value.proto",
        "structuredefinition_explicit_type_name.proto",
    ],
    deps = [
        ":datatypes",
    ],
)

java_proto_library(
    name = "extensions_java_proto",
    strict_deps = 0,
    deps = [":extensions"],
)

# We also make the .proto files readable for testing.
filegroup(
    name = "testdata",
    srcs = glob([
        "*.proto",
    ]),
)
