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
"""Test annotation_utils functionality."""

import sys

from absl.testing import absltest
from proto.google.fhir.proto.r4 import uscore_codes_pb2
from proto.google.fhir.proto.r4 import uscore_pb2
from proto.google.fhir.proto.r4.core import datatypes_pb2
from proto.google.fhir.proto.r4.core import valuesets_pb2
from proto.google.fhir.proto.r4.core.resources import observation_pb2
from proto.google.fhir.proto.r4.core.resources import patient_pb2
from google.fhir.utils import annotation_utils

try:
  from testdata.r4.profiles import test_pb2
except ImportError:
  # TODO: Add test protos to PYTHONPATH during dist testing.
  pass  # Fall through

_ADDRESS_USECODE_FHIR_VALUESET_URL = 'http://hl7.org/fhir/ValueSet/address-use'
_BODY_LENGTH_UNITS_VALUESET_URL = 'http://hl7.org/fhir/ValueSet/ucum-bodylength'
_BOOLEAN_STRUCTURE_DEFINITION_URL = 'http://hl7.org/fhir/StructureDefinition/boolean'
_BOOLEAN_VALUE_REGEX = 'true|false'
_CODE_STRUCTURE_DEFINITION_URL = 'http://hl7.org/fhir/StructureDefinition/code'
_CODE_VALUE_REGEX = '[^\\s]+(\\s[^\\s]+)*'
_PATIENT_STRUCTURE_DEFINITION_URL = 'http://hl7.org/fhir/StructureDefinition/Patient'
_R4_FHIR_VERSION = 4


