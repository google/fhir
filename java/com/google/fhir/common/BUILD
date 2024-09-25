load("@rules_java//java:defs.bzl", "java_library")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

java_library(
    name = "exceptions",
    srcs = ["InvalidFhirException.java"],
)

java_library(
    name = "annotation_utils",
    srcs = ["AnnotationUtils.java"],
    deps = [
        "//proto/google/fhir/proto:annotations_java_proto",
        "@com_google_protobuf//:protobuf_java",
    ],
)

java_library(
    name = "proto_utils",
    srcs = ["ProtoUtils.java"],
    deps = [
        ":exceptions",
        "//java/com/google/fhir/common:annotation_utils",
        "@com_google_protobuf//:protobuf_java",
    ],
)

java_library(
    name = "fhir_version",
    srcs = [
        "FhirVersion.java",
    ],
    deps = [
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core:extensions_java_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/google/fhir/proto/stu3:codes_java_proto",
        "//proto/google/fhir/proto/stu3:datatypes_java_proto",
        "//proto/google/fhir/proto/stu3:extensions_java_proto",
        "//proto/google/fhir/proto/stu3:metadatatypes_java_proto",
        "//proto/google/fhir/proto/stu3:resources_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "big_query_schema",
    srcs = ["BigQuerySchema.java"],
    deps = [
        ":annotation_utils",
        "//java/com/google/fhir/common:proto_utils",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_apis_google_api_services_bigquery",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "json_format",
    srcs = ["JsonFormat.java"],
    deps = [
        ":annotation_utils",
        ":exceptions",
        ":fhir_types",
        ":proto_gen_transformer",
        ":proto_utils",
        ":resource_utils",
        "//java/com/google/fhir/wrappers:primitive_wrappers",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto/r4:fhirproto_extensions_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/google/fhir/proto/stu3:datatypes_java_proto",
        "//proto/google/fhir/proto/stu3:resources_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_code_gson_gson",
        "@maven//:com_google_errorprone_error_prone_annotations",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "resource_utils",
    srcs = [
        "ResourceUtils.java",
    ],
    deps = [
        ":annotation_utils",
        ":exceptions",
        ":proto_utils",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "resource_validator",
    srcs = ["ResourceValidator.java"],
    deps = [
        ":annotation_utils",
        ":proto_utils",
        "//java/com/google/fhir/common:exceptions",
        "//java/com/google/fhir/wrappers:primitive_wrappers",
        "//proto/google/fhir/proto:annotations_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/stu3:datatypes_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "proto_gen_transformer",
    srcs = ["ProtoGenTransformer.java"],
    deps = [
        "//proto/google/fhir/proto/r4/core:codes_java_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:capability_statement_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:code_system_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:compartment_definition_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:concept_map_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:operation_definition_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:structure_definition_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:value_set_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_code_gson_gson",
        "@maven//:com_google_errorprone_error_prone_annotations",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "fhir_types",
    srcs = ["FhirTypes.java"],
    deps = [
        ":annotation_utils",
        "//proto/google/fhir/proto:annotations_java_proto",
        "@com_google_protobuf//:protobuf_java",
    ],
)

java_library(
    name = "descriptor_memos_map",
    srcs = ["DescriptorMemosMap.java"],
    deps = [
        "@com_google_protobuf//:protobuf_java",
    ],
)

java_library(
    name = "extensions",
    srcs = ["Extensions.java"],
    deps = [
        ":exceptions",
        ":proto_utils",
        "@com_google_protobuf//:protobuf_java",
    ],
)

java_library(
    name = "codes",
    srcs = ["Codes.java"],
    deps = [
        ":descriptor_memos_map",
        ":exceptions",
        ":fhir_types",
        ":proto_utils",
        "//proto/google/fhir/proto:annotations_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "terminology_expander",
    srcs = ["TerminologyExpander.java"],
    deps = [
        ":exceptions",
        "//java/com/google/fhir/protogen:fhir_package",
        "//proto/google/fhir/proto/r4/core:datatypes_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:code_system_java_proto",
        "//proto/google/fhir/proto/r4/core/resources:value_set_java_proto",
        "@maven//:com_google_guava_guava",
    ],
)
