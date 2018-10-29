# Structure Definitions for the Us-Core profiles.

package(default_visibility = ["//visibility:public"])

load("//bazel:protogen.bzl", "zip_file")

licenses(["notice"])

exports_files(["uscore_package_info.prototxt"])

# These files contain us core codes.
filegroup(
    name = "structure_definitions",
    srcs = glob(["*.json"]),
)

zip_file(
    name = "structure_definitions.zip",
    srcs = glob([
        "*.json",
    ]),
)
