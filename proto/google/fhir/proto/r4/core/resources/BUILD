load("//bazel:proto.bzl", "fhir_proto_library")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

PROTO_DEPS = [
    ":account_proto",
    ":activity_definition_proto",
    ":adverse_event_proto",
    ":allergy_intolerance_proto",
    ":appointment_proto",
    ":appointment_response_proto",
    ":audit_event_proto",
    ":basic_proto",
    ":binary_proto",
    ":biologically_derived_product_proto",
    ":body_structure_proto",
    ":capability_statement_proto",
    ":care_plan_proto",
    ":care_team_proto",
    ":catalog_entry_proto",
    ":charge_item_definition_proto",
    ":charge_item_proto",
    ":claim_proto",
    ":claim_response_proto",
    ":clinical_impression_proto",
    ":code_system_proto",
    ":communication_proto",
    ":communication_request_proto",
    ":compartment_definition_proto",
    ":composition_proto",
    ":concept_map_proto",
    ":condition_proto",
    ":consent_proto",
    ":contract_proto",
    ":coverage_eligibility_request_proto",
    ":coverage_eligibility_response_proto",
    ":coverage_proto",
    ":detected_issue_proto",
    ":device_definition_proto",
    ":device_metric_proto",
    ":device_proto",
    ":device_request_proto",
    ":device_use_statement_proto",
    ":diagnostic_report_proto",
    ":document_manifest_proto",
    ":document_reference_proto",
    ":domain_resource_proto",
    ":effect_evidence_synthesis_proto",
    ":encounter_proto",
    ":endpoint_proto",
    ":enrollment_request_proto",
    ":enrollment_response_proto",
    ":episode_of_care_proto",
    ":event_definition_proto",
    ":evidence_proto",
    ":evidence_variable_proto",
    ":example_scenario_proto",
    ":explanation_of_benefit_proto",
    ":family_member_history_proto",
    ":flag_proto",
    ":goal_proto",
    ":graph_definition_proto",
    ":group_proto",
    ":guidance_response_proto",
    ":healthcare_service_proto",
    ":imaging_study_proto",
    ":immunization_evaluation_proto",
    ":immunization_proto",
    ":immunization_recommendation_proto",
    ":implementation_guide_proto",
    ":insurance_plan_proto",
    ":invoice_proto",
    ":library_proto",
    ":linkage_proto",
    ":list_proto",
    ":location_proto",
    ":measure_proto",
    ":measure_report_proto",
    ":media_proto",
    ":medication_administration_proto",
    ":medication_dispense_proto",
    ":medication_knowledge_proto",
    ":medication_proto",
    ":medication_request_proto",
    ":medication_statement_proto",
    ":medicinal_product_authorization_proto",
    ":medicinal_product_contraindication_proto",
    ":medicinal_product_indication_proto",
    ":medicinal_product_ingredient_proto",
    ":medicinal_product_interaction_proto",
    ":medicinal_product_manufactured_proto",
    ":medicinal_product_packaged_proto",
    ":medicinal_product_pharmaceutical_proto",
    ":medicinal_product_proto",
    ":medicinal_product_undesirable_effect_proto",
    ":message_definition_proto",
    ":message_header_proto",
    ":metadata_resource_proto",
    ":molecular_sequence_proto",
    ":naming_system_proto",
    ":nutrition_order_proto",
    ":observation_definition_proto",
    ":observation_proto",
    ":operation_definition_proto",
    ":operation_outcome_proto",
    ":organization_affiliation_proto",
    ":organization_proto",
    ":parameters_proto",
    ":patient_proto",
    ":payment_notice_proto",
    ":payment_reconciliation_proto",
    ":person_proto",
    ":plan_definition_proto",
    ":practitioner_proto",
    ":practitioner_role_proto",
    ":procedure_proto",
    ":provenance_proto",
    ":questionnaire_proto",
    ":questionnaire_response_proto",
    ":related_person_proto",
    ":request_group_proto",
    ":research_definition_proto",
    ":research_element_definition_proto",
    ":research_study_proto",
    ":research_subject_proto",
    ":resource_proto",
    ":risk_assessment_proto",
    ":risk_evidence_synthesis_proto",
    ":schedule_proto",
    ":search_parameter_proto",
    ":service_request_proto",
    ":slot_proto",
    ":specimen_definition_proto",
    ":specimen_proto",
    ":structure_definition_proto",
    ":structure_map_proto",
    ":subscription_proto",
    ":substance_nucleic_acid_proto",
    ":substance_polymer_proto",
    ":substance_protein_proto",
    ":substance_proto",
    ":substance_reference_information_proto",
    ":substance_source_material_proto",
    ":substance_specification_proto",
    ":supply_delivery_proto",
    ":supply_request_proto",
    ":task_proto",
    ":terminology_capabilities_proto",
    ":test_report_proto",
    ":test_script_proto",
    ":value_set_proto",
    ":verification_result_proto",
    ":vision_prescription_proto",
    "@com_google_protobuf//:any_proto",
    "//proto/google/fhir/proto:annotations_proto",
    "//proto/google/fhir/proto/r4/core:codes_proto",
    "//proto/google/fhir/proto/r4/core:datatypes_proto",
    "//proto/google/fhir/proto/r4/core:valuesets_proto",
]

