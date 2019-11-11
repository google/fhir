load("//bazel:proto.bzl", "fhir_proto_library")

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

RESOURCE_DEPS = [
    "//proto/r4/core:codes_proto",
    "//proto/r4/core:datatypes_proto",
    "//proto/r4/core:valuesets_proto",
    "//proto:annotations_proto",
    "@com_google_protobuf//:any_proto",
]

fhir_proto_library(
    srcs = ["bundle_and_contained_resource.proto"],
    proto_deps = RESOURCE_DEPS + [
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
        ":charge_item_proto",
        ":charge_item_definition_proto",
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
        ":coverage_proto",
        ":coverage_eligibility_request_proto",
        ":coverage_eligibility_response_proto",
        ":detected_issue_proto",
        ":device_proto",
        ":device_definition_proto",
        ":device_metric_proto",
        ":device_request_proto",
        ":device_use_statement_proto",
        ":diagnostic_report_proto",
        ":document_manifest_proto",
        ":document_reference_proto",
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
        ":immunization_proto",
        ":immunization_evaluation_proto",
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
        ":medication_proto",
        ":medication_administration_proto",
        ":medication_dispense_proto",
        ":medication_knowledge_proto",
        ":medication_request_proto",
        ":medication_statement_proto",
        ":medicinal_product_proto",
        ":medicinal_product_authorization_proto",
        ":medicinal_product_contraindication_proto",
        ":medicinal_product_indication_proto",
        ":medicinal_product_ingredient_proto",
        ":medicinal_product_interaction_proto",
        ":medicinal_product_manufactured_proto",
        ":medicinal_product_packaged_proto",
        ":medicinal_product_pharmaceutical_proto",
        ":medicinal_product_undesirable_effect_proto",
        ":message_definition_proto",
        ":message_header_proto",
        ":metadata_resource_proto",
        ":molecular_sequence_proto",
        ":naming_system_proto",
        ":nutrition_order_proto",
        ":observation_proto",
        ":observation_definition_proto",
        ":operation_definition_proto",
        ":operation_outcome_proto",
        ":organization_proto",
        ":organization_affiliation_proto",
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
        ":risk_assessment_proto",
        ":risk_evidence_synthesis_proto",
        ":schedule_proto",
        ":search_parameter_proto",
        ":service_request_proto",
        ":slot_proto",
        ":specimen_proto",
        ":specimen_definition_proto",
        ":structure_definition_proto",
        ":structure_map_proto",
        ":subscription_proto",
        ":substance_proto",
        ":substance_nucleic_acid_proto",
        ":substance_polymer_proto",
        ":substance_protein_proto",
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
    ],
    proto_library_prefix = "bundle_and_contained_resource",
)