class AnnotationUtilsTest(absltest.TestCase):
  """Unit tests for functionality in annotation_utils.py."""

  def testIsTypedReferenceField_withValidTypedReferenceField_returnsTrue(self):
    """Test is_typed_reference_field functionality on valid input."""
    reference = datatypes_pb2.Reference()
    practitioner_role_id_field = reference.DESCRIPTOR.fields_by_name[
        'practitioner_role_id']
    self.assertTrue(
        annotation_utils.is_typed_reference_field(practitioner_role_id_field))

  def testIsTypedReferenceField_withInvalidTypedReferenceField_returnsFalse(
      self):
    """Test is_typed_reference_field functionality on invalid input."""
    reference = datatypes_pb2.Reference()
    uri_field = reference.DESCRIPTOR.fields_by_name['uri']
    self.assertFalse(annotation_utils.is_typed_reference_field(uri_field))

  def testIsResource_withPatient_returnsTrue(self):
    """Test is_resource functionality on non-primitive input."""
    patient = patient_pb2.Patient()
    self.assertTrue(annotation_utils.is_resource(patient))
    self.assertTrue(annotation_utils.is_resource(patient.DESCRIPTOR))

  def testIsResource_withPrimitives_returnsFalse(self):
    """Test is_resource functionality on primitive input."""
    boolean = datatypes_pb2.Boolean()
    code = datatypes_pb2.Code()
    self.assertFalse(annotation_utils.is_resource(boolean))
    self.assertFalse(annotation_utils.is_resource(boolean.DESCRIPTOR))
    self.assertFalse(annotation_utils.is_resource(code))
    self.assertFalse(annotation_utils.is_resource(code.DESCRIPTOR))

  def testIsPrimitiveType_withPrimitives_returnsTrue(self):
    """Test is_primitive_type functionality on primitive input."""
    boolean = datatypes_pb2.Boolean()
    code = datatypes_pb2.Code()
    self.assertTrue(annotation_utils.is_primitive_type(boolean))
    self.assertTrue(annotation_utils.is_primitive_type(boolean.DESCRIPTOR))
    self.assertTrue(annotation_utils.is_primitive_type(code))
    self.assertTrue(annotation_utils.is_primitive_type(code.DESCRIPTOR))

  def testIsPrimitiveType_withPatient_returnsFalse(self):
    """Test is_primitive_type functionality on non-primitive input."""
    patient = patient_pb2.Patient()
    self.assertFalse(annotation_utils.is_primitive_type(patient))
    self.assertFalse(annotation_utils.is_primitive_type(patient.DESCRIPTOR))

  def testIsChoiceType_withValidChoiceType_returnsTrue(self):
    """Test is_choice_type functionality on valid input."""
    dosage = datatypes_pb2.Dosage()
    as_needed_fd = dosage.DESCRIPTOR.fields_by_name['as_needed']
    self.assertTrue(annotation_utils.is_choice_type_field(as_needed_fd))

  def testIsChoiceType_withInvalidChoiceType_returnsFalse(self):
    """Test is_choice_type functionality on invalid input."""
    boolean = datatypes_pb2.Boolean()
    value_fd = boolean.DESCRIPTOR.fields_by_name['value']
    self.assertFalse(annotation_utils.is_choice_type_field(value_fd))

    patient = patient_pb2.Patient()
    text_fd = patient.DESCRIPTOR.fields_by_name['text']
    self.assertFalse(annotation_utils.is_choice_type_field(text_fd))

  def testIsReference_withValidReferenceType_returnsTrue(self):
    """Test is_reference functionality on valid input."""
    reference = datatypes_pb2.Reference()
    self.assertTrue(annotation_utils.is_reference(reference))
    self.assertTrue(annotation_utils.is_reference(reference.DESCRIPTOR))

  def testIsReference_withInvalidReferenceType_returnsFalse(self):
    """Test is_reference functionality on invalid input."""
    boolean = datatypes_pb2.Boolean()
    code = datatypes_pb2.Code()
    self.assertFalse(annotation_utils.is_reference(boolean))
    self.assertFalse(annotation_utils.is_reference(boolean.DESCRIPTOR))
    self.assertFalse(annotation_utils.is_reference(code))
    self.assertFalse(annotation_utils.is_reference(code.DESCRIPTOR))

  @absltest.skipIf(
      'testdata' not in sys.modules,
      'google-fhir package does not build+install tertiary testdata protos.')
  def testGetFixedCodingSystem_withValidFixedCodingSystem_returnsValue(self):
    """Test get_fixed_coding_system functionality when annotation is present."""
    expected_system = 'http://hl7.org/fhir/metric-color'
    coding = (
        test_pb2.TestPatient.CodeableConceptForMaritalStatus.ColorCoding
        .FixedCode())
    self.assertEqual(
        annotation_utils.get_fixed_coding_system(coding), expected_system)
    self.assertEqual(
        annotation_utils.get_fixed_coding_system(coding.DESCRIPTOR),
        expected_system)

  def testGetFixedCodingSystem_withInvalidMessage_returnsNone(self):
    """Test get_fixed_coding_system functionality with no annotation present."""
    boolean = datatypes_pb2.Boolean()
    self.assertIsNone(annotation_utils.get_fixed_coding_system(boolean))

    code = datatypes_pb2.Code()
    self.assertIsNone(annotation_utils.get_fixed_coding_system(code))

  def testGetSourceCodeSystem_withValidCodeSystem_returnsValue(self):
    """Test get_source_code_system when source_code_system is present."""
    birth_sex_valueset = uscore_codes_pb2.BirthSexValueSet()
    female_value_descriptor = (
        birth_sex_valueset.Value.DESCRIPTOR.values_by_number[
            birth_sex_valueset.Value.F])
    self.assertEqual(
        annotation_utils.get_source_code_system(female_value_descriptor),
        'http://terminology.hl7.org/CodeSystem/v3-AdministrativeGender')

    male_value_descriptor = (
        birth_sex_valueset.Value.DESCRIPTOR.values_by_number[
            birth_sex_valueset.Value.M])
    self.assertEqual(
        annotation_utils.get_source_code_system(male_value_descriptor),
        'http://terminology.hl7.org/CodeSystem/v3-AdministrativeGender')

    unk_value_descriptor = (
        birth_sex_valueset.Value.DESCRIPTOR.values_by_number[
            birth_sex_valueset.Value.UNK])
    self.assertEqual(
        annotation_utils.get_source_code_system(unk_value_descriptor),
        'http://terminology.hl7.org/CodeSystem/v3-NullFlavor')

  def testGetSourceCodeSystem_withInvalidCodeSystem_returnsNone(self):
    """Test get_source_code_system when source_code_system is not present."""
    year_value_descriptor = (
        datatypes_pb2.Date.Precision.DESCRIPTOR.values_by_number[
            datatypes_pb2.Date.Precision.YEAR])
    self.assertIsNone(
        annotation_utils.get_source_code_system(year_value_descriptor))

  def testHasSourceCodeSystem_withValidCodeSystem_returnsTrue(self):
    """Test has_source_code_system when source_code_system is present."""
    birth_sex_valueset = uscore_codes_pb2.BirthSexValueSet()
    female_value_descriptor = (
        birth_sex_valueset.Value.DESCRIPTOR.values_by_number[
            birth_sex_valueset.Value.F])
    self.assertTrue(
        annotation_utils.has_source_code_system(female_value_descriptor))

    male_value_descriptor = (
        birth_sex_valueset.Value.DESCRIPTOR.values_by_number[
            birth_sex_valueset.Value.M])
    self.assertTrue(
        annotation_utils.has_source_code_system(male_value_descriptor))

    unk_value_descriptor = (
        birth_sex_valueset.Value.DESCRIPTOR.values_by_number[
            birth_sex_valueset.Value.UNK])
    self.assertTrue(
        annotation_utils.has_source_code_system(unk_value_descriptor))

  def testHasSourceCodeSystem_withInvalidCodeSystem_returnsFalse(self):
    """Test has_source_code_system when source_code_system is not present."""
    year_value_descriptor = (
        datatypes_pb2.Date.Precision.DESCRIPTOR.values_by_number[
            datatypes_pb2.Date.Precision.YEAR])
    self.assertFalse(
        annotation_utils.has_source_code_system(year_value_descriptor))

  def testGetValueRegexForPrimitiveType_withPrimitive_returnsValue(self):
    """Test get_value_regex_for_primitive_type functionality on primitives."""
    boolean = datatypes_pb2.Boolean()
    code = datatypes_pb2.Code()
    self.assertEqual(
        annotation_utils.get_value_regex_for_primitive_type(boolean),
        _BOOLEAN_VALUE_REGEX)
    self.assertEqual(
        annotation_utils.get_value_regex_for_primitive_type(boolean.DESCRIPTOR),
        _BOOLEAN_VALUE_REGEX)
    self.assertEqual(
        annotation_utils.get_value_regex_for_primitive_type(code),
        _CODE_VALUE_REGEX)
    self.assertEqual(
        annotation_utils.get_value_regex_for_primitive_type(code.DESCRIPTOR),
        _CODE_VALUE_REGEX)

  def testGetValueRegexForPrimitiveType_withCompound_returnsNone(self):
    """Test get_value_regex_for_primitive_type on non-primitives."""
    patient = patient_pb2.Patient()
    self.assertIsNone(
        annotation_utils.get_value_regex_for_primitive_type(patient))
    self.assertIsNone(
        annotation_utils.get_value_regex_for_primitive_type(patient.DESCRIPTOR))

  def testGetStructureDefinitionUrl_withFhirType_returnsValue(self):
    """Test get_structure_definition_url functionality on FHIR types."""
    boolean = datatypes_pb2.Boolean()
    code = datatypes_pb2.Code()
    patient = patient_pb2.Patient()
    self.assertEqual(
        annotation_utils.get_structure_definition_url(boolean),
        _BOOLEAN_STRUCTURE_DEFINITION_URL)
    self.assertEqual(
        annotation_utils.get_structure_definition_url(boolean.DESCRIPTOR),
        _BOOLEAN_STRUCTURE_DEFINITION_URL)
    self.assertEqual(
        annotation_utils.get_structure_definition_url(code),
        _CODE_STRUCTURE_DEFINITION_URL)
    self.assertEqual(
        annotation_utils.get_structure_definition_url(patient),
        _PATIENT_STRUCTURE_DEFINITION_URL)
    self.assertEqual(
        annotation_utils.get_structure_definition_url(patient.DESCRIPTOR),
        _PATIENT_STRUCTURE_DEFINITION_URL)

  def testGetFhirValuesetUrl_withFhirValueSet_returnsCorrectValue(self):
    """Tests get_fhir_valueset_url with a valid FHIR valueset."""
    use_code = datatypes_pb2.Address.UseCode()
    self.assertEqual(
        annotation_utils.get_fhir_valueset_url(use_code),
        _ADDRESS_USECODE_FHIR_VALUESET_URL)
    self.assertEqual(
        annotation_utils.get_fhir_valueset_url(use_code.DESCRIPTOR),
        _ADDRESS_USECODE_FHIR_VALUESET_URL)

  def testGetFhirValuesetUrl_withGenericCode_returnsNone(self):
    """Tests get_fhir_valueset_url with a generic instance of Code."""
    self.assertIsNone(
        annotation_utils.get_fhir_valueset_url(datatypes_pb2.Code()))

  def testGetEnumValuesetUrl_withEnumValueSet_returnsCorrectValue(self):
    """Tests get_enum_valueset_url with a valid enum valueset."""
    body_length_descriptor = valuesets_pb2.BodyLengthUnitsValueSet().DESCRIPTOR
    value_enum_descriptor = body_length_descriptor.enum_types_by_name['Value']
    self.assertEqual(
        annotation_utils.get_enum_valueset_url(value_enum_descriptor),
        _BODY_LENGTH_UNITS_VALUESET_URL)

  def testGetEnumValuesetUrl_withMessageDescriptor_raisesException(self):
    """Tests get_enum_valueset_url raises without passing an EnumDescriptor."""
    with self.assertRaises(KeyError) as ke:
      annotation_utils.get_enum_valueset_url(patient_pb2.Patient())

    self.assertIsInstance(ke.exception, KeyError)

  def testHasFhirValuesetUrl_withFhirValueSet_returnsTrue(self):
    """Tests has_fhir_valueset_url with a valid FHIR valueset."""
    use_code = datatypes_pb2.Address.UseCode()
    self.assertTrue(annotation_utils.has_fhir_valueset_url(use_code))
    self.assertTrue(annotation_utils.has_fhir_valueset_url(use_code.DESCRIPTOR))

  def testIsProfileOf_withValidProfile_returnsTrue(self):
    """Tests is_profile_of functionality with a valid Patient profile."""
    patient = patient_pb2.Patient()
    uscore_patient_profile = uscore_pb2.USCorePatientProfile()
    self.assertTrue(
        annotation_utils.is_profile_of(patient, uscore_patient_profile))
    self.assertTrue(
        annotation_utils.is_profile_of(patient.DESCRIPTOR,
                                       uscore_patient_profile))
    self.assertTrue(
        annotation_utils.is_profile_of(patient,
                                       uscore_patient_profile.DESCRIPTOR))
    self.assertTrue(
        annotation_utils.is_profile_of(patient.DESCRIPTOR,
                                       uscore_patient_profile.DESCRIPTOR))

  def testIsProfileOf_withInvalidProfile_returnsFalse(self):
    """Tests is_profile_of functionality with an invalid profile of Patient."""
    patient = patient_pb2.Patient()
    observation = observation_pb2.Observation()
    self.assertFalse(annotation_utils.is_profile_of(patient, observation))
    self.assertFalse(
        annotation_utils.is_profile_of(patient, observation.DESCRIPTOR))
    self.assertFalse(
        annotation_utils.is_profile_of(patient.DESCRIPTOR, observation))
    self.assertFalse(
        annotation_utils.is_profile_of(patient.DESCRIPTOR,
                                       observation.DESCRIPTOR))

  def testGetFhirVersion_withValidInput_returnsCorrectVersion(self):
    """Tests get_fhir_version to ensure that the correct version is returned."""
    patient = patient_pb2.Patient()
    uscore_patient = uscore_pb2.USCorePatientProfile()
    self.assertEqual(
        annotation_utils.get_fhir_version(patient), _R4_FHIR_VERSION)
    self.assertEqual(
        annotation_utils.get_fhir_version(uscore_patient), _R4_FHIR_VERSION)
    self.assertEqual(
        annotation_utils.get_fhir_version(patient.DESCRIPTOR), _R4_FHIR_VERSION)
    self.assertEqual(
        annotation_utils.get_fhir_version(uscore_patient.DESCRIPTOR),
        _R4_FHIR_VERSION)
    self.assertEqual(
        annotation_utils.get_fhir_version(patient.DESCRIPTOR.file),
        _R4_FHIR_VERSION)
    self.assertEqual(
        annotation_utils.get_fhir_version(uscore_patient.DESCRIPTOR.file),
        _R4_FHIR_VERSION)


if __name__ == '__main__':
  absltest.main()
