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
from typing import TypeVar, Type

from google.protobuf import message
from absl.testing import absltest
from absl.testing import parameterized
from proto.google.fhir.proto.stu3 import datatypes_pb2
from proto.google.fhir.proto.stu3 import resources_pb2
from google.fhir.json_format import json_format_test
from google.fhir.stu3 import json_format
from google.fhir.testing import testdata_utils
from google.fhir.utils import proto_utils

_BIGQUERY_PATH = os.path.join('testdata', 'stu3', 'bigquery')
_EXAMPLES_PATH = os.path.join('testdata', 'stu3', 'examples')
_FHIR_SPEC_PATH = os.path.join('spec', 'hl7.fhir.core', '3.0.1', 'package')
_VALIDATION_PATH = os.path.join('testdata', 'stu3', 'validation')

_T = TypeVar('_T', bound=message.Message)


class JsonFormatTest(json_format_test.JsonFormatTest):
  """Unit tests for functionality in json_format.py."""

  @parameterized.named_parameters(
      ('_withAccountExample', 'Account-example'),
      ('_withAccountEwg', 'Account-ewg'),
  )
  def testJsonFormat_forValidAccount_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Account)

  @parameterized.named_parameters(
      ('_withActivityDefinitionReferralPrimaryCareMentalHealth',
       'ActivityDefinition-referralPrimaryCareMentalHealth'),
      ('_withActivityDefinitionCitalopramPrescription',
       'ActivityDefinition-citalopramPrescription'),
      ('_withActivityDefinitionReferralPrimaryCareMentalHealthInitial',
       'ActivityDefinition-referralPrimaryCareMentalHealth-initial'),
      ('_withActivityDefinitionHeartValveReplacement',
       'ActivityDefinition-heart-valve-replacement'),
      ('_withActivityDefinitionBloodTubesSupply',
       'ActivityDefinition-blood-tubes-supply'),
  )
  def testJsonFormat_forValidActivityDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ActivityDefinition)

  @parameterized.named_parameters(
      ('_withAdverseEventExample', 'AdverseEvent-example'),)
  def testJsonFormat_forValidAdverseEvent_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.AdverseEvent)

  @parameterized.named_parameters(
      ('_withAllergyIntoleranceExample', 'AllergyIntolerance-example'),)
  def testJsonFormat_forValidAllergyIntolerance_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.AllergyIntolerance)

  @parameterized.named_parameters(
      ('_withAppointmentExample', 'Appointment-example'),
      ('_withAppointment2docs', 'Appointment-2docs'),
      ('_withAppointmentExampleReq', 'Appointment-examplereq'),
  )
  def testJsonFormat_forValidAppointment_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Appointment)

  @parameterized.named_parameters(
      ('_withAppointmentResponseExample', 'AppointmentResponse-example'),
      ('_withAppointmentResponseExampleResp',
       'AppointmentResponse-exampleresp'),
  )
  def testJsonFormat_forValidAppointmentResponse_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.AppointmentResponse)

  @parameterized.named_parameters(
      ('_withAuditEventExample', 'AuditEvent-example'),
      ('_withAuditEventExampleDisclosure', 'AuditEvent-example-disclosure'),
      ('_withAuditEventExampleLogin', 'AuditEvent-example-login'),
      ('_withAuditEventExampleLogout', 'AuditEvent-example-logout'),
      ('_withAuditEventExampleMedia', 'AuditEvent-example-media'),
      ('_withAuditEventExamplePixQuery', 'AuditEvent-example-pixQuery'),
      ('_withAuditEventExampleSearch', 'AuditEvent-example-search'),
      ('_withAuditEventExampleRest', 'AuditEvent-example-rest'),
  )
  def testJsonFormat_forValidAuditEvent_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.AuditEvent)

  @parameterized.named_parameters(
      ('_withBasicReferral', 'Basic-referral'),
      ('_withBasicClassModel', 'Basic-classModel'),
      ('_withBasicBasicExampleNarrative', 'Basic-basic-example-narrative'),
  )
  def testJsonFormat_forValidBasic_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Basic)

  @parameterized.named_parameters(
      ('_withBodySiteFetus', 'BodySite-fetus'),
      ('_withBodySiteSkinPatch', 'BodySite-skin-patch'),
      ('_withBodySiteTumor', 'BodySite-tumor'),
  )
  def testJsonFormat_forValidBodySite_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.BodySite)

  @parameterized.named_parameters(
      ('_withBundleBundleExample', 'Bundle-bundle-example'),
      ('_withBundle72ac849352ac41bd8d5d7258c289b5ea',
       'Bundle-72ac8493-52ac-41bd-8d5d-7258c289b5ea'),
      ('_withBundleHla1', 'Bundle-hla-1'),
      ('_withBundleFather', 'Bundle-father'),
      ('_withBundleB0a5e427783c44adb87e2E3efe3369b6f',
       'Bundle-b0a5e4277-83c4-4adb-87e2-e3efe3369b6f'),
      ('_withBundle3ad0687eF477468cAfd5Fcc2bf897819',
       'Bundle-3ad0687e-f477-468c-afd5-fcc2bf897819'),
      ('_withPatientExamplesCypressTemplate',
       'patient-examples-cypress-template'),
      ('_withBundleB248b1b216864b94993637d7a5f94b51',
       'Bundle-b248b1b2-1686-4b94-9936-37d7a5f94b51'),
      ('_withBundle3ad0687eF477468cAfd5Fcc2bf897809',
       'Bundle-3ad0687e-f477-468c-afd5-fcc2bf897809'),
      ('_withBundle3ad0687eF477468cAfd5Fcc2bf897808',
       'Bundle-3ad0687e-f477-468c-afd5-fcc2bf897808'),
      ('_withBundleUssgFht', 'Bundle-ussg-fht'),
      ('_withBundleXds', 'Bundle-xds'),
  )
  def testJsonFormat_forValidBundle_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Bundle)

  @parameterized.named_parameters(
      ('_withCapabilityStatementExample', 'CapabilityStatement-example'),
      ('_withCapabilityStatementPhr', 'CapabilityStatement-phr'),
  )
  def testJsonFormat_forValidCapabilityStatement_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.CapabilityStatement)

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
                                                   resources_pb2.CarePlan)

  @parameterized.named_parameters(
      ('_withCareTeamExample', 'CareTeam-example'),)
  def testJsonFormat_forValidCareTeam_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.CareTeam)

  @parameterized.named_parameters(
      ('_withChargeItemExample', 'ChargeItem-example'),)
  def testJsonFormat_forValidChargeItem_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.ChargeItem)

  @parameterized.named_parameters(
      ('_withClaim100150', 'Claim-100150'),
      ('_withClaim960150', 'Claim-960150'),
      ('_withClaim960151', 'Claim-960151'),
      ('_withClaim100151', 'Claim-100151'),
      ('_withClaim100156', 'Claim-100156'),
      ('_withClaim100152', 'Claim-100152'),
      ('_withClaim100155', 'Claim-100155'),
      ('_withClaim100154', 'Claim-100154'),
      ('_withClaim100153', 'Claim-100153'),
      ('_withClaim760150', 'Claim-760150'),
      ('_withClaim760152', 'Claim-760152'),
      ('_withClaim760151', 'Claim-760151'),
      ('_withClaim860150', 'Claim-860150'),
      ('_withClaim660150', 'Claim-660150'),
      ('_withClaim660151', 'Claim-660151'),
      ('_withClaim660152', 'Claim-660152'),
  )
  def testJsonFormat_forValidClaim_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Claim)

  @parameterized.named_parameters(
      ('_withClaimResponseR3500', 'ClaimResponse-R3500'),)
  def testJsonFormat_forValidClaimResponse_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.ClaimResponse)

  @parameterized.named_parameters(
      ('_withClinicalImpressionExample', 'ClinicalImpression-example'),)
  def testJsonFormat_forValidClinicalImpression_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ClinicalImpression)

  @parameterized.named_parameters(
      ('_withCodeSystemExample', 'CodeSystem-example'),
      ('_withCodeSystemListExampleCodes', 'CodeSystem-list-example-codes'),
  )
  def testJsonFormat_forValidCodeSystem_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.CodeSystem)

  @parameterized.named_parameters(
      ('_withCommunicationExample', 'Communication-example'),
      ('_withCommunicationFmAttachment', 'Communication-fm-attachment'),
      ('_withCommunicationFmSolicited', 'Communication-fm-solicited'),
  )
  def testJsonFormat_forValidCommunication_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Communication)

  @parameterized.named_parameters(
      ('_withCommunicationRequestExample', 'CommunicationRequest-example'),
      ('_withCommunicationRequestFmSolicit', 'CommunicationRequest-fm-solicit'),
  )
  def testJsonFormat_forValidCommunicationRequest_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.CommunicationRequest)

  @parameterized.named_parameters(
      ('_withCompartmentDefinitionExample', 'CompartmentDefinition-example'),)
  def testJsonFormat_forValidCompartmentDefinition_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.CompartmentDefinition)

  @parameterized.named_parameters(
      ('_withCompositionExample', 'Composition-example'),)
  def testJsonFormat_forValidComposition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Composition)

  @parameterized.named_parameters(
      ('_withConceptmapExample', 'conceptmap-example'),
      ('_withConceptmapExample2', 'conceptmap-example-2'),
      ('_withConceptmapExampleSpecimenType',
       'conceptmap-example-specimen-type'),
  )
  def testJsonFormat_forValidConceptMap_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.ConceptMap)

  @parameterized.named_parameters(
      ('_withConditionExample', 'Condition-example'),
      ('_withConditionExample2', 'Condition-example2'),
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
                                                   resources_pb2.Condition)

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
                                                   resources_pb2.Consent)

  @parameterized.named_parameters(
      ('_withAllergyIntoleranceExample', 'AllergyIntolerance-example',
       resources_pb2.AllergyIntolerance, 'allergy_intolerance'),
      ('_withCapabilityStatementExample', 'CapabilityStatement-example',
       resources_pb2.CapabilityStatement, 'capability_statement'),
      ('_withImmunizationExample', 'Immunization-example',
       resources_pb2.Immunization, 'immunization'),
      ('_withMedicationMed0305', 'Medication-med0305', resources_pb2.Medication,
       'medication'),
      ('_withObservationF004', 'Observation-f004', resources_pb2.Observation,
       'observation'),
      ('_withPatientExample', 'patient-example', resources_pb2.Patient,
       'patient'),
      ('_withPractitionerF003', 'Practitioner-f003', resources_pb2.Practitioner,
       'practitioner'),
      ('_withProcedureAmbulation', 'Procedure-ambulation',
       resources_pb2.Procedure, 'procedure'),
      ('_withTaskExample4', 'Task-example4', resources_pb2.Task, 'task'),
  )
  def testJsonFormat_forValidContainedResource_succeeds(
      self, file_name: str, proto_cls: Type[message.Message],
      contained_field: str):
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
      ('_withContractC123', 'Contract-C-123'),
      ('_withContractC2121', 'Contract-C-2121'),
      ('_withContractPcdExampleNotAuthor', 'Contract-pcd-example-notAuthor'),
      ('_withContractPcdExampleNotLabs', 'Contract-pcd-example-notLabs'),
      ('_withContractPcdExampleNotOrg', 'Contract-pcd-example-notOrg'),
      ('_withContractPcdExampleNotThem', 'Contract-pcd-example-notThem'),
      ('_withContractPcdExampleNotThis', 'Contract-pcd-example-notThis'),
  )
  def testJsonFormat_forValidContract_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Contract)

  @parameterized.named_parameters(
      ('_withCoverage9876B1', 'Coverage-9876B1'),
      ('_withCoverage7546D', 'Coverage-7546D'),
      ('_withCoverage7547E', 'Coverage-7547E'),
      ('_withCoverageSP1234', 'Coverage-SP1234'),
  )
  def testJsonFormat_forValidCoverage_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Coverage)

  @parameterized.named_parameters(
      ('_withDataElementGender', 'DataElement-gender'),
      ('_withDataElementProthrombin', 'DataElement-prothrombin'),
  )
  def testJsonFormat_forValidDataElement_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.DataElement)

  @parameterized.named_parameters(
      ('_withDetectedIssueDdi', 'DetectedIssue-ddi'),
      ('_withDetectedIssueAllergy', 'DetectedIssue-allergy'),
      ('_withDetectedIssueDuplicate', 'DetectedIssue-duplicate'),
      ('_withDetectedIssueLab', 'DetectedIssue-lab'),
  )
  def testJsonFormat_forValidDetectedIssue_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.DetectedIssue)

  @parameterized.named_parameters(
      ('_withDeviceExample', 'Device-example'),
      ('_withDeviceF001', 'Device-f001'),
      ('_withDeviceIhePcd', 'Device-ihe-pcd'),
      ('_withDeviceExamplePacemaker', 'Device-example-pacemaker'),
      ('_withDeviceSoftware', 'Device-software'),
      ('_withDeviceExampleUdi1', 'Device-example-udi1'),
      ('_withDeviceExampleUdi2', 'Device-example-udi2'),
      ('_withDeviceExampleUdi3', 'Device-example-udi3'),
      ('_withDeviceExampleUdi4', 'Device-example-udi4'),
  )
  def testJsonFormat_forValidDevice_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Device)

  @parameterized.named_parameters(
      ('_withDeviceComponentExample', 'DeviceComponent-example'),
      ('_withDeviceComponentExampleProdspec',
       'DeviceComponent-example-prodspec'),
  )
  def testJsonFormat_forValidDeviceComponent_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.DeviceComponent)

  @parameterized.named_parameters(
      ('_withDeviceMetricExample', 'DeviceMetric-example'),)
  def testJsonFormat_forValidDeviceMetric_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.DeviceMetric)

  @parameterized.named_parameters(
      ('_withDeviceRequestExample', 'DeviceRequest-example'),
      ('_withDeviceRequestInsulinPump', 'DeviceRequest-insulinpump'),
  )
  def testJsonFormat_forValidDeviceRequest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.DeviceRequest)

  @parameterized.named_parameters(
      ('_withDeviceUseStatementExample', 'DeviceUseStatement-example'),)
  def testJsonFormat_forValidDeviceUseStatement_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.DeviceUseStatement)

  @parameterized.named_parameters(
      ('_withDiagnosticReport101', 'DiagnosticReport-101'),
      ('_withDiagnosticReport102', 'DiagnosticReport-102'),
      ('_withDiagnosticReportF001', 'DiagnosticReport-f001'),
      ('_withDiagnosticReportF201', 'DiagnosticReport-f201'),
      ('_withDiagnosticReportF202', 'DiagnosticReport-f202'),
      ('_withDiagnosticReportGhp', 'DiagnosticReport-ghp'),
      ('_withDiagnosticReportGingivalMass', 'DiagnosticReport-gingival-mass'),
      ('_withDiagnosticReportLipids', 'DiagnosticReport-lipids'),
      ('_withDiagnosticReportPap', 'DiagnosticReport-pap'),
      ('_withDiagnosticReportExamplePgx', 'DiagnosticReport-example-pgx'),
      ('_withDiagnosticReportUltrasound', 'DiagnosticReport-ultrasound'),
      ('_withDiagnosticReportDg2', 'DiagnosticReport-dg2'),
  )
  def testJsonFormat_forValidDiagnosticReport_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.DiagnosticReport)

  @parameterized.named_parameters(
      ('_withDocumentManifestExample', 'DocumentManifest-example'),)
  def testJsonFormat_forValidDocumentManifest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.DocumentManifest)

  @parameterized.named_parameters(
      ('_withDocumentReferenceExample', 'DocumentReference-example'),)
  def testJsonFormat_forValidDocumentReference_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.DocumentReference)

  @parameterized.named_parameters(
      ('_withEligibilityRequest52345', 'EligibilityRequest-52345'),
      ('_withEligibilityRequest52346', 'EligibilityRequest-52346'),
  )
  def testJsonFormat_forValidEligibilityRequest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.EligibilityRequest)

  @parameterized.named_parameters(
      ('_withEligibilityResponseE2500', 'EligibilityResponse-E2500'),
      ('_withEligibilityResponseE2501', 'EligibilityResponse-E2501'),
      ('_withEligibilityResponseE2502', 'EligibilityResponse-E2502'),
      ('_withEligibilityResponseE2503', 'EligibilityResponse-E2503'),
  )
  def testJsonFormat_forValidEligibilityResponse_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.EligibilityResponse)

  @parameterized.named_parameters(
      ('_withParametersEmptyResource', 'Parameters-empty-resource',
       resources_pb2.Parameters),)
  def testJsonFormat_forValidEmptyNestedResource_succeeds(
      self, file_name: str, proto_cls: Type[message.Message]):
    self.assert_parse_and_print_examples_equals_golden(file_name, proto_cls)

  @parameterized.named_parameters(
      ('_withEncounterExample', 'Encounter-example'),
      ('_withEncounterEmerg', 'Encounter-emerg'),
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
                                                   resources_pb2.Encounter)

  @parameterized.named_parameters(
      ('_withEndpointExample', 'Endpoint-example'),
      ('_withEndpointExampleIid', 'Endpoint-example-iid'),
      ('_withEndpointExampleWadors', 'Endpoint-example-wadors'),
  )
  def testJsonFormat_forValidEndpoint_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Endpoint)

  @parameterized.named_parameters(
      ('_withEnrollmentRequest22345', 'EnrollmentRequest-22345'),)
  def testJsonFormat_forValidEnrollmentRequest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.EnrollmentRequest)

  @parameterized.named_parameters(
      ('_withEnrollmentResponseEr2500', 'EnrollmentResponse-ER2500'),)
  def testJsonFormat_forValidEnrollmentResponse_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.EnrollmentResponse)

  @parameterized.named_parameters(
      ('_withEpisodeOfCareExample', 'EpisodeOfCare-example'),)
  def testJsonFormat_forValidEpisodeOfCare_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.EpisodeOfCare)

  @parameterized.named_parameters(
      ('_withExpansionProfileExample', 'ExpansionProfile-example'),)
  def testJsonFormat_forValidExpansionProfile_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ExpansionProfile)

  @parameterized.named_parameters(
      ('_withExplanationOfBenefitEb3500', 'ExplanationOfBenefit-EB3500'),)
  def testJsonFormat_forValidExplanationOfBenefit_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ExplanationOfBenefit)

  @parameterized.named_parameters(
      ('_withFamilyMemberHistoryFather', 'FamilyMemberHistory-father'),
      ('_withFamilyMemberHistoryMother', 'FamilyMemberHistory-mother'),
  )
  def testJsonFormat_forValidFamilyMemberHistory_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.FamilyMemberHistory)

  @parameterized.named_parameters(
      ('_withFlagExample', 'Flag-example'),
      ('_withFlagExampleEncounter', 'Flag-example-encounter'),
  )
  def testJsonFormat_forValidFlag_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Flag)

  @parameterized.named_parameters(
      ('_withGoalExample', 'Goal-example'),
      ('_withGoalStopSmoking', 'Goal-stop-smoking'),
  )
  def testJsonFormat_forValidGoal_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Goal)

  @parameterized.named_parameters(
      ('_withGraphDefinitionExample', 'GraphDefinition-example'),)
  def testJsonFormat_forValidGraphDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.GraphDefinition)

  @parameterized.named_parameters(
      ('_withGroup101', 'Group-101'),
      ('_withGroup102', 'Group-102'),
  )
  def testJsonFormat_forValidGroup_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Group)

  @parameterized.named_parameters(
      ('_withGuidanceResponseExample', 'GuidanceResponse-example'),)
  def testJsonFormat_forValidGuidanceResponse_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.GuidanceResponse)

  @parameterized.named_parameters(
      ('_withHealthcareServiceExample', 'HealthcareService-example'),)
  def testJsonFormat_forValidHealthcareService_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.HealthcareService)

  @parameterized.named_parameters(
      ('_withImagingManifestExample', 'ImagingManifest-example'),)
  def testJsonFormat_forValidImagingManifest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ImagingManifest)

  @parameterized.named_parameters(
      ('_withImagingStudyExample', 'ImagingStudy-example'),
      ('_withImagingStudyExampleXr', 'ImagingStudy-example-xr'),
  )
  def testJsonFormat_forValidImagingStudy_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.ImagingStudy)

  @parameterized.named_parameters(
      ('_withImmunizationExample', 'Immunization-example'),
      ('_withImmunizationHistorical', 'Immunization-historical'),
      ('_withImmunizationNotGiven', 'Immunization-notGiven'),
  )
  def testJsonFormat_forValidImmunization_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Immunization)

  @parameterized.named_parameters(
      ('_withImmunizationRecommendationExample',
       'ImmunizationRecommendation-example'),
      ('_withImmunizationRecommendationTargetDiseaseExample',
       'immunizationrecommendation-target-disease-example'),
  )
  def testJsonFormat_forValidImmunizationRecommendation_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ImmunizationRecommendation)

  @parameterized.named_parameters(
      ('_withImplementationGuideExample', 'ImplementationGuide-example'),)
  def testJsonFormat_forValidImplementationGuide_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ImplementationGuide)

  @parameterized.named_parameters(
      ('_withLibraryLibraryCms146Example', 'Library-library-cms146-example'),
      ('_withLibraryCompositionExample', 'Library-composition-example'),
      ('_withLibraryExample', 'Library-example'),
      ('_withLibraryLibraryFhirHelpersPredecessor',
       'Library-library-fhir-helpers-predecessor'),
  )
  def testJsonFormat_forValidLibrary_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Library)

  @parameterized.named_parameters(
      ('_withLinkageExample', 'Linkage-example'),)
  def testJsonFormat_forValidLinkage_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Linkage)

  @parameterized.named_parameters(
      ('_withListExample', 'List-example'),
      ('_withListCurrentAllergies', 'List-current-allergies'),
      ('_withListExampleDoubleCousinRelationship',
       'List-example-double-cousin-relationship'),
      ('_withListExampleEmpty', 'List-example-empty'),
      ('_withListF201', 'List-f201'),
      ('_withListGenetic', 'List-genetic'),
      ('_withListPrognosis', 'List-prognosis'),
      ('_withListMedList', 'List-med-list'),
      ('_withListExampleSimpleEmpty', 'List-example-simple-empty'),
  )
  def testJsonFormat_forValidList_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.List)

  @parameterized.named_parameters(
      ('_withLocation1', 'Location-1'),
      ('_withLocationAmb', 'Location-amb'),
      ('_withLocationHl7', 'Location-hl7'),
      ('_withLocationPh', 'Location-ph'),
      ('_withLocation2', 'Location-2'),
      ('_withLocationUkp', 'Location-ukp'),
  )
  def testJsonFormat_forValidLocation_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Location)

  @parameterized.named_parameters(
      ('_withMeasureMeasureCms146Example', 'Measure-measure-cms146-example'),
      ('_withMeasureComponentAExample', 'Measure-component-a-example'),
      ('_withMeasureComponentBExample', 'Measure-component-b-example'),
      ('_withMeasureCompositeExample', 'Measure-composite-example'),
      ('_withMeasureMeasurePredecessorExample',
       'Measure-measure-predecessor-example'),
  )
  def testJsonFormat_forValidMeasure_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Measure)

  @parameterized.named_parameters(
      ('_withMeasureReportMeasureReportCms146Cat1Example',
       'MeasureReport-measurereport-cms146-cat1-example'),
      ('_withMeasureReportMeasurereportCms146Cat2Example',
       'MeasureReport-measurereport-cms146-cat2-example'),
      ('_withMeasureReportMeasurereportCms146Cat3Example',
       'MeasureReport-measurereport-cms146-cat3-example'),
  )
  def testJsonFormat_forValidMeasureReport_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.MeasureReport)

  @parameterized.named_parameters(
      ('_withMediaExample', 'Media-example'),
      ('_withMedia1_2_840_11361907579238403408700_3_0_14_19970327150033',
       'Media-1.2.840.11361907579238403408700.3.0.14.19970327150033'),
      ('_withMediaSound', 'Media-sound'),
      ('_withMediaXray', 'Media-xray'),
  )
  def testJsonFormat_forValidMedia_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Media)

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
      ('_withMedicationMedicationExample1', 'Medication-medicationexample1'),
      ('_withMedicationMedExample015', 'Medication-medexample015'),
  )
  def testJsonFormat_forValidMedication_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Medication)

  @parameterized.named_parameters(
      ('_withMedicationAdministrationMedadminExample03',
       'MedicationAdministration-medadminexample03'),)
  def testJsonFormat_forValidMedicationAdministration_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.MedicationAdministration)

  @parameterized.named_parameters(
      ('_withMedicationDispenseMeddisp008', 'MedicationDispense-meddisp008'),)
  def testJsonFormat_forValidMedicationDispense_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.MedicationDispense)

  @parameterized.named_parameters(
      ('_withMedicationRequestMedrx0311', 'MedicationRequest-medrx0311'),
      ('_withMedicationRequestMedrx002', 'MedicationRequest-medrx002'),
  )
  def testJsonFormat_forValidMedicationRequest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.MedicationRequest)

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
        file_name, resources_pb2.MedicationStatement)

  @parameterized.named_parameters(
      ('_withMessageDefinitionExample', 'MessageDefinition-example'),)
  def testJsonFormat_forValidMessageDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.MessageDefinition)

  @parameterized.named_parameters(
      ('_withMessageHeader1cbdfb97585948a48301D54eab818d68',
       'MessageHeader-1cbdfb97-5859-48a4-8301-d54eab818d68'),)
  def testJsonFormat_forValidMessageHeader_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.MessageHeader)

  @parameterized.named_parameters(
      ('_withNamingSystemExample', 'NamingSystem-example'),
      ('_withNamingSystemExampleId', 'NamingSystem-example-id'),
      ('_withNamingSystemExampleReplaced', 'NamingSystem-example-replaced'),
  )
  def testJsonFormat_forValidNamingSystem_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.NamingSystem)

  @parameterized.named_parameters(
      ('_withNutritionOrderCardiacDiet', 'NutritionOrder-cardiacdiet'),
      ('_withNutritionOrderDiabeticDiet', 'NutritionOrder-diabeticdiet'),
      ('_withNutritionOrderDiabeticSupplement',
       'NutritionOrder-diabeticsupplement'),
      ('_withNutritionOrderEnergySupplement',
       'NutritionOrder-energysupplement'),
      ('_withNutritionOrderEnteralbolus', 'NutritionOrder-enteralbolus'),
      ('_withNutritionOrderEnteralContinuous',
       'NutritionOrder-enteralcontinuous'),
      ('_withNutritionOrderFiberRestrictedDiet',
       'NutritionOrder-fiberrestricteddiet'),
      ('_withNutritionOrderInfantenteral', 'NutritionOrder-infantenteral'),
      ('_withNutritionOrderProteinSupplement',
       'NutritionOrder-proteinsupplement'),
      ('_withNutritionOrderPureedDiet', 'NutritionOrder-pureeddiet'),
      ('_withNutritionOrderPureeddietSimple',
       'NutritionOrder-pureeddiet-simple'),
      ('_withNutritionOrderRenalDiet', 'NutritionOrder-renaldiet'),
      ('_withNutritionOrderTextureModified', 'NutritionOrder-texturemodified'),
  )
  def testJsonFormat_forValidNutritionOrder_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.NutritionOrder)

  @parameterized.named_parameters(
      ('_withObservationExample', 'Observation-example'),
      ('_withObservation10MinuteApgarScore',
       'Observation-10minute-apgar-score'),
      ('_withObservation1MinuteApgarScore', 'Observation-1minute-apgar-score'),
      ('_withObservation20MinuteApgarScore',
       'Observation-20minute-apgar-score'),
      ('_withObservation2MinuteApgarScore', 'Observation-2minute-apgar-score'),
      ('_withObservation5MinuteApgarScore', 'Observation-5minute-apgar-score'),
      ('_withObservationBloodPressure', 'Observation-blood-pressure'),
      ('_withObservationBloodPressureCancel',
       'Observation-blood-pressure-cancel'),
      ('_withObservationBloodPressureDar', 'Observation-blood-pressure-dar'),
      ('_withObservationBmd', 'Observation-bmd'),
      ('_withObservationBmi', 'Observation-bmi'),
      ('_withObservationBodyHeight', 'Observation-body-height'),
      ('_withObservationBodyLength', 'Observation-body-length'),
      ('_withObservationBodyTemperature', 'Observation-body-temperature'),
      ('_withObservationDateLastmp', 'Observation-date-lastmp'),
      ('_withObservationExampleDiplotype1', 'Observation-example-diplotype1'),
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
      ('_withObservationExampleGenetics1', 'Observation-example-genetics-1'),
      ('_withObservationExampleGenetics2', 'Observation-example-genetics-2'),
      ('_withObservationExampleGenetics3', 'Observation-example-genetics-3'),
      ('_withObservationExampleGenetics4', 'Observation-example-genetics-4'),
      ('_withObservationExampleGenetics5', 'Observation-example-genetics-5'),
      ('_withObservationGlasgow', 'Observation-glasgow'),
      ('_withObservationGcsQa', 'Observation-gcs-qa'),
      ('_withObservationExampleHaplotype1', 'Observation-example-haplotype1'),
      ('_withObservationExampleHaplotype2', 'Observation-example-haplotype2'),
      ('_withObservationHeadCircumference', 'Observation-head-circumference'),
      ('_withObservationHeartRate', 'Observation-heart-rate'),
      ('_withObservationMbp', 'Observation-mbp'),
      ('_withObservationExamplePhenotype', 'Observation-example-phenotype'),
      ('_withObservationRespiratoryRate', 'Observation-respiratory-rate'),
      ('_withObservationEkg', 'Observation-ekg'),
      ('_withObservationSatO2', 'Observation-satO2'),
      ('_withObservationExampleTpmtDiplotype',
       'Observation-example-TPMT-diplotype'),
      ('_withObservationExampleTpmtHaplotypeOne',
       'Observation-example-TPMT-haplotype-one'),
      ('_withObservationExampleTpmtHaplotypeTwo',
       'Observation-example-TPMT-haplotype-two'),
      ('_withObservationUnsat', 'Observation-unsat'),
      ('_withObservationVitalsPanel', 'Observation-vitals-panel'),
  )
  def testJsonFormat_forValidObservation_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Observation)

  @parameterized.named_parameters(
      ('_withOperationDefinitionExample', 'OperationDefinition-example'),)
  def testJsonFormat_forValidOperationDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.OperationDefinition)

  @parameterized.named_parameters(
      ('_withOperationOutcome101', 'OperationOutcome-101'),
      ('_withOperationOutcomeAllok', 'OperationOutcome-allok'),
      ('_withOperationOutcomeBreakTheGlass',
       'OperationOutcome-break-the-glass'),
      ('_withOperationOutcomeException', 'OperationOutcome-exception'),
      ('_withOperationOutcomeSearchFail', 'OperationOutcome-searchfail'),
      ('_withOperationOutcomeValidationFail',
       'OperationOutcome-validationfail'),
  )
  def testJsonFormat_forValidOperationOutcome_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.OperationOutcome)

  @parameterized.named_parameters(
      ('_withOrganizationHl7', 'Organization-hl7'),
      ('_withOrganizationF001', 'Organization-f001'),
      ('_withOrganizationF002', 'Organization-f002'),
      ('_withOrganizationF003', 'Organization-f003'),
      ('_withOrganizationF201', 'Organization-f201'),
      ('_withOrganizationF203', 'Organization-f203'),
      ('_withOrganization1', 'Organization-1'),
      ('_withOrganization2_16_840_1_113883_19_5',
       'Organization-2.16.840.1.113883.19.5'),
      ('_withOrganization2', 'Organization-2'),
      ('_withOrganization1832473e2fe0452dAbe93cdb9879522f',
       'Organization-1832473e-2fe0-452d-abe9-3cdb9879522f'),
      ('_withOrganizationMmanu', 'Organization-mmanu'),
  )
  def testJsonFormat_forValidOrganization_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Organization)

  @parameterized.named_parameters(
      ('_withParametersExample', 'Parameters-example'),)
  def testJsonFormat_forValidParameters_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Parameters)

  @parameterized.named_parameters(
      ('_withPatientExample', 'patient-example'),
      ('_withPatientExampleA', 'patient-example-a'),
      ('_withPatientExampleAnimal', 'patient-example-animal'),
      ('_withPatientExampleB', 'patient-example-b'),
      ('_withPatientExampleC', 'patient-example-c'),
      ('_withPatientExampleChinese', 'patient-example-chinese'),
      ('_withPatientExampleD', 'patient-example-d'),
      ('_withPatientExampleDicom', 'patient-example-dicom'),
      ('_withPatientExampleF001Pieter', 'patient-example-f001-pieter'),
      ('_withPatientExampleF201Roel', 'patient-example-f201-roel'),
      ('_withPatientExampleIhePcd', 'patient-example-ihe-pcd'),
      ('_withPatientExampleProband', 'patient-example-proband'),
      ('_withPatientExampleXcda', 'patient-example-xcda'),
      ('_withPatientExampleXds', 'patient-example-xds'),
      ('_withPatientGeneticsExample1', 'patient-genetics-example1'),
      ('_withPatientGlossyExample', 'patient-glossy-example'),
  )
  def testJsonFormat_forValidPatient_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Patient)

  @parameterized.named_parameters(
      ('_withPaymentNotice77654', 'PaymentNotice-77654'),)
  def testJsonFormat_forValidPaymentNotice_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.PaymentNotice)

  @parameterized.named_parameters(
      ('_withPaymentReconciliationER2500', 'PaymentReconciliation-ER2500'),)
  def testJsonFormat_forValidPaymentReconciliation_succeeds(
      self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.PaymentReconciliation)

  @parameterized.named_parameters(
      ('_withPersonExample', 'Person-example'),
      ('_withPersonF002', 'Person-f002'),
  )
  def testJsonFormat_forValidPerson_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Person)

  @parameterized.named_parameters(
      ('_withPlanDefinitionLowSuicideRiskOrderSet',
       'PlanDefinition-low-suicide-risk-order-set'),
      ('_withPlanDefinitionKdn5', 'PlanDefinition-KDN5'),
      ('_withPlanDefinitionOptionsExample', 'PlanDefinition-options-example'),
      ('_withPlanDefinitionZikaVirusInterventionInitial',
       'PlanDefinition-zika-virus-intervention-initial'),
      ('_withPlanDefinitionProtocolExample', 'PlanDefinition-protocol-example'),
  )
  def testJsonFormat_forValidPlanDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.PlanDefinition)

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
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Practitioner)

  @parameterized.named_parameters(
      ('_withPractitionerRoleExample', 'PractitionerRole-example'),)
  def testJsonFormat_forValidPractitionerRole_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.PractitionerRole)

  @parameterized.named_parameters(
      ('_withBase64Binary', 'base64_binary', datatypes_pb2.Base64Binary),
      ('_withBoolean', 'boolean', datatypes_pb2.Boolean),
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
      ('_withProcedureExample', 'Procedure-example'),
      ('_withProcedureAmbulation', 'Procedure-ambulation'),
      ('_withProcedureAppendectomyNarrative',
       'Procedure-appendectomy-narrative'),
      ('_withProcedureBiopsy', 'Procedure-biopsy'),
      ('_withProcedureColonBiopsy', 'Procedure-colon-biopsy'),
      ('_withProcedureColonoscopy', 'Procedure-colonoscopy'),
      ('_withProcedureEducation', 'Procedure-education'),
      ('_withProcedureF001', 'Procedure-f001'),
      ('_withProcedureF002', 'Procedure-f002'),
      ('_withProcedureF003', 'Procedure-f003'),
      ('_withProcedureF004', 'Procedure-f004'),
      ('_withProcedureF201', 'Procedure-f201'),
      ('_withProcedureExampleImplant', 'Procedure-example-implant'),
      ('_withProcedureOb', 'Procedure-ob'),
      ('_withProcedurePhysicalTherapy', 'Procedure-physical-therapy'),
  )
  def testJsonFormat_forValidProcedure_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Procedure)

  @parameterized.named_parameters(
      ('_withProcedureRequestExample', 'ProcedureRequest-example'),
      ('_withProcedureRequestPhysiotherapy', 'ProcedureRequest-physiotherapy'),
      ('_withProcedureRequestDoNotTurn', 'ProcedureRequest-do-not-turn'),
      ('_withProcedureRequestBenchpress', 'ProcedureRequest-benchpress'),
      ('_withProcedureRequestAmbulation', 'ProcedureRequest-ambulation'),
      ('_withProcedureRequestAppendectomyNarrative',
       'ProcedureRequest-appendectomy-narrative'),
      ('_withProcedureRequestColonoscopy', 'ProcedureRequest-colonoscopy'),
      ('_withProcedureRequestColonBiopsy', 'ProcedureRequest-colon-biopsy'),
      ('_withProcedureRequestDi', 'ProcedureRequest-di'),
      ('_withProcedureRequestEducation', 'ProcedureRequest-education'),
      ('_withProcedureRequestFt4', 'ProcedureRequest-ft4'),
      ('_withProcedureRequestExampleImplant',
       'ProcedureRequest-example-implant'),
      ('_withProcedureRequestLipid', 'ProcedureRequest-lipid'),
      ('_withProcedureRequestOb', 'ProcedureRequest-ob'),
      ('_withProcedureRequestExamplePgx', 'ProcedureRequest-example-pgx'),
      ('_withProcedureRequestPhysicalTherapy',
       'ProcedureRequest-physical-therapy'),
      ('_withProcedureRequestSubrequest', 'ProcedureRequest-subrequest'),
      ('_withProcedureRequestOgExample1', 'ProcedureRequest-og-example1'),
  )
  def testJsonFormat_forValidProcedureRequest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ProcedureRequest)

  @parameterized.named_parameters(
      ('_withProcessRequest1110', 'ProcessRequest-1110'),
      ('_withProcessRequest1115', 'ProcessRequest-1115'),
      ('_withProcessRequest1113', 'ProcessRequest-1113'),
      ('_withProcessRequest1112', 'ProcessRequest-1112'),
      ('_withProcessRequest1114', 'ProcessRequest-1114'),
      ('_withProcessRequest1111', 'ProcessRequest-1111'),
      ('_withProcessRequest44654', 'ProcessRequest-44654'),
      ('_withProcessRequest87654', 'ProcessRequest-87654'),
      ('_withProcessRequest87655', 'ProcessRequest-87655'),
  )
  def testJsonFormat_forValidProcessRequest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.ProcessRequest)

  @parameterized.named_parameters(
      ('_withProcessResponseSR2500', 'ProcessResponse-SR2500'),
      ('_withProcessResponseSR2349', 'ProcessResponse-SR2349'),
      ('_withProcessResponseSR2499', 'ProcessResponse-SR2499'),
  )
  def testJsonFormat_forValidProcessResponse_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ProcessResponse)

  @parameterized.named_parameters(
      ('_withProvenanceExample', 'Provenance-example'),
      ('_withProvenanceExampleBiocomputeObject',
       'Provenance-example-biocompute-object'),
      ('_withProvenanceExampleCwl', 'Provenance-example-cwl'),
      ('_withProvenanceSignature', 'Provenance-signature'),
  )
  def testJsonFormat_forValidProvenance_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Provenance)

  @parameterized.named_parameters(
      ('_withQuestionnaire3141', 'Questionnaire-3141'),
      ('_withQuestionnaireBb', 'Questionnaire-bb'),
      ('_withQuestionnaireF201', 'Questionnaire-f201'),
      ('_withQuestionnaireGcs', 'Questionnaire-gcs'),
  )
  def testJsonFormat_forValidQuestionnaire_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Questionnaire)

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
        file_name, resources_pb2.QuestionnaireResponse)

  @parameterized.named_parameters(
      ('_withReferralRequestExample', 'ReferralRequest-example'),)
  def testJsonFormat_forValidReferralRequest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ReferralRequest)

  @parameterized.named_parameters(
      ('_withRelatedPersonBenedicte', 'RelatedPerson-benedicte'),
      ('_withRelatedPersonF001', 'RelatedPerson-f001'),
      ('_withRelatedPersonF002', 'RelatedPerson-f002'),
      ('_withRelatedPersonPeter', 'RelatedPerson-peter'),
  )
  def testJsonFormat_forValidRelatedPerson_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.RelatedPerson)

  @parameterized.named_parameters(
      ('_withRequestGroupExample', 'RequestGroup-example'),
      ('_withRequestGroupKdn5Example', 'RequestGroup-kdn5-example'),
  )
  def testJsonFormat_forValidRequestGroup_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.RequestGroup)

  @parameterized.named_parameters(
      ('_withResearchStudyExample', 'ResearchStudy-example'),)
  def testJsonFormat_forValidResearchStudy_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.ResearchStudy)

  @parameterized.named_parameters(
      ('_withResearchSubjectExample', 'ResearchSubject-example'),)
  def testJsonFormat_forValidResearchSubject_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ResearchSubject)

  @parameterized.named_parameters(
      ('_withRiskAssessmentGenetic', 'RiskAssessment-genetic'),
      ('_withRiskAssessmentCardiac', 'RiskAssessment-cardiac'),
      ('_withRiskAssessmentPopulation', 'RiskAssessment-population'),
      ('_withRiskAssessmentPrognosis', 'RiskAssessment-prognosis'),
  )
  def testJsonFormat_forValidRiskAssessment_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.RiskAssessment)

  @parameterized.named_parameters(
      ('_withScheduleExample', 'Schedule-example'),
      ('_withScheduleExampleloc1', 'Schedule-exampleloc1'),
      ('_withScheduleExampleloc2', 'Schedule-exampleloc2'),
  )
  def testJsonFormat_forValidSchedule_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Schedule)

  @parameterized.named_parameters(
      ('_withSearchParameterExample', 'SearchParameter-example'),
      ('_withSearchParameterExampleExtension',
       'SearchParameter-example-extension'),
      ('_withSearchParameterExampleReference',
       'SearchParameter-example-reference'),
  )
  def testJsonFormat_forValidSearchParameter_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.SearchParameter)

  @parameterized.named_parameters(
      ('_withSequenceCoord0Base', 'Sequence-coord-0-base'),
      ('_withSequenceCoord1Base', 'Sequence-coord-1-base'),
      ('_withSequenceExample', 'Sequence-example'),
      ('_withSequenceFdaExample', 'Sequence-fda-example'),
      ('_withSequenceFdaVcfComparison', 'Sequence-fda-vcf-comparison'),
      ('_withSequenceFdaVcfevalComparison', 'Sequence-fda-vcfeval-comparison'),
      ('_withSequenceExamplePgx1', 'Sequence-example-pgx-1'),
      ('_withSequenceExamplePgx2', 'Sequence-example-pgx-2'),
      ('_withSequenceExampleTPMTOne', 'Sequence-example-TPMT-one'),
      ('_withSequenceExampleTPMTTwo', 'Sequence-example-TPMT-two'),
      ('_withSequenceGraphicExample1', 'Sequence-graphic-example-1'),
      ('_withSequenceGraphicExample2', 'Sequence-graphic-example-2'),
      ('_withSequenceGraphicExample3', 'Sequence-graphic-example-3'),
      ('_withSequenceGraphicExample4', 'Sequence-graphic-example-4'),
      ('_withSequenceGraphicExample5', 'Sequence-graphic-example-5'),
  )
  def testJsonFormat_forValidSequence_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Sequence)

  @parameterized.named_parameters(
      ('_withServiceDefinitionExample', 'ServiceDefinition-example'),)
  def testJsonFormat_forValidServiceDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.ServiceDefinition)

  @parameterized.named_parameters(
      ('_withSlotExample', 'Slot-example'),
      ('_withSlot1', 'Slot-1'),
      ('_withSlot2', 'Slot-2'),
      ('_withSlot3', 'Slot-3'),
  )
  def testJsonFormat_forValidSlot_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Slot)

  @parameterized.named_parameters(
      ('_withSpecimen101', 'Specimen-101'),
      ('_withSpecimenIsolate', 'Specimen-isolate'),
      ('_withSpecimenSst', 'Specimen-sst'),
      ('_withSpecimenVmaUrine', 'Specimen-vma-urine'),
  )
  def testJsonFormat_forValidSpecimen_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Specimen)

  @parameterized.named_parameters(
      ('_withStructureDefinitionExample', 'StructureDefinition-example'),)
  def testJsonFormat_forValidStructureDefinition_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.StructureDefinition)

  @parameterized.named_parameters(
      ('_withStructureMapExample', 'StructureMap-example'),)
  def testJsonFormat_forValidStructureMap_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.StructureMap)

  @parameterized.named_parameters(
      ('_withSubscriptionExample', 'Subscription-example'),
      ('_withSubscriptionExampleError', 'Subscription-example-error'),
  )
  def testJsonFormat_forValidSubscription_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Subscription)

  @parameterized.named_parameters(
      ('_withSubstanceExample', 'Substance-example'),
      ('_withSubstanceF205', 'Substance-f205'),
      ('_withSubstanceF201', 'Substance-f201'),
      ('_withSubstanceF202', 'Substance-f202'),
      ('_withSubstanceF203', 'Substance-f203'),
      ('_withSubstanceF204', 'Substance-f204'),
  )
  def testJsonFormat_forValidSubstance_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Substance)

  @parameterized.named_parameters(
      ('_withSupplyDeliverySimpleDelivery', 'SupplyDelivery-simpledelivery'),
      ('_withSupplyDeliveryPumpDelivery', 'SupplyDelivery-pumpdelivery'),
  )
  def testJsonFormat_forValidSupplyDelivery_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.SupplyDelivery)

  @parameterized.named_parameters(
      ('_withSupplyRequestSimpleOrder', 'SupplyRequest-simpleorder'),)
  def testJsonFormat_forValidSupplyRequest_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.SupplyRequest)

  @parameterized.named_parameters(
      ('_withTaskExample1', 'Task-example1'),
      ('_withTaskExample2', 'Task-example2'),
      ('_withTaskExample3', 'Task-example3'),
      ('_withTaskExample4', 'Task-example4'),
      ('_withTaskExample5', 'Task-example5'),
      ('_withTaskExample6', 'Task-example6'),
  )
  def testJsonFormat_forValidTask_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.Task)

  @parameterized.named_parameters(
      ('_withTestReportTestReportExample', 'TestReport-testreport-example'),)
  def testJsonFormat_forValidTestReport_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.TestReport)

  @parameterized.named_parameters(
      ('_withTestScriptTestScriptExample', 'TestScript-testscript-example'),
      ('_withTestScriptTestScriptExampleHistory',
       'TestScript-testscript-example-history'),
      ('_withTestScriptTestScriptExampleMultisystem',
       'TestScript-testscript-example-multisystem'),
      ('_withTestScriptTestScriptExampleReadtest',
       'TestScript-testscript-example-readtest'),
      ('_withTestScriptTestScriptExampleRule',
       'TestScript-testscript-example-rule'),
      ('_withTestScriptTestScriptExampleSearch',
       'TestScript-testscript-example-search'),
      ('_withTestScriptTestScriptExampleUpdate',
       'TestScript-testscript-example-update'),
  )
  def testJsonFormat_forValidTestScript_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.TestScript)

  @parameterized.named_parameters(
      ('_withValueSetExampleExpansion', 'ValueSet-example-expansion'),
      ('_withValueSetExampleExtensional', 'ValueSet-example-extensional'),
      ('_withValueSetExampleIntensional', 'ValueSet-example-intensional'),
  )
  def testJsonFormat_forValidValueSet_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(file_name,
                                                   resources_pb2.ValueSet)

  @parameterized.named_parameters(
      ('_withVisionPrescription33123', 'VisionPrescription-33123'),
      ('_withVisionPrescription33124', 'VisionPrescription-33124'),
  )
  def testJsonFormat_forValidVisionPrescription_succeeds(self, file_name: str):
    self.assert_parse_and_print_spec_equals_golden(
        file_name, resources_pb2.VisionPrescription)

  @parameterized.named_parameters(
      ('_withCompositionExample', 'Composition-example',
       resources_pb2.Composition),
      ('_withEcounterHome', 'Encounter-home', resources_pb2.Encounter),
      ('_withObservationExampleGenetics1', 'Observation-example-genetics-1',
       resources_pb2.Observation),
      ('_withPatientExample', 'patient-example', resources_pb2.Patient),
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
    """Convenience method for performing assertions on FHIR STU3 examples."""
    json_path = os.path.join(_EXAMPLES_PATH, file_name + '.json')
    proto_path = os.path.join(_EXAMPLES_PATH, file_name + '.prototxt')
    self.assert_parse_and_print_equals_golden(json_path, proto_path, proto_cls)

  def assert_parse_and_print_spec_equals_golden(
      self, file_name: str, proto_cls: Type[message.Message]):
    """Convenience method for performing assertions on the FHIR STU3 spec."""
    json_path = os.path.join(_FHIR_SPEC_PATH, file_name + '.json')
    proto_path = os.path.join(_EXAMPLES_PATH, file_name + '.prototxt')
    self.assert_parse_and_print_equals_golden(json_path, proto_path, proto_cls)

  def assert_parse_and_print_equals_golden(self, json_path: str,
                                           proto_path: str,
                                           proto_cls: Type[message.Message]):
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