fhir_proto_library(
    name = "bundle_and_contained_resource",
    srcs = ["bundle_and_contained_resource.proto"],
    proto_deps = PROTO_DEPS,
)

fhir_proto_library(
    name = "account",
    srcs = ["account.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "activity_definition",
    srcs = ["activity_definition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "adverse_event",
    srcs = ["adverse_event.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "allergy_intolerance",
    srcs = ["allergy_intolerance.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "appointment",
    srcs = ["appointment.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "appointment_response",
    srcs = ["appointment_response.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "audit_event",
    srcs = ["audit_event.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "basic",
    srcs = ["basic.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "binary",
    srcs = ["binary.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "biologically_derived_product",
    srcs = ["biologically_derived_product.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "body_structure",
    srcs = ["body_structure.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "capability_statement",
    srcs = ["capability_statement.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "care_plan",
    srcs = ["care_plan.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "care_team",
    srcs = ["care_team.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "catalog_entry",
    srcs = ["catalog_entry.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "charge_item",
    srcs = ["charge_item.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "charge_item_definition",
    srcs = ["charge_item_definition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "claim",
    srcs = ["claim.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "claim_response",
    srcs = ["claim_response.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "clinical_impression",
    srcs = ["clinical_impression.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "code_system",
    srcs = ["code_system.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "communication",
    srcs = ["communication.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "communication_request",
    srcs = ["communication_request.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "compartment_definition",
    srcs = ["compartment_definition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "composition",
    srcs = ["composition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "concept_map",
    srcs = ["concept_map.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "condition",
    srcs = ["condition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "consent",
    srcs = ["consent.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "contract",
    srcs = ["contract.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "coverage",
    srcs = ["coverage.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "coverage_eligibility_request",
    srcs = ["coverage_eligibility_request.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "coverage_eligibility_response",
    srcs = ["coverage_eligibility_response.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "detected_issue",
    srcs = ["detected_issue.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "device",
    srcs = ["device.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "device_definition",
    srcs = ["device_definition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "device_metric",
    srcs = ["device_metric.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "device_request",
    srcs = ["device_request.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "device_use_statement",
    srcs = ["device_use_statement.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "diagnostic_report",
    srcs = ["diagnostic_report.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "document_manifest",
    srcs = ["document_manifest.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "document_reference",
    srcs = ["document_reference.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "domain_resource",
    srcs = ["domain_resource.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "effect_evidence_synthesis",
    srcs = ["effect_evidence_synthesis.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "encounter",
    srcs = ["encounter.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "endpoint",
    srcs = ["endpoint.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "enrollment_request",
    srcs = ["enrollment_request.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "enrollment_response",
    srcs = ["enrollment_response.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "episode_of_care",
    srcs = ["episode_of_care.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "event_definition",
    srcs = ["event_definition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "evidence",
    srcs = ["evidence.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "evidence_variable",
    srcs = ["evidence_variable.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "example_scenario",
    srcs = ["example_scenario.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "explanation_of_benefit",
    srcs = ["explanation_of_benefit.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "family_member_history",
    srcs = ["family_member_history.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "flag",
    srcs = ["flag.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "goal",
    srcs = ["goal.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "graph_definition",
    srcs = ["graph_definition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "group",
    srcs = ["group.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "guidance_response",
    srcs = ["guidance_response.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "healthcare_service",
    srcs = ["healthcare_service.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "imaging_study",
    srcs = ["imaging_study.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "immunization",
    srcs = ["immunization.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "immunization_evaluation",
    srcs = ["immunization_evaluation.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "immunization_recommendation",
    srcs = ["immunization_recommendation.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "implementation_guide",
    srcs = ["implementation_guide.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "insurance_plan",
    srcs = ["insurance_plan.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "invoice",
    srcs = ["invoice.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "library",
    srcs = ["library.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "linkage",
    srcs = ["linkage.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "list",
    srcs = ["list.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "location",
    srcs = ["location.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "measure",
    srcs = ["measure.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "measure_report",
    srcs = ["measure_report.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "media",
    srcs = ["media.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medication",
    srcs = ["medication.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medication_administration",
    srcs = ["medication_administration.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medication_dispense",
    srcs = ["medication_dispense.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medication_knowledge",
    srcs = ["medication_knowledge.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medication_request",
    srcs = ["medication_request.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medication_statement",
    srcs = ["medication_statement.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medicinal_product",
    srcs = ["medicinal_product.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medicinal_product_authorization",
    srcs = ["medicinal_product_authorization.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medicinal_product_contraindication",
    srcs = ["medicinal_product_contraindication.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medicinal_product_indication",
    srcs = ["medicinal_product_indication.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medicinal_product_ingredient",
    srcs = ["medicinal_product_ingredient.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medicinal_product_interaction",
    srcs = ["medicinal_product_interaction.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medicinal_product_manufactured",
    srcs = ["medicinal_product_manufactured.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medicinal_product_packaged",
    srcs = ["medicinal_product_packaged.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medicinal_product_pharmaceutical",
    srcs = ["medicinal_product_pharmaceutical.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "medicinal_product_undesirable_effect",
    srcs = ["medicinal_product_undesirable_effect.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "message_definition",
    srcs = ["message_definition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "message_header",
    srcs = ["message_header.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "molecular_sequence",
    srcs = ["molecular_sequence.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "naming_system",
    srcs = ["naming_system.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "nutrition_order",
    srcs = ["nutrition_order.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "observation",
    srcs = ["observation.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "observation_definition",
    srcs = ["observation_definition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "operation_definition",
    srcs = ["operation_definition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "operation_outcome",
    srcs = ["operation_outcome.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "organization",
    srcs = ["organization.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "organization_affiliation",
    srcs = ["organization_affiliation.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "parameters",
    srcs = ["parameters.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "patient",
    srcs = ["patient.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "payment_notice",
    srcs = ["payment_notice.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "payment_reconciliation",
    srcs = ["payment_reconciliation.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "person",
    srcs = ["person.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "plan_definition",
    srcs = ["plan_definition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "practitioner",
    srcs = ["practitioner.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "practitioner_role",
    srcs = ["practitioner_role.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "procedure",
    srcs = ["procedure.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "provenance",
    srcs = ["provenance.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "questionnaire",
    srcs = ["questionnaire.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "questionnaire_response",
    srcs = ["questionnaire_response.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "related_person",
    srcs = ["related_person.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "request_group",
    srcs = ["request_group.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "research_definition",
    srcs = ["research_definition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "research_element_definition",
    srcs = ["research_element_definition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "research_study",
    srcs = ["research_study.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "research_subject",
    srcs = ["research_subject.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "resource",
    srcs = ["resource.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "risk_assessment",
    srcs = ["risk_assessment.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "risk_evidence_synthesis",
    srcs = ["risk_evidence_synthesis.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "schedule",
    srcs = ["schedule.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "search_parameter",
    srcs = ["search_parameter.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "service_request",
    srcs = ["service_request.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "slot",
    srcs = ["slot.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "specimen",
    srcs = ["specimen.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "specimen_definition",
    srcs = ["specimen_definition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "structure_definition",
    srcs = ["structure_definition.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "structure_map",
    srcs = ["structure_map.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "subscription",
    srcs = ["subscription.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "substance",
    srcs = ["substance.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "substance_nucleic_acid",
    srcs = ["substance_nucleic_acid.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "substance_polymer",
    srcs = ["substance_polymer.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "substance_protein",
    srcs = ["substance_protein.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "substance_reference_information",
    srcs = ["substance_reference_information.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "substance_source_material",
    srcs = ["substance_source_material.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "substance_specification",
    srcs = ["substance_specification.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "supply_delivery",
    srcs = ["supply_delivery.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "supply_request",
    srcs = ["supply_request.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "task",
    srcs = ["task.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "terminology_capabilities",
    srcs = ["terminology_capabilities.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "test_report",
    srcs = ["test_report.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "test_script",
    srcs = ["test_script.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "value_set",
    srcs = ["value_set.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "verification_result",
    srcs = ["verification_result.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "vision_prescription",
    srcs = ["vision_prescription.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

fhir_proto_library(
    name = "metadata_resource",
    srcs = ["metadata_resource.proto"],
    proto_deps = [
        "@com_google_protobuf//:any_proto",
        "//proto/google/fhir/proto:annotations_proto",
        "//proto/google/fhir/proto/r4/core:codes_proto",
        "//proto/google/fhir/proto/r4/core:datatypes_proto",
        "//proto/google/fhir/proto/r4/core:valuesets_proto",
    ],
)

# We also make the .proto files readable for testing.
filegroup(
    name = "testdata",
    srcs = glob([
        "*.proto",
    ]),
)

# Generate a java proto library that includes all core resources for distributing
# as a maven jar. No bazel targets should ever depend on this.
java_proto_library(
    name = "all_java_proto",
    visibility = ["//java/com/google/fhir/release:__pkg__"],
    deps = PROTO_DEPS + [":bundle_and_contained_resource_proto"],
)
