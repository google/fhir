load("//bazel:proto.bzl", "fhir_proto_library")

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

RESOURCE_DEPS = [
    "//proto/r4/core:codes_proto",
    "//proto/r4/core:datatypes_proto",
    "//proto/r4/core:extensions_proto",
    "//proto/r4/core:valuesets_proto",
    "//proto:annotations_proto",
    "@com_google_protobuf//:any_proto",
]

fhir_proto_library(
    srcs = ["actual_group.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "actual_group",
)

fhir_proto_library(
    srcs = ["cds_hooks_guidance_response.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "cds_hooks_guidance_response",
)

fhir_proto_library(
    srcs = ["cds_hooks_request_group.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "cds_hooks_request_group",
)

fhir_proto_library(
    srcs = ["cds_hooks_service_plan_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "cds_hooks_service_plan_definition",
)

fhir_proto_library(
    srcs = ["cholesterol.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "cholesterol",
)

fhir_proto_library(
    srcs = ["clinical_document.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "clinical_document",
)

fhir_proto_library(
    srcs = ["computable_plan_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "computable_plan_definition",
)

fhir_proto_library(
    srcs = ["cqf_questionnaire.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "cqf_questionnaire",
)

fhir_proto_library(
    srcs = ["cql_library.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "cql_library",
)

fhir_proto_library(
    srcs = ["data_element_constraint_on_element_definition_data_type.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "data_element_constraint_on_element_definition_data_type",
)

fhir_proto_library(
    srcs = ["device_metric_observation_profile.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "device_metric_observation_profile",
)

fhir_proto_library(
    srcs = ["diagnostic_report_genetics.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "diagnostic_report_genetics",
)

fhir_proto_library(
    srcs = ["ehrsfm_record_lifecycle_event_audit_event.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "ehrsfm_record_lifecycle_event_audit_event",
)

fhir_proto_library(
    srcs = ["ehrsfm_record_lifecycle_event_provenance.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "ehrsfm_record_lifecycle_event_provenance",
)

fhir_proto_library(
    srcs = ["evidence_synthesis_profile.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "evidence_synthesis_profile",
)

fhir_proto_library(
    srcs = ["group_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "group_definition",
)

fhir_proto_library(
    srcs = ["hdl_cholesterol.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "hdl_cholesterol",
)

fhir_proto_library(
    srcs = ["ldl_cholesterol.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "ldl_cholesterol",
)

fhir_proto_library(
    srcs = ["lipid_profile.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "lipid_profile",
)

fhir_proto_library(
    srcs = ["observation_bmi.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "observation_bmi",
)

fhir_proto_library(
    srcs = ["observation_bodyheight.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "observation_bodyheight",
)

fhir_proto_library(
    srcs = ["observation_bodytemp.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "observation_bodytemp",
)

fhir_proto_library(
    srcs = ["observation_bodyweight.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "observation_bodyweight",
)

fhir_proto_library(
    srcs = ["observation_bp.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "observation_bp",
)

fhir_proto_library(
    srcs = ["observation_genetics.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "observation_genetics",
)

fhir_proto_library(
    srcs = ["observation_headcircum.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "observation_headcircum",
)

fhir_proto_library(
    srcs = ["observation_heartrate.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "observation_heartrate",
)

fhir_proto_library(
    srcs = ["observation_oxygensat.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "observation_oxygensat",
)

fhir_proto_library(
    srcs = ["observation_resprate.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "observation_resprate",
)

fhir_proto_library(
    srcs = ["observation_vitalsigns.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "observation_vitalsigns",
)

fhir_proto_library(
    srcs = ["observation_vitalspanel.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "observation_vitalspanel",
)

fhir_proto_library(
    srcs = ["pico_element_profile.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "pico_element_profile",
)

fhir_proto_library(
    srcs = ["profile_for_catalog.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "profile_for_catalog",
)

fhir_proto_library(
    srcs = ["profile_for_hla_genotyping_results.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "profile_for_hla_genotyping_results",
)

fhir_proto_library(
    srcs = ["provenance_relevant_history.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "provenance_relevant_history",
)

fhir_proto_library(
    srcs = ["service_request_genetics.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "service_request_genetics",
)

fhir_proto_library(
    srcs = ["shareable_activity_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "shareable_activity_definition",
)

fhir_proto_library(
    srcs = ["shareable_code_system.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "shareable_code_system",
)

fhir_proto_library(
    srcs = ["shareable_library.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "shareable_library",
)

fhir_proto_library(
    srcs = ["shareable_measure.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "shareable_measure",
)

fhir_proto_library(
    srcs = ["shareable_plan_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "shareable_plan_definition",
)

fhir_proto_library(
    srcs = ["shareable_value_set.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "shareable_value_set",
)

fhir_proto_library(
    srcs = ["triglyceride.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "triglyceride",
)
