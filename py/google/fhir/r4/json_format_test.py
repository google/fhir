#
# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Test FHIR R4 parsing/printing functionality."""

import os
from typing import Type, TypeVar

from google.protobuf import message
from absl.testing import absltest
from absl.testing import parameterized
from proto.google.fhir.proto.r4.core import datatypes_pb2
from proto.google.fhir.proto.r4.core.resources import account_pb2
from proto.google.fhir.proto.r4.core.resources import activity_definition_pb2
from proto.google.fhir.proto.r4.core.resources import adverse_event_pb2
from proto.google.fhir.proto.r4.core.resources import allergy_intolerance_pb2
from proto.google.fhir.proto.r4.core.resources import appointment_pb2
from proto.google.fhir.proto.r4.core.resources import appointment_response_pb2
from proto.google.fhir.proto.r4.core.resources import audit_event_pb2
from proto.google.fhir.proto.r4.core.resources import basic_pb2
from proto.google.fhir.proto.r4.core.resources import binary_pb2
from proto.google.fhir.proto.r4.core.resources import biologically_derived_product_pb2
from proto.google.fhir.proto.r4.core.resources import body_structure_pb2
from proto.google.fhir.proto.r4.core.resources import bundle_and_contained_resource_pb2
from proto.google.fhir.proto.r4.core.resources import capability_statement_pb2
from proto.google.fhir.proto.r4.core.resources import care_plan_pb2
from proto.google.fhir.proto.r4.core.resources import care_team_pb2
from proto.google.fhir.proto.r4.core.resources import catalog_entry_pb2
from proto.google.fhir.proto.r4.core.resources import charge_item_definition_pb2
from proto.google.fhir.proto.r4.core.resources import charge_item_pb2
from proto.google.fhir.proto.r4.core.resources import claim_pb2
from proto.google.fhir.proto.r4.core.resources import claim_response_pb2
from proto.google.fhir.proto.r4.core.resources import clinical_impression_pb2
from proto.google.fhir.proto.r4.core.resources import code_system_pb2
from proto.google.fhir.proto.r4.core.resources import communication_pb2
from proto.google.fhir.proto.r4.core.resources import communication_request_pb2
from proto.google.fhir.proto.r4.core.resources import compartment_definition_pb2
from proto.google.fhir.proto.r4.core.resources import composition_pb2
from proto.google.fhir.proto.r4.core.resources import condition_pb2
from proto.google.fhir.proto.r4.core.resources import consent_pb2
from proto.google.fhir.proto.r4.core.resources import contract_pb2
from proto.google.fhir.proto.r4.core.resources import coverage_eligibility_request_pb2
from proto.google.fhir.proto.r4.core.resources import coverage_eligibility_response_pb2
from proto.google.fhir.proto.r4.core.resources import coverage_pb2
from proto.google.fhir.proto.r4.core.resources import detected_issue_pb2
from proto.google.fhir.proto.r4.core.resources import device_definition_pb2
from proto.google.fhir.proto.r4.core.resources import device_metric_pb2
from proto.google.fhir.proto.r4.core.resources import device_pb2
from proto.google.fhir.proto.r4.core.resources import device_request_pb2
from proto.google.fhir.proto.r4.core.resources import device_use_statement_pb2
from proto.google.fhir.proto.r4.core.resources import diagnostic_report_pb2
from proto.google.fhir.proto.r4.core.resources import document_manifest_pb2
from proto.google.fhir.proto.r4.core.resources import document_reference_pb2
from proto.google.fhir.proto.r4.core.resources import effect_evidence_synthesis_pb2
from proto.google.fhir.proto.r4.core.resources import encounter_pb2
from proto.google.fhir.proto.r4.core.resources import endpoint_pb2
from proto.google.fhir.proto.r4.core.resources import enrollment_request_pb2
from proto.google.fhir.proto.r4.core.resources import enrollment_response_pb2
from proto.google.fhir.proto.r4.core.resources import episode_of_care_pb2
from proto.google.fhir.proto.r4.core.resources import event_definition_pb2
from proto.google.fhir.proto.r4.core.resources import evidence_pb2
from proto.google.fhir.proto.r4.core.resources import evidence_variable_pb2
from proto.google.fhir.proto.r4.core.resources import example_scenario_pb2
from proto.google.fhir.proto.r4.core.resources import explanation_of_benefit_pb2
from proto.google.fhir.proto.r4.core.resources import family_member_history_pb2
from proto.google.fhir.proto.r4.core.resources import flag_pb2
from proto.google.fhir.proto.r4.core.resources import goal_pb2
from proto.google.fhir.proto.r4.core.resources import graph_definition_pb2
from proto.google.fhir.proto.r4.core.resources import group_pb2
from proto.google.fhir.proto.r4.core.resources import guidance_response_pb2
from proto.google.fhir.proto.r4.core.resources import healthcare_service_pb2
from proto.google.fhir.proto.r4.core.resources import imaging_study_pb2
from proto.google.fhir.proto.r4.core.resources import immunization_evaluation_pb2
from proto.google.fhir.proto.r4.core.resources import immunization_pb2
from proto.google.fhir.proto.r4.core.resources import immunization_recommendation_pb2
from proto.google.fhir.proto.r4.core.resources import implementation_guide_pb2
from proto.google.fhir.proto.r4.core.resources import insurance_plan_pb2
from proto.google.fhir.proto.r4.core.resources import invoice_pb2
from proto.google.fhir.proto.r4.core.resources import library_pb2
from proto.google.fhir.proto.r4.core.resources import linkage_pb2
from proto.google.fhir.proto.r4.core.resources import list_pb2
from proto.google.fhir.proto.r4.core.resources import location_pb2
from proto.google.fhir.proto.r4.core.resources import measure_pb2
from proto.google.fhir.proto.r4.core.resources import measure_report_pb2
from proto.google.fhir.proto.r4.core.resources import media_pb2
from proto.google.fhir.proto.r4.core.resources import medication_administration_pb2
from proto.google.fhir.proto.r4.core.resources import medication_dispense_pb2
from proto.google.fhir.proto.r4.core.resources import medication_knowledge_pb2
from proto.google.fhir.proto.r4.core.resources import medication_pb2
from proto.google.fhir.proto.r4.core.resources import medication_request_pb2
from proto.google.fhir.proto.r4.core.resources import medication_statement_pb2
from proto.google.fhir.proto.r4.core.resources import medicinal_product_authorization_pb2
from proto.google.fhir.proto.r4.core.resources import medicinal_product_contraindication_pb2
from proto.google.fhir.proto.r4.core.resources import medicinal_product_indication_pb2
from proto.google.fhir.proto.r4.core.resources import medicinal_product_ingredient_pb2
from proto.google.fhir.proto.r4.core.resources import medicinal_product_interaction_pb2
from proto.google.fhir.proto.r4.core.resources import medicinal_product_manufactured_pb2
from proto.google.fhir.proto.r4.core.resources import medicinal_product_packaged_pb2
from proto.google.fhir.proto.r4.core.resources import medicinal_product_pb2
from proto.google.fhir.proto.r4.core.resources import medicinal_product_pharmaceutical_pb2
from proto.google.fhir.proto.r4.core.resources import medicinal_product_undesirable_effect_pb2
from proto.google.fhir.proto.r4.core.resources import message_definition_pb2
from proto.google.fhir.proto.r4.core.resources import message_header_pb2
from proto.google.fhir.proto.r4.core.resources import molecular_sequence_pb2
from proto.google.fhir.proto.r4.core.resources import naming_system_pb2
from proto.google.fhir.proto.r4.core.resources import nutrition_order_pb2
from proto.google.fhir.proto.r4.core.resources import observation_definition_pb2
from proto.google.fhir.proto.r4.core.resources import observation_pb2
from proto.google.fhir.proto.r4.core.resources import operation_definition_pb2
from proto.google.fhir.proto.r4.core.resources import operation_outcome_pb2
from proto.google.fhir.proto.r4.core.resources import organization_affiliation_pb2
from proto.google.fhir.proto.r4.core.resources import organization_pb2
from proto.google.fhir.proto.r4.core.resources import parameters_pb2
from proto.google.fhir.proto.r4.core.resources import patient_pb2
from proto.google.fhir.proto.r4.core.resources import payment_notice_pb2
from proto.google.fhir.proto.r4.core.resources import payment_reconciliation_pb2
from proto.google.fhir.proto.r4.core.resources import person_pb2
from proto.google.fhir.proto.r4.core.resources import plan_definition_pb2
from proto.google.fhir.proto.r4.core.resources import practitioner_pb2
from proto.google.fhir.proto.r4.core.resources import practitioner_role_pb2
from proto.google.fhir.proto.r4.core.resources import procedure_pb2
from proto.google.fhir.proto.r4.core.resources import provenance_pb2
from proto.google.fhir.proto.r4.core.resources import questionnaire_pb2
from proto.google.fhir.proto.r4.core.resources import questionnaire_response_pb2
from proto.google.fhir.proto.r4.core.resources import related_person_pb2
from proto.google.fhir.proto.r4.core.resources import request_group_pb2
from proto.google.fhir.proto.r4.core.resources import research_definition_pb2
from proto.google.fhir.proto.r4.core.resources import research_element_definition_pb2
from proto.google.fhir.proto.r4.core.resources import research_study_pb2
from proto.google.fhir.proto.r4.core.resources import research_subject_pb2
from proto.google.fhir.proto.r4.core.resources import risk_assessment_pb2
from proto.google.fhir.proto.r4.core.resources import risk_evidence_synthesis_pb2
from proto.google.fhir.proto.r4.core.resources import schedule_pb2
from proto.google.fhir.proto.r4.core.resources import service_request_pb2
from proto.google.fhir.proto.r4.core.resources import slot_pb2
from proto.google.fhir.proto.r4.core.resources import specimen_definition_pb2
from proto.google.fhir.proto.r4.core.resources import specimen_pb2
from proto.google.fhir.proto.r4.core.resources import structure_definition_pb2
from proto.google.fhir.proto.r4.core.resources import structure_map_pb2
from proto.google.fhir.proto.r4.core.resources import subscription_pb2
from proto.google.fhir.proto.r4.core.resources import substance_pb2
from proto.google.fhir.proto.r4.core.resources import substance_specification_pb2
from proto.google.fhir.proto.r4.core.resources import supply_delivery_pb2
from proto.google.fhir.proto.r4.core.resources import supply_request_pb2
from proto.google.fhir.proto.r4.core.resources import task_pb2
from proto.google.fhir.proto.r4.core.resources import terminology_capabilities_pb2
from proto.google.fhir.proto.r4.core.resources import test_report_pb2
from proto.google.fhir.proto.r4.core.resources import test_script_pb2
from proto.google.fhir.proto.r4.core.resources import verification_result_pb2
from proto.google.fhir.proto.r4.core.resources import vision_prescription_pb2
from google.fhir.core import fhir_errors
from google.fhir.core.internal.json_format import json_format_test
from google.fhir.core.testing import testdata_utils
from google.fhir.core.utils import proto_utils
from google.fhir.r4 import json_format

_BIGQUERY_PATH = os.path.join('testdata', 'r4', 'bigquery')
_EXAMPLES_PATH = os.path.join('testdata', 'r4', 'examples')
_FHIR_SPEC_PATH = os.path.join('spec', 'hl7.fhir.r4.examples', '4.0.1',
                               'package')
_VALIDATION_PATH = os.path.join('testdata', 'r4', 'validation')

_INVALID_RECORDS = frozenset([
    os.path.join(_FHIR_SPEC_PATH, 'Bundle-dataelements.json'),
    os.path.join(_FHIR_SPEC_PATH, 'Questionnaire-qs1.json'),
    os.path.join(_FHIR_SPEC_PATH, 'Observation-clinical-gender.json'),
    os.path.join(_FHIR_SPEC_PATH, 'DeviceMetric-example.json'),
    os.path.join(_FHIR_SPEC_PATH, 'DeviceUseStatement-example.json'),
    os.path.join(_FHIR_SPEC_PATH, 'MedicationRequest-medrx0301.json'),
])

_T = TypeVar('_T', bound=message.Message)


