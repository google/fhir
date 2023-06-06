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
"""Test FHIR STU3 parsing/printing functionality."""

import os
from typing import Type, TypeVar
import unittest

from google.protobuf import message
from absl.testing import absltest
from absl.testing import parameterized
from proto.google.fhir.proto.stu3 import datatypes_pb2
from proto.google.fhir.proto.stu3 import resources_pb2
from google.fhir.core.internal.json_format import json_format_test
from google.fhir.core.testing import testdata_utils
from google.fhir.core.utils import proto_utils
from google.fhir.stu3 import json_format

_BIGQUERY_PATH = os.path.join('testdata', 'stu3', 'bigquery')
_EXAMPLES_PATH = os.path.join('testdata', 'stu3', 'examples')
_FHIR_SPEC_PATH = os.path.join('spec', 'hl7.fhir.core', '3.0.1', 'package')
_VALIDATION_PATH = os.path.join('testdata', 'stu3', 'validation')

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
                                                   resources_pb2.CodeSystem)

  @parameterized.named_parameters(
      ('with_account_example', 'Account-example'),
      ('with_account_ewg', 'Account-ewg'),
  )
  def test_json_format_for_valid_account_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Account)

  @parameterized.named_parameters(
      (
          'with_activity_definition_referral_primary_care_mental_health',
          'ActivityDefinition-referralPrimaryCareMentalHealth',
      ),
      (
          'with_activity_definition_citalopram_prescription',
          'ActivityDefinition-citalopramPrescription',
      ),
      (
          'with_activity_definition_referral_primary_care_mental_health_initial',
          'ActivityDefinition-referralPrimaryCareMentalHealth-initial',
      ),
      (
          'with_activity_definition_heart_valve_replacement',
          'ActivityDefinition-heart-valve-replacement',
      ),
      (
          'with_activity_definition_blood_tubes_supply',
          'ActivityDefinition-blood-tubes-supply',
      ),
  )
  def test_json_format_for_valid_activity_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ActivityDefinition)

  @parameterized.named_parameters(
      ('with_adverse_event_example', 'AdverseEvent-example'),
  )
  def test_json_format_for_valid_adverse_event_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.AdverseEvent)

  @parameterized.named_parameters(
      ('with_allergy_intolerance_example', 'AllergyIntolerance-example'),
  )
  def test_json_format_for_valid_allergy_intolerance_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.AllergyIntolerance)

  @parameterized.named_parameters(
      ('with_appointment_example', 'Appointment-example'),
      ('with_appointment2docs', 'Appointment-2docs'),
      ('with_appointment_example_req', 'Appointment-examplereq'),
  )
  def test_json_format_for_valid_appointment_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Appointment)

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
        file_name, resources_pb2.AppointmentResponse)

  @parameterized.named_parameters(
      ('with_audit_event_example', 'AuditEvent-example'),
      ('with_audit_event_example_disclosure', 'AuditEvent-example-disclosure'),
      ('with_audit_event_example_login', 'AuditEvent-example-login'),
      ('with_audit_event_example_logout', 'AuditEvent-example-logout'),
      ('with_audit_event_example_media', 'AuditEvent-example-media'),
      ('with_audit_event_example_pix_query', 'AuditEvent-example-pixQuery'),
      ('with_audit_event_example_search', 'AuditEvent-example-search'),
      ('with_audit_event_example_rest', 'AuditEvent-example-rest'),
  )
  def test_json_format_for_valid_audit_event_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.AuditEvent)

  @parameterized.named_parameters(
      ('with_basic_referral', 'Basic-referral'),
      ('with_basic_class_model', 'Basic-classModel'),
      ('with_basic_basic_example_narrative', 'Basic-basic-example-narrative'),
  )
  def test_json_format_for_valid_basic_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Basic)

  @parameterized.named_parameters(
      ('with_body_site_fetus', 'BodySite-fetus'),
      ('with_body_site_skin_patch', 'BodySite-skin-patch'),
      ('with_body_site_tumor', 'BodySite-tumor'),
  )
  def test_json_format_for_valid_body_site_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.BodySite)

  @parameterized.named_parameters(
      ('with_bundle_bundle_example', 'Bundle-bundle-example'),
      (
          'with_bundle72ac849352ac41bd8d5d7258c289b5ea',
          'Bundle-72ac8493-52ac-41bd-8d5d-7258c289b5ea',
      ),
      ('with_bundle_hla1', 'Bundle-hla-1'),
      ('with_bundle_father', 'Bundle-father'),
      (
          'with_bundle_b0a5e427783c44adb87e2e3efe3369b6f',
          'Bundle-b0a5e4277-83c4-4adb-87e2-e3efe3369b6f',
      ),
      (
          'with_bundle3ad0687e_f477468c_afd5_fcc2bf897819',
          'Bundle-3ad0687e-f477-468c-afd5-fcc2bf897819',
      ),
      (
          'with_patient_examples_cypress_template',
          'patient-examples-cypress-template',
      ),
      (
          'with_bundle_b248b1b216864b94993637d7a5f94b51',
          'Bundle-b248b1b2-1686-4b94-9936-37d7a5f94b51',
      ),
      (
          'with_bundle3ad0687e_f477468c_afd5_fcc2bf897809',
          'Bundle-3ad0687e-f477-468c-afd5-fcc2bf897809',
      ),
      (
          'with_bundle3ad0687e_f477468c_afd5_fcc2bf897808',
          'Bundle-3ad0687e-f477-468c-afd5-fcc2bf897808',
      ),
      ('with_bundle_ussg_fht', 'Bundle-ussg-fht'),
      ('with_bundle_xds', 'Bundle-xds'),
  )
  def test_json_format_for_valid_bundle_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Bundle)

  @parameterized.named_parameters(
      ('with_capability_statement_example', 'CapabilityStatement-example'),
      ('with_capability_statement_phr', 'CapabilityStatement-phr'),
  )
  def test_json_format_for_valid_capability_statement_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.CapabilityStatement)

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
                                                   resources_pb2.CarePlan)

  @parameterized.named_parameters(
      ('with_care_team_example', 'CareTeam-example'),
  )
  def test_json_format_for_valid_care_team_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.CareTeam)

  @parameterized.named_parameters(
      ('with_charge_item_example', 'ChargeItem-example'),
  )
  def test_json_format_for_valid_charge_item_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.ChargeItem)

  @parameterized.named_parameters(
      ('with_claim100150', 'Claim-100150'),
      ('with_claim960150', 'Claim-960150'),
      ('with_claim960151', 'Claim-960151'),
      ('with_claim100151', 'Claim-100151'),
      ('with_claim100156', 'Claim-100156'),
      ('with_claim100152', 'Claim-100152'),
      ('with_claim100155', 'Claim-100155'),
      ('with_claim100154', 'Claim-100154'),
      ('with_claim100153', 'Claim-100153'),
      ('with_claim760150', 'Claim-760150'),
      ('with_claim760152', 'Claim-760152'),
      ('with_claim760151', 'Claim-760151'),
      ('with_claim860150', 'Claim-860150'),
      ('with_claim660150', 'Claim-660150'),
      ('with_claim660151', 'Claim-660151'),
      ('with_claim660152', 'Claim-660152'),
  )
  def test_json_format_for_valid_claim_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Claim)

  @parameterized.named_parameters(
      ('with_claim_response_r3500', 'ClaimResponse-R3500'),
  )
  def test_json_format_for_valid_claim_response_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.ClaimResponse)

  @parameterized.named_parameters(
      ('with_clinical_impression_example', 'ClinicalImpression-example'),
  )
  def test_json_format_for_valid_clinical_impression_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ClinicalImpression)

  @parameterized.named_parameters(
      ('with_code_system_example', 'CodeSystem-example'),
      ('with_code_system_list_example_codes', 'CodeSystem-list-example-codes'),
  )
  def test_json_format_for_valid_code_system_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.CodeSystem)

  @parameterized.named_parameters(
      ('with_communication_example', 'Communication-example'),
      ('with_communication_fm_attachment', 'Communication-fm-attachment'),
      ('with_communication_fm_solicited', 'Communication-fm-solicited'),
  )
  def test_json_format_for_valid_communication_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Communication)

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
        file_name, resources_pb2.CommunicationRequest)

  @parameterized.named_parameters(
      ('with_compartment_definition_example', 'CompartmentDefinition-example'),
  )
  def test_json_format_for_valid_compartment_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.CompartmentDefinition)

  @parameterized.named_parameters(
      ('with_composition_example', 'Composition-example'),
  )
  def test_json_format_for_valid_composition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Composition)

  @parameterized.named_parameters(
      ('with_conceptmap_example', 'conceptmap-example'),
      ('with_conceptmap_example2', 'conceptmap-example-2'),
      (
          'with_conceptmap_example_specimen_type',
          'conceptmap-example-specimen-type',
      ),
  )
  def test_json_format_for_valid_concept_map_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.ConceptMap)

  @parameterized.named_parameters(
      ('with_condition_example', 'Condition-example'),
      ('with_condition_example2', 'Condition-example2'),
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
                                                   resources_pb2.Condition)

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
                                                   resources_pb2.Consent)

  @parameterized.named_parameters(
      (
          'with_allergy_intolerance_example',
          'AllergyIntolerance-example',
          resources_pb2.AllergyIntolerance,
          'allergy_intolerance',
      ),
      (
          'with_capability_statement_example',
          'CapabilityStatement-example',
          resources_pb2.CapabilityStatement,
          'capability_statement',
      ),
      (
          'with_immunization_example',
          'Immunization-example',
          resources_pb2.Immunization,
          'immunization',
      ),
      (
          'with_medication_med0305',
          'Medication-med0305',
          resources_pb2.Medication,
          'medication',
      ),
      (
          'with_observation_f004',
          'Observation-f004',
          resources_pb2.Observation,
          'observation',
      ),
      (
          'with_patient_example',
          'patient-example',
          resources_pb2.Patient,
          'patient',
      ),
      (
          'with_practitioner_f003',
          'Practitioner-f003',
          resources_pb2.Practitioner,
          'practitioner',
      ),
      (
          'with_procedure_ambulation',
          'Procedure-ambulation',
          resources_pb2.Procedure,
          'procedure',
      ),
      ('with_task_example4', 'Task-example4', resources_pb2.Task, 'task'),
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
    contained = resources_pb2.ContainedResource()
    proto_utils.set_value_at_field(contained, contained_field, golden_proto)

    # Validate printing and then parsing the print output against the golden
    contained_json_str = json_format.print_fhir_to_json_string(contained)
    parsed_contained = json_format.json_fhir_string_to_proto(
        contained_json_str,
        resources_pb2.ContainedResource,
        validate=True,
        default_timezone='Australia/Sydney')
    self.assertEqual(contained, parsed_contained)

  @parameterized.named_parameters(
      ('with_contract_c123', 'Contract-C-123'),
      ('with_contract_c2121', 'Contract-C-2121'),
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
                                                   resources_pb2.Contract)

  @parameterized.named_parameters(
      ('with_coverage9876b1', 'Coverage-9876B1'),
      ('with_coverage7546d', 'Coverage-7546D'),
      ('with_coverage7547e', 'Coverage-7547E'),
      ('with_coverage_sp1234', 'Coverage-SP1234'),
  )
  def test_json_format_for_valid_coverage_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Coverage)

  @parameterized.named_parameters(
      ('with_data_element_gender', 'DataElement-gender'),
      ('with_data_element_prothrombin', 'DataElement-prothrombin'),
  )
  def test_json_format_for_valid_data_element_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.DataElement)

  @parameterized.named_parameters(
      ('with_detected_issue_ddi', 'DetectedIssue-ddi'),
      ('with_detected_issue_allergy', 'DetectedIssue-allergy'),
      ('with_detected_issue_duplicate', 'DetectedIssue-duplicate'),
      ('with_detected_issue_lab', 'DetectedIssue-lab'),
  )
  def test_json_format_for_valid_detected_issue_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.DetectedIssue)

  @parameterized.named_parameters(
      ('with_device_example', 'Device-example'),
      ('with_device_f001', 'Device-f001'),
      ('with_device_ihe_pcd', 'Device-ihe-pcd'),
      ('with_device_example_pacemaker', 'Device-example-pacemaker'),
      ('with_device_software', 'Device-software'),
      ('with_device_example_udi1', 'Device-example-udi1'),
      ('with_device_example_udi2', 'Device-example-udi2'),
      ('with_device_example_udi3', 'Device-example-udi3'),
      ('with_device_example_udi4', 'Device-example-udi4'),
  )
  def test_json_format_for_valid_device_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Device)

  @parameterized.named_parameters(
      ('with_device_component_example', 'DeviceComponent-example'),
      (
          'with_device_component_example_prodspec',
          'DeviceComponent-example-prodspec',
      ),
  )
  def test_json_format_for_valid_device_component_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.DeviceComponent)

  @parameterized.named_parameters(
      ('with_device_metric_example', 'DeviceMetric-example'),
  )
  def test_json_format_for_valid_device_metric_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.DeviceMetric)

  @parameterized.named_parameters(
      ('with_device_request_example', 'DeviceRequest-example'),
      ('with_device_request_insulin_pump', 'DeviceRequest-insulinpump'),
  )
  def test_json_format_for_valid_device_request_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.DeviceRequest)

  @parameterized.named_parameters(
      ('with_device_use_statement_example', 'DeviceUseStatement-example'),
  )
  def test_json_format_for_valid_device_use_statement_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.DeviceUseStatement)

  # TODO(b/223660148): Investigate why test is timing out in Python 3.9
  @unittest.skip('Test timing out in Python 3.9')
  @parameterized.named_parameters(
      ('with_diagnostic_report101', 'DiagnosticReport-101'),
      ('with_diagnostic_report102', 'DiagnosticReport-102'),
      ('with_diagnostic_report_f001', 'DiagnosticReport-f001'),
      ('with_diagnostic_report_f201', 'DiagnosticReport-f201'),
      ('with_diagnostic_report_f202', 'DiagnosticReport-f202'),
      ('with_diagnostic_report_ghp', 'DiagnosticReport-ghp'),
      (
          'with_diagnostic_report_gingival_mass',
          'DiagnosticReport-gingival-mass',
      ),
      ('with_diagnostic_report_lipids', 'DiagnosticReport-lipids'),
      ('with_diagnostic_report_pap', 'DiagnosticReport-pap'),
      ('with_diagnostic_report_example_pgx', 'DiagnosticReport-example-pgx'),
      ('with_diagnostic_report_ultrasound', 'DiagnosticReport-ultrasound'),
      ('with_diagnostic_report_dg2', 'DiagnosticReport-dg2'),
  )
  def test_json_format_for_valid_diagnostic_report_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.DiagnosticReport)

  @parameterized.named_parameters(
      ('with_document_manifest_example', 'DocumentManifest-example'),
  )
  def test_json_format_for_valid_document_manifest_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.DocumentManifest)

  @parameterized.named_parameters(
      ('with_document_reference_example', 'DocumentReference-example'),
  )
  def test_json_format_for_valid_document_reference_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.DocumentReference)

  @parameterized.named_parameters(
      ('with_eligibility_request52345', 'EligibilityRequest-52345'),
      ('with_eligibility_request52346', 'EligibilityRequest-52346'),
  )
  def test_json_format_for_valid_eligibility_request_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.EligibilityRequest)

  @parameterized.named_parameters(
      ('with_eligibility_response_e2500', 'EligibilityResponse-E2500'),
      ('with_eligibility_response_e2501', 'EligibilityResponse-E2501'),
      ('with_eligibility_response_e2502', 'EligibilityResponse-E2502'),
      ('with_eligibility_response_e2503', 'EligibilityResponse-E2503'),
  )
  def test_json_format_for_valid_eligibility_response_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.EligibilityResponse)

  @parameterized.named_parameters(
      (
          'with_parameters_empty_resource',
          'Parameters-empty-resource',
          resources_pb2.Parameters,
      ),
  )
  def test_json_format_for_valid_empty_nested_resource_succeeds(
      self, file_name: str, proto_cls: Type[message.Message]
  ):
    self.assert_parse_and_print_examples_equals_golden(file_name, proto_cls)

  @parameterized.named_parameters(
      ('with_encounter_example', 'Encounter-example'),
      ('with_encounter_emerg', 'Encounter-emerg'),
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
                                                   resources_pb2.Encounter)

  @parameterized.named_parameters(
      ('with_endpoint_example', 'Endpoint-example'),
      ('with_endpoint_example_iid', 'Endpoint-example-iid'),
      ('with_endpoint_example_wadors', 'Endpoint-example-wadors'),
  )
  def test_json_format_for_valid_endpoint_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Endpoint)

  @parameterized.named_parameters(
      ('with_enrollment_request22345', 'EnrollmentRequest-22345'),
  )
  def test_json_format_for_valid_enrollment_request_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.EnrollmentRequest)

  @parameterized.named_parameters(
      ('with_enrollment_response_er2500', 'EnrollmentResponse-ER2500'),
  )
  def test_json_format_for_valid_enrollment_response_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.EnrollmentResponse)

  @parameterized.named_parameters(
      ('with_episode_of_care_example', 'EpisodeOfCare-example'),
  )
  def test_json_format_for_valid_episode_of_care_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.EpisodeOfCare)

  @parameterized.named_parameters(
      ('with_expansion_profile_example', 'ExpansionProfile-example'),
  )
  def test_json_format_for_valid_expansion_profile_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ExpansionProfile)

  @parameterized.named_parameters(
      ('with_explanation_of_benefit_eb3500', 'ExplanationOfBenefit-EB3500'),
  )
  def test_json_format_for_valid_explanation_of_benefit_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ExplanationOfBenefit)

  @parameterized.named_parameters(
      ('with_family_member_history_father', 'FamilyMemberHistory-father'),
      ('with_family_member_history_mother', 'FamilyMemberHistory-mother'),
  )
  def test_json_format_for_valid_family_member_history_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.FamilyMemberHistory)

  @parameterized.named_parameters(
      ('with_flag_example', 'Flag-example'),
      ('with_flag_example_encounter', 'Flag-example-encounter'),
  )
  def test_json_format_for_valid_flag_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Flag)

  @parameterized.named_parameters(
      ('with_goal_example', 'Goal-example'),
      ('with_goal_stop_smoking', 'Goal-stop-smoking'),
  )
  def test_json_format_for_valid_goal_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Goal)

  @parameterized.named_parameters(
      ('with_graph_definition_example', 'GraphDefinition-example'),
  )
  def test_json_format_for_valid_graph_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.GraphDefinition)

  @parameterized.named_parameters(
      ('with_group101', 'Group-101'),
      ('with_group102', 'Group-102'),
  )
  def test_json_format_for_valid_group_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Group)

  @parameterized.named_parameters(
      ('with_guidance_response_example', 'GuidanceResponse-example'),
  )
  def test_json_format_for_valid_guidance_response_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.GuidanceResponse)

  @parameterized.named_parameters(
      ('with_healthcare_service_example', 'HealthcareService-example'),
  )
  def test_json_format_for_valid_healthcare_service_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.HealthcareService)

  @parameterized.named_parameters(
      ('with_imaging_manifest_example', 'ImagingManifest-example'),
  )
  def test_json_format_for_valid_imaging_manifest_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ImagingManifest)

  @parameterized.named_parameters(
      ('with_imaging_study_example', 'ImagingStudy-example'),
      ('with_imaging_study_example_xr', 'ImagingStudy-example-xr'),
  )
  def test_json_format_for_valid_imaging_study_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.ImagingStudy)

  @parameterized.named_parameters(
      ('with_immunization_example', 'Immunization-example'),
      ('with_immunization_historical', 'Immunization-historical'),
      ('with_immunization_not_given', 'Immunization-notGiven'),
  )
  def test_json_format_for_valid_immunization_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Immunization)

  @parameterized.named_parameters(
      (
          'with_immunization_recommendation_example',
          'ImmunizationRecommendation-example',
      ),
      (
          'with_immunization_recommendation_target_disease_example',
          'immunizationrecommendation-target-disease-example',
      ),
  )
  def test_json_format_for_valid_immunization_recommendation_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ImmunizationRecommendation)

  @parameterized.named_parameters(
      ('with_implementation_guide_example', 'ImplementationGuide-example'),
  )
  def test_json_format_for_valid_implementation_guide_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ImplementationGuide)

  @parameterized.named_parameters(
      ('with_library_library_cms146_example', 'Library-library-cms146-example'),
      ('with_library_composition_example', 'Library-composition-example'),
      ('with_library_example', 'Library-example'),
      (
          'with_library_library_fhir_helpers_predecessor',
          'Library-library-fhir-helpers-predecessor',
      ),
  )
  def test_json_format_for_valid_library_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Library)

  @parameterized.named_parameters(
      ('with_linkage_example', 'Linkage-example'),
  )
  def test_json_format_for_valid_linkage_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Linkage)

  @parameterized.named_parameters(
      ('with_list_example', 'List-example'),
      ('with_list_current_allergies', 'List-current-allergies'),
      (
          'with_list_example_double_cousin_relationship',
          'List-example-double-cousin-relationship',
      ),
      ('with_list_example_empty', 'List-example-empty'),
      ('with_list_f201', 'List-f201'),
      ('with_list_genetic', 'List-genetic'),
      ('with_list_prognosis', 'List-prognosis'),
      ('with_list_med_list', 'List-med-list'),
      ('with_list_example_simple_empty', 'List-example-simple-empty'),
  )
  def test_json_format_for_valid_list_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.List)

  @parameterized.named_parameters(
      ('with_location1', 'Location-1'),
      ('with_location_amb', 'Location-amb'),
      ('with_location_hl7', 'Location-hl7'),
      ('with_location_ph', 'Location-ph'),
      ('with_location2', 'Location-2'),
      ('with_location_ukp', 'Location-ukp'),
  )
  def test_json_format_for_valid_location_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Location)

  @parameterized.named_parameters(
      ('with_measure_measure_cms146_example', 'Measure-measure-cms146-example'),
      ('with_measure_component_a_example', 'Measure-component-a-example'),
      ('with_measure_component_b_example', 'Measure-component-b-example'),
      ('with_measure_composite_example', 'Measure-composite-example'),
      (
          'with_measure_measure_predecessor_example',
          'Measure-measure-predecessor-example',
      ),
  )
  def test_json_format_for_valid_measure_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Measure)

  @parameterized.named_parameters(
      (
          'with_measure_report_measure_report_cms146_cat1_example',
          'MeasureReport-measurereport-cms146-cat1-example',
      ),
      (
          'with_measure_report_measurereport_cms146_cat2_example',
          'MeasureReport-measurereport-cms146-cat2-example',
      ),
      (
          'with_measure_report_measurereport_cms146_cat3_example',
          'MeasureReport-measurereport-cms146-cat3-example',
      ),
  )
  def test_json_format_for_valid_measure_report_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.MeasureReport)

  @parameterized.named_parameters(
      ('with_media_example', 'Media-example'),
      (
          'with_media1_2_840_11361907579238403408700_3_0_14_19970327150033',
          'Media-1.2.840.11361907579238403408700.3.0.14.19970327150033',
      ),
      ('with_media_sound', 'Media-sound'),
      ('with_media_xray', 'Media-xray'),
  )
  def test_json_format_for_valid_media_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Media)

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
      ('with_medication_medication_example1', 'Medication-medicationexample1'),
      ('with_medication_med_example015', 'Medication-medexample015'),
  )
  def test_json_format_for_valid_medication_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Medication)

  @parameterized.named_parameters(
      (
          'with_medication_administration_medadmin_example03',
          'MedicationAdministration-medadminexample03',
      ),
  )
  def test_json_format_for_valid_medication_administration_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.MedicationAdministration)

  @parameterized.named_parameters(
      ('with_medication_dispense_meddisp008', 'MedicationDispense-meddisp008'),
  )
  def test_json_format_for_valid_medication_dispense_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.MedicationDispense)

  @parameterized.named_parameters(
      ('with_medication_request_medrx0311', 'MedicationRequest-medrx0311'),
      ('with_medication_request_medrx002', 'MedicationRequest-medrx002'),
  )
  def test_json_format_for_valid_medication_request_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.MedicationRequest)

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
        file_name, resources_pb2.MedicationStatement)

  @parameterized.named_parameters(
      ('with_message_definition_example', 'MessageDefinition-example'),
  )
  def test_json_format_for_valid_message_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.MessageDefinition)

  @parameterized.named_parameters(
      (
          'with_message_header1cbdfb97585948a48301d54eab818d68',
          'MessageHeader-1cbdfb97-5859-48a4-8301-d54eab818d68',
      ),
  )
  def test_json_format_for_valid_message_header_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.MessageHeader)

  @parameterized.named_parameters(
      ('with_naming_system_example', 'NamingSystem-example'),
      ('with_naming_system_example_id', 'NamingSystem-example-id'),
      ('with_naming_system_example_replaced', 'NamingSystem-example-replaced'),
  )
  def test_json_format_for_valid_naming_system_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.NamingSystem)

  @parameterized.named_parameters(
      ('with_nutrition_order_cardiac_diet', 'NutritionOrder-cardiacdiet'),
      ('with_nutrition_order_diabetic_diet', 'NutritionOrder-diabeticdiet'),
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
          'with_nutrition_order_enteral_continuous',
          'NutritionOrder-enteralcontinuous',
      ),
      (
          'with_nutrition_order_fiber_restricted_diet',
          'NutritionOrder-fiberrestricteddiet',
      ),
      ('with_nutrition_order_infantenteral', 'NutritionOrder-infantenteral'),
      (
          'with_nutrition_order_protein_supplement',
          'NutritionOrder-proteinsupplement',
      ),
      ('with_nutrition_order_pureed_diet', 'NutritionOrder-pureeddiet'),
      (
          'with_nutrition_order_pureeddiet_simple',
          'NutritionOrder-pureeddiet-simple',
      ),
      ('with_nutrition_order_renal_diet', 'NutritionOrder-renaldiet'),
      (
          'with_nutrition_order_texture_modified',
          'NutritionOrder-texturemodified',
      ),
  )
  def test_json_format_for_valid_nutrition_order_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.NutritionOrder)

  @parameterized.named_parameters(
      ('with_observation_example', 'Observation-example'),
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
      ('with_observation_blood_pressure', 'Observation-blood-pressure'),
      (
          'with_observation_blood_pressure_cancel',
          'Observation-blood-pressure-cancel',
      ),
      ('with_observation_blood_pressure_dar', 'Observation-blood-pressure-dar'),
      ('with_observation_bmd', 'Observation-bmd'),
      ('with_observation_bmi', 'Observation-bmi'),
      ('with_observation_body_height', 'Observation-body-height'),
      ('with_observation_body_length', 'Observation-body-length'),
      ('with_observation_body_temperature', 'Observation-body-temperature'),
      ('with_observation_date_lastmp', 'Observation-date-lastmp'),
      ('with_observation_example_diplotype1', 'Observation-example-diplotype1'),
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
      ('with_observation_example_genetics1', 'Observation-example-genetics-1'),
      ('with_observation_example_genetics2', 'Observation-example-genetics-2'),
      ('with_observation_example_genetics3', 'Observation-example-genetics-3'),
      ('with_observation_example_genetics4', 'Observation-example-genetics-4'),
      ('with_observation_example_genetics5', 'Observation-example-genetics-5'),
      ('with_observation_glasgow', 'Observation-glasgow'),
      ('with_observation_gcs_qa', 'Observation-gcs-qa'),
      ('with_observation_example_haplotype1', 'Observation-example-haplotype1'),
      ('with_observation_example_haplotype2', 'Observation-example-haplotype2'),
      ('with_observation_head_circumference', 'Observation-head-circumference'),
      ('with_observation_heart_rate', 'Observation-heart-rate'),
      ('with_observation_mbp', 'Observation-mbp'),
      ('with_observation_example_phenotype', 'Observation-example-phenotype'),
      ('with_observation_respiratory_rate', 'Observation-respiratory-rate'),
      ('with_observation_ekg', 'Observation-ekg'),
      ('with_observation_sat_o2', 'Observation-satO2'),
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
      ('with_observation_unsat', 'Observation-unsat'),
      ('with_observation_vitals_panel', 'Observation-vitals-panel'),
  )
  def test_json_format_for_valid_observation_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Observation)

  @parameterized.named_parameters(
      ('with_operation_definition_example', 'OperationDefinition-example'),
  )
  def test_json_format_for_valid_operation_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.OperationDefinition)

  @parameterized.named_parameters(
      ('with_operation_outcome101', 'OperationOutcome-101'),
      ('with_operation_outcome_allok', 'OperationOutcome-allok'),
      (
          'with_operation_outcome_break_the_glass',
          'OperationOutcome-break-the-glass',
      ),
      ('with_operation_outcome_exception', 'OperationOutcome-exception'),
      ('with_operation_outcome_search_fail', 'OperationOutcome-searchfail'),
      (
          'with_operation_outcome_validation_fail',
          'OperationOutcome-validationfail',
      ),
  )
  def test_json_format_for_valid_operation_outcome_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.OperationOutcome)

  @parameterized.named_parameters(
      ('with_organization_hl7', 'Organization-hl7'),
      ('with_organization_f001', 'Organization-f001'),
      ('with_organization_f002', 'Organization-f002'),
      ('with_organization_f003', 'Organization-f003'),
      ('with_organization_f201', 'Organization-f201'),
      ('with_organization_f203', 'Organization-f203'),
      ('with_organization1', 'Organization-1'),
      (
          'with_organization2_16_840_1_113883_19_5',
          'Organization-2.16.840.1.113883.19.5',
      ),
      ('with_organization2', 'Organization-2'),
      (
          'with_organization1832473e2fe0452d_abe93cdb9879522f',
          'Organization-1832473e-2fe0-452d-abe9-3cdb9879522f',
      ),
      ('with_organization_mmanu', 'Organization-mmanu'),
  )
  def test_json_format_for_valid_organization_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Organization)

  @parameterized.named_parameters(
      ('with_parameters_example', 'Parameters-example'),
  )
  def test_json_format_for_valid_parameters_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Parameters)

  @parameterized.named_parameters(
      ('with_patient_example', 'patient-example'),
      ('with_patient_example_a', 'patient-example-a'),
      ('with_patient_example_animal', 'patient-example-animal'),
      ('with_patient_example_b', 'patient-example-b'),
      ('with_patient_example_c', 'patient-example-c'),
      ('with_patient_example_chinese', 'patient-example-chinese'),
      ('with_patient_example_d', 'patient-example-d'),
      ('with_patient_example_dicom', 'patient-example-dicom'),
      ('with_patient_example_f001_pieter', 'patient-example-f001-pieter'),
      ('with_patient_example_f201_roel', 'patient-example-f201-roel'),
      ('with_patient_example_ihe_pcd', 'patient-example-ihe-pcd'),
      ('with_patient_example_proband', 'patient-example-proband'),
      ('with_patient_example_xcda', 'patient-example-xcda'),
      ('with_patient_example_xds', 'patient-example-xds'),
      ('with_patient_genetics_example1', 'patient-genetics-example1'),
      ('with_patient_glossy_example', 'patient-glossy-example'),
  )
  def test_json_format_for_valid_patient_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Patient)

  @parameterized.named_parameters(
      ('with_payment_notice77654', 'PaymentNotice-77654'),
  )
  def test_json_format_for_valid_payment_notice_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.PaymentNotice)

  @parameterized.named_parameters(
      ('with_payment_reconciliation_er2500', 'PaymentReconciliation-ER2500'),
  )
  def test_json_format_for_valid_payment_reconciliation_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.PaymentReconciliation)

  @parameterized.named_parameters(
      ('with_person_example', 'Person-example'),
      ('with_person_f002', 'Person-f002'),
  )
  def test_json_format_for_valid_person_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Person)

  @parameterized.named_parameters(
      (
          'with_plan_definition_low_suicide_risk_order_set',
          'PlanDefinition-low-suicide-risk-order-set',
      ),
      ('with_plan_definition_kdn5', 'PlanDefinition-KDN5'),
      (
          'with_plan_definition_options_example',
          'PlanDefinition-options-example',
      ),
      (
          'with_plan_definition_zika_virus_intervention_initial',
          'PlanDefinition-zika-virus-intervention-initial',
      ),
      (
          'with_plan_definition_protocol_example',
          'PlanDefinition-protocol-example',
      ),
  )
  def test_json_format_for_valid_plan_definition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.PlanDefinition)

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
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Practitioner)

  @parameterized.named_parameters(
      ('with_practitioner_role_example', 'PractitionerRole-example'),
  )
  def test_json_format_for_valid_practitioner_role_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.PractitionerRole)

  @parameterized.named_parameters(
      ('with_base64_binary', 'base64_binary', datatypes_pb2.Base64Binary),
      ('with_boolean', 'boolean', datatypes_pb2.Boolean),
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
      ('with_xhtml', 'xhtml', datatypes_pb2.Xhtml),
  )
  def test_json_format_for_valid_primitive_succeeds(
      self, file_name: str, primitive_cls: Type[message.Message]
  ):
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
      ('with_procedure_example', 'Procedure-example'),
      ('with_procedure_ambulation', 'Procedure-ambulation'),
      (
          'with_procedure_appendectomy_narrative',
          'Procedure-appendectomy-narrative',
      ),
      ('with_procedure_biopsy', 'Procedure-biopsy'),
      ('with_procedure_colon_biopsy', 'Procedure-colon-biopsy'),
      ('with_procedure_colonoscopy', 'Procedure-colonoscopy'),
      ('with_procedure_education', 'Procedure-education'),
      ('with_procedure_f001', 'Procedure-f001'),
      ('with_procedure_f002', 'Procedure-f002'),
      ('with_procedure_f003', 'Procedure-f003'),
      ('with_procedure_f004', 'Procedure-f004'),
      ('with_procedure_f201', 'Procedure-f201'),
      ('with_procedure_example_implant', 'Procedure-example-implant'),
      ('with_procedure_ob', 'Procedure-ob'),
      ('with_procedure_physical_therapy', 'Procedure-physical-therapy'),
  )
  def test_json_format_for_valid_procedure_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Procedure)

  @parameterized.named_parameters(
      ('with_procedure_request_example', 'ProcedureRequest-example'),
      (
          'with_procedure_request_physiotherapy',
          'ProcedureRequest-physiotherapy',
      ),
      ('with_procedure_request_do_not_turn', 'ProcedureRequest-do-not-turn'),
      ('with_procedure_request_benchpress', 'ProcedureRequest-benchpress'),
      ('with_procedure_request_ambulation', 'ProcedureRequest-ambulation'),
      (
          'with_procedure_request_appendectomy_narrative',
          'ProcedureRequest-appendectomy-narrative',
      ),
      ('with_procedure_request_colonoscopy', 'ProcedureRequest-colonoscopy'),
      ('with_procedure_request_colon_biopsy', 'ProcedureRequest-colon-biopsy'),
      ('with_procedure_request_di', 'ProcedureRequest-di'),
      ('with_procedure_request_education', 'ProcedureRequest-education'),
      ('with_procedure_request_ft4', 'ProcedureRequest-ft4'),
      (
          'with_procedure_request_example_implant',
          'ProcedureRequest-example-implant',
      ),
      ('with_procedure_request_lipid', 'ProcedureRequest-lipid'),
      ('with_procedure_request_ob', 'ProcedureRequest-ob'),
      ('with_procedure_request_example_pgx', 'ProcedureRequest-example-pgx'),
      (
          'with_procedure_request_physical_therapy',
          'ProcedureRequest-physical-therapy',
      ),
      ('with_procedure_request_subrequest', 'ProcedureRequest-subrequest'),
      ('with_procedure_request_og_example1', 'ProcedureRequest-og-example1'),
  )
  def test_json_format_for_valid_procedure_request_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ProcedureRequest)

  @parameterized.named_parameters(
      ('with_process_request1110', 'ProcessRequest-1110'),
      ('with_process_request1115', 'ProcessRequest-1115'),
      ('with_process_request1113', 'ProcessRequest-1113'),
      ('with_process_request1112', 'ProcessRequest-1112'),
      ('with_process_request1114', 'ProcessRequest-1114'),
      ('with_process_request1111', 'ProcessRequest-1111'),
      ('with_process_request44654', 'ProcessRequest-44654'),
      ('with_process_request87654', 'ProcessRequest-87654'),
      ('with_process_request87655', 'ProcessRequest-87655'),
  )
  def test_json_format_for_valid_process_request_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.ProcessRequest)

  @parameterized.named_parameters(
      ('with_process_response_sr2500', 'ProcessResponse-SR2500'),
      ('with_process_response_sr2349', 'ProcessResponse-SR2349'),
      ('with_process_response_sr2499', 'ProcessResponse-SR2499'),
  )
  def test_json_format_for_valid_process_response_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ProcessResponse)

  @parameterized.named_parameters(
      ('with_provenance_example', 'Provenance-example'),
      (
          'with_provenance_example_biocompute_object',
          'Provenance-example-biocompute-object',
      ),
      ('with_provenance_example_cwl', 'Provenance-example-cwl'),
      ('with_provenance_signature', 'Provenance-signature'),
  )
  def test_json_format_for_valid_provenance_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Provenance)

  @parameterized.named_parameters(
      ('with_questionnaire3141', 'Questionnaire-3141'),
      ('with_questionnaire_bb', 'Questionnaire-bb'),
      ('with_questionnaire_f201', 'Questionnaire-f201'),
      ('with_questionnaire_gcs', 'Questionnaire-gcs'),
  )
  def test_json_format_for_valid_questionnaire_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Questionnaire)

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
        file_name, resources_pb2.QuestionnaireResponse)

  @parameterized.named_parameters(
      ('with_referral_request_example', 'ReferralRequest-example'),
  )
  def test_json_format_for_valid_referral_request_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ReferralRequest)

  @parameterized.named_parameters(
      ('with_related_person_benedicte', 'RelatedPerson-benedicte'),
      ('with_related_person_f001', 'RelatedPerson-f001'),
      ('with_related_person_f002', 'RelatedPerson-f002'),
      ('with_related_person_peter', 'RelatedPerson-peter'),
  )
  def test_json_format_for_valid_related_person_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.RelatedPerson)

  @parameterized.named_parameters(
      ('with_request_group_example', 'RequestGroup-example'),
      ('with_request_group_kdn5_example', 'RequestGroup-kdn5-example'),
  )
  def test_json_format_for_valid_request_group_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.RequestGroup)

  @parameterized.named_parameters(
      ('with_research_study_example', 'ResearchStudy-example'),
  )
  def test_json_format_for_valid_research_study_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.ResearchStudy)

  @parameterized.named_parameters(
      ('with_research_subject_example', 'ResearchSubject-example'),
  )
  def test_json_format_for_valid_research_subject_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ResearchSubject)

  @parameterized.named_parameters(
      ('with_risk_assessment_genetic', 'RiskAssessment-genetic'),
      ('with_risk_assessment_cardiac', 'RiskAssessment-cardiac'),
      ('with_risk_assessment_population', 'RiskAssessment-population'),
      ('with_risk_assessment_prognosis', 'RiskAssessment-prognosis'),
  )
  def test_json_format_for_valid_risk_assessment_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.RiskAssessment)

  @parameterized.named_parameters(
      ('with_schedule_example', 'Schedule-example'),
      ('with_schedule_exampleloc1', 'Schedule-exampleloc1'),
      ('with_schedule_exampleloc2', 'Schedule-exampleloc2'),
  )
  def test_json_format_for_valid_schedule_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Schedule)

  @parameterized.named_parameters(
      ('with_search_parameter_example', 'SearchParameter-example'),
      (
          'with_search_parameter_example_extension',
          'SearchParameter-example-extension',
      ),
      (
          'with_search_parameter_example_reference',
          'SearchParameter-example-reference',
      ),
  )
  def test_json_format_for_valid_search_parameter_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.SearchParameter)

  @parameterized.named_parameters(
      ('with_sequence_coord0_base', 'Sequence-coord-0-base'),
      ('with_sequence_coord1_base', 'Sequence-coord-1-base'),
      ('with_sequence_example', 'Sequence-example'),
      ('with_sequence_fda_example', 'Sequence-fda-example'),
      ('with_sequence_fda_vcf_comparison', 'Sequence-fda-vcf-comparison'),
      (
          'with_sequence_fda_vcfeval_comparison',
          'Sequence-fda-vcfeval-comparison',
      ),
      ('with_sequence_example_pgx1', 'Sequence-example-pgx-1'),
      ('with_sequence_example_pgx2', 'Sequence-example-pgx-2'),
      ('with_sequence_example_tpmt_one', 'Sequence-example-TPMT-one'),
      ('with_sequence_example_tpmt_two', 'Sequence-example-TPMT-two'),
      ('with_sequence_graphic_example1', 'Sequence-graphic-example-1'),
      ('with_sequence_graphic_example2', 'Sequence-graphic-example-2'),
      ('with_sequence_graphic_example3', 'Sequence-graphic-example-3'),
      ('with_sequence_graphic_example4', 'Sequence-graphic-example-4'),
      ('with_sequence_graphic_example5', 'Sequence-graphic-example-5'),
  )
  def test_json_format_for_valid_sequence_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Sequence)

  @parameterized.named_parameters(
      ('with_service_definition_example', 'ServiceDefinition-example'),
  )
  def test_json_format_for_valid_service_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ServiceDefinition)

  @parameterized.named_parameters(
      ('with_slot_example', 'Slot-example'),
      ('with_slot1', 'Slot-1'),
      ('with_slot2', 'Slot-2'),
      ('with_slot3', 'Slot-3'),
  )
  def test_json_format_for_valid_slot_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Slot)

  @parameterized.named_parameters(
      ('with_specimen101', 'Specimen-101'),
      ('with_specimen_isolate', 'Specimen-isolate'),
      ('with_specimen_sst', 'Specimen-sst'),
      ('with_specimen_vma_urine', 'Specimen-vma-urine'),
  )
  def test_json_format_for_valid_specimen_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Specimen)

  @parameterized.named_parameters(
      ('with_structure_definition_example', 'StructureDefinition-example'),
  )
  def test_json_format_for_valid_structure_definition_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.StructureDefinition)

  @parameterized.named_parameters(
      ('with_structure_map_example', 'StructureMap-example'),
  )
  def test_json_format_for_valid_structure_map_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.StructureMap)

  @parameterized.named_parameters(
      ('with_subscription_example', 'Subscription-example'),
      ('with_subscription_example_error', 'Subscription-example-error'),
  )
  def test_json_format_for_valid_subscription_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Subscription)

  @parameterized.named_parameters(
      ('with_substance_example', 'Substance-example'),
      ('with_substance_f205', 'Substance-f205'),
      ('with_substance_f201', 'Substance-f201'),
      ('with_substance_f202', 'Substance-f202'),
      ('with_substance_f203', 'Substance-f203'),
      ('with_substance_f204', 'Substance-f204'),
  )
  def test_json_format_for_valid_substance_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Substance)

  @parameterized.named_parameters(
      ('with_supply_delivery_simple_delivery', 'SupplyDelivery-simpledelivery'),
      ('with_supply_delivery_pump_delivery', 'SupplyDelivery-pumpdelivery'),
  )
  def test_json_format_for_valid_supply_delivery_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.SupplyDelivery)

  @parameterized.named_parameters(
      ('with_supply_request_simple_order', 'SupplyRequest-simpleorder'),
  )
  def test_json_format_for_valid_supply_request_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.SupplyRequest)

  @parameterized.named_parameters(
      ('with_task_example1', 'Task-example1'),
      ('with_task_example2', 'Task-example2'),
      ('with_task_example3', 'Task-example3'),
      ('with_task_example4', 'Task-example4'),
      ('with_task_example5', 'Task-example5'),
      ('with_task_example6', 'Task-example6'),
  )
  def test_json_format_for_valid_task_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Task)

  @parameterized.named_parameters(
      ('with_test_report_test_report_example', 'TestReport-testreport-example'),
  )
  def test_json_format_for_valid_test_report_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.TestReport)

  @parameterized.named_parameters(
      ('with_test_script_test_script_example', 'TestScript-testscript-example'),
      (
          'with_test_script_test_script_example_history',
          'TestScript-testscript-example-history',
      ),
      (
          'with_test_script_test_script_example_multisystem',
          'TestScript-testscript-example-multisystem',
      ),
      (
          'with_test_script_test_script_example_readtest',
          'TestScript-testscript-example-readtest',
      ),
      (
          'with_test_script_test_script_example_rule',
          'TestScript-testscript-example-rule',
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
                                                   resources_pb2.TestScript)

  @parameterized.named_parameters(
      ('with_value_set_example_expansion', 'ValueSet-example-expansion'),
      ('with_value_set_example_extensional', 'ValueSet-example-extensional'),
      ('with_value_set_example_intensional', 'ValueSet-example-intensional'),
  )
  def test_json_format_for_valid_value_set_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.ValueSet)

  @parameterized.named_parameters(
      ('with_vision_prescription33123', 'VisionPrescription-33123'),
      ('with_vision_prescription33124', 'VisionPrescription-33124'),
  )
  def test_json_format_for_valid_vision_prescription_succeeds(
      self, file_name: str
  ):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.VisionPrescription)

  @parameterized.named_parameters(
      (
          'with_composition_example',
          'Composition-example',
          resources_pb2.Composition,
      ),
      ('with_ecounter_home', 'Encounter-home', resources_pb2.Encounter),
      (
          'with_observation_example_genetics1',
          'Observation-example-genetics-1',
          resources_pb2.Observation,
      ),
      ('with_patient_example', 'patient-example', resources_pb2.Patient),
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

  def assert_parse_and_print_examples_equals_golden(
      self, file_name: str, proto_cls: Type[message.Message]) -> None:
    """Convenience method for performing assertions on FHIR STU3 examples."""
    json_path = os.path.join(_EXAMPLES_PATH, file_name + '.json')
    proto_path = os.path.join(_EXAMPLES_PATH, file_name + '.prototxt')
    self.assert_parse_and_print_equals_golden(json_path, proto_path, proto_cls)

  def assert_parse_and_print_spec_equals_golden(
      self, file_name: str, proto_cls: Type[message.Message]) -> None:
    """Convenience method for performing assertions on the FHIR STU3 spec."""
    json_path = os.path.join(_FHIR_SPEC_PATH, file_name + '.json')
    proto_path = os.path.join(_EXAMPLES_PATH, file_name + '.prototxt')
    self.assert_parse_and_print_equals_golden(json_path, proto_path, proto_cls)

  def assert_parse_and_print_equals_golden(
      self, json_path: str, proto_path: str,
      proto_cls: Type[message.Message]) -> None:
    """Convenience method for performing assertions against goldens."""
    # Assert parse
    self.assert_parse_equals_golden(
        json_path,
        proto_path,
        proto_cls,
        parse_f=json_format.json_fhir_string_to_proto,
        validate=True,
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
