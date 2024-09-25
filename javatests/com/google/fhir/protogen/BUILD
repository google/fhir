load("@rules_java//java:defs.bzl", "java_test")

licenses(["notice"])

java_test(
    name = "ProtoFilePrinterTest",
    size = "medium",
    srcs = ["ProtoFilePrinterTest.java"],
    data = [
        "//proto/google/fhir/proto/r4/core:testdata",
        "//proto/google/fhir/proto/r4/core/resources:testdata",
        "//testdata/r4:descriptors",
        "@hl7.fhir.r4.core_4.0.1//file",
    ],
    test_class = "com.google.fhir.protogen.ProtoFilePrinterTest",
    deps = [
        "//java/com/google/fhir/common:annotation_utils",
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/protogen:fhir_package",
        "//java/com/google/fhir/protogen:generator_utils",
        "//java/com/google/fhir/protogen:proto_file_printer",
        "//java/com/google/fhir/protogen:proto_generator_v2",
        "//java/com/google/fhir/protogen:value_set_generator_v2",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto:protogenerator_annotations_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "ProtoGeneratorTest",
    size = "large",
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
        "//java/com/google/fhir/protogen:fhir_package",
        "//java/com/google/fhir/protogen:generator_utils",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto:protogenerator_annotations_java_proto",
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "ProtoGeneratorV2Test",
    size = "large",
    srcs = ["ProtoGeneratorV2Test.java"],
    data = [
        "@hl7.fhir.r4.core_4.0.1//file",
        "@hl7.fhir.r5.core_5.0.0//file",
    ],
    test_class = "com.google.fhir.protogen.ProtoGeneratorV2Test",
    deps = [
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/protogen:fhir_package",
        "//java/com/google/fhir/protogen:proto_generator_main_v2",
        "//java/com/google/fhir/protogen:proto_generator_test_utils",
        "//java/com/google/fhir/protogen:proto_generator_v2",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "//proto/google/fhir/proto/r5/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r5/core/resources:bundle_and_contained_resource_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_testparameterinjector_test_parameter_injector",
        "@maven//:com_google_truth_extensions_truth_proto_extension",
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
        "//java/com/google/fhir/protogen:fhir_package",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:code_system_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:search_parameter_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:value_set_java_proto",
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
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:protogenerator_annotations_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_extensions_truth_proto_extension",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "ResourceCollectionTest",
    size = "small",
    srcs = ["ResourceCollectionTest.java"],
    test_class = "com.google.fhir.protogen.ResourceCollectionTest",
    deps = [
        "//java/com/google/fhir/common:json_format",
        "//java/com/google/fhir/protogen:ResourceCollection",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "@maven//:com_google_code_gson_gson",
        "@maven//:com_google_testparameterinjector_test_parameter_injector",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "ValueSetGeneratorV2Test",
    size = "large",
    srcs = ["ValueSetGeneratorV2Test.java"],
    data = [
        "@hl7.fhir.r4.core_4.0.1//file",
        "@hl7.fhir.r5.core_5.0.0//file",
    ],
    deps = [
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/protogen:fhir_package",
        "//java/com/google/fhir/protogen:proto_generator_test_utils",
        "//java/com/google/fhir/protogen:value_set_generator_v2",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_java_proto",
        "//proto/google/fhir/proto/r5/core:codes_java_proto",
        "//proto/google/fhir/proto/r5/core:valuesets_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_testparameterinjector_test_parameter_injector",
        "@maven//:com_google_truth_extensions_truth_proto_extension",
        "@maven//:junit_junit",
    ],
)

exports_files(["GeneratedProtoTest.java"])
