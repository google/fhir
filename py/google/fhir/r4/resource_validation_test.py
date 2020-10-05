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
"""Test resource validation functionality."""

import os
from typing import Type

from google.protobuf import message
from absl.testing import absltest
from proto.google.fhir.proto.r4.core.resources import bundle_and_contained_resource_pb2
from proto.google.fhir.proto.r4.core.resources import encounter_pb2
from proto.google.fhir.proto.r4.core.resources import observation_pb2
from google.fhir import fhir_errors
from google.fhir.r4 import resource_validation
from google.fhir.testing import testdata_utils

_VALIDATION_DIR = os.path.join('testdata', 'r4', 'validation')


class ResourceValidationTest(absltest.TestCase):
  """Basic unit test suite ensuring that resource validation works correctly."""

  def testResourceValidation_withMissingRequiredField_raises(self):
    self._invalid_test('observation_invalid_missing_required',
                       observation_pb2.Observation)

  def testResourceValidation_withInvalidPrimitive_raises(self):
    self._invalid_test('observation_invalid_primitive',
                       observation_pb2.Observation)

  def testResourceValidation_withValidReference_succeeds(self):
    self._valid_test('observation_valid_reference', observation_pb2.Observation)

  def testResourceValidation_withInvalidReference_raises(self):
    self._invalid_test('observation_invalid_reference',
                       observation_pb2.Observation)

  # TODO: Implement FHIR-Path validation for Python API
  # def testResourceValidation_withFhirPathViolation_raises(self):
  #   self._invalid_test('observation_invalid_fhirpath_violation',
  #                     observation_pb2.Observation)

  def testResourceValidation_withValidRepeatedReference_succeeds(self):
    self._valid_test('encounter_valid_repeated_reference',
                     encounter_pb2.Encounter)

  def testResourceValidation_withInvalidRepeatedReference_raies(self):
    self._invalid_test('encounter_invalid_repeated_reference',
                       encounter_pb2.Encounter)

  def testResourceValidation_withInvalidEmptyOneof_Raises(self):
    self._invalid_test('observation_invalid_empty_oneof',
                       observation_pb2.Observation)

  def testResourceValidation_withValidBundle_succeeds(self):
    self._valid_test('bundle_valid', bundle_and_contained_resource_pb2.Bundle)

  def testResourceValidation_withStartLaterThanEnd_raises(self):
    self._invalid_test('encounter_invalid_start_later_than_end',
                       encounter_pb2.Encounter)

  def testResourceValidation_withStartLaterThanEndWithEndPrecision_succeeds(
      self):
    self._valid_test('encounter_valid_start_later_than_end_day_precision',
                     encounter_pb2.Encounter)

  def testResourceValidation_withValidEncounter_succeeds(self):
    self._valid_test('encounter_valid', encounter_pb2.Encounter)

  def testResourceValidation_withValidNumericTimezone_succeeds(self):
    self._valid_test('encounter_valid_numeric_timezone',
                     encounter_pb2.Encounter)

  def _valid_test(self, name: str, message_cls: Type[message.Message]):
    msg = testdata_utils.read_protos(
        os.path.join(_VALIDATION_DIR, name + '.prototxt'), message_cls)[0]
    resource_validation.validate_resource(msg)

  def _invalid_test(self, name: str, message_cls: Type[message.Message]):
    msg = testdata_utils.read_protos(
        os.path.join(_VALIDATION_DIR, name + '.prototxt'), message_cls)[0]

    with self.assertRaises(fhir_errors.InvalidFhirError) as fe:
      resource_validation.validate_resource(msg)

    self.assertIsInstance(fe.exception, fhir_errors.InvalidFhirError)


if __name__ == '__main__':
  absltest.main()
