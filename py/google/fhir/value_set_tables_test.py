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
"""Test value_set_tables functionality."""

import sqlalchemy

from google.fhir import value_set_tables
from absl.testing import absltest
from proto.google.fhir.proto.r4.core.resources import value_set_pb2


class ValueSetsTest(absltest.TestCase):

  def testValueSetToInsertStatement_withValueSet_buildsValidQuery(self):
    value_set = value_set_pb2.ValueSet()
    value_set.url.value = 'vs-url'
    value_set.version.value = 'vs-version'

    for code, system in (('c1', 's1'), ('c2', 's2'), ('c3', 's3')):
      coding = value_set.expansion.contains.add()
      coding.code.value = code
      coding.system.value = system

    table = build_valueset_codes_table()

    result = value_set_tables.valueset_codes_insert_statement_for([value_set],
                                                                  table)
    query = list(result)[0]
    query_string = str(query.compile(compile_kwargs={'literal_binds': True}))
    self.assertEqual(query_string, (
        'INSERT INTO valueset_codes (valueseturi, valuesetversion, system, code) '
        'SELECT codes.valueseturi, codes.valuesetversion, codes.system, codes.code '
        '\nFROM ('
        "SELECT 'vs-url' AS valueseturi, 'vs-version' AS valuesetversion, 's1' AS system, 'c1' AS code "
        'UNION ALL '
        "SELECT 'vs-url' AS valueseturi, 'vs-version' AS valuesetversion, 's2' AS system, 'c2' AS code "
        'UNION ALL '
        "SELECT 'vs-url' AS valueseturi, 'vs-version' AS valuesetversion, 's3' AS system, 'c3' AS code"
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

  def testValueSetToInsertStatement_withBatches_buildsBatchedInserts(self):
    value_set = value_set_pb2.ValueSet()
    value_set.url.value = 'vs-url'
    value_set.version.value = 'vs-version'

    for code, system in (('c1', 's1'), ('c2', 's2'), ('c3', 's3')):
      coding = value_set.expansion.contains.add()
      coding.code.value = code
      coding.system.value = system

    table = build_valueset_codes_table()

    result = value_set_tables.valueset_codes_insert_statement_for([value_set],
                                                                  table,
                                                                  batch_size=2)
    expected_1 = (
        'INSERT INTO valueset_codes '
        '(valueseturi, valuesetversion, system, code) '
        'SELECT '
        'codes.valueseturi, codes.valuesetversion, codes.system, codes.code '
        '\nFROM ('
        "SELECT 'vs-url' AS valueseturi, 'vs-version' AS valuesetversion, "
        "'s1' AS system, 'c1' AS code "
        'UNION ALL '
        "SELECT 'vs-url' AS valueseturi, 'vs-version' AS valuesetversion, "
        "'s2' AS system, 'c2' AS code"
        ') AS codes '
        'LEFT OUTER JOIN valueset_codes '
        'ON codes.valueseturi = valueset_codes.valueseturi '
        'AND codes.valuesetversion = valueset_codes.valuesetversion '
        'AND codes.system = valueset_codes.system '
        'AND codes.code = valueset_codes.code '
        '\nWHERE valueset_codes.valueseturi IS NULL '
        'AND valueset_codes.valuesetversion IS NULL '
        'AND valueset_codes.system IS NULL '
        'AND valueset_codes.code IS NULL')

    expected_2 = (
        'INSERT INTO valueset_codes '
        '(valueseturi, valuesetversion, system, code) '
        'SELECT '
        'codes.valueseturi, codes.valuesetversion, codes.system, codes.code '
        '\nFROM ('
        "SELECT 'vs-url' AS valueseturi, 'vs-version' AS valuesetversion, "
        "'s3' AS system, 'c3' AS code"
        ') AS codes '
        'LEFT OUTER JOIN valueset_codes '
        'ON codes.valueseturi = valueset_codes.valueseturi '
        'AND codes.valuesetversion = valueset_codes.valuesetversion '
        'AND codes.system = valueset_codes.system '
        'AND codes.code = valueset_codes.code '
        '\nWHERE valueset_codes.valueseturi IS NULL '
        'AND valueset_codes.valuesetversion IS NULL '
        'AND valueset_codes.system IS NULL '
        'AND valueset_codes.code IS NULL')

    result_queries = [
        str(query.compile(compile_kwargs={'literal_binds': True}))
        for query in result
    ]
    self.assertListEqual(result_queries, [expected_1, expected_2])

  def testValueSetToInsertStatement_withEmptyValues_rendersNulls(self):
    value_set = value_set_pb2.ValueSet()
    value_set.url.value = 'vs-url'

    coding = value_set.expansion.contains.add()
    coding.code.value = 'code'

    table = build_valueset_codes_table()

    result = value_set_tables.valueset_codes_insert_statement_for([value_set],
                                                                  table)
    query = list(result)[0]
    query_string = str(query.compile(compile_kwargs={'literal_binds': True}))
    self.assertEqual(
        query_string,
        ('INSERT INTO valueset_codes '
         '(valueseturi, valuesetversion, system, code) '
         'SELECT '
         'codes.valueseturi, codes.valuesetversion, codes.system, codes.code \n'
         'FROM (SELECT '
         "'vs-url' AS valueseturi, "
         'NULL AS valuesetversion, '
         'NULL AS system, '
         "'code' AS code"
         ') AS codes '
         'LEFT OUTER JOIN valueset_codes ON '
         'codes.valueseturi = valueset_codes.valueseturi AND '
         'codes.valuesetversion = valueset_codes.valuesetversion AND '
         'codes.system = valueset_codes.system AND '
         'codes.code = valueset_codes.code \n'
         'WHERE '
         'valueset_codes.valueseturi IS NULL AND '
         'valueset_codes.valuesetversion IS NULL AND '
         'valueset_codes.system IS NULL AND '
         'valueset_codes.code IS NULL'))


def build_valueset_codes_table() -> sqlalchemy.sql.expression.TableClause:
  return sqlalchemy.table(
      'valueset_codes',
      sqlalchemy.column('valueseturi'),
      sqlalchemy.column('valuesetversion'),
      sqlalchemy.column('system'),
      sqlalchemy.column('code'),
  )


if __name__ == '__main__':
  absltest.main()
