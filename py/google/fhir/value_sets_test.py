#
# Copyright 2022 Google LLC
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
"""Test value_sets functionality."""

import os
import unittest.mock

from absl import flags
from google.fhir import value_sets
from absl.testing import absltest
from proto.google.fhir.proto.r4.core.resources import structure_definition_pb2
from proto.google.fhir.proto.r4.core.resources import value_set_pb2
from google.fhir.utils import fhir_package

FLAGS = flags.FLAGS


class ValueSetsTest(absltest.TestCase):

  def setUp(self):
    super().setUp()

    # TODO upgrade version of absl in the testing environment so we
    # can reference absl.testing.absltest.TEST_SRCDIR instead.
    package_filepath = os.path.join(
        FLAGS.test_srcdir, 'com_google_fhir/spec/fhir_r4_package.zip')

    package_manager = fhir_package.FhirPackageManager()
    package_manager.add_package_at_path(package_filepath)

    self.resolver = value_sets.ValueSetResolver(package_manager)

  def testValueSetFromUrl_withUsCoreDefinitions_findsValueSet(self):
    value_set = self.resolver.value_set_from_url(
        'http://hl7.org/fhir/ValueSet/financial-taskcode')
    self.assertIsNotNone(value_set)
    self.assertEqual(value_set.url.value,
                     'http://hl7.org/fhir/ValueSet/financial-taskcode')

  def testValueSetFromUrl_withUnknownUrl_raisesError(self):
    with self.assertRaises(ValueError):
      self.resolver.value_set_from_url('http://hl7.org/fhir/ValueSet/mystery')

  def testValueSetFromUrl_withWrongResourceType_raisesError(self):
    with unittest.mock.patch.object(self.resolver.package_manager,
                                    'get_resource') as m_get_resource:
      m_get_resource.return_value = (
          structure_definition_pb2.StructureDefinition())
      with self.assertRaises(ValueError):
        self.resolver.value_set_from_url('http://hl7.org/fhir/ValueSet/mystery')

  def testValueSetsFromStructureDefinition_withValueSets_succeeds(self):
    definition = structure_definition_pb2.StructureDefinition()

    # Add an a element to the differential definition.
    element = definition.differential.element.add()
    element.binding.value_set.value = 'http://hl7.org/fhir/ValueSet/financial-taskcode'

    # Add an element without a URL.
    definition.differential.element.add()

    # Add another element to the differential definition.
    another_element = definition.differential.element.add()
    another_element.binding.value_set.value = 'http://hl7.org/fhir/ValueSet/account-status'

    # Add an element to the snapshot definition.
    snapshot_element = definition.snapshot.element.add()
    snapshot_element.binding.value_set.value = 'http://hl7.org/fhir/ValueSet/action-participant-role'

    # Add an element with a duplicated url which should be ignored.
    duplicate_element = definition.snapshot.element.add()
    duplicate_element.binding.value_set.value = 'http://hl7.org/fhir/ValueSet/financial-taskcode'

    result = self.resolver.value_sets_from_structure_definition(definition)
    self.assertCountEqual(
        [value_set.url.value for value_set in result],
        [
            element.binding.value_set.value,
            another_element.binding.value_set.value,
            snapshot_element.binding.value_set.value
        ],
    )

  def testValueSetsFromStructureDefinition_withNoValueSets_returnsEmpty(self):
    definition = structure_definition_pb2.StructureDefinition()
    self.assertEqual(
        list(self.resolver.value_sets_from_structure_definition(definition)),
        [],
    )

  def testValueSetsFromFhirPackage_withValueSets_succeeds(self):
    definition = structure_definition_pb2.StructureDefinition()
    element = definition.differential.element.add()
    element.binding.value_set.value = 'http://hl7.org/fhir/ValueSet/financial-taskcode'

    another_definition = structure_definition_pb2.StructureDefinition()
    another_element = definition.differential.element.add()
    another_element.binding.value_set.value = 'http://hl7.org/fhir/ValueSet/action-participant-role'

    value_set = value_set_pb2.ValueSet()
    value_set.url.value = 'a-url'

    another_value_set = value_set_pb2.ValueSet()
    another_value_set.url.value = 'another-url'

    duplicate_value_set = value_set_pb2.ValueSet()
    duplicate_value_set.url.value = 'http://hl7.org/fhir/ValueSet/action-participant-role'

    package = fhir_package.FhirPackage(
        package_info=unittest.mock.MagicMock(),
        structure_definitions=[definition, another_definition],
        search_parameters=[],
        code_systems=[],
        value_sets=[value_set, another_value_set, duplicate_value_set],
    )

    result = self.resolver.value_sets_from_fhir_package(package)

    self.assertCountEqual(
        [vs.url.value for vs in result],
        [
            element.binding.value_set.value,
            another_element.binding.value_set.value,
            value_set.url.value,
            another_value_set.url.value,
        ],
    )

  def testValueSetsFromFhirPackage_withEmptyPackage_returnsEmpty(self):
    package = fhir_package.FhirPackage(
        package_info=unittest.mock.MagicMock(),
        structure_definitions=[],
        search_parameters=[],
        code_systems=[],
        value_sets=[],
    )
    self.assertEqual(
        list(self.resolver.value_sets_from_fhir_package(package)), [])


if __name__ == '__main__':
  absltest.main()