fhir_proto_library(
    srcs = ["account.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "account",
)

fhir_proto_library(
    srcs = ["activity_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "activity_definition",
)

fhir_proto_library(
    srcs = ["adverse_event.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "adverse_event",
)

fhir_proto_library(
    srcs = ["allergy_intolerance.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "allergy_intolerance",
)

fhir_proto_library(
    srcs = ["appointment.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "appointment",
)

fhir_proto_library(
    srcs = ["appointment_response.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "appointment_response",
)

fhir_proto_library(
    srcs = ["audit_event.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "audit_event",
)

fhir_proto_library(
    srcs = ["basic.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "basic",
)

fhir_proto_library(
    srcs = ["binary.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "binary",
)

fhir_proto_library(
    srcs = ["biologically_derived_product.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "biologically_derived_product",
)

fhir_proto_library(
    srcs = ["body_structure.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "body_structure",
)

fhir_proto_library(
    srcs = ["capability_statement.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "capability_statement",
)

fhir_proto_library(
    srcs = ["care_plan.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "care_plan",
)

fhir_proto_library(
    srcs = ["care_team.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "care_team",
)

fhir_proto_library(
    srcs = ["catalog_entry.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "catalog_entry",
)

fhir_proto_library(
    srcs = ["charge_item.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "charge_item",
)

fhir_proto_library(
    srcs = ["charge_item_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "charge_item_definition",
)

fhir_proto_library(
    srcs = ["claim.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "claim",
)

fhir_proto_library(
    srcs = ["claim_response.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "claim_response",
)

fhir_proto_library(
    srcs = ["clinical_impression.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "clinical_impression",
)

fhir_proto_library(
    srcs = ["code_system.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "code_system",
)

fhir_proto_library(
    srcs = ["communication.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "communication",
)

fhir_proto_library(
    srcs = ["communication_request.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "communication_request",
)

fhir_proto_library(
    srcs = ["compartment_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "compartment_definition",
)

fhir_proto_library(
    srcs = ["composition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "composition",
)

fhir_proto_library(
    srcs = ["concept_map.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "concept_map",
)

fhir_proto_library(
    srcs = ["condition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "condition",
)

fhir_proto_library(
    srcs = ["consent.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "consent",
)

fhir_proto_library(
    srcs = ["contract.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "contract",
)

fhir_proto_library(
    srcs = ["coverage.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "coverage",
)

fhir_proto_library(
    srcs = ["coverage_eligibility_request.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "coverage_eligibility_request",
)

fhir_proto_library(
    srcs = ["coverage_eligibility_response.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "coverage_eligibility_response",
)

fhir_proto_library(
    srcs = ["detected_issue.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "detected_issue",
)

fhir_proto_library(
    srcs = ["device.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "device",
)

fhir_proto_library(
    srcs = ["device_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "device_definition",
)

fhir_proto_library(
    srcs = ["device_metric.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "device_metric",
)

fhir_proto_library(
    srcs = ["device_request.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "device_request",
)

fhir_proto_library(
    srcs = ["device_use_statement.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "device_use_statement",
)

fhir_proto_library(
    srcs = ["diagnostic_report.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "diagnostic_report",
)

fhir_proto_library(
    srcs = ["document_manifest.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "document_manifest",
)

fhir_proto_library(
    srcs = ["document_reference.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "document_reference",
)

fhir_proto_library(
    srcs = ["domain_resource.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "domain_resource",
)

fhir_proto_library(
    srcs = ["effect_evidence_synthesis.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "effect_evidence_synthesis",
)

fhir_proto_library(
    srcs = ["encounter.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "encounter",
)

fhir_proto_library(
    srcs = ["endpoint.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "endpoint",
)

fhir_proto_library(
    srcs = ["enrollment_request.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "enrollment_request",
)

fhir_proto_library(
    srcs = ["enrollment_response.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "enrollment_response",
)

fhir_proto_library(
    srcs = ["episode_of_care.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "episode_of_care",
)

fhir_proto_library(
    srcs = ["event_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "event_definition",
)

fhir_proto_library(
    srcs = ["evidence.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "evidence",
)

fhir_proto_library(
    srcs = ["evidence_variable.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "evidence_variable",
)

fhir_proto_library(
    srcs = ["example_scenario.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "example_scenario",
)

fhir_proto_library(
    srcs = ["explanation_of_benefit.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "explanation_of_benefit",
)

fhir_proto_library(
    srcs = ["family_member_history.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "family_member_history",
)

fhir_proto_library(
    srcs = ["flag.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "flag",
)

fhir_proto_library(
    srcs = ["goal.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "goal",
)

fhir_proto_library(
    srcs = ["graph_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "graph_definition",
)

fhir_proto_library(
    srcs = ["group.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "group",
)

fhir_proto_library(
    srcs = ["guidance_response.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "guidance_response",
)

fhir_proto_library(
    srcs = ["healthcare_service.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "healthcare_service",
)

fhir_proto_library(
    srcs = ["imaging_study.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "imaging_study",
)

fhir_proto_library(
    srcs = ["immunization.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "immunization",
)

fhir_proto_library(
    srcs = ["immunization_evaluation.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "immunization_evaluation",
)

fhir_proto_library(
    srcs = ["immunization_recommendation.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "immunization_recommendation",
)

fhir_proto_library(
    srcs = ["implementation_guide.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "implementation_guide",
)

fhir_proto_library(
    srcs = ["insurance_plan.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "insurance_plan",
)

fhir_proto_library(
    srcs = ["invoice.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "invoice",
)

fhir_proto_library(
    srcs = ["library.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "library",
)

fhir_proto_library(
    srcs = ["linkage.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "linkage",
)

fhir_proto_library(
    srcs = ["list.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "list",
)

fhir_proto_library(
    srcs = ["location.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "location",
)

fhir_proto_library(
    srcs = ["measure.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "measure",
)

fhir_proto_library(
    srcs = ["measure_report.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "measure_report",
)

fhir_proto_library(
    srcs = ["media.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "media",
)

fhir_proto_library(
    srcs = ["medication.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medication",
)

fhir_proto_library(
    srcs = ["medication_administration.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medication_administration",
)

fhir_proto_library(
    srcs = ["medication_dispense.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medication_dispense",
)

fhir_proto_library(
    srcs = ["medication_knowledge.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medication_knowledge",
)

fhir_proto_library(
    srcs = ["medication_request.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medication_request",
)

fhir_proto_library(
    srcs = ["medication_statement.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medication_statement",
)

fhir_proto_library(
    srcs = ["medicinal_product.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medicinal_product",
)

fhir_proto_library(
    srcs = ["medicinal_product_authorization.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medicinal_product_authorization",
)

fhir_proto_library(
    srcs = ["medicinal_product_contraindication.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medicinal_product_contraindication",
)

fhir_proto_library(
    srcs = ["medicinal_product_indication.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medicinal_product_indication",
)

fhir_proto_library(
    srcs = ["medicinal_product_ingredient.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medicinal_product_ingredient",
)

fhir_proto_library(
    srcs = ["medicinal_product_interaction.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medicinal_product_interaction",
)

fhir_proto_library(
    srcs = ["medicinal_product_manufactured.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medicinal_product_manufactured",
)

fhir_proto_library(
    srcs = ["medicinal_product_packaged.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medicinal_product_packaged",
)

fhir_proto_library(
    srcs = ["medicinal_product_pharmaceutical.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medicinal_product_pharmaceutical",
)

fhir_proto_library(
    srcs = ["medicinal_product_undesirable_effect.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "medicinal_product_undesirable_effect",
)

fhir_proto_library(
    srcs = ["message_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "message_definition",
)

fhir_proto_library(
    srcs = ["message_header.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "message_header",
)

fhir_proto_library(
    srcs = ["molecular_sequence.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "molecular_sequence",
)

fhir_proto_library(
    srcs = ["naming_system.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "naming_system",
)

fhir_proto_library(
    srcs = ["nutrition_order.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "nutrition_order",
)

fhir_proto_library(
    srcs = ["observation.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "observation",
)

fhir_proto_library(
    srcs = ["observation_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "observation_definition",
)

fhir_proto_library(
    srcs = ["operation_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "operation_definition",
)

fhir_proto_library(
    srcs = ["operation_outcome.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "operation_outcome",
)

fhir_proto_library(
    srcs = ["organization.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "organization",
)

fhir_proto_library(
    srcs = ["organization_affiliation.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "organization_affiliation",
)

fhir_proto_library(
    srcs = ["parameters.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "parameters",
)

fhir_proto_library(
    srcs = ["patient.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "patient",
)

fhir_proto_library(
    srcs = ["payment_notice.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "payment_notice",
)

fhir_proto_library(
    srcs = ["payment_reconciliation.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "payment_reconciliation",
)

fhir_proto_library(
    srcs = ["person.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "person",
)

fhir_proto_library(
    srcs = ["plan_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "plan_definition",
)

fhir_proto_library(
    srcs = ["practitioner.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "practitioner",
)

fhir_proto_library(
    srcs = ["practitioner_role.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "practitioner_role",
)

fhir_proto_library(
    srcs = ["procedure.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "procedure",
)

fhir_proto_library(
    srcs = ["provenance.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "provenance",
)

fhir_proto_library(
    srcs = ["questionnaire.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "questionnaire",
)

fhir_proto_library(
    srcs = ["questionnaire_response.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "questionnaire_response",
)

fhir_proto_library(
    srcs = ["related_person.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "related_person",
)

fhir_proto_library(
    srcs = ["request_group.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "request_group",
)

fhir_proto_library(
    srcs = ["research_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "research_definition",
)

fhir_proto_library(
    srcs = ["research_element_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "research_element_definition",
)

fhir_proto_library(
    srcs = ["research_study.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "research_study",
)

fhir_proto_library(
    srcs = ["research_subject.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "research_subject",
)

fhir_proto_library(
    srcs = ["resource.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "resource",
)

fhir_proto_library(
    srcs = ["risk_assessment.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "risk_assessment",
)

fhir_proto_library(
    srcs = ["risk_evidence_synthesis.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "risk_evidence_synthesis",
)

fhir_proto_library(
    srcs = ["schedule.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "schedule",
)

fhir_proto_library(
    srcs = ["search_parameter.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "search_parameter",
)

fhir_proto_library(
    srcs = ["service_request.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "service_request",
)

fhir_proto_library(
    srcs = ["slot.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "slot",
)

fhir_proto_library(
    srcs = ["specimen.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "specimen",
)

fhir_proto_library(
    srcs = ["specimen_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "specimen_definition",
)

fhir_proto_library(
    srcs = ["structure_definition.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "structure_definition",
)

fhir_proto_library(
    srcs = ["structure_map.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "structure_map",
)

fhir_proto_library(
    srcs = ["subscription.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "subscription",
)

fhir_proto_library(
    srcs = ["substance.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "substance",
)

fhir_proto_library(
    srcs = ["substance_nucleic_acid.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "substance_nucleic_acid",
)

fhir_proto_library(
    srcs = ["substance_polymer.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "substance_polymer",
)

fhir_proto_library(
    srcs = ["substance_protein.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "substance_protein",
)

fhir_proto_library(
    srcs = ["substance_reference_information.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "substance_reference_information",
)

fhir_proto_library(
    srcs = ["substance_source_material.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "substance_source_material",
)

fhir_proto_library(
    srcs = ["substance_specification.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "substance_specification",
)

fhir_proto_library(
    srcs = ["supply_delivery.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "supply_delivery",
)

fhir_proto_library(
    srcs = ["supply_request.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "supply_request",
)

fhir_proto_library(
    srcs = ["task.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "task",
)

fhir_proto_library(
    srcs = ["terminology_capabilities.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "terminology_capabilities",
)

fhir_proto_library(
    srcs = ["test_report.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "test_report",
)

fhir_proto_library(
    srcs = ["test_script.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "test_script",
)

fhir_proto_library(
    srcs = ["value_set.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "value_set",
)

fhir_proto_library(
    srcs = ["verification_result.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "verification_result",
)

fhir_proto_library(
    srcs = ["vision_prescription.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "vision_prescription",
)

fhir_proto_library(
    srcs = ["metadata_resource.proto"],
    proto_deps = RESOURCE_DEPS,
    proto_library_prefix = "metadata_resource",
)

# We also make the .proto files readable for testing.
filegroup(
    name = "testdata",
    srcs = glob([
        "*.proto",
    ]),
)
