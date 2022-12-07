package(
    default_applicable_licenses = ["//:license"],
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

java_library(
    name = "proto_generator_test_utils",
    testonly = 1,
    srcs = ["ProtoGeneratorTestUtils.java"],
    deps = [
        ":protogen",
        "//java/com/google/fhir/common:exceptions",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto:protogenerator_annotations_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
    ],
)

java_binary(
    name = "ValueSetGenerator",
    srcs = ["ValueSetGeneratorMain.java"],
    main_class = "com.google.fhir.protogen.ValueSetGeneratorMain",
    deps = [
        ":protogen",
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:json_format",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_beust_jcommander",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "generator_utils",
    srcs = ["GeneratorUtils.java"],
    deps = [
        "//java/com/google/fhir/common:exceptions",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:search_parameter_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "protogen",
    srcs = glob(
        ["*.java"],
        exclude = [
            "GeneratorUtils.java",
            "*Main.java",
            "ProtoGeneratorTestUtils.java",
            "ResourceCollection.java",
        ],
    ),
    deps = [
        ":ResourceCollection",
        ":generator_utils",
        "//java/com/google/fhir/common:annotation_utils",
        "//java/com/google/fhir/common:codes",
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:extensions",
        "//java/com/google/fhir/common:fhir_types",
        "//java/com/google/fhir/common:fhir_version",
        "//java/com/google/fhir/common:json_format",
        "//java/com/google/fhir/common:resource_validator",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto:protogenerator_annotations_java_proto",
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:code_system_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:search_parameter_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:value_set_java_proto",
        "//proto/google/fhir/proto/stu3:datatypes_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_code_gson_gson",
        "@maven//:com_google_guava_guava",
        "@maven//:org_apache_commons_commons_compress",
    ],
)

java_library(
    name = "proto_generator_binary_lib",
    srcs = ["ProtoGeneratorMain.java"],
    deps = [
        ":generator_utils",
        ":protogen",
        "//java/com/google/fhir/common:annotation_utils",
        "//java/com/google/fhir/common:exceptions",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_beust_jcommander",
        "@maven//:com_google_guava_guava",
    ],
)

java_binary(
    name = "ProtoGenerator",
    main_class = "com.google.fhir.protogen.ProtoGeneratorMain",
    runtime_deps = ["//java/com/google/fhir/protogen:proto_generator_binary_lib"],
)

java_binary(
    name = "ProfileGenerator",
    srcs = ["ProfileGeneratorMain.java"],
    main_class = "com.google.fhir.protogen.ProfileGeneratorMain",
    deps = [
        ":protogen",
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:json_format",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_beust_jcommander",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "field_retagger",
    srcs = ["FieldRetagger.java"],
    deps = [
        "//proto/google/fhir/proto:protogenerator_annotations_java_proto",
        "@com_google_protobuf//:protobuf_java",
    ],
)

java_library(
    name = "ResourceCollection",
    srcs = ["ResourceCollection.java"],
    deps = [
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:json_format",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_code_gson_gson",
    ],
)
