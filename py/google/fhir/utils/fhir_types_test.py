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
"""Test fhir_types functionality."""

from absl.testing import absltest
from proto.google.fhir.proto.r4 import fhirproto_extensions_pb2
from proto.google.fhir.proto.r4 import uscore_pb2
from proto.google.fhir.proto.r4.core import datatypes_pb2
from proto.google.fhir.proto.r4.core.resources import patient_pb2
from google.fhir.utils import fhir_types


class FhirTypesTest(absltest.TestCase):
  """Tests functionality provided by the fhir_types module."""

  def testIsCode_withCode_returnsTrue(self):
    """Tests that is_code returns True when given a Code."""
    self.assertTrue(fhir_types.is_code(datatypes_pb2.Code()))

  def testIsCode_withProfileOfCode_returnsFalse(self):
    """Tests that is_code returns False when given a profile of Code."""
    self.assertFalse(fhir_types.is_code(datatypes_pb2.Address.UseCode()))

  def testIsProfileOfCode_withProfileOfCode_returnsTrue(self):
    """Tests that is_profile_of_code returns True for a profile of Code."""
    self.assertTrue(
        fhir_types.is_profile_of_code(datatypes_pb2.Address.UseCode()))

  def testIsProfileOfCode_withCode_returnsFalse(self):
    """Tests that is_profile_of_code returns False for a base Code."""
    self.assertFalse(fhir_types.is_profile_of_code(datatypes_pb2.Code()))

  def testIsTypeOrProfileOfCode_withProfileOfCode_returnsTrue(self):
    """Tests that is_type_or_profile_of_code returns True for a profile."""
    self.assertTrue(
        fhir_types.is_type_or_profile_of_code(datatypes_pb2.Address.UseCode()))

  def testIsTypeOrProfileOfCode_withCode_returnsTrue(self):
    """Tests that is_type_or_profile_of_code returns True for a base Code."""
    self.assertTrue(fhir_types.is_type_or_profile_of_code(datatypes_pb2.Code()))

  def testIsTypeOrProfileOfCode_withNonCode_returnsFalse(self):
    """Tests that is_type_or_profile_of_code returns False for a non-Code."""
    self.assertFalse(
        fhir_types.is_type_or_profile_of_code(patient_pb2.Patient()))

  def testIsCoding_withCoding_returnsTrue(self):
    """Tests that is_coding returns True when given a Coding instance."""
    self.assertTrue(fhir_types.is_coding(datatypes_pb2.Coding()))

  def testIsCoding_withProfileOfCoding_returnsFalse(self):
    """Tests that is_coding returns False when given a profile."""
    self.assertFalse(fhir_types.is_coding(datatypes_pb2.CodingWithFixedCode()))

  def testIsProfileOfCoding_withCoding_returnsTrue(self):
    """Tests that is_profile_of_coding returns True for a profile."""
    self.assertTrue(
        fhir_types.is_profile_of_coding(datatypes_pb2.CodingWithFixedCode()))

  def testIsProfileOfCoding_withCoding_returnsFalse(self):
    """Tests that is_profile_of_coding returns False for a base Coding type."""
    self.assertFalse(fhir_types.is_profile_of_coding(datatypes_pb2.Coding()))

  def testIsTypeOrProfileOfCoding_withCoding_returnsTrue(self):
    """Tests that is_type_or_profile_of_coding returns True for profile."""
    self.assertTrue(
        fhir_types.is_type_or_profile_of_coding(
            datatypes_pb2.CodingWithFixedCode()))

  def testIsTypeOrProfileOfCoding_withNonCoding_returnsFalse(self):
    """Tests that is_type_or_profile_of_coding returns False for non-Coding."""
    self.assertFalse(
        fhir_types.is_type_or_profile_of_coding(patient_pb2.Patient()))

  def testIsPeriod_withPeriod_returnsTrue(self):
    """Tests that is_period returns True when given a Period instance."""
    self.assertTrue(fhir_types.is_period(datatypes_pb2.Period()))

  def testIsPeriod_withCoding_returnsFalse(self):
    """Tests that is_period returns False when given a profile of Coding."""
    self.assertFalse(fhir_types.is_period(datatypes_pb2.Coding()))

  def testIsDateTime_withDateTime_returnsTrue(self):
    """Tests that is_date_time returns True when given a DateTime instance."""
    self.assertTrue(fhir_types.is_date_time(datatypes_pb2.DateTime()))

  def testIsDateTime_withCoding_returnsFalse(self):
    """Tests that is_date_time returns False when given a profile of Coding."""
    self.assertFalse(fhir_types.is_date_time(datatypes_pb2.Coding()))

  def testIsExtension_withExtension_returnsTrue(self):
    """Tests that is_extension returns True when given an Extension."""
    self.assertTrue(fhir_types.is_extension(datatypes_pb2.Extension()))

  def testIsExtension_withDateTime_returnsFalse(self):
    """Tests that is_extension returns False when given a DateTime."""
    self.assertFalse(fhir_types.is_extension(datatypes_pb2.DateTime()))

  def testIsProfileOfExtension_withBase64BinarySeparatorStride_returnsTrue(
      self):
    """Tests that is_profile_of_extension returns True for valid profile."""
    self.assertTrue(
        fhir_types.is_profile_of_extension(
            fhirproto_extensions_pb2.Base64BinarySeparatorStride()))

  def testIsTypeOrProfileOfExtension_withExtension_returnsTrue(self):
    """Tests that is_type_or_profile_of_extension returns True for Extension."""
    self.assertTrue(
        fhir_types.is_type_or_profile_of_extension(datatypes_pb2.Extension()))

  def testIsTypeOrProfileOfExtension_withExtensionProfile_returnsTrue(self):
    """Tests that is_type_or_profile_of_extension returns True for  profile."""
    self.assertTrue(
        fhir_types.is_type_or_profile_of_extension(
            fhirproto_extensions_pb2.Base64BinarySeparatorStride()))

  def testIsTypeOrProfileOfExtensions_withDateTime_returnsFalse(self):
    """Tests that is_type_or_profile_of_extension returns False for DateTime."""
    self.assertFalse(
        fhir_types.is_type_or_profile_of_extension(datatypes_pb2.DateTime()))

  def testIsTypeOrProfileOfPatient_withPatient_returnsTrue(self):
    """Tests that IsTypeOfProfileOfPatient returns True for a Patient type."""
    self.assertTrue(
        fhir_types.is_type_or_profile_of_patient(patient_pb2.Patient()))

  def testIsTypeOrProfileOfPatient_withCoding_returnsFalse(self):
    """Tests that IsTypeOfProfileOfPatient returns False for a Coding type."""
    self.assertFalse(
        fhir_types.is_type_or_profile_of_patient(datatypes_pb2.Coding()))

  def testIsTypeOrProfileOfPatient_withPatientProfile_returnsTrue(self):
    """Tests that IsTypeOfProfileOfPatient returns True for Patient profile."""
    self.assertTrue(
        fhir_types.is_type_or_profile_of_patient(
            uscore_pb2.USCorePatientProfile()))


if __name__ == '__main__':
  absltest.main()
