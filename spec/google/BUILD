# Profiles and Extensions published by Google

load("//bazel:protogen.bzl", "gen_fhir_definitions_and_protos")

licenses(["notice"])

package(
    default_applicable_licenses = [""],
    default_visibility = ["//visibility:public"],
)

gen_fhir_definitions_and_protos(
    name = "stu3",
    # Automatic regression testing disabled because these files are manually
    # moved from this directory to //proto/r4
    disable_test = True,
    extensions = [
        "fhirproto_extensions.prototxt",
    ],
    package_info = "google_stu3_package_info.prototxt",
    package_json = "google_stu3_package.json",
)

gen_fhir_definitions_and_protos(
    name = "r4",
    # Automatic regression testing disabled because these files are manually
    # moved from this directory to //proto/r4
    disable_test = True,
    extensions = [
        "fhirproto_extensions.prototxt",
    ],
    package_info = "google_r4_package_info.prototxt",
    package_json = "google_r4_package.json",
    profiles = [
        "fhirproto.prototxt",
    ],
)
