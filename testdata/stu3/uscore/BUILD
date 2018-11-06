# Structure Definitions for the Us-Core profiles.

package(default_visibility = ["//visibility:public"])

load("//bazel:protogen.bzl", "zip_file")
load("//bazel:protogen.bzl", "structure_definition_package")

licenses(["notice"])

exports_files(["uscore_package_info.prototxt"])

# These files contain us core codes.
filegroup(
    name = "uscore_structure_definitions",
    srcs = glob(["*.json"]),
)

zip_file(
    name = "uscore_structure_definitions.zip",
    srcs = glob([
        "*.json",
    ]),
)

structure_definition_package(
    package_name = "uscore",
    package_info = "uscore_package_info.prototxt",
    structure_definitions_zip = "uscore_structure_definitions.zip",
)
