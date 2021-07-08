load("//bazel:protogen.bzl", "gen_fhir_definitions_and_protos")

licenses(["notice"])

# This build rule uses the baseline "test" protos as the golden_java_proto_rules
# but generates its own protos and shifts around some fields.
# Thus, the proto generation regression test automatically included by
# gen_fhir_definitions_and_protos act as an integration test for the field
# retagging.
gen_fhir_definitions_and_protos(
    name = "retagging_test",
    extensions = [
        "//testdata/r4/profiles:test_extensions.prototxt",
    ],
    golden_java_proto_rules = ["//testdata/r4/profiles:test_java_proto"],
    package_deps = [
        "//spec:uscore_r4_package",
    ],
    package_info = "//testdata/r4/profiles:test_package_info.prototxt",
    profiles = [
        "retagging_test_profiles.prototxt",
    ],
    terminologies = [
        "//testdata/r4/profiles:test_terminologies.prototxt",
    ],
)

# Export config files for use in "split" version of test profile.
exports_files(
    [
        "retagging_test_profiles.prototxt",
    ],
    visibility = ["//testdata/r4/profiles:__subpackages__"],
)
