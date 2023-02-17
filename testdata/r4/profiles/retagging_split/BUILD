load("//bazel:protogen.bzl", "gen_fhir_definitions_and_protos")

licenses(["notice"])

package(
    default_applicable_licenses = [""],
    default_visibility = ["//visibility:public"],
)

# This build rule uses the baseline "split_test" protos as the golden_java_proto_rules
# but generates its own protos and shifts around some fields.
# Thus, the proto generation regression test automatically included by
# gen_fhir_definitions_and_protos act as an integration test for the field
# retagging in the "split protos" case.
# Note that this is in its own directory so that there is not a collision between the
# generated resources between the retagging split case and the normal split case.
gen_fhir_definitions_and_protos(
    name = "retagging_split_test",
    extensions = [
        "//testdata/r4/profiles:test_extensions.prototxt",
    ],
    golden_java_proto_rules = ["//testdata/r4/profiles/split:all_java_proto"],
    package_deps = [
        "//spec:uscore_r4_package",
    ],
    package_info = "//testdata/r4/profiles/split:package_info.prototxt",
    package_json = "//testdata/r4/profiles/split:package.json",
    profiles = [
        "//testdata/r4/profiles/retagging:retagging_test_profiles.prototxt",
    ],
    terminologies = [
        "//testdata/r4/profiles:test_terminologies.prototxt",
    ],
)