class JsonFormatTest(json_format_test.JsonFormatTest):
  """Unit tests for functionality in json_format.py."""

  @parameterized.named_parameters(
      ('with_code_system_v20003', 'CodeSystem-v2-0003'),
      ('with_code_systemv20061', 'CodeSystem-v2-0061'),
  )
  def test_json_format_for_resource_with_primitive_extension_nested_choice_type_succeeds(
      self, file_name: str
  ):
    """Tests parsing/printing with a primitive extension nested choice field."""
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   code_system_pb2.CodeSystem)

  @parameterized.named_parameters(
      ('with_account_ewg', 'Account-ewg'),
      ('with_account_example', 'Account-example'),
  )
  def test_json_format_for_valid_account_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   account_pb2.Account)

  @parameterized.named_parameters(
      (
          'with_activity_definition_administer_zika_virus_exposure_assessment',
          'ActivityDefinition-administer-zika-virus-exposure-assessment',
      ),
      (
          'with_activity_definition_blood_tubes_supply',
          'ActivityDefinition-blood-tubes-supply',
      ),
      (
          'with_activity_definition_citalopram_prescription',
          'ActivityDefinition-citalopramPrescription',
      ),
      (
          'with_activity_definition_heart_valve_replacement',
          'ActivityDefinition-heart-valve-replacement',
      ),
      (
          'with_activity_definition_provide_mosquito_prevention_advice',
          'ActivityDefinition-provide-mosquito-prevention-advice',
      ),
      (
          'with_activity_definition_referral_primary_care_mental_health_initial',
          'ActivityDefinition-referralPrimaryCareMentalHealth-initial',
      ),
      (
          'with_activity_definition_referral_primary_care_mental_health',
          'ActivityDefinition-referralPrimaryCareMentalHealth',
      ),
      (
          'with_activity_definition_serum_dengue_virus_igm',
          'ActivityDefinition-serum-dengue-virus-igm',
      ),
      (
          'with_activity_definition_serum_zika_dengue_virus_igm',
          'ActivityDefinition-serum-zika-dengue-virus-igm',
      ),
  )
  def test_json_format_for_valid_activity_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, activity_definition_pb2.ActivityDefinition)

  @parameterized.named_parameters(
      ('with_adverse_event_example', 'AdverseEvent-example'),
  )
  def test_json_format_for_valid_adverse_event_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, adverse_event_pb2.AdverseEvent)

  @parameterized.named_parameters(
      ('with_allergy_intolerance_example', 'AllergyIntolerance-example'),
      (
          'with_allergy_intolerance_fishallergy',
          'AllergyIntolerance-fishallergy',
      ),
      ('with_allergy_intolerance_medication', 'AllergyIntolerance-medication'),
      ('with_allergy_intolerance_nka', 'AllergyIntolerance-nka'),
      ('with_allergy_intolerance_nkda', 'AllergyIntolerance-nkda'),
      ('with_allergy_intolerance_nkla', 'AllergyIntolerance-nkla'),
  )
  def test_json_format_for_valid_allergy_intolerance_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, allergy_intolerance_pb2.AllergyIntolerance)

  @parameterized.named_parameters(
      ('with_appointment2docs', 'Appointment-2docs'),
      ('with_appointment_example', 'Appointment-example'),
      ('with_appointment_example_req', 'Appointment-examplereq'),
  )
  def test_json_format_for_valid_appointment_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   appointment_pb2.Appointment)

  @parameterized.named_parameters(
      ('with_appointment_response_example', 'AppointmentResponse-example'),
      (
          'with_appointment_response_example_resp',
          'AppointmentResponse-exampleresp',
      ),
  )
  def test_json_format_for_valid_appointment_response_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, appointment_response_pb2.AppointmentResponse)

  @parameterized.named_parameters(
      ('with_audit_event_example_disclosure', 'AuditEvent-example-disclosure'),
      ('with_audit_event_example_error', 'AuditEvent-example-error'),
      ('with_audit_event_example', 'AuditEvent-example'),
      ('with_audit_event_example_login', 'AuditEvent-example-login'),
      ('with_audit_event_example_logout', 'AuditEvent-example-logout'),
      ('with_audit_event_example_media', 'AuditEvent-example-media'),
      ('with_audit_event_example_pix_query', 'AuditEvent-example-pixQuery'),
      ('with_audit_event_example_rest', 'AuditEvent-example-rest'),
      ('with_audit_event_example_search', 'AuditEvent-example-search'),
  )
  def test_json_format_for_valid_audit_event_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   audit_event_pb2.AuditEvent)

  @parameterized.named_parameters(
      ('with_basic_basic_example_narrative', 'Basic-basic-example-narrative'),
      ('with_basic_class_model', 'Basic-classModel'),
      ('with_basic_referral', 'Basic-referral'),
  )
  def test_json_format_for_valid_basic_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, basic_pb2.Basic)

  @parameterized.named_parameters(
      ('with_binary_example', 'Binary-example'),
      ('with_binary_f006', 'Binary-f006'),
  )
  def test_json_format_for_valid_binary_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, binary_pb2.Binary)

  @parameterized.named_parameters(
      (
          'with_biologically_derived_product_example',
          'BiologicallyDerivedProduct-example',
      ),
  )
  def test_json_format_for_valid_biologically_derived_product_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, biologically_derived_product_pb2.BiologicallyDerivedProduct)

  @parameterized.named_parameters(
      ('with_body_structure_fetus', 'BodyStructure-fetus'),
      ('with_body_structure_skin_patch', 'BodyStructure-skin-patch'),
      ('with_body_structure_tumor', 'BodyStructure-tumor'),
  )
  def test_json_format_for_valid_body_structure_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, body_structure_pb2.BodyStructure)

  @parameterized.named_parameters(
      ('with_bundle101', 'Bundle-101'),
      (
          'with_bundle10bb101f_a1214264a92067be9cb82c74',
          'Bundle-10bb101f-a121-4264-a920-67be9cb82c74',
      ),
      (
          'with_bundle3a0707d3549e4467b8b85a2ab3800efe',
          'Bundle-3a0707d3-549e-4467-b8b8-5a2ab3800efe',
      ),
      (
          'with_bundle3ad0687e_f477468c_afd5_fcc2bf897808',
          'Bundle-3ad0687e-f477-468c-afd5-fcc2bf897808',
      ),
      (
          'with_bundle3ad0687e_f477468c_afd5_fcc2bf897809',
          'Bundle-3ad0687e-f477-468c-afd5-fcc2bf897809',
      ),
      (
          'with_bundle3ad0687e_f477468c_afd5_fcc2bf897819',
          'Bundle-3ad0687e-f477-468c-afd5-fcc2bf897819',
      ),
      (
          'with_bundle72ac849352ac41bd8d5d7258c289b5ea',
          'Bundle-72ac8493-52ac-41bd-8d5d-7258c289b5ea',
      ),
      (
          'with_bundle_b0a5e427783c44adb87e2e3efe3369b6f',
          'Bundle-b0a5e4277-83c4-4adb-87e2-e3efe3369b6f',
      ),
      (
          'with_bundle_b248b1b216864b94993637d7a5f94b51',
          'Bundle-b248b1b2-1686-4b94-9936-37d7a5f94b51',
      ),
      ('with_bundle_bundle_example', 'Bundle-bundle-example'),
      ('with_bundle_bundle_references', 'Bundle-bundle-references'),
      (
          'with_bundle_bundle_request_medsallergies',
          'Bundle-bundle-request-medsallergies',
      ),
      (
          'with_bundle_bundle_request_simple_summary',
          'Bundle-bundle-request-simplesummary',
      ),
      ('with_bundle_bundle_response', 'Bundle-bundle-response'),
      (
          'with_bundle_bundle_response_meds_allergies',
          'Bundle-bundle-response-medsallergies',
      ),
      (
          'with_bundle_bundle_response_simple_summary',
          'Bundle-bundle-response-simplesummary',
      ),
      ('with_bundle_bundle_search_warning', 'Bundle-bundle-search-warning'),
      ('with_bundle_bundle_transaction', 'Bundle-bundle-transaction'),
      ('with_bundle_concept_maps', 'Bundle-conceptmaps'),
      # TODO(b/152317976): Investigate test timeouts
      # ('with_bundle_data_elements', 'Bundle-dataelements'),
      # ('with_bundle_dg2', 'Bundle-dg2'),
      # ('with_bundle_extensions', 'Bundle-extensions'),
      # ('with_bundle_externals', 'Bundle-externals'),
      # ('with_bundle_f001', 'Bundle-f001'),
      # ('with_bundle_f202', 'Bundle-f202'),
      # ('with_bundle_father', 'Bundle-father'),
      # ('with_bundle_ghp', 'Bundle-ghp'),
      # ('with_bundle_hla1', 'Bundle-hla-1'),
      # ('with_bundle_lipids', 'Bundle-lipids'),
      # ('with_bundle_lri_example', 'Bundle-lri-example'),
      # ('with_bundle_micro', 'Bundle-micro'),
      # ('with_bundle_profiles_others', 'Bundle-profiles-others'),
      # ('with_bundle_registry', 'Bundle-registry'),
      # ('with_bundle_report', 'Bundle-report'),
      # ('with_bundle_search_params', 'Bundle-searchParams'),
      # ('with_bundle_terminologies', 'Bundle-terminologies'),
      # ('with_bundle_types', 'Bundle-types'),
      # ('with_bundle_ussg_fht', 'Bundle-ussg-fht'),
      # ('with_bundle_valueset_expansions', 'Bundle-valueset-expansions'),
      # ('with_bundle_xds', 'Bundle-xds'),
      # ('with_bundle_resources', 'Bundle-resources'),
  )
  def test_json_format_for_valid_bundle_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, bundle_and_contained_resource_pb2.Bundle)

  @parameterized.named_parameters(
      ('with_capability_statement_base2', 'CapabilityStatement-base2'),
      ('with_capability_statement_base', 'CapabilityStatement-base'),
      ('with_capability_statement_example', 'CapabilityStatement-example'),
      (
          'with_capability_statement_knowledge_repository',
          'CapabilityStatement-knowledge-repository',
      ),
      (
          'with_capability_statement_measure_processor',
          'CapabilityStatement-measure-processor',
      ),
      (
          'with_capability_statement_messagedefinition',
          'CapabilityStatement-messagedefinition',
      ),
      ('with_capability_statement_phr', 'CapabilityStatement-phr'),
      (
          'with_capability_statement_terminology_server',
          'CapabilityStatement-terminology-server',
      ),
  )
  def test_json_format_for_valid_capability_statement_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, capability_statement_pb2.CapabilityStatement)

  @parameterized.named_parameters(
      ('with_care_plan_example', 'CarePlan-example'),
      ('with_care_plan_f001', 'CarePlan-f001'),
      ('with_care_plan_f002', 'CarePlan-f002'),
      ('with_care_plan_f003', 'CarePlan-f003'),
      ('with_care_plan_f201', 'CarePlan-f201'),
      ('with_care_plan_f202', 'CarePlan-f202'),
      ('with_care_plan_f203', 'CarePlan-f203'),
      ('with_care_plan_gpvisit', 'CarePlan-gpvisit'),
      ('with_care_plan_integrate', 'CarePlan-integrate'),
      ('with_care_plan_obesity_narrative', 'CarePlan-obesity-narrative'),
      ('with_care_plan_preg', 'CarePlan-preg'),
  )
  def test_json_format_for_valid_care_plan_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   care_plan_pb2.CarePlan)

  @parameterized.named_parameters(
      ('with_care_team_example', 'CareTeam-example'),
  )
  def test_json_format_for_valid_care_team_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   care_team_pb2.CareTeam)

  @parameterized.named_parameters(
      ('with_catalog_entry_example', 'CatalogEntry-example'),
  )
  def test_json_format_for_valid_catalog_entry_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, catalog_entry_pb2.CatalogEntry)

  @parameterized.named_parameters(
      ('with_charge_item_example', 'ChargeItem-example'),
  )
  def test_json_format_for_valid_charge_item_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   charge_item_pb2.ChargeItem)

  @parameterized.named_parameters(
      ('with_charge_item_definition_device', 'ChargeItemDefinition-device'),
      ('with_charge_item_definition_ebm', 'ChargeItemDefinition-ebm'),
  )
  def test_json_format_for_valid_charge_item_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, charge_item_definition_pb2.ChargeItemDefinition)

  @parameterized.named_parameters(
      ('with_claim100150', 'Claim-100150'),
      ('with_claim100151', 'Claim-100151'),
      ('with_claim100152', 'Claim-100152'),
      ('with_claim100153', 'Claim-100153'),
      ('with_claim100154', 'Claim-100154'),
      ('with_claim100155', 'Claim-100155'),
      ('with_claim100156', 'Claim-100156'),
      ('with_claim660150', 'Claim-660150'),
      ('with_claim660151', 'Claim-660151'),
      ('with_claim660152', 'Claim-660152'),
      ('with_claim760150', 'Claim-760150'),
      ('with_claim760151', 'Claim-760151'),
      ('with_claim760152', 'Claim-760152'),
      ('with_claim860150', 'Claim-860150'),
      ('with_claim960150', 'Claim-960150'),
      ('with_claim960151', 'Claim-960151'),
      ('with_claim_med00050', 'Claim-MED-00050'),
  )
  def test_json_format_for_valid_claim_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, claim_pb2.Claim)

  @parameterized.named_parameters(
      ('with_claim_response_r3500', 'ClaimResponse-R3500'),
      ('with_claim_response_r3501', 'ClaimResponse-R3501'),
      ('with_claim_response_r3502', 'ClaimResponse-R3502'),
      ('with_claim_response_r3503', 'ClaimResponse-R3503'),
      ('with_claim_response_ur3503', 'ClaimResponse-UR3503'),
  )
  def test_json_format_for_valid_claim_response_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, claim_response_pb2.ClaimResponse)

  @parameterized.named_parameters(
      ('with_clinical_impression_example', 'ClinicalImpression-example'),
  )
  def test_json_format_for_valid_clinical_impression_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, clinical_impression_pb2.ClinicalImpression)

  @parameterized.named_parameters(
      ('with_code_system_example', 'CodeSystem-example'),
      ('with_code_system_list_example_codes', 'CodeSystem-list-example-codes'),
  )
  def test_json_format_for_valid_code_system_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   code_system_pb2.CodeSystem)

  @parameterized.named_parameters(
      ('with_communication_example', 'Communication-example'),
      ('with_communication_fm_attachment', 'Communication-fm-attachment'),
      ('with_communication_fm_solicited', 'Communication-fm-solicited'),
  )
  def test_json_format_for_valid_communication_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, communication_pb2.Communication)

  @parameterized.named_parameters(
      ('with_communication_request_example', 'CommunicationRequest-example'),
      (
          'with_communication_request_fm_solicit',
          'CommunicationRequest-fm-solicit',
      ),
  )
  def test_json_format_for_valid_communication_request_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, communication_request_pb2.CommunicationRequest)

  @parameterized.named_parameters(
      ('with_compartment_definition_device', 'CompartmentDefinition-device'),
      (
          'with_compartment_definition_encounter',
          'CompartmentDefinition-encounter',
      ),
      ('with_compartment_definition_example', 'CompartmentDefinition-example'),
      ('with_compartment_definition_patient', 'CompartmentDefinition-patient'),
      (
          'with_compartment_definition_practitioner',
          'CompartmentDefinition-practitioner',
      ),
      (
          'with_compartment_definition_related_person',
          'CompartmentDefinition-relatedPerson',
      ),
  )
  def test_json_format_for_valid_compartment_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, compartment_definition_pb2.CompartmentDefinition)

  @parameterized.named_parameters(
      ('with_composition_example', 'Composition-example'),
      ('with_composition_example_mixed', 'Composition-example-mixed'),
  )
  def test_json_format_for_valid_composition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   composition_pb2.Composition)

  @parameterized.named_parameters(
      ('with_condition_example2', 'Condition-example2'),
      ('with_condition_example', 'Condition-example'),
      ('with_condition_f001', 'Condition-f001'),
      ('with_condition_f002', 'Condition-f002'),
      ('with_condition_f003', 'Condition-f003'),
      ('with_condition_f201', 'Condition-f201'),
      ('with_condition_f202', 'Condition-f202'),
      ('with_condition_f203', 'Condition-f203'),
      ('with_condition_f204', 'Condition-f204'),
      ('with_condition_f205', 'Condition-f205'),
      ('with_condition_family_history', 'Condition-family-history'),
      ('with_condition_stroke', 'Condition-stroke'),
  )
  def test_json_format_for_valid_condition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   condition_pb2.Condition)

  @parameterized.named_parameters(
      ('with_consent_consent_example_basic', 'Consent-consent-example-basic'),
      (
          'with_consent_consent_example_emergency',
          'Consent-consent-example-Emergency',
      ),
      (
          'with_consent_consent_example_grantor',
          'Consent-consent-example-grantor',
      ),
      (
          'with_consent_consent_example_not_author',
          'Consent-consent-example-notAuthor',
      ),
      (
          'with_consent_consent_example_not_org',
          'Consent-consent-example-notOrg',
      ),
      (
          'with_consent_consent_example_not_them',
          'Consent-consent-example-notThem',
      ),
      (
          'with_consent_consent_example_not_this',
          'Consent-consent-example-notThis',
      ),
      (
          'with_consent_consent_example_not_time',
          'Consent-consent-example-notTime',
      ),
      ('with_consent_consent_example_out', 'Consent-consent-example-Out'),
      ('with_consent_consent_example_pkb', 'Consent-consent-example-pkb'),
      (
          'with_consent_consent_example_signature',
          'Consent-consent-example-signature',
      ),
      (
          'with_consent_consent_example_smartonfhir',
          'Consent-consent-example-smartonfhir',
      ),
  )
  def test_json_format_for_valid_consent_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   consent_pb2.Consent)

  @parameterized.named_parameters(
      (
          'with_allergy_intolerance_example',
          'AllergyIntolerance-example',
          allergy_intolerance_pb2.AllergyIntolerance,
          'allergy_intolerance',
      ),
      (
          'with_capability_statement_base',
          'CapabilityStatement-base',
          capability_statement_pb2.CapabilityStatement,
          'capability_statement',
      ),
      (
          'with_immunization_example',
          'Immunization-example',
          immunization_pb2.Immunization,
          'immunization',
      ),
      (
          'with_medication_med0305',
          'Medication-med0305',
          medication_pb2.Medication,
          'medication',
      ),
      (
          'with_observation_f004',
          'Observation-f004',
          observation_pb2.Observation,
          'observation',
      ),
      ('with_patient_animal', 'Patient-animal', patient_pb2.Patient, 'patient'),
      (
          'with_practitioner_f003',
          'Practitioner-f003',
          practitioner_pb2.Practitioner,
          'practitioner',
      ),
      (
          'with_procedure_ambulation',
          'Procedure-ambulation',
          procedure_pb2.Procedure,
          'procedure',
      ),
      ('with_task_example4', 'Task-example4', task_pb2.Task, 'task'),
  )
  def test_json_format_for_valid_contained_resource_succeeds(
      self,
      file_name: str,
      proto_cls: Type[message.Message],
      contained_field: str,
  ):
    """Checks equality of print-parse 'round-trip' for a contained resource."""
    proto_path = os.path.join(_EXAMPLES_PATH, file_name + '.prototxt')
    golden_proto = testdata_utils.read_protos(proto_path, proto_cls)[0]

    # Construct the contained resource to validate
    contained = bundle_and_contained_resource_pb2.ContainedResource()
    proto_utils.set_value_at_field(contained, contained_field, golden_proto)

    # Validate printing and then parsing the print output against the golden
    contained_json_str = json_format.print_fhir_to_json_string(contained)
    parsed_contained = json_format.json_fhir_string_to_proto(
        contained_json_str,
        bundle_and_contained_resource_pb2.ContainedResource,
        validate=True,
        default_timezone='Australia/Sydney')
    self.assertEqual(contained, parsed_contained)

  @parameterized.named_parameters(
      ('with_contract_c123', 'Contract-C-123'),
      ('with_contract_c2121', 'Contract-C-2121'),
      ('with_contract_ins101', 'Contract-INS-101'),
      (
          'with_contract_pcd_example_not_author',
          'Contract-pcd-example-notAuthor',
      ),
      ('with_contract_pcd_example_not_labs', 'Contract-pcd-example-notLabs'),
      ('with_contract_pcd_example_not_org', 'Contract-pcd-example-notOrg'),
      ('with_contract_pcd_example_not_them', 'Contract-pcd-example-notThem'),
      ('with_contract_pcd_example_not_this', 'Contract-pcd-example-notThis'),
  )
  def test_json_format_for_valid_contract_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   contract_pb2.Contract)

  @parameterized.named_parameters(
      ('with_coverage7546d', 'Coverage-7546D'),
      ('with_coverage7547e', 'Coverage-7547E'),
      ('with_coverage9876b1', 'Coverage-9876B1'),
      ('with_coverage_sp1234', 'Coverage-SP1234'),
  )
  def test_json_format_for_valid_coverage_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   coverage_pb2.Coverage)

  @parameterized.named_parameters(
      (
          'with_coverage_eligibility_request52345',
          'CoverageEligibilityRequest-52345',
      ),
      (
          'with_coverage_eligibility_request52346',
          'CoverageEligibilityRequest-52346',
      ),
  )
  def test_json_format_for_valid_coverage_eligibility_request_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, coverage_eligibility_request_pb2.CoverageEligibilityRequest)

  @parameterized.named_parameters(
      (
          'with_coverage_eligibility_response_e2500',
          'CoverageEligibilityResponse-E2500',
      ),
      (
          'with_coverage_eligibility_response_e2501',
          'CoverageEligibilityResponse-E2501',
      ),
      (
          'with_coverage_eligibility_response_e2502',
          'CoverageEligibilityResponse-E2502',
      ),
      (
          'with_coverage_eligibility_response_e2503',
          'CoverageEligibilityResponse-E2503',
      ),
  )
  def test_json_format_for_valid_coverage_eligibility_response_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name,
        coverage_eligibility_response_pb2.CoverageEligibilityResponse)

  @parameterized.named_parameters(
      ('with_detected_issue_allergy', 'DetectedIssue-allergy'),
      ('with_detected_issue_ddi', 'DetectedIssue-ddi'),
      ('with_detected_issue_duplicate', 'DetectedIssue-duplicate'),
      ('with_detected_issue_lab', 'DetectedIssue-lab'),
  )
  def test_json_format_for_valid_detected_issue_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, detected_issue_pb2.DetectedIssue)

  @parameterized.named_parameters(
      ('with_device_example', 'Device-example'),
      ('with_device_f001', 'Device-f001'),
  )
  def test_json_format_for_valid_device_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, device_pb2.Device)

  @parameterized.named_parameters(
      ('with_device_definition_example', 'DeviceDefinition-example'),
  )
  def test_json_format_for_valid_device_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, device_definition_pb2.DeviceDefinition)

  @parameterized.named_parameters(
      ('with_device_metric_example', 'DeviceMetric-example'),
  )
  def test_json_format_for_valid_device_metric_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, device_metric_pb2.DeviceMetric)

  @parameterized.named_parameters(
      ('with_device_request_example', 'DeviceRequest-example'),
      ('with_device_request_insulin_pump', 'DeviceRequest-insulinpump'),
      ('with_device_request_left_lens', 'DeviceRequest-left-lens'),
      ('with_device_request_right_lens', 'DeviceRequest-right-lens'),
  )
  def test_json_format_for_valid_device_request_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, device_request_pb2.DeviceRequest)

  @parameterized.named_parameters(
      ('with_device_use_statement_example', 'DeviceUseStatement-example'),
  )
  def test_json_format_for_valid_device_use_statement_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, device_use_statement_pb2.DeviceUseStatement)

  @parameterized.named_parameters(
      ('with_diagnostic_report102', 'DiagnosticReport-102'),
      ('with_diagnostic_report_example_pgx', 'DiagnosticReport-example-pgx'),
      ('with_diagnostic_report_f201', 'DiagnosticReport-f201'),
      # TODO(b/185511968): Disabling due to flaky failures.
      # ('with_diagnostic_report_gingival_mass',
      # 'DiagnosticReport-gingival-mass'),
      ('with_diagnostic_report_pap', 'DiagnosticReport-pap'),
      ('with_diagnostic_report_ultrasound', 'DiagnosticReport-ultrasound'),
  )
  def test_json_format_for_valid_diagnostic_report_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, diagnostic_report_pb2.DiagnosticReport)

  @parameterized.named_parameters(
      ('with_document_manifest654789', 'DocumentManifest-654789'),
      ('with_document_manifest_example', 'DocumentManifest-example'),
  )
  def test_json_format_for_valid_document_manifest_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, document_manifest_pb2.DocumentManifest)

  @parameterized.named_parameters(
      ('with_document_reference_example', 'DocumentReference-example'),
  )
  def test_json_format_for_valid_document_reference_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, document_reference_pb2.DocumentReference)

  @parameterized.named_parameters(
      (
          'with_effect_evidence_synthesis_example',
          'EffectEvidenceSynthesis-example',
      ),
  )
  def test_json_format_for_valid_effect_evidence_synthesis_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, effect_evidence_synthesis_pb2.EffectEvidenceSynthesis)

  @parameterized.named_parameters(
      (
          'with_parameters_empty_resource',
          'Parameters-empty-resource',
          parameters_pb2.Parameters,
      ),
  )
  def test_json_format_for_valid_empty_nested_resource_succeeds(
      self, file_name: str, proto_cls: Type[message.Message]
  ):
    self.assert_parse_and_print_examples_equals_golden(file_name, proto_cls)

  @parameterized.named_parameters(
      ('with_encounter_emerg', 'Encounter-emerg'),
      ('with_encounter_example', 'Encounter-example'),
      ('with_encounter_f001', 'Encounter-f001'),
      ('with_encounter_f002', 'Encounter-f002'),
      ('with_encounter_f003', 'Encounter-f003'),
      ('with_encounter_f201', 'Encounter-f201'),
      ('with_encounter_f202', 'Encounter-f202'),
      ('with_encounter_f203', 'Encounter-f203'),
      ('with_encounter_home', 'Encounter-home'),
      ('with_encounter_xcda', 'Encounter-xcda'),
  )
  def test_json_format_for_valid_encounter_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   encounter_pb2.Encounter)

  @parameterized.named_parameters(
      ('with_endpoint_direct_endpoint', 'Endpoint-direct-endpoint'),
      ('with_endpoint_example_iid', 'Endpoint-example-iid'),
      ('with_endpoint_example', 'Endpoint-example'),
      ('with_endpoint_example_wadors', 'Endpoint-example-wadors'),
  )
  def test_json_format_for_valid_endpoint_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   endpoint_pb2.Endpoint)

  @parameterized.named_parameters(
      ('with_enrollment_request22345', 'EnrollmentRequest-22345'),
  )
  def test_json_format_for_valid_enrollment_request_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, enrollment_request_pb2.EnrollmentRequest)

  @parameterized.named_parameters(
      ('with_enrollment_response_er2500', 'EnrollmentResponse-ER2500'),
  )
  def test_json_format_for_valid_enrollment_response_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, enrollment_response_pb2.EnrollmentResponse)

  @parameterized.named_parameters(
      ('with_episode_of_care_example', 'EpisodeOfCare-example'),
  )
  def test_json_format_for_valid_episode_of_care_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, episode_of_care_pb2.EpisodeOfCare)

  @parameterized.named_parameters(
      ('with_event_definition_example', 'EventDefinition-example'),
  )
  def test_json_format_for_valid_event_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, event_definition_pb2.EventDefinition)

  @parameterized.named_parameters(
      ('with_evidence_example', 'Evidence-example'),
  )
  def test_json_format_for_valid_evidence_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   evidence_pb2.Evidence)

  @parameterized.named_parameters(
      ('with_evidence_variable_example', 'EvidenceVariable-example'),
  )
  def test_json_format_for_valid_evidence_variable_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, evidence_variable_pb2.EvidenceVariable)

  @parameterized.named_parameters(
      ('with_example_scenario_example', 'ExampleScenario-example'),
  )
  def test_json_format_for_valid_example_scenario_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, example_scenario_pb2.ExampleScenario)

  @parameterized.named_parameters(
      ('with_explanation_of_benefit_eb3500', 'ExplanationOfBenefit-EB3500'),
      ('with_explanation_of_benefit_eb3501', 'ExplanationOfBenefit-EB3501'),
  )
  def test_json_format_for_valid_explanation_of_benefit_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, explanation_of_benefit_pb2.ExplanationOfBenefit)

  @parameterized.named_parameters(
      ('with_family_member_history_father', 'FamilyMemberHistory-father'),
      ('with_family_member_history_mother', 'FamilyMemberHistory-mother'),
  )
  def test_json_format_for_valid_family_member_history_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, family_member_history_pb2.FamilyMemberHistory)

  @parameterized.named_parameters(
      ('with_flag_example_encounter', 'Flag-example-encounter'),
      ('with_flag_example', 'Flag-example'),
  )
  def test_json_format_for_valid_flag_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, flag_pb2.Flag)

  @parameterized.named_parameters(
      ('with_goal_example', 'Goal-example'),
      ('with_goal_stop_smoking', 'Goal-stop-smoking'),
  )
  def test_json_format_for_valid_goal_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, goal_pb2.Goal)

  @parameterized.named_parameters(
      ('with_graph_definition_example', 'GraphDefinition-example'),
  )
  def test_json_format_for_valid_graph_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, graph_definition_pb2.GraphDefinition)

  @parameterized.named_parameters(
      ('with_group101', 'Group-101'),
      ('with_group102', 'Group-102'),
      ('with_group_example_patientlist', 'Group-example-patientlist'),
      ('with_group_herd1', 'Group-herd1'),
  )
  def test_json_format_for_valid_group_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, group_pb2.Group)

  @parameterized.named_parameters(
      ('with_guidance_response_example', 'GuidanceResponse-example'),
  )
  def test_json_format_for_valid_guidance_response_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, guidance_response_pb2.GuidanceResponse)

  @parameterized.named_parameters(
      ('with_healthcare_service_example', 'HealthcareService-example'),
  )
  def test_json_format_for_valid_healthcare_service_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, healthcare_service_pb2.HealthcareService)

  @parameterized.named_parameters(
      ('with_imaging_study_example', 'ImagingStudy-example'),
      ('with_imaging_study_example_xr', 'ImagingStudy-example-xr'),
  )
  def test_json_format_for_valid_imaging_study_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, imaging_study_pb2.ImagingStudy)

  @parameterized.named_parameters(
      ('with_immunization_example', 'Immunization-example'),
      ('with_immunization_historical', 'Immunization-historical'),
      ('with_immunization_not_given', 'Immunization-notGiven'),
      ('with_immunization_protocol', 'Immunization-protocol'),
      ('with_immunization_subpotent', 'Immunization-subpotent'),
  )
  def test_json_format_for_valid_immunization_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, immunization_pb2.Immunization)

  @parameterized.named_parameters(
      (
          'with_immunization_evaluation_example',
          'ImmunizationEvaluation-example',
      ),
      (
          'with_immunization_evaluation_not_valid',
          'ImmunizationEvaluation-notValid',
      ),
  )
  def test_json_format_for_valid_immunization_evaluation_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, immunization_evaluation_pb2.ImmunizationEvaluation)

  @parameterized.named_parameters(
      (
          'with_immunization_recommendation_example',
          'ImmunizationRecommendation-example',
      ),
  )
  def test_json_format_for_valid_immunization_recommendation_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, immunization_recommendation_pb2.ImmunizationRecommendation)

  @parameterized.named_parameters(
      # 'ImplementationGuide-fhir' and 'ig-r4' do not parse because they contain
      # a reference to an invalid resource.
      # https://gforge.hl7.org/gf/project/fhir/tracker/?action=TrackerItemEdit&tracker_item_id=22489
      # ('with_implementation_guide_fhir', 'ImplementationGuide-fhir'),
      # ('with_ig_r4', 'ig-r4'),
      ('with_implementation_guide_example', 'ImplementationGuide-example'),
  )
  def test_json_format_for_valid_implementation_guide_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, implementation_guide_pb2.ImplementationGuide)

  @parameterized.named_parameters(
      ('with_insurance_plan_example', 'InsurancePlan-example'),
  )
  def test_json_format_for_valid_insurance_plan_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, insurance_plan_pb2.InsurancePlan)

  @parameterized.named_parameters(
      ('with_invoice_example', 'Invoice-example'),
  )
  def test_json_format_for_valid_invoice_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   invoice_pb2.Invoice)

  @parameterized.named_parameters(
      ('with_library_composition_example', 'Library-composition-example'),
      ('with_library_example', 'Library-example'),
      ('with_library_hiv_indicators', 'Library-hiv-indicators'),
      ('with_library_library_cms146_example', 'Library-library-cms146-example'),
      (
          'with_library_library_exclusive_breastfeeding_cds_logic',
          'Library-library-exclusive-breastfeeding-cds-logic',
      ),
      (
          'with_library_library_exclusive_breastfeeding_cqm_logic',
          'Library-library-exclusive-breastfeeding-cqm-logic',
      ),
      ('with_library_library_fhir_helpers', 'Library-library-fhir-helpers'),
      (
          'with_library_library_fhir_helpers_predecessor',
          'Library-library-fhir-helpers-predecessor',
      ),
      (
          'with_library_library_fhir_model_definition',
          'Library-library-fhir-model-definition',
      ),
      (
          'with_library_library_quick_model_definition',
          'Library-library-quick-model-definition',
      ),
      ('with_library_omtk_logic', 'Library-omtk-logic'),
      ('with_library_omtk_modelinfo', 'Library-omtk-modelinfo'),
      ('with_library_opioidcds_common', 'Library-opioidcds-common'),
      (
          'with_library_opioidcds_recommendation04',
          'Library-opioidcds-recommendation-04',
      ),
      (
          'with_library_opioidcds_recommendation05',
          'Library-opioidcds-recommendation-05',
      ),
      (
          'with_library_opioidcds_recommendation07',
          'Library-opioidcds-recommendation-07',
      ),
      (
          'with_library_opioidcds_recommendation08',
          'Library-opioidcds-recommendation-08',
      ),
      (
          'with_library_opioidcds_recommendation10',
          'Library-opioidcds-recommendation-10',
      ),
      (
          'with_library_opioidcds_recommendation11',
          'Library-opioidcds-recommendation-11',
      ),
      (
          'with_library_suiciderisk_orderset_logic',
          'Library-suiciderisk-orderset-logic',
      ),
      (
          'with_library_zika_virus_intervention_logic',
          'Library-zika-virus-intervention-logic',
      ),
  )
  def test_json_format_for_valid_library_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   library_pb2.Library)

  @parameterized.named_parameters(
      ('with_linkage_example', 'Linkage-example'),
  )
  def test_json_format_for_valid_linkage_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   linkage_pb2.Linkage)

  @parameterized.named_parameters(
      ('with_list_current_allergies', 'List-current-allergies'),
      (
          'with_list_example_double_cousin_relationship',
          'List-example-double-cousin-relationship',
      ),
      ('with_list_example_empty', 'List-example-empty'),
      ('with_list_example', 'List-example'),
      ('with_list_example_simple_empty', 'List-example-simple-empty'),
      ('with_list_f201', 'List-f201'),
      ('with_list_genetic', 'List-genetic'),
      ('with_list_long', 'List-long'),
      ('with_list_med_list', 'List-med-list'),
      ('with_list_prognosis', 'List-prognosis'),
  )
  def test_json_format_for_valid_list_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, list_pb2.List)

  @parameterized.named_parameters(
      ('with_location1', 'Location-1'),
      ('with_location2', 'Location-2'),
      ('with_location_amb', 'Location-amb'),
      ('with_location_hl7', 'Location-hl7'),
      ('with_location_ph', 'Location-ph'),
      ('with_location_ukp', 'Location-ukp'),
  )
  def test_json_format_for_valid_location_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   location_pb2.Location)

  @parameterized.named_parameters(
      ('with_measure_component_a_example', 'Measure-component-a-example'),
      ('with_measure_component_b_example', 'Measure-component-b-example'),
      ('with_measure_composite_example', 'Measure-composite-example'),
      ('with_measure_hiv_indicators', 'Measure-hiv-indicators'),
      ('with_measure_measure_cms146_example', 'Measure-measure-cms146-example'),
      (
          'with_measure_measure_exclusive_breastfeeding',
          'Measure-measure-exclusive-breastfeeding',
      ),
      (
          'with_measure_measure_predecessor_example',
          'Measure-measure-predecessor-example',
      ),
  )
  def test_json_format_for_valid_measure_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   measure_pb2.Measure)

  @parameterized.named_parameters(
      ('with_measure_report_hiv_indicators', 'MeasureReport-hiv-indicators'),
      (
          'with_measure_report_measure_report_cms146_cat1_example',
          'MeasureReport-measurereport-cms146-cat1-example',
      ),
      (
          'with_measure_report_measure_report_cms146_cat2_example',
          'MeasureReport-measurereport-cms146-cat2-example',
      ),
      (
          'with_measure_report_measure_report_cms146_cat3_example',
          'MeasureReport-measurereport-cms146-cat3-example',
      ),
  )
  def test_json_format_for_valid_measure_report_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, measure_report_pb2.MeasureReport)

  @parameterized.named_parameters(
      (
          'with_media1_2_840_11361907579238403408700_3_1_04_19970327150033',
          'Media-1.2.840.11361907579238403408700.3.1.04.19970327150033',
      ),
      ('with_media_example', 'Media-example'),
      ('with_media_sound', 'Media-sound'),
      ('with_media_xray', 'Media-xray'),
  )
  def test_json_format_for_valid_media_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, media_pb2.Media)

  @parameterized.named_parameters(
      ('with_medication_med0301', 'Medication-med0301'),
      ('with_medication_med0302', 'Medication-med0302'),
      ('with_medication_med0303', 'Medication-med0303'),
      ('with_medication_med0304', 'Medication-med0304'),
      ('with_medication_med0305', 'Medication-med0305'),
      ('with_medication_med0306', 'Medication-med0306'),
      ('with_medication_med0307', 'Medication-med0307'),
      ('with_medication_med0308', 'Medication-med0308'),
      ('with_medication_med0309', 'Medication-med0309'),
      ('with_medication_med0310', 'Medication-med0310'),
      ('with_medication_med0311', 'Medication-med0311'),
      ('with_medication_med0312', 'Medication-med0312'),
      ('with_medication_med0313', 'Medication-med0313'),
      ('with_medication_med0314', 'Medication-med0314'),
      ('with_medication_med0315', 'Medication-med0315'),
      ('with_medication_med0316', 'Medication-med0316'),
      ('with_medication_med0317', 'Medication-med0317'),
      ('with_medication_med0318', 'Medication-med0318'),
      ('with_medication_med0319', 'Medication-med0319'),
      ('with_medication_med0320', 'Medication-med0320'),
      ('with_medication_med0321', 'Medication-med0321'),
      ('with_medication_medexample015', 'Medication-medexample015'),
      ('with_medication_medicationexample1', 'Medication-medicationexample1'),
  )
  def test_json_format_for_valid_medication_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   medication_pb2.Medication)

  @parameterized.named_parameters(
      (
          'with_medication_administration_medadmin0301',
          'MedicationAdministration-medadmin0301',
      ),
      (
          'with_medication_administration_medadmin0302',
          'MedicationAdministration-medadmin0302',
      ),
      (
          'with_medication_administration_medadmin0303',
          'MedicationAdministration-medadmin0303',
      ),
      (
          'with_medication_administration_medadmin0304',
          'MedicationAdministration-medadmin0304',
      ),
      (
          'with_medication_administration_medadmin0305',
          'MedicationAdministration-medadmin0305',
      ),
      (
          'with_medication_administration_medadmin0306',
          'MedicationAdministration-medadmin0306',
      ),
      (
          'with_medication_administration_medadmin0307',
          'MedicationAdministration-medadmin0307',
      ),
      (
          'with_medication_administration_medadmin0308',
          'MedicationAdministration-medadmin0308',
      ),
      (
          'with_medication_administration_medadmin0309',
          'MedicationAdministration-medadmin0309',
      ),
      (
          'with_medication_administration_medadmin0310',
          'MedicationAdministration-medadmin0310',
      ),
      (
          'with_medication_administration_medadmin0311',
          'MedicationAdministration-medadmin0311',
      ),
      (
          'with_medication_administration_medadmin0312',
          'MedicationAdministration-medadmin0312',
      ),
      (
          'with_medication_administration_medadmin0313',
          'MedicationAdministration-medadmin0313',
      ),
      (
          'with_medication_administration_medadminexample03',
          'MedicationAdministration-medadminexample03',
      ),
  )
  def test_json_format_for_valid_medication_administration_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medication_administration_pb2.MedicationAdministration)

  @parameterized.named_parameters(
      ('with_medication_dispense_meddisp008', 'MedicationDispense-meddisp008'),
      (
          'with_medication_dispense_meddisp0301',
          'MedicationDispense-meddisp0301',
      ),
      (
          'with_medication_dispense_meddisp0302',
          'MedicationDispense-meddisp0302',
      ),
      (
          'with_medication_dispense_meddisp0303',
          'MedicationDispense-meddisp0303',
      ),
      (
          'with_medication_dispense_meddisp0304',
          'MedicationDispense-meddisp0304',
      ),
      (
          'with_medication_dispense_meddisp0305',
          'MedicationDispense-meddisp0305',
      ),
      (
          'with_medication_dispense_meddisp0306',
          'MedicationDispense-meddisp0306',
      ),
      (
          'with_medication_dispense_meddisp0307',
          'MedicationDispense-meddisp0307',
      ),
      (
          'with_medication_dispense_meddisp0308',
          'MedicationDispense-meddisp0308',
      ),
      (
          'with_medication_dispense_meddisp0309',
          'MedicationDispense-meddisp0309',
      ),
      (
          'with_medication_dispense_meddisp0310',
          'MedicationDispense-meddisp0310',
      ),
      (
          'with_medication_dispense_meddisp0311',
          'MedicationDispense-meddisp0311',
      ),
      (
          'with_medication_dispense_meddisp0312',
          'MedicationDispense-meddisp0312',
      ),
      (
          'with_medication_dispense_meddisp0313',
          'MedicationDispense-meddisp0313',
      ),
      (
          'with_medication_dispense_meddisp0314',
          'MedicationDispense-meddisp0314',
      ),
      (
          'with_medication_dispense_meddisp0315',
          'MedicationDispense-meddisp0315',
      ),
      (
          'with_medication_dispense_meddisp0316',
          'MedicationDispense-meddisp0316',
      ),
      (
          'with_medication_dispense_meddisp0317',
          'MedicationDispense-meddisp0317',
      ),
      (
          'with_medication_dispense_meddisp0318',
          'MedicationDispense-meddisp0318',
      ),
      (
          'with_medication_dispense_meddisp0319',
          'MedicationDispense-meddisp0319',
      ),
      (
          'with_medication_dispense_meddisp0320',
          'MedicationDispense-meddisp0320',
      ),
      (
          'with_medication_dispense_meddisp0321',
          'MedicationDispense-meddisp0321',
      ),
      (
          'with_medication_dispense_meddisp0322',
          'MedicationDispense-meddisp0322',
      ),
      (
          'with_medication_dispense_meddisp0324',
          'MedicationDispense-meddisp0324',
      ),
      (
          'with_medication_dispense_meddisp0325',
          'MedicationDispense-meddisp0325',
      ),
      (
          'with_medication_dispense_meddisp0326',
          'MedicationDispense-meddisp0326',
      ),
      (
          'with_medication_dispense_meddisp0327',
          'MedicationDispense-meddisp0327',
      ),
      (
          'with_medication_dispense_meddisp0328',
          'MedicationDispense-meddisp0328',
      ),
      (
          'with_medication_dispense_meddisp0329',
          'MedicationDispense-meddisp0329',
      ),
      (
          'with_medication_dispense_meddisp0330',
          'MedicationDispense-meddisp0330',
      ),
      (
          'with_medication_dispense_meddisp0331',
          'MedicationDispense-meddisp0331',
      ),
  )
  def test_json_format_for_valid_medication_dispense_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medication_dispense_pb2.MedicationDispense)

  @parameterized.named_parameters(
      ('with_medication_knowledge_example', 'MedicationKnowledge-example'),
  )
  def test_json_format_for_valid_medication_knowledge_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medication_knowledge_pb2.MedicationKnowledge)

  @parameterized.named_parameters(
      ('with_medication_request_medrx002', 'MedicationRequest-medrx002'),
      ('with_medication_request_medrx0301', 'MedicationRequest-medrx0301'),
      ('with_medication_request_medrx0302', 'MedicationRequest-medrx0302'),
      ('with_medication_request_medrx0303', 'MedicationRequest-medrx0303'),
      ('with_medication_request_medrx0304', 'MedicationRequest-medrx0304'),
      ('with_medication_request_medrx0305', 'MedicationRequest-medrx0305'),
      ('with_medication_request_medrx0306', 'MedicationRequest-medrx0306'),
      ('with_medication_request_medrx0307', 'MedicationRequest-medrx0307'),
      ('with_medication_request_medrx0308', 'MedicationRequest-medrx0308'),
      ('with_medication_request_medrx0309', 'MedicationRequest-medrx0309'),
      ('with_medication_request_medrx0310', 'MedicationRequest-medrx0310'),
      ('with_medication_request_medrx0311', 'MedicationRequest-medrx0311'),
      ('with_medication_request_medrx0312', 'MedicationRequest-medrx0312'),
      ('with_medication_request_medrx0313', 'MedicationRequest-medrx0313'),
      ('with_medication_request_medrx0314', 'MedicationRequest-medrx0314'),
      ('with_medication_request_medrx0315', 'MedicationRequest-medrx0315'),
      ('with_medication_request_medrx0316', 'MedicationRequest-medrx0316'),
      ('with_medication_request_medrx0317', 'MedicationRequest-medrx0317'),
      ('with_medication_request_medrx0318', 'MedicationRequest-medrx0318'),
      ('with_medication_request_medrx0319', 'MedicationRequest-medrx0319'),
      ('with_medication_request_medrx0320', 'MedicationRequest-medrx0320'),
      ('with_medication_request_medrx0321', 'MedicationRequest-medrx0321'),
      ('with_medication_request_medrx0322', 'MedicationRequest-medrx0322'),
      ('with_medication_request_medrx0323', 'MedicationRequest-medrx0323'),
      ('with_medication_request_medrx0324', 'MedicationRequest-medrx0324'),
      ('with_medication_request_medrx0325', 'MedicationRequest-medrx0325'),
      ('with_medication_request_medrx0326', 'MedicationRequest-medrx0326'),
      ('with_medication_request_medrx0327', 'MedicationRequest-medrx0327'),
      ('with_medication_request_medrx0328', 'MedicationRequest-medrx0328'),
      ('with_medication_request_medrx0329', 'MedicationRequest-medrx0329'),
      ('with_medication_request_medrx0330', 'MedicationRequest-medrx0330'),
      ('with_medication_request_medrx0331', 'MedicationRequest-medrx0331'),
      ('with_medication_request_medrx0332', 'MedicationRequest-medrx0332'),
      ('with_medication_request_medrx0333', 'MedicationRequest-medrx0333'),
      ('with_medication_request_medrx0334', 'MedicationRequest-medrx0334'),
      ('with_medication_request_medrx0335', 'MedicationRequest-medrx0335'),
      ('with_medication_request_medrx0336', 'MedicationRequest-medrx0336'),
      ('with_medication_request_medrx0337', 'MedicationRequest-medrx0337'),
      ('with_medication_request_medrx0338', 'MedicationRequest-medrx0338'),
      ('with_medication_request_medrx0339', 'MedicationRequest-medrx0339'),
  )
  def test_json_format_for_valid_medication_request_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medication_request_pb2.MedicationRequest)

  @parameterized.named_parameters(
      (
          'with_medication_statement_example001',
          'MedicationStatement-example001',
      ),
      (
          'with_medication_statement_example002',
          'MedicationStatement-example002',
      ),
      (
          'with_medication_statement_example003',
          'MedicationStatement-example003',
      ),
      (
          'with_medication_statement_example004',
          'MedicationStatement-example004',
      ),
      (
          'with_medication_statement_example005',
          'MedicationStatement-example005',
      ),
      (
          'with_medication_statement_example006',
          'MedicationStatement-example006',
      ),
      (
          'with_medication_statement_example007',
          'MedicationStatement-example007',
      ),
  )
  def test_json_format_for_valid_medication_statement_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medication_statement_pb2.MedicationStatement)

  @parameterized.named_parameters(
      ('with_medicinal_product_example', 'MedicinalProduct-example'),
  )
  def test_json_format_for_valid_medicinal_product_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medicinal_product_pb2.MedicinalProduct)

  @parameterized.named_parameters(
      (
          'with_medicinal_product_authorization_example',
          'MedicinalProductAuthorization-example',
      ),
  )
  def test_json_format_for_valid_medicinal_product_authorization_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name,
        medicinal_product_authorization_pb2.MedicinalProductAuthorization)

  @parameterized.named_parameters(
      (
          'with_medicinal_product_contraindication_example',
          'MedicinalProductContraindication-example',
      ),
  )
  def test_json_format_for_valid_medicinal_product_contraindication_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name,
        medicinal_product_contraindication_pb2.MedicinalProductContraindication)

  @parameterized.named_parameters(
      (
          'with_medicinal_product_indication_example',
          'MedicinalProductIndication-example',
      ),
  )
  def test_json_format_for_valid_medicinal_product_indication_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medicinal_product_indication_pb2.MedicinalProductIndication)

  @parameterized.named_parameters(
      (
          'with_medicinal_product_ingredient_example',
          'MedicinalProductIngredient-example',
      ),
  )
  def test_json_format_for_valid_medicinal_product_ingredient_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medicinal_product_ingredient_pb2.MedicinalProductIngredient)

  @parameterized.named_parameters(
      (
          'with_medicinal_product_interaction_example',
          'MedicinalProductInteraction-example',
      ),
  )
  def test_json_format_for_valid_medicinal_product_interaction_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name,
        medicinal_product_interaction_pb2.MedicinalProductInteraction)

  @parameterized.named_parameters(
      (
          'with_medicinal_product_manufactured_example',
          'MedicinalProductManufactured-example',
      ),
  )
  def test_json_format_for_valid_medicinal_product_manufactured_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name,
        medicinal_product_manufactured_pb2.MedicinalProductManufactured)

  @parameterized.named_parameters(
      (
          'with_medicinal_product_packaged_example',
          'MedicinalProductPackaged-example',
      ),
  )
  def test_json_format_for_valid_medicinal_product_packaged_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medicinal_product_packaged_pb2.MedicinalProductPackaged)

  @parameterized.named_parameters(
      (
          'with_medicinal_product_pharmaceutical_example',
          'MedicinalProductPharmaceutical-example',
      ),
  )
  def test_json_format_for_valid_medicinal_product_pharmaceutical_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name,
        medicinal_product_pharmaceutical_pb2.MedicinalProductPharmaceutical)

  @parameterized.named_parameters(
      (
          'with_medicinal_product_undesirable_effect_example',
          'MedicinalProductUndesirableEffect-example',
      ),
  )
  def test_json_format_for_valid_medicinal_product_undesirable_effect_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medicinal_product_undesirable_effect_pb2
        .MedicinalProductUndesirableEffect)

  @parameterized.named_parameters(
      ('with_message_definition_example', 'MessageDefinition-example'),
      (
          'with_message_definition_patient_link_notification',
          'MessageDefinition-patient-link-notification',
      ),
      (
          'with_message_definition_patient_link_response',
          'MessageDefinition-patient-link-response',
      ),
  )
  def test_json_format_for_valid_message_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, message_definition_pb2.MessageDefinition)

  @parameterized.named_parameters(
      (
          'with_message_header1cbdfb97585948a48301d54eab818d68',
          'MessageHeader-1cbdfb97-5859-48a4-8301-d54eab818d68',
      ),
  )
  def test_json_format_for_valid_message_header_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, message_header_pb2.MessageHeader)

  @parameterized.named_parameters(
      (
          'with_molecular_sequence_breastcancer',
          'MolecularSequence-breastcancer',
      ),
      ('with_molecular_sequence_coord0_base', 'MolecularSequence-coord-0-base'),
      ('with_molecular_sequence_coord1_base', 'MolecularSequence-coord-1-base'),
      ('with_molecular_sequence_example', 'MolecularSequence-example'),
      (
          'with_molecular_sequence_example_pgx1',
          'MolecularSequence-example-pgx-1',
      ),
      (
          'with_molecular_sequence_example_pgx2',
          'MolecularSequence-example-pgx-2',
      ),
      (
          'with_molecular_sequence_example_tpmt_one',
          'MolecularSequence-example-TPMT-one',
      ),
      (
          'with_molecular_sequence_example_tpmt_two',
          'MolecularSequence-example-TPMT-two',
      ),
      ('with_molecular_sequence_fda_example', 'MolecularSequence-fda-example'),
      (
          'with_molecular_sequence_fda_vcf_comparison',
          'MolecularSequence-fda-vcf-comparison',
      ),
      (
          'with_molecular_sequence_fda_vcfeval_comparison',
          'MolecularSequence-fda-vcfeval-comparison',
      ),
      (
          'with_molecular_sequence_graphic_example1',
          'MolecularSequence-graphic-example-1',
      ),
      (
          'with_molecular_sequence_graphic_example2',
          'MolecularSequence-graphic-example-2',
      ),
      (
          'with_molecular_sequence_graphic_example3',
          'MolecularSequence-graphic-example-3',
      ),
      (
          'with_molecular_sequence_graphic_example4',
          'MolecularSequence-graphic-example-4',
      ),
      (
          'with_molecular_sequence_graphic_example5',
          'MolecularSequence-graphic-example-5',
      ),
      (
          'with_molecular_sequence_sequence_complex_variant',
          'MolecularSequence-sequence-complex-variant',
      ),
  )
  def test_json_format_for_valid_molecular_sequence_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, molecular_sequence_pb2.MolecularSequence)

  @parameterized.named_parameters(
      ('with_naming_system_example_id', 'NamingSystem-example-id'),
      ('with_naming_system_example', 'NamingSystem-example'),
  )
  def test_json_format_for_valid_naming_system_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, naming_system_pb2.NamingSystem)

  @parameterized.named_parameters(
      ('with_nutrition_order_cardiacdiet', 'NutritionOrder-cardiacdiet'),
      ('with_nutrition_order_diabeticdiet', 'NutritionOrder-diabeticdiet'),
      (
          'with_nutrition_order_diabetic_supplement',
          'NutritionOrder-diabeticsupplement',
      ),
      (
          'with_nutrition_order_energy_supplement',
          'NutritionOrder-energysupplement',
      ),
      ('with_nutrition_order_enteralbolus', 'NutritionOrder-enteralbolus'),
      (
          'with_nutrition_order_enteralcontinuous',
          'NutritionOrder-enteralcontinuous',
      ),
      (
          'with_nutrition_order_fiberrestricteddiet',
          'NutritionOrder-fiberrestricteddiet',
      ),
      ('with_nutrition_order_infant_enteral', 'NutritionOrder-infantenteral'),
      (
          'with_nutrition_order_protein_supplement',
          'NutritionOrder-proteinsupplement',
      ),
      ('with_nutrition_order_pureed_diet', 'NutritionOrder-pureeddiet'),
      (
          'with_nutrition_order_pureed_diet_simple',
          'NutritionOrder-pureeddiet-simple',
      ),
      ('with_nutrition_order_renal_diet', 'NutritionOrder-renaldiet'),
      (
          'with_nutrition_order_texture_modified',
          'NutritionOrder-texturemodified',
      ),
  )
  def test_json_format_for_valid_nutrition_order_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, nutrition_order_pb2.NutritionOrder)

  @parameterized.named_parameters(
      (
          'with_observation10_minute_apgar_score',
          'Observation-10minute-apgar-score',
      ),
      (
          'with_observation1_minute_apgar_score',
          'Observation-1minute-apgar-score',
      ),
      (
          'with_observation20_minute_apgar_score',
          'Observation-20minute-apgar-score',
      ),
      (
          'with_observation2_minute_apgar_score',
          'Observation-2minute-apgar-score',
      ),
      (
          'with_observation5_minute_apgar_score',
          'Observation-5minute-apgar-score',
      ),
      ('with_observation656', 'Observation-656'),
      ('with_observation_abdo_tender', 'Observation-abdo-tender'),
      ('with_observation_alcohol_type', 'Observation-alcohol-type'),
      ('with_observation_bgpanel', 'Observation-bgpanel'),
      ('with_observation_bloodgroup', 'Observation-bloodgroup'),
      (
          'with_observation_blood_pressure_cancel',
          'Observation-blood-pressure-cancel',
      ),
      ('with_observation_blood_pressure_dar', 'Observation-blood-pressure-dar'),
      ('with_observation_blood_pressure', 'Observation-blood-pressure'),
      ('with_observation_bmd', 'Observation-bmd'),
      ('with_observation_bmi', 'Observation-bmi'),
      ('with_observation_bmi_using_related', 'Observation-bmi-using-related'),
      ('with_observation_body_height', 'Observation-body-height'),
      ('with_observation_body_length', 'Observation-body-length'),
      ('with_observation_body_temperature', 'Observation-body-temperature'),
      ('with_observation_clinical_gender', 'Observation-clinical-gender'),
      ('with_observation_date_lastmp', 'Observation-date-lastmp'),
      ('with_observation_decimal', 'Observation-decimal'),
      ('with_observation_ekg', 'Observation-ekg'),
      ('with_observation_example_diplotype1', 'Observation-example-diplotype1'),
      ('with_observation_example_genetics1', 'Observation-example-genetics-1'),
      ('with_observation_example_genetics2', 'Observation-example-genetics-2'),
      ('with_observation_example_genetics3', 'Observation-example-genetics-3'),
      ('with_observation_example_genetics4', 'Observation-example-genetics-4'),
      ('with_observation_example_genetics5', 'Observation-example-genetics-5'),
      (
          'with_observation_example_genetics_brcapat',
          'Observation-example-genetics-brcapat',
      ),
      ('with_observation_example_haplotype1', 'Observation-example-haplotype1'),
      ('with_observation_example_haplotype2', 'Observation-example-haplotype2'),
      ('with_observation_example', 'Observation-example'),
      ('with_observation_example_phenotype', 'Observation-example-phenotype'),
      (
          'with_observation_example_tpmt_diplotype',
          'Observation-example-TPMT-diplotype',
      ),
      (
          'with_observation_example_tpmt_haplotype_one',
          'Observation-example-TPMT-haplotype-one',
      ),
      (
          'with_observation_example_tpmt_haplotype_two',
          'Observation-example-TPMT-haplotype-two',
      ),
      ('with_observation_eye_color', 'Observation-eye-color'),
      ('with_observation_f001', 'Observation-f001'),
      ('with_observation_f002', 'Observation-f002'),
      ('with_observation_f003', 'Observation-f003'),
      ('with_observation_f004', 'Observation-f004'),
      ('with_observation_f005', 'Observation-f005'),
      ('with_observation_f202', 'Observation-f202'),
      ('with_observation_f203', 'Observation-f203'),
      ('with_observation_f204', 'Observation-f204'),
      ('with_observation_f205', 'Observation-f205'),
      ('with_observation_f206', 'Observation-f206'),
      ('with_observation_gcs_qa', 'Observation-gcs-qa'),
      ('with_observation_glasgow', 'Observation-glasgow'),
      ('with_observation_head_circumference', 'Observation-head-circumference'),
      ('with_observation_heart_rate', 'Observation-heart-rate'),
      ('with_observation_herd1', 'Observation-herd1'),
      ('with_observation_map_sitting', 'Observation-map-sitting'),
      ('with_observation_mbp', 'Observation-mbp'),
      ('with_observation_respiratory_rate', 'Observation-respiratory-rate'),
      ('with_observation_rhstatus', 'Observation-rhstatus'),
      ('with_observation_sat_o2', 'Observation-satO2'),
      ('with_observation_second_smoke', 'Observation-secondsmoke'),
      ('with_observation_trach_care', 'Observation-trachcare'),
      ('with_observation_unsat', 'Observation-unsat'),
      ('with_observation_vitals_panel', 'Observation-vitals-panel'),
      ('with_observation_vomiting', 'Observation-vomiting'),
      ('with_observation_vp_oyster', 'Observation-vp-oyster'),
  )
  def test_json_format_for_valid_observation_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   observation_pb2.Observation)

  @parameterized.named_parameters(
      ('with_observation_definition_example', 'ObservationDefinition-example'),
  )
  def test_json_format_for_valid_observation_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, observation_definition_pb2.ObservationDefinition)

  @parameterized.named_parameters(
      (
          'with_operation_definition_activity_definition_apply',
          'OperationDefinition-ActivityDefinition-apply',
      ),
      (
          'with_operation_definition_activity_definition_data_requirements',
          'OperationDefinition-ActivityDefinition-data-requirements',
      ),
      (
          'with_operation_definition_capability_statement_conforms',
          'OperationDefinition-CapabilityStatement-conforms',
      ),
      (
          'with_operation_definition_capability_statement_implements',
          'OperationDefinition-CapabilityStatement-implements',
      ),
      (
          'with_operation_definition_capability_statement_subset',
          'OperationDefinition-CapabilityStatement-subset',
      ),
      (
          'with_operation_definition_capability_statement_versions',
          'OperationDefinition-CapabilityStatement-versions',
      ),
      (
          'with_operation_definition_charge_item_definition_apply',
          'OperationDefinition-ChargeItemDefinition-apply',
      ),
      (
          'with_operation_definition_claim_submit',
          'OperationDefinition-Claim-submit',
      ),
      (
          'with_operation_definition_code_system_find_matches',
          'OperationDefinition-CodeSystem-find-matches',
      ),
      (
          'with_operation_definition_code_system_lookup',
          'OperationDefinition-CodeSystem-lookup',
      ),
      (
          'with_operation_definition_code_system_subsumes',
          'OperationDefinition-CodeSystem-subsumes',
      ),
      (
          'with_operation_definition_code_system_validate_code',
          'OperationDefinition-CodeSystem-validate-code',
      ),
      (
          'with_operation_definition_composition_document',
          'OperationDefinition-Composition-document',
      ),
      (
          'with_operation_definition_concept_map_closure',
          'OperationDefinition-ConceptMap-closure',
      ),
      (
          'with_operation_definition_concept_map_translate',
          'OperationDefinition-ConceptMap-translate',
      ),
      (
          'with_operation_definition_coverage_eligibility_request_submit',
          'OperationDefinition-CoverageEligibilityRequest-submit',
      ),
      (
          'with_operation_definition_encounter_everything',
          'OperationDefinition-Encounter-everything',
      ),
      ('with_operation_definition_example', 'OperationDefinition-example'),
      (
          'with_operation_definition_group_everything',
          'OperationDefinition-Group-everything',
      ),
      (
          'with_operation_definition_library_data_requirements',
          'OperationDefinition-Library-data-requirements',
      ),
      ('with_operation_definition_list_find', 'OperationDefinition-List-find'),
      (
          'with_operation_definition_measure_care_gaps',
          'OperationDefinition-Measure-care-gaps',
      ),
      (
          'with_operation_definition_measure_collect_data',
          'OperationDefinition-Measure-collect-data',
      ),
      (
          'with_operation_definition_measure_data_requirements',
          'OperationDefinition-Measure-data-requirements',
      ),
      (
          'with_operation_definition_measure_evaluate_measure',
          'OperationDefinition-Measure-evaluate-measure',
      ),
      (
          'with_operation_definition_measure_submit_data',
          'OperationDefinition-Measure-submit-data',
      ),
      (
          'with_operation_definition_medicinal_product_everything',
          'OperationDefinition-MedicinalProduct-everything',
      ),
      (
          'with_operation_definition_message_header_process_message',
          'OperationDefinition-MessageHeader-process-message',
      ),
      (
          'with_operation_definition_naming_system_preferred_id',
          'OperationDefinition-NamingSystem-preferred-id',
      ),
      (
          'with_operation_definition_observation_lastn',
          'OperationDefinition-Observation-lastn',
      ),
      (
          'with_operation_definition_observation_stats',
          'OperationDefinition-Observation-stats',
      ),
      (
          'with_operation_definition_patient_everything',
          'OperationDefinition-Patient-everything',
      ),
      (
          'with_operation_definition_patient_match',
          'OperationDefinition-Patient-match',
      ),
      (
          'with_operation_definition_plan_definition_apply',
          'OperationDefinition-PlanDefinition-apply',
      ),
      (
          'with_operation_definition_plan_definition_data_requirements',
          'OperationDefinition-PlanDefinition-data-requirements',
      ),
      (
          'with_operation_definition_resource_convert',
          'OperationDefinition-Resource-convert',
      ),
      (
          'with_operation_definition_resource_graph',
          'OperationDefinition-Resource-graph',
      ),
      (
          'with_operation_definition_resource_graphql',
          'OperationDefinition-Resource-graphql',
      ),
      (
          'with_operation_definition_resource_meta_add',
          'OperationDefinition-Resource-meta-add',
      ),
      (
          'with_operation_definition_resource_meta_delete',
          'OperationDefinition-Resource-meta-delete',
      ),
      (
          'with_operation_definition_resource_meta',
          'OperationDefinition-Resource-meta',
      ),
      (
          'with_operation_definition_resource_validate',
          'OperationDefinition-Resource-validate',
      ),
      (
          'with_operation_definition_structure_definition_questionnaire',
          'OperationDefinition-StructureDefinition-questionnaire',
      ),
      (
          'with_operation_definition_structure_definition_snapshot',
          'OperationDefinition-StructureDefinition-snapshot',
      ),
      (
          'with_operation_definition_structure_map_transform',
          'OperationDefinition-StructureMap-transform',
      ),
      (
          'with_operation_definition_value_set_expand',
          'OperationDefinition-ValueSet-expand',
      ),
      (
          'with_operation_definition_value_set_validate_code',
          'OperationDefinition-ValueSet-validate-code',
      ),
  )
  def test_json_format_for_valid_operation_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, operation_definition_pb2.OperationDefinition)

  @parameterized.named_parameters(
      ('with_operation_outcome101', 'OperationOutcome-101'),
      ('with_operation_outcome_allok', 'OperationOutcome-allok'),
      (
          'with_operation_outcome_break_the_glass',
          'OperationOutcome-break-the-glass',
      ),
      ('with_operation_outcome_exception', 'OperationOutcome-exception'),
      ('with_operation_outcome_searchfail', 'OperationOutcome-searchfail'),
      (
          'with_operation_outcome_validationfail',
          'OperationOutcome-validationfail',
      ),
  )
  def test_json_format_for_valid_operation_outcome_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, operation_outcome_pb2.OperationOutcome)

  @parameterized.named_parameters(
      (
          'with_organization1832473e2fe0452d_abe93cdb9879522f',
          'Organization-1832473e-2fe0-452d-abe9-3cdb9879522f',
      ),
      ('with_organization1', 'Organization-1'),
      (
          'with_organization2.16.840.1.113883.19.5',
          'Organization-2.16.840.1.113883.19.5',
      ),
      ('with_organization2', 'Organization-2'),
      ('with_organization3', 'Organization-3'),
      ('with_organization_f001', 'Organization-f001'),
      ('with_organization_f002', 'Organization-f002'),
      ('with_organization_f003', 'Organization-f003'),
      ('with_organization_f201', 'Organization-f201'),
      ('with_organization_f203', 'Organization-f203'),
      ('with_organization_hl7', 'Organization-hl7'),
      ('with_organization_hl7_pay', 'Organization-hl7pay'),
      ('with_organization_mmanu', 'Organization-mmanu'),
  )
  def test_json_format_for_valid_organization_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, organization_pb2.Organization)

  @parameterized.named_parameters(
      (
          'with_organization_affiliation_example',
          'OrganizationAffiliation-example',
      ),
      (
          'with_organization_affiliation_orgrole1',
          'OrganizationAffiliation-orgrole1',
      ),
      (
          'with_organization_affiliation_orgrole2',
          'OrganizationAffiliation-orgrole2',
      ),
  )
  def test_json_format_for_valid_organization_affiliation_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, organization_affiliation_pb2.OrganizationAffiliation)

  @parameterized.named_parameters(
      ('with_patient_animal', 'Patient-animal'),
      ('with_patient_ch_example', 'Patient-ch-example'),
      ('with_patient_dicom', 'Patient-dicom'),
      ('with_patient_example', 'Patient-example'),
      ('with_patient_f001', 'Patient-f001'),
      ('with_patient_f201', 'Patient-f201'),
      ('with_patient_genetics_example1', 'Patient-genetics-example1'),
      ('with_patient_glossy', 'Patient-glossy'),
      ('with_patient_ihe_pcd', 'Patient-ihe-pcd'),
      ('with_patient_infant_fetal', 'Patient-infant-fetal'),
      ('with_patient_infant_mom', 'Patient-infant-mom'),
      ('with_patient_infant_twin1', 'Patient-infant-twin-1'),
      ('with_patient_infant_twin2', 'Patient-infant-twin-2'),
      ('with_patient_mom', 'Patient-mom'),
      ('with_patient_newborn', 'Patient-newborn'),
      ('with_patient_pat1', 'Patient-pat1'),
      ('with_patient_pat2', 'Patient-pat2'),
      ('with_patient_pat3', 'Patient-pat3'),
      ('with_patient_pat4', 'Patient-pat4'),
      ('with_patient_proband', 'Patient-proband'),
      ('with_patient_xcda', 'Patient-xcda'),
      ('with_patient_xds', 'Patient-xds'),
  )
  def test_json_format_for_valid_patient_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   patient_pb2.Patient)

  @parameterized.named_parameters(
      ('with_payment_notice77654', 'PaymentNotice-77654'),
  )
  def test_json_format_for_valid_payment_notice_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, payment_notice_pb2.PaymentNotice)

  @parameterized.named_parameters(
      ('with_payment_reconciliation_er2500', 'PaymentReconciliation-ER2500'),
  )
  def test_json_format_for_valid_payment_reconciliation_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, payment_reconciliation_pb2.PaymentReconciliation)

  @parameterized.named_parameters(
      ('with_person_example', 'Person-example'),
      ('with_person_f002', 'Person-f002'),
      ('with_person_grahame', 'Person-grahame'),
      ('with_person_pd', 'Person-pd'),
      ('with_person_pp', 'Person-pp'),
  )
  def test_json_format_for_valid_person_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, person_pb2.Person)

  @parameterized.named_parameters(
      (
          'with_plan_definition_chlamydia_screening_intervention',
          'PlanDefinition-chlamydia-screening-intervention',
      ),
      (
          'with_plan_definition_example_cardiology_os',
          'PlanDefinition-example-cardiology-os',
      ),
      (
          'with_plan_definition_exclusive_breastfeeding_intervention01',
          'PlanDefinition-exclusive-breastfeeding-intervention-01',
      ),
      (
          'with_plan_definition_exclusive_breastfeeding_intervention02',
          'PlanDefinition-exclusive-breastfeeding-intervention-02',
      ),
      (
          'with_plan_definition_exclusive_breastfeeding_intervention03',
          'PlanDefinition-exclusive-breastfeeding-intervention-03',
      ),
      (
          'with_plan_definition_exclusive_breastfeeding_intervention04',
          'PlanDefinition-exclusive-breastfeeding-intervention-04',
      ),
      ('with_plan_definition_kdn5', 'PlanDefinition-KDN5'),
      (
          'with_plan_definition_low_suicide_risk_order_set',
          'PlanDefinition-low-suicide-risk-order-set',
      ),
      ('with_plan_definition_opioidcds04', 'PlanDefinition-opioidcds-04'),
      ('with_plan_definition_opioidcds05', 'PlanDefinition-opioidcds-05'),
      ('with_plan_definition_opioidcds07', 'PlanDefinition-opioidcds-07'),
      ('with_plan_definition_opioidcds08', 'PlanDefinition-opioidcds-08'),
      ('with_plan_definition_opioidcds10', 'PlanDefinition-opioidcds-10'),
      ('with_plan_definition_opioidcds11', 'PlanDefinition-opioidcds-11'),
      (
          'with_plan_definition_options_example',
          'PlanDefinition-options-example',
      ),
      (
          'with_plan_definition_protocol_example',
          'PlanDefinition-protocol-example',
      ),
      (
          'with_plan_definition_zika_virus_intervention_initial',
          'PlanDefinition-zika-virus-intervention-initial',
      ),
      (
          'with_plan_definition_zika_virus_intervention',
          'PlanDefinition-zika-virus-intervention',
      ),
  )
  def test_json_format_for_valid_plan_definition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, plan_definition_pb2.PlanDefinition)

  @parameterized.named_parameters(
      ('with_practitioner_example', 'Practitioner-example'),
      ('with_practitioner_f001', 'Practitioner-f001'),
      ('with_practitioner_f002', 'Practitioner-f002'),
      ('with_practitioner_f003', 'Practitioner-f003'),
      ('with_practitioner_f004', 'Practitioner-f004'),
      ('with_practitioner_f005', 'Practitioner-f005'),
      ('with_practitioner_f006', 'Practitioner-f006'),
      ('with_practitioner_f007', 'Practitioner-f007'),
      ('with_practitioner_f201', 'Practitioner-f201'),
      ('with_practitioner_f202', 'Practitioner-f202'),
      ('with_practitioner_f203', 'Practitioner-f203'),
      ('with_practitioner_f204', 'Practitioner-f204'),
      ('with_practitioner_xcda1', 'Practitioner-xcda1'),
      ('with_practitioner_xcda_author', 'Practitioner-xcda-author'),
  )
  def test_json_format_for_valid_practitioner_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, practitioner_pb2.Practitioner)

  @parameterized.named_parameters(
      ('with_practitioner_role_example', 'PractitionerRole-example'),
  )
  def test_json_format_for_valid_practitioner_role_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, practitioner_role_pb2.PractitionerRole)

  @parameterized.named_parameters(
      ('with_base64_binary', 'base64_binary', datatypes_pb2.Base64Binary),
      ('with_boolean', 'boolean', datatypes_pb2.Boolean),
      ('with_canonical', 'canonical', datatypes_pb2.Canonical),
      ('with_code', 'code', datatypes_pb2.Code),
      ('with_date', 'date', datatypes_pb2.Date),
      ('with_date_time', 'date_time', datatypes_pb2.DateTime),
      ('with_decimal', 'decimal', datatypes_pb2.Decimal),
      ('with_id', 'id', datatypes_pb2.Id),
      ('with_instant', 'instant', datatypes_pb2.Instant),
      ('with_integer', 'integer', datatypes_pb2.Integer),
      ('with_markdown', 'markdown', datatypes_pb2.Markdown),
      ('with_oid', 'oid', datatypes_pb2.Oid),
      ('with_positive_int', 'positive_int', datatypes_pb2.PositiveInt),
      ('with_string', 'string', datatypes_pb2.String),
      ('with_time', 'time', datatypes_pb2.Time),
      ('with_unsigned_int', 'unsigned_int', datatypes_pb2.UnsignedInt),
      ('with_uri', 'uri', datatypes_pb2.Uri),
      ('with_url', 'url', datatypes_pb2.Url),
      ('with_uuid', 'uuid', datatypes_pb2.Uuid),
      ('with_xhtml', 'xhtml', datatypes_pb2.Xhtml),
  )
  def test_json_format_for_valid_primitive_succeeds(
      self, file_name: str, primitive_cls: Type[message.Message]
  ):
    json_path = os.path.join(_VALIDATION_PATH, file_name + '.valid.ndjson')
    proto_path = os.path.join(_VALIDATION_PATH, file_name + '.valid.prototxt')

    def json_object_parser(json_str, *args, **kwargs):
      """Parsing function which exercises json_fhir_object_to_proto."""
      json_value = json_format.load_json(json_str)
      return json_format.json_fhir_object_to_proto(json_value, *args, **kwargs)

    self.assert_parse_equals_golden(
        json_path,
        proto_path,
        primitive_cls,
        parse_f=json_format.json_fhir_string_to_proto,
        json_delimiter='\n',
        proto_delimiter='\n---\n',
        validate=True,
        default_timezone='Australia/Sydney')
    self.assert_parse_equals_golden(
        json_path,
        proto_path,
        primitive_cls,
        parse_f=json_object_parser,
        json_delimiter='\n',
        proto_delimiter='\n---\n',
        validate=True,
        default_timezone='Australia/Sydney')
    self.assert_print_equals_golden(
        json_path,
        proto_path,
        primitive_cls,
        print_f=json_format.pretty_print_fhir_to_json_string,
        json_delimiter='\n',
        proto_delimiter='\n---\n')

  @parameterized.named_parameters(
      ('with_procedure_ambulation', 'Procedure-ambulation'),
      (
          'with_procedure_appendectomy_narrative',
          'Procedure-appendectomy-narrative',
      ),
      ('with_procedure_biopsy', 'Procedure-biopsy'),
      ('with_procedure_colon_biopsy', 'Procedure-colon-biopsy'),
      ('with_procedure_colonoscopy', 'Procedure-colonoscopy'),
      ('with_procedure_education', 'Procedure-education'),
      ('with_procedure_example_implant', 'Procedure-example-implant'),
      ('with_procedure_example', 'Procedure-example'),
      ('with_procedure_f001', 'Procedure-f001'),
      ('with_procedure_f002', 'Procedure-f002'),
      ('with_procedure_f003', 'Procedure-f003'),
      ('with_procedure_f004', 'Procedure-f004'),
      ('with_procedure_f201', 'Procedure-f201'),
      ('with_procedure_hcbs', 'Procedure-HCBS'),
      ('with_procedure_ob', 'Procedure-ob'),
      ('with_procedure_physical_therapy', 'Procedure-physical-therapy'),
  )
  def test_json_format_for_valid_procedure_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   procedure_pb2.Procedure)

  @parameterized.named_parameters(
      ('with_provenance_consent_signature', 'Provenance-consent-signature'),
      (
          'with_provenance_example_biocompute_object',
          'Provenance-example-biocompute-object',
      ),
      ('with_provenance_example_cwl', 'Provenance-example-cwl'),
      ('with_provenance_example', 'Provenance-example'),
      ('with_provenance_signature', 'Provenance-signature'),
  )
  def test_json_format_for_valid_provenance_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   provenance_pb2.Provenance)

  @parameterized.named_parameters(
      ('with_questionnaire3141', 'Questionnaire-3141'),
      ('with_questionnaire_bb', 'Questionnaire-bb'),
      ('with_questionnaire_f201', 'Questionnaire-f201'),
      ('with_questionnaire_gcs', 'Questionnaire-gcs'),
      (
          'with_questionnaire_phq9_questionnaire',
          'Questionnaire-phq-9-questionnaire',
      ),
      ('with_questionnaire_qs1', 'Questionnaire-qs1'),
      (
          'with_questionnaire_zika_virus_exposure_assessment',
          'Questionnaire-zika-virus-exposure-assessment',
      ),
  )
  def test_json_format_for_valid_questionnaire_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, questionnaire_pb2.Questionnaire)

  @parameterized.named_parameters(
      ('with_questionnaire_response3141', 'QuestionnaireResponse-3141'),
      ('with_questionnaire_response_bb', 'QuestionnaireResponse-bb'),
      ('with_questionnaire_response_f201', 'QuestionnaireResponse-f201'),
      ('with_questionnaire_response_gcs', 'QuestionnaireResponse-gcs'),
      (
          'with_questionnaire_response_ussg_fht_answers',
          'QuestionnaireResponse-ussg-fht-answers',
      ),
  )
  def test_json_format_for_valid_questionnaire_response_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, questionnaire_response_pb2.QuestionnaireResponse)

  @parameterized.named_parameters(
      ('with_related_person_benedicte', 'RelatedPerson-benedicte'),
      ('with_related_person_f001', 'RelatedPerson-f001'),
      ('with_related_person_f002', 'RelatedPerson-f002'),
      ('with_related_person_newborn_mom', 'RelatedPerson-newborn-mom'),
      ('with_related_person_peter', 'RelatedPerson-peter'),
  )
  def test_json_format_for_valid_related_person_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, related_person_pb2.RelatedPerson)

  @parameterized.named_parameters(
      ('with_request_group_example', 'RequestGroup-example'),
      ('with_request_group_kdn5_example', 'RequestGroup-kdn5-example'),
  )
  def test_json_format_for_valid_request_group_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, request_group_pb2.RequestGroup)

  @parameterized.named_parameters(
      ('with_research_definition_example', 'ResearchDefinition-example'),
  )
  def test_json_format_for_valid_research_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, research_definition_pb2.ResearchDefinition)

  @parameterized.named_parameters(
      (
          'with_research_element_definition_example',
          'ResearchElementDefinition-example',
      ),
  )
  def test_json_format_for_valid_research_element_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, research_element_definition_pb2.ResearchElementDefinition)

  @parameterized.named_parameters(
      ('with_research_study_example', 'ResearchStudy-example'),
  )
  def test_json_format_for_valid_research_study_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, research_study_pb2.ResearchStudy)

  @parameterized.named_parameters(
      ('with_research_subject_example', 'ResearchSubject-example'),
  )
  def test_json_format_for_valid_research_subject_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, research_subject_pb2.ResearchSubject)

  @parameterized.named_parameters(
      (
          'with_risk_assessment_breastcancer_risk',
          'RiskAssessment-breastcancer-risk',
      ),
      ('with_risk_assessment_cardiac', 'RiskAssessment-cardiac'),
      ('with_risk_assessment_genetic', 'RiskAssessment-genetic'),
      ('with_risk_assessment_population', 'RiskAssessment-population'),
      ('with_risk_assessment_prognosis', 'RiskAssessment-prognosis'),
      ('with_risk_assessment_riskexample', 'RiskAssessment-riskexample'),
  )
  def test_json_format_for_valid_risk_assessment_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, risk_assessment_pb2.RiskAssessment)

  @parameterized.named_parameters(
      ('with_risk_evidence_synthesis_example', 'RiskEvidenceSynthesis-example'),
  )
  def test_json_format_for_valid_risk_evidence_synthesis_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, risk_evidence_synthesis_pb2.RiskEvidenceSynthesis)

  @parameterized.named_parameters(
      ('with_schedule_example', 'Schedule-example'),
      ('with_schedule_example_loc1', 'Schedule-exampleloc1'),
      ('with_schedule_example_loc2', 'Schedule-exampleloc2'),
  )
  def test_json_format_for_valid_schedule_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   schedule_pb2.Schedule)

  @parameterized.named_parameters(
      ('with_service_request_ambulation', 'ServiceRequest-ambulation'),
      (
          'with_service_request_appendectomy_narrative',
          'ServiceRequest-appendectomy-narrative',
      ),
      ('with_service_request_benchpress', 'ServiceRequest-benchpress'),
      ('with_service_request_colon_biopsy', 'ServiceRequest-colon-biopsy'),
      ('with_service_request_colonoscopy', 'ServiceRequest-colonoscopy'),
      ('with_service_request_di', 'ServiceRequest-di'),
      ('with_service_request_do_not_turn', 'ServiceRequest-do-not-turn'),
      ('with_service_request_education', 'ServiceRequest-education'),
      (
          'with_service_request_example_implant',
          'ServiceRequest-example-implant',
      ),
      ('with_service_request_example', 'ServiceRequest-example'),
      ('with_service_request_example_pgx', 'ServiceRequest-example-pgx'),
      ('with_service_request_ft4', 'ServiceRequest-ft4'),
      ('with_service_request_lipid', 'ServiceRequest-lipid'),
      ('with_service_request_myringotomy', 'ServiceRequest-myringotomy'),
      ('with_service_request_ob', 'ServiceRequest-ob'),
      ('with_service_request_og_example1', 'ServiceRequest-og-example1'),
      (
          'with_service_request_physical_therapy',
          'ServiceRequest-physical-therapy',
      ),
      ('with_service_request_physiotherapy', 'ServiceRequest-physiotherapy'),
      ('with_service_request_subrequest', 'ServiceRequest-subrequest'),
      ('with_service_request_vent', 'ServiceRequest-vent'),
  )
  def test_json_format_for_valid_service_request_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, service_request_pb2.ServiceRequest)

  @parameterized.named_parameters(
      ('with_slot1', 'Slot-1'),
      ('with_slot2', 'Slot-2'),
      ('with_slot3', 'Slot-3'),
      ('with_slot_example', 'Slot-example'),
  )
  def test_json_format_for_valid_slot_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, slot_pb2.Slot)

  @parameterized.named_parameters(
      ('with_specimen101', 'Specimen-101'),
      ('with_specimen_isolate', 'Specimen-isolate'),
      ('with_specimen_pooled_serum', 'Specimen-pooled-serum'),
      ('with_specimen_sst', 'Specimen-sst'),
      ('with_specimen_vma_urine', 'Specimen-vma-urine'),
  )
  def test_json_format_for_valid_specimen_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   specimen_pb2.Specimen)

  @parameterized.named_parameters(
      ('with_specimen_definition2364', 'SpecimenDefinition-2364'),
  )
  def test_json_format_for_valid_specimen_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, specimen_definition_pb2.SpecimenDefinition)

  @parameterized.named_parameters(
      ('with_structure_map_example', 'StructureMap-example'),
      (
          'with_structure_map_supplyrequest_transform',
          'StructureMap-supplyrequest-transform',
      ),
  )
  def test_json_format_for_valid_structure_map_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, structure_map_pb2.StructureMap)

  @parameterized.named_parameters(
      ('with_structure_definition_coding', 'StructureDefinition-Coding'),
      (
          'with_structure_definition_lipid_profile',
          'StructureDefinition-lipidprofile',
      ),
  )
  def test_json_format_for_valid_structure_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, structure_definition_pb2.StructureDefinition)

  @parameterized.named_parameters(
      ('with_subscription_example_error', 'Subscription-example-error'),
      ('with_subscription_example', 'Subscription-example'),
  )
  def test_json_format_for_valid_subscription_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, subscription_pb2.Subscription)

  @parameterized.named_parameters(
      ('with_substance_example', 'Substance-example'),
      ('with_substance_f201', 'Substance-f201'),
      ('with_substance_f202', 'Substance-f202'),
      ('with_substance_f203', 'Substance-f203'),
      ('with_substance_f204', 'Substance-f204'),
      ('with_substance_f205', 'Substance-f205'),
  )
  def test_json_format_for_valid_substance_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   substance_pb2.Substance)

  @parameterized.named_parameters(
      (
          'with_substance_specification_example',
          'SubstanceSpecification-example',
      ),
  )
  def test_json_format_for_valid_substance_specification_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, substance_specification_pb2.SubstanceSpecification)

  @parameterized.named_parameters(
      ('with_supply_delivery_pumpdelivery', 'SupplyDelivery-pumpdelivery'),
      ('with_supply_delivery_simpledelivery', 'SupplyDelivery-simpledelivery'),
  )
  def test_json_format_for_valid_supply_delivery_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, supply_delivery_pb2.SupplyDelivery)

  @parameterized.named_parameters(
      ('with_supply_request_simpleorder', 'SupplyRequest-simpleorder'),
  )
  def test_json_format_for_valid_supply_request_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, supply_request_pb2.SupplyRequest)

  @parameterized.named_parameters(
      ('with_task_example1', 'Task-example1'),
      ('with_task_example2', 'Task-example2'),
      ('with_task_example3', 'Task-example3'),
      ('with_task_example4', 'Task-example4'),
      ('with_task_example5', 'Task-example5'),
      ('with_task_example6', 'Task-example6'),
      ('with_task_fm_example1', 'Task-fm-example1'),
      ('with_task_fm_example2', 'Task-fm-example2'),
      ('with_task_fm_example3', 'Task-fm-example3'),
      ('with_task_fm_example4', 'Task-fm-example4'),
      ('with_task_fm_example5', 'Task-fm-example5'),
      ('with_task_fm_example6', 'Task-fm-example6'),
  )
  def test_json_format_for_valid_task_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, task_pb2.Task)

  @parameterized.named_parameters(
      (
          'with_terminology_capabilities_example',
          'TerminologyCapabilities-example',
      ),
  )
  def test_json_format_for_valid_terminology_capabilities_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, terminology_capabilities_pb2.TerminologyCapabilities)

  @parameterized.named_parameters(
      ('with_test_report_test_report_example', 'TestReport-testreport-example'),
  )
  def test_json_format_for_valid_test_report_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   test_report_pb2.TestReport)

  @parameterized.named_parameters(
      (
          'with_test_script_test_script_example_history',
          'TestScript-testscript-example-history',
      ),
      ('with_test_script_test_script_example', 'TestScript-testscript-example'),
      (
          'with_test_script_test_script_example_multisystem',
          'TestScript-testscript-example-multisystem',
      ),
      (
          'with_test_script_test_script_example_readtest',
          'TestScript-testscript-example-readtest',
      ),
      (
          'with_test_script_test_script_example_search',
          'TestScript-testscript-example-search',
      ),
      (
          'with_test_script_test_script_example_update',
          'TestScript-testscript-example-update',
      ),
  )
  def test_json_format_for_valid_test_script_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   test_script_pb2.TestScript)

  @parameterized.named_parameters(
      ('with_verification_result_example', 'VerificationResult-example'),
  )
  def test_json_format_for_valid_verification_result_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, verification_result_pb2.VerificationResult)

  @parameterized.named_parameters(
      ('with_vision_prescription33123', 'VisionPrescription-33123'),
      ('with_vision_prescription33124', 'VisionPrescription-33124'),
  )
  def test_json_format_for_valid_vision_prescription_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, vision_prescription_pb2.VisionPrescription)

  @parameterized.named_parameters(
      (
          'with_composition_example',
          'Composition-example',
          composition_pb2.Composition,
      ),
      ('with_ecounter_home', 'Encounter-home', encounter_pb2.Encounter),
      (
          'with_observation_example_genetics1',
          'Observation-example-genetics-1',
          observation_pb2.Observation,
      ),
      ('with_patient_example', 'Patient-example', patient_pb2.Patient),
  )
  def test_print_for_analytics_for_valid_resource_succeeds(
      self, file_name: str, proto_cls: Type[message.Message]
  ):
    json_path = os.path.join(_BIGQUERY_PATH, file_name + '.json')
    proto_path = os.path.join(_EXAMPLES_PATH, file_name + '.prototxt')

    # Assert print for analytics (standard and "pretty")
    self.assert_print_equals_golden(
        json_path,
        proto_path,
        proto_cls,
        print_f=json_format.print_fhir_to_json_string_for_analytics)
    self.assert_print_equals_golden(
        json_path,
        proto_path,
        proto_cls,
        print_f=json_format.pretty_print_fhir_to_json_string_for_analytics)

  def test_json_format_for_invalid_json_fails_validation(self):
    """Ensure we run validations by default."""
    with self.assertRaises(fhir_errors.InvalidFhirError):
      json_format.json_fhir_string_to_proto('{}', code_system_pb2.CodeSystem)

    with self.assertRaises(fhir_errors.InvalidFhirError):
      json_format.json_fhir_object_to_proto({}, code_system_pb2.CodeSystem)

    with self.assertRaises(fhir_errors.InvalidFhirError):
      json_format.merge_json_fhir_string_into_proto(
          '{}', code_system_pb2.CodeSystem())

    with self.assertRaises(fhir_errors.InvalidFhirError):
      json_format.merge_json_fhir_object_into_proto(
          {}, code_system_pb2.CodeSystem())

  def test_json_format_for_invalid_json_skips_validation(self):
    """Ensure validations can be skipped."""
    json_format.json_fhir_string_to_proto(
        '{}', code_system_pb2.CodeSystem, validate=False)
    json_format.json_fhir_object_to_proto({},
                                          code_system_pb2.CodeSystem,
                                          validate=False)

    json_format.merge_json_fhir_string_into_proto(
        '{}', code_system_pb2.CodeSystem(), validate=False)
    json_format.merge_json_fhir_object_into_proto({},
                                                  code_system_pb2.CodeSystem(),
                                                  validate=False)

  def assert_parse_and_print_examples_equals_golden(
      self, file_name: str, proto_cls: Type[message.Message]) -> None:
    """Convenience method for performing assertions on FHIR R4 examples."""
    json_path = os.path.join(_EXAMPLES_PATH, file_name + '.json')
    proto_path = os.path.join(_EXAMPLES_PATH, file_name + '.prototxt')
    self.assert_parse_and_print_equals_golden(json_path, proto_path, proto_cls)

  def assert_parse_and_print_spec_equals_golden(
      self, file_name: str, proto_cls: Type[message.Message]) -> None:
    """Convenience method for performing assertions on the FHIR R4 spec."""
    json_path = os.path.join(_FHIR_SPEC_PATH, file_name + '.json')
    proto_path = os.path.join(_EXAMPLES_PATH, file_name + '.prototxt')
    self.assert_parse_and_print_equals_golden(json_path, proto_path, proto_cls)

  def assert_parse_and_print_equals_golden(
      self, json_path: str, proto_path: str,
      proto_cls: Type[message.Message]) -> None:
    """Convenience method for performing assertions against goldens."""
    # Assert parse
    validate = json_path not in _INVALID_RECORDS
    self.assert_parse_equals_golden(
        json_path,
        proto_path,
        proto_cls,
        parse_f=json_format.json_fhir_string_to_proto,
        validate=validate,
        default_timezone='Australia/Sydney')

    # Assert print (standard and "pretty")
    self.assert_print_equals_golden(
        json_path,
        proto_path,
        proto_cls,
        print_f=json_format.print_fhir_to_json_string)
    self.assert_print_equals_golden(
        json_path,
        proto_path,
        proto_cls,
        print_f=json_format.pretty_print_fhir_to_json_string)


if __name__ == '__main__':
  absltest.main()
