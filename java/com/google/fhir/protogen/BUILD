package(
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
    visibility = ["//javatests/com/google/fhir/protogen:__pkg__"],
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
        ],
    ),
    deps = [
        ":generator_utils",
        "//java/com/google/fhir/common:annotation_utils",
        "//java/com/google/fhir/common:codes",
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:extensions",
        "//java/com/google/fhir/common:fhir_types",
        "//java/com/google/fhir/common:fhir_version",
        "//java/com/google/fhir/common:json_format",
        "//java/com/google/fhir/common:resource_utils",
        "//java/com/google/fhir/common:resource_validator",
        "//java/com/google/fhir/wrappers:primitive_wrappers",
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
        "@maven//:com_google_guava_guava",
    ],
)

java_binary(
    name = "ProtoGenerator",
    srcs = ["ProtoGeneratorMain.java"],
    main_class = "com.google.fhir.protogen.ProtoGeneratorMain",
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
