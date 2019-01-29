# Profiles and Extensions published by Google

load("//bazel:protogen.bzl", "gen_fhir_definitions_and_protos")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

gen_fhir_definitions_and_protos(
    name = "google",
    extensions = [
        "google_extensions.prototxt",
    ],
    package_info = "google_package_info.prototxt",
    separate_extensions = True,
)
