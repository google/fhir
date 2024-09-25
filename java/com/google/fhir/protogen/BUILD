load("@rules_java//java:defs.bzl", "java_binary", "java_library")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

java_library(
    name = "generator_utils",
    srcs = ["GeneratorUtils.java"],
    deps = [
        "//java/com/google/fhir/common:exceptions",
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:search_parameter_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "fhir_package",
    srcs = ["FhirPackage.java"],
    deps = [
        ":ResourceCollection",
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:fhir_version",
        "//java/com/google/fhir/common:json_format",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:code_system_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:search_parameter_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:value_set_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_code_gson_gson",
        "@maven//:com_google_guava_guava",
        "@maven//:org_apache_commons_commons_compress",
    ],
)

java_library(
    name = "proto_file_printer",
    srcs = ["ProtoFilePrinter.java"],
    deps = [
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto:protogenerator_annotations_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "protogen",
    srcs = [
        "ProfileGenerator.java",
        "ProtoGenerator.java",
        "TerminologyGenerator.java",
        "ValueSetGenerator.java",
    ],
    deps = [
        ":fhir_package",
        ":field_retagger",
        ":generator_utils",
        "//java/com/google/fhir/common:annotation_utils",
        "//java/com/google/fhir/common:codes",
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:extensions",
        "//java/com/google/fhir/common:fhir_types",
        "//java/com/google/fhir/common:fhir_version",
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
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "proto_generator_binary_lib",
    srcs = ["ProtoGeneratorMain.java"],
    deps = [
        ":fhir_package",
        ":generator_utils",
        ":proto_file_printer",
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
        ":fhir_package",
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
    name = "proto_generator_main_v2",
    srcs = ["ProtoGeneratorMainV2.java"],
    deps = [
        ":fhir_package",
        ":generator_utils",
        ":proto_file_printer",
        ":proto_generator_v2",
        ":value_set_generator_v2",
        "//java/com/google/fhir/common:exceptions",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_beust_jcommander",
    ],
)

java_binary(
    name = "ProtoGeneratorV2",
    main_class = "com.google.fhir.protogen.ProtoGeneratorMainV2",
    runtime_deps = [
        ":proto_generator_main_v2",
        # For field retagging against R4
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_java_proto",
    ],
)

java_library(
    name = "field_retagger",
    srcs = ["FieldRetagger.java"],
    deps = [
        "//java/com/google/fhir/common:codes",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto:protogenerator_annotations_java_proto",
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
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

java_library(
    name = "value_set_generator_v2",
    srcs = ["ValueSetGeneratorV2.java"],
    deps = [
        ":fhir_package",
        ":field_retagger",
        ":generator_utils",
        "//java/com/google/fhir/common:annotation_utils",
        "//java/com/google/fhir/common:codes",
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:terminology_expander",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto:protogenerator_annotations_java_proto",
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:code_system_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:value_set_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "proto_generator_v2",
    srcs = ["ProtoGeneratorV2.java"],
    deps = [
        ":fhir_package",
        ":field_retagger",
        ":generator_utils",
        ":value_set_generator_v2",
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/common:extensions",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto:profile_config_java_proto",
        "//proto/google/fhir/proto:protogenerator_annotations_java_proto",
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_errorprone_error_prone_annotations",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "proto_generator_test_utils",
    srcs = ["ProtoGeneratorTestUtils.java"],
    deps = [
        "//proto/google/fhir/proto:protogenerator_annotations_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_errorprone_error_prone_annotations",
    ],
)
