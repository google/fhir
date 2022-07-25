licenses(["notice"])

java_test(
    name = "ProtoFilePrinterTest",
    size = "medium",
    srcs = ["ProtoFilePrinterTest.java"],
    data = [
        "//proto/google/fhir/proto/r4/core:testdata",
        "//proto/google/fhir/proto/r4/core/resources:testdata",
        "//spec:fhir_r4_filegroup",
        "//spec:fhir_r4_package.zip",
        "//testdata/r4:descriptors",
    ],
    test_class = "com.google.fhir.protogen.ProtoFilePrinterTest",
    deps = [
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:json_format",
        "//java/com/google/fhir/protogen",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "ProtoGeneratorTest",
    size = "medium",
    srcs = ["ProtoGeneratorTest.java"],
    data = [
        "//proto/google/fhir/proto/r4/core:testdata",
        "//proto/google/fhir/proto/r4/core/profiles:proto_files",
        "//proto/google/fhir/proto/r4/core/resources:testdata",
        "//spec:fhir_r4_filegroup",
        "//spec:fhir_r4_package.zip",
        "//testdata/r4:descriptors",
    ],
    test_class = "com.google.fhir.protogen.ProtoGeneratorTest",
    runtime_deps = [
        "//proto/google/fhir/proto/r4/core/profiles:test_rule_java_proto",
    ],
    deps = [
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:json_format",
        "//java/com/google/fhir/protogen",
        "//java/com/google/fhir/protogen:proto_generator_test_utils",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:protogenerator_annotations_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "ProfileGeneratorTest",
    size = "medium",
    testonly = 1,
    srcs = ["ProfileGeneratorTest.java"],
    data = [
        "//spec:fhir_r4_filegroup",
        "//spec:fhir_stu3_filegroup",
        "//testdata/r4:profiles",
        "//testdata/stu3:profiles",
    ],
    test_class = "com.google.fhir.protogen.ProfileGeneratorTest",
    deps = [
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:json_format",
        "//java/com/google/fhir/protogen",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "FhirPackageTest",
    srcs = ["FhirPackageTest.java"],
    test_class = "com.google.fhir.protogen.FhirPackageTest",
    deps = [
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/protogen",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_guava_guava_testlib",
        "@maven//:com_google_testparameterinjector_test_parameter_injector",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
        "@maven//:org_apache_commons_commons_compress",
    ],
)

java_test(
    name = "FieldRetaggerTest",
    size = "medium",
    srcs = ["FieldRetaggerTest.java"],
    test_class = "com.google.fhir.protogen.FieldRetaggerTest",
    deps = [
        "//java/com/google/fhir/protogen:field_retagger",
        "//proto/google/fhir/proto:protogenerator_annotations_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_extensions_truth_proto_extension",
        "@maven//:junit_junit",
    ],
)

exports_files(["GeneratedProtoTest.java"])
