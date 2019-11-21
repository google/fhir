# Tools for working with FHIR protocol buffers in java.
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

java_library(
    name = "primitive_wrappers",
    srcs = glob(["src/main/java/com/google/fhir/wrappers/*"]),
    deps = [
        ":common",
        "//proto/stu3:datatypes_java_proto",
        "//proto:annotations_java_proto",
        "//proto/r4:google_extensions_java_proto",
        "//proto/r4/core:datatypes_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_code_gson_gson",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "protogen",
    srcs = glob(
        ["src/main/java/com/google/fhir/protogen/*"],
        exclude = ["src/main/java/com/google/fhir/protogen/*Main.java"],
    ),
    deps = [
        ":common",
        ":fhir_version",
        ":json_format",
        ":primitive_wrappers",
        ":resource_utils",
        ":structure_definition_transformer",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:extensions_java_proto",
        "//proto:annotations_java_proto",
        "//proto:profile_config_java_proto",
        "//proto:protogenerator_annotations_java_proto",
        "//proto/r4/core:codes_java_proto",
        "//proto/r4/core:datatypes_java_proto",
        "//proto/r4/core:extensions_java_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/r4/core/resources:code_system_java_proto",
        "//proto/r4/core/resources:structure_definition_java_proto",
        "//proto/r4/core/resources:value_set_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "examples",
    srcs = [
        "src/main/java/com/google/fhir/examples/ConvertNdJsonForBigQueryMain.java",
        "src/main/java/com/google/fhir/examples/JsonParserArgs.java",
        "src/main/java/com/google/fhir/examples/JsonToProtoMain.java",
        "src/main/java/com/google/fhir/examples/SplitBundleMain.java",
        "src/main/java/com/google/fhir/examples/SyntheaToTFRecordsMain.java",
    ],
    deps = [
        ":big_query_schema",
        ":json_format",
        ":resource_utils",
        "//proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@com_google_protobuf//:protobuf_java_util",
        "@maven//:com_beust_jcommander",
        "@maven//:com_google_apis_google_api_services_bigquery",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_http_client_google_http_client_gson",
        "@maven//:org_apache_beam_beam_sdks_java_core",
    ],
)

java_library(
    name = "beam-runners",
    runtime_deps = [
        "@maven//:com_fasterxml_jackson_core_jackson_annotations",
        "@maven//:com_fasterxml_jackson_core_jackson_core",
        "@maven//:com_fasterxml_jackson_core_jackson_databind",
        "@maven//:com_google_api_client_google_api_client",
        "@maven//:com_google_apis_google_api_services_clouddebugger",
        "@maven//:com_google_apis_google_api_services_dataflow",
        "@maven//:com_google_apis_google_api_services_storage",
        "@maven//:com_google_auth_google_auth_library_credentials",
        "@maven//:com_google_cloud_bigdataoss_util",
        "@maven//:joda_time_joda_time",
        "@maven//:org_apache_beam_beam_model_pipeline",
        "@maven//:org_apache_beam_beam_runners_direct_java",
        "@maven//:org_apache_beam_beam_runners_google_cloud_dataflow_java",
        "@maven//:org_apache_beam_beam_sdks_java_extensions_google_cloud_platform_core",
        "@maven//:org_apache_beam_beam_sdks_java_io_google_cloud_platform",
        "@maven//:org_apache_beam_beam_vendor_grpc_v1_13_1",
        "@maven//:org_slf4j_slf4j_api",
        "@maven//:org_slf4j_slf4j_simple",
        "@maven//:org_xerial_snappy_snappy_java",
    ],
)

java_binary(
    name = "ConvertNdJsonForBigQuery",
    main_class = "com.google.fhir.examples.ConvertNdJsonForBigQueryMain",
    runtime_deps = [":examples"],
)

java_binary(
    name = "JsonToProto",
    main_class = "com.google.fhir.examples.JsonToProtoMain",
    runtime_deps = [":examples"],
)

java_binary(
    name = "SplitBundle",
    main_class = "com.google.fhir.examples.SplitBundleMain",
    runtime_deps = [":examples"],
)

java_binary(
    name = "SyntheaToTFRecords",
    main_class = "com.google.fhir.examples.SyntheaToTFRecordsMain",
    runtime_deps = [
        ":beam-runners",
        ":examples",
    ],
)

java_library(
    name = "structure_definition_transformer",
    srcs = [
        "src/main/java/com/google/fhir/dstu2/StructureDefinitionTransformer.java",
    ],
    deps = [
        "@maven//:com_beust_jcommander",
        "@maven//:com_google_code_gson_gson",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_http_client_google_http_client",
        "@maven//:com_google_http_client_google_http_client_gson",
    ],
)

java_library(
    name = "common",
    srcs = [
        "src/main/java/com/google/fhir/common/AnnotationUtils.java",
        "src/main/java/com/google/fhir/common/InvalidFhirException.java",
        "src/main/java/com/google/fhir/common/ProtoUtils.java",
    ],
    deps = [
        "//proto:annotations_java_proto",
        "@com_google_protobuf//:protobuf_java",
    ],
)

java_library(
    name = "fhir_version",
    srcs = [
        "src/main/java/com/google/fhir/common/FhirVersion.java",
    ],
    deps = [
        "//proto/stu3:codes_java_proto",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:extensions_java_proto",
        "//proto/stu3:metadatatypes_java_proto",
        "//proto/stu3:resources_java_proto",
        "//proto:annotations_java_proto",
        "//proto/r4/core:codes_java_proto",
        "//proto/r4/core:datatypes_java_proto",
        "//proto/r4/core:extensions_java_proto",
        "//proto/r4/core:valuesets_java_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "resource_utils",
    srcs = [
        "src/main/java/com/google/fhir/common/ResourceUtils.java",
    ],
    deps = [
        ":common",
        ":primitive_wrappers",
        "//proto/stu3:resources_java_proto",
        "//proto/r4/core:datatypes_java_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "json_format",
    srcs = ["src/main/java/com/google/fhir/common/JsonFormat.java"],
    deps = [
        ":common",
        ":primitive_wrappers",
        ":resource_utils",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:extensions_java_proto",
        "//proto/stu3:resources_java_proto",
        "//proto:annotations_java_proto",
        "//proto/r4/core:codes_java_proto",
        "//proto/r4/core:datatypes_java_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/r4/core/resources:capability_statement_java_proto",
        "//proto/r4/core/resources:code_system_java_proto",
        "//proto/r4/core/resources:compartment_definition_java_proto",
        "//proto/r4/core/resources:concept_map_java_proto",
        "//proto/r4/core/resources:operation_definition_java_proto",
        "//proto/r4/core/resources:structure_definition_java_proto",
        "//proto/r4/core/resources:value_set_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_code_gson_gson",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "resource_validator",
    srcs = ["src/main/java/com/google/fhir/common/ResourceValidator.java"],
    deps = [
        ":common",
        ":primitive_wrappers",
        "//proto/stu3:datatypes_java_proto",
        "//proto:annotations_java_proto",
        "//proto/r4/core:datatypes_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
    ],
)

java_test(
    name = "ResourceValidatorTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/common/ResourceValidatorR4Test.java"],
    data = [
        "//testdata/r4:validation",
    ],
    test_class = "com.google.fhir.common.ResourceValidatorR4Test",
    deps = [
        ":common",
        ":resource_validator",
        "//proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/r4/core/resources:encounter_java_proto",
        "//proto/r4/core/resources:observation_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_library(
    name = "file_utils",
    srcs = ["src/main/java/com/google/fhir/common/FileUtils.java"],
    deps = [
        ":json_format",
        "//proto/r4/core/resources:structure_definition_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
    ],
)

java_library(
    name = "big_query_schema",
    srcs = ["src/main/java/com/google/fhir/common/BigQuerySchema.java"],
    deps = [
        ":common",
        "//proto:annotations_java_proto",
        "//proto/r4/core:datatypes_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_apis_google_api_services_bigquery",
        "@maven//:com_google_guava_guava",
    ],
)

java_test(
    name = "StructureDefinitionTransformerTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/dstu2/StructureDefinitionTransformerTest.java"],
    test_class = "com.google.fhir.dstu2.StructureDefinitionTransformerTest",
    deps = [
        ":structure_definition_transformer",
        "@maven//:com_google_code_gson_gson",
        "@maven//:com_google_http_client_google_http_client",
        "@maven//:com_google_http_client_google_http_client_gson",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_library(
    name = "big_query_schema_generator_lib",
    srcs = ["src/main/java/com/google/fhir/examples/BigQuerySchemaGeneratorMain.java"],
    deps = [
        ":big_query_schema",
        "//examples/profiles:demo_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_apis_google_api_services_bigquery",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_http_client_google_http_client_gson",
    ],
)

java_binary(
    name = "BigQuerySchemaGenerator",
    main_class = "com.google.fhir.examples.BigQuerySchemaGeneratorMain",
    runtime_deps = [
        ":big_query_schema_generator_lib",
    ],
)

java_test(
    name = "AnnotationUtilsTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/common/AnnotationUtilsTest.java"],
    test_class = "com.google.fhir.common.AnnotationUtilsTest",
    deps = [
        ":common",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:resources_java_proto",
        "//proto/stu3:uscore_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "BigQuerySchemaTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/common/BigQuerySchemaTest.java"],
    data = [
        "//spec:stu3",
        "//testdata/stu3:bigquery",
        "//testdata/stu3:examples",
    ],
    test_class = "com.google.fhir.common.BigQuerySchemaTest",
    deps = [
        ":big_query_schema",
        "//proto/stu3:resources_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_apis_google_api_services_bigquery",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_http_client_google_http_client_gson",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "DateTimeWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/wrappers/DateTimeWrapperTest.java"],
    test_class = "com.google.fhir.wrappers.DateTimeWrapperTest",
    deps = [
        ":primitive_wrappers",
        "//proto/r4/core:datatypes_java_proto",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "DateWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/wrappers/DateWrapperTest.java"],
    test_class = "com.google.fhir.wrappers.DateWrapperTest",
    deps = [
        ":primitive_wrappers",
        "//proto/r4/core:datatypes_java_proto",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "DecimalWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/wrappers/DecimalWrapperTest.java"],
    test_class = "com.google.fhir.wrappers.DecimalWrapperTest",
    deps = [
        ":primitive_wrappers",
        "//proto/r4/core:datatypes_java_proto",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "ExtensionWrapperTest",
    size = "medium",
    srcs = ["src/test/java/com/google/fhir/wrappers/ExtensionWrapperTest.java"],
    data = [
        "//testdata/stu3:google",
    ],
    test_class = "com.google.fhir.wrappers.ExtensionWrapperTest",
    deps = [
        ":primitive_wrappers",
        "//proto/stu3:extensions_java_proto",
        "//proto/r4/core:datatypes_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "InstantWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/wrappers/InstantWrapperTest.java"],
    test_class = "com.google.fhir.wrappers.InstantWrapperTest",
    deps = [
        ":primitive_wrappers",
        "//proto/r4/core:datatypes_java_proto",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_library(
    name = "json_format_test_base",
    testonly = 1,
    srcs = ["src/main/java/com/google/fhir/common/JsonFormatTestBase.java"],
    deps = [
        ":json_format",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_code_gson_gson",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "JsonFormatStu3Test",
    size = "large",
    srcs = ["src/test/java/com/google/fhir/stu3/JsonFormatTest.java"],
    data = [
        "//spec:stu3",
        "//testdata/stu3:bigquery",
        "//testdata/stu3:examples",
        "//testdata/stu3:profiles",
    ],
    shard_count = 10,
    test_class = "com.google.fhir.stu3.JsonFormatTest",
    deps = [
        ":json_format_test_base",
        "//proto/stu3:resources_java_proto",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "JsonFormatR4Test",
    size = "large",
    srcs = ["src/test/java/com/google/fhir/r4/JsonFormatTest.java"],
    data = [
        "//spec:r4",
        "//testdata/r4:bigquery",
        "//testdata/r4:examples",
    ],
    shard_count = 10,
    test_class = "com.google.fhir.r4.JsonFormatTest",
    deps = [
        ":json_format_test_base",
        "//proto/r4/core/resources:account_java_proto",
        "//proto/r4/core/resources:activity_definition_java_proto",
        "//proto/r4/core/resources:adverse_event_java_proto",
        "//proto/r4/core/resources:allergy_intolerance_java_proto",
        "//proto/r4/core/resources:appointment_java_proto",
        "//proto/r4/core/resources:appointment_response_java_proto",
        "//proto/r4/core/resources:audit_event_java_proto",
        "//proto/r4/core/resources:basic_java_proto",
        "//proto/r4/core/resources:binary_java_proto",
        "//proto/r4/core/resources:biologically_derived_product_java_proto",
        "//proto/r4/core/resources:body_structure_java_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/r4/core/resources:capability_statement_java_proto",
        "//proto/r4/core/resources:care_plan_java_proto",
        "//proto/r4/core/resources:care_team_java_proto",
        "//proto/r4/core/resources:catalog_entry_java_proto",
        "//proto/r4/core/resources:charge_item_definition_java_proto",
        "//proto/r4/core/resources:charge_item_java_proto",
        "//proto/r4/core/resources:claim_java_proto",
        "//proto/r4/core/resources:claim_response_java_proto",
        "//proto/r4/core/resources:clinical_impression_java_proto",
        "//proto/r4/core/resources:communication_java_proto",
        "//proto/r4/core/resources:communication_request_java_proto",
        "//proto/r4/core/resources:compartment_definition_java_proto",
        "//proto/r4/core/resources:composition_java_proto",
        "//proto/r4/core/resources:condition_java_proto",
        "//proto/r4/core/resources:consent_java_proto",
        "//proto/r4/core/resources:contract_java_proto",
        "//proto/r4/core/resources:coverage_eligibility_request_java_proto",
        "//proto/r4/core/resources:coverage_eligibility_response_java_proto",
        "//proto/r4/core/resources:coverage_java_proto",
        "//proto/r4/core/resources:detected_issue_java_proto",
        "//proto/r4/core/resources:device_definition_java_proto",
        "//proto/r4/core/resources:device_java_proto",
        "//proto/r4/core/resources:device_metric_java_proto",
        "//proto/r4/core/resources:device_request_java_proto",
        "//proto/r4/core/resources:device_use_statement_java_proto",
        "//proto/r4/core/resources:diagnostic_report_java_proto",
        "//proto/r4/core/resources:document_manifest_java_proto",
        "//proto/r4/core/resources:document_reference_java_proto",
        "//proto/r4/core/resources:effect_evidence_synthesis_java_proto",
        "//proto/r4/core/resources:encounter_java_proto",
        "//proto/r4/core/resources:endpoint_java_proto",
        "//proto/r4/core/resources:enrollment_request_java_proto",
        "//proto/r4/core/resources:enrollment_response_java_proto",
        "//proto/r4/core/resources:episode_of_care_java_proto",
        "//proto/r4/core/resources:event_definition_java_proto",
        "//proto/r4/core/resources:evidence_java_proto",
        "//proto/r4/core/resources:evidence_variable_java_proto",
        "//proto/r4/core/resources:example_scenario_java_proto",
        "//proto/r4/core/resources:explanation_of_benefit_java_proto",
        "//proto/r4/core/resources:family_member_history_java_proto",
        "//proto/r4/core/resources:flag_java_proto",
        "//proto/r4/core/resources:goal_java_proto",
        "//proto/r4/core/resources:graph_definition_java_proto",
        "//proto/r4/core/resources:group_java_proto",
        "//proto/r4/core/resources:guidance_response_java_proto",
        "//proto/r4/core/resources:healthcare_service_java_proto",
        "//proto/r4/core/resources:imaging_study_java_proto",
        "//proto/r4/core/resources:immunization_evaluation_java_proto",
        "//proto/r4/core/resources:immunization_java_proto",
        "//proto/r4/core/resources:immunization_recommendation_java_proto",
        "//proto/r4/core/resources:implementation_guide_java_proto",
        "//proto/r4/core/resources:insurance_plan_java_proto",
        "//proto/r4/core/resources:invoice_java_proto",
        "//proto/r4/core/resources:library_java_proto",
        "//proto/r4/core/resources:linkage_java_proto",
        "//proto/r4/core/resources:list_java_proto",
        "//proto/r4/core/resources:location_java_proto",
        "//proto/r4/core/resources:measure_java_proto",
        "//proto/r4/core/resources:measure_report_java_proto",
        "//proto/r4/core/resources:media_java_proto",
        "//proto/r4/core/resources:medication_administration_java_proto",
        "//proto/r4/core/resources:medication_dispense_java_proto",
        "//proto/r4/core/resources:medication_java_proto",
        "//proto/r4/core/resources:medication_knowledge_java_proto",
        "//proto/r4/core/resources:medication_request_java_proto",
        "//proto/r4/core/resources:medication_statement_java_proto",
        "//proto/r4/core/resources:medicinal_product_authorization_java_proto",
        "//proto/r4/core/resources:medicinal_product_contraindication_java_proto",
        "//proto/r4/core/resources:medicinal_product_indication_java_proto",
        "//proto/r4/core/resources:medicinal_product_ingredient_java_proto",
        "//proto/r4/core/resources:medicinal_product_interaction_java_proto",
        "//proto/r4/core/resources:medicinal_product_java_proto",
        "//proto/r4/core/resources:medicinal_product_manufactured_java_proto",
        "//proto/r4/core/resources:medicinal_product_packaged_java_proto",
        "//proto/r4/core/resources:medicinal_product_pharmaceutical_java_proto",
        "//proto/r4/core/resources:medicinal_product_undesirable_effect_java_proto",
        "//proto/r4/core/resources:message_definition_java_proto",
        "//proto/r4/core/resources:message_header_java_proto",
        "//proto/r4/core/resources:molecular_sequence_java_proto",
        "//proto/r4/core/resources:naming_system_java_proto",
        "//proto/r4/core/resources:nutrition_order_java_proto",
        "//proto/r4/core/resources:observation_definition_java_proto",
        "//proto/r4/core/resources:observation_java_proto",
        "//proto/r4/core/resources:operation_definition_java_proto",
        "//proto/r4/core/resources:operation_outcome_java_proto",
        "//proto/r4/core/resources:organization_affiliation_java_proto",
        "//proto/r4/core/resources:organization_java_proto",
        "//proto/r4/core/resources:patient_java_proto",
        "//proto/r4/core/resources:payment_notice_java_proto",
        "//proto/r4/core/resources:payment_reconciliation_java_proto",
        "//proto/r4/core/resources:person_java_proto",
        "//proto/r4/core/resources:plan_definition_java_proto",
        "//proto/r4/core/resources:practitioner_java_proto",
        "//proto/r4/core/resources:practitioner_role_java_proto",
        "//proto/r4/core/resources:procedure_java_proto",
        "//proto/r4/core/resources:provenance_java_proto",
        "//proto/r4/core/resources:questionnaire_java_proto",
        "//proto/r4/core/resources:questionnaire_response_java_proto",
        "//proto/r4/core/resources:related_person_java_proto",
        "//proto/r4/core/resources:request_group_java_proto",
        "//proto/r4/core/resources:research_definition_java_proto",
        "//proto/r4/core/resources:research_element_definition_java_proto",
        "//proto/r4/core/resources:research_study_java_proto",
        "//proto/r4/core/resources:research_subject_java_proto",
        "//proto/r4/core/resources:risk_assessment_java_proto",
        "//proto/r4/core/resources:risk_evidence_synthesis_java_proto",
        "//proto/r4/core/resources:schedule_java_proto",
        "//proto/r4/core/resources:service_request_java_proto",
        "//proto/r4/core/resources:slot_java_proto",
        "//proto/r4/core/resources:specimen_definition_java_proto",
        "//proto/r4/core/resources:specimen_java_proto",
        "//proto/r4/core/resources:structure_map_java_proto",
        "//proto/r4/core/resources:subscription_java_proto",
        "//proto/r4/core/resources:substance_java_proto",
        "//proto/r4/core/resources:substance_specification_java_proto",
        "//proto/r4/core/resources:supply_delivery_java_proto",
        "//proto/r4/core/resources:supply_request_java_proto",
        "//proto/r4/core/resources:task_java_proto",
        "//proto/r4/core/resources:terminology_capabilities_java_proto",
        "//proto/r4/core/resources:test_report_java_proto",
        "//proto/r4/core/resources:test_script_java_proto",
        "//proto/r4/core/resources:verification_result_java_proto",
        "//proto/r4/core/resources:vision_prescription_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "ProtoFilePrinterTest",
    size = "medium",
    srcs = ["src/test/java/com/google/fhir/protogen/ProtoFilePrinterTest.java"],
    data = [
        "//proto/r4/core:testdata",
        "//proto/r4/core/resources:testdata",
        "//spec:fhir_r4_package.zip",
        "//spec:fhir_r4_package_filegroup",
        "//testdata/r4:descriptors",
    ],
    test_class = "com.google.fhir.protogen.ProtoFilePrinterTest",
    deps = [
        ":json_format",
        ":protogen",
        "//proto:annotations_java_proto",
        "//proto:profile_config_java_proto",
        "//proto/r4/core:datatypes_java_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/r4/core/resources:structure_definition_java_proto",
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
    srcs = ["src/test/java/com/google/fhir/protogen/ProtoGeneratorTest.java"],
    data = [
        "//spec:fhir_r4_package.zip",
        "//spec:fhir_r4_package_filegroup",
        "//spec:fhir_stu3_package_filegroup",
        "//spec:uscore_stu3_package_filegroup",
        "//testdata/r4:descriptors",
        "//testdata/stu3:descriptors",
        "//testdata/stu3:google",
    ],
    test_class = "com.google.fhir.protogen.ProtoGeneratorTest",
    deps = [
        ":json_format",
        ":protogen",
        "//proto:annotations_java_proto",
        "//proto:protogenerator_annotations_java_proto",
        "//proto/r4/core/resources:structure_definition_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "TimeWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/wrappers/TimeWrapperTest.java"],
    test_class = "com.google.fhir.wrappers.TimeWrapperTest",
    deps = [
        ":primitive_wrappers",
        "//proto/r4/core:datatypes_java_proto",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "ValidationTest",
    size = "medium",
    srcs = ["src/test/java/com/google/fhir/wrappers/ValidationTest.java"],
    data = [
        "//testdata/stu3:validation",
    ],
    test_class = "com.google.fhir.wrappers.ValidationTest",
    deps = [
        ":json_format",
        ":primitive_wrappers",
        "//proto/stu3:codes_java_proto",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:extensions_java_proto",
        "@bazel_tools//tools/java/runfiles",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_binary(
    name = "ProtoGenerator",
    srcs = ["src/main/java/com/google/fhir/protogen/ProtoGeneratorMain.java"],
    main_class = "com.google.fhir.protogen.ProtoGeneratorMain",
    deps = [
        ":common",
        ":file_utils",
        ":json_format",
        ":protogen",
        ":structure_definition_transformer",
        "//proto:annotations_java_proto",
        "//proto:profile_config_java_proto",
        "//proto/r4/core:codes_java_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/r4/core/resources:structure_definition_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_beust_jcommander",
        "@maven//:com_google_guava_guava",
    ],
)

java_test(
    name = "ProfileGeneratorTest",
    size = "medium",
    testonly = 1,
    srcs = ["src/test/java/com/google/fhir/protogen/ProfileGeneratorTest.java"],
    data = [
        "//spec:fhir_stu3_package_filegroup",
        "//testdata/stu3:profilegenerator",
    ],
    test_class = "com.google.fhir.protogen.ProfileGeneratorTest",
    deps = [
        ":json_format",
        ":protogen",
        "//proto:profile_config_java_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/r4/core/resources:structure_definition_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_binary(
    name = "ProfileGenerator",
    srcs = ["src/main/java/com/google/fhir/protogen/ProfileGeneratorMain.java"],
    main_class = "com.google.fhir.protogen.ProfileGeneratorMain",
    deps = [
        ":file_utils",
        ":json_format",
        ":protogen",
        "//proto:annotations_java_proto",
        "//proto:profile_config_java_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/r4/core/resources:structure_definition_java_proto",
        "@maven//:com_beust_jcommander",
        "@maven//:com_google_guava_guava",
    ],
)

java_binary(
    name = "ValueSetGenerator",
    srcs = ["src/main/java/com/google/fhir/protogen/ValueSetGeneratorMain.java"],
    main_class = "com.google.fhir.protogen.ValueSetGeneratorMain",
    deps = [
        ":json_format",
        ":protogen",
        "//proto:annotations_java_proto",
        "//proto:profile_config_java_proto",
        "//proto/r4/core/resources:bundle_and_contained_resource_java_proto",
        "//proto/r4/core/resources:structure_definition_java_proto",
        "@com_google_protobuf//:protobuf_java",
        "@maven//:com_beust_jcommander",
        "@maven//:com_google_guava_guava",
    ],
)
