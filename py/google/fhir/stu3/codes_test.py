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
"""Test codes functionality."""

import os
from typing import Type

from google.protobuf import message
from absl.testing import absltest
from absl.testing import parameterized
from proto.google.fhir.proto.stu3 import codes_pb2
from proto.google.fhir.proto.stu3 import datatypes_pb2
from google.fhir import codes
from google.fhir import fhir_errors
from google.fhir.testing import testdata_utils

_CODES_DIR = os.path.join('testdata', 'stu3', 'codes')


class CodesTest(parameterized.TestCase):
  """Tests functionality provided by the codes module."""

  def testGetCodeAsString_withStringValueType(self):
    """Tests get_code_as_string with a string value-field type."""
    code = datatypes_pb2.Code(value='foo')
    self.assertEqual('foo', codes.get_code_as_string(code))

  def testGetCodeAsString_withEnumValueType(self):
    """Tests get_code_as_string with an enum value-field type."""
    code = codes_pb2.AdministrativeGenderCode(
        value=codes_pb2.AdministrativeGenderCode.Value.FEMALE)
    self.assertEqual('female', codes.get_code_as_string(code))

  def testGetCodeAsString_withInvalidType(self):
    """Tests get_code_as_string with an invalid value-field type."""
    not_a_code = datatypes_pb2.String(value='foo')
    with self.assertRaises(ValueError) as ve:
      _ = codes.get_code_as_string(not_a_code)
    self.assertIsInstance(ve.exception, ValueError)

  def testEnumValueDescriptorToCodeString(self):
    """Tests enum_value_descriptor_to_code_string functionality."""
    female_value_descriptor = (
        codes_pb2.AdministrativeGenderCode.Value.DESCRIPTOR.values_by_number[
            codes_pb2.AdministrativeGenderCode.FEMALE])
    self.assertEqual(
        'female',
        codes.enum_value_descriptor_to_code_string(female_value_descriptor))

  def testCodeStringToEnumValueDescriptor_withValidCodeString(self):
    """Tests code_string_to_enum_value_descriptor functionality."""
    enum_descriptor = codes_pb2.AssertionOperatorTypeCode.Value.DESCRIPTOR
    enum_value_descriptor = enum_descriptor.values_by_name['GREATERTHAN']
    result = codes.code_string_to_enum_value_descriptor('greaterthan',
                                                        enum_descriptor)
    self.assertEqual(result.name, enum_value_descriptor.name)

  def testCodeStringToEnumValueDescriptor_withInvalidCodeString(self):
    """Tests code_string_to_enum_value_descriptor error handling."""
    enum_descriptor = codes_pb2.AssertionOperatorTypeCode.Value.DESCRIPTOR
    with self.assertRaises(fhir_errors.InvalidFhirError) as fe:
      _ = codes.code_string_to_enum_value_descriptor('InvalidCode!',
                                                     enum_descriptor)
    self.assertIsInstance(fe.exception, fhir_errors.InvalidFhirError)

  def testCopyCode_fromTypedToGeneric(self):
    """Tests copy_code from a generic to typed Code."""
    typed_code = codes_pb2.AdministrativeGenderCode(
        value=codes_pb2.AdministrativeGenderCode.Value.FEMALE)
    generic_code = datatypes_pb2.Code()
    codes.copy_code(typed_code, generic_code)
    self.assertEqual('female', generic_code.value)

  def testCopyCode_fromGenericToTyped(self):
    """Tests copy_code from a typed to a generic Code."""
    typed_code = codes_pb2.AdministrativeGenderCode()
    generic_code = datatypes_pb2.Code(value='female')
    codes.copy_code(generic_code, typed_code)
    self.assertEqual(codes_pb2.AdministrativeGenderCode.Value.FEMALE,
                     typed_code.value)

  def testCopyCode_fromGenericToGeneric(self):
    """Tests copy_code form a generic to a generic Code."""
    source = datatypes_pb2.Code(value='female')
    target = datatypes_pb2.Code()
    codes.copy_code(source, target)
    self.assertEqual('female', target.value)

  def testCopyCode_fromTypedToTyped(self):
    """Tests copy_code from a typed to a typed Code."""
    source = codes_pb2.AdministrativeGenderCode(
        value=codes_pb2.AdministrativeGenderCode.Value.FEMALE)
    target = codes_pb2.AdministrativeGenderCode()
    codes.copy_code(source, target)
    self.assertEqual(codes_pb2.AdministrativeGenderCode.Value.FEMALE,
                     target.value)

  @parameterized.named_parameters(
      ('_withUsCoreOmb1', 'uscore_omb_1'),
      ('_withUsCoreOmb2', 'uscore_omb_2'),
  )
  def testCopyCoding_fromGenericToGeneric(self, name: str):
    """Tests copy_coding from a generic Coding to a generic Coding."""
    source = self._coding_from_file(name + '_raw.prototxt',
                                    datatypes_pb2.Coding)
    target = datatypes_pb2.Coding()
    codes.copy_coding(source, target)
    self.assertEqual(source, target)

  def _coding_from_file(self, name: str,
                        coding_cls: Type[message.Message]) -> message.Message:
    """Reads data from the CODES_DIR/name into an instance of coding_cls."""
    return testdata_utils.read_protos(
        os.path.join(_CODES_DIR, name), coding_cls)[0]


if __name__ == '__main__':
  absltest.main()
