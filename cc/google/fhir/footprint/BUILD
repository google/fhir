package(default_visibility = ["//visibility:public"])

licenses(["notice"])

filegroup(
    name = "footprints",
    srcs = glob(["*.fp"]),
)

cc_library(
    name = "proto_footprint",
    srcs = ["proto_footprint.cc"],
    hdrs = ["proto_footprint.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "//cc/google/fhir:annotations",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "@com_google_absl//absl/container:btree",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_binary(
    name = "GenerateFootprint",
    srcs = ["generate_footprint.cc"],
    deps = [
        ":proto_footprint",
        "//proto/google/fhir/proto/r4:uscore_cc_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "//proto/google/fhir/proto/stu3:uscore_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "ProtoFootprintTest",
    srcs = ["proto_footprint_test.cc"],
    data = [
        "//testdata/r4:footprint",
        "//testdata/stu3:footprint",
    ],
    deps = [
        ":proto_footprint",
        "//cc/google/fhir:test_helper",
        "//proto/google/fhir/proto/r4:uscore_cc_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_cc_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_cc_proto",
        "//proto/google/fhir/proto/stu3:datatypes_cc_proto",
        "//proto/google/fhir/proto/stu3:resources_cc_proto",
        "//proto/google/fhir/proto/stu3:uscore_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)
