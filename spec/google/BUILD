# Profiles and Extensions published by Google

load("//bazel:protogen.bzl", "gen_fhir_definitions_and_protos")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

gen_fhir_definitions_and_protos(
    name = "stu3",
    extensions = [
        "google_extensions.prototxt",
    ],
    package_info = "google_stu3_package_info.prototxt",
    separate_extensions = True,
)

gen_fhir_definitions_and_protos(
    name = "r4",
    extensions = [
        "google_extensions.prototxt",
    ],
    package_info = "google_r4_package_info.prototxt",
    separate_extensions = True,
)
