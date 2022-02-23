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
import sqlalchemy

from google.fhir import value_sets
from absl.testing import absltest
from proto.google.fhir.proto.r4.core import datatypes_pb2
from proto.google.fhir.proto.r4.core.resources import structure_definition_pb2
from proto.google.fhir.proto.r4.core.resources import value_set_pb2
from google.fhir import terminology_service_client
from google.fhir.utils import fhir_package

FLAGS = flags.FLAGS


class ValueSetsTest(absltest.TestCase):

  def testValueSetFromUrl_withUsCoreDefinitions_findsValueSet(self):
    value_set = get_resolver().value_set_from_url(
        'http://hl7.org/fhir/ValueSet/financial-taskcode')
    self.assertIsNotNone(value_set)
    self.assertEqual(value_set.url.value,
                     'http://hl7.org/fhir/ValueSet/financial-taskcode')

  def testValueSetFromUrl_withUnknownUrl_raisesError(self):
    with self.assertRaises(ValueError):
      get_resolver().value_set_from_url('http://hl7.org/fhir/ValueSet/mystery')

  def testValueSetFromUrl_withWrongResourceType_raisesError(self):
    with unittest.mock.patch.object(get_resolver().package_manager,
                                    'get_resource') as m_get_resource:
      m_get_resource.return_value = (
          structure_definition_pb2.StructureDefinition())
      with self.assertRaises(ValueError):
        get_resolver().value_set_from_url(
            'http://hl7.org/fhir/ValueSet/mystery')

  def testValueSetFromUrl_withVersionedUrl_findsValueSet(self):
    value_set = get_resolver().value_set_from_url(
        'http://hl7.org/fhir/ValueSet/financial-taskcode|4.0.1')
    self.assertIsNotNone(value_set)
    self.assertEqual(value_set.url.value,
                     'http://hl7.org/fhir/ValueSet/financial-taskcode')

  def testValueSetFromUrl_withBadVersionedUrl_raisesError(self):
    with self.assertRaises(ValueError):
      get_resolver().value_set_from_url(
          'http://hl7.org/fhir/ValueSet/financial-taskcode|500.0.1')

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

    result = get_resolver().value_sets_from_structure_definition(definition)
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
        list(get_resolver().value_sets_from_structure_definition(definition)),
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

    result = get_resolver().value_sets_from_fhir_package(package)

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
        list(get_resolver().value_sets_from_fhir_package(package)), [])

  def testValueSetToInsertStatement_withValueSet_buildsValidQuery(self):
    value_set = value_set_pb2.ValueSet()
    value_set.url.value = 'vs-url'
    value_set.version.value = 'vs-version'

    for code, system in (('c1', 's1'), ('c2', 's2')):
      coding = value_set.expansion.contains.add()
      coding.code.value = code
      coding.system.value = system

    table = build_valueset_codes_table()

    query = value_sets.valueset_codes_insert_statement_for(value_set, table)
    query_string = str(query.compile(compile_kwargs={'literal_binds': True}))
    self.assertEqual(query_string, (
        'INSERT INTO valueset_codes (valueseturi, valuesetversion, system, code) '
        'SELECT codes.valueseturi, codes.valuesetversion, codes.system, codes.code '
        '\nFROM ('
        "SELECT 'vs-url' AS valueseturi, 'vs-version' AS valuesetversion, 's1' AS system, 'c1' AS code "
        'UNION ALL '
        "SELECT 'vs-url' AS valueseturi, 'vs-version' AS valuesetversion, 's2' AS system, 'c2' AS code"
        ') AS codes '
        'LEFT OUTER JOIN valueset_codes '
        'ON codes.valueseturi = valueset_codes.valueseturi '
        'AND codes.valuesetversion = valueset_codes.valuesetversion '
        'AND codes.system = valueset_codes.system '
        'AND codes.code = valueset_codes.code '
        '\nWHERE valueset_codes.valueseturi IS NULL '
        'AND valueset_codes.valuesetversion IS NULL '
        'AND valueset_codes.system IS NULL '
        'AND valueset_codes.code IS NULL'))

  def testValueSetToInsertStatement_withEmptyCodes_raisesError(self):
    value_set = value_set_pb2.ValueSet()
    table = build_valueset_codes_table()

    with self.assertRaises(ValueError):
      value_sets.valueset_codes_insert_statement_for(value_set, table)

  def testValueSetExpansionForExtensionalSets_withExtensionalSet_expandsCodes(
      self):
    value_set = value_set_pb2.ValueSet()

    # Add an include set with three codes.
    include_1 = value_set.compose.include.add()
    include_1.version.value = 'include-version-1'
    include_1.system.value = 'include-system-1'

    code_1_1 = include_1.concept.add()
    code_1_1.code.value = 'code-1-1'

    code_1_2 = include_1.concept.add()
    code_1_2.code.value = 'code-1-2'

    code_1_3 = include_1.concept.add()
    code_1_3.code.value = 'code-1-3'

    # Add an include set with one code.
    include_2 = value_set.compose.include.add()
    include_2.version.value = 'include-version-2'
    include_2.system.value = 'include-system-2'

    code_2_1 = include_2.concept.add()
    code_2_1.code.value = 'code-2-1'

    # Add a copy of code_1_3 to the exclude set.
    exclude = value_set.compose.exclude.add()
    exclude.version.value = 'include-version-1'
    exclude.system.value = 'include-system-1'
    exclude_code = exclude.concept.add()
    exclude_code.code.value = 'code-1-3'

    result = value_sets._expand_extensional_value_set(value_set)
    expected = [
        value_set_pb2.ValueSet.Expansion.Contains(
            system=datatypes_pb2.Uri(value='include-system-1'),
            version=datatypes_pb2.String(value='include-version-1'),
            code=datatypes_pb2.Code(value='code-1-1'),
        ),
        value_set_pb2.ValueSet.Expansion.Contains(
            system=datatypes_pb2.Uri(value='include-system-1'),
            version=datatypes_pb2.String(value='include-version-1'),
            code=datatypes_pb2.Code(value='code-1-2'),
        ),
        value_set_pb2.ValueSet.Expansion.Contains(
            system=datatypes_pb2.Uri(value='include-system-2'),
            version=datatypes_pb2.String(value='include-version-2'),
            code=datatypes_pb2.Code(value='code-2-1'),
        ),
    ]
    self.assertCountEqual(result.expansion.contains, expected)

  def testValueSetExpansionForExtensionalSets_withIntensionalSet_raisesError(
      self):
    value_set = value_set_pb2.ValueSet()

    include = value_set.compose.include.add()
    filter_ = include.filter.add()
    filter_.op.value = 1
    filter_.value.value = 'medicine'
    self.assertIsNone(value_sets._expand_extensional_value_set(value_set))

  def testConceptSetToExpansion_wtihConceptSet_buildsExpansion(self):
    concept_set = value_set_pb2.ValueSet.Compose.ConceptSet()
    concept_set.system.value = 'system'
    concept_set.version.value = 'version'

    code_1 = concept_set.concept.add()
    code_1.code.value = 'code_1'
    code_1.display.value = 'display_1'
    designation_1 = code_1.designation.add()
    designation_1.value.value = 'doing great'

    code_2 = concept_set.concept.add()
    code_2.code.value = 'code_2'

    result = value_sets._concept_set_to_expansion(concept_set)

    expected = [
        value_set_pb2.ValueSet.Expansion.Contains(),
        value_set_pb2.ValueSet.Expansion.Contains(),
    ]
    expected[0].system.value = 'system'
    expected[0].version.value = 'version'
    expected[0].code.value = 'code_1'
    expected[0].display.value = 'display_1'
    expected_designation = expected[0].designation.add()
    expected_designation.value.value = 'doing great'

    expected[1].system.value = 'system'
    expected[1].version.value = 'version'
    expected[1].code.value = 'code_2'

    self.assertCountEqual(result, expected)

  @unittest.mock.patch.object(
      value_sets, '_expand_extensional_value_set', autospec=True)
  def testExpandValueSet_withIntensionalValueSet_makesExpectedCalls(
      self, mock_expand_extensional_value_set):
    mock_expand_extensional_value_set.return_value = None

    expanded_value_set = value_set_pb2.ValueSet()
    codes = [
        value_set_pb2.ValueSet.Expansion.Contains(
            system=datatypes_pb2.Uri(value='include-system-1'),
            version=datatypes_pb2.String(value='include-version-1'),
            code=datatypes_pb2.Code(value='code-1-2'),
        ),
        value_set_pb2.ValueSet.Expansion.Contains(
            system=datatypes_pb2.Uri(value='include-system-2'),
            version=datatypes_pb2.String(value='include-version-2'),
            code=datatypes_pb2.Code(value='code-2-1'),
        ),
    ]
    expanded_value_set.expansion.contains.extend(codes)

    mock_client = unittest.mock.MagicMock(
        spec=terminology_service_client.TerminologyServiceClient)
    mock_client.expand_value_set.return_value = expanded_value_set

    value_set = value_set_pb2.ValueSet()
    result = value_sets.expand_value_set(value_set, mock_client)
    self.assertCountEqual(result.expansion.contains, codes)


def build_valueset_codes_table() -> sqlalchemy.sql.expression.TableClause:
  return sqlalchemy.table(
      'valueset_codes',
      sqlalchemy.column('valueseturi'),
      sqlalchemy.column('valuesetversion'),
      sqlalchemy.column('system'),
      sqlalchemy.column('code'),
  )


def get_resolver() -> value_sets.ValueSetResolver:
  """Build a ValueSetResolver for the common core package."""
  # TODO upgrade version of absl in the testing environment so we
  # can reference absl.testing.absltest.TEST_SRCDIR instead.
  package_filepath = os.path.join(
      FLAGS.test_srcdir, 'com_google_fhir/spec/fhir_r4_package.zip')
  package_manager = fhir_package.FhirPackageManager()
  package_manager.add_package_at_path(package_filepath)

  return value_sets.ValueSetResolver(package_manager)


if __name__ == '__main__':
  absltest.main()
