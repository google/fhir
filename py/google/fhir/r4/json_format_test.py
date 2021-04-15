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
from typing import TypeVar, Type

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
from google.fhir.json_format import json_format_test
from google.fhir.r4 import json_format
from google.fhir.testing import testdata_utils
from google.fhir.utils import proto_utils

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
      ('_withAccountEwg', 'Account-ewg'),
      ('_withAccountExample', 'Account-example'),
  )
  def testJsonFormat_forValidAccount_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   account_pb2.Account)

  @parameterized.named_parameters(
      ('_withActivityDefinitionAdministerZikaVirusExposureAssessment',
       'ActivityDefinition-administer-zika-virus-exposure-assessment'),
      ('_withActivityDefinitionBloodTubesSupply',
       'ActivityDefinition-blood-tubes-supply'),
      ('_withActivityDefinitionCitalopramPrescription',
       'ActivityDefinition-citalopramPrescription'),
      ('_withActivityDefinitionHeartValveReplacement',
       'ActivityDefinition-heart-valve-replacement'),
      ('_withActivityDefinitionProvideMosquitoPreventionAdvice',
       'ActivityDefinition-provide-mosquito-prevention-advice'),
      ('_withActivityDefinitionReferralPrimaryCareMentalHealthInitial',
       'ActivityDefinition-referralPrimaryCareMentalHealth-initial'),
      ('_withActivityDefinitionReferralPrimaryCareMentalHealth',
       'ActivityDefinition-referralPrimaryCareMentalHealth'),
      ('_withActivityDefinitionSerumDengueVirusIgm',
       'ActivityDefinition-serum-dengue-virus-igm'),
      ('_withActivityDefinitionSerumZikaDengueVirusIgm',
       'ActivityDefinition-serum-zika-dengue-virus-igm'),
  )
  def testJsonFormat_forValidActivityDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, activity_definition_pb2.ActivityDefinition)

  @parameterized.named_parameters(
      ('_withAdverseEventExample', 'AdverseEvent-example'),)
  def testJsonFormat_forValidAdverseEvent_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, adverse_event_pb2.AdverseEvent)

  @parameterized.named_parameters(
      ('_withAllergyIntoleranceExample', 'AllergyIntolerance-example'),
      ('_withAllergyIntoleranceFishallergy', 'AllergyIntolerance-fishallergy'),
      ('_withAllergyIntoleranceMedication', 'AllergyIntolerance-medication'),
      ('_withAllergyIntoleranceNka', 'AllergyIntolerance-nka'),
      ('_withAllergyIntoleranceNkda', 'AllergyIntolerance-nkda'),
      ('_withAllergyIntoleranceNkla', 'AllergyIntolerance-nkla'),
  )
  def testJsonFormat_forValidAllergyIntolerance_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, allergy_intolerance_pb2.AllergyIntolerance)

  @parameterized.named_parameters(
      ('_withAppointment2docs', 'Appointment-2docs'),
      ('_withAppointmentExample', 'Appointment-example'),
      ('_withAppointmentExampleReq', 'Appointment-examplereq'),
  )
  def testJsonFormat_forValidAppointment_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   appointment_pb2.Appointment)

  @parameterized.named_parameters(
      ('_withAppointmentResponseExample', 'AppointmentResponse-example'),
      ('_withAppointmentResponseExampleResp',
       'AppointmentResponse-exampleresp'),
  )
  def testJsonFormat_forValidAppointmentResponse_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, appointment_response_pb2.AppointmentResponse)

  @parameterized.named_parameters(
      ('_withAuditEventExampleDisclosure', 'AuditEvent-example-disclosure'),
      ('_withAuditEventExampleError', 'AuditEvent-example-error'),
      ('_withAuditEventExample', 'AuditEvent-example'),
      ('_withAuditEventExampleLogin', 'AuditEvent-example-login'),
      ('_withAuditEventExampleLogout', 'AuditEvent-example-logout'),
      ('_withAuditEventExampleMedia', 'AuditEvent-example-media'),
      ('_withAuditEventExamplePixQuery', 'AuditEvent-example-pixQuery'),
      ('_withAuditEventExampleRest', 'AuditEvent-example-rest'),
      ('_withAuditEventExampleSearch', 'AuditEvent-example-search'),
  )
  def testJsonFormat_forValidAuditEvent_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   audit_event_pb2.AuditEvent)

  @parameterized.named_parameters(
      ('_withBasicBasicExampleNarrative', 'Basic-basic-example-narrative'),
      ('_withBasicClassModel', 'Basic-classModel'),
      ('_withBasicReferral', 'Basic-referral'),
  )
  def testJsonFormat_forValidBasic_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, basic_pb2.Basic)

  @parameterized.named_parameters(
      ('_withBinaryExample', 'Binary-example'),
      ('_withBinaryF006', 'Binary-f006'),
  )
  def testJsonFormat_forValidBinary_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, binary_pb2.Binary)

  @parameterized.named_parameters(
      ('_withBiologicallyDerivedProductExample',
       'BiologicallyDerivedProduct-example'),)
  def testJsonFormat_forValidBiologicallyDerivedProduct_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, biologically_derived_product_pb2.BiologicallyDerivedProduct)

  @parameterized.named_parameters(
      ('_withBodyStructureFetus', 'BodyStructure-fetus'),
      ('_withBodyStructureSkinPatch', 'BodyStructure-skin-patch'),
      ('_withBodyStructureTumor', 'BodyStructure-tumor'),
  )
  def testJsonFormat_forValidBodyStructure_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, body_structure_pb2.BodyStructure)

  @parameterized.named_parameters(
      ('_withBundle101', 'Bundle-101'),
      ('_withBundle10bb101fA1214264A92067be9cb82c74',
       'Bundle-10bb101f-a121-4264-a920-67be9cb82c74'),
      ('_withBundle3a0707d3549e4467B8b85a2ab3800efe',
       'Bundle-3a0707d3-549e-4467-b8b8-5a2ab3800efe'),
      ('_withBundle3ad0687eF477468cAfd5Fcc2bf897808',
       'Bundle-3ad0687e-f477-468c-afd5-fcc2bf897808'),
      ('_withBundle3ad0687eF477468cAfd5Fcc2bf897809',
       'Bundle-3ad0687e-f477-468c-afd5-fcc2bf897809'),
      ('_withBundle3ad0687eF477468cAfd5Fcc2bf897819',
       'Bundle-3ad0687e-f477-468c-afd5-fcc2bf897819'),
      ('_withBundle72ac849352ac41bd8d5d7258c289b5ea',
       'Bundle-72ac8493-52ac-41bd-8d5d-7258c289b5ea'),
      ('_withBundleB0a5e427783c44adb87e2E3efe3369b6f',
       'Bundle-b0a5e4277-83c4-4adb-87e2-e3efe3369b6f'),
      ('_withBundleB248b1b216864b94993637d7a5f94b51',
       'Bundle-b248b1b2-1686-4b94-9936-37d7a5f94b51'),
      ('_withBundleBundleExample', 'Bundle-bundle-example'),
      ('_withBundleBundleReferences', 'Bundle-bundle-references'),
      ('_withBundleBundleRequestMedsallergies',
       'Bundle-bundle-request-medsallergies'),
      ('_withBundleBundleRequestSimpleSummary',
       'Bundle-bundle-request-simplesummary'),
      ('_withBundleBundleResponse', 'Bundle-bundle-response'),
      ('_withBundleBundleResponseMedsAllergies',
       'Bundle-bundle-response-medsallergies'),
      ('_withBundleBundleResponseSimpleSummary',
       'Bundle-bundle-response-simplesummary'),
      ('_withBundleBundleSearchWarning', 'Bundle-bundle-search-warning'),
      ('_withBundleBundleTransaction', 'Bundle-bundle-transaction'),
      ('_withBundleConceptMaps', 'Bundle-conceptmaps'),

      # TODO: Investigate test timeouts
      # ('_withBundleDataElements', 'Bundle-dataelements'),
      # ('_withBundleDg2', 'Bundle-dg2'),
      # ('_withBundleExtensions', 'Bundle-extensions'),
      # ('_withBundleExternals', 'Bundle-externals'),
      # ('_withBundleF001', 'Bundle-f001'),
      # ('_withBundleF202', 'Bundle-f202'),
      # ('_withBundleFather', 'Bundle-father'),
      # ('_withBundleGhp', 'Bundle-ghp'),
      # ('_withBundleHla1', 'Bundle-hla-1'),
      # ('_withBundleLipids', 'Bundle-lipids'),
      # ('_withBundleLriExample', 'Bundle-lri-example'),
      # ('_withBundleMicro', 'Bundle-micro'),
      # ('_withBundleProfilesOthers', 'Bundle-profiles-others'),
      # ('_withBundleRegistry', 'Bundle-registry'),
      # ('_withBundleReport', 'Bundle-report'),
      # ('_withBundleSearchParams', 'Bundle-searchParams'),
      # ('_withBundleTerminologies', 'Bundle-terminologies'),
      # ('_withBundleTypes', 'Bundle-types'),
      # ('_withBundleUssgFht', 'Bundle-ussg-fht'),
      # ('_withBundleValuesetExpansions', 'Bundle-valueset-expansions'),
      # ('_withBundleXds', 'Bundle-xds'),
      # ('_withBundleResources', 'Bundle-resources'),
  )
  def testJsonFormat_forValidBundle_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, bundle_and_contained_resource_pb2.Bundle)

  @parameterized.named_parameters(
      ('_withCapabilityStatementBase2', 'CapabilityStatement-base2'),
      ('_withCapabilityStatementBase', 'CapabilityStatement-base'),
      ('_withCapabilityStatementExample', 'CapabilityStatement-example'),
      ('_withCapabilityStatementKnowledgeRepository',
       'CapabilityStatement-knowledge-repository'),
      ('_withCapabilityStatementMeasureProcessor',
       'CapabilityStatement-measure-processor'),
      ('_withCapabilityStatementMessagedefinition',
       'CapabilityStatement-messagedefinition'),
      ('_withCapabilityStatementPhr', 'CapabilityStatement-phr'),
      ('_withCapabilityStatementTerminologyServer',
       'CapabilityStatement-terminology-server'),
  )
  def testJsonFormat_forValidCapabilityStatement_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, capability_statement_pb2.CapabilityStatement)

  @parameterized.named_parameters(
      ('_withCarePlanExample', 'CarePlan-example'),
      ('_withCarePlanF001', 'CarePlan-f001'),
      ('_withCarePlanF002', 'CarePlan-f002'),
      ('_withCarePlanF003', 'CarePlan-f003'),
      ('_withCarePlanF201', 'CarePlan-f201'),
      ('_withCarePlanF202', 'CarePlan-f202'),
      ('_withCarePlanF203', 'CarePlan-f203'),
      ('_withCarePlanGpvisit', 'CarePlan-gpvisit'),
      ('_withCarePlanIntegrate', 'CarePlan-integrate'),
      ('_withCarePlanObesityNarrative', 'CarePlan-obesity-narrative'),
      ('_withCarePlanPreg', 'CarePlan-preg'),
  )
  def testJsonFormat_forValidCarePlan_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   care_plan_pb2.CarePlan)

  @parameterized.named_parameters(
      ('_withCareTeamExample', 'CareTeam-example'),)
  def testJsonFormat_forValidCareTeam_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   care_team_pb2.CareTeam)

  @parameterized.named_parameters(
      ('_withCatalogEntryExample', 'CatalogEntry-example'),)
  def testJsonFormat_forValidCatalogEntry_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, catalog_entry_pb2.CatalogEntry)

  @parameterized.named_parameters(
      ('_withChargeItemExample', 'ChargeItem-example'),)
  def testJsonFormat_forValidChargeItem_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   charge_item_pb2.ChargeItem)

  @parameterized.named_parameters(
      ('_withChargeItemDefinitionDevice', 'ChargeItemDefinition-device'),
      ('_withChargeItemDefinitionEbm', 'ChargeItemDefinition-ebm'),
  )
  def testJsonFormat_forValidChargeItemDefinition_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, charge_item_definition_pb2.ChargeItemDefinition)

  @parameterized.named_parameters(
      ('_withClaim100150', 'Claim-100150'),
      ('_withClaim100151', 'Claim-100151'),
      ('_withClaim100152', 'Claim-100152'),
      ('_withClaim100153', 'Claim-100153'),
      ('_withClaim100154', 'Claim-100154'),
      ('_withClaim100155', 'Claim-100155'),
      ('_withClaim100156', 'Claim-100156'),
      ('_withClaim660150', 'Claim-660150'),
      ('_withClaim660151', 'Claim-660151'),
      ('_withClaim660152', 'Claim-660152'),
      ('_withClaim760150', 'Claim-760150'),
      ('_withClaim760151', 'Claim-760151'),
      ('_withClaim760152', 'Claim-760152'),
      ('_withClaim860150', 'Claim-860150'),
      ('_withClaim960150', 'Claim-960150'),
      ('_withClaim960151', 'Claim-960151'),
      ('_withClaimMED00050', 'Claim-MED-00050'),
  )
  def testJsonFormat_forValidClaim_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, claim_pb2.Claim)

  @parameterized.named_parameters(
      ('_withClaimResponseR3500', 'ClaimResponse-R3500'),
      ('_withClaimResponseR3501', 'ClaimResponse-R3501'),
      ('_withClaimResponseR3502', 'ClaimResponse-R3502'),
      ('_withClaimResponseR3503', 'ClaimResponse-R3503'),
      ('_withClaimResponseUr3503', 'ClaimResponse-UR3503'),
  )
  def testJsonFormat_forValidClaimResponse_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, claim_response_pb2.ClaimResponse)

  @parameterized.named_parameters(
      ('_withClinicalImpressionExample', 'ClinicalImpression-example'),)
  def testJsonFormat_forValidClinicalImpression_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, clinical_impression_pb2.ClinicalImpression)

  @parameterized.named_parameters(
      ('_withCommunicationExample', 'Communication-example'),
      ('_withCommunicationFmAttachment', 'Communication-fm-attachment'),
      ('_withCommunicationFmSolicited', 'Communication-fm-solicited'),
  )
  def testJsonFormat_forValidCommunication_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, communication_pb2.Communication)

  @parameterized.named_parameters(
      ('_withCommunicationRequestExample', 'CommunicationRequest-example'),
      ('_withCommunicationRequestFmSolicit', 'CommunicationRequest-fm-solicit'),
  )
  def testJsonFormat_forValidCommunicationRequest_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, communication_request_pb2.CommunicationRequest)

  @parameterized.named_parameters(
      ('_withCompartmentDefinitionDevice', 'CompartmentDefinition-device'),
      ('_withCompartmentDefinitionEncounter',
       'CompartmentDefinition-encounter'),
      ('_withCompartmentDefinitionExample', 'CompartmentDefinition-example'),
      ('_withCompartmentDefinitionPatient', 'CompartmentDefinition-patient'),
      ('_withCompartmentDefinitionPractitioner',
       'CompartmentDefinition-practitioner'),
      ('_withCompartmentDefinitionRelatedPerson',
       'CompartmentDefinition-relatedPerson'),
  )
  def testJsonFormat_forValidCompartmentDefinition_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, compartment_definition_pb2.CompartmentDefinition)

  @parameterized.named_parameters(
      ('_withCompositionExample', 'Composition-example'),
      ('_withCompositionExampleMixed', 'Composition-example-mixed'),
  )
  def testJsonFormat_forValidComposition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   composition_pb2.Composition)

  @parameterized.named_parameters(
      ('_withConditionExample2', 'Condition-example2'),
      ('_withConditionExample', 'Condition-example'),
      ('_withConditionF001', 'Condition-f001'),
      ('_withConditionF002', 'Condition-f002'),
      ('_withConditionF003', 'Condition-f003'),
      ('_withConditionF201', 'Condition-f201'),
      ('_withConditionF202', 'Condition-f202'),
      ('_withConditionF203', 'Condition-f203'),
      ('_withConditionF204', 'Condition-f204'),
      ('_withConditionF205', 'Condition-f205'),
      ('_withConditionFamilyHistory', 'Condition-family-history'),
      ('_withConditionStroke', 'Condition-stroke'),
  )
  def testJsonFormat_forValidCondition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   condition_pb2.Condition)

  @parameterized.named_parameters(
      ('_withConsentConsentExampleBasic', 'Consent-consent-example-basic'),
      ('_withConsentConsentExampleEmergency',
       'Consent-consent-example-Emergency'),
      ('_withConsentConsentExampleGrantor', 'Consent-consent-example-grantor'),
      ('_withConsentConsentExampleNotAuthor',
       'Consent-consent-example-notAuthor'),
      ('_withConsentConsentExampleNotOrg', 'Consent-consent-example-notOrg'),
      ('_withConsentConsentExampleNotThem', 'Consent-consent-example-notThem'),
      ('_withConsentConsentExampleNotThis', 'Consent-consent-example-notThis'),
      ('_withConsentConsentExampleNotTime', 'Consent-consent-example-notTime'),
      ('_withConsentConsentExampleOut', 'Consent-consent-example-Out'),
      ('_withConsentConsentExamplePkb', 'Consent-consent-example-pkb'),
      ('_withConsentConsentExampleSignature',
       'Consent-consent-example-signature'),
      ('_withConsentConsentExampleSmartonfhir',
       'Consent-consent-example-smartonfhir'),
  )
  def testJsonFormat_forValidConsent_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   consent_pb2.Consent)

  @parameterized.named_parameters(
      ('_withAllergyIntoleranceExample', 'AllergyIntolerance-example',
       allergy_intolerance_pb2.AllergyIntolerance, 'allergy_intolerance'),
      ('_withCapabilityStatementBase', 'CapabilityStatement-base',
       capability_statement_pb2.CapabilityStatement, 'capability_statement'),
      ('_withImmunizationExample', 'Immunization-example',
       immunization_pb2.Immunization, 'immunization'),
      ('_withMedicationMed0305', 'Medication-med0305',
       medication_pb2.Medication, 'medication'),
      ('_withObservationF004', 'Observation-f004', observation_pb2.Observation,
       'observation'),
      ('_withPatientAnimal', 'Patient-animal', patient_pb2.Patient, 'patient'),
      ('_withPractitionerF003', 'Practitioner-f003',
       practitioner_pb2.Practitioner, 'practitioner'),
      ('_withProcedureAmbulation', 'Procedure-ambulation',
       procedure_pb2.Procedure, 'procedure'),
      ('_withTaskExample4', 'Task-example4', task_pb2.Task, 'task'),
  )
  def testJsonFormat_forValidContainedResource_succeeds(
      self, file_name: str, proto_cls: Type[message.Message],
      contained_field: str):
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
      ('_withContractC123', 'Contract-C-123'),
      ('_withContractC2121', 'Contract-C-2121'),
      ('_withContractIns101', 'Contract-INS-101'),
      ('_withContractPcdExampleNotAuthor', 'Contract-pcd-example-notAuthor'),
      ('_withContractPcdExampleNotLabs', 'Contract-pcd-example-notLabs'),
      ('_withContractPcdExampleNotOrg', 'Contract-pcd-example-notOrg'),
      ('_withContractPcdExampleNotThem', 'Contract-pcd-example-notThem'),
      ('_withContractPcdExampleNotThis', 'Contract-pcd-example-notThis'),
  )
  def testJsonFormat_forValidContract_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   contract_pb2.Contract)

  @parameterized.named_parameters(
      ('_withCoverage7546D', 'Coverage-7546D'),
      ('_withCoverage7547E', 'Coverage-7547E'),
      ('_withCoverage9876B1', 'Coverage-9876B1'),
      ('_withCoverageSP1234', 'Coverage-SP1234'),
  )
  def testJsonFormat_forValidCoverage_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   coverage_pb2.Coverage)

  @parameterized.named_parameters(
      ('_withCoverageEligibilityRequest52345',
       'CoverageEligibilityRequest-52345'),
      ('_withCoverageEligibilityRequest52346',
       'CoverageEligibilityRequest-52346'),
  )
  def testJsonFormat_forValidCoverageEligibilityRequest_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, coverage_eligibility_request_pb2.CoverageEligibilityRequest)

  @parameterized.named_parameters(
      ('_withCoverageEligibilityResponseE2500',
       'CoverageEligibilityResponse-E2500'),
      ('_withCoverageEligibilityResponseE2501',
       'CoverageEligibilityResponse-E2501'),
      ('_withCoverageEligibilityResponseE2502',
       'CoverageEligibilityResponse-E2502'),
      ('_withCoverageEligibilityResponseE2503',
       'CoverageEligibilityResponse-E2503'),
  )
  def testJsonFormat_forValidCoverageEligibilityResponse_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name,
        coverage_eligibility_response_pb2.CoverageEligibilityResponse)

  @parameterized.named_parameters(
      ('_withDetectedIssueAllergy', 'DetectedIssue-allergy'),
      ('_withDetectedIssueDdi', 'DetectedIssue-ddi'),
      ('_withDetectedIssueDuplicate', 'DetectedIssue-duplicate'),
      ('_withDetectedIssueLab', 'DetectedIssue-lab'),
  )
  def testJsonFormat_forValidDetectedIssue_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, detected_issue_pb2.DetectedIssue)

  @parameterized.named_parameters(
      ('_withDeviceExample', 'Device-example'),
      ('_withDeviceF001', 'Device-f001'),
  )
  def testJsonFormat_forValidDevice_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, device_pb2.Device)

  @parameterized.named_parameters(
      ('_withDeviceDefinitionExample', 'DeviceDefinition-example'),)
  def testJsonFormat_forValidDeviceDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, device_definition_pb2.DeviceDefinition)

  @parameterized.named_parameters(
      ('_withDeviceMetricExample', 'DeviceMetric-example'),)
  def testJsonFormat_forValidDeviceMetric_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, device_metric_pb2.DeviceMetric)

  @parameterized.named_parameters(
      ('_withDeviceRequestExample', 'DeviceRequest-example'),
      ('_withDeviceRequestInsulinPump', 'DeviceRequest-insulinpump'),
      ('_withDeviceRequestLeftLens', 'DeviceRequest-left-lens'),
      ('_withDeviceRequestRightLens', 'DeviceRequest-right-lens'),
  )
  def testJsonFormat_forValidDeviceRequest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, device_request_pb2.DeviceRequest)

  @parameterized.named_parameters(
      ('_withDeviceUseStatementExample', 'DeviceUseStatement-example'),)
  def testJsonFormat_forValidDeviceUseStatement_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, device_use_statement_pb2.DeviceUseStatement)

  @parameterized.named_parameters(
      ('_withDiagnosticReport102', 'DiagnosticReport-102'),
      ('_withDiagnosticReportExamplePgx', 'DiagnosticReport-example-pgx'),
      ('_withDiagnosticReportF201', 'DiagnosticReport-f201'),
      # TODO: Disabling due to flaky failures.
      # ('_withDiagnosticReportGingivalMass', 'DiagnosticReport-gingival-mass'),
      ('_withDiagnosticReportPap', 'DiagnosticReport-pap'),
      ('_withDiagnosticReportUltrasound', 'DiagnosticReport-ultrasound'),
  )
  def testJsonFormat_forValidDiagnosticReport_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, diagnostic_report_pb2.DiagnosticReport)

  @parameterized.named_parameters(
      ('_withDocumentManifest654789', 'DocumentManifest-654789'),
      ('_withDocumentManifestExample', 'DocumentManifest-example'),
  )
  def testJsonFormat_forValidDocumentManifest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, document_manifest_pb2.DocumentManifest)

  @parameterized.named_parameters(
      ('_withDocumentReferenceExample', 'DocumentReference-example'),)
  def testJsonFormat_forValidDocumentReference_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, document_reference_pb2.DocumentReference)

  @parameterized.named_parameters(
      ('_withEffectEvidenceSynthesisExample',
       'EffectEvidenceSynthesis-example'),)
  def testJsonFormat_forValidEffectEvidenceSynthesis_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, effect_evidence_synthesis_pb2.EffectEvidenceSynthesis)

  @parameterized.named_parameters(
      ('_withParametersEmptyResource', 'Parameters-empty-resource',
       parameters_pb2.Parameters),)
  def testJsonFormat_forValidEmptyNestedResource_succeeds(
      self, file_name: str, proto_cls: Type[message.Message]):
    self.assert_parse_and_print_examples_equals_golden(file_name, proto_cls)

  @parameterized.named_parameters(
      ('_withEncounterEmerg', 'Encounter-emerg'),
      ('_withEncounterExample', 'Encounter-example'),
      ('_withEncounterF001', 'Encounter-f001'),
      ('_withEncounterF002', 'Encounter-f002'),
      ('_withEncounterF003', 'Encounter-f003'),
      ('_withEncounterF201', 'Encounter-f201'),
      ('_withEncounterF202', 'Encounter-f202'),
      ('_withEncounterF203', 'Encounter-f203'),
      ('_withEncounterHome', 'Encounter-home'),
      ('_withEncounterXcda', 'Encounter-xcda'),
  )
  def testJsonFormat_forValidEncounter_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   encounter_pb2.Encounter)

  @parameterized.named_parameters(
      ('_withEndpointDirectEndpoint', 'Endpoint-direct-endpoint'),
      ('_withEndpointExampleIid', 'Endpoint-example-iid'),
      ('_withEndpointExample', 'Endpoint-example'),
      ('_withEndpointExampleWadors', 'Endpoint-example-wadors'),
  )
  def testJsonFormat_forValidEndpoint_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   endpoint_pb2.Endpoint)

  @parameterized.named_parameters(
      ('_withEnrollmentRequest22345', 'EnrollmentRequest-22345'),)
  def testJsonFormat_forValidEnrollmentRequest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, enrollment_request_pb2.EnrollmentRequest)

  @parameterized.named_parameters(
      ('_withEnrollmentResponseER2500', 'EnrollmentResponse-ER2500'),)
  def testJsonFormat_forValidEnrollmentResponse_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, enrollment_response_pb2.EnrollmentResponse)

  @parameterized.named_parameters(
      ('_withEpisodeOfCareExample', 'EpisodeOfCare-example'),)
  def testJsonFormat_forValidEpisodeOfCare_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, episode_of_care_pb2.EpisodeOfCare)

  @parameterized.named_parameters(
      ('_withEventDefinitionExample', 'EventDefinition-example'),)
  def testJsonFormat_forValidEventDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, event_definition_pb2.EventDefinition)

  @parameterized.named_parameters(
      ('_withEvidenceExample', 'Evidence-example'),)
  def testJsonFormat_forValidEvidence_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   evidence_pb2.Evidence)

  @parameterized.named_parameters(
      ('_withEvidenceVariableExample', 'EvidenceVariable-example'),)
  def testJsonFormat_forValidEvidenceVariable_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, evidence_variable_pb2.EvidenceVariable)

  @parameterized.named_parameters(
      ('_withExampleScenarioExample', 'ExampleScenario-example'),)
  def testJsonFormat_forValidExampleScenario_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, example_scenario_pb2.ExampleScenario)

  @parameterized.named_parameters(
      ('_withExplanationOfBenefitEb3500', 'ExplanationOfBenefit-EB3500'),
      ('_withExplanationOfBenefitEb3501', 'ExplanationOfBenefit-EB3501'),
  )
  def testJsonFormat_forValidExplanationOfBenefit_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, explanation_of_benefit_pb2.ExplanationOfBenefit)

  @parameterized.named_parameters(
      ('_withFamilyMemberHistoryFather', 'FamilyMemberHistory-father'),
      ('_withFamilyMemberHistoryMother', 'FamilyMemberHistory-mother'),
  )
  def testJsonFormat_forValidFamilyMemberHistory_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, family_member_history_pb2.FamilyMemberHistory)

  @parameterized.named_parameters(
      ('_withFlagExampleEncounter', 'Flag-example-encounter'),
      ('_withFlagExample', 'Flag-example'),
  )
  def testJsonFormat_forValidFlag_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, flag_pb2.Flag)

  @parameterized.named_parameters(
      ('_withGoalExample', 'Goal-example'),
      ('_withGoalStopSmoking', 'Goal-stop-smoking'),
  )
  def testJsonFormat_forValidGoal_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, goal_pb2.Goal)

  @parameterized.named_parameters(
      ('_withGraphDefinitionExample', 'GraphDefinition-example'),)
  def testJsonFormat_forValidGraphDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, graph_definition_pb2.GraphDefinition)

  @parameterized.named_parameters(
      ('_withGroup101', 'Group-101'),
      ('_withGroup102', 'Group-102'),
      ('_withGroupExamplePatientlist', 'Group-example-patientlist'),
      ('_withGroupHerd1', 'Group-herd1'),
  )
  def testJsonFormat_forValidGroup_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, group_pb2.Group)

  @parameterized.named_parameters(
      ('_withGuidanceResponseExample', 'GuidanceResponse-example'),)
  def testJsonFormat_forValidGuidanceResponse_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, guidance_response_pb2.GuidanceResponse)

  @parameterized.named_parameters(
      ('_withHealthcareServiceExample', 'HealthcareService-example'),)
  def testJsonFormat_forValidHealthcareService_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, healthcare_service_pb2.HealthcareService)

  @parameterized.named_parameters(
      ('_withImagingStudyExample', 'ImagingStudy-example'),
      ('_withImagingStudyExampleXr', 'ImagingStudy-example-xr'),
  )
  def testJsonFormat_forValidImagingStudy_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, imaging_study_pb2.ImagingStudy)

  @parameterized.named_parameters(
      ('_withImmunizationExample', 'Immunization-example'),
      ('_withImmunizationHistorical', 'Immunization-historical'),
      ('_withImmunizationNotGiven', 'Immunization-notGiven'),
      ('_withImmunizationProtocol', 'Immunization-protocol'),
      ('_withImmunizationSubpotent', 'Immunization-subpotent'),
  )
  def testJsonFormat_forValidImmunization_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, immunization_pb2.Immunization)

  @parameterized.named_parameters(
      ('_withImmunizationEvaluationExample', 'ImmunizationEvaluation-example'),
      ('_withImmunizationEvaluationNotValid',
       'ImmunizationEvaluation-notValid'),
  )
  def testJsonFormat_forValidImmunizationEvaluation_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, immunization_evaluation_pb2.ImmunizationEvaluation)

  @parameterized.named_parameters(
      ('_withImmunizationRecommendationExample',
       'ImmunizationRecommendation-example'),)
  def testJsonFormat_forValidImmunizationRecommendation_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, immunization_recommendation_pb2.ImmunizationRecommendation)

  @parameterized.named_parameters(
      # 'ImplementationGuide-fhir' and 'ig-r4' do not parse because they contain
      # a reference to an invalid resource.
      # https://gforge.hl7.org/gf/project/fhir/tracker/?action=TrackerItemEdit&tracker_item_id=22489
      # ('_withImplementationGuideFhir', 'ImplementationGuide-fhir'),
      # ('_withIgR4', 'ig-r4'),
      ('_withImplementationGuideExample', 'ImplementationGuide-example'),)
  def testJsonFormat_forValidImplementationGuide_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, implementation_guide_pb2.ImplementationGuide)

  @parameterized.named_parameters(
      ('_withInsurancePlanExample', 'InsurancePlan-example'),)
  def testJsonFormat_forValidInsurancePlan_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, insurance_plan_pb2.InsurancePlan)

  @parameterized.named_parameters(
      ('_withInvoiceExample', 'Invoice-example'),)
  def testJsonFormat_forValidInvoice_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   invoice_pb2.Invoice)

  @parameterized.named_parameters(
      ('_withLibraryCompositionExample', 'Library-composition-example'),
      ('_withLibraryExample', 'Library-example'),
      ('_withLibraryHivIndicators', 'Library-hiv-indicators'),
      ('_withLibraryLibraryCms146Example', 'Library-library-cms146-example'),
      ('_withLibraryLibraryExclusiveBreastfeedingCdsLogic',
       'Library-library-exclusive-breastfeeding-cds-logic'),
      ('_withLibraryLibraryExclusiveBreastfeedingCqmLogic',
       'Library-library-exclusive-breastfeeding-cqm-logic'),
      ('_withLibraryLibraryFhirHelpers', 'Library-library-fhir-helpers'),
      ('_withLibraryLibraryFhirHelpersPredecessor',
       'Library-library-fhir-helpers-predecessor'),
      ('_withLibraryLibraryFhirModelDefinition',
       'Library-library-fhir-model-definition'),
      ('_withLibraryLibraryQuickModelDefinition',
       'Library-library-quick-model-definition'),
      ('_withLibraryOmtkLogic', 'Library-omtk-logic'),
      ('_withLibraryOmtkModelinfo', 'Library-omtk-modelinfo'),
      ('_withLibraryOpioidcdsCommon', 'Library-opioidcds-common'),
      ('_withLibraryOpioidcdsRecommendation04',
       'Library-opioidcds-recommendation-04'),
      ('_withLibraryOpioidcdsRecommendation05',
       'Library-opioidcds-recommendation-05'),
      ('_withLibraryOpioidcdsRecommendation07',
       'Library-opioidcds-recommendation-07'),
      ('_withLibraryOpioidcdsRecommendation08',
       'Library-opioidcds-recommendation-08'),
      ('_withLibraryOpioidcdsRecommendation10',
       'Library-opioidcds-recommendation-10'),
      ('_withLibraryOpioidcdsRecommendation11',
       'Library-opioidcds-recommendation-11'),
      ('_withLibrarySuicideriskOrdersetLogic',
       'Library-suiciderisk-orderset-logic'),
      ('_withLibraryZikaVirusInterventionLogic',
       'Library-zika-virus-intervention-logic'),
  )
  def testJsonFormat_forValidLibrary_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   library_pb2.Library)

  @parameterized.named_parameters(
      ('_withLinkageExample', 'Linkage-example'),)
  def testJsonFormat_forValidLinkage_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   linkage_pb2.Linkage)

  @parameterized.named_parameters(
      ('_withListCurrentAllergies', 'List-current-allergies'),
      ('_withListExampleDoubleCousinRelationship',
       'List-example-double-cousin-relationship'),
      ('_withListExampleEmpty', 'List-example-empty'),
      ('_withListExample', 'List-example'),
      ('_withListExampleSimpleEmpty', 'List-example-simple-empty'),
      ('_withListF201', 'List-f201'),
      ('_withListGenetic', 'List-genetic'),
      ('_withListLong', 'List-long'),
      ('_withListMedList', 'List-med-list'),
      ('_withListPrognosis', 'List-prognosis'),
  )
  def testJsonFormat_forValidList_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, list_pb2.List)

  @parameterized.named_parameters(
      ('_withLocation1', 'Location-1'),
      ('_withLocation2', 'Location-2'),
      ('_withLocationAmb', 'Location-amb'),
      ('_withLocationHl7', 'Location-hl7'),
      ('_withLocationPh', 'Location-ph'),
      ('_withLocationUkp', 'Location-ukp'),
  )
  def testJsonFormat_forValidLocation_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   location_pb2.Location)

  @parameterized.named_parameters(
      ('_withMeasureComponentAExample', 'Measure-component-a-example'),
      ('_withMeasureComponentBExample', 'Measure-component-b-example'),
      ('_withMeasureCompositeExample', 'Measure-composite-example'),
      ('_withMeasureHivIndicators', 'Measure-hiv-indicators'),
      ('_withMeasureMeasureCms146Example', 'Measure-measure-cms146-example'),
      ('_withMeasureMeasureExclusiveBreastfeeding',
       'Measure-measure-exclusive-breastfeeding'),
      ('_withMeasureMeasurePredecessorExample',
       'Measure-measure-predecessor-example'),
  )
  def testJsonFormat_forValidMeasure_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   measure_pb2.Measure)

  @parameterized.named_parameters(
      ('_withMeasureReportHivIndicators', 'MeasureReport-hiv-indicators'),
      ('_withMeasureReportMeasureReportCms146Cat1Example',
       'MeasureReport-measurereport-cms146-cat1-example'),
      ('_withMeasureReportMeasureReportCms146Cat2Example',
       'MeasureReport-measurereport-cms146-cat2-example'),
      ('_withMeasureReportMeasureReportCms146Cat3Example',
       'MeasureReport-measurereport-cms146-cat3-example'),
  )
  def testJsonFormat_forValidMeasureReport_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, measure_report_pb2.MeasureReport)

  @parameterized.named_parameters(
      ('_withMedia1_2_840_11361907579238403408700_3_1_04_19970327150033',
       'Media-1.2.840.11361907579238403408700.3.1.04.19970327150033'),
      ('_withMediaExample', 'Media-example'),
      ('_withMediaSound', 'Media-sound'),
      ('_withMediaXray', 'Media-xray'),
  )
  def testJsonFormat_forValidMedia_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, media_pb2.Media)

  @parameterized.named_parameters(
      ('_withMedicationMed0301', 'Medication-med0301'),
      ('_withMedicationMed0302', 'Medication-med0302'),
      ('_withMedicationMed0303', 'Medication-med0303'),
      ('_withMedicationMed0304', 'Medication-med0304'),
      ('_withMedicationMed0305', 'Medication-med0305'),
      ('_withMedicationMed0306', 'Medication-med0306'),
      ('_withMedicationMed0307', 'Medication-med0307'),
      ('_withMedicationMed0308', 'Medication-med0308'),
      ('_withMedicationMed0309', 'Medication-med0309'),
      ('_withMedicationMed0310', 'Medication-med0310'),
      ('_withMedicationMed0311', 'Medication-med0311'),
      ('_withMedicationMed0312', 'Medication-med0312'),
      ('_withMedicationMed0313', 'Medication-med0313'),
      ('_withMedicationMed0314', 'Medication-med0314'),
      ('_withMedicationMed0315', 'Medication-med0315'),
      ('_withMedicationMed0316', 'Medication-med0316'),
      ('_withMedicationMed0317', 'Medication-med0317'),
      ('_withMedicationMed0318', 'Medication-med0318'),
      ('_withMedicationMed0319', 'Medication-med0319'),
      ('_withMedicationMed0320', 'Medication-med0320'),
      ('_withMedicationMed0321', 'Medication-med0321'),
      ('_withMedicationMedexample015', 'Medication-medexample015'),
      ('_withMedicationMedicationexample1', 'Medication-medicationexample1'),
  )
  def testJsonFormat_forValidMedication_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   medication_pb2.Medication)

  @parameterized.named_parameters(
      ('_withMedicationAdministrationMedadmin0301',
       'MedicationAdministration-medadmin0301'),
      ('_withMedicationAdministrationMedadmin0302',
       'MedicationAdministration-medadmin0302'),
      ('_withMedicationAdministrationMedadmin0303',
       'MedicationAdministration-medadmin0303'),
      ('_withMedicationAdministrationMedadmin0304',
       'MedicationAdministration-medadmin0304'),
      ('_withMedicationAdministrationMedadmin0305',
       'MedicationAdministration-medadmin0305'),
      ('_withMedicationAdministrationMedadmin0306',
       'MedicationAdministration-medadmin0306'),
      ('_withMedicationAdministrationMedadmin0307',
       'MedicationAdministration-medadmin0307'),
      ('_withMedicationAdministrationMedadmin0308',
       'MedicationAdministration-medadmin0308'),
      ('_withMedicationAdministrationMedadmin0309',
       'MedicationAdministration-medadmin0309'),
      ('_withMedicationAdministrationMedadmin0310',
       'MedicationAdministration-medadmin0310'),
      ('_withMedicationAdministrationMedadmin0311',
       'MedicationAdministration-medadmin0311'),
      ('_withMedicationAdministrationMedadmin0312',
       'MedicationAdministration-medadmin0312'),
      ('_withMedicationAdministrationMedadmin0313',
       'MedicationAdministration-medadmin0313'),
      ('_withMedicationAdministrationMedadminexample03',
       'MedicationAdministration-medadminexample03'),
  )
  def testJsonFormat_forValidMedicationAdministration_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medication_administration_pb2.MedicationAdministration)

  @parameterized.named_parameters(
      ('_withMedicationDispenseMeddisp008', 'MedicationDispense-meddisp008'),
      ('_withMedicationDispenseMeddisp0301', 'MedicationDispense-meddisp0301'),
      ('_withMedicationDispenseMeddisp0302', 'MedicationDispense-meddisp0302'),
      ('_withMedicationDispenseMeddisp0303', 'MedicationDispense-meddisp0303'),
      ('_withMedicationDispenseMeddisp0304', 'MedicationDispense-meddisp0304'),
      ('_withMedicationDispenseMeddisp0305', 'MedicationDispense-meddisp0305'),
      ('_withMedicationDispenseMeddisp0306', 'MedicationDispense-meddisp0306'),
      ('_withMedicationDispenseMeddisp0307', 'MedicationDispense-meddisp0307'),
      ('_withMedicationDispenseMeddisp0308', 'MedicationDispense-meddisp0308'),
      ('_withMedicationDispenseMeddisp0309', 'MedicationDispense-meddisp0309'),
      ('_withMedicationDispenseMeddisp0310', 'MedicationDispense-meddisp0310'),
      ('_withMedicationDispenseMeddisp0311', 'MedicationDispense-meddisp0311'),
      ('_withMedicationDispenseMeddisp0312', 'MedicationDispense-meddisp0312'),
      ('_withMedicationDispenseMeddisp0313', 'MedicationDispense-meddisp0313'),
      ('_withMedicationDispenseMeddisp0314', 'MedicationDispense-meddisp0314'),
      ('_withMedicationDispenseMeddisp0315', 'MedicationDispense-meddisp0315'),
      ('_withMedicationDispenseMeddisp0316', 'MedicationDispense-meddisp0316'),
      ('_withMedicationDispenseMeddisp0317', 'MedicationDispense-meddisp0317'),
      ('_withMedicationDispenseMeddisp0318', 'MedicationDispense-meddisp0318'),
      ('_withMedicationDispenseMeddisp0319', 'MedicationDispense-meddisp0319'),
      ('_withMedicationDispenseMeddisp0320', 'MedicationDispense-meddisp0320'),
      ('_withMedicationDispenseMeddisp0321', 'MedicationDispense-meddisp0321'),
      ('_withMedicationDispenseMeddisp0322', 'MedicationDispense-meddisp0322'),
      ('_withMedicationDispenseMeddisp0324', 'MedicationDispense-meddisp0324'),
      ('_withMedicationDispenseMeddisp0325', 'MedicationDispense-meddisp0325'),
      ('_withMedicationDispenseMeddisp0326', 'MedicationDispense-meddisp0326'),
      ('_withMedicationDispenseMeddisp0327', 'MedicationDispense-meddisp0327'),
      ('_withMedicationDispenseMeddisp0328', 'MedicationDispense-meddisp0328'),
      ('_withMedicationDispenseMeddisp0329', 'MedicationDispense-meddisp0329'),
      ('_withMedicationDispenseMeddisp0330', 'MedicationDispense-meddisp0330'),
      ('_withMedicationDispenseMeddisp0331', 'MedicationDispense-meddisp0331'),
  )
  def testJsonFormat_forValidMedicationDispense_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medication_dispense_pb2.MedicationDispense)

  @parameterized.named_parameters(
      ('_withMedicationKnowledgeExample', 'MedicationKnowledge-example'),)
  def testJsonFormat_forValidMedicationKnowledge_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medication_knowledge_pb2.MedicationKnowledge)

  @parameterized.named_parameters(
      ('_withMedicationRequestMedrx002', 'MedicationRequest-medrx002'),
      ('_withMedicationRequestMedrx0301', 'MedicationRequest-medrx0301'),
      ('_withMedicationRequestMedrx0302', 'MedicationRequest-medrx0302'),
      ('_withMedicationRequestMedrx0303', 'MedicationRequest-medrx0303'),
      ('_withMedicationRequestMedrx0304', 'MedicationRequest-medrx0304'),
      ('_withMedicationRequestMedrx0305', 'MedicationRequest-medrx0305'),
      ('_withMedicationRequestMedrx0306', 'MedicationRequest-medrx0306'),
      ('_withMedicationRequestMedrx0307', 'MedicationRequest-medrx0307'),
      ('_withMedicationRequestMedrx0308', 'MedicationRequest-medrx0308'),
      ('_withMedicationRequestMedrx0309', 'MedicationRequest-medrx0309'),
      ('_withMedicationRequestMedrx0310', 'MedicationRequest-medrx0310'),
      ('_withMedicationRequestMedrx0311', 'MedicationRequest-medrx0311'),
      ('_withMedicationRequestMedrx0312', 'MedicationRequest-medrx0312'),
      ('_withMedicationRequestMedrx0313', 'MedicationRequest-medrx0313'),
      ('_withMedicationRequestMedrx0314', 'MedicationRequest-medrx0314'),
      ('_withMedicationRequestMedrx0315', 'MedicationRequest-medrx0315'),
      ('_withMedicationRequestMedrx0316', 'MedicationRequest-medrx0316'),
      ('_withMedicationRequestMedrx0317', 'MedicationRequest-medrx0317'),
      ('_withMedicationRequestMedrx0318', 'MedicationRequest-medrx0318'),
      ('_withMedicationRequestMedrx0319', 'MedicationRequest-medrx0319'),
      ('_withMedicationRequestMedrx0320', 'MedicationRequest-medrx0320'),
      ('_withMedicationRequestMedrx0321', 'MedicationRequest-medrx0321'),
      ('_withMedicationRequestMedrx0322', 'MedicationRequest-medrx0322'),
      ('_withMedicationRequestMedrx0323', 'MedicationRequest-medrx0323'),
      ('_withMedicationRequestMedrx0324', 'MedicationRequest-medrx0324'),
      ('_withMedicationRequestMedrx0325', 'MedicationRequest-medrx0325'),
      ('_withMedicationRequestMedrx0326', 'MedicationRequest-medrx0326'),
      ('_withMedicationRequestMedrx0327', 'MedicationRequest-medrx0327'),
      ('_withMedicationRequestMedrx0328', 'MedicationRequest-medrx0328'),
      ('_withMedicationRequestMedrx0329', 'MedicationRequest-medrx0329'),
      ('_withMedicationRequestMedrx0330', 'MedicationRequest-medrx0330'),
      ('_withMedicationRequestMedrx0331', 'MedicationRequest-medrx0331'),
      ('_withMedicationRequestMedrx0332', 'MedicationRequest-medrx0332'),
      ('_withMedicationRequestMedrx0333', 'MedicationRequest-medrx0333'),
      ('_withMedicationRequestMedrx0334', 'MedicationRequest-medrx0334'),
      ('_withMedicationRequestMedrx0335', 'MedicationRequest-medrx0335'),
      ('_withMedicationRequestMedrx0336', 'MedicationRequest-medrx0336'),
      ('_withMedicationRequestMedrx0337', 'MedicationRequest-medrx0337'),
      ('_withMedicationRequestMedrx0338', 'MedicationRequest-medrx0338'),
      ('_withMedicationRequestMedrx0339', 'MedicationRequest-medrx0339'),
  )
  def testJsonFormat_forValidMedicationRequest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medication_request_pb2.MedicationRequest)

  @parameterized.named_parameters(
      ('_withMedicationStatementExample001', 'MedicationStatement-example001'),
      ('_withMedicationStatementExample002', 'MedicationStatement-example002'),
      ('_withMedicationStatementExample003', 'MedicationStatement-example003'),
      ('_withMedicationStatementExample004', 'MedicationStatement-example004'),
      ('_withMedicationStatementExample005', 'MedicationStatement-example005'),
      ('_withMedicationStatementExample006', 'MedicationStatement-example006'),
      ('_withMedicationStatementExample007', 'MedicationStatement-example007'),
  )
  def testJsonFormat_forValidMedicationStatement_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medication_statement_pb2.MedicationStatement)

  @parameterized.named_parameters(
      ('_withMedicinalProductExample', 'MedicinalProduct-example'),)
  def testJsonFormat_forValidMedicinalProduct_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medicinal_product_pb2.MedicinalProduct)

  @parameterized.named_parameters(
      ('_withMedicinalProductAuthorizationExample',
       'MedicinalProductAuthorization-example'),)
  def testJsonFormat_forValidMedicinalProductAuthorization_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name,
        medicinal_product_authorization_pb2.MedicinalProductAuthorization)

  @parameterized.named_parameters(
      ('_withMedicinalProductContraindicationExample',
       'MedicinalProductContraindication-example'),)
  def testJsonFormat_forValidMedicinalProductContraindication_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name,
        medicinal_product_contraindication_pb2.MedicinalProductContraindication)

  @parameterized.named_parameters(
      ('_withMedicinalProductIndicationExample',
       'MedicinalProductIndication-example'),)
  def testJsonFormat_forValidMedicinalProductIndication_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medicinal_product_indication_pb2.MedicinalProductIndication)

  @parameterized.named_parameters(
      ('_withMedicinalProductIngredientExample',
       'MedicinalProductIngredient-example'),)
  def testJsonFormat_forValidMedicinalProductIngredient_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medicinal_product_ingredient_pb2.MedicinalProductIngredient)

  @parameterized.named_parameters(
      ('_withMedicinalProductInteractionExample',
       'MedicinalProductInteraction-example'),)
  def testJsonFormat_forValidMedicinalProductInteraction_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name,
        medicinal_product_interaction_pb2.MedicinalProductInteraction)

  @parameterized.named_parameters(
      ('_withMedicinalProductManufacturedExample',
       'MedicinalProductManufactured-example'),)
  def testJsonFormat_forValidMedicinalProductManufactured_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name,
        medicinal_product_manufactured_pb2.MedicinalProductManufactured)

  @parameterized.named_parameters(
      ('_withMedicinalProductPackagedExample',
       'MedicinalProductPackaged-example'),)
  def testJsonFormat_forValidMedicinalProductPackaged_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medicinal_product_packaged_pb2.MedicinalProductPackaged)

  @parameterized.named_parameters(
      ('_withMedicinalProductPharmaceuticalExample',
       'MedicinalProductPharmaceutical-example'),)
  def testJsonFormat_forValidMedicinalProductPharmaceutical_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name,
        medicinal_product_pharmaceutical_pb2.MedicinalProductPharmaceutical)

  @parameterized.named_parameters(
      ('_withMedicinalProductUndesirableEffectExample',
       'MedicinalProductUndesirableEffect-example'),)
  def testJsonFormat_forValidMedicinalProductUndesirableEffect_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, medicinal_product_undesirable_effect_pb2
        .MedicinalProductUndesirableEffect)

  @parameterized.named_parameters(
      ('_withMessageDefinitionExample', 'MessageDefinition-example'),
      ('_withMessageDefinitionPatientLinkNotification',
       'MessageDefinition-patient-link-notification'),
      ('_withMessageDefinitionPatientLinkResponse',
       'MessageDefinition-patient-link-response'),
  )
  def testJsonFormat_forValidMessageDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, message_definition_pb2.MessageDefinition)

  @parameterized.named_parameters(
      ('_withMessageHeader1cbdfb97585948a48301D54eab818d68',
       'MessageHeader-1cbdfb97-5859-48a4-8301-d54eab818d68'),)
  def testJsonFormat_forValidMessageHeader_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, message_header_pb2.MessageHeader)

  @parameterized.named_parameters(
      ('_withMolecularSequenceBreastcancer', 'MolecularSequence-breastcancer'),
      ('_withMolecularSequenceCoord0Base', 'MolecularSequence-coord-0-base'),
      ('_withMolecularSequenceCoord1Base', 'MolecularSequence-coord-1-base'),
      ('_withMolecularSequenceExample', 'MolecularSequence-example'),
      ('_withMolecularSequenceExamplePgx1', 'MolecularSequence-example-pgx-1'),
      ('_withMolecularSequenceExamplePgx2', 'MolecularSequence-example-pgx-2'),
      ('_withMolecularSequenceExampleTpmtOne',
       'MolecularSequence-example-TPMT-one'),
      ('_withMolecularSequenceExampleTpmtTwo',
       'MolecularSequence-example-TPMT-two'),
      ('_withMolecularSequenceFdaExample', 'MolecularSequence-fda-example'),
      ('_withMolecularSequenceFdaVcfComparison',
       'MolecularSequence-fda-vcf-comparison'),
      ('_withMolecularSequenceFdaVcfevalComparison',
       'MolecularSequence-fda-vcfeval-comparison'),
      ('_withMolecularSequenceGraphicExample1',
       'MolecularSequence-graphic-example-1'),
      ('_withMolecularSequenceGraphicExample2',
       'MolecularSequence-graphic-example-2'),
      ('_withMolecularSequenceGraphicExample3',
       'MolecularSequence-graphic-example-3'),
      ('_withMolecularSequenceGraphicExample4',
       'MolecularSequence-graphic-example-4'),
      ('_withMolecularSequenceGraphicExample5',
       'MolecularSequence-graphic-example-5'),
      ('_withMolecularSequenceSequenceComplexVariant',
       'MolecularSequence-sequence-complex-variant'),
  )
  def testJsonFormat_forValidMolecularSequence_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, molecular_sequence_pb2.MolecularSequence)

  @parameterized.named_parameters(
      ('_withNamingSystemExampleId', 'NamingSystem-example-id'),
      ('_withNamingSystemExample', 'NamingSystem-example'),
  )
  def testJsonFormat_forValidNamingSystem_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, naming_system_pb2.NamingSystem)

  @parameterized.named_parameters(
      ('_withNutritionOrderCardiacdiet', 'NutritionOrder-cardiacdiet'),
      ('_withNutritionOrderDiabeticdiet', 'NutritionOrder-diabeticdiet'),
      ('_withNutritionOrderDiabeticSupplement',
       'NutritionOrder-diabeticsupplement'),
      ('_withNutritionOrderEnergySupplement',
       'NutritionOrder-energysupplement'),
      ('_withNutritionOrderEnteralbolus', 'NutritionOrder-enteralbolus'),
      ('_withNutritionOrderEnteralcontinuous',
       'NutritionOrder-enteralcontinuous'),
      ('_withNutritionOrderFiberrestricteddiet',
       'NutritionOrder-fiberrestricteddiet'),
      ('_withNutritionOrderInfantEnteral', 'NutritionOrder-infantenteral'),
      ('_withNutritionOrderProteinSupplement',
       'NutritionOrder-proteinsupplement'),
      ('_withNutritionOrderPureedDiet', 'NutritionOrder-pureeddiet'),
      ('_withNutritionOrderPureedDietSimple',
       'NutritionOrder-pureeddiet-simple'),
      ('_withNutritionOrderRenalDiet', 'NutritionOrder-renaldiet'),
      ('_withNutritionOrderTextureModified', 'NutritionOrder-texturemodified'),
  )
  def testJsonFormat_forValidNutritionOrder_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, nutrition_order_pb2.NutritionOrder)

  @parameterized.named_parameters(
      ('_withObservation10MinuteApgarScore',
       'Observation-10minute-apgar-score'),
      ('_withObservation1MinuteApgarScore', 'Observation-1minute-apgar-score'),
      ('_withObservation20MinuteApgarScore',
       'Observation-20minute-apgar-score'),
      ('_withObservation2MinuteApgarScore', 'Observation-2minute-apgar-score'),
      ('_withObservation5MinuteApgarScore', 'Observation-5minute-apgar-score'),
      ('_withObservation656', 'Observation-656'),
      ('_withObservationAbdoTender', 'Observation-abdo-tender'),
      ('_withObservationAlcoholType', 'Observation-alcohol-type'),
      ('_withObservationBgpanel', 'Observation-bgpanel'),
      ('_withObservationBloodgroup', 'Observation-bloodgroup'),
      ('_withObservationBloodPressureCancel',
       'Observation-blood-pressure-cancel'),
      ('_withObservationBloodPressureDar', 'Observation-blood-pressure-dar'),
      ('_withObservationBloodPressure', 'Observation-blood-pressure'),
      ('_withObservationBmd', 'Observation-bmd'),
      ('_withObservationBmi', 'Observation-bmi'),
      ('_withObservationBmiUsingRelated', 'Observation-bmi-using-related'),
      ('_withObservationBodyHeight', 'Observation-body-height'),
      ('_withObservationBodyLength', 'Observation-body-length'),
      ('_withObservationBodyTemperature', 'Observation-body-temperature'),
      ('_withObservationClinicalGender', 'Observation-clinical-gender'),
      ('_withObservationDateLastmp', 'Observation-date-lastmp'),
      ('_withObservationDecimal', 'Observation-decimal'),
      ('_withObservationEkg', 'Observation-ekg'),
      ('_withObservationExampleDiplotype1', 'Observation-example-diplotype1'),
      ('_withObservationExampleGenetics1', 'Observation-example-genetics-1'),
      ('_withObservationExampleGenetics2', 'Observation-example-genetics-2'),
      ('_withObservationExampleGenetics3', 'Observation-example-genetics-3'),
      ('_withObservationExampleGenetics4', 'Observation-example-genetics-4'),
      ('_withObservationExampleGenetics5', 'Observation-example-genetics-5'),
      ('_withObservationExampleGeneticsBrcapat',
       'Observation-example-genetics-brcapat'),
      ('_withObservationExampleHaplotype1', 'Observation-example-haplotype1'),
      ('_withObservationExampleHaplotype2', 'Observation-example-haplotype2'),
      ('_withObservationExample', 'Observation-example'),
      ('_withObservationExamplePhenotype', 'Observation-example-phenotype'),
      ('_withObservationExampleTpmtDiplotype',
       'Observation-example-TPMT-diplotype'),
      ('_withObservationExampleTpmtHaplotypeOne',
       'Observation-example-TPMT-haplotype-one'),
      ('_withObservationExampleTpmtHaplotypeTwo',
       'Observation-example-TPMT-haplotype-two'),
      ('_withObservationEyeColor', 'Observation-eye-color'),
      ('_withObservationF001', 'Observation-f001'),
      ('_withObservationF002', 'Observation-f002'),
      ('_withObservationF003', 'Observation-f003'),
      ('_withObservationF004', 'Observation-f004'),
      ('_withObservationF005', 'Observation-f005'),
      ('_withObservationF202', 'Observation-f202'),
      ('_withObservationF203', 'Observation-f203'),
      ('_withObservationF204', 'Observation-f204'),
      ('_withObservationF205', 'Observation-f205'),
      ('_withObservationF206', 'Observation-f206'),
      ('_withObservationGcsQa', 'Observation-gcs-qa'),
      ('_withObservationGlasgow', 'Observation-glasgow'),
      ('_withObservationHeadCircumference', 'Observation-head-circumference'),
      ('_withObservationHeartRate', 'Observation-heart-rate'),
      ('_withObservationHerd1', 'Observation-herd1'),
      ('_withObservationMapSitting', 'Observation-map-sitting'),
      ('_withObservationMbp', 'Observation-mbp'),
      ('_withObservationRespiratoryRate', 'Observation-respiratory-rate'),
      ('_withObservationRhstatus', 'Observation-rhstatus'),
      ('_withObservationSatO2', 'Observation-satO2'),
      ('_withObservationSecondSmoke', 'Observation-secondsmoke'),
      ('_withObservationTrachCare', 'Observation-trachcare'),
      ('_withObservationUnsat', 'Observation-unsat'),
      ('_withObservationVitalsPanel', 'Observation-vitals-panel'),
      ('_withObservationVomiting', 'Observation-vomiting'),
      ('_withObservationVpOyster', 'Observation-vp-oyster'),
  )
  def testJsonFormat_forValidObservation_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   observation_pb2.Observation)

  @parameterized.named_parameters(
      ('_withObservationDefinitionExample', 'ObservationDefinition-example'),)
  def testJsonFormat_forValidObservationDefinition_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, observation_definition_pb2.ObservationDefinition)

  @parameterized.named_parameters(
      ('_withOperationDefinitionActivityDefinitionApply',
       'OperationDefinition-ActivityDefinition-apply'),
      ('_withOperationDefinitionActivityDefinitionDataRequirements',
       'OperationDefinition-ActivityDefinition-data-requirements'),
      ('_withOperationDefinitionCapabilityStatementConforms',
       'OperationDefinition-CapabilityStatement-conforms'),
      ('_withOperationDefinitionCapabilityStatementImplements',
       'OperationDefinition-CapabilityStatement-implements'),
      ('_withOperationDefinitionCapabilityStatementSubset',
       'OperationDefinition-CapabilityStatement-subset'),
      ('_withOperationDefinitionCapabilityStatementVersions',
       'OperationDefinition-CapabilityStatement-versions'),
      ('_withOperationDefinitionChargeItemDefinitionApply',
       'OperationDefinition-ChargeItemDefinition-apply'),
      ('_withOperationDefinitionClaimSubmit',
       'OperationDefinition-Claim-submit'),
      ('_withOperationDefinitionCodeSystemFindMatches',
       'OperationDefinition-CodeSystem-find-matches'),
      ('_withOperationDefinitionCodeSystemLookup',
       'OperationDefinition-CodeSystem-lookup'),
      ('_withOperationDefinitionCodeSystemSubsumes',
       'OperationDefinition-CodeSystem-subsumes'),
      ('_withOperationDefinitionCodeSystemValidateCode',
       'OperationDefinition-CodeSystem-validate-code'),
      ('_withOperationDefinitionCompositionDocument',
       'OperationDefinition-Composition-document'),
      ('_withOperationDefinitionConceptMapClosure',
       'OperationDefinition-ConceptMap-closure'),
      ('_withOperationDefinitionConceptMapTranslate',
       'OperationDefinition-ConceptMap-translate'),
      ('_withOperationDefinitionCoverageEligibilityRequestSubmit',
       'OperationDefinition-CoverageEligibilityRequest-submit'),
      ('_withOperationDefinitionEncounterEverything',
       'OperationDefinition-Encounter-everything'),
      ('_withOperationDefinitionExample', 'OperationDefinition-example'),
      ('_withOperationDefinitionGroupEverything',
       'OperationDefinition-Group-everything'),
      ('_withOperationDefinitionLibraryDataRequirements',
       'OperationDefinition-Library-data-requirements'),
      ('_withOperationDefinitionListFind', 'OperationDefinition-List-find'),
      ('_withOperationDefinitionMeasureCareGaps',
       'OperationDefinition-Measure-care-gaps'),
      ('_withOperationDefinitionMeasureCollectData',
       'OperationDefinition-Measure-collect-data'),
      ('_withOperationDefinitionMeasureDataRequirements',
       'OperationDefinition-Measure-data-requirements'),
      ('_withOperationDefinitionMeasureEvaluateMeasure',
       'OperationDefinition-Measure-evaluate-measure'),
      ('_withOperationDefinitionMeasureSubmitData',
       'OperationDefinition-Measure-submit-data'),
      ('_withOperationDefinitionMedicinalProductEverything',
       'OperationDefinition-MedicinalProduct-everything'),
      ('_withOperationDefinitionMessageHeaderProcessMessage',
       'OperationDefinition-MessageHeader-process-message'),
      ('_withOperationDefinitionNamingSystemPreferredId',
       'OperationDefinition-NamingSystem-preferred-id'),
      ('_withOperationDefinitionObservationLastn',
       'OperationDefinition-Observation-lastn'),
      ('_withOperationDefinitionObservationStats',
       'OperationDefinition-Observation-stats'),
      ('_withOperationDefinitionPatientEverything',
       'OperationDefinition-Patient-everything'),
      ('_withOperationDefinitionPatientMatch',
       'OperationDefinition-Patient-match'),
      ('_withOperationDefinitionPlanDefinitionApply',
       'OperationDefinition-PlanDefinition-apply'),
      ('_withOperationDefinitionPlanDefinitionDataRequirements',
       'OperationDefinition-PlanDefinition-data-requirements'),
      ('_withOperationDefinitionResourceConvert',
       'OperationDefinition-Resource-convert'),
      ('_withOperationDefinitionResourceGraph',
       'OperationDefinition-Resource-graph'),
      ('_withOperationDefinitionResourceGraphql',
       'OperationDefinition-Resource-graphql'),
      ('_withOperationDefinitionResourceMetaAdd',
       'OperationDefinition-Resource-meta-add'),
      ('_withOperationDefinitionResourceMetaDelete',
       'OperationDefinition-Resource-meta-delete'),
      ('_withOperationDefinitionResourceMeta',
       'OperationDefinition-Resource-meta'),
      ('_withOperationDefinitionResourceValidate',
       'OperationDefinition-Resource-validate'),
      ('_withOperationDefinitionStructureDefinitionQuestionnaire',
       'OperationDefinition-StructureDefinition-questionnaire'),
      ('_withOperationDefinitionStructureDefinitionSnapshot',
       'OperationDefinition-StructureDefinition-snapshot'),
      ('_withOperationDefinitionStructureMapTransform',
       'OperationDefinition-StructureMap-transform'),
      ('_withOperationDefinitionValueSetExpand',
       'OperationDefinition-ValueSet-expand'),
      ('_withOperationDefinitionValueSetValidateCode',
       'OperationDefinition-ValueSet-validate-code'),
  )
  def testJsonFormat_forValidOperationDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, operation_definition_pb2.OperationDefinition)

  @parameterized.named_parameters(
      ('_withOperationOutcome101', 'OperationOutcome-101'),
      ('_withOperationOutcomeAllok', 'OperationOutcome-allok'),
      ('_withOperationOutcomeBreakTheGlass',
       'OperationOutcome-break-the-glass'),
      ('_withOperationOutcomeException', 'OperationOutcome-exception'),
      ('_withOperationOutcomeSearchfail', 'OperationOutcome-searchfail'),
      ('_withOperationOutcomeValidationfail',
       'OperationOutcome-validationfail'),
  )
  def testJsonFormat_forValidOperationOutcome_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, operation_outcome_pb2.OperationOutcome)

  @parameterized.named_parameters(
      ('_withOrganization1832473e2fe0452dAbe93cdb9879522f',
       'Organization-1832473e-2fe0-452d-abe9-3cdb9879522f'),
      ('_withOrganization1', 'Organization-1'),
      ('_withOrganization2.16.840.1.113883.19.5',
       'Organization-2.16.840.1.113883.19.5'),
      ('_withOrganization2', 'Organization-2'),
      ('_withOrganization3', 'Organization-3'),
      ('_withOrganizationF001', 'Organization-f001'),
      ('_withOrganizationF002', 'Organization-f002'),
      ('_withOrganizationF003', 'Organization-f003'),
      ('_withOrganizationF201', 'Organization-f201'),
      ('_withOrganizationF203', 'Organization-f203'),
      ('_withOrganizationHl7', 'Organization-hl7'),
      ('_withOrganizationHl7Pay', 'Organization-hl7pay'),
      ('_withOrganizationMmanu', 'Organization-mmanu'),
  )
  def testJsonFormat_forValidOrganization_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, organization_pb2.Organization)

  @parameterized.named_parameters(
      ('_withOrganizationAffiliationExample',
       'OrganizationAffiliation-example'),
      ('_withOrganizationAffiliationOrgrole1',
       'OrganizationAffiliation-orgrole1'),
      ('_withOrganizationAffiliationOrgrole2',
       'OrganizationAffiliation-orgrole2'),
  )
  def testJsonFormat_forValidOrganizationAffiliation_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, organization_affiliation_pb2.OrganizationAffiliation)

  @parameterized.named_parameters(
      ('_withPatientAnimal', 'Patient-animal'),
      ('_withPatientChExample', 'Patient-ch-example'),
      ('_withPatientDicom', 'Patient-dicom'),
      ('_withPatientExample', 'Patient-example'),
      ('_withPatientF001', 'Patient-f001'),
      ('_withPatientF201', 'Patient-f201'),
      ('_withPatientGeneticsExample1', 'Patient-genetics-example1'),
      ('_withPatientGlossy', 'Patient-glossy'),
      ('_withPatientIhePcd', 'Patient-ihe-pcd'),
      ('_withPatientInfantFetal', 'Patient-infant-fetal'),
      ('_withPatientInfantMom', 'Patient-infant-mom'),
      ('_withPatientInfantTwin1', 'Patient-infant-twin-1'),
      ('_withPatientInfantTwin2', 'Patient-infant-twin-2'),
      ('_withPatientMom', 'Patient-mom'),
      ('_withPatientNewborn', 'Patient-newborn'),
      ('_withPatientPat1', 'Patient-pat1'),
      ('_withPatientPat2', 'Patient-pat2'),
      ('_withPatientPat3', 'Patient-pat3'),
      ('_withPatientPat4', 'Patient-pat4'),
      ('_withPatientProband', 'Patient-proband'),
      ('_withPatientXcda', 'Patient-xcda'),
      ('_withPatientXds', 'Patient-xds'),
  )
  def testJsonFormat_forValidPatient_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   patient_pb2.Patient)

  @parameterized.named_parameters(
      ('_withPaymentNotice77654', 'PaymentNotice-77654'),)
  def testJsonFormat_forValidPaymentNotice_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, payment_notice_pb2.PaymentNotice)

  @parameterized.named_parameters(
      ('_withPaymentReconciliationER2500', 'PaymentReconciliation-ER2500'),)
  def testJsonFormat_forValidPaymentReconciliation_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, payment_reconciliation_pb2.PaymentReconciliation)

  @parameterized.named_parameters(
      ('_withPersonExample', 'Person-example'),
      ('_withPersonF002', 'Person-f002'),
      ('_withPersonGrahame', 'Person-grahame'),
      ('_withPersonPd', 'Person-pd'),
      ('_withPersonPp', 'Person-pp'),
  )
  def testJsonFormat_forValidPerson_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, person_pb2.Person)

  @parameterized.named_parameters(
      ('_withPlanDefinitionChlamydiaScreeningIntervention',
       'PlanDefinition-chlamydia-screening-intervention'),
      ('_withPlanDefinitionExampleCardiologyOs',
       'PlanDefinition-example-cardiology-os'),
      ('_withPlanDefinitionExclusiveBreastfeedingIntervention01',
       'PlanDefinition-exclusive-breastfeeding-intervention-01'),
      ('_withPlanDefinitionExclusiveBreastfeedingIntervention02',
       'PlanDefinition-exclusive-breastfeeding-intervention-02'),
      ('_withPlanDefinitionExclusiveBreastfeedingIntervention03',
       'PlanDefinition-exclusive-breastfeeding-intervention-03'),
      ('_withPlanDefinitionExclusiveBreastfeedingIntervention04',
       'PlanDefinition-exclusive-breastfeeding-intervention-04'),
      ('_withPlanDefinitionKDN5', 'PlanDefinition-KDN5'),
      ('_withPlanDefinitionLowSuicideRiskOrderSet',
       'PlanDefinition-low-suicide-risk-order-set'),
      ('_withPlanDefinitionOpioidcds04', 'PlanDefinition-opioidcds-04'),
      ('_withPlanDefinitionOpioidcds05', 'PlanDefinition-opioidcds-05'),
      ('_withPlanDefinitionOpioidcds07', 'PlanDefinition-opioidcds-07'),
      ('_withPlanDefinitionOpioidcds08', 'PlanDefinition-opioidcds-08'),
      ('_withPlanDefinitionOpioidcds10', 'PlanDefinition-opioidcds-10'),
      ('_withPlanDefinitionOpioidcds11', 'PlanDefinition-opioidcds-11'),
      ('_withPlanDefinitionOptionsExample', 'PlanDefinition-options-example'),
      ('_withPlanDefinitionProtocolExample', 'PlanDefinition-protocol-example'),
      ('_withPlanDefinitionZikaVirusInterventionInitial',
       'PlanDefinition-zika-virus-intervention-initial'),
      ('_withPlanDefinitionZikaVirusIntervention',
       'PlanDefinition-zika-virus-intervention'),
  )
  def testJsonFormat_forValidPlanDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, plan_definition_pb2.PlanDefinition)

  @parameterized.named_parameters(
      ('_withPractitionerExample', 'Practitioner-example'),
      ('_withPractitionerF001', 'Practitioner-f001'),
      ('_withPractitionerF002', 'Practitioner-f002'),
      ('_withPractitionerF003', 'Practitioner-f003'),
      ('_withPractitionerF004', 'Practitioner-f004'),
      ('_withPractitionerF005', 'Practitioner-f005'),
      ('_withPractitionerF006', 'Practitioner-f006'),
      ('_withPractitionerF007', 'Practitioner-f007'),
      ('_withPractitionerF201', 'Practitioner-f201'),
      ('_withPractitionerF202', 'Practitioner-f202'),
      ('_withPractitionerF203', 'Practitioner-f203'),
      ('_withPractitionerF204', 'Practitioner-f204'),
      ('_withPractitionerXcda1', 'Practitioner-xcda1'),
      ('_withPractitionerXcdaAuthor', 'Practitioner-xcda-author'),
  )
  def testJsonFormat_forValidPractitioner_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, practitioner_pb2.Practitioner)

  @parameterized.named_parameters(
      ('_withPractitionerRoleExample', 'PractitionerRole-example'),)
  def testJsonFormat_forValidPractitionerRole_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, practitioner_role_pb2.PractitionerRole)

  @parameterized.named_parameters(
      ('_withBase64Binary', 'base64_binary', datatypes_pb2.Base64Binary),
      ('_withBoolean', 'boolean', datatypes_pb2.Boolean),
      ('_withCanonical', 'canonical', datatypes_pb2.Canonical),
      ('_withCode', 'code', datatypes_pb2.Code),
      ('_withDate', 'date', datatypes_pb2.Date),
      ('_withDateTime', 'date_time', datatypes_pb2.DateTime),
      ('_withDecimal', 'decimal', datatypes_pb2.Decimal),
      ('_withId', 'id', datatypes_pb2.Id),
      ('_withInstant', 'instant', datatypes_pb2.Instant),
      ('_withInteger', 'integer', datatypes_pb2.Integer),
      ('_withMarkdown', 'markdown', datatypes_pb2.Markdown),
      ('_withOid', 'oid', datatypes_pb2.Oid),
      ('_withPositiveInt', 'positive_int', datatypes_pb2.PositiveInt),
      ('_withString', 'string', datatypes_pb2.String),
      ('_withTime', 'time', datatypes_pb2.Time),
      ('_withUnsignedInt', 'unsigned_int', datatypes_pb2.UnsignedInt),
      ('_withUri', 'uri', datatypes_pb2.Uri),
      ('_withUrl', 'url', datatypes_pb2.Url),
      ('_withUuid', 'uuid', datatypes_pb2.Uuid),
      ('_withXhtml', 'xhtml', datatypes_pb2.Xhtml),
  )
  def testJsonFormat_forValidPrimitive_succeeds(
      self, file_name: str, primitive_cls: Type[message.Message]):
    json_path = os.path.join(_VALIDATION_PATH, file_name + '.valid.ndjson')
    proto_path = os.path.join(_VALIDATION_PATH, file_name + '.valid.prototxt')
    self.assert_parse_equals_golden(
        json_path,
        proto_path,
        primitive_cls,
        parse_f=json_format.json_fhir_string_to_proto,
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
      ('_withProcedureAmbulation', 'Procedure-ambulation'),
      ('_withProcedureAppendectomyNarrative',
       'Procedure-appendectomy-narrative'),
      ('_withProcedureBiopsy', 'Procedure-biopsy'),
      ('_withProcedureColonBiopsy', 'Procedure-colon-biopsy'),
      ('_withProcedureColonoscopy', 'Procedure-colonoscopy'),
      ('_withProcedureEducation', 'Procedure-education'),
      ('_withProcedureExampleImplant', 'Procedure-example-implant'),
      ('_withProcedureExample', 'Procedure-example'),
      ('_withProcedureF001', 'Procedure-f001'),
      ('_withProcedureF002', 'Procedure-f002'),
      ('_withProcedureF003', 'Procedure-f003'),
      ('_withProcedureF004', 'Procedure-f004'),
      ('_withProcedureF201', 'Procedure-f201'),
      ('_withProcedureHcbs', 'Procedure-HCBS'),
      ('_withProcedureOb', 'Procedure-ob'),
      ('_withProcedurePhysicalTherapy', 'Procedure-physical-therapy'),
  )
  def testJsonFormat_forValidProcedure_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   procedure_pb2.Procedure)

  @parameterized.named_parameters(
      ('_withProvenanceConsentSignature', 'Provenance-consent-signature'),
      ('_withProvenanceExampleBiocomputeObject',
       'Provenance-example-biocompute-object'),
      ('_withProvenanceExampleCwl', 'Provenance-example-cwl'),
      ('_withProvenanceExample', 'Provenance-example'),
      ('_withProvenanceSignature', 'Provenance-signature'),
  )
  def testJsonFormat_forValidProvenance_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   provenance_pb2.Provenance)

  @parameterized.named_parameters(
      ('_withQuestionnaire3141', 'Questionnaire-3141'),
      ('_withQuestionnaireBb', 'Questionnaire-bb'),
      ('_withQuestionnaireF201', 'Questionnaire-f201'),
      ('_withQuestionnaireGcs', 'Questionnaire-gcs'),
      ('_withQuestionnairePhq9Questionnaire',
       'Questionnaire-phq-9-questionnaire'),
      ('_withQuestionnaireQs1', 'Questionnaire-qs1'),
      ('_withQuestionnaireZikaVirusExposureAssessment',
       'Questionnaire-zika-virus-exposure-assessment'),
  )
  def testJsonFormat_forValidQuestionnaire_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, questionnaire_pb2.Questionnaire)

  @parameterized.named_parameters(
      ('_withQuestionnaireResponse3141', 'QuestionnaireResponse-3141'),
      ('_withQuestionnaireResponseBb', 'QuestionnaireResponse-bb'),
      ('_withQuestionnaireResponseF201', 'QuestionnaireResponse-f201'),
      ('_withQuestionnaireResponseGcs', 'QuestionnaireResponse-gcs'),
      ('_withQuestionnaireResponseUssgFhtAnswers',
       'QuestionnaireResponse-ussg-fht-answers'),
  )
  def testJsonFormat_forValidQuestionnaireResponse_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, questionnaire_response_pb2.QuestionnaireResponse)

  @parameterized.named_parameters(
      ('_withRelatedPersonBenedicte', 'RelatedPerson-benedicte'),
      ('_withRelatedPersonF001', 'RelatedPerson-f001'),
      ('_withRelatedPersonF002', 'RelatedPerson-f002'),
      ('_withRelatedPersonNewbornMom', 'RelatedPerson-newborn-mom'),
      ('_withRelatedPersonPeter', 'RelatedPerson-peter'),
  )
  def testJsonFormat_forValidRelatedPerson_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, related_person_pb2.RelatedPerson)

  @parameterized.named_parameters(
      ('_withRequestGroupExample', 'RequestGroup-example'),
      ('_withRequestGroupKdn5Example', 'RequestGroup-kdn5-example'),
  )
  def testJsonFormat_forValidRequestGroup_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, request_group_pb2.RequestGroup)

  @parameterized.named_parameters(
      ('_withResearchDefinitionExample', 'ResearchDefinition-example'),)
  def testJsonFormat_forValidResearchDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, research_definition_pb2.ResearchDefinition)

  @parameterized.named_parameters(
      ('_withResearchElementDefinitionExample',
       'ResearchElementDefinition-example'),)
  def testJsonFormat_forValidResearchElementDefinition_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, research_element_definition_pb2.ResearchElementDefinition)

  @parameterized.named_parameters(
      ('_withResearchStudyExample', 'ResearchStudy-example'),)
  def testJsonFormat_forValidResearchStudy_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, research_study_pb2.ResearchStudy)

  @parameterized.named_parameters(
      ('_withResearchSubjectExample', 'ResearchSubject-example'),)
  def testJsonFormat_forValidResearchSubject_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, research_subject_pb2.ResearchSubject)

  @parameterized.named_parameters(
      ('_withRiskAssessmentBreastcancerRisk',
       'RiskAssessment-breastcancer-risk'),
      ('_withRiskAssessmentCardiac', 'RiskAssessment-cardiac'),
      ('_withRiskAssessmentGenetic', 'RiskAssessment-genetic'),
      ('_withRiskAssessmentPopulation', 'RiskAssessment-population'),
      ('_withRiskAssessmentPrognosis', 'RiskAssessment-prognosis'),
      ('_withRiskAssessmentRiskexample', 'RiskAssessment-riskexample'),
  )
  def testJsonFormat_forValidRiskAssessment_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, risk_assessment_pb2.RiskAssessment)

  @parameterized.named_parameters(
      ('_withRiskEvidenceSynthesisExample', 'RiskEvidenceSynthesis-example'),)
  def testJsonFormat_forValidRiskEvidenceSynthesis_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, risk_evidence_synthesis_pb2.RiskEvidenceSynthesis)

  @parameterized.named_parameters(
      ('_withScheduleExample', 'Schedule-example'),
      ('_withScheduleExampleLoc1', 'Schedule-exampleloc1'),
      ('_withScheduleExampleLoc2', 'Schedule-exampleloc2'),
  )
  def testJsonFormat_forValidSchedule_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   schedule_pb2.Schedule)

  @parameterized.named_parameters(
      ('_withServiceRequestAmbulation', 'ServiceRequest-ambulation'),
      ('_withServiceRequestAppendectomyNarrative',
       'ServiceRequest-appendectomy-narrative'),
      ('_withServiceRequestBenchpress', 'ServiceRequest-benchpress'),
      ('_withServiceRequestColonBiopsy', 'ServiceRequest-colon-biopsy'),
      ('_withServiceRequestColonoscopy', 'ServiceRequest-colonoscopy'),
      ('_withServiceRequestDi', 'ServiceRequest-di'),
      ('_withServiceRequestDoNotTurn', 'ServiceRequest-do-not-turn'),
      ('_withServiceRequestEducation', 'ServiceRequest-education'),
      ('_withServiceRequestExampleImplant', 'ServiceRequest-example-implant'),
      ('_withServiceRequestExample', 'ServiceRequest-example'),
      ('_withServiceRequestExamplePgx', 'ServiceRequest-example-pgx'),
      ('_withServiceRequestFt4', 'ServiceRequest-ft4'),
      ('_withServiceRequestLipid', 'ServiceRequest-lipid'),
      ('_withServiceRequestMyringotomy', 'ServiceRequest-myringotomy'),
      ('_withServiceRequestOb', 'ServiceRequest-ob'),
      ('_withServiceRequestOgExample1', 'ServiceRequest-og-example1'),
      ('_withServiceRequestPhysicalTherapy', 'ServiceRequest-physical-therapy'),
      ('_withServiceRequestPhysiotherapy', 'ServiceRequest-physiotherapy'),
      ('_withServiceRequestSubrequest', 'ServiceRequest-subrequest'),
      ('_withServiceRequestVent', 'ServiceRequest-vent'),
  )
  def testJsonFormat_forValidServiceRequest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, service_request_pb2.ServiceRequest)

  @parameterized.named_parameters(
      ('_withSlot1', 'Slot-1'),
      ('_withSlot2', 'Slot-2'),
      ('_withSlot3', 'Slot-3'),
      ('_withSlotExample', 'Slot-example'),
  )
  def testJsonFormat_forValidSlot_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, slot_pb2.Slot)

  @parameterized.named_parameters(
      ('_withSpecimen101', 'Specimen-101'),
      ('_withSpecimenIsolate', 'Specimen-isolate'),
      ('_withSpecimenPooledSerum', 'Specimen-pooled-serum'),
      ('_withSpecimenSst', 'Specimen-sst'),
      ('_withSpecimenVmaUrine', 'Specimen-vma-urine'),
  )
  def testJsonFormat_forValidSpecimen_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   specimen_pb2.Specimen)

  @parameterized.named_parameters(
      ('_withSpecimenDefinition2364', 'SpecimenDefinition-2364'),)
  def testJsonFormat_forValidSpecimenDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, specimen_definition_pb2.SpecimenDefinition)

  @parameterized.named_parameters(
      ('_withStructureMapExample', 'StructureMap-example'),
      ('_withStructureMapSupplyrequestTransform',
       'StructureMap-supplyrequest-transform'),
  )
  def testJsonFormat_forValidStructureMap_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, structure_map_pb2.StructureMap)

  @parameterized.named_parameters(
      ('_withStructureDefinitionCoding', 'StructureDefinition-Coding'),
      ('_withStructureDefinitionLipidProfile',
       'StructureDefinition-lipidprofile'),
  )
  def testJsonFormat_forValidStructureDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, structure_definition_pb2.StructureDefinition)

  @parameterized.named_parameters(
      ('_withSubscriptionExampleError', 'Subscription-example-error'),
      ('_withSubscriptionExample', 'Subscription-example'),
  )
  def testJsonFormat_forValidSubscription_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, subscription_pb2.Subscription)

  @parameterized.named_parameters(
      ('_withSubstanceExample', 'Substance-example'),
      ('_withSubstanceF201', 'Substance-f201'),
      ('_withSubstanceF202', 'Substance-f202'),
      ('_withSubstanceF203', 'Substance-f203'),
      ('_withSubstanceF204', 'Substance-f204'),
      ('_withSubstanceF205', 'Substance-f205'),
  )
  def testJsonFormat_forValidSubstance_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   substance_pb2.Substance)

  @parameterized.named_parameters(
      ('_withSubstanceSpecificationExample', 'SubstanceSpecification-example'),)
  def testJsonFormat_forValidSubstanceSpecification_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, substance_specification_pb2.SubstanceSpecification)

  @parameterized.named_parameters(
      ('_withSupplyDeliveryPumpdelivery', 'SupplyDelivery-pumpdelivery'),
      ('_withSupplyDeliverySimpledelivery', 'SupplyDelivery-simpledelivery'),
  )
  def testJsonFormat_forValidSupplyDelivery_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, supply_delivery_pb2.SupplyDelivery)

  @parameterized.named_parameters(
      ('_withSupplyRequestSimpleorder', 'SupplyRequest-simpleorder'),)
  def testJsonFormat_forValidSupplyRequest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, supply_request_pb2.SupplyRequest)

  @parameterized.named_parameters(
      ('_withTaskExample1', 'Task-example1'),
      ('_withTaskExample2', 'Task-example2'),
      ('_withTaskExample3', 'Task-example3'),
      ('_withTaskExample4', 'Task-example4'),
      ('_withTaskExample5', 'Task-example5'),
      ('_withTaskExample6', 'Task-example6'),
      ('_withTaskFmExample1', 'Task-fm-example1'),
      ('_withTaskFmExample2', 'Task-fm-example2'),
      ('_withTaskFmExample3', 'Task-fm-example3'),
      ('_withTaskFmExample4', 'Task-fm-example4'),
      ('_withTaskFmExample5', 'Task-fm-example5'),
      ('_withTaskFmExample6', 'Task-fm-example6'),
  )
  def testJsonFormat_forValidTask_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name, task_pb2.Task)

  @parameterized.named_parameters(
      ('_withTerminologyCapabilitiesExample',
       'TerminologyCapabilities-example'),)
  def testJsonFormat_forValidTerminologyCapabilities_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, terminology_capabilities_pb2.TerminologyCapabilities)

  @parameterized.named_parameters(
      ('_withTestReportTestReportExample', 'TestReport-testreport-example'),)
  def testJsonFormat_forValidTestReport_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   test_report_pb2.TestReport)

  @parameterized.named_parameters(
      ('_withTestScriptTestScriptExampleHistory',
       'TestScript-testscript-example-history'),
      ('_withTestScriptTestScriptExample', 'TestScript-testscript-example'),
      ('_withTestScriptTestScriptExampleMultisystem',
       'TestScript-testscript-example-multisystem'),
      ('_withTestScriptTestScriptExampleReadtest',
       'TestScript-testscript-example-readtest'),
      ('_withTestScriptTestScriptExampleSearch',
       'TestScript-testscript-example-search'),
      ('_withTestScriptTestScriptExampleUpdate',
       'TestScript-testscript-example-update'),
  )
  def testJsonFormat_forValidTestScript_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   test_script_pb2.TestScript)

  @parameterized.named_parameters(
      ('_withVerificationResultExample', 'VerificationResult-example'),)
  def testJsonFormat_forValidVerificationResult_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, verification_result_pb2.VerificationResult)

  @parameterized.named_parameters(
      ('_withVisionPrescription33123', 'VisionPrescription-33123'),
      ('_withVisionPrescription33124', 'VisionPrescription-33124'),
  )
  def testJsonFormat_forValidVisionPrescription_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, vision_prescription_pb2.VisionPrescription)

  @parameterized.named_parameters(
      ('_withCompositionExample', 'Composition-example',
       composition_pb2.Composition),
      ('_withEcounterHome', 'Encounter-home', encounter_pb2.Encounter),
      ('_withObservationExampleGenetics1', 'Observation-example-genetics-1',
       observation_pb2.Observation),
      ('_withPatientExample', 'Patient-example', patient_pb2.Patient),
  )
  def testPrintForAnalytics_forValidResource_succeeds(
      self, file_name: str, proto_cls: Type[message.Message]):
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

  def assert_parse_and_print_examples_equals_golden(
      self, file_name: str, proto_cls: Type[message.Message]):
    """Convenience method for performing assertions on FHIR R4 examples."""
    json_path = os.path.join(_EXAMPLES_PATH, file_name + '.json')
    proto_path = os.path.join(_EXAMPLES_PATH, file_name + '.prototxt')
    self.assert_parse_and_print_equals_golden(json_path, proto_path, proto_cls)

  def assert_parse_and_print_spec_equals_golden(
      self, file_name: str, proto_cls: Type[message.Message]):
    """Convenience method for performing assertions on the FHIR R4 spec."""
    json_path = os.path.join(_FHIR_SPEC_PATH, file_name + '.json')
    proto_path = os.path.join(_EXAMPLES_PATH, file_name + '.prototxt')
    self.assert_parse_and_print_equals_golden(json_path, proto_path, proto_cls)

  def assert_parse_and_print_equals_golden(self, json_path: str,
                                           proto_path: str,
                                           proto_cls: Type[message.Message]):
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
